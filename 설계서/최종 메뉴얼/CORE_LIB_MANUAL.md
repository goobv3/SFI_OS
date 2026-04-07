# Smart Farm OS - Core Libraries Manual
이 문서는 Smart Farm Intelligence OS의 핵심 백엔드 라이브러리(Core Layer)에 대한 종합 메뉴얼입니다. 향후 새로운 데이터베이스, 인증 체계, 프로토콜 통신으로 커스터마이징할 때 참고하시기 바랍니다.

## 1. Database Library (`Core::Database`)

### 1-1. 역할 및 아키텍처
- **MariaDB 전용 연결**을 수행하는 Singleton 패턴 기반 레퍼 라이브러리.
- `libmariadb` C API를 직접 호출하여 오버헤드를 최소화하고 성능을 극대화합니다.
- 멀티스레드 환경(웹 프레임워크인 Crow 스레드풀)에서 안전하게 병렬 처리되도록 내부적으로 `std::mutex`를 사용해 연결 및 쿼리 액세스를 직렬화합니다.

### 1-2. 함수 설명

#### `Database::getInstance()`
- **역할**: 전역에 유일한 Database 객체를 반환합니다. Meyers' Singleton 방식이므로 호출 시 쓰레드 세이프하게 초기화됩니다.

#### `Database::connect() / disconnect()`
- **역할**: TCP 소켓을 통해 `sf_mariadb` 호스트(기본값)에 연결합니다. 연결 시 언어셋을 `utf8mb4`로 고정합니다.

#### `Database::execute(query)` / `Database::fetchAll(query)`
- **역할**: 외부 입력이 **전혀 없는** 단순 DDL, DML을 실행할 때 사용합니다.
- **반환**: fetchAll은 `std::vector<std::map<std::string, std::string>>` (각 행이 Key-Value Map으로 떨어짐)을 반환합니다.

#### `Database::executePS(sql, params)` / `Database::fetchAllPS(sql, params)`
- **역할**: **Prepared Statement(PS)** 를 사용한 SQL Injection 방어용 쿼리 전송.
- **파라미터**: `sql` 인자에 데이터를 꽂을 자리에 `?`를 넣고, `params` `std::vector<std::string>`에 순서대로 바인딩할 데이터를 담아 전달합니다.
- **커스터마이징 제안**: 현재 모든 PS 바인딩 타입은 `MYSQL_TYPE_STRING`으로 통일되어 있습니다. 쿼리 성능 최적화를 위해 입력 타입에 따라 `MYSQL_TYPE_LONG` 등으로 분기하도록 수정할 수 있습니다.

---

## 2. MQTT Library (`Core::MqttClient`)

### 2-1. 역할 및 아키텍처
- 센서 데이터 수신 및 액추에이터 제어 명령을 비동기적으로 중계하는 Paho MQTT C++ 라이브러리의 래퍼입니다.
- QoS 수준을 **1 (At least once)** 로 고정하여, 메시지 유실율을 낮추었습니다.

### 2-2. 동작 프로토콜
1. **서버 시작 시**: `getInstance().connect()`를 통해 tcp://sf_mosquitto:1883으로 비동기 연결.
2. **구독 (Subscribe)**: `subscribe("smartfarm/sensors/+/value", callback_lambda)`를 호출하면 해당 Topic에 매칭시 내부 콜백 수행.
3. **토픽 매칭**: 와일드카드 `+`를 커스텀 로직(message_arrived 내 string::find)으로 파싱해 처리합니다. 향후 `#`(Multi-level 와일드카드)를 지원하려면 이 부분을 커스터마이징하세요.

### 2-3. 주요 함수
- `MqttClient::publish(topic, payload)`: 최대 10초를 기다리며 MQTT에 명령을 전파.
- `MqttClient::subscribe(topic, callback)`: std::function 기반 람다를 맵 구조체에 밀어넣어 메시지가 도착하면 스레드 세이프하게 핸들러 수행.

---

## 3. Security Library (`Core::JwtUtils`)

### 3-1. 역할 및 아키텍처
외부의 무거운 OpenSSL 래퍼 라이브러리 대신, OpenSSL 기본 `EVP` 및 `HMAC` 객체를 조합해 **HS256** 토큰을 만들고, PBKDF2 해싱을 자체 구현했습니다.

### 3-2. 함수 설명
- `generateToken(username, role, expiry_hours)`: 유저 이름과 역할 문자열을 받아 JSON을 구성하고, `JWT_SECRET` 환경변수를 키값으로 HMAC-SHA256 서명.
- `verifyToken(token)`: 점(`.`)을 기준으로 헤더, 페이로드, 시그니처 3조각을 잘라, 서버단에서 시그니처를 재계산해 일치 여부 확인 (`TokenPayload` 구조체 리턴).
- `hashPassword(password, salt, iterations)`: 레인보우 테이블 공격을 막기 위해 1만번의 `PKCS5_PBKDF2_HMAC` 루프를 돌려 비밀번호 문자열을 해싱.

---

## 4. Alert Library (`Core::EmailSender`)

### 4-1. 역할 및 아키텍처
libcurl을 활용해 SMTP 서버로 직접 메일을 전송합니다. 외부 클라우드 의존성 없이 로컬 C++에서 바로 처리됩니다.

### 4-2. 동작 프로토콜
- 이메일 구조는 RFC 5322 규약을 따릅니다.
- KMA_SERVICE_KEY 처럼 `SMTP_HOST`, `SMTP_PASS`, `ALERT_EMAIL` 환경변수가 세팅되어야 전송이 실행됩니다.
- 본문인 payload는 `read_callback` 이라는 C-스타일 함수 콜백에 의해 버퍼 사이즈(청크 단위)만큼 순차적으로 네트워크에 로드됩니다. C++ 스트림 객체에서 string으로 치환되어 동작합니다.
