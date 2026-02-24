from fastapi import FastAPI, HTTPException, BackgroundTasks, Query
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
from typing import Optional, List
from database import get_db_connection
from arbitration import load_rules, check_interlock_rules
from contextlib import contextmanager

app = FastAPI(title="Smart Farm Intelligence OS API")

# --- CORS Settings ---
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

rules = load_rules()

# --- Pydantic Models ---
class HouseCreate(BaseModel):
    house_id: str
    name: str

class HouseUpdate(BaseModel):
    name: str

class SensorMetadataCreate(BaseModel):
    sensor_id: str
    house_id: str
    alias: str
    type: str
    unit: str

class ActuatorMetadataCreate(BaseModel):
    actuator_id: str
    house_id: str
    alias: str
    type: str

class DeviceUpdate(BaseModel):
    alias: str
    type: str
    unit: Optional[str] = None # Sensor only

class SensorData(BaseModel):
    sensor_id: str
    value: float

class ControlCommand(BaseModel):
    actuator_id: str
    command: str
    source: str = "Manual"
    priority: int = 2

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
        with conn.cursor() as cursor:
            cursor.execute("SELECT house_id, name, created_at FROM houses")
            return cursor.fetchall()

@app.post("/api/houses")
def create_house(house: HouseCreate):
    with db_session() as conn:
        with conn.cursor() as cursor:
            try:
                cursor.execute("INSERT INTO houses (house_id, name) VALUES (%s, %s)", (house.house_id, house.name))
                conn.commit()
            except Exception as e:
                raise HTTPException(status_code=400, detail=str(e))
    return {"status": "success"}

@app.delete("/api/houses/{house_id}")
def delete_house(house_id: str):
    with db_session() as conn:
        with conn.cursor() as cursor:
            cursor.execute("DELETE FROM houses WHERE house_id = %s", (house_id,))
            conn.commit()
    return {"status": "success"}

@app.put("/api/houses/{house_id}")
def update_house(house_id: str, house_update: HouseUpdate):
    with db_session() as conn:
        with conn.cursor() as cursor:
            try:
                cursor.execute("UPDATE houses SET name = %s WHERE house_id = %s", (house_update.name, house_id))
                conn.commit()
            except Exception as e:
                raise HTTPException(status_code=400, detail=str(e))
    return {"status": "success"}

# --- Devices Metadata Management ---
@app.get("/api/houses/{house_id}/devices")
def get_house_devices(house_id: str):
    with db_session() as conn:
        with conn.cursor() as cursor:
            cursor.execute("SELECT * FROM sensor_metadata WHERE house_id = %s", (house_id,))
            sensors = cursor.fetchall()
            cursor.execute("""
                SELECT am.*, ast.status, ast.target_value, ast.manual_lock 
                FROM actuator_metadata am
                LEFT JOIN actuator_status ast ON am.actuator_id = ast.actuator_id
                WHERE am.house_id = %s
            """, (house_id,))
            actuators = cursor.fetchall()
            return {"sensors": sensors, "actuators": actuators}

@app.post("/api/metadata/sensors")
def create_sensor_metadata(meta: SensorMetadataCreate):
    with db_session() as conn:
        with conn.cursor() as cursor:
            try:
                cursor.execute("""
                    INSERT INTO sensor_metadata (sensor_id, house_id, alias, type, unit)
                    VALUES (%s, %s, %s, %s, %s)
                """, (meta.sensor_id, meta.house_id, meta.alias, meta.type, meta.unit))
                conn.commit()
            except Exception as e:
                raise HTTPException(status_code=400, detail=str(e))
    return {"status": "success"}

@app.delete("/api/metadata/sensors/{sensor_id}")
def delete_sensor_metadata(sensor_id: str):
    with db_session() as conn:
        with conn.cursor() as cursor:
            cursor.execute("DELETE FROM sensor_metadata WHERE sensor_id = %s", (sensor_id,))
            conn.commit()
    return {"status": "success"}

@app.post("/api/metadata/actuators")
def create_actuator_metadata(meta: ActuatorMetadataCreate):
    with db_session() as conn:
        with conn.cursor() as cursor:
            try:
                cursor.execute("""
                    INSERT INTO actuator_metadata (actuator_id, house_id, alias, type)
                    VALUES (%s, %s, %s, %s)
                """, (meta.actuator_id, meta.house_id, meta.alias, meta.type))
                cursor.execute("""
                    INSERT IGNORE INTO actuator_status (actuator_id, status)
                    VALUES (%s, 'Off')
                """, (meta.actuator_id,))
                conn.commit()
            except Exception as e:
                raise HTTPException(status_code=400, detail=str(e))
    return {"status": "success"}

@app.delete("/api/metadata/actuators/{actuator_id}")
def delete_actuator_metadata(actuator_id: str):
    with db_session() as conn:
        with conn.cursor() as cursor:
            cursor.execute("DELETE FROM actuator_metadata WHERE actuator_id = %s", (actuator_id,))
            conn.commit()
    return {"status": "success"}

