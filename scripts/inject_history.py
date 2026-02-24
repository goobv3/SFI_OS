import pymysql
import os
import random
from datetime import datetime, timedelta

# DB 연결
conn = pymysql.connect(
    host=os.getenv('DB_HOST', 'mariadb'),
    user=os.getenv('DB_USER', 'root'), # or farmuser based on env
    password=os.getenv('DB_PASSWORD', 'rootsecret'),
    database=os.getenv('DB_NAME', 'smartfarm')
)
cursor = conn.cursor(pymysql.cursors.DictCursor)

cursor.execute("SELECT sensor_id, type FROM sensor_metadata")
rows = cursor.fetchall()

SENSORS = []
for r in rows:
    SENSORS.append({
        "id": r['sensor_id'], 
        "type": r['type'], 
        "base": 25.0 if "temp" in r['type'].lower() else 50.0, 
        "var": 5.0 if "temp" in r['type'].lower() else 15.0
    })

if not SENSORS:
    print("No sensors found in sensor_metadata. Please add some sensors first.")
    cursor.close()
    conn.close()
    exit(0)

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
