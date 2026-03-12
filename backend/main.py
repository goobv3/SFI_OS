# ---------------------------------------------------------
# 스마트팜 백엔드 메인 서버 (Smart Farm Backend API Server)
# ---------------------------------------------------------
# 이 파일은 전체 시스템의 '뇌' 역할을 합니다. 
# 프론트엔드 화면에서 들어오는 조작 요청(문 열기, 센서 달기 등)을 받아
# 데이터베이스에 저장하거나 읽어오는 작업을 수행합니다.
# 또한 주기적으로 기상청에 접속해 날씨 데이터를 가져오는 백그라운드 작업도 담당합니다.
# ---------------------------------------------------------

import asyncio
import httpx
from datetime import datetime, timedelta, timezone
from fastapi import FastAPI, HTTPException, BackgroundTasks, Query
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
from typing import Optional, List
from database import get_db_connection
from arbitration import load_rules
from contextlib import contextmanager
from weather_lib import KMAWeatherFetcher
from house_lib import HouseManager
from sensor_lib import SensorManager, MQTTSensorListener
from control_lib import ControlManager
import os

# MQTT Listener 인스턴스 (글로벌 변수로 유지)
mqtt_listener = None

# FastAPI 서버 앱을 생성합니다. (우리가 만들 서버의 이름입니다)
app = FastAPI(title="Smart Farm Intelligence OS API")

# --- CORS (Cross-Origin Resource Sharing) 설정 ---
# 프론트엔드 화면(예: localhost:5173)과 백엔드 서버(localhost:8000)가
# 서로 주소가 달라도 통신할 수 있게 허락해주는 설정입니다.
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"], # Frontend URLs (Updated to all for local dev)
    allow_credentials=True,
    allow_methods=["GET", "POST", "PUT", "DELETE", "OPTIONS"],
    allow_headers=["*"],
)

@app.options("/{full_path:path}")
def preflight_handler(full_path: str):
    return {}

# 안전 규칙 모듈(arbitration.py)에서 저장된 YAML 규칙을 불러와서 서버 메모리에 저장해둡니다.
rules = load_rules()

# --- Pydantic Models (데이터 구조 정의) ---
# 프론트엔드에서 데이터를 보낼 때, 반드시 이 통의 모양(규칙)에 맞게
# 데이터가 들어와야 서버가 에러 없이 받을 수 있도록 틀을 짜놓은 것입니다.

class HouseCreate(BaseModel):
    # 하우스(온실) 구역을 새로 만들 때 필요한 틀
    house_id: str
    name: str
    display_order: int = 0

class HouseUpdate(BaseModel):
    name: str
    display_order: int = 0

class SensorMetadataCreate(BaseModel):
    sensor_id: str
    house_id: str
    alias: str
    type: str
    unit: str
    display_order: int = 0
    is_active: bool = True

class ActuatorMetadataCreate(BaseModel):
    actuator_id: str
    house_id: str
    alias: str
    type: str

class DeviceUpdate(BaseModel):
    alias: str
    type: str
    unit: Optional[str] = None # Sensor only
    display_order: int = 0
    is_active: bool = True
    warn_high: Optional[float] = None
    warn_low: Optional[float] = None
    crit_high: Optional[float] = None
    crit_low: Optional[float] = None

class SensorData(BaseModel):
    sensor_id: str
    value: float

class ControlCommand(BaseModel):
    actuator_id: str
    command: str
    source: str = "Manual"
    priority: int = 2

class WeatherData(BaseModel):
    source: str = "FARM" # "FARM" or "KMA"
    forecast_offset: int = 0
    wind_speed: Optional[float] = None
    wind_direction: Optional[str] = None
    rainfall: Optional[float] = None
    solar_radiation: Optional[float] = None
    temperature: Optional[float] = None
    humidity: Optional[float] = None

# --- Helpers ---
@contextmanager
def db_session():
    conn = get_db_connection()
    try:
        yield conn
    finally:
        if conn:
            conn.close()

def log_control(conn, actuator_id, command, source, priority, result, message):
    with conn.cursor() as cursor:
        cursor.execute("""
            INSERT INTO control_logs (actuator_id, command, source, priority, result, message)
            VALUES (%s, %s, %s, %s, %s, %s)
        """, (actuator_id, command, source, priority, result, message))


# --- API Endpoints ---
@app.get("/")
def read_root():
    return {"status": "Smart Farm Intelligence OS API is running"}

# --- House Management ---
@app.get("/api/houses")
def get_houses():
    with db_session() as conn:
        return HouseManager(conn).get_all_houses()

@app.post("/api/houses")
def create_house(house: HouseCreate):
    with db_session() as conn:
        try:
            HouseManager(conn).create_house(house.house_id, house.name, house.display_order)
        except Exception as e:
            raise HTTPException(status_code=400, detail=str(e))
    return {"status": "success"}

@app.delete("/api/houses/{house_id}")
def delete_house(house_id: str):
    with db_session() as conn:
        HouseManager(conn).delete_house(house_id)
    return {"status": "success"}

