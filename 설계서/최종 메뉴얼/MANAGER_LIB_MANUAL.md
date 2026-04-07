# Smart Farm OS - Manager Libraries Manual
비즈니스 로직(Business/Service Layer)을 담당하는 Manager 클래스들의 사용법과 동작 시퀀스입니다. 이들은 대부분 Core Layer(DB, MQTT)와 상호작용합니다.

---

## 1. Sensor & Actuator Management (`HouseManager`, `SensorManager`, `ControlManager`)

### 1-1. `SensorManager` (경보 및 센서 로깅)
- **과정 1. 수신**: `processIncomingData(sensor_id, value)` 가 1차 진입점.
- **과정 2. 메타데이터 조회**: 등록되지 않은 센서라면 `discovered_devices`라는 특수 테이블에 추가하고 리턴합니다.
- **과정 3. 저장 및 룰 연계**: 등록된 센서라면 `sensor_data`에 로그성 저장 후, `checkAlarms()`를 호출해 임계값을 이탈했는지 검사합니다. 이후 `RuleEngine::evaluate()`로 패스합니다.
- **알람 등급 프로토콜**:
  - `CRITICAL_HIGH`, `CRITICAL_LOW`: Email 발송이 즉시 수행되는 최고 위험 등급.
  - `WARNING_HIGH`, `WARNING_LOW`: 대시보드상 뱃지만 띄우는 경고.

### 1-2. `ControlManager` (제어 명령 하달)
- **과정 1. 커맨드 해석**: `processControlCommand(actuator_id, command, user_id)`가 호출되면, 먼저 `actuator_metadata`를 쳐서 권한/존재 여부를 봅니다.
- **과정 2. 로그 및 송신**: `control_logs` 테이블에 무조건 이력(audit)을 남기고 `MqttClient::publish()`를 사용해 `smartfarm/actuators/<ID>/command` 토픽으로 ON/OFF 문자열을 전송합니다.

### 1-3. `HouseManager` (추상적 그룹화)
- 여러 센서와 액추에이터는 반드시 1개의 House(동, 온실 단위)에 속합니다.
- `createSensor()`, `createActuator()`는 각 디바이스가 렌더링될 `display_order`를 자동/수동 제어합니다.
- **커스터마이징 제안**: 하우스별 재배 시기(Planting Date)에 맞춰 자동으로 센서 임계값이 바뀌는 로직을 추가하려면 `HouseManager` 내부에 Cron Job 스레드를 만들어 각 센서의 `metadata` UPDATE 쿼리를 날리도록 할 수 있습니다.

---

## 2. Automation (`RuleEngine`)

### 2-1. 룰 조건식 (Automation Protocol)
- `evaluate(sensor_id, value)`
- `automation_rules` DB 테이블 내 **`condition_type`**: `GT`(기호 >), `LT`(<), `GTE`(>=), `LTE`(<=).
- **로직 시퀀스**:
  1. 센서값이 들어올때마다 룰 전체 조회 수행 (`WHERE is_enabled = 1`)
  2. `value`와 `threshold_value`를 `condition_type`에 기반하여 비교
  3. 일치할 경우 **Cooldown(재발동 제한)** 시간 검증 수행 (단위: minute)
  4. 쿨다운이 지난 상태라면 `ControlManager::processControlCommand()`에 룰 아이디를 담아 제어 전송.

### 2-2. 커스터마이징 제안
- 현재는 1:1 (특정 센서값 → 액추에이터) 구조입니다. 복합 조건 (센서 A가 높고 센서 B도 높을때만 작동)을 추가하시려면, `evaluate` 내부에서 `Database::fetchAllPS`로 B 센서의 최근 1분 평균값을 계산해오는 로직을 중간에 끼워넣으시면 됩니다.

---

## 3. Weather Integration (`WeatherManager`)

### 3-1. 복합 날씨 아키텍처
스마트팜은 농장 자체의 마이크로 기후(로컬 데이터)와, 기상청(KMA) 거시 기상 단기예보를 동시에 화면에 띄웁니다.

- `getLiveForecast()`: cpr 라이브러리(libcurl C++ 래퍼)를 이용해 `http://apis.data.go.kr/1360000/VilageFcstInfoService_2.0/getUltraSrtFcst` 공공 API를 호출합니다.
- 이 과정에서 위경도(Lat/Lon)를 기상청 전용 `X, Y 격자`로 치환하는 `convertGrid` 알고리즘이 내장되어 있습니다. (한국 한정)
- HTTP 오버헤드가 발생할 수 있으므로, 하루 호출 한도 초과 오류(Access Denied) 등에 대비하여 Python의 `weather_fetcher.py` 쪽에 스케줄러를 분담시키기도 했습니다. 즉 C++과 Python 혼합형 하이브리드 아키텍처입니다.
