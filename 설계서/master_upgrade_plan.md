# Smart Farm OS — 통합 업그레이드 마스터 플랜
> 현재 ~47% → 목표 ~83% | 우선순위 기반 단계별 실행 가이드

---

## 전체 작업 우선순위 한눈에 보기

| 순위 | 작업 | 영역 | 완성도 기여 | 난이도 |
|------|------|------|------------|--------|
| 🥇 **P1** | SQL Injection 보안 패치 | 보안 | +4% | ⭐⭐ |
| 🥇 **P2** | UX 레이아웃 + 디자인 시스템 혁신 | UX | +8% | ⭐⭐⭐ |
| 🥈 **P3** | 자동제어 룰 엔진 | 핵심기능 | +10% | ⭐⭐⭐⭐ |
| 🥈 **P4** | JWT 사용자 인증 | 보안 | +8% | ⭐⭐⭐ |
| 🥉 **P5** | 멀티사이트 계층 구조 | 아키텍처 | +5% | ⭐⭐⭐ |
| **P6** | 마이크로 인터랙션 + 실시간성 | UX | +3% | ⭐⭐ |
| **P7** | Nginx + HTTPS | 인프라 | +5% | ⭐⭐ |
| **P8** | 이메일 알림 연동 | 알림 | +4% | ⭐⭐⭐ |
| **P9** | 모바일 반응형 UI | UX | +4% | ⭐⭐⭐ |
| **P10** | 데이터 보존 정책 | DB | +3% | ⭐⭐ |

> **P1~P5 완료 시 누적 ~82% 달성.**  
> 각 프롬프트를 **새 대화**에서 하나씩 사용하세요.

---
---

# 🥇 P1 — SQL Injection 보안 패치

**왜 먼저?** 다른 모든 기능의 전제 조건. 인증 추가 전에 DB가 뚫려있으면 의미 없음.

```
## 작업 범위 (이 파일들만 읽어서 진행)
- backend/src/core/Database.h
- backend/src/core/Database.cpp
- backend/src/managers/HouseManager.cpp (가장 중요)
- backend/src/managers/ControlManager.cpp
- backend/src/managers/SensorManager.cpp

## 출력 제한
변경된 파일만 전체 출력. 변경 없는 파일은 출력 생략.

---

위 5개 파일을 읽고 다음을 구현해 주세요.

모든 Manager 파일의 SQL 쿼리가 문자열 연결 방식으로 외부 입력을 직접 삽입하고 있어
SQL Injection에 취약합니다. MySQL Prepared Statement로 전환합니다.

구현:
1. Database.h/cpp에 다음 두 메서드 추가:
   - bool executePS(string sql, vector<string> params)
   - vector<map<string,string>> fetchAllPS(string sql, vector<string> params)
   MySQL C API의 mysql_stmt_* 함수 사용, ? 플레이스홀더 바인딩 구현.

2. HouseManager.cpp: house_id, sensor_id, actuator_id, name 등
   외부 입력이 들어가는 모든 WHERE 절과 INSERT VALUES를 executePS/fetchAllPS로 교체.
   상수 쿼리(ORDER BY display_order 등)는 기존 execute/fetchAll 유지해도 됨.

3. ControlManager.cpp, SensorManager.cpp도 동일하게 교체.

기존 함수 시그니처와 반환값은 그대로 유지. 기능 변경 없음.
```

---

# 🥇 P2 — UX 레이아웃 + 디자인 시스템 혁신

**왜 먼저?** 이후 모든 UI 작업의 기반. 레이아웃이 바뀌어야 P5(멀티사이트) UI도 자연스럽게 붙음.

