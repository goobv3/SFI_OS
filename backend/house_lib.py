# ---------------------------------------------------------
# 스마트팜 농장(구역) 및 장치 관리 라이브러리 (House & Device Library)
# ---------------------------------------------------------
# 이 모듈은 농장 구획(House/Zone)을 생성하고,
# 해당 구역에 배치되는 센서(Sensor)와 구동기(Actuator)의 설정값(Metadata),
# 그리고 새로 발견된 미등록 장치함(Discovery Inbox) 관리 로직을 독립시킨 범용 라이브러리입니다.
# ---------------------------------------------------------

class HouseManager:
    def __init__(self, db_conn):
        """
        :param db_conn: 데이터베이스 연결 객체 (yield conn 으로 넘어온 객체 등)
        """
        self.conn = db_conn

    # ==========================
    # 하우스(구역) 파트
    # ==========================
    def get_all_houses(self):
        """
        데이터베이스에 저장된 모든 하우스(구역) 목록을 조회합니다.
        표시 순서(display_order) 기준으로 오름차순 정렬되어 반환됩니다.
        
        :return: 하우스 정보가 담긴 딕셔너리 리스트 (예: [{'house_id': '...', 'name': '...'}, ...])
        """
        with self.conn.cursor() as cursor:
            cursor.execute("SELECT house_id, name, display_order, created_at FROM houses ORDER BY display_order ASC")
            return cursor.fetchall()

    def create_house(self, house_id: str, name: str, display_order: int = 0):
        """
        새로운 하우스(온실 구역)를 데이터베이스에 생성합니다.

        :param house_id: 하우스의 고유 영문 ID (예: 'ZONE_A')
        :param name: 화면에 표시될 하우스의 한글 이름 (예: '제1농장 토마토구역')
        :param display_order: 화면 정렬 순서 (숫자가 작을수록 먼저 표시됨)
        """
        with self.conn.cursor() as cursor:
            cursor.execute("INSERT INTO houses (house_id, name, display_order) VALUES (%s, %s, %s)", 
                           (house_id, name, display_order))
            self.conn.commit()

    def update_house(self, house_id: str, name: str, display_order: int):
        """
        기존 하우스의 이름이나 정렬 순서를 변경합니다.

        :param house_id: 수정할 대상 하우스의 고유 ID
        :param name: 변경할 새 이름
        :param display_order: 변경할 새 정렬 순서
        """
        with self.conn.cursor() as cursor:
            cursor.execute("UPDATE houses SET name = %s, display_order = %s WHERE house_id = %s", 
                           (name, display_order, house_id))
            self.conn.commit()

    def delete_house(self, house_id: str):
        """
        특정 하우스를 데이터베이스에서 완전히 삭제합니다.
        (경고: 하우스에 소속된 센서/구동기도 종속성 설정에 따라 함께 지워질 수 있습니다)

        :param house_id: 삭제할 대상 하우스의 고유 ID
        """
        with self.conn.cursor() as cursor:
            cursor.execute("DELETE FROM houses WHERE house_id = %s", (house_id,))
            self.conn.commit()

    # ==========================
    # 메타데이터 (설정) 파트
    # ==========================
    def get_house_devices(self, house_id: str):
        """
        특정 하우스에 소속된 모든 센서와 구동기의 설정 정보(메타데이터) 및 현재 상태를 가져옵니다.

        :param house_id: 장치 목록을 조회할 하우스의 고유 ID
        :return: 'sensors'와 'actuators' 리스트를 포함하는 딕셔너리
                 (예: {'sensors': [{'sensor_id': 'TEMP_1', ...}], 'actuators': [...]})
        """
        with self.conn.cursor() as cursor:
            cursor.execute("SELECT * FROM sensor_metadata WHERE house_id = %s ORDER BY display_order ASC", (house_id,))
            sensors = cursor.fetchall()
            
            cursor.execute("""
                SELECT am.*, ast.status, ast.target_value, ast.manual_lock 
                FROM actuator_metadata am
                LEFT JOIN actuator_status ast ON am.actuator_id = ast.actuator_id
                WHERE am.house_id = %s
            """, (house_id,))
            actuators = cursor.fetchall()
            
            return {"sensors": sensors, "actuators": actuators}

    # -- 센서 메타데이터 --
    def create_sensor_metadata(self, sensor_id: str, house_id: str, alias: str, type: str, unit: str, display_order: int = 0, is_active: bool = True):
        """
        새로운 센서를 특정 하우스에 등록(매핑)합니다.

        :param sensor_id: 하드웨어에서 전송하는 센서의 실제 고유 ID (예: 'ESP32_TEMP_01')
        :param house_id: 이 센서를 설치할 하우스 ID
        :param alias: 화면에 보여질 센서의 별명 (예: '1구역 내부 온도')
        :param type: 센서 유형 (예: 'Temperature', 'Humidity')
        :param unit: 측정 단위 (예: '℃', '%')
        :param display_order: 화면 배치 순서
        :param is_active: 데이터 수집 활성화 여부 (False면 데이터가 들어와도 무시함)
        """
        with self.conn.cursor() as cursor:
            cursor.execute("""
                INSERT INTO sensor_metadata (sensor_id, house_id, alias, type, unit, display_order, is_active)
                VALUES (%s, %s, %s, %s, %s, %s, %s)
            """, (sensor_id, house_id, alias, type, unit, display_order, is_active))
            self.conn.commit()

    def update_sensor_metadata(self, sensor_id: str, alias: str, type: str, unit: str, display_order: int, is_active: bool,
                               warn_high=None, warn_low=None, crit_high=None, crit_low=None):
        """
        기존에 등록된 센서의 설정값을 수정합니다. 
        알람 발생 기준점(Threshold) 지정도 여기서 수행합니다.

        :param sensor_id: 수정할 대상 센서 ID
        :param alias: 새 별명
        :param type: 새 유형
        :param unit: 새 측정 단위
        :param display_order: 새 정렬 순서
        :param is_active: 활성화 여부 토글
        :param warn_high: 경고(주의)단계 상한치 (이 숫자를 넘으면 Warning 알람 발생)
        :param warn_low: 경고(주의)단계 하한치
        :param crit_high: 위험단계 상한치 (이 숫자를 넘으면 Critical 알람 발생)
        :param crit_low: 위험단계 하한치
        """
        with self.conn.cursor() as cursor:
            cursor.execute("""
                UPDATE sensor_metadata 
                SET alias = %s, type = %s, unit = %s, display_order = %s, is_active = %s,
                    warn_high = %s, warn_low = %s, crit_high = %s, crit_low = %s 
                WHERE sensor_id = %s
            """, (alias, type, unit, display_order, is_active, warn_high, warn_low, crit_high, crit_low, sensor_id))
            self.conn.commit()

    def delete_sensor_metadata(self, sensor_id: str):
        """
        특정 센서를 시스템에서 제거합니다.

        :param sensor_id: 삭제할 대상 센서 ID
        """
        with self.conn.cursor() as cursor:
            cursor.execute("DELETE FROM sensor_metadata WHERE sensor_id = %s", (sensor_id,))
            self.conn.commit()

    # -- 구동기 메타데이터 --
    def create_actuator_metadata(self, actuator_id: str, house_id: str, alias: str, type: str):
        """
        새로운 구동기(모터, 펌프 등)를 특정 하우스에 등록합니다.
        메타데이터 등록 시 구동기의 초기 현재 상태 레코드(Off)도 안전을 위해 자동 생성됩니다.

        :param actuator_id: 하드웨어 고유 제어명 (예: 'RELAY_ROLL_UP')
        :param house_id: 구동기를 설치할 하우스 ID
        :param alias: 구동기 화면 별칭 (예: '좌측 지붕 개폐기')
        :param type: 구동기 부품 유형 (예: 'Window', 'Pump', 'Heater')
        """
        with self.conn.cursor() as cursor:
            cursor.execute("""
                INSERT INTO actuator_metadata (actuator_id, house_id, alias, type)
                VALUES (%s, %s, %s, %s)
            """, (actuator_id, house_id, alias, type))
            # 구동기는 메타데이터가 생길 때 기본 상태(Off)도 무조건 같이 생성해 주어야 안전합니다.
            cursor.execute("""
                INSERT IGNORE INTO actuator_status (actuator_id, status)
                VALUES (%s, 'Off')
            """, (actuator_id,))
            self.conn.commit()

    def update_actuator_metadata(self, actuator_id: str, alias: str, type: str):
        """
        기존에 등록된 구동기의 이름이나 종류 설정을 변경합니다.

        :param actuator_id: 대상 구동기 ID
        :param alias: 새 별칭
        :param type: 새 기계 유형
        """
        with self.conn.cursor() as cursor:
            cursor.execute("UPDATE actuator_metadata SET alias = %s, type = %s WHERE actuator_id = %s", 
                           (alias, type, actuator_id))
            self.conn.commit()

    def delete_actuator_metadata(self, actuator_id: str):
        """
        구동기 메타데이터를 시스템에서 제거합니다.

        :param actuator_id: 삭제 대상 구동기 ID
        """
        with self.conn.cursor() as cursor:
            cursor.execute("DELETE FROM actuator_metadata WHERE actuator_id = %s", (actuator_id,))
            self.conn.commit()

    # ==========================
    # 디스커버리 (미등록 기기함) 파트
    # ==========================
    def get_discovered_devices(self):
        """
        우리 농장에 공식 등록되지는 않았으나 통신망을 통해 데이터가 날아와 
        자동으로 격리수용(Discovery Inbox)된 장치들의 목록을 가져옵니다.
        
        :return: 미등록 기기 딕셔너리 리스트 
        """
        with self.conn.cursor() as cursor:
            cursor.execute("SELECT device_id, device_type, first_seen, last_seen, last_value FROM unregistered_devices ORDER BY last_seen DESC")
            return cursor.fetchall()
            
    def remove_discovered_device(self, device_id: str):
        """
        고장나서 노이즈를 보내거나 남의 농장 장치처럼 쓸모없는 미등록 기기 목록을
        격리함(Discovery Inbox)에서 삭제합니다.

        :param device_id: 삭제할 대상의 실제 장치 ID
        """
        with self.conn.cursor() as cursor:
            cursor.execute("DELETE FROM unregistered_devices WHERE device_id = %s", (device_id,))
            self.conn.commit()
