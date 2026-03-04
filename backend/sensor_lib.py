# ---------------------------------------------------------
# 스마트팜 센서 및 알람 처리 라이브러리 (Sensor & History Library)
# ---------------------------------------------------------
# 이 모듈은 실제 센서 하드웨어(Arduino, ESP32 등)로부터 전송된
# 로우(Raw) 데이터를 정제하여 데이터베이스에 기록하고,
# 지정된 임계치(Threshold)를 초과할 경우 알람 이벤트를 발생시키는
# 자동화된 데이터 처리 엔진입니다. 더불어 일/월/연 단위의
# 복잡한 센서 이력(History) 통계값 구하기 기능도 한 곳에 모아두었습니다.
# ---------------------------------------------------------
from fastapi import HTTPException # For API error parity, can be swapped for ValueError in raw python

class SensorManager:
    def __init__(self, db_conn):
        self.conn = db_conn

    # ==========================
    # 1. 센서 데이터 수집 엔진
    # ==========================
    def process_incoming_data(self, sensor_id: str, value: float):
        """
        하드웨어(아두이노 등)에서 날아온 실시간 센서 측정값을 분석하고 DB에 기록합니다.
        기록과 동시에 해당 센서의 임계치(위험/경고)를 초과했는지 검사하여 자동으로 알람을 생성합니다.

        :param sensor_id: 측정값을 보낸 센서의 고유 ID (예: 'TEMP_01')
        :param value: 센서가 측정한 실제 값 (실수형)
        :return: 데이터 처리 결과 상태를 알리는 딕셔너리 (예: {'status': 'success', 'message': '...'})
        """
        with self.conn.cursor() as cursor:
            # 1. 센서가 우리 농장에 등록된 진짜 센서인지 & 작동 중(is_active)인지 확인
            cursor.execute("SELECT is_active, alias, warn_high, warn_low, crit_high, crit_low FROM sensor_metadata WHERE sensor_id = %s", (sensor_id,))
            meta = cursor.fetchone()
            
            # 메타데이터는 있으나 비활성화 상태면 무시
            if meta and meta.get('is_active') == 0:
                return {"status": "skipped", "message": "Sensor data collection is paused (is_active=0)"}

            try:
                # 2. 정상 센서면 이력(History) 테이블에 추가
                cursor.execute("""
                    INSERT INTO sensors (sensor_id, value)
                    VALUES (%s, %s)
                """, (sensor_id, value))
                
                # 혹시 미등록 (Discovery) 상태였던 녀석이면, 이제 정식 등록되었으니 미등록함에서 삭제
                cursor.execute("DELETE FROM unregistered_devices WHERE device_id = %s", (sensor_id,))
                
                # 3. 알람(경고/위험) 조건 검사
                if meta:
                    self._evaluate_alarms(cursor, sensor_id, value, meta)
                    
                self.conn.commit()
                return {"status": "success", "message": "Sensor data ingested"}
                
            except Exception as e:
                # 외래키 제약조건 위반 등 -> 아예 모르는 센서가 접근함 -> 미등록(Discovery) 함으로 유배
                cursor.execute("""
                    INSERT INTO unregistered_devices (device_id, device_type, last_value)
                    VALUES (%s, 'sensor', %s)
                    ON DUPLICATE KEY UPDATE last_seen = CURRENT_TIMESTAMP, last_value = %s
                """, (sensor_id, value, value))
                self.conn.commit()
                return {"status": "accepted", "message": "Unknown sensor added to Discovery Inbox"}

    def _evaluate_alarms(self, cursor, sensor_id: str, value: float, meta: dict):
        """
        (내부 함수) 주어진 센서값이 설정된 임계치(Threshold)를 넘었는지 평가하여
        위험(Critical) 또는 경고(Warning) 알람을 생성합니다.
        동일한 센서에 대해 동일한 레벨의 알람이 이미 떠있고 관리자가 확인하지 않았다면
        알람 도배를 막기 위해 추가 생성하지 않습니다.

        :param cursor: DB 통신 커서
        :param sensor_id: 측정 센서 ID
        :param value: 측정값
        :param meta: 해당 센서의 메타데이터 (설정 정보 딕셔너리)
        """
        level = None
        message = ""
        alias = meta['alias'] or sensor_id
        
        # 위험(Critical) 단계부터 검사
        if meta['crit_high'] is not None and value >= meta['crit_high']:
            level = 'Critical'
            message = f"{alias} exceeds critical high threshold: {value}"
        elif meta['crit_low'] is not None and value <= meta['crit_low']:
            level = 'Critical'
            message = f"{alias} below critical low threshold: {value}"
        # 경고(Warning) 단계 검사
        elif meta['warn_high'] is not None and value >= meta['warn_high']:
            level = 'Warning'
            message = f"{alias} exceeds warning high threshold: {value}"
        elif meta['warn_low'] is not None and value <= meta['warn_low']:
            level = 'Warning'
            message = f"{alias} below warning low threshold: {value}"
            
        if level:
            # 아직 운영자가 확인안한 동일 센서/동일 레벨의 알람이 떠있나 확인
            cursor.execute("SELECT id FROM alarms WHERE sensor_id = %s AND level = %s AND is_acknowledged = FALSE", (sensor_id, level))
            existing_alarm = cursor.fetchone()
            
            # 팝업 도배 방지: 기존 것 확인 안했으면 추가 생성하지 않음
            if not existing_alarm:
                cursor.execute("INSERT INTO alarms (sensor_id, level, message) VALUES (%s, %s, %s)", (sensor_id, level, message))

    # ==========================
    # 2. 알람(Alarm) 관리 파트
    # ==========================
    def get_unacknowledged_alarms(self):
        """
        시스템에 발생한 알람 중, 관리자가 아직 '확인(Acknowledge)' 버튼을 누르지 않은
        현재 진행형인 알람 목록 전체를 조회합니다.

        :return: 미확인 알람 딕셔너리 리스트 (관련 센서의 화면 별침(alias) 포함)
        """
        with self.conn.cursor() as cursor:
            cursor.execute("""
                SELECT a.*, s.alias 
                FROM alarms a 
                LEFT JOIN sensor_metadata s ON a.sensor_id = s.sensor_id 
                WHERE a.is_acknowledged = FALSE 
                ORDER BY a.created_at DESC
            """)
            return cursor.fetchall()
            
    def acknowledge_alarm(self, alarm_id: int):
        """
        특정 알람을 관리자가 인지(확인)했음을 시스템에 기록합니다.
        이 함수가 호출되면 해당 알람은 더이상 프론트엔드 경고창에 뜨지 않습니다.

        :param alarm_id: 확인 처리할 알람의 고유 일련번호(DB ID)
        """
        with self.conn.cursor() as cursor:
            cursor.execute("UPDATE alarms SET is_acknowledged = TRUE, acknowledged_at = CURRENT_TIMESTAMP WHERE id = %s", (alarm_id,))
            self.conn.commit()

    # ==========================
    # 3. 센서 이력(History) 통계 엔진
    # ==========================
    def get_aggregate_history(self, sensor_id: str, period: str = 'daily'):
        """
        단일 센서에 대한 시간에 따른 요약 통계(평균값) 데이터를 빠르게 제공합니다.
        (예: 지난 24시간 동안의 시간별 평균, 지난 30일 동안의 일별 평균 등)

        :param sensor_id: 조회할 센서 ID
        :param period: 'daily'(지난 하루), 'monthly'(지난 한달), 'yearly'(지난 일년) 중 택 1
        :return: 시간(time)과 평균값(avg_value) 쌍으로 이루어진 리스트
        :raises ValueError: 지원하지 않는 period 값이 들어오면 발생
        """
        with self.conn.cursor() as cursor:
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
                raise ValueError("Invalid period. Choose from 'daily', 'monthly', 'yearly'")
            return cursor.fetchall()

    def get_custom_range_history(self, sensor_ids: list, start_dt, end_dt):
        """
        여러 센서의 과거 데이터를 사용자가 원하는 특정 기간 내에서 조회합니다.
        조회 기간의 길이에 따라 데이터 간격(버킷)을 5분/1시간/1일 단위로 자동 압축(Pivot)하여 
        UI 차트 라이브러리(Recharts 등)에 바로 넣을 수 있는 형태의 배열을 만들어 줍니다.

        :param sensor_ids: 합쳐서 통계를 낼 여러 센서 ID 리스트 (예: ['TEMP_1', 'HUMID_1'])
        :param start_dt: 조회 시작 일시 (datetime 객체)
        :param end_dt: 조회 종료 일시 (datetime 객체)
        :return: Pivot 된 차트용 데이터 리스트 (예: [{"time": "14:30", "TEMP_1": 25.1, "HUMID_1": 60.5}, ...])
        :raises ValueError: 파라미터 누락시 발생
        """
        if not sensor_ids:
            raise ValueError("No sensor IDs provided")
            
        diff_hours = (end_dt - start_dt).total_seconds() / 3600
        
        # 간격(Bucket) 자동 결정
        if diff_hours <= 3:
            date_format = '%m-%d %H:%i'
            group_by = "DATE(timestamp), HOUR(timestamp), MINUTE(timestamp)"
            bucket_expr = "timestamp"
        elif diff_hours <= 12:
            date_format = '%m-%d %H:%i'
            group_by = "DATE(timestamp), HOUR(timestamp), FLOOR(MINUTE(timestamp) / 5) * 5"
            bucket_expr = "TIMESTAMP(DATE(timestamp), MAKETIME(HOUR(timestamp), FLOOR(MINUTE(timestamp)/5)*5, 0))"
        elif diff_hours <= 24:
            date_format = '%m-%d %H:%i'
            group_by = "DATE(timestamp), HOUR(timestamp), FLOOR(MINUTE(timestamp) / 10) * 10"
            bucket_expr = "TIMESTAMP(DATE(timestamp), MAKETIME(HOUR(timestamp), FLOOR(MINUTE(timestamp)/10)*10, 0))"
        elif diff_hours <= 168:
            date_format = '%m-%d %H:00'
            group_by = "DATE(timestamp), HOUR(timestamp)"
            bucket_expr = "timestamp"
        elif diff_hours <= 730:
            date_format = '%Y-%m-%d'
            group_by = "DATE(timestamp)"
            bucket_expr = "timestamp"
        else:
            date_format = 'Week %v, %Y'
            group_by = "YEARWEEK(timestamp, 1)"
            bucket_expr = "timestamp"

        placeholders = ','.join(['%s'] * len(sensor_ids))
        query = f"""
            SELECT 
                DATE_FORMAT(MIN({bucket_expr}), %s) as time, 
                sensor_id, 
                AVG(value) as avg_value
            FROM sensors
            WHERE sensor_id IN ({placeholders}) 
              AND timestamp >= %s 
              AND timestamp <= %s
            GROUP BY {group_by}, sensor_id
            ORDER BY MIN(timestamp) ASC
        """
        params = (date_format,) + tuple(sensor_ids) + (start_dt.strftime('%Y-%m-%d %H:%M:%S'), end_dt.strftime('%Y-%m-%d %H:%M:%S'))

        with self.conn.cursor() as cursor:
            cursor.execute(query, params)
            rows = cursor.fetchall()

        # Pivot data: 차원에 맞게 여러 센서값을 하나의 시간에 맞춤
        # [{"time": "14:30", "TEMP_1": 25.1, "HUMID_1": 60.5}, ...]
        pivot_dict = {}
        for row in rows:
            t = row['time']
            sid = row['sensor_id']
            val = float(row['avg_value'])
            
            if t not in pivot_dict:
                pivot_dict[t] = {"time": t}
            pivot_dict[t][sid] = round(val, 2)
            
        return list(pivot_dict.values())