```
## 작업 범위 (이 파일들만 읽어서 진행)
- frontend/src/App.tsx
- frontend/src/index.css
- frontend/src/components/SensorDashboard.tsx
- frontend/src/components/SensorGauge.tsx

## 출력 제한
변경된 파일만 전체 출력. 나머지(WeatherWidget, ManageMode 등)는 건드리지 말고 출력도 생략.

---

위 4개 파일을 읽고 다음을 구현해 주세요.
기술 스택: React 18 + TypeScript + Tailwind CSS v4 + Recharts

## 1. index.css — 디자인 토큰 추가
기존 토큰 유지하고 다음 추가:
- --color-neon-purple: #b94fff
- --color-neon-teal: #00d4aa
- --color-surface-card: #1e2130
- --color-surface-elevated: #252836
CSS @layer utilities에 추가:
- .animate-fade-in-up { animation: fadeInUp 0.3s ease forwards; }
  @keyframes fadeInUp { from { opacity:0; transform:translateY(8px); } to { opacity:1; transform:none; } }
- .hover-lift { transition: transform 0.2s, box-shadow 0.2s; }
  .hover-lift:hover { transform: translateY(-2px); box-shadow: 0 8px 24px rgba(0,0,0,0.4); }

## 2. App.tsx — 사이드바 + 탭 레이아웃
현재 단일 세로 스크롤 → 좌우 분할 레이아웃:
- 전체 구조: flex h-screen overflow-hidden
- 좌측 Sidebar (w-56 shrink-0, bg-surface-card, border-r):
  - 상단: 앱 로고 (기존 헤더의 로고 이동)
  - 중앙: house 목록 (현재 드롭다운 → 세로 리스트)
    각 항목: 클릭 시 selectedHouseId 변경, 활성 항목에 좌측 2px neon-blue 바
  - 하단: 날씨 접기/펼치기 토글 버튼 + 시스템 상태 dot
- 우측 Main (flex-1 overflow-y-auto):
  - 슬림 헤더 (h-12): 현재 house명 브레드크럼 | 알람배지 | 언어토글 | 설정버튼
  - 상단 탭바: [환경현황 | 제어 | 날씨] 탭 (state로 관리)
  - 탭 콘텐츠: 환경현황→SensorDashboard, 제어→ActuatorControl, 날씨→WeatherWidget
- 모바일(md 미만): 사이드바 숨김, 하단 fixed 탭바(4개 아이콘)로 대체

기존 ManageMode, AlarmToast, QuickAddModal은 위치 그대로 유지.
discoveredDevices 폴링 로직 그대로 유지.

## 3. SensorDashboard.tsx — 카드 개선
- 그리드: grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-3
- 각 카드 최소 높이: min-h-[180px], bg-surface-card, rounded-xl
- 카드 내부 구조 (위→아래):
  Row1: [타입 아이콘(색상적용)] [센서 alias] [설정 버튼]
  Row2: SensorGauge (flex-1, 더 넓게)
  Row3: [마지막 업데이트 "N분 전"] [상태 뱃지]
- hover: hover-lift 클래스 + border-color transition to 해당 타입 색상
- 비활성: grayscale + opacity-40 + "수집 중지" 오버레이
- 센서 타입별 색상 매핑:
  temp → neon-orange, humidity → neon-blue,
  co2 → neon-purple, solar → yellow-400, 기타 → neon-teal

## 4. SensorGauge.tsx — 수치 가독성 향상
- 값 표시 폰트: text-2xl (현재 text-lg에서 확장)
- 상태 텍스트: useLanguage 훅 import 후 t('statusNormal') 등으로 다국어 처리
  (훅이 없으면 prop으로 lang 받아도 됨)
- 임계치 배경 아크 opacity: 0.15 (현재 0.3에서 축소, 더 은은하게)
- 전체 컨테이너 height: h-[130px] (현재 h-[110px]에서 확장)
```

---

# 🥈 P3 — 자동제어 룰 엔진

**왜 중요?** 이게 없으면 "모니터링 SW". 핵심 가치 창출 기능.  
**선행 조건:** P1(SQL 보안 패치) 완료 후 진행 권장.

