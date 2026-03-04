import sys
from database import get_db_connection

def test_query(hours_ahead):
    conn = get_db_connection()
    with conn.cursor() as cursor:
        for src in ['KMA', 'FARM']:
            if hours_ahead > 0 and src == 'KMA':
                cursor.execute("""
                    SELECT timestamp, wind_speed, wind_direction, rainfall, solar_radiation, temperature, humidity 
                    FROM weather_data 
                    WHERE source = %s AND forecast_offset = %s
                    ORDER BY id DESC LIMIT 1
                """, (src, hours_ahead))
            else:
                cursor.execute("""
                    SELECT timestamp, wind_speed, wind_direction, rainfall, solar_radiation, temperature, humidity 
                    FROM weather_data 
                    WHERE source = %s AND forecast_offset = 0
                    ORDER BY timestamp DESC LIMIT 1
                """, (src,))
            row = cursor.fetchone()
            print(f"hours_ahead={hours_ahead}, src={src} -> {row}")

if __name__ == "__main__":
    test_query(0)
    test_query(1)
    test_query(3)
    test_query(6)
