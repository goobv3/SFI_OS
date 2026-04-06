# 🚀 Smart Farm Intelligence OS — 상용화 80% 로드맵
### 현재 수준 ~47% → 목표 ~82%

---

## 전체 아키텍처 변화 설계도

```
┌─────────────────────────────────────── 완성 후 전체 구조 ───────────────────────────────────────┐
│                                                                                                   │
│  [ 브라우저 / 모바일 ]                                                                            │
│        │  HTTPS (443)                                                                             │
│        ▼                                                                                          │
│  ┌─────────────┐        ┌─────────────────────────────────────────────────────┐                  │
│  │   Nginx     │◄──────►│  Phase 3: HTTPS + Reverse Proxy + Static Serving   │                  │
│  │ (80 → 443)  │        └─────────────────────────────────────────────────────┘                  │
│  └──────┬──────┘                                                                                  │
│         │ /api/*                                                                                   │
│         ▼                                                                                          │
│  ┌─────────────────────────────────────────────────────────────────────────────────────┐          │
│  │                    C++ Crow Backend (현재 구조 유지)                                  │          │
│  │                                                                                       │          │
│  │  ┌──────────────┐   ┌──────────────┐   ┌──────────────┐   ┌──────────────────────┐ │          │
│  │  │  Router.cpp  │   │  AuthMiddle  │   │  RuleEngine  │   │  NotificationSvc     │ │          │
│  │  │ (기존 라우트) │   │  (Phase 2:  │   │  (Phase 4:  │   │  (Phase 6: Kakao/    │ │          │
│  │  │              │   │   JWT 검증)  │   │  자동제어)   │   │   FCM 알림)          │ │          │
│  │  └──────────────┘   └──────────────┘   └──────────────┘   └──────────────────────┘ │          │
│  │                                                                                       │          │
│  │  ┌──────────────┐   ┌──────────────┐   ┌──────────────┐                            │          │
│  │  │HouseManager  │   │SensorManager │   │ControlManager│ ← Phase 1: SQL 보안 패치   │          │
│  │  │(Prepared Stmt│   │(Prepared Stmt│   │(Prepared Stmt│   모든 Manager에 적용       │          │
│  │  └──────────────┘   └──────────────┘   └──────────────┘                            │          │
│  └──────────────────────────────────────────────────────────────────────────────┬──────┘          │
│                                                                                  │                  │
│         ┌────────────────────────────────────────────────────────────────────────┘                  │
│         ▼                                                                                           │
│  ┌─────────────┐    ┌──────────────────────────────┐                                              │
│  │  MariaDB    │    │  Phase 7: 데이터 보존 정책    │                                              │
│  │             │◄───│  - 이벤트 스케줄러 (자동      │                                              │
│  │  + users 테 │    │    다운샘플링 & 90일 TTL)     │                                              │
│  │  이블 추가  │    └──────────────────────────────┘                                              │
│  └─────────────┘                                                                                   │
│                                                                                                     │
│  ┌─────────────────────────────────────────────────────────────────────────────────────┐           │
│  │                    React Frontend (Phase 5: 모바일 반응형)                           │           │
│  │  - Tailwind 브레이크포인트 전면 적용                                                  │           │
│  │  - 모바일용 하단 네비게이션 바 추가                                                    │           │
│  │  - 게이지 차트 크기 동적 조정                                                          │           │
│  └─────────────────────────────────────────────────────────────────────────────────────┘           │
└─────────────────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## Phase별 완성도 기여 예상치

| Phase | 내용 | 완성도 기여 | 완료 후 누적 |
|-------|------|------------|------------|
| 현재 | - | - | **~47%** |
| Phase 1 | SQL Injection 보안 패치 | +4% | **~51%** |
| Phase 2 | JWT 사용자 인증 | +8% | **~59%** |
| Phase 3 | Nginx + HTTPS | +5% | **~64%** |
| Phase 4 | 자동제어 룰 엔진 | +10% | **~74%** |
| Phase 5 | 모바일 반응형 UI | +5% | **~79%** |
| Phase 6 | 외부 알림 연동 | +4% | **~83%** ✅ |
| Phase 7 | 데이터 보존 정책 | +3% | **~86%** 🏆 |

> **Phase 1~5 완료만으로도 목표 80% 달성 가능.**  
> Phase 6~7은 추가 품질 향상용.

---
---

# PHASE 1 — SQL Injection 보안 패치
**난이도:** ⭐⭐ (중급) | **예상 소요:** 2~3시간

## 문제 설명
현재 `HouseManager.cpp`, `ControlManager.cpp` 등 모든 Manager 파일에서  
SQL 쿼리를 문자열로 직접 조합하고 있음. 사용자 입력값이 그대로 DB에 전달되어  
**SQL Injection 공격에 완전히 노출**된 상태.

```cpp
// 현재 취약한 코드 예시 (HouseManager.cpp:22)
std::string sensorQuery = "SELECT COUNT(*) as c FROM sensor_metadata WHERE house_id='" 
                          + obj["house_id"].get<std::string>() + "'";  // ← 위험!