```
## 작업 범위 (이 파일들만 읽어서 진행)
- backend/src/managers/SensorManager.cpp + SensorManager.h
- backend/src/managers/ControlManager.cpp + ControlManager.h
- backend/src/api/Router.cpp
- database/init.sql (테이블 구조 파악용, 수정 필요)
- frontend/src/api/client.ts

## 출력 제한
신규 파일(RuleEngine.h, RuleEngine.cpp, AutomationPanel.tsx)은 전체 출력.
기존 파일은 변경된 부분만 diff 형태로 출력 (변경 전 → 변경 후).
프론트엔드 완성 컴포넌트 코드는 핵심 구조만, 반복적인 JSX는 요약 가능.

---

위 파일들을 읽고 다음을 구현해 주세요.

## 백엔드
1. database/init.sql에 추가:
CREATE TABLE automation_rules (
  id BIGINT AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(100),
  house_id VARCHAR(50),
  trigger_sensor_id VARCHAR(50),
  condition_type ENUM('GT','LT','GTE','LTE') NOT NULL,
  threshold_value FLOAT NOT NULL,
  actuator_id VARCHAR(50),
  action_command VARCHAR(50),
  is_enabled BOOLEAN DEFAULT TRUE,
  cooldown_minutes INT DEFAULT 5,
  last_triggered_at DATETIME NULL,
  FOREIGN KEY (house_id) REFERENCES houses(house_id) ON DELETE CASCADE,
  FOREIGN KEY (trigger_sensor_id) REFERENCES sensor_metadata(sensor_id) ON DELETE CASCADE,
  FOREIGN KEY (actuator_id) REFERENCES actuator_metadata(actuator_id) ON DELETE CASCADE
);

2. backend/src/managers/RuleEngine.h + RuleEngine.cpp 신규 생성:
   싱글톤, evaluate(sensor_id, double value) 함수:
   - is_enabled=true 룰 중 해당 sensor_id 조회
   - 조건(GT/LT/GTE/LTE) 평가
   - cooldown 체크 (last_triggered_at + cooldown_minutes < now)
   - 통과 시 ControlManager::getInstance().processControlCommand(actuator_id, action_command, "AutoRule") 호출
   - last_triggered_at 업데이트
   CRUD: getRules(house_id), createRule(json), updateRule(id,json), deleteRule(id), toggleRule(id,bool)

3. SensorManager.cpp의 processIncomingData() 끝에 한 줄 추가:
   Managers::RuleEngine::getInstance().evaluate(sensor_id, value);

4. Router.cpp에 추가:
   GET/POST /api/automation/rules (query: ?house_id=)
   PUT/DELETE /api/automation/rules/<int:id>
   PATCH /api/automation/rules/<int:id>/toggle

## 프론트엔드
5. frontend/src/components/AutomationPanel.tsx 신규 생성:
   - 룰 목록 카드: "IF [센서명] [조건기호] [값] → [액추에이터] [명령]" 시각적 표현
   - 각 카드: 활성토글(switch), 편집, 삭제 버튼
   - 룰 추가 모달: 하우스선택→센서선택→조건/값→액추에이터→명령→쿨다운 순서
   - 기존 Neon 다크 테마 유지 (neon-blue/green/orange 강조색)

6. client.ts에 automation API 함수 추가 (기존 패턴과 동일 방식)

7. P2에서 만든 탭바에 [자동화] 탭 추가하여 AutomationPanel 연결.
   P2 미완료 시: App.tsx에 별도 "자동화" 버튼으로 모달 형태로 연결.
```

---

# 🥈 P4 — JWT 사용자 인증

**선행 조건:** P1 완료 후 진행.

