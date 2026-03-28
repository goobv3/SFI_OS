FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libmariadb-dev \
    libpaho-mqtt-dev \
    libpaho-mqttpp-dev \
    libssl-dev \
    libasio-dev \
    libcurl4-openssl-dev \
    libboost-all-dev \
    curl \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN mkdir -p build && cd build && cmake .. && make -j2

EXPOSE 8000
CMD ["./build/smartfarm_server"]