@app.put("/api/houses/{house_id}")
def update_house(house_id: str, house_update: HouseUpdate):
    with db_session() as conn:
        try:
            HouseManager(conn).update_house(house_id, house_update.name, house_update.display_order)
        except Exception as e:
            raise HTTPException(status_code=400, detail=str(e))
    return {"status": "success"}

# --- Devices Metadata Management ---
@app.get("/api/houses/{house_id}/devices")
def get_house_devices(house_id: str):
    with db_session() as conn:
        return HouseManager(conn).get_house_devices(house_id)

@app.post("/api/metadata/sensors")
def create_sensor_metadata(meta: SensorMetadataCreate):
    with db_session() as conn:
        try:
            HouseManager(conn).create_sensor_metadata(
                meta.sensor_id, meta.house_id, meta.alias, meta.type, meta.unit, meta.display_order, meta.is_active
            )
        except Exception as e:
            raise HTTPException(status_code=400, detail=str(e))
    return {"status": "success"}

@app.delete("/api/metadata/sensors/{sensor_id}")
def delete_sensor_metadata(sensor_id: str):
    with db_session() as conn:
        HouseManager(conn).delete_sensor_metadata(sensor_id)
    return {"status": "success"}

@app.post("/api/metadata/actuators")
def create_actuator_metadata(meta: ActuatorMetadataCreate):
    with db_session() as conn:
        try:
            HouseManager(conn).create_actuator_metadata(
                meta.actuator_id, meta.house_id, meta.alias, meta.type
            )
        except Exception as e:
            raise HTTPException(status_code=400, detail=str(e))
    return {"status": "success"}

@app.delete("/api/metadata/actuators/{actuator_id}")
def delete_actuator_metadata(actuator_id: str):
    with db_session() as conn:
        HouseManager(conn).delete_actuator_metadata(actuator_id)
    return {"status": "success"}

@app.put("/api/metadata/sensors/{sensor_id}")
def update_sensor_metadata(sensor_id: str, diff: DeviceUpdate):
    with db_session() as conn:
        try:
            HouseManager(conn).update_sensor_metadata(
                sensor_id, diff.alias, diff.type, diff.unit, diff.display_order, diff.is_active,
                diff.warn_high, diff.warn_low, diff.crit_high, diff.crit_low
            )
        except Exception as e:
            raise HTTPException(status_code=400, detail=str(e))
    return {"status": "success"}

@app.put("/api/metadata/actuators/{actuator_id}")
def update_actuator_metadata(actuator_id: str, diff: DeviceUpdate):
    with db_session() as conn:
        try:
            HouseManager(conn).update_actuator_metadata(actuator_id, diff.alias, diff.type)
        except Exception as e:
            raise HTTPException(status_code=400, detail=str(e))
    return {"status": "success"}

# --- Device Discovery (Inbox) Management ---
@app.get("/api/discovery")
def get_discovered_devices():
    with db_session() as conn:
        return HouseManager(conn).get_discovered_devices()

@app.delete("/api/discovery/{device_id}")
def remove_discovered_device(device_id: str):
    with db_session() as conn:
        HouseManager(conn).remove_discovered_device(device_id)
    return {"status": "success"}

# --- Sensor Data & Control ---
@app.post("/api/sensors")
def receive_sensor_data(data: SensorData):
    with db_session() as conn:
        if not conn:
            raise HTTPException(status_code=500, detail="Database connection failed")
        
        manager = SensorManager(conn)
        result = manager.process_incoming_data(data.sensor_id, data.value)
        return result

# --- Alarms Management ---
@app.get("/api/alarms")
def get_alarms():
    with db_session() as conn:
        return SensorManager(conn).get_unacknowledged_alarms()

@app.post("/api/alarms/{alarm_id}/acknowledge")
def acknowledge_alarm(alarm_id: int):
    with db_session() as conn:
        SensorManager(conn).acknowledge_alarm(alarm_id)
    return {"status": "success"}

@app.get("/api/sensors/{sensor_id}/history")
def get_sensor_history(sensor_id: str, period: str = 'daily'):
    with db_session() as conn:
        try:
            return SensorManager(conn).get_aggregate_history(sensor_id, period)
        except ValueError as e:
            raise HTTPException(status_code=400, detail=str(e))

@app.get("/api/sensors/history_range")
def get_sensor_history_range(sensor_ids: str, start_time: str, end_time: str):
    """ Fetch history data for one or multiple sensors within a custom time range. """
    id_list = [sid.strip() for sid in sensor_ids.split(",") if sid.strip()]
    if not id_list:
        raise HTTPException(status_code=400, detail="No sensor IDs provided")
        
    try:
        from datetime import datetime
        start_utc = datetime.fromisoformat(start_time.replace('Z', '+00:00'))
        end_utc = datetime.fromisoformat(end_time.replace('Z', '+00:00'))
        start_dt = start_utc.astimezone()
        end_dt = end_utc.astimezone()
    except Exception:
        raise HTTPException(status_code=400, detail="Invalid date format. Use ISO format.")

    with db_session() as conn:
        try:
            return SensorManager(conn).get_custom_range_history(id_list, start_dt, end_dt)
        except Exception as e:
            raise HTTPException(status_code=500, detail=str(e))

