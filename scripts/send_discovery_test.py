import requests
import time
import random

# 백엔드 API 주소
API_URL = "http://localhost:8000/api/sensors"

# 임의의(아직 서버에 수동 등록되지 않은) 센서 ID들
DISCOVERY_SENSORS = [
    "NEW_HOUSE3_TEMP",
    "NEW_HOUSE3_HUMID"
]

def send_dummy_data():
    for sensor_id in DISCOVERY_SENSORS:
        # 온도: 20~30 사이의 난수, 습도: 40~60 사이의 난수
        if "TEMP" in sensor_id:
            val = round(random.uniform(20.0, 30.0), 1)
        else:
            val = round(random.uniform(40.0, 60.0), 1)
            
        payload = {
            "sensor_id": sensor_id,
            "value": val
        }
        
        try:
            response = requests.post(API_URL, json=payload)
            print(f"[전송] {sensor_id} 값: {val} -> 응답: {response.json()}")
        except Exception as e:
            print(f"[실패] {sensor_id} 전송 에러: {e}")

if __name__ == "__main__":
    print("🚀 센서 신호 전송 시뮬레이션을 시작합니다. (Discovery Inbox 테스트용)")
    # 3번만 전송
    for _ in range(3):
        send_dummy_data()
        time.sleep(1.5)
    print("✅ 데이터 전송 모사 완료!")
