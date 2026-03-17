import asyncio
import os
import pymysql
import httpx
from datetime import datetime, timedelta
import time

# --- [설정] 기상청 API 및 DB 연결 정보 ---
# [복구] 사용자가 제공한 실제 API 키를 적용했습니다.
SERVICE_KEY = os.getenv("KMA_SERVICE_KEY", "OJfd9VikT06X3fVYpC9OkQ")
DB_HOST = os.getenv("DB_HOST", "mariadb")
DB_USER = os.getenv("DB_USER", "root")
DB_PASS = os.getenv("DB_PASSWORD", "rootsecret")
DB_NAME = os.getenv("DB_NAME", "smartfarm")

class KMAWeatherFetcher:
    """
    기상청 단기예보/초단기실황 API 페처
    (WEATHER_LIB_MANUAL.md 의 로직을 기반으로 구현됨)
    """
    def __init__(self, service_key, nx=55, ny=127):
        self.service_key = service_key
        self.nx = nx
        self.ny = ny
        self.base_url = "http://apis.data.go.kr/1360000/VilageFcstInfoService_2.0"

    def get_base_time(self, type="obs"):
        now = datetime.now()
        # 실황은 매시 40분 업데이트, 예보는 매시 45분 업데이트
        if type == "obs":
            if now.minute < 40:
                now = now - timedelta(hours=1)
            return now.strftime("%Y%m%d"), now.strftime("%H00")
        else: # ultra_fcst
            if now.minute < 45:
                now = now - timedelta(hours=1)
            return now.strftime("%Y%m%d"), now.strftime("%H30")

    async def fetch_obs(self):
        date, time = self.get_base_time("obs")
        url = f"{self.base_url}/getUltraSrtNcst"
        params = {
            "serviceKey": self.service_key,
            "pageNo": 1,
            "numOfRows": 10,
            "dataType": "JSON",
            "base_date": date,
            "base_time": time,
            "nx": self.nx,
            "ny": self.ny
        }
        async with httpx.AsyncClient() as client:
            try:
                res = await client.get(url, params=params, timeout=10.0)
                if res.status_code == 200:
                    data = res.json()
                    items = data.get("response", {}).get("body", {}).get("items", {}).get("item", [])
                    obs = {}
                    for item in items:
                        mapping = {"T1H": "temperature", "REH": "humidity", "WSD": "wind_speed", "VEC": "wind_direction", "RN1": "rainfall"}
                        if item["category"] in mapping:
                            val = float(item["obsrValue"])
                            if item["category"] == "VEC":
                                directions = ["N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW", "N"]
                                val = directions[int((val + 11.25) / 22.5)]
                            obs[mapping[item["category"]]] = val
                    return obs
            except Exception as e:
                print(f"[KMA] 실황 데이터 페치 오류: {e}")
        return None

    async def fetch_fcst(self):
        date, time = self.get_base_time("fcst")
        url = f"{self.base_url}/getUltraSrtFcst"
        params = {
            "serviceKey": self.service_key,
            "pageNo": 1,
            "numOfRows": 60,
            "dataType": "JSON",
            "base_date": date,
            "base_time": time,
            "nx": self.nx,
            "ny": self.ny
        }
        async with httpx.AsyncClient() as client:
            try:
                res = await client.get(url, params=params, timeout=10.0)
                if res.status_code == 200:
                    data = res.json()
                    items = data.get("response", {}).get("body", {}).get("items", {}).get("item", [])
                    # 1~6시간 예보 추출 로직 (단순화)
                    fcsts = {}
                    for item in items:
                        ftime = item["fcstTime"]
                        if ftime not in fcsts: fcsts[ftime] = {"forecast_offset": 0}
                        # 현재 시간 대비 오프셋 계산 (대략적)
                        mapping = {"T1H": "temperature", "REH": "humidity", "WSD": "wind_speed", "VEC": "wind_direction", "RN1": "rainfall"}
                        if item["category"] in mapping:
                            val = float(item["fcstValue"])
                            if item["category"] == "VEC":
                                directions = ["N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW", "N"]
                                val = directions[int((val + 11.25) / 22.5)]
                            fcsts[ftime][mapping[item["category"]]] = val
                    
                    # 오프셋 할당 및 리스트 변환
                    sorted_times = sorted(fcsts.keys())
                    result_list = []
                    for i, t in enumerate(sorted_times[:6]):
                        fcsts[t]["forecast_offset"] = i + 1
                        result_list.append(fcsts[t])
                    return result_list
            except Exception as e:
                print(f"[KMA] 예보 데이터 페치 오류: {e}")
        return []

async def main():
    print("=== 기상청 날씨 데이터 페처 시작 (Smart Farm Intelligence OS) ===")
    
    fetcher = KMAWeatherFetcher(service_key=SERVICE_KEY)
    
    while True:
        try:
            print(f"[{datetime.now()}] 업데이트 시도 중...")
            
            # DB 연결
            conn = pymysql.connect(host=DB_HOST, user=DB_USER, password=DB_PASS, database=DB_NAME, autocommit=True)
            cursor = conn.cursor()
            
            # 1. 실황 페치 및 저장
            obs = await fetcher.fetch_obs()
            if obs:
                cursor.execute(
                    "INSERT INTO weather_data (source, forecast_offset, wind_speed, wind_direction, rainfall, temperature, humidity) "
                    "VALUES ('KMA', 0, %s, %s, %s, %s, %s)",
                    (obs.get('wind_speed'), obs.get('wind_direction'), obs.get('rainfall'), obs.get('temperature'), obs.get('humidity'))
                )
                print("  -> 실황 데이터 저장 완료")

            # 2. 예보 페치 및 저장
            fcsts = await fetcher.fetch_fcst()
            for f in fcsts:
                cursor.execute(
                    "INSERT INTO weather_data (source, forecast_offset, wind_speed, wind_direction, rainfall, temperature, humidity) "
                    "VALUES ('KMA', %s, %s, %s, %s, %s, %s)",
                    (f['forecast_offset'], f.get('wind_speed'), f.get('wind_direction'), f.get('rainfall'), f.get('temperature'), f.get('humidity'))
                )
            if fcsts: print(f"  -> {len(fcsts)}개의 예보 데이터 저장 완료")
            
            conn.close()
            
        except Exception as e:
            print(f"!!! 루프 오류 발생: {e}")
            
        # 1시간마다 반복 (기상청 데이터는 시간 단위 업데이트)
        await asyncio.sleep(3600)

if __name__ == "__main__":
    asyncio.run(main())