@app.post("/api/control")
def process_control(cmd: ControlCommand):
    with db_session() as conn:
        if not conn:
            raise HTTPException(status_code=500, detail="Database connection failed")
            
        manager = ControlManager(conn, rules)
        try:
            result = manager.process_control_command(
                cmd.actuator_id, cmd.command, cmd.source, cmd.priority
            )
            return result
        except ValueError as e:
            raise HTTPException(status_code=404, detail=str(e))
        except RuntimeError as e:
            raise HTTPException(status_code=500, detail=str(e))

# --- Weather Data Integration ---
@app.post("/api/weather/farm")
def receive_farm_weather(data: WeatherData):
    with db_session() as conn:
        with conn.cursor() as cursor:
            try:
                cursor.execute("""
                    INSERT INTO weather_data (source, forecast_offset, wind_speed, wind_direction, rainfall, solar_radiation, temperature, humidity)
                    VALUES (%s, %s, %s, %s, %s, %s, %s, %s)
                """, (data.source, data.forecast_offset, data.wind_speed, data.wind_direction, data.rainfall, data.solar_radiation, data.temperature, data.humidity))
                conn.commit()
            except Exception as e:
                raise HTTPException(status_code=500, detail=str(e))
    return {"status": "success", "message": "Farm weather data ingested"}

@app.get("/api/weather/latest")
def get_latest_weather(hours_ahead: int = 0):
    with db_session() as conn:
        with conn.cursor() as cursor:
            weather_res = {}
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
                weather_res[src] = row if row else None
            return weather_res

async def fetch_kma_weather():
    SERVICE_KEY = "3b1b77976ce0ca80247d4cd707834cff148c26d257a9b03826dda6829b75f0ad"
    fetcher = KMAWeatherFetcher(service_key=SERVICE_KEY, nx=55, ny=127)
    
    while True:
        try:
            weather_data = await fetcher.fetch_all()
            
            with db_session() as conn:
                with conn.cursor() as cursor:
                    kst = timezone(timedelta(hours=9))
                    # 1. Save Observation (Offset 0)
                    obs = weather_data.get("observation")
                    if obs and obs.get("temperature") is not None:
                        actual_time_str = datetime.now(kst).strftime("%Y-%m-%d %H:%M:%S")
                        cursor.execute("""
                            INSERT INTO weather_data (source, timestamp, forecast_offset, temperature, humidity, wind_speed, wind_direction, rainfall)
                            VALUES (%s, %s, 0, %s, %s, %s, %s, %s)
                        """, ('KMA', actual_time_str, obs['temperature'], obs['humidity'], obs['wind_speed'], obs['wind_direction'], obs['rainfall']))
                        
                    # 2. Save Forecasts (Offsets 1~6)
                    forecasts = weather_data.get("forecasts", [])
                    for fcst in forecasts:
                        # Reconstruct the forecasted time string roughly based on now + offset for simplicity in indexing
                        fcst_time = datetime.now(kst) + timedelta(hours=fcst['offset_hours'])
                        cursor.execute("""
                            INSERT INTO weather_data (source, timestamp, forecast_offset, temperature, humidity, wind_speed, wind_direction, rainfall)
                            VALUES (%s, %s, %s, %s, %s, %s, %s, %s)
                        """, ('KMA', fcst_time.strftime("%Y-%m-%d %H:%M:%S"), fcst['offset_hours'], fcst['temperature'], fcst['humidity'], fcst['wind_speed'], fcst['wind_direction'], fcst['rainfall']))
                        
                conn.commit()
                print(f"[{datetime.now(kst)}] Fetched and saved KMA Observation and Forecasts via weather_lib.")
                
        except Exception as e:
            print(f"KMA Fetch Error: {e}")
            
        await asyncio.sleep(1800)

@app.on_event("startup")
async def startup_event():
    global mqtt_listener
    
    # 1. 기상청 백그라운드 태스크 시작
    asyncio.create_task(fetch_kma_weather())
    
    # 2. MQTT 백그라운드 리스너 시작
    db_params = {
        'host': os.getenv("DB_HOST", "mariadb"),
        'port': int(os.getenv("DB_PORT", 3306)),
        'user': os.getenv("DB_USER", "farmuser"),
        'password': os.getenv("DB_PASSWORD", "farmsecret"),
        'db_name': os.getenv("DB_NAME", "smartfarm")
    }
    # docker-compose 에서는 'mosquitto' 서비스명으로 컨테이너간 통신
    mqtt_host = os.getenv("MQTT_HOST", "mosquitto")
    mqtt_listener = MQTTSensorListener(db_params, mqtt_host=mqtt_host)
    mqtt_listener.start()

@app.on_event("shutdown")
async def shutdown_event():
    global mqtt_listener
    if mqtt_listener:
        mqtt_listener.stop()

