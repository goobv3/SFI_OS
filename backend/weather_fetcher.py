import asyncio
import os
import pymysql
import httpx
from datetime import datetime, timedelta
import time

# --- [설정] 기상청 API 및 DB 연결 정보 ---
# [복구] 사용자가 제공한 API Hub 전용 키와 위치 정보를 적용했습니다.
# 위치: 경기도 이천시 백사면 상용리 531-8 (근처 위경도 적용)
AUTH_KEY = os.getenv("KMA_SERVICE_KEY", "OJfd9VikT06X3fVYpC9OkQ")
LAT = 37.3390  # 위도
LON = 127.4907 # 경도

DB_HOST = os.getenv("DB_HOST", "mariadb")
DB_USER = os.getenv("DB_USER", "farmuser")  # docker-compose 설정 반영
DB_PASS = os.getenv("DB_PASSWORD", "farmsecret")
DB_NAME = os.getenv("DB_NAME", "smartfarm")

class KMAApiHubFetcher:
    """
    기상청 API Hub (apihub.kma.go.kr) 특정지점 다중요소 API 페처
    """
    def __init__(self, auth_key, lat, lon):
        self.auth_key = auth_key
        self.lat = lat
        self.lon = lon
        self.base_url = "https://apihub.kma.go.kr/api/typ01/url/sfc_nc_var.php"

    async def fetch_current(self):
        """
        특정 지점의 현재 기상 요소를 가져옵니다.
        """
        now = datetime.now()
        tm = now.strftime("%Y%m%d%H%M")
        
        params = {
            "tm1": tm,
            "tm2": tm,
            "lat": self.lat,
            "lon": self.lon,
            "authKey": self.auth_key,
            "help": 0
        }
        
        async with httpx.AsyncClient() as client:
            try:
                # API Hub는 보안 정책상 https를 권장하며, 데이터는 텍스트(CSV 형태)로 반환될 수 있습니다.
                res = await client.get(self.base_url, params=params, timeout=15.0)
                if res.status_code == 200:
                    text = res.text
                    print(f"[KMA Hub] 응답 수신:\n{text[:200]}...") # 로그 확인용
                    
                    # API Hub 응답 파싱 (일반적으로 '#'으로 시작하는 주석 매칭 후 데이터 줄 파싱)
                    lines = [l.strip() for l in text.split('\n') if l.strip() and not l.startswith('#')]
                    if not lines:
                        print("[KMA Hub] 유효한 데이터 줄이 없습니다.")
                        return None
                    
                    # 데이터 예시: tm, ta, td, hm, ws_10m, rn_15m, rn_60m, rn_day, vs, pa, ps
                    fields = [f.strip() for f in lines[0].split(',')]
                    
                    result = {
                        "temperature": None,
                        "humidity": None,
                        "wind_speed": None,
                        "wind_direction": None,
                        "rainfall": 0.0
                    }
                    
                    try:
                        # tm:0, ta:1, td:2, hm:3, ws:4, rn_15:5...
                        if len(fields) > 1:
                            result["temperature"] = float(fields[1])
                        if len(fields) > 3:
                            result["humidity"] = float(fields[3])
                        if len(fields) > 4:
                            result["wind_speed"] = float(fields[4])
                        if len(fields) > 5:
                            result["rainfall"] = float(fields[5])
                    except Exception as e:
                        print(f"[KMA Hub] 데이터 파싱 오류: {e}")
                        
                    return result
            except Exception as e:
                print(f"[KMA Hub] 페치 오류: {e}")
        return None

async def main():
    print("=== 기상청 API Hub 데이터 페처 시작 (이천시 백사면) ===")
    
    fetcher = KMAApiHubFetcher(auth_key=AUTH_KEY, lat=LAT, lon=LON)
    
    while True:
        try:
            print(f"[{datetime.now()}] 업데이트 시도 중 (Location: {LAT}, {LON})...")
            
            # DB 연결
            conn = pymysql.connect(host=DB_HOST, user=DB_USER, password=DB_PASS, database=DB_NAME, autocommit=True)
            cursor = conn.cursor()
            
            # 실황 데이터 가져오기
            obs = await fetcher.fetch_current()
            if obs and (obs['temperature'] is not None):
                # 대시보드의 다양한 예보 시점(0:현재, 1, 3, 6시간 후)을 위해 데이터 저장
                # (현 API는 예보 기능이 없으므로 현재 값을 모든 시점에 채워넣음)
                for offset in [0, 1, 3, 6]:
                    cursor.execute(
                        "INSERT INTO weather_data (source, forecast_offset, wind_speed, wind_direction, rainfall, temperature, humidity, timestamp) "
                        "VALUES ('KMA', %s, %s, %s, %s, %s, %s, %s)",
                        (offset, obs.get('wind_speed'), obs.get('wind_direction'), obs.get('rainfall'), obs.get('temperature'), obs.get('humidity'), datetime.now())
                    )
                print(f"  -> 데이터 저장 완료 (Offsets: 0, 1, 3, 6): 기온 {obs['temperature']}°C / 습도 {obs['humidity']}%")
            else:
                print("  -> 유효한 데이터를 가져오지 못했습니다.")
            
            conn.close()
            
        except Exception as e:
            print(f"!!! 루프 오류 발생: {e}")
            
        # 1시간마다 반복
        await asyncio.sleep(3600)

if __name__ == "__main__":
    asyncio.run(main())
