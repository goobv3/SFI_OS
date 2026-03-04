import yaml
import os

# ---------------------------------------------------------
# 제어기 통제 및 안전 중재 모듈 (Arbitration / Interlock Module)
# ---------------------------------------------------------
# 사용자가 스마트팜의 장치(예: 난방기, 냉방기)를 켤 때,
# 동시에 켜지면 위험한 기기들이 없는지 확인하고 차단(인터락)하는 안전 장치 모듈입니다.
# ---------------------------------------------------------

def load_rules(filepath="config/interlock_rules.yaml"):
    """
    안전 규칙(Rule)이 적혀있는 YAML 설정 파일을 읽어오는 함수입니다.
    """
    if not os.path.exists(filepath):
        print(f"Warning: Rule file {filepath} not found. (규칙 파일을 찾을 수 없습니다.)")
        return {}
    try:
        # 파일을 열고 yaml 형식의 글자를 Python 딕셔너리로 변환합니다.
        with open(filepath, 'r') as file:
            return yaml.safe_load(file)
    except Exception as e:
        print(f"Error loading rules (규칙 로딩 중 에러 발생): {e}")
        return {}

def check_interlock_rules(rules, actuator_type, command, connection):
    """
    사용자의 제어 명령(command)이 안전한지 기존 데이터베이스 상태와 비교하여 평가합니다.
    
    매개변수:
    - rules: load_rules()로 불러온 안전 규칙 목록
    - actuator_type: 조작하려는 기기 종류 (예: 'HEATER', 'COOLER')
    - command: 내릴 명령 (예: 'ON', 'OFF')
    - connection: 데이터베이스 연결 객체 (현재 다른 기기가 켜져있는지 확인하기 위함)
    """
    # 규칙이 없거나 DB 연결이 끊어졌으면 통제할 수 없으므로 무조건 승인(차단 안함)합니다.
    if not rules or not connection:
        return {'blocked': False}
        
    interlocks = rules.get('interlocks', [])
    
    with connection.cursor() as cursor:
        for rule in interlocks:
            condition = rule.get('condition', '')
            
            # Rule 1: 난방기(HEATER) vs 냉방기(COOLER) 동시 작동 금지 규칙
            if "target.type == 'HEATER' and command == 'ON'" in condition:
                # 사용자가 난방기를 켜려고('ON') 할 경우
                if actuator_type == 'HEATER' and command == 'ON':
                    # DB에서 현재 냉방기가 켜져있는지 확인합니다.
                    cursor.execute("""
                        SELECT status FROM actuator_status s 
                        JOIN actuator_metadata m ON s.actuator_id = m.actuator_id 
                        WHERE m.type = 'COOLER'
                    """)
                    coolers = cursor.fetchall()
                    # 냉방기 중 하나라도 켜져('ON')있다면 위험하므로 조작을 막습니다(blocked: True).
                    if any(c.get('status') == 'ON' for c in coolers):
                        return {'blocked': True, 'reason': rule['reason']}

            # Rule 2: 냉방기(COOLER) vs 난방기(HEATER) 동시 작동 금지 규칙
            if "target.type == 'COOLER' and command == 'ON'" in condition:
                # 사용자가 냉방기를 켜려고('ON') 할 경우
                if actuator_type == 'COOLER' and command == 'ON':
                    # DB에서 현재 난방기가 켜져있는지 확인합니다.
                    cursor.execute("""
                        SELECT status FROM actuator_status s 
                        JOIN actuator_metadata m ON s.actuator_id = m.actuator_id 
                        WHERE m.type = 'HEATER'
                    """)
                    heaters = cursor.fetchall()
                    # 난방기 중 하나라도 켜져('ON')있다면 에너지가 낭비되고 고장날 수 있으므로 막습니다.
                    if any(h.get('status') == 'ON' for h in heaters):
                        return {'blocked': True, 'reason': rule['reason']}

    # 모든 규칙을 확인했는데 걸리는 것이 없다면 차단하지 않고 명령을 승인합니다.
    return {'blocked': False}
