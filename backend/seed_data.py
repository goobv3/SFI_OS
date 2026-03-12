import pymysql
import os
from datetime import datetime, timedelta
import random

def get_db_connection():
    # If DB_HOST is not set, default to localhost for local testing outside of Docker
    db_host = os.getenv("DB_HOST", "localhost")
    db_user = os.getenv("DB_USER", "root")
    db_password = os.getenv("DB_PASSWORD", "rootsecret")
    db_name = os.getenv("DB_NAME", "smartfarm")

    return pymysql.connect(
        host=db_host,
        port=int(os.getenv("DB_PORT", 3306)),
        user=db_user,
        password=db_password,
        database=db_name,
        cursorclass=pymysql.cursors.DictCursor,
        autocommit=True
    )

def seed():
    try:
        conn = get_db_connection()
        cursor = conn.cursor()

        # Clear old data for these sensors to prevent massive buildup if run multiple times
        cursor.execute("DELETE FROM sensors WHERE sensor_id IN ('TEMP_01', 'HUM_01')")
        
        now = datetime.now()
        
        # Base values
        temp_base = 25.0
        hum_base = 60.0

        print("Seeding sensor data... (Past 24 hours)")
        count = 0
        
        # Generate data for the last 24 hours (1440 minutes), every 5 minutes (288 points)
        for i in range(288, -1, -1):
            point_time = now - timedelta(minutes=i*5)
            
            # Add some random walk for realism
            temp_val = temp_base + random.uniform(-1.0, 1.0)
            hum_val = hum_base + random.uniform(-2.5, 2.5)
            
            # Keep them in sane bounds
            temp_val = max(10.0, min(40.0, temp_val))
            hum_val = max(30.0, min(95.0, hum_val))
            
            # Update base for next step
            temp_base = temp_val
            hum_base = hum_val

            cursor.execute(
                "INSERT INTO sensors (timestamp, sensor_id, value) VALUES (%s, %s, %s)",
                (point_time.strftime('%Y-%m-%d %H:%M:%S'), 'TEMP_01', round(temp_val, 1))
            )
            cursor.execute(
                "INSERT INTO sensors (timestamp, sensor_id, value) VALUES (%s, %s, %s)",
                (point_time.strftime('%Y-%m-%d %H:%M:%S'), 'HUM_01', round(hum_val, 1))
            )
            count += 2
            
        print(f"Inserted {count} rows of sensor data successfully.")
        conn.close()
    except Exception as e:
        print(f"Error seeding DB: {e}")

if __name__ == "__main__":
    seed()
