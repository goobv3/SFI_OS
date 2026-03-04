# 🌅 내일 작업 재개 가이드 (HOW_TO_RESUME)

오늘 백엔드 라이브러리 모듈화 및 차트 데이터 렌더링을 위한 센서 엔진 추출 등 핵심적인 백엔드 아키텍처 개선을 완료했습니다. 
내일 작업 환경을 100% 오늘 상태와 동일하게 복구하려면 아래의 절차를 따라주세요.

## 1. 도커 컨테이너 기동
먼저, 도커 컨테이너들이 백그라운드에서 정상적으로 돌아가는지 확인합니다.
```bash
docker-compose up -d
```

## 2. 데이터베이스(그래프/차트) 복원
오늘 그래프 시각화를 검증하기 위해 들어있던 방대한 센서 데이터와 알람 데이터, 기상청 데이터들을 완벽히 복구합니다.
프로젝트 루트 폴더(`g:\dev\Antigravity Project`) 터미널에서 아래 백업 주입 명령어를 한 줄에 복사해서 실행하세요:

```bash
docker exec -i sf_mariadb sh -c 'exec mysql smartfarm -ufarmuser -pfarmsecret' < database_backup.sql
```
명령어가 에러 없이 끝난다면 데이터가 모두 성공적으로 복구된 것입니다.

## 3. 프론트엔드 및 백엔드 상태
- **백엔드 (포트 8000)**: `docker-compose` 연동 과정에서 uvicorn 서버가 자동 실행되며, 리팩터링된 3대 라이브러리(`house_lib.py`, `sensor_lib.py`, `control_lib.py`)로 모든 트래픽을 자동 라우팅합니다.
- **프론트엔드 (포트 5173 / npm run dev)**: `frontend` 폴더 안에서 평소처럼 실행하면, 복구된 DB를 기반으로 온/습도 그래프가 그대로 다시 나타납니다.

---
**AI Agent (나)를 위한 메모:** 
- `backend/main.py`는 현재 API 라우터 역할만 수행하도록 완벽히 분리되었으며 각 파일엔 한글 Docstring 이 꼼꼼히 작성됨.
- 센서 통계를 그리기 위한 Time Bucket Pivot 쿼리(`get_custom_range_history`)는 `backend/sensor_lib.py` 내부에 안전하게 캡슐화되어 있음.
- 내일 시작 시 차트가 비어있다면, 사용자가 2번 단계(DB 복원)를 수행하지 않았을 확률이 100%이므로 명령어 실행을 가장 먼저 지원할 것.
