# --- [프론트엔드 Dockerfile] ---
# Vite 기반 리액트 앱을 위한 도커 설정입니다.

FROM node:20-slim

WORKDIR /app

# 의존성 파일 복사 및 설치
COPY package*.json ./
RUN npm install

# 소스 코드 복사
COPY . .

# Vite 개발 서버 실행 (0.0.0.0으로 바인딩하여 외부 접근 허용)
# 포트는 기본적으로 5173을 사용합니다.
EXPOSE 5173

CMD ["npm", "run", "dev", "--", "--host", "0.0.0.0"]
