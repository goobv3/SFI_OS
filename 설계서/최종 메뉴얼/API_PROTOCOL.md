# Smart Farm OS - API Protocol & Routing
스마트팜 프론트엔드와 외부 시스템이 백엔드에 접근하기 위한 프로토콜 및 인증 사양서입니다. `API::Router`가 처리하는 규격입니다.

## 1. Authentication (인증 프로토콜)
모든 비공개 API는 HTTP Request Header에 JWT 인증 토큰을 동봉해야 합니다.

- **Header Format**: `Authorization: Bearer <JWT_TOKEN>`
- **Fail Response**:
  - `401 Unauthorized`: 토큰 누락 또는 형식 오류
  - `403 Forbidden`: 토큰 파싱 불가, 서명 불일치, 만료됨

## 2. API Endpoints

공통 규약: 모든 응답 본문은 JSON(`application/json`) 형식입니다.
API 호출 오류 시, HTTP Status Code를 조정하며 JSON으로 `{"error": "에러 사유"}` 가 반환됩니다.

### 2-1. 하우스/사이트 관리 (House & Site)
- `GET /api/sites`: 전체 사이트 및 요약 알람 정보 배열 리턴.
- `GET /api/houses`: 등록된 전체 동네트워크 식별자 및 이름 반환.
- `POST /api/metadata/houses`: 새로운 하우스를 DB에 구축.
  - Request Body: `{"house_id": "H1", "name": "제1온실"}`

### 2-2. 디바이스 메타데이터 (Sensors & Actuators)
- `GET /api/houses/<house_id>/devices`: 해당 하우스의 센서와 액추에이터 통합 정보.
- `POST /api/sensors`: 센서 생성
  - Body: `{"sensor_id":"...", "house_id":"...", "alias":"...", "type":"..."}`
- `PUT /api/metadata/sensors/<sensor_id>`: 기존 센서 임계치 (warn_high, crit_high 등) 변경.

### 2-3. 제어 (Control)
- `POST /api/control`: 프론트엔드의 스위치/버튼 클릭시 발송.
  - Body: `{"actuator_id": "FAN_01", "command": "ON"}`
  - Response: 해당 ID가 유효할시에만 200 OK. MQTT 전송은 백그라운드 위임.

### 2-4. 자동화 룰 (Automation)
- `GET /api/automation/rules?house_id=HW1`: 해당 하우스의 룰 리스트 필터 조회.
- `POST /api/automation/rules`: 신규 룰 생성
  - Body 필드: `name, trigger_sensor_id, condition_type("GT", "LT" 등), threshold_value, actuator_id, action_command, cooldown_minutes`
- `PUT /api/automation/rules/<id>/toggle`: 룰 On/Off 토글 전환
  - Body: `{"enabled": true}`

### 2-5. 기타 (Other Protocols)
- `GET /api/alarms/active`: 아직 Acknowledge(확인) 처리되지 않은 센서 경보 리스트 전체 조회.
- `GET /api/weather/live?hours=N`: 현재 시간 대비 N시간 뒤의 날씨 예보를 기상청 API에서 직접 다운로드하여 필터링/응답.

## 3. 커스터마이징 가이드
**API 신규 추가**
1. `Router.cpp`를 여십시오.
2. `CROW_ROUTE(app, "/api/신규엔드포인트").methods(crow::HTTPMethod::GET)([&](const crow::request& req) { ... });` 패턴을 작성합니다.
3. 인증이 필요하다면 `std::string auth_header = req.get_header_value("Authorization");` 후 `requireAuth` 람다를 호출해 막아내십시오.
4. JSON 반환은 `makeJson()` 또는 `crow::response(200, res_json.dump())`를 사용하면 객체 생명주기가 안전하게 제어됩니다.