```
## 작업 범위 (이 파일들만 읽어서 진행)
- backend/src/api/Router.cpp (라우트 구조 파악)
- backend/src/core/Database.h (DB 메서드 확인)
- backend/CMakeLists.txt (의존성 추가용)
- docker-compose.yml (환경변수 추가용)
- frontend/src/api/client.ts
- frontend/src/App.tsx (Provider 감싸기용)

## 출력 제한
신규 파일(JwtUtils.h/cpp, LoginPage.tsx, AuthContext.tsx)은 전체 출력.
기존 파일은 변경된 코드 블록만 출력 (함수 단위).
SQL은 ALTER/INSERT 구문만 출력.

---

위 파일들을 읽고 JWT 인증을 추가해 주세요.

## 백엔드
1. database/init.sql에 추가 (기존 테이블 변경 없이):
   CREATE TABLE users (...) + INSERT admin 계정
   admin 비밀번호 'admin1234'를 bcrypt 해시로 저장

2. backend/src/core/JwtUtils.h + JwtUtils.cpp 신규:
   - HS256 JWT 생성/검증 (외부 라이브러리 없이 HMAC-SHA256 직접 구현 또는
     header-only jwt-cpp 사용, CMakeLists.txt도 함께 수정)
   - generateToken(username, role, expiry_hours) → string
   - verifyToken(token) → {valid:bool, username:string, role:string}
   - SECRET: 환경변수 JWT_SECRET (기본값 "sf_secret_2026")

3. Router.cpp 수정:
   - POST /api/auth/login → DB users 조회, bcrypt 검증, 토큰 반환
   - GET /api/auth/me → Bearer 토큰 검증 후 사용자 정보 반환
   - 토큰 검증 미들웨어: /api/auth/* 제외 모든 /api/* 에 적용
     유효하지 않으면 {"error":"Unauthorized"} 401 반환

## 프론트엔드
4. frontend/src/contexts/AuthContext.tsx 신규:
   login/logout 함수, isAuthenticated, user(username,role) 상태
   access_token을 localStorage에 저장

5. frontend/src/pages/LoginPage.tsx 신규:
   기존 Neon 다크 테마 적용, username/password 입력, 에러 메시지

6. client.ts에 Axios 인터셉터 추가:
   - 요청에 Authorization: Bearer {token} 자동 첨부
   - 401 응답 시 localStorage 토큰 삭제 → /login 이동

7. App.tsx: AuthContext.Provider로 감싸기, 미인증 시 LoginPage 렌더링
```

---

# 🥉 P5 — 멀티사이트(농장) 계층 구조

**선행 조건:** P1, P4 완료 권장 (역할별 접근제어 연계).

```
## 작업 범위 (이 파일들만 읽어서 진행)
- database/init.sql
- backend/src/api/Router.cpp
- backend/src/managers/HouseManager.h (구조 파악용)
- frontend/src/App.tsx
- frontend/src/api/client.ts

## 출력 제한
신규 파일(SiteManager.h/cpp, SiteOverview.tsx)은 전체 출력.
기존 파일은 추가/변경된 부분만 출력.
SiteOverview JSX는 카드 1개 예시만 쓰고 나머지는 map으로 표현.

---

위 파일들을 읽고 멀티사이트 계층을 추가해 주세요.

## 백엔드
1. init.sql에 추가:
   CREATE TABLE sites (site_id PK, name, location, timezone, created_at)
   ALTER TABLE houses ADD COLUMN site_id VARCHAR(50) REFERENCES sites
   기본 사이트 INSERT: ('SITE_DEFAULT', '기본 농장')
   기존 더미 HOUSE_1에 site_id='SITE_DEFAULT' 연결

2. SiteManager.h + SiteManager.cpp 신규:
   getSites() → 각 site에 house_count, active_alarm_count 포함
   getSiteOverview(site_id) → 사이트 내 모든 sensor의 최신값 요약
   createSite/updateSite/deleteSite
   getSiteHouses(site_id)

3. Router.cpp에 추가:
   GET/POST /api/sites
   GET /api/sites/<string>/overview
   GET /api/sites/<string>/houses
   PUT/DELETE /api/sites/<string>

## 프론트엔드
4. SiteOverview.tsx 신규 (관리자용 전체 현황):
   모든 사이트 카드 그리드, 각 카드에 농장명/위치/동수/알람상태/평균온도
   경보 있는 카드: 빨간/주황 border glow, 30초 자동 갱신

5. App.tsx 사이드바 수정 (P2 완료 기준):
   사이드바에 site → house 2단계 트리 메뉴
   site 클릭 → house 목록 펼침, house 클릭 → 기존 대시보드
   헤더 브레드크럼: "A농장 > 1동"
   site가 1개뿐이면 기존처럼 동 목록만 표시 (하위 호환)

6. client.ts에 Site API 함수 추가
   getSites(), getSiteOverview(siteId), createSite(data) 등
```

---