@app.put("/api/metadata/sensors/{sensor_id}")
def update_sensor_metadata(sensor_id: str, diff: DeviceUpdate):
    with db_session() as conn:
        with conn.cursor() as cursor:
            try:
                cursor.execute("UPDATE sensor_metadata SET alias = %s, type = %s, unit = %s WHERE sensor_id = %s", (diff.alias, diff.type, diff.unit, sensor_id))
                conn.commit()
            except Exception as e:
                raise HTTPException(status_code=400, detail=str(e))
    return {"status": "success"}

@app.put("/api/metadata/actuators/{actuator_id}")
def update_actuator_metadata(actuator_id: str, diff: DeviceUpdate):
    with db_session() as conn:
        with conn.cursor() as cursor:
            try:
                cursor.execute("UPDATE actuator_metadata SET alias = %s, type = %s WHERE actuator_id = %s", (diff.alias, diff.type, actuator_id))
                conn.commit()
            except Exception as e:
                raise HTTPException(status_code=400, detail=str(e))
    return {"status": "success"}

# --- Device Discovery (Inbox) Management ---
@app.get("/api/discovery")
def get_discovered_devices():
    with db_session() as conn:
        with conn.cursor() as cursor:
            cursor.execute("SELECT device_id, device_type, first_seen, last_seen, last_value FROM unregistered_devices ORDER BY last_seen DESC")
            return cursor.fetchall()

@app.delete("/api/discovery/{device_id}")
def remove_discovered_device(device_id: str):
    with db_session() as conn:
        with conn.cursor() as cursor:
            cursor.execute("DELETE FROM unregistered_devices WHERE device_id = %s", (device_id,))
            conn.commit()
    return {"status": "success"}

# --- Sensor Data & Control ---
@app.post("/api/sensors")
def receive_sensor_data(data: SensorData):
    with db_session() as conn:
        if not conn:
            raise HTTPException(status_code=500, detail="Database connection failed")
            
        with conn.cursor() as cursor:
            try:
                # 1. Try to insert as official sensor reading
                cursor.execute("""
                    INSERT INTO sensors (sensor_id, value)
                    VALUES (%s, %s)
                """, (data.sensor_id, data.value))
                
                # If succeeded, ensure it's removed from discovery inbox if it was there
                cursor.execute("DELETE FROM unregistered_devices WHERE device_id = %s", (data.sensor_id,))
                conn.commit()
            except Exception as e:
                # 2. Foreign Key Failure (Sensor not in sensor_metadata!)
                # Put it into Auto-Discovery Inbox
                cursor.execute("""
                    INSERT INTO unregistered_devices (device_id, device_type, last_value)
                    VALUES (%s, 'sensor', %s)
                    ON DUPLICATE KEY UPDATE last_seen = CURRENT_TIMESTAMP, last_value = %s
                """, (data.sensor_id, data.value, data.value))
                conn.commit()
                return {"status": "accepted", "message": "Unknown sensor added to Discovery Inbox"}
            
    return {"status": "success", "message": "Sensor data ingested"}

@app.get("/api/sensors/{sensor_id}/history")
def get_sensor_history(sensor_id: str, period: str = 'daily'):
    # period: 'daily' (last 24h), 'monthly' (last 30 days daily avg), 'yearly' (monthly avg)
    with db_session() as conn:
        with conn.cursor() as cursor:
            if period == 'daily':
                cursor.execute("""
                    SELECT DATE_FORMAT(timestamp, '%%H:00') as time, AVG(value) as avg_value
                    FROM sensors
                    WHERE sensor_id = %s AND timestamp >= NOW() - INTERVAL 1 DAY
                    GROUP BY HOUR(timestamp)
                    ORDER BY MIN(timestamp) ASC
                """, (sensor_id,))
            elif period == 'monthly':
                cursor.execute("""
                    SELECT DATE_FORMAT(timestamp, '%%m-%%d') as time, AVG(value) as avg_value
                    FROM sensors
                    WHERE sensor_id = %s AND timestamp >= NOW() - INTERVAL 30 DAY
                    GROUP BY DATE(timestamp)
                    ORDER BY MIN(timestamp) ASC
                """, (sensor_id,))
            elif period == 'yearly':
                cursor.execute("""
                    SELECT DATE_FORMAT(timestamp, '%%Y-%%m') as time, AVG(value) as avg_value
                    FROM sensors
                    WHERE sensor_id = %s AND timestamp >= NOW() - INTERVAL 1 YEAR
                    GROUP BY YEAR(timestamp), MONTH(timestamp)
                    ORDER BY MIN(timestamp) ASC
                """, (sensor_id,))
            else:
                raise HTTPException(status_code=400, detail="Invalid period")
            
            return cursor.fetchall()

