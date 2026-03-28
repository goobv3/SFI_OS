# 하우스 및 장치 관리 라이브러리 매뉴얼 (House Library Manual) 🏠

이 문서는 스마트팜 프로젝트에서 추상화된 `house_lib.py` 라이브러리의 사용 설명서입니다.

`HouseManager` 클래스는 농장의 물리적 구역(House), 그리고 그 곳에 배치된 센서와 구동기의 기본 메타데이터(설정값, 표시 여부 등)를 데이터베이스를 통해 관리하도록 도와주는 도구입니다. 복잡한 SQL 쿼리 없이 파이썬 함수 하나로 쉽게 기기를 생성하고 삭제할 수 있습니다.

---

## 🚀 1. 빠른 시작 (Quick Start)

본 라이브러리는 `MariaDB(MySQL)` 데이터베이스 연결 객체(`conn`)를 인자로 받도록 설계되었습니다. 따라서 특정 프레임워크에 종속되지 않습니다.

```python
import pymysql
from house_lib import HouseManager

def main():
    # 1. DB 연결 (예시)
    conn = pymysql.connect(
        host='localhost', user='root', password='password', 
        database='smartfarm', cursorclass=pymysql.cursors.DictCursor
    )
    
    try:
        # 2. HouseManager 인스턴스 생성
        manager = HouseManager(conn)
        
        # --- [기능 1] 새로운 비닐 하우스 구역 등록하기 ---
        manager.create_house(house_id="Zone_A", name="제1농장 토마토구역", display_order=1)
        print("하우스 생성 완료!")
        
        # --- [기능 2] 등록된 전체 하우스 목록 보기 ---
        houses = manager.get_all_houses()
        for h in houses:
            print(f"ID: {h['house_id']}, 이름: {h['name']}")
            
        # --- [기능 3] 센서 맵핑 (센서를 하우스에 붙이기) ---
        manager.create_sensor_metadata(
            sensor_id="TEMP_001",
            house_id="Zone_A",
            alias="메인 온도센서",
            type="Temperature",
            unit="℃",
            is_active=True
        )
        
        # --- [기능 4] 구동기 맵핑 (모터를 하우스에 붙이기) ---
        # (이 함수 내부에서 자동으로 'Off' 라는 기본 안전 상태까지 세팅해줍니다)
        manager.create_actuator_metadata(
            actuator_id="MOTOR_001",
            house_id="Zone_A",
            alias="지붕 개폐기",
            type="Window"
        )
        print("기기 등록 완료!")
        
    finally:
        conn.close()

if __name__ == "__main__":
    main()
```

---

## 🌐 3. C++ 백엔드 REST API (Smart Farm OS)

본 프로젝트의 C++ 백엔드(Crow 기반)에서 제공하는 하우스 및 기기 관리 인터페이스입니다.

### 하우스(동) 관리 엔드포인트
| 기능 | 메서드 | 경로 (Endpoint) | 비고 |
|:---|:---:|:---|:---|
| 목록 조회 | GET | `/api/houses` | 전체 하우스 목록 및 메타데이터 |
| 신규 생성 | POST | `/api/metadata/houses` | `house_id`, `name`, `display_order` 포함 |
| 정보 수정 | PUT | `/api/metadata/houses/<id>` | 이름 및 표시 순서 수정 |
| 삭제 | DELETE | `/api/metadata/houses/<id>` | 해당 하우스 및 종속 기기 삭제(Cascade 아님 주의) |

### 🛠️ 라우팅 호환성 레이어 (Compatibility Layer)
기존 프론트엔드 코드와의 호환성을 위해 다음의 단축 경로(Shortcut)도 지원합니다.
- `POST /api/houses` (생성)
- `PUT /api/houses/<id>` (수정)
- `DELETE /api/houses/<id>` (삭제)

> [!TIP]
> 모든 API 응답은 `application/json` 형식이며, 성공 시 `{"status": "created/updated/deleted"}` 형태를 반환합니다.

### 기기(센서/구동기) 메타데이터 관리
- **센서 생성:** `POST /api/metadata/sensors`
- **구동기 생성:** `POST /api/metadata/actuators`
- **센서 수정:** `PUT /api/metadata/sensors/<id>` (알람 임계치 포함)

---

## 📦 4. 주요 기능 및 메서드 목록 (개발자용)

### 하우스 (구역) 관리
- `get_all_houses()`: 딕셔너리 리스트 반환 (display_order 순 정렬)
- `create_house(house_id, name, display_order)`
- `update_house(house_id, name, display_order)`
- `delete_house(house_id)`

### 메타데이터 (기기 설정) 관리
- `get_house_devices(house_id)`:
  특정 하우스에 종속된 기기 전체를 반환합니다. 
  반환 구조: `{"sensors": [...], "actuators": [...]}`
  
- `create_sensor_metadata(sensor_id, house_id, alias, type, unit, ...)`
- `update_sensor_metadata(sensor_id, alias, type, unit, warn_high, crit_high ...)`: 알람 발생 기준(임계치)도 함께 수정할 수 있습니다.
- `delete_sensor_metadata(sensor_id)`

- `create_actuator_metadata(actuator_id, house_id, alias, type)`: DB에 안전 장치 생성 보장 로직 내장.
- `update_actuator_metadata(actuator_id, alias, type)`
- `delete_actuator_metadata(actuator_id)`

### 미등록 기기 (디스커버리 탭) 관리
시스템에 무단으로 전송되는 센서값들은 자동 격리되어 관리됩니다.
- `get_discovered_devices()`: 센서 ID를 모르는 기기 목록을 확인합니다.
- `remove_discovered_device(device_id)`: 무시할 기기 캐시를 날립니다.
