import requests
import random
import time
from datetime import datetime

API_URL = "http://localhost:8000/api"

def generate_sensor_data():
    sensors = [
        {"id": "TEMP_01", "base": 22.0, "variance": 1.5},
        {"id": "HUM_01", "base": 65.0, "variance": 5.0},
        {"id": "CO2_01", "base": 400.0, "variance": 20.0},
        {"id": "LIGHT_01", "base": 12000.0, "variance": 1000.0}
    ]
    
    print(f"[{datetime.now()}] Generating sensor data...")
    for s in sensors:
        val = s["base"] + random.uniform(-s["variance"], s["variance"])
        payload = {
            "sensor_id": s["id"],
            "value": round(val, 2)
        }
        try:
            res = requests.post(f"{API_URL}/sensors", json=payload)
            print(f"  -> {s['id']}: {val:.2f} | Status: {res.status_code}")
        except Exception as e:
            print(f"  -> {s['id']}: Error - {e}")

def test_interlock_logic():
    print(f"\n[{datetime.now()}] Testing Arbitration Logic...")
    
    test_cases = [
        # 1. Turn on Heater (Should succeed if Cooler is Off)
        {"id": "HEATER_1", "cmd": "ON", "src": "Auto", "pri": 3, "expected": "success"},
        
        # 2. Try to turn on Cooler while Heater is ON (Should be Blocked by Interlock)
        {"id": "COOLER_1", "cmd": "ON", "src": "Manual", "pri": 2, "expected": "blocked"},
        
        # 3. Turn off Heater
        {"id": "HEATER_1", "cmd": "OFF", "src": "Manual", "pri": 2, "expected": "success"},
        
        # 4. Turn on Cooler (Should succeed now since Heater is OFF)
        {"id": "COOLER_1", "cmd": "ON", "src": "Manual", "pri": 2, "expected": "success"}
    ]
    
    for case in test_cases:
        payload = {
            "actuator_id": case["id"],
            "command": case["cmd"],
            "source": case["src"],
            "priority": case["pri"]
        }
        try:
            res = requests.post(f"{API_URL}/control", json=payload)
            data = res.json()
            status = data.get("status")
            print(f"  -> Command {case['id']} {case['cmd']} | Expected: {case['expected']} | Actual: {status} | Reason: {data.get('reason', '')}")
            time.sleep(1) # Sleep to allow DB sequence 
        except Exception as e:
            print(f"  -> Error: {e}")

if __name__ == "__main__":
    for i in range(3):
        generate_sensor_data()
        time.sleep(2)
        
    test_interlock_logic()
    
    print("\nTest completed.")
