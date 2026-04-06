FROM nginx:alpine

# OpenSSL 설치
RUN apk update && apk add --no-cache openssl

# 인증서 디렉토리 생성 및 자가서명 인증서 생성
RUN mkdir -p /etc/nginx/certs && \
    openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
    -keyout /etc/nginx/certs/selfsigned.key \
    -out /etc/nginx/certs/selfsigned.crt \
    -subj "/C=KR/ST=Seoul/L=Seoul/O=SmartFarm/OU=IT/CN=localhost"

# nginx.conf 복사
COPY nginx.conf /etc/nginx/nginx.conf

# 포트 노출
EXPOSE 80 443
