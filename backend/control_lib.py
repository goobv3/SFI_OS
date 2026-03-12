# ---------------------------------------------------------
# 스마트팜 구동기 제어 및 안전 라이브러리 (Control & Interlock Library)
# ---------------------------------------------------------
# 이 모듈은 사용자가 펌프, 모터, 팬 등의 구동기(Actuator)를 제어하려 할 때
# 즉각적으로 명령을 실행하지 않고, 사전에 정의된 안전 규칙(Interlock Rules)을
# 검사하여 하드웨어 파손을 막아주는 핵심 제어 라이브러리입니다.
# ---------------------------------------------------------
import os
import paho.mqtt.publish as mqtt_publish

class ControlManager:
    def __init__(self, db_params_or_conn=None, rules: list = None):
        """
        ControlManager 객체를 초기화합니다.
        
        :param db_conn: 데이터베이스 연결 객체 (MySQL/MariaDB 등)
        :param rules: 제어 충돌을 방지하기 위한 안전(Interlock) 규칙 리스트 
                      (참고: 주로 arbitration.py 에서 YAML 파일을 읽어와 전달합니다)
        """
        self.conn = db_params_or_conn
        self.rules = rules if rules is not None else []

    def process_control_command(self, actuator_id: str, command: str, source: str = "Manual", priority: int = 2):
        """
        사용자 또는 자동화 시스템이 발송한 구동기 제어 명령을 안전하게 검사하고 실행합니다.
        이 함수 내부에서 수동 잠금(Manual Lock), 구동 중(Transitioning) 상태, 
        그리고 YAML 기반 인터락 규칙(Interlock Rules) 검사를 모두 거쳐 사고를 방지합니다.
        
        :param actuator_id: 제어할 대상 구동기의 고유 ID (예: 'RELAY_HEATER_1')
        :param command: 실행할 명령 ('Open', 'Close', 'Stop', 'On', 'Off' 등)
        :param source: 명령 출처를 식별하는 태그 ('Manual', 'Auto_Script', 'Emergency')
        :param priority: 시스템 명령 우선순위 
                         [1: 관리자/긴급, 2: 일반 사용자 버튼, 3: 타이머/자동화 로직]
        :return: 제어 결과 상태 딕셔너리 (예: {'status': 'success'}, 
                 차단 시 {'status': 'blocked', 'reason': '차단 사유'})
        :raises ValueError: 데이터베이스에 존재하지 않는 구동기 ID일 때
        :raises RuntimeError: 제어 내역 업데이트 트랜잭션 도중 DB 에러 발생 시
        """
        with self.conn.cursor() as cursor:
            # 1. 대상 구동기가 존재하는지 및 현재 상태 스캔
            cursor.execute("""
                SELECT s.status, s.manual_lock, m.type 
                FROM actuator_status s
                JOIN actuator_metadata m ON s.actuator_id = m.actuator_id
                WHERE s.actuator_id = %s
            """, (actuator_id,))
            actuator = cursor.fetchone()
            
            if not actuator:
                raise ValueError("Actuator not found")

            current_status = actuator['status']
            manual_lock = actuator['manual_lock']
            act_type = actuator['type']

            # 2. 강제 록(Lock) 검사: 
            # 누군가 기계를 수리하느라 화면에서 '수동 잠금(manual_lock)'을 켜두었다면, 
            # 최고 우선순위(1)가 아닌 일반 명령(2,3)은 무조건 차단합니다.
            if manual_lock and priority > 1:
                reason = "Blocked by Safety/Emergency Lock"
                self._log_control(cursor, actuator_id, command, source, priority, "Blocked", reason)
                self.conn.commit()
                return {"status": "blocked", "reason": reason}

            # 3. 트랜지션(이동 중) 방어 로직:
            # 모터가 현재 열리는 도중(Opening)이거나 닫히는 도중(Closing) 이라면 충돌 방지를 위해 명령 차단
            if current_status in ['Opening', 'Closing']:
                reason = "Actuator is currently in transition"
                self._log_control(cursor, actuator_id, command, source, priority, "Blocked", reason)
                self.conn.commit()
                return {"status": "blocked", "reason": reason}

            # 4. 인터락 룰(Interlock Rules) 위반 검사:
            # "비가 내릴 때 지붕을 열지 마라", "A모터가 돌 때 B히터를 틀지 마라" 
            interlock_result = self._check_interlock_rules(act_type, command, cursor)
            if interlock_result['blocked']:
                reason = interlock_result['reason']
                self._log_control(cursor, actuator_id, command, source, priority, "Blocked", reason)
                self.conn.commit()
                return {"status": "blocked", "reason": reason}

            # 5. 모든 안전 검사를 무사히 통과함 -> 진짜 제어 상태로 DB 업데이트 수행
            try:
                cursor.execute("""
                    UPDATE actuator_status SET status = %s WHERE actuator_id = %s
                """, (command, actuator_id))
                
                self._log_control(cursor, actuator_id, command, source, priority, "Success", "Command executed")
                self.conn.commit()
                
                # MQTT 브로커로 제어 명령(Publish) 즉각 발송 (Fire-and-forget)
                try:
                    mqtt_host = os.getenv("MQTT_HOST", "mosquitto")
                    topic = f"smartfarm/actuators/{actuator_id}/command"
                    mqtt_publish.single(topic, command, hostname=mqtt_host, port=1883)
                except Exception as mqtt_e:
                    print(f"[MQTT] Warning: Failed to publish control command: {mqtt_e}")
                    
                return {"status": "success", "message": "Command executed"}
                
            except Exception as e:
                self._log_control(cursor, actuator_id, command, source, priority, "Error", str(e))
                self.conn.commit()
                raise RuntimeError(f"Execution failed: {e}")

    def _log_control(self, cursor, actuator_id, command, source, priority, result, message):
        """
        (내부 함수) 접수된 모든 제어 명령의 처리 결과(성공여부, 차단사유 등)를
        데이터베이스(`control_logs`)에 영구 기록합니다.

        :param cursor: DB 통신 커서
        :param actuator_id: 제어 대상 구동기 ID
        :param command: 시도했던 제어 명령
        :param source: 명령 출처
        :param priority: 명령의 우선권 레벨
        :param result: 최종 승인(Success)/거절(Blocked)/오류(Error) 여부
        :param message: 사용자에게 보여줄 로그 세부 내용
        """
        cursor.execute("""
            INSERT INTO control_logs (actuator_id, command, source, priority, result, message)
            VALUES (%s, %s, %s, %s, %s, %s)
        """, (actuator_id, command, source, priority, result, message))

    # ==========================
    # 인터락(안전 교차점검) 엔진 코어
    # ==========================
    def _check_interlock_rules(self, req_actuator_type, req_command, cursor):
        """
        (내부 함수) 메모리에 적재된 인터락(안전 교차점검) 규칙들을 순회하며,
        현재 DB에 저장된 다른 기기들의 상태와 비교 분석하여 위험 요소가 있는지 판별합니다.
        
        :param req_actuator_type: 제청하려는 구동기의 하드웨어 유형 (예: 'Window', 'Heater')
        :param req_command: 신청한 제어 명령 (예: 'Open')
        :param cursor: DB 통신 커서
        :return: {'blocked': True/False, 'reason': 문자열 혹은 None}
        """
        for rule in self.rules:
            # 만약 룰에 타겟 구동기 타입이 지정되어 있고, 내가 요청한 타입과 다르다면 패스
            if 'target_device_type' in rule and req_actuator_type not in rule['target_device_type']:
                continue
                
            # 만약 타겟 커맨드가 규칙에 명시되어 있고, 내가 요청한 커맨드가 아니라면 패스
            if 'target_command' in rule and req_command not in rule['target_command']:
                continue
                
            # 조건 덩어리들을 확인
            conditions_matched = True
            for condition in rule.get('conditions', []):
                cond_type = condition.get('type')
                
                # 1) 다른 장치(actuator) 상태 검사 조건
                if cond_type == 'device_state':
                    check_type = condition['device_type']
                    avoid_state = condition['state']
                    
                    cursor.execute("""
                        SELECT s.status 
                        FROM actuator_status s
                        JOIN actuator_metadata m ON s.actuator_id = m.actuator_id
                        WHERE m.type = %s
                    """, (check_type,))
                    
                    rows = cursor.fetchall()
                    state_found = False
                    for row in rows:
                        # 규칙에 정의된 '하면 안되는 상태'인 기기가 하나라도 속해있으면 발동
                        if row['status'] == avoid_state:
                            state_found = True
                            break
                    
                    if not state_found:
                        conditions_matched = False
                        break
                        
                # 2) 다른 특정 조건 방식이 나중에 추가된다면 이 아래로 else if 로 확장
            
            # 모든 conditions_matched 가 여전히 True 라면 -> 이 규칙을 완벽하게 위반했다는 뜻!
            if conditions_matched:
                return {'blocked': True, 'reason': f"Interlock rule violation: {rule['name']}"}
                
        # 아무 룰에도 걸리지 않았음
        return {'blocked': False, 'reason': None}
