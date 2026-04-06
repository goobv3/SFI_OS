# Stage 1: Build
FROM node:20-alpine AS builder

WORKDIR /app

# 의존성 파일 복사 및 설치
COPY package*.json ./
RUN npm install

# 소스 코드 복사 및 프로덕션 빌드
COPY . .
RUN npm run build

# Stage 2: Serve
FROM nginx:alpine

# 빌드된 정적 파일(dist)을 Nginx 웹 루트로 복사
COPY --from=builder /app/dist /usr/share/nginx/html

# 내부 노출 포트
EXPOSE 80

CMD ["nginx", "-g", "daemon off;"]