```

## 수정 대상 파일
- `backend/src/core/Database.h` / `Database.cpp` — `executePrepared()` 메서드 추가
- `backend/src/managers/HouseManager.cpp` — 모든 쿼리 교체
- `backend/src/managers/SensorManager.cpp` — 모든 쿼리 교체
- `backend/src/managers/ControlManager.cpp` — 모든 쿼리 교체
- `backend/src/managers/WeatherManager.cpp` — 모든 쿼리 교체

---

### 📋 PHASE 1 프롬프트 (복사하여 그대로 사용)

```
현재 Smart Farm Intelligence OS 백엔드(C++ Crow 프레임워크)의 모든 Manager 파일이
SQL 쿼리를 문자열 연결(string concatenation)로 조합하고 있어 SQL Injection에 취약합니다.

## 현재 프로젝트 구조
- backend/src/core/Database.h — MariaDB 연결 싱글톤, fetchAll(string sql) / execute(string sql) 메서드 보유
- backend/src/core/Database.cpp — mysql_real_query 기반 구현
- backend/src/managers/HouseManager.cpp — 가장 많은 SQL 쿼리 포함
- backend/src/managers/ControlManager.cpp — INSERT/SELECT 쿼리 포함
- backend/src/managers/SensorManager.cpp — SELECT/INSERT 쿼리 포함
- backend/src/managers/WeatherManager.cpp — INSERT/SELECT 쿼리 포함

## 요청 사항
1. Database.h / Database.cpp에 MySQL C API의 Prepared Statement를 사용하는
   `executePS(string sql, vector<string> params)` 와
   `fetchAllPS(string sql, vector<string> params)` 메서드를 추가해 주세요.
   
2. HouseManager.cpp의 모든 쿼리를 위 Prepared Statement 메서드로 교체해 주세요.
   특히 house_id, sensor_id, actuator_id 등 외부 입력값이 들어가는 모든 WHERE 절과
   INSERT VALUES를 ? 플레이스홀더로 교체해야 합니다.

3. ControlManager.cpp의 모든 쿼리도 동일하게 교체해 주세요.

4. SensorManager.cpp의 모든 쿼리도 동일하게 교체해 주세요.

5. 기존 fetchAll(string) / execute(string) 메서드는 내부 전용(상수 쿼리)으로 유지하되,
   외부 입력을 받는 모든 호출부를 PS 버전으로 교체해 주세요.

기존 코드 기능(CRUD, 순서 저장, 알람 등)은 그대로 유지되어야 합니다.
변경된 파일 전체를 모두 출력해 주세요.
```

---
---

# PHASE 2 — JWT 사용자 인증 시스템
**난이도:** ⭐⭐⭐ (고급) | **예상 소요:** 4~6시간

## 문제 설명
현재 API 엔드포인트에 인증이 전혀 없어 누구나 `/api/control`로 제어 명령을 보낼 수 있음.  
상용화를 위해 최소한 **로그인 → JWT 토큰 발급 → 이후 요청에 Bearer 토큰 첨부** 흐름이 필요.

## 추가될 구조
```
DB: users 테이블 신규 추가
    (id, username, password_hash, role, created_at)

