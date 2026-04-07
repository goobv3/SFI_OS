"""
@file weather_fetcher.py
@brief KMA(기상청) 기상 데이터 백그라운드 수집 데몬 프로세스 스크립트.

[역할 및 아키텍처]
- 이 스크립트는 C++ 백엔드 서버와 별개로, 정기적으로 기상청 API에 접속해 데이터를 가져와 MariaDB에 적재합니다.
- C++ 백엔드가 실시간 통신 오버헤드를 줄일 수 있도록 도와줍니다.
- asyncio와 httpx를 사용하여 비동기적으로 빠르고 안정적으로 외부 HTTP 통신을 수행합니다.

[환경변수]
- KMA_HUB_AUTH_KEY : 기상자료개방포털 API 키
- KMA_SERVICE_KEY : 공공데이터포털(단기예보) API 키
- DB 설정 (DB_HOST, DB_USER, DB_PASSWORD, DB_NAME)
"""
import asyncio
import os
import pymysql
import httpx
import math
from datetime import datetime, timedelta
import time

# --- [설정] 기상청 API 및 DB 연결 정보 ---
AUTH_KEY_HUB = os.getenv("KMA_HUB_AUTH_KEY", "OJfd9VikT06X3fVYpC9OkQ")
AUTH_KEY_DATA = os.getenv("KMA_SERVICE_KEY", "3b1b77976ce0ca80247d4cd707834cff148c26d257a9b03826dda6829b75f0ad")
LAT = 37.3390  # 위도 (이천시 백사면 근처)
LON = 127.4907 # 경도

DB_HOST = os.getenv("DB_HOST", "mariadb")
DB_USER = os.getenv("DB_USER", "farmuser")
DB_PASS = os.getenv("DB_PASSWORD", "farmsecret")
DB_NAME = os.getenv("DB_NAME", "smartfarm")

def convert_grid(lat, lon):
    """
    [기상청 공식 위경도 -> XY 격자 변환 함수]
    WGS84 위도/경도를 기상청의 LCC(Lambert Conformal Conic) 격자망 좌표(X, Y)로 변환합니다.
    초단기 및 단기예보 OpenAPI 호출 시 반드시 이 격자 좌표를 파라미터로 사용해야 합니다.
    """
    RE = 6371.00877
    GRID = 5.0
    SLAT1 = 30.0
    SLAT2 = 60.0
    OLON = 126.0
    OLAT = 38.0
    XO = 43
    YO = 136

    DEGRAD = math.pi / 180.0
    re = RE / GRID
    slat1 = SLAT1 * DEGRAD
    slat2 = SLAT2 * DEGRAD
    olon = OLON * DEGRAD
    olat = OLAT * DEGRAD

    sn = math.tan(math.pi * 0.25 + slat2 * 0.5) / math.tan(math.pi * 0.25 + slat1 * 0.5)
    sn = math.log(math.cos(slat1) / math.cos(slat2)) / math.log(sn)
    sf = math.tan(math.pi * 0.25 + slat1 * 0.5)
    sf = math.pow(sf, sn) * math.cos(slat1) / sn
    ro = math.tan(math.pi * 0.25 + olat * 0.5)
    ro = re * sf / math.pow(ro, sn)
    
    ra = math.tan(math.pi * 0.25 + lat * DEGRAD * 0.5)
    ra = re * sf / math.pow(ra, sn)
    theta = lon * DEGRAD - olon
    if theta > math.pi: theta -= 2.0 * math.pi
    if theta < -math.pi: theta += 2.0 * math.pi
    theta *= sn
    
    nx = math.floor(ra * math.sin(theta) + XO + 0.5)
    ny = math.floor(ro - ra * math.cos(theta) + YO + 0.5)
    return nx, ny

class KMAApiHubFetcher:
    """
    기상청 API Hub (apihub.kma.go.kr) 특정지점 다중요소 API 페처 (실시간 관측 전용)
    """
    def __init__(self, auth_key, lat, lon):
        self.auth_key = auth_key
        self.lat = lat
        self.lon = lon
        self.base_url = "https://apihub.kma.go.kr/api/typ01/url/sfc_nc_var.php"

    async def fetch_current(self):
        kst_now = datetime.now() + timedelta(hours=9)
        query_time = kst_now - timedelta(minutes=15) # 조금 더 여유 있게
        tm = query_time.strftime("%Y%m%d%H%M")
        
        params = {"tm1": tm, "tm2": tm, "lat": self.lat, "lon": self.lon, "authKey": self.auth_key, "help": 0}
        async with httpx.AsyncClient() as client:
            try:
                res = await client.get(self.base_url, params=params, timeout=15.0)
                if res.status_code == 200:
                    lines = [l.strip() for l in res.text.split('\n') if l.strip()]
                    data_lines = [l for l in lines if not l.startswith('#')]
                    if not data_lines: data_lines = [l.replace('#7777END', '').strip() for l in lines if l.startswith('#7777END')]
                    if not data_lines: return None
                    fields = [f.strip() for f in data_lines[0].split(',')]
                    result = {"temperature": None, "humidity": None, "wind_speed": None, "wind_direction": None, "rainfall": 0.0}
                    # tm:0, ta:1, td:2, hm:3, ws:4, rn_15:5, rn_60:6...
                    if len(fields) > 1: result["temperature"] = float(fields[1])
                    if len(fields) > 3: result["humidity"] = float(fields[3])
                    if len(fields) > 4: result["wind_speed"] = float(fields[4])
                    # [수정] rn_15(15분 강수) 대신 rn_60(60분 강수)를 사용하여 사용자 체감에 맞게 조정
                    if len(fields) > 6: result["rainfall"] = float(fields[6])
                    elif len(fields) > 5: result["rainfall"] = float(fields[5])
                    return result
            except Exception as e: print(f"[KMA Hub] Error: {e}")
        return None