# P6 — 마이크로 인터랙션 + 실시간성

```
## 작업 범위 (이 파일들만 읽어서 진행)
- frontend/src/components/SensorDashboard.tsx
- frontend/src/components/ActuatorControl.tsx
- frontend/src/components/AlarmToast.tsx

## 출력 제한
변경된 파일만 출력. 변경 없는 파일 생략.
반복적인 sensor.map() 내부 JSX는 핵심 변경 부분만 출력.

---

위 3개 파일을 읽고 다음 인터랙션을 추가해 주세요.

1. SensorDashboard: 30초 자동 폴링 추가 (useEffect interval)
   값 변화 감지 시 이전값 ref로 보관, 증감 표시 ↑↓ + 변화량 (초록/파랑)
   카드 하단에 마지막 업데이트 시간 "N분 전" 포맷 (date-fns 없이 직접 구현)
   로딩 중: Skeleton UI (animate-pulse bg-gray-800 rounded으로 카드 형태 유지)

2. ActuatorControl: ON/OFF 토글을 스위치 디자인으로 교체
   명령 전송 중 loading state (버튼 비활성화 + 스피너)
   성공: 체크마크 0.8초 표시 후 복귀
   실패: shake 애니메이션 (CSS @keyframes shake)

3. AlarmToast: 우측 하단 fixed, 최대 3개 스택
   새 알람: 우측에서 슬라이드 인 (transform translateX)
   확인 클릭: 슬라이드 아웃
   critical 알람: 배경 pulse 레드 glow
```

---

# P7 — Nginx + HTTPS

```
## 작업 범위 (이 파일들만 읽어서 진행)
- docker-compose.yml
- frontend/Dockerfile

## 출력 제한
nginx/nginx.conf, nginx/Dockerfile 신규 파일 전체 출력.
docker-compose.yml, frontend/Dockerfile은 전체 출력 (짧은 파일이므로).

---

위 파일들을 읽고 Nginx 리버스 프록시를 추가해 주세요.

1. nginx/nginx.conf:
   80 → 443 리디렉션, 자가서명 SSL (경로 /etc/nginx/certs/)
   /api/* → http://backend:8000 프록시, 나머지 → React 정적 파일
   gzip 활성화, proxy_timeout 60s

2. nginx/Dockerfile:
   nginx:alpine 베이스, openssl로 자가서명 인증서 빌드 시 자동 생성

3. frontend/Dockerfile을 멀티스테이지로 변경:
   stage1(builder): node:20-alpine, npm run build
   stage2: nginx:alpine, builder의 /app/dist를 /usr/share/nginx/html에 복사

4. docker-compose.yml 수정:
   nginx 서비스 추가 (80:80, 443:443), frontend/backend 외부 포트 제거
   nginx가 frontend/backend에 depends_on
```

---

# P8 — 이메일 알림

```
## 작업 범위 (이 파일들만 읽어서 진행)
- backend/src/managers/SensorManager.cpp (알람 발생 위치 확인)
- backend/CMakeLists.txt (libcurl 의존성 추가용)
- docker-compose.yml (환경변수 추가용)

## 출력 제한
신규 파일(EmailSender.h/cpp) 전체 출력.
기존 파일은 변경 부분만 출력. 이메일 HTML 본문은 간단한 텍스트로 대체 가능.

---

위 파일들을 읽고 이메일 알림을 추가해 주세요.

1. backend/src/core/EmailSender.h + EmailSender.cpp 신규:
   libcurl SMTP 사용, sendAlert(sensor_alias, level, value, message) 함수
   환경변수: SMTP_HOST, SMTP_PORT, SMTP_USER, SMTP_PASS, ALERT_EMAIL
   이메일 제목: "[스마트팜 경보] {alias} {level} 감지"
   본문: 현재값, 임계치, 발생시각 (한국어 텍스트)

2. SensorManager.cpp의 알람 INSERT 직후:
   level == "critical" 일 때만 EmailSender::sendAlert() 호출
   warn 레벨은 DB 기록만

3. CMakeLists.txt에 libcurl 링크 추가
4. docker-compose.yml backend environment에 SMTP 변수 추가 (빈값)
```

