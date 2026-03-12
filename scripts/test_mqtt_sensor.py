import time
import random
import paho.mqtt.client as mqtt

broker_address = "localhost" # Docker 밖(HostPC)에서 1883 포트로 쏘면 컨테이너가 매핑받음
port = 1883

print(f"Connecting to MQTT broker at {broker_address}:{port}...")

client = mqtt.Client(client_id="mock_arduino_sensor")
try:
    client.connect(broker_address, port=port)
    client.loop_start()
    print("Connected! Mock Arduino is now publishing data every 5 seconds.")
    print("Press Ctrl+C to stop.\n")
    
    temp = 25.0
    humid = 60.0
    
    while True:
        # 워크(Random walk) 로직
        temp = max(10.0, min(40.0, temp + random.uniform(-0.5, 0.5)))
        humid = max(30.0, min(90.0, humid + random.uniform(-1.0, 1.0)))
        
        t_topic = "smartfarm/sensors/TEMP_01/value"
        h_topic = "smartfarm/sensors/HUM_01/value"
        
        client.publish(t_topic, str(round(temp, 1)))
        client.publish(h_topic, str(round(humid, 1)))
        
        print(f"[Publish] {t_topic} -> {round(temp, 1)}")
        print(f"[Publish] {h_topic} -> {round(humid, 1)}")
        
        time.sleep(5)
        
except KeyboardInterrupt:
    print("Stopping mock sensor...")
except Exception as e:
    print(f"Error: {e}")
finally:
    client.loop_stop()
    client.disconnect()
