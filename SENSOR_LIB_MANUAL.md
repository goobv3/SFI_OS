# 센서 데이터 및 알람 처리 라이브러리 매뉴얼 (Sensor Library Manual) 🌡️

이 문서는 스마트팜 프로젝트에서 분리된 `sensor_lib.py` 라이브러리의 사용 방법 안내서입니다.

`SensorManager` 클래스는 농장에서 발생하는 수많은 센서(온도, 습도, 토양 수분 등) 데이터를 데이터베이스에 기록하고, 기록과 동시에 사전에 설정된 **알람 임계치(위험/경고)** 를 초과했는지 자동으로 검사해 알람을 띄워주는 역할을 수행합니다. 더불어, 차트를 그리기 위한 복잡한 통계 데이터(일간/월간/연간)도 제공합니다.

---

## 🚀 1. 빠른 시작 (Quick Start)

본 라이브러리는 `MariaDB(MySQL)` 데이터베이스 연결 객체(`conn`)를 인자로 받아 동작합니다. 

```python
import pymysql
from datetime import datetime, timedelta
from sensor_lib import SensorManager

def main():
    # 1. DB 연결
    conn = pymysql.connect(
        host='localhost', user='root', password='password', 
        database='smartfarm', cursorclass=pymysql.cursors.DictCursor
    )
    
    try:
        manager = SensorManager(conn)
        
        # --- [기능 1] 센서 데이터 수집 및 알람 검사 ---
        # 아두이노나 라즈베리파이에서 'TEMP_001' 센서의 값이 '35.5도' 로 들어왔을 때
        result = manager.process_incoming_data(sensor_id="TEMP_001", value=35.5)
        print("수집 결과:", result['message'])
        # 내부 로직: 만약 이 값이 위험 수치(예: 35도)를 넘었다면 자동으로 Alarms 테이블에 경고를 등록함.
        
        # --- [기능 2] 아직 관리자가 확인하지 않은 미확인 알람 보기 ---
        alarms = manager.get_unacknowledged_alarms()
        for a in alarms:
            print(f"[경고 ⚠️] {a['level']} - {a['message']} (발생시간: {a['created_at']})")
            
            # --- [기능 3] 알람 확인 처리 (더이상 화면에 띄우지 않음) ---
            manager.acknowledge_alarm(alarm_id=a['id'])
            print(f"알람 ID {a['id']} 확인 처리 완료.")
            
        # --- [기능 4] 차트용 그룹화된 센서 통계 데이터 뽑기 (예: 지난 7일간 데이터) ---
        end_date = datetime.now()
        start_date = end_date - timedelta(days=7)
        
        chart_data = manager.get_custom_range_history(
            sensor_ids=["TEMP_001", "HUMID_001"], 
            start_dt=start_date, 
            end_dt=end_date
        )
        print("차트 데이터:", chart_data)
        
    finally:
        conn.close()

if __name__ == "__main__":
    main()
```

---

## 📦 2. 주요 기능 및 메서드 목록

### 📡 데이터 수집 및 자동 알람 엔진
`process_incoming_data(sensor_id, value)`
이 함수 하나가 호출될 때 내부적으로 다음과 같은 일들이 순식간에 일어납니다:
1. 해당 센서가 우리 시스템에 등록된 센서인지 확인.
2. 수집 버튼이 꺼져있으면(`is_active=False`) 데이터 저장 스킵.
3. 데이터(`value`)를 DB에 안전하게 적재.
4. 만약 이 센서에 경고(`warn_high/low`) 혹은 위험(`crit_high/low`) 임계치가 설정되어 있고, 그 값을 벗어났다면 즉각 `alarms` DB 테이블에 경고 메시지를 기록. (동일한 수준의 미확인 경고가 이미 떠있다면 중복 생성 방지 기능 포함)
5. 만약 전혀 모르는 낯선 센서가 접속했다면, 미등록 기기함(`unregistered_devices`)으로 자동으로 격리.

### ⏰ 관리자 알람 센터
- `get_unacknowledged_alarms()`: 사용자가 아직 "확인" 버튼을 누르지 않은 현재 떠있는 알람 목록 반환.
- `acknowledge_alarm(alarm_id)`: 알람 ID를 받아 "확인됨(is_acknowledged=True)"으로 변경.

### 📈 차트 및 통계 데이터 엔진
- `get_aggregate_history(sensor_id, period)`
  - 단일 센서의 정해진 기간(`daily`, `monthly`, `yearly`) 평균 데이터를 가져옵니다.
- `get_custom_range_history(sensor_ids_list, start_datetime, end_datetime)`
  - **(강력함)** 여러 개의 센서 ID 리스트를 받아, 시작 시점과 종료 시점 사이의 데이터를 동적 버킷(1분/5분/1시간 단위 등)으로 묶어 여러 센서를 하나의 시간에 합쳐서(Pivot) 보여줍니다. UI 차트(Recharts 등) 라이브러리에 바로 던져주면 그려지는 형태의 딕셔너리를 반환합니다.