@app.get("/api/sensors/history_range")
def get_sensor_history_range(sensor_ids: str, start_time: str, end_time: str):
    """
    Fetch history data for one or multiple sensors within a custom time range.
    `sensor_ids` should be a comma-separated string of sensor IDs.
    """
    id_list = [sid.strip() for sid in sensor_ids.split(",") if sid.strip()]
    if not id_list:
        raise HTTPException(status_code=400, detail="No sensor IDs provided")
        
    try:
        from datetime import datetime
        start_dt = datetime.fromisoformat(start_time.replace('Z', '+00:00'))
        end_dt = datetime.fromisoformat(end_time.replace('Z', '+00:00'))
        diff_hours = (end_dt - start_dt).total_seconds() / 3600
    except Exception as e:
        raise HTTPException(status_code=400, detail="Invalid date format. Use ISO format.")

    # Determine grouping format based on duration
    if diff_hours <= 24: # Less than a day -> group by Minute or every 10 mins depending on density (we'll use minute since user requested high density for 3h)
        date_format = '%m-%d %H:%i' if diff_hours > 3 else '%H:%i' 
        group_by = "DATE(timestamp), HOUR(timestamp), MINUTE(timestamp)"
    elif diff_hours <= 168: # 1 to 7 days -> group by Hour
        date_format = '%m-%d %H:00'
        group_by = "DATE(timestamp), HOUR(timestamp)"
    else: # More than 7 days -> group by Day
        date_format = '%Y-%m-%d'
        group_by = "DATE(timestamp)"

    placeholders = ','.join(['%s'] * len(id_list))
    query = f"""
        SELECT 
            DATE_FORMAT(timestamp, '{date_format}') as time, 
            sensor_id, 
            AVG(value) as avg_value
        FROM sensors
        WHERE sensor_id IN ({placeholders}) 
          AND timestamp >= %s 
          AND timestamp <= %s
        GROUP BY {group_by}, sensor_id
        ORDER BY MIN(timestamp) ASC
    """
    
    params = tuple(id_list) + (start_dt.strftime('%Y-%m-%d %H:%M:%S'), end_dt.strftime('%Y-%m-%d %H:%M:%S'))

    with db_session() as conn:
        with conn.cursor() as cursor:
            cursor.execute(query, params)
            rows = cursor.fetchall()

    # Pivot data so each time point has all requested sensor values
    # e.g., Output: [{"time": "14:30", "TEMP_1": 25.1, "HUMID_1": 60.5}, ...]
    pivot_dict = {}
    for row in rows:
        t = row['time']
        sid = row['sensor_id']
        val = float(row['avg_value'])
        
        if t not in pivot_dict:
            pivot_dict[t] = {"time": t}
        pivot_dict[t][sid] = round(val, 2)
        
    return list(pivot_dict.values())

@app.post("/api/control")
def process_control(cmd: ControlCommand):
    with db_session() as conn:
        if not conn:
            raise HTTPException(status_code=500, detail="Database connection failed")
            
        with conn.cursor() as cursor:
            cursor.execute("""
                SELECT s.status, s.manual_lock, m.type 
                FROM actuator_status s
                JOIN actuator_metadata m ON s.actuator_id = m.actuator_id
                WHERE s.actuator_id = %s
            """, (cmd.actuator_id,))
            actuator = cursor.fetchone()
            
            if not actuator:
                raise HTTPException(status_code=404, detail="Actuator not found")

            current_status = actuator['status']
            manual_lock = actuator['manual_lock']
            act_type = actuator['type']

            if manual_lock and cmd.priority > 1:
                reason = "Blocked by Safety/Emergency Lock"
                log_control(conn, cmd.actuator_id, cmd.command, cmd.source, cmd.priority, "Blocked", reason)
                conn.commit()
                return {"status": "blocked", "reason": reason}

            interlock_result = check_interlock_rules(rules, act_type, cmd.command, conn)
            if interlock_result['blocked']:
                reason = interlock_result['reason']
                log_control(conn, cmd.actuator_id, cmd.command, cmd.source, cmd.priority, "Blocked", reason)
                conn.commit()
                return {"status": "blocked", "reason": reason}

            if current_status in ['Opening', 'Closing']:
                reason = "Actuator is currently in transition"
                log_control(conn, cmd.actuator_id, cmd.command, cmd.source, cmd.priority, "Blocked", reason)
                conn.commit()
                return {"status": "blocked", "reason": reason}

            try:
                cursor.execute("""
                    UPDATE actuator_status SET status = %s WHERE actuator_id = %s
                """, (cmd.command, cmd.actuator_id))
                
                log_control(conn, cmd.actuator_id, cmd.command, cmd.source, cmd.priority, "Success", "Command executed")
                conn.commit()
                return {"status": "success", "message": "Command executed"}
            except Exception as e:
                log_control(conn, cmd.actuator_id, cmd.command, cmd.source, cmd.priority, "Error", str(e))
                conn.commit()
                raise HTTPException(status_code=500, detail="Execution failed")
