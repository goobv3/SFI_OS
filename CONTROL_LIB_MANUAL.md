# 구동기 제어 및 안전 인터락 설정 라이브러리 매뉴얼 (Control & Interlock Library Manual) ⚙️

이 문서는 스마트팜 프로젝트에서 분리된 `control_lib.py` 라이브러리의 사용 설명서입니다.

`ControlManager` 클래스는 사용자가 모터, 펌프, 히터 등 하드웨어를 제어하도록 명령을 내릴 때, 기계가 파손되거나 위험한 상황이 발생하지 않도록 **사전에 안전 규칙(Interlock Rules)을 검사한 뒤 제어 상태를 업데이트**하는 스마트한 보호회로 모듈입니다.

---

## 🚀 1. 빠른 시작 (Quick Start)

본 라이브러리는 `MariaDB(MySQL)` 데이터베이스 연결 객체(`conn`)와, 사전에 정의해둔 인터락 규칙(YAML 등 파싱된 리스트)를 인자로 받아 초기화합니다.

```python
import pymysql
import yaml
from control_lib import ControlManager

def main():
    # 1. DB 연결 (예시)
    conn = pymysql.connect(
        host='localhost', user='root', password='password', 
        database='smartfarm', cursorclass=pymysql.cursors.DictCursor
    )
    
    # 2. 안전 규칙 세트(YAML) 로드 (예시)
    rules_text = """
    - name: "비가 올 때 창문 열림 방지"
      target_device_type: ["Window"]
      target_command: ["Open"]
      # 조건: 비 감지 센서가 알림 상태일 때 창문 금지 등의 룰 작성 가능
    """
    rules = yaml.safe_load(rules_text)
    
    try:
        # 3. ControlManager 인스턴스 생성
        manager = ControlManager(conn, rules)
        
        # --- [기능 1] 구동기 제어 명령 내리기 ---
        # 사용자가 화면에서 'MOTOR_001' (창문 개폐기)를 열어라('Open')고 버튼을 눌렀을 때
        result = manager.process_control_command(
            actuator_id="MOTOR_001", 
            command="Open", 
            source="UserInterface", 
            priority=2   # 일반 사용자 명령순위
        )
        
        # 4. 결과 출력
        if result['status'] == 'success':
            print("명령이 데이터베이스에 정상적으로 전달되었습니다.")
        elif result['status'] == 'blocked':
            # 만약 현재 비가 와서 인터락 규칙에 걸렸거나, 기가게 수리중(Locked) 상태라면?
            print(f"안전 장치에 의해 명령이 차단되었습니다! 이유: {result['reason']}")
            
    finally:
        conn.close()

if __name__ == "__main__":
    main()
```

---

## 📦 2. 주요 기능 상세 및 안전장치 메커니즘

`process_control_command(actuator_id, command, source, priority)` 메서드는 내부적으로 아래의 **5단계 엄격한 안전망**을 거칩니다.

1. **대상 탐색:** 해당 `actuator_id`가 DB상에 존재하는 진짜 기계인지, 그리고 무슨 타입(모터인지 펌프인지)인지 검사합니다.
2. **수동 강제 잠금(Manual Lock) 검사:**
   현장 작업자가 '기계 수리중' 표시를 켜놓았다면(`manual_lock=1`), 최고 관리자 권한(우선순위 1) 명령을 제외한 모든 앱, 스크립트, 자동화 로직(우선순위 2,3)의 제어 명령이 전면 차단됩니다.
3. **작동 중 충돌(Transition) 검사:**
   만약 거대한 천장 모터가 현재 '열리는 중(Opening)'인데 반대로 '닫혀라(Close)'라는 명령이 바로 날아오면 기어가 부서질 수 있습니다. 라이브러리가 이것을 인지하고 현재 기계가 이동 중이면 새 명령을 잠시 무시합니다.
4. **인터락 룰(Interlock Rules) 검사 엔진:**
   로딩된 룰(`self.rules`)과 비교하여 "A가 켜져있을 때 B를 켜면 안된다"는 논리조건을 검색해 매칭되면 셧다운합니다.
5. **DB 업데이트 및 로깅 기록:**
   위의 1~4단계를 무사히 통과했다면 실제로 DB 상태를 업데이트하고, 이 성공했다는 내역 자체를 시간과 함께 로그(Log) DB에 영구 기록합니다. 만약 2,3,4번에서 차단당했다면 "차단 당함" 과 "차단당한 이유" 까지 빠짐없이 로그에 남깁니다.
