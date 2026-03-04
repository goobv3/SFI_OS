import asyncio
import httpx
from datetime import datetime, timedelta, timezone

# ---------------------------------------------------------
# 기상청(KMA) 날씨 정보 Fetch 모듈 (Weather Library)
# ---------------------------------------------------------
# 이 모듈은 기상청 API(초단기실황, 초단기예보)와 통신하여
# 현재 날씨와 향후 1~6시간 예보 데이터를 파이썬 딕셔너리로 깔끔하게 가공하여 반환합니다.
# 데이터베이스 의존성 없이 독립적으로 사용할 수 있는 범용 라이브러리입니다.
# ---------------------------------------------------------

class KMAWeatherFetcher:
    def __init__(self, service_key: str, nx: int = 55, ny: int = 127):
        """
        :param service_key: 기상청 API 발급 키 (디코딩 혹은 인코딩된 원형 문자열)
        :param nx: X 좌표값 (기본값: 서울 55)
        :param ny: Y 좌표값 (기본값: 서울 127)
        """
        self.service_key = service_key
        self.nx = nx
        self.ny = ny
        self.ncst_url = "https://apis.data.go.kr/1360000/VilageFcstInfoService_2.0/getUltraSrtNcst"
        self.fcst_url = "https://apis.data.go.kr/1360000/VilageFcstInfoService_2.0/getUltraSrtFcst"
        self.kst = timezone(timedelta(hours=9)) # 한국 표준시(KST)

    def _get_base_time_ncst(self):
        """ 초단기실황(NCST) API 호출을 위한 기준 시간(Base Time) 계산 """
        now = datetime.now(self.kst)
        base_date = now.strftime("%Y%m%d")
        base_time = now.strftime("%H") + "00"
        if now.minute < 40:
            calc_hour = now.hour - 1
            if calc_hour < 0:
                calc_hour = 23
                base_date = (now - timedelta(days=1)).strftime("%Y%m%d")
            base_time = f"{calc_hour:02d}00"
        return base_date, base_time
        
    def _get_base_time_fcst(self):
        """ 초단기예보(FCST) API 호출을 위한 기준 시간(Base Time) 계산 """
        now = datetime.now(self.kst)
        base_date = now.strftime("%Y%m%d")
        base_time = now.strftime("%H") + "30"
        if now.minute < 45:
            calc_hour = now.hour - 1
            if calc_hour < 0:
                calc_hour = 23
                base_date = (now - timedelta(days=1)).strftime("%Y%m%d")
            base_time = f"{calc_hour:02d}30"
        return base_date, base_time

    async def fetch_all(self):
        """
        실황(Observation) 데이터와 예보(Forecast, 1~6시간) 데이터를 모두 가져와 딕셔너리로 반환합니다.
        반환 예:
        {
            "observation": { "temperature": 15.0, "humidity": 60, ... },
            "forecasts": [
                { "offset_hours": 1, "temperature": 16.0, ... },
                { "offset_hours": 2, "temperature": 17.0, ... }
            ]
        }
        """
        result = {"observation": None, "forecasts": []}
        
        ncst_date, ncst_time = self._get_base_time_ncst()
        fcst_date, fcst_time = self._get_base_time_fcst()

        async with httpx.AsyncClient(verify=False) as client:
            # 1. Fetch Observation
            resp_ncst = await client.get(self.ncst_url, params={
                "serviceKey": self.service_key, "pageNo": "1", "numOfRows": "1000",
                "dataType": "JSON", "base_date": ncst_date, "base_time": ncst_time, 
                "nx": str(self.nx), "ny": str(self.ny)
            }, timeout=10.0)
            
            if resp_ncst.status_code == 200:
                data = resp_ncst.json()
                items = data.get("response", {}).get("body", {}).get("items", {}).get("item", [])
                if items:
                    obs = {"temperature": None, "humidity": None, "wind_speed": None, "wind_direction": None, "rainfall": None}
                    for item in items:
                        cat = item.get("category")
                        val = item.get("obsrValue")
                        try:
                            fval = float(val) if val else None
                            if cat == "T1H": obs["temperature"] = fval
                            elif cat == "REH": obs["humidity"] = fval
                            elif cat == "WSD": obs["wind_speed"] = fval
                            elif cat == "RN1": obs["rainfall"] = fval
                            elif cat == "VEC" and fval is not None:
                                dirs = ['N', 'NNE', 'NE', 'ENE', 'E', 'ESE', 'SE', 'SSE', 'S', 'SSW', 'SW', 'WSW', 'W', 'WNW', 'NW', 'NNW', 'N']
                                obs["wind_direction"] = dirs[int((fval / 22.5) + 0.5) % 16]
                        except Exception: pass
                    result["observation"] = obs

            # 2. Fetch Forecast
            resp_fcst = await client.get(self.fcst_url, params={
                "serviceKey": self.service_key, "pageNo": "1", "numOfRows": "1000",
                "dataType": "JSON", "base_date": fcst_date, "base_time": fcst_time, 
                "nx": str(self.nx), "ny": str(self.ny)
            }, timeout=10.0)
            
            if resp_fcst.status_code == 200:
                data = resp_fcst.json()
                items = data.get("response", {}).get("body", {}).get("items", {}).get("item", [])
                if items:
                    raw_forecasts = {}
                    for item in items:
                        ftime = item.get("fcstTime")
                        fdate = item.get("fcstDate")
                        cat = item.get("category")
                        val = item.get("fcstValue")
                        
                        if ftime not in raw_forecasts:
                            raw_forecasts[ftime] = {"fcstDate": fdate, "temperature": None, "humidity": None, "wind_speed": None, "wind_direction": None, "rainfall": None}
                            
                        try:
                            if val == "강수없음": fval = 0.0
                            else:
                                try: fval = float(val) if val else None
                                except ValueError: fval = None
                            
                            if cat == "T1H": raw_forecasts[ftime]["temperature"] = fval
                            elif cat == "REH": raw_forecasts[ftime]["humidity"] = fval
                            elif cat == "WSD": raw_forecasts[ftime]["wind_speed"] = fval
                            elif cat == "RN1": raw_forecasts[ftime]["rainfall"] = fval
                            elif cat == "VEC" and fval is not None:
                                dirs = ['N', 'NNE', 'NE', 'ENE', 'E', 'ESE', 'SE', 'SSE', 'S', 'SSW', 'SW', 'WSW', 'W', 'WNW', 'NW', 'NNW', 'N']
                                raw_forecasts[ftime]["wind_direction"] = dirs[int((fval / 22.5) + 0.5) % 16]
                        except Exception: pass
                        
                    # Sort dynamically based on real chronological order
                    parsed_ftimes = []
                    for ftime, wd in raw_forecasts.items():
                        try:
                            dt_str = f'{wd["fcstDate"]} {ftime}'
                            actual_time = datetime.strptime(dt_str, "%Y%m%d %H%M")
                            parsed_ftimes.append((actual_time, wd))
                        except Exception: pass
                    
                    parsed_ftimes.sort(key=lambda x: x[0])
                    
                    # Store sorted forecasts and assign progressive offsets (1h to 6h)
                    offset = 1
                    for actual_time, wd in parsed_ftimes:
                        wd['offset_hours'] = offset
                        result["forecasts"].append(wd)
                        offset += 1
                        if offset > 6: break
                        
        return result