class KMAShortTermForecastFetcher:
    """
    공공데이터포털 초단기예보 API 페처
    """
    def __init__(self, service_key, nx, ny):
        self.service_key = service_key
        self.nx = nx
        self.ny = ny
        self.base_url = "http://apis.data.go.kr/1360000/VilageFcstInfoService_2.0/getUltraSrtFcst"

    def parse_fcst_value(self, category, val_str):
        """
        기상청 특유의 문자열 포함 측정값(예: "1.0mm", "1mm 미만", "강수없음")을 숫자로 변환합니다.
        """
        if category == 'RN1':
            if '강수없음' in val_str: return 0.0
            if '미만' in val_str: return 0.1 # "1mm 미만" -> 0.1
            return float(val_str.replace('mm', '').strip())
        try:
            return float(val_str)
        except:
            return None

    async def fetch_forecasts(self):
        kst_now = datetime.now() + timedelta(hours=9)
        # 초단기 예보는 매시간 45분 발표, 30분에 데이터 생성됨
        if kst_now.minute < 45:
            base_time_dt = kst_now - timedelta(hours=1)
        else:
            base_time_dt = kst_now
        
        base_date = base_date = base_time_dt.strftime("%Y%m%d")
        base_time = base_time_dt.strftime("%H00")
        
        params = {
            "serviceKey": self.service_key,
            "numOfRows": 100,
            "pageNo": 1,
            "dataType": "JSON",
            "base_date": base_date,
            "base_time": base_time,
            "nx": self.nx,
            "ny": self.ny
        }
        
        async with httpx.AsyncClient() as client:
            try:
                res = await client.get(self.base_url, params=params, timeout=15.0)
                if res.status_code == 200:
                    data = res.json()
                    if data['response']['header']['resultCode'] == '00':
                        items = data['response']['body']['items']['item']
                        forecasts = {}
                        
                        for item in items:
                            fcst_time_str = item['fcstDate'] + item['fcstTime']
                            fcst_dt = datetime.strptime(fcst_time_str, "%Y%m%d%H%M")
                            # 기준 시간(정시)으로부터 몇 시간 뒤인지 계산
                            ref_dt = base_time_dt.replace(minute=0, second=0, microsecond=0)
                            offset = int((fcst_dt - ref_dt).total_seconds() // 3600)
                            
                            if offset not in [1, 3, 6]: continue
                            if offset not in forecasts: forecasts[offset] = {}
                            
                            category = item['category']
                            val = self.parse_fcst_value(category, item['fcstValue'])
                            
                            if category == 'T1H': forecasts[offset]['temperature'] = val
                            elif category == 'REH': forecasts[offset]['humidity'] = val
                            elif category == 'WSD': forecasts[offset]['wind_speed'] = val
                            elif category == 'VEC': forecasts[offset]['wind_direction'] = val
                            elif category == 'RN1': forecasts[offset]['rainfall'] = val
                        
                        return forecasts
            except Exception as e: print(f"[KMA Forecast] Error: {e}")
        return None

async def main():
    print("=== 기상청 통합 데이터 페처 시작 (관측+예보) ===")
    
    nx, ny = convert_grid(LAT, LON)
    print(f"위치 격자: nx={nx}, ny={ny}")
    
    hub_fetcher = KMAApiHubFetcher(auth_key=AUTH_KEY_HUB, lat=LAT, lon=LON)
    fcst_fetcher = KMAShortTermForecastFetcher(service_key=AUTH_KEY_DATA, nx=nx, ny=ny)
    
    while True:
        try:
            kst = datetime.now() + timedelta(hours=9)
            print(f"[{kst}] 데이터 업데이트 중...")
            
            conn = pymysql.connect(host=DB_HOST, user=DB_USER, password=DB_PASS, database=DB_NAME, autocommit=True)
            cursor = conn.cursor()
            
            # 1. 실시간 관측 (offset 0)
            obs = await hub_fetcher.fetch_current()
            if obs and (obs['temperature'] is not None):
                cursor.execute(
                    "INSERT INTO weather_data (source, forecast_offset, wind_speed, wind_direction, rainfall, temperature, humidity, timestamp) "
                    "VALUES ('KMA', 0, %s, %s, %s, %s, %s, %s)",
                    (obs.get('wind_speed'), obs.get('wind_direction'), obs.get('rainfall'), obs.get('temperature'), obs.get('humidity'), kst)
                )
                print(f"  -> 관측 저장 완료: {obs['temperature']}°C")
            
            # [수정] 예보 데이터(offset 1, 3, 6)는 이제 DB에 저장하지 않습니다.
            # C++ 백엔드에서 사용자 요청 시 실시간으로 가져오도록 변경되었습니다.
            
            conn.close()
        except Exception as e:
            print(f"!!! Error in loop: {e}")
        
        await asyncio.sleep(300) # 5분 주기로 변경

if __name__ == "__main__":
    asyncio.run(main())
