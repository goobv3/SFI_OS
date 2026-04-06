# Smart Farm Intelligence OS - 작업 로그 (Changelog)

## [2026-04-06] - 상용화 고도화 및 인프라 구축 (v2.0)

### 🚀 기능 추가 (New Features)
- **보안 및 인증**: 
  - JWT (JSON Web Token) HS256 인증 시스템 전면 도입.
  - PBKDF2-SHA256 패스워드 해싱 및 솔팅 로직 구현.
  - `/api/auth/login`, `/api/auth/me` 엔드포인트 신설.
- **인프라 및 배포**:
  - Nginx 리버프 프록시 추가 (80/443 포트 통합).
  - OpenSSL 자가서명 인증서를 이용한 HTTPS(TLS) 환경 구축.
  - Frontend 멀티스테이지 빌드(Nginx 서빙) Dockerfile 고도화.
- **모바일 반응형 UI**:
  - 하단 고정 탭바(Bottom Tab Bar) UI 구현 (모바일 환경 자동 전환).
  - Lucide React 아이콘셋 기반의 사용자 인터페이스 개선.
  - 날씨 위젯 가로 스크롤(Horizontal Snap Scroll) 레이아웃 적용.
- **데이터 관리 및 보존**:
  - `sensors_hourly`, `sensors_daily` 테이블을 통한 데이터 집계 기능.
  - MariaDB Event Scheduler 기반의 데이터 자동 삭제 정책 관리(30일 RAW, 365일 정산).
  - API 히스토리 조회 시 조회 범위에 따른 해상도(Resolution) 자동 선택 로직 탑재.
- **알림 시스템**:
  - `libcurl` 기반의 SMTP 이메일 발송 모듈 구축 (`critical` 이벤트 발생 시 이메일 전송).

### 🛠️ 수정 및 개선 (Fixes & Improvements)
- **연결 이슈**: 프론트엔드의 하드코딩된 API 주소를 상대 경로(`/api`)로 수정하여 리버스 프록시 연동 및 로그인 불가 문제 해결.
- **빌드 최적화**: 린트 오류(미사용 변수 등)를 수정하여 `npm run build` 성공률 100% 달성.
- **DB 안정성**: SQL 초기화 구문(`init.sql`)의 이벤트 문법 오류 수정.
- **컴포넌트 고도화**: `ManageMode`의 탭 전환 인터페이스 및 모바일 95vh 모달 크기 조정.

---
*작업일자: 2026년 4월 6일*
*작업자: Antigravity AI*
