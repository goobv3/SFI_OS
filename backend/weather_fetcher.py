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
DB_USER = os.getenv("DB_USER", "farmuser")
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

    def get_kst_now(self):
        """
        컨테이너가 UTC인 경우를 대비해 KST(UTC+9) 시간을 계산합니다.
        """
        return datetime.now() + timedelta(hours=9)

    async def fetch_current(self):
        """
        특정 지점의 현재 기상 요소를 가져옵니다.
        """
        # [수정] API Hub는 KST 기반이므로 UTC 컨테이너에서도 KST로 요청하도록 보정함
        kst_now = self.get_kst_now()
        
        # 실제 데이터가 업데이트되는 시간을 고려하여 5~10분 전 데이터 요청
        # (너무 현재 시간 정각이면 데이터가 아직 안 올라왔을 수 있음)
        query_time = kst_now - timedelta(minutes=10)
        tm = query_time.strftime("%Y%m%d%H%M")
        
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
                res = await client.get(self.base_url, params=params, timeout=15.0)
                if res.status_code == 200:
                    text = res.text
                    
                    # API Hub 응답 파싱
                    lines = [l.strip() for l in text.split('\n') if l.strip()]
                    # 데이터 라인은 보통 #으로 시작하지 않거나, #7777END 앞에 데이터가 있음
                    # help=0 인 경우 #7777END 뒤에 데이터가 오거나 형식이 다를 수 있음
                    # 실험 결과 sfc_nc_var.php는 데이터 줄이 #으로 시작하지 않는 줄이 데이터임
                    data_lines = [l for l in lines if not l.startswith('#')]
                    
                    if not data_lines:
                        # #7777END로 시작하는 줄에 데이터가 포함된 경우 처리
                        data_lines = [l.replace('#7777END', '').strip() for l in lines if l.startswith('#7777END')]
                    
                    if not data_lines:
                        print(f"[KMA Hub] {tm} 시점의 데이터가 유효하지 않습니다. 응답: {text[:50]}...")
                        return None
                        
                    fields = [f.strip() for f in data_lines[0].split(',')]
                    
                    result = {
                        "temperature": None,
                        "humidity": None,
                        "wind_speed": None,
                        "wind_direction": None,
                        "rainfall": 0.0
                    }
                    
                    try:
                        # tm:0, ta:1, td:2, hm:3, ws:4, rn_15:5...
                        if len(fields) > 1: result["temperature"] = float(fields[1])
                        if len(fields) > 3: result["humidity"] = float(fields[3])
                        if len(fields) > 4: result["wind_speed"] = float(fields[4])
                        if len(fields) > 5: result["rainfall"] = float(fields[5])
                    except Exception as e:
                        print(f"[KMA Hub] 파싱 오류 ({tm}): {e}")
                        
                    return result
            except Exception as e:
                print(f"[KMA Hub] 패치 오류: {e}")
        return None

async def main():
    print("=== 기상청 API Hub 데이터 페처 시작 (UTC 보정 적용) ===")
    
    fetcher = KMAApiHubFetcher(auth_key=AUTH_KEY, lat=LAT, lon=LON)
    
    while True:
        try:
            kst = fetcher.get_kst_now()
            print(f"[{kst}] 업데이트 시도 중 (KST 기준 조회)...")
            
            conn = pymysql.connect(host=DB_HOST, user=DB_USER, password=DB_PASS, database=DB_NAME, autocommit=True)
            cursor = conn.cursor()
            
            obs = await fetcher.fetch_current()
            if obs and (obs['temperature'] is not None):
                # [참고] 현재 사용중인 '특정지점 다중요소' API는 예보 기능이 없으므로
                # 대시보드 UI 호환성을 위해 현재 관측값을 예보 오프셋(0, 1, 3, 6)에 모두 저장함.
                # 실제 예보 정보를 원하실 경우 '단기예보' API 연동이 필요합니다.
                for offset in [0, 1, 3, 6]:
                    cursor.execute(
                        "INSERT INTO weather_data (source, forecast_offset, wind_speed, wind_direction, rainfall, temperature, humidity, timestamp) "
                        "VALUES ('KMA', %s, %s, %s, %s, %s, %s, %s)",
                        (offset, obs.get('wind_speed'), obs.get('wind_direction'), obs.get('rainfall'), obs.get('temperature'), obs.get('humidity'), kst)
                    )
                print(f"  -> 저장 완료 ({kst}): {obs['temperature']}°C / {obs['humidity']}%")
            else:
                print("  -> 유효한 데이터를 수신하지 못했습니다.")
            
            conn.close()
            
        except Exception as e:
            print(f"!!! 루프 오류: {e}")
            
        await asyncio.sleep(3600)

if __name__ == "__main__":
    asyncio.run(main())