---

# P9 — 모바일 반응형 UI

```
## 작업 범위 (이 파일들만 읽어서 진행)
- frontend/src/App.tsx (P2 완료 후 버전 기준)
- frontend/src/components/WeatherWidget.tsx
- frontend/src/components/ManageMode.tsx

## 출력 제한
각 파일에서 반응형 클래스가 추가된 JSX 블록만 출력.
변경 없는 함수/로직은 "// ... 기존 코드 유지" 코멘트로 대체.
ManageMode는 레이아웃 구조 변경 부분만 출력.

---

위 3개 파일을 읽고 모바일 반응형을 적용해 주세요.
기준: sm(640px), md(768px), lg(1024px)

1. App.tsx (P2 기준 사이드바 레이아웃):
   md 미만에서 사이드바 숨김 + 하단 fixed 탭바로 대체 (z-50, h-14)
   탭바 아이콘: 대시보드/제어/날씨/설정 (lucide-react 아이콘 사용)

2. WeatherWidget.tsx:
   모바일: 현재 날씨만 표시, 예보는 가로 스크롤 flex (overflow-x-auto, snap-x)
   PC: 기존 레이아웃 유지

3. ManageMode.tsx:
   모바일: 현재 좌우 2단(w-1/3 + flex-1) → 상단 탭 전환(하우스목록 탭 | 기기편집 탭)
   모달 크기: 모바일 h-[95vh], PC 기존 h-[82vh] 유지
```

---

# P10 — 데이터 보존 정책

```
## 작업 범위 (이 파일들만 읽어서 진행)
- database/init.sql (sensors 테이블 구조 확인)
- frontend/src/components/HistoryChart.tsx (조회 기간 로직 확인)
- backend/src/api/Router.cpp (history API 위치 확인)

## 출력 제한
init.sql은 추가되는 CREATE TABLE + EVENT 구문만 출력.
HistoryChart는 조회 기간 판단 로직과 API 호출 부분만 출력.
Router.cpp는 신규 추가 라우트 블록만 출력.

---

위 파일들을 읽고 데이터 보존 정책을 추가해 주세요.

1. init.sql에 추가:
   CREATE TABLE sensors_hourly (sensor_id, hour_ts DATETIME, avg_val, min_val, max_val, cnt)
   CREATE TABLE sensors_daily (sensor_id, day_date DATE, avg_val, min_val, max_val, cnt)
   
   MariaDB EVENT 2개:
   - 매시간: sensors에서 직전 1시간 데이터를 sensors_hourly로 집계 INSERT IGNORE
   - 매일 자정:
     * sensors_daily에 전날 데이터 집계
     * sensors에서 30일 초과 데이터 DELETE
     * sensors_hourly에서 365일 초과 데이터 DELETE
   SET GLOBAL event_scheduler = ON; 포함

2. Router.cpp에 추가:
   GET /api/sensors/<string>/history?start=&end=&resolution=auto
   resolution=auto: 기간 7일 이하→sensors RAW, 7~90일→sensors_hourly, 90일+→sensors_daily

3. HistoryChart.tsx 수정:
   날짜 범위 계산 후 daysDiff에 따라 API resolution 파라미터 자동 선택
   기존 API 호출 부분만 수정, 나머지 차트/UI 로직 유지
```

---

## 빠른 참조 — 파일별 작업 매핑

| 파일 | 관련 작업 |
|------|----------|
| `database/init.sql` | P1, P3, P4, P5, P10 |
| `backend/src/core/Database.h/cpp` | P1, P4 |
| `backend/src/api/Router.cpp` | P3, P4, P5, P7, P10 |
| `backend/src/managers/SensorManager` | P3, P8 |
| `frontend/src/App.tsx` | P2, P4, P5, P9 |
| `frontend/src/index.css` | P2 |
| `frontend/src/components/SensorDashboard` | P2, P6 |
| `frontend/src/components/SensorGauge` | P2 |
| `frontend/src/api/client.ts` | P4, P5 |
| `docker-compose.yml` | P7, P8 |
