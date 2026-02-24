# Smart Farm Intelligence OS (스마트팜 지능형 운영체제)

![Smart Farm OS Preview](frontend/public/vite.svg)

**Smart Farm Intelligence OS**는 최첨단 퓨처리즘(Futurism) 및 사이버네틱(Cybernetic) 테마의 UI/UX를 갖춘 스마트 온실 종합 관제 시스템입니다.
이 시스템은 센서 데이터의 수집, 다양한 제어기(천창, 관수, 유동팬 등)의 원격 제어, 그리고 추이 분석을 위한 이력(History) 차트 기능을 제공합니다.

---

## 🎯 주요 기능 및 특징 소개
- **데이터 기반 대시보드:** 온·습도, 조도, CO2 농도 등 다양한 환경 센서 데이터를 실시간으로 직관적으로 통합 조회
- **제어기 인터락(Interlock) 시스템 체계:** 기기 보호를 위한 안전장치, 수동 모드 잠금 시스템(Safety Lock), 개폐/구동 중 동시 구동 차단
- **동적 디바이스 할당(Auto-Discovery):** 미등록 센서가 신호를 보낼 시 '미등록 보관함'으로 분류되며 UI에서 클릭하여 간편하게 정식 등록
- **분 단위 초정밀 데이터 집계:** 특정 센서 및 하우스의 데이터를 커스텀 기간(Custom Date Range)에 맞추어 분/시/일 단위 다중 그래프로 교차 비교 분석 
- **투야(Tuya) 스타일 기기 관리:** UI 내에서 동적으로 새로운 하우스(온실) 구역을 추가하고, 보유한 센서와 제어기를 직관적으로 드래그 앤 드롭 수준으로 등록 및 제어

---

## 📝 릴리스 노트 (버전별 업데이트 내역)

### 📌 v1.2.0 - 2026.02 (최근 업데이트)
**"다중 지표 커스텀 차트 및 UI 경험 고도화"** 🚀
- **히스토리 차트 전면 개편:**
  - 사용자가 임의의 날짜/시간 범위를 분(Minute) 단위까지 직접 지정할 수 있는 [Range Picker] 추가.
  - 3시간 이하 범위 조회 시 '분' 단위까지 세밀하게 데이터를 추출(Grouping)하여 렌더링.
  - [온도만 표시 / 모든 센서 동시 겹쳐 보기] 등 다중 지표(Multi-Metric) 필터링 지원 토글 버튼 구현.
  - 1분 주기로 1,400개 이상의 더미 데이터를 주입 및 성능 테스트 완료.
- **UI 텍스트 및 레이아웃 개선:** 대시보드 메인 타이틀을 `Antigravity OS`에서 `Smart Farm Intelligence OS`로 변경.

### 📌 v1.1.0 - 2026.02
**"동적 자동 스캔(Auto-Discovery) 시스템 및 하우스 관리 구역화"** 🔄
- 하드코딩된 더미 하우스와 센서 목록 탈피 -> DB와 백엔드를 연동한 완전한 CRUD 관리 모드 제공.
- 백엔드가 등록되지 않은 센서 데이터를 수신했을 경우 폐기하지 않고 DB 내 대기실(`unregistered_devices` 테이블)로 적재.
- 프론트엔드 모달 내에 투야(Tuya)형 '설정/관리 모드'를 구성하여, 발견된 기기를 사용자가 직접 속성과 이름을 부여해 시스템 내에 정식 편입(승급)할 수 있는 인프라 구축.

### 📌 v1.0.0 - (Initial Release)
**"스마트팜 백엔드 인터락 구조 및 퓨쳐리즘 대시보드 기틀 확립"** 🌟
- FastAPI 기반 Python 백엔드 아키텍처 및 MariaDB 데이터베이스 초기(`init.sql`) 구성.
- React(Vite) + Recharts + Tailwind CSS 기반의 네온(Neon) 테마 다크 모드 UI 확립.
- 제어기 동시 동작 안전 모드 및 우선순위(Priority), 비상 정지 등의 중재/인터락 보안 및 로직 구조(`arbitration.py`) 완비.
- `Docker Compose`를 이용한 무중단 오케스트레이션 및 프론트/백/DB 도커라이징 완료.

---

## 🛠 기술 스택 (Tech Stack)
### Frontend
- **React 18 & Typescript**, **Vite**
- **시각화:** Recharts, Lucide-react (아이콘)
- **스타일링:** Tailwind CSS, Vanilla CSS 기반 글로우(Glow)/애니메이션 효과
- **통신:** Axios 라이브러리 연동 API

### Backend & DB
- **Framework:** Python `FastAPI`
- **DB:** `MariaDB 11` & `PyMySQL`
- **컨테이너화:** `Docker` & `Docker-Compose`

---

## 🚀 기동(실행) 방법
이 저장소에는 프론트엔드, 백엔드, DB의 환경이 포함되어 있습니다. Docker가 설치된 환경에서 아래 명령어 한 줄로 즉시 모든 시스템 구동이 가능합니다.

```bash
# 최상위 폴더(Goob/dev/Antigravity Project)에서 다음 명령어 실행
docker compose up -d --build
```

1. **프론트엔드 URL:** [http://localhost:5173/](http://localhost:5173/) 로 접속합니다 (또는 Vite 개발서버 구동 시).
2. **백엔드 API Server:** [http://localhost:8000/docs](http://localhost:8000/docs) 에서 자동화된 Swagger REST API 명세서를 확인할 수 있습니다.
