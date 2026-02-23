import pymysql
import os
import random
from datetime import datetime, timedelta

# DB 연결
conn = pymysql.connect(
    host=os.getenv('DB_HOST', 'mariadb'),
    user=os.getenv('DB_USER', 'root'),
    password=os.getenv('DB_PASSWORD', 'rootsecret'),
    database=os.getenv('DB_NAME', 'smartfarm')
)
cursor = conn.cursor()

# 대상 센서 고정 (미리 프론트에 등록되어 있다고 가정)
SENSORS = [
    {"id": "HISTORY_TEMP_1", "type": "temperature", "base": 25.0, "var": 5.0},
    {"id": "HISTORY_HUMID_1", "type": "humidity", "base": 50.0, "var": 10.0}
]

# 먼저 메타데이터에 없으면 House 1 추가
cursor.execute("SELECT house_id FROM houses LIMIT 1")
house = cursor.fetchone()
if house:
    house_id = house[0]
else:
    house_id = "HOUSE_DEFAULT"
    cursor.execute("INSERT IGNORE INTO houses (house_id, name) VALUES (%s, %s)", (house_id, "Main House"))

for s in SENSORS:
    cursor.execute("""
        INSERT IGNORE INTO sensor_metadata (sensor_id, house_id, alias, type, unit)
        VALUES (%s, %s, %s, %s, %s)
    """, (s['id'], house_id, f"Virtual {s['type'].title()}", s['type'], "C" if s['type'] == "temperature" else "%"))

conn.commit()

# 데이터 생성: 최근 3일은 분 단위, 그 이전 27일은 시간 단위로 생성
now = datetime.now()
data_to_insert = []

for s in SENSORS:
    # --- 1. 과거 30일 ~ 3일 전 (시간 단위 데이터) ---
    for days_ago in range(30, 3, -1):
        for hour in range(0, 24):
            time_offset = (hour - 14) / 12  # 14시가 피크
            val = s['base'] + (s['var'] * time_offset) + random.uniform(-2, 2)
            val = round(val, 1)
            
            t = now - timedelta(days=days_ago)
            t = t.replace(hour=hour, minute=0, second=0, microsecond=0)
            
            if t > now:
                continue
                
            data_to_insert.append((s['id'], val, t.strftime('%Y-%m-%d %H:%M:%S')))

    # --- 2. 최근 3일 (분 단위 데이터) ---
    for days_ago in range(3, -1, -1):
        for hour in range(0, 24):
            for minute in range(0, 60):
                # 미세한 분 단위 변동폭 추가
                time_offset = (hour - 14) / 12
                minute_noise = random.uniform(-0.5, 0.5)
                val = s['base'] + (s['var'] * time_offset) + minute_noise
                val = round(val, 2)
                
                t = now - timedelta(days=days_ago)
                t = t.replace(hour=hour, minute=minute, second=0, microsecond=0)
                
                if t > now:
                    continue
                    
                data_to_insert.append((s['id'], val, t.strftime('%Y-%m-%d %H:%M:%S')))

# Batch Insert DB (너무 많으면 나눠서 처리)
batch_size = 5000
insert_query = "INSERT INTO sensors (sensor_id, value, timestamp) VALUES (%s, %s, %s)"

for i in range(0, len(data_to_insert), batch_size):
    batch = data_to_insert[i:i + batch_size]
    cursor.executemany(insert_query, batch)
    conn.commit()

print(f"✅ 총 {len(data_to_insert)}개의 가상 히스토리 데이터 스트림 주입 완료!")
cursor.close()
conn.close()