API:
    POST /api/auth/login    → username/password → JWT 토큰 반환
    POST /api/auth/refresh  → refresh token → 새 access token 반환
    GET  /api/auth/me       → 현재 로그인 정보 (토큰 필요)

미들웨어:
    모든 /api/* 요청에서 Authorization: Bearer <token> 헤더 검증
    /api/auth/login 만 예외 처리
```

## 수정/추가 대상 파일
- `database/init.sql` — `users` 테이블 추가
- `backend/src/core/` — `JwtUtils.h` / `JwtUtils.cpp` 신규 추가
- `backend/src/api/Router.cpp` — auth 라우트 추가 + 미들웨어 적용
- `backend/CMakeLists.txt` — jwt-cpp 또는 직접 구현 라이브러리 추가
- `frontend/src/api/client.ts` — Authorization 헤더 자동 첨부
- `frontend/src/` — `LoginPage.tsx` 및 `AuthContext.tsx` 신규 추가

---

### 📋 PHASE 2 프롬프트 (복사하여 그대로 사용)

```
Smart Farm Intelligence OS에 JWT 기반 사용자 인증 시스템을 추가해 주세요.

## 현재 프로젝트 정보
- 백엔드: C++ Crow 프레임워크, crow::App<crow::CORSHandler> 사용
- 프론트엔드: React 18 + TypeScript + Vite + Tailwind CSS (Neon 다크 테마)
- DB: MariaDB 11 (Docker), database/init.sql에 초기 스키마 정의
- API 통신: frontend/src/api/client.ts (Axios 기반 smartFarmApi 객체)

## 구현 요청 사항

### 백엔드 (C++)
1. database/init.sql에 users 테이블을 추가해 주세요:
   - 컬럼: id(AUTO_INCREMENT), username(VARCHAR50, UNIQUE), 
     password_hash(VARCHAR255), role(ENUM 'admin','viewer', DEFAULT 'viewer'),
     created_at(DATETIME)
   - 초기 admin 계정 INSERT: username='admin', password는 'admin1234'를 
     bcrypt 해시값으로 저장 (해시값을 직접 SQL에 넣어주세요)

2. backend/src/core/ 에 JwtUtils.h / JwtUtils.cpp를 새로 만들어 주세요:
   - HS256 알고리즘으로 JWT 생성/검증
   - 외부 라이브러리 없이 구현하거나, header-only jwt-cpp 라이브러리 사용
   - ACCESS_TOKEN 유효시간: 2시간 / REFRESH_TOKEN: 7일
   - SECRET_KEY는 환경변수 JWT_SECRET에서 읽음 (기본값: "smartfarm_secret_key")

3. backend/src/api/Router.cpp에 다음 라우트를 추가해 주세요:
   - POST /api/auth/login : body {username, password} → {access_token, refresh_token, role}
   - GET /api/auth/me : Bearer 토큰 필요 → {username, role}
   - 그리고 /api/auth/* 를 제외한 모든 /api/* 라우트에 토큰 검증 미들웨어를 적용해 주세요.
     유효하지 않은 토큰이면 401 JSON 응답을 반환합니다.

### 프론트엔드 (React + TypeScript)
4. frontend/src/contexts/AuthContext.tsx를 새로 만들어 주세요:
   - login(username, password) 함수: API 호출 후 localStorage에 토큰 저장
   - logout() 함수: 토큰 제거 후 로그인 페이지로 이동
   - isAuthenticated, user(username, role) 상태 관리

5. frontend/src/pages/LoginPage.tsx를 새로 만들어 주세요:
   - 기존 Neon 다크 테마(네온블루 글로우)에 맞는 로그인 폼
   - username / password 입력, 로그인 버튼
   - 로그인 실패 시 에러 메시지 표시

6. frontend/src/api/client.ts를 수정해 주세요:
   - 모든 요청에 localStorage의 access_token을 Authorization: Bearer 헤더로 자동 첨부
   - 401 응답 수신 시 자동으로 로그인 페이지로 이동

7. frontend/src/App.tsx를 수정해 주세요:
   - AuthContext Provider로 전체 앱을 감싸기
   - isAuthenticated가 false이면 LoginPage 렌더링

기존 코드 기능(대시보드, 기기 관리, 날씨 위젯 등)은 그대로 유지해야 합니다.
변경/추가된 파일 전체를 모두 출력해 주세요.
```

---
---

# PHASE 3 — Nginx HTTPS 리버스 프록시
**난이도:** ⭐⭐ (중급) | **예상 소요:** 1~2시간

## 문제 설명
현재 HTTP로만 서비스되어 통신 내용(JWT 토큰, 센서값 등)이 평문으로 노출됨.  
Nginx를 추가하여 HTTPS 연결을 처리하고, React 빌드 파일을 직접 서빙하게 함.

## 변경 대상 파일
- `docker-compose.yml` — nginx 서비스 추가
- `nginx/` 폴더 신규 — `nginx.conf`, `Dockerfile`
- `frontend/Dockerfile` — 빌드 전용으로 변경 (nginx가 서빙)

---

### 📋 PHASE 3 프롬프트 (복사하여 그대로 사용)

```
Smart Farm Intelligence OS의 docker-compose 구성에 Nginx 리버스 프록시를 추가해 주세요.

## 현재 docker-compose.yml 구조
- frontend 서비스: React Vite 개발서버 (포트 5173)
- backend 서비스: C++ Crow REST API (포트 8000)
- db 서비스: MariaDB (포트 3306)
- mosquitto 서비스: MQTT 브로커 (포트 1883)

## 현재 frontend/Dockerfile 내용
(Vite 개발서버 기반, npm run dev 실행)

## 요청 사항

1. nginx/ 폴더를 새로 만들고 다음 파일들을 생성해 주세요:
   - nginx/nginx.conf:
     * 80 포트 → 443으로 리디렉션
     * 443 포트에서 SSL 처리 (자가 서명 인증서 경로: /etc/nginx/certs/)
     * /api/* 요청 → backend:8000으로 프록시 패스
     * 그 외 모든 요청 → React 빌드 정적 파일 서빙 (/usr/share/nginx/html)
     * gzip 압축 활성화
     * proxy_read_timeout, proxy_connect_timeout 60초 설정
   - nginx/Dockerfile:
     * nginx:alpine 베이스
     * 자가 서명 SSL 인증서를 Docker 빌드 시 자동 생성 (openssl 사용)

2. frontend/Dockerfile을 수정해 주세요:
   - Multi-stage build 적용: builder stage(npm run build) → nginx stage(정적 파일 복사)
   - 최종 이미지는 nginx 기반, 포트 80 노출

3. docker-compose.yml을 수정해 주세요:
   - nginx 서비스 추가 (포트 80:80, 443:443 노출)
   - frontend, backend 서비스는 내부 네트워크만 사용 (외부 포트 제거)
   - nginx가 frontend, backend에 의존(depends_on) 설정
   - 볼륨으로 nginx.conf 마운트

개발 환경에서도 자가 서명 인증서로 HTTPS가 작동해야 합니다.
브라우저 경고는 개발용이므로 무시합니다.
변경된 파일 전체를 모두 출력해 주세요.
```

---
---

# PHASE 4 — 자동제어 룰 엔진 (Automation Rule Engine)
**난이도:** ⭐⭐⭐⭐ (최고급) | **예상 소요:** 6~10시간

## 문제 설명
현재 ControlManager가 단순 MQTT 중계만 함 (28줄).  
상용 스마트팜의 핵심은 **"온도가 30°C 초과 시 창문을 자동으로 열어라"** 같은  
**조건부 자동제어 룰**임. 이것이 없으면 "모니터링 SW"에 불과.

## 추가될 구조
```
DB: automation_rules 테이블
    (id, name, house_id, trigger_sensor_id, condition(GT/LT/BETWEEN),
     threshold_value, actuator_id, action_command, is_enabled, cooldown_minutes)

로직 흐름:
    센서 데이터 수신(MQTT)
    → SensorManager::processIncomingData()
    → RuleEngine::evaluate(sensor_id, new_value)  ← 신규
    → 매칭 룰 발견 시 ControlManager::processControlCommand() 자동 호출
    → 쿨다운(cooldown) 체크 (중복 실행 방지)
    → MQTT publish + control_logs 기록

프론트엔드:
    ManageMode.tsx 또는 별도 AutomationPage.tsx에
    룰 생성/편집/삭제/토글 UI 추가
```

---

### 📋 PHASE 4 프롬프트 (복사하여 그대로 사용)

```
Smart Farm Intelligence OS에 자동제어 룰 엔진(Automation Rule Engine)을 추가해 주세요.
이것이 단순 모니터링 SW를 스마트팜 제어 SW로 격상시키는 핵심 기능입니다.

## 현재 관련 코드 구조
- SensorManager::processIncomingData(sensor_id, value): MQTT로 수신한 센서값을 DB에 저장하고 임계치 알람 발생
- ControlManager::processControlCommand(actuator_id, command, user_id): MQTT로 제어 명령 발행 + DB 로그
- database/init.sql: 현재 테이블 구조 (houses, sensor_metadata, sensors, actuator_metadata, actuator_status, control_logs, alarms)
- backend/src/managers/ 폴더에 각 Manager 파일 존재
- 프론트엔드: ManageMode.tsx에 하우스/기기 관리 UI 존재 (Tailwind CSS + 네온 다크 테마)

## 요청 사항

### 백엔드 (C++)
1. database/init.sql에 automation_rules 테이블을 추가해 주세요:
   - id(BIGINT AUTO_INCREMENT PK), name(VARCHAR100), house_id(VARCHAR50, FK→houses),
     trigger_sensor_id(VARCHAR50, FK→sensor_metadata),
     condition_type(ENUM: 'GT','LT','GTE','LTE','BETWEEN'),
     threshold_low(FLOAT NULL), threshold_high(FLOAT NULL),
     actuator_id(VARCHAR50, FK→actuator_metadata),
     action_command(VARCHAR50, e.g. 'ON','OFF','OPEN','CLOSE'),
     is_enabled(BOOLEAN DEFAULT TRUE),
     cooldown_minutes(INT DEFAULT 5),
     last_triggered_at(DATETIME NULL),
     created_at(DATETIME DEFAULT CURRENT_TIMESTAMP)

2. backend/src/managers/RuleEngine.h / RuleEngine.cpp를 새로 만들어 주세요:
   - 싱글톤 패턴 (다른 Manager와 동일)
   - evaluate(sensor_id, double value) 함수:
     * DB에서 해당 sensor_id를 trigger로 사용하는 is_enabled=true인 룰 조회
     * 각 룰의 condition_type에 따라 value 조건 평가
     * 조건 충족 시 last_triggered_at 기준 cooldown_minutes 경과 여부 확인
     * 쿨다운 통과 시 ControlManager::processControlCommand() 호출 (source='AutoRule')
     * last_triggered_at을 현재 시간으로 업데이트
   - CRUD 함수: getRules(house_id), createRule(json), updateRule(id, json), deleteRule(id), toggleRule(id, bool)

3. SensorManager::processIncomingData() 함수 끝에 RuleEngine::evaluate() 호출을 추가해 주세요.

4. backend/src/api/Router.cpp에 자동화 룰 CRUD API를 추가해 주세요:
   - GET    /api/automation/rules?house_id=XXX
   - POST   /api/automation/rules
   - PUT    /api/automation/rules/<int:id>
   - DELETE /api/automation/rules/<int:id>
   - PATCH  /api/automation/rules/<int:id>/toggle

### 프론트엔드 (React + TypeScript)
5. frontend/src/components/AutomationPanel.tsx를 새로 만들어 주세요:
   - 기존 Neon 다크 테마(네온블루/그린 글로우)에 맞는 UI
   - 상단: "[+ 새 룰 추가]" 버튼
   - 룰 목록 카드: 각 카드에 룰 이름, "IF [센서명] [조건] [값] THEN [액추에이터] [명령]" 
     형태의 로직을 시각적으로 보여주기
   - 각 카드에 활성화/비활성화 토글 스위치, 편집, 삭제 버튼
   - 룰 추가 모달: 하우스 선택 → 센서 선택 → 조건/값 입력 → 액추에이터 선택 → 명령 선택 → 쿨다운 입력

6. frontend/src/App.tsx 또는 ManageMode.tsx에 AutomationPanel로 이동하는 상단 탭/버튼을 추가해 주세요.

7. frontend/src/api/client.ts에 자동화 룰 관련 API 함수를 추가해 주세요.

기존 코드 기능은 그대로 유지해야 합니다.
변경/추가된 파일 전체를 모두 출력해 주세요.
```

---
---

# PHASE 5 — 모바일 반응형 UI
**난이도:** ⭐⭐⭐ (고급) | **예상 소요:** 4~6시간

## 문제 설명
현재 UI가 PC 전용 레이아웃으로 고정되어 있어, 농가 현장에서 스마트폰으로 접속 시  
대시보드가 세로로 길게 잘리거나 겹쳐서 사용 불가능.

## 변경 대상
- 모든 컴포넌트에 Tailwind 반응형 브레이크포인트 적용
- `App.tsx` — 모바일 하단 내비게이션 바 추가
- `SensorGauge.tsx` — 화면 크기에 따라 게이지 크기 동적 조정
- `HistoryChart.tsx` — 모바일에서 차트 높이 축소
- `WeatherWidget.tsx` — 모바일에서 단순화된 레이아웃
- `ManageMode.tsx` — 모바일에서 2단 → 1단 레이아웃 전환

---

### 📋 PHASE 5 프롬프트 (복사하여 그대로 사용)

```
Smart Farm Intelligence OS 프론트엔드 전체를 모바일 반응형으로 개선해 주세요.
현재 PC 전용 레이아웃을 스마트폰(375px~)부터 태블릿(768px), PC(1280px+)까지 모두 지원해야 합니다.

## 현재 프론트엔드 구조
- frontend/src/App.tsx — 메인 레이아웃 (SensorDashboard, WeatherWidget, HistoryChart, ActuatorControl 배치)
- frontend/src/components/SensorGauge.tsx — Recharts ResponsiveContainer 기반 도넛 게이지
- frontend/src/components/SensorDashboard.tsx — 게이지 그리드 레이아웃
- frontend/src/components/HistoryChart.tsx — Recharts 라인 차트
- frontend/src/components/WeatherWidget.tsx — 기상청 날씨 + 예보 위젯
- frontend/src/components/ActuatorControl.tsx — 액추에이터 스위치 패널
- frontend/src/components/ManageMode.tsx — 좌우 2단 설정 모달 (w-1/3 + flex-1)
- 스타일: Tailwind CSS + 네온 다크 테마 (neon-blue, neon-green, cyber-border 커스텀 색상)

## 요청 사항

1. App.tsx 수정:
   - 모바일(768px 미만)에서 상단 헤더를 간소화하고, 하단에 고정 내비게이션 탭 바 추가
     (탭: 대시보드, 제어, 날씨, 이력)
   - PC에서는 기존 레이아웃 유지
   - 현재 세로로 긴 레이아웃을 모바일에서는 탭 방식으로 섹션 전환

2. SensorDashboard.tsx 수정:
   - 모바일: grid-cols-2 (2열), 태블릿: grid-cols-3, PC: grid-cols-4 이상

3. SensorGauge.tsx 수정:
   - 모바일에서 게이지 원 크기 축소, 센서명/값 텍스트 크기 조정
   - useWindowSize 훅 또는 CSS만으로 반응형 처리

4. WeatherWidget.tsx 수정:
   - 모바일: 현재 날씨만 표시, 예보는 가로 스크롤 가능한 슬라이드 형태
   - PC: 기존 레이아웃 유지

5. ManageMode.tsx 수정:
   - 모바일: 좌우 2단(1/3 + 2/3) → 상하 탭 전환(하우스 목록 탭 / 기기 편집 탭)
   - 모달 크기: 모바일에서 전체 화면(h-screen) 점유

6. HistoryChart.tsx 수정:
   - 모바일: 차트 높이 250px, 범례 숨김 또는 하단 이동
   - PC: 기존 레이아웃 유지

7. ActuatorControl.tsx 수정:
   - 모바일: 그리드 2열, 각 버튼 크기 터치 친화적으로(최소 44px 높이)

추가 가이드라인:
- Tailwind 기본 브레이크포인트 사용 (sm:640, md:768, lg:1024)
- 기존 색상 테마(neon-blue, neon-green, 다크 배경)는 절대 변경하지 마세요
- 기존 기능과 이벤트 핸들러는 그대로 유지해야 합니다

변경된 파일 전체를 모두 출력해 주세요.
```

---
---

# PHASE 6 — 외부 알림 연동 (카카오 알림톡 / 웹 Push)
**난이도:** ⭐⭐⭐ (고급) | **예상 소요:** 3~5시간

## 문제 설명
현재 알람이 대시보드 내 Toast 팝업으로만 표시됨.  
사용자가 앱을 열고 있지 않으면 알람을 못 봄.  
농가 현장에서는 **스마트폰 문자/카카오 알림**이 핵심.

## 선택지
- **옵션 A (권장):** 카카오 알림톡 (국내 스마트팜 현장에서 가장 효과적)
- **옵션 B:** 웹 Push Notification (PWA 방식, 별도 앱 불필요)
- **옵션 C:** 이메일 알림 (SMTP, 가장 구현 간단)

---

### 📋 PHASE 6 프롬프트 — 옵션 C: 이메일 알림 (가장 빠른 구현)

```
Smart Farm Intelligence OS에 이메일 알림 기능을 추가해 주세요.
임계치 알람 발생 시 설정된 이메일 주소로 경보 메일을 자동 발송합니다.

## 현재 알람 흐름
- SensorManager::processIncomingData() 에서 임계치 초과 시 DB alarms 테이블에 INSERT
- 프론트엔드가 주기적으로 GET /api/alarms 폴링하여 Toast 표시

## 요청 사항

### 백엔드 (C++)
1. backend/src/core/ 에 EmailSender.h / EmailSender.cpp를 새로 만들어 주세요:
   - libcurl을 사용한 SMTP 이메일 발송 (backend/CMakeLists.txt에도 추가)
   - 환경변수에서 설정 읽기:
     * SMTP_HOST (예: smtp.gmail.com)
     * SMTP_PORT (예: 587)
     * SMTP_USER (발신 이메일)
     * SMTP_PASS (앱 비밀번호)
     * ALERT_EMAIL (수신 이메일)
   - sendAlert(sensor_alias, level, value, threshold_info) 함수 구현
   - 이메일 제목: "[스마트팜 경보] {sensor_alias}에서 {level} 수준 이상 감지"
   - 이메일 본문: 한국어로 현재값, 임계치, 발생시각 포함

2. SensorManager::processIncomingData()에서 알람 INSERT 직후 EmailSender::sendAlert() 호출 추가
   - 단, warn 레벨은 이메일 없이 DB만, crit 레벨만 이메일 발송

3. docker-compose.yml의 backend 서비스 environment에 위 환경변수 항목 추가 (값은 빈 문자열로)

### 프론트엔드 (React + TypeScript)
4. frontend/src/components/ManageMode.tsx 또는 별도 SettingsPanel.tsx에
   이메일 알림 설정 UI 섹션을 추가해 주세요:
   - 수신 이메일 주소 입력 필드
   - "테스트 이메일 발송" 버튼
   - warn/crit 레벨별 이메일 발송 여부 토글

5. 이 설정값을 저장/조회할 API 엔드포인트를 추가해 주세요:
   - GET /api/settings/notifications
   - PUT /api/settings/notifications

기존 알람 Toast 기능은 그대로 유지해야 합니다.
변경/추가된 파일 전체를 모두 출력해 주세요.
```

---
---

# PHASE 7 — 데이터 보존 정책 (Data Retention)
**난이도:** ⭐⭐ (중급) | **예상 소요:** 2~3시간

## 문제 설명
현재 `sensors` 테이블에 모든 RAW 데이터가 무한정 쌓임.  
1분에 10건 수신 시 → 하루 14,400건 → 1년 525만 건 → 쿼리 급격히 느려짐.

## 전략
```
sensors 테이블: 최근 30일 RAW 데이터만 유지 (이후 삭제)
sensors_hourly: 1시간 집계 데이터 → 1년 유지 (신규 테이블)
sensors_daily:  1일 집계 데이터 → 5년 유지 (신규 테이블)
MariaDB 이벤트 스케줄러로 매일 자동 집계 & 삭제 실행
```

---

### 📋 PHASE 7 프롬프트 (복사하여 그대로 사용)

```
Smart Farm Intelligence OS의 MariaDB에 데이터 보존 정책을 추가해 주세요.
현재 sensors 테이블에 무한정 쌓이는 RAW 데이터를 관리하고,
장기 추이 분석을 위한 집계 테이블을 추가합니다.

## 현재 DB 구조 (관련 테이블)
- sensors: id(BIGINT AK), timestamp(DATETIME), sensor_id(VARCHAR50), value(FLOAT)
- sensor_metadata: sensor_id, house_id, alias, type, unit, ... (설정 정보)

## 요청 사항

### 1. database/init.sql에 집계 테이블 및 이벤트 스케줄러 추가
   
   다음 테이블들을 추가해 주세요:
   - sensors_hourly (sensor_id, hour_timestamp, avg_value, min_value, max_value, sample_count)
   - sensors_daily (sensor_id, day_date, avg_value, min_value, max_value, sample_count)

   다음 MariaDB 이벤트를 추가해 주세요:
   - 매시간 실행: sensors 테이블에서 방금 지난 1시간 데이터를 sensors_hourly에 집계 INSERT
   - 매일 자정 실행:
     * sensors_daily에 전날 데이터 집계 INSERT
     * sensors에서 30일 이상 지난 데이터 DELETE
     * sensors_hourly에서 365일 이상 지난 데이터 DELETE

### 2. HistoryChart.tsx 수정 (프론트엔드)
   - 현재 조회 기간이 7일 이하: sensors 테이블 RAW 데이터 사용 (기존 동작)
   - 7일~90일: sensors_hourly 데이터 사용 (신규 API 호출)
   - 90일 초과: sensors_daily 데이터 사용 (신규 API 호출)
   - 날짜 범위 선택 시 자동으로 적절한 데이터 소스 선택

### 3. Router.cpp에 집계 데이터 조회 API 추가 (백엔드)
   - GET /api/sensors/{sensor_id}/history?start=ISO8601&end=ISO8601&resolution=auto
     * resolution=auto 이면 기간에 따라 raw/hourly/daily 자동 선택
     * resolution=hourly, daily 이면 강제 선택
   - 기존 히스토리 API 엔드포인트도 유지

### 4. HouseManager.cpp 또는 새 DataManager.cpp에 집계 데이터 조회 로직 추가
   - getHistory(sensor_id, start, end, resolution) 함수

변경/추가된 파일 전체를 모두 출력해 주세요.
```

---
---

## 📌 실행 순서 권장 가이드

```
✅ Phase 1 (SQL 보안)    → 먼저! 다른 모든 작업의 전제 조건
✅ Phase 2 (인증)        → Phase 1 완료 후 진행
✅ Phase 4 (룰 엔진)     → Phase 1 완료 후 독립적으로 진행 가능
✅ Phase 5 (반응형 UI)   → 백엔드와 무관, 언제든 진행 가능
✅ Phase 3 (Nginx)       → Phase 2 완료 후 진행 (토큰 쿠키/헤더 설정 연계)
✅ Phase 6 (알림)        → Phase 1 완료 후 진행
✅ Phase 7 (데이터 보존) → 가장 마지막 (운영 안정화 후)
```

> **각 프롬프트는 독립적으로 Antigravity에 붙여넣기하여 사용 가능합니다.**  
> 한 번에 하나씩 진행하고, 각 Phase 완료 후 `docker compose up -d --build`로 검증 권장.
