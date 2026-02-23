import urllib.request
import json
import time
import random

url = "http://localhost:8000/api/sensors"
headers = {'Content-Type': 'application/json'}
sensors = ["VIRTUAL_DEVICE_X", "VIRTUAL_DEVICE_Y"]

print("🚀 백그라운드 테스트 신호 발송을 시작합니다... (3초 간격)")
for i in range(50):
    for s_id in sensors:
        data = {
            "sensor_id": s_id,
            "value": round(random.uniform(20.0, 30.0), 1)
        }
        
        req = urllib.request.Request(
            url, 
            data=json.dumps(data).encode('utf-8'), 
            headers=headers
        )
        try:
            with urllib.request.urlopen(req) as response:
                print(f"[{i}] Sent {s_id}: {data['value']} -> Response OK")
        except Exception as e:
            print(f"[{i}] Error sending {s_id}: {e}")
            
    time.sleep(3)
print("✅ 테스트 신호 발송 종료.")
