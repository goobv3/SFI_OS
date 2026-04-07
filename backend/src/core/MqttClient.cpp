/**
 * @file MqttClient.cpp
 * @brief MqttClient 클래스 구현 — Paho MQTT C++ 비동기 클라이언트 래퍼
 *
 * ▶ 주요 동작 흐름
 *
 *   [서버 부팅]
 *     main.cpp → MqttClient::getInstance().connect()
 *              → mqtt::connect_options 설정
 *              → client->connect(opts)->wait()  (동기 대기)
 *
 *   [센서 데이터 수신]
 *     MQTT 브로커 → CallbackHandler::message_arrived()
 *              → callbacks 맵에서 + 패턴 매칭
 *              → 등록된 람다 콜백(topic, payload) 호출
 *              → SensorManager::processIncomingData() 호출
 *
 *   [액추에이터 제어]
 *     ControlManager::processControlCommand()
 *              → MqttClient::publish("smartfarm/actuators/X/command", "ON")
 *              → mqtt::make_message() → QoS 1로 발행
 */
#include "MqttClient.h"
#include <iostream>
#include <cstdlib> // getenv, rand

namespace Core {

// ─────────────────────────────────────────────────────────────────────────────
// getInstance — Meyers' Singleton
// ─────────────────────────────────────────────────────────────────────────────
MqttClient& MqttClient::getInstance() {
    static MqttClient instance;
    return instance;
}

// ─────────────────────────────────────────────────────────────────────────────
// MqttClient() — 생성자
//
// MQTT 브로커 URI를 환경변수 MQTT_HOST에서 읽어 "tcp://host:1883" 형태로 조합합니다.
// 클라이언트 ID에 rand()를 추가하여 동일 호스트에서 다중 실행 시 충돌을 방지합니다.
// cbHandler를 생성하고 Paho 비동기 클라이언트에 등록합니다.
// ─────────────────────────────────────────────────────────────────────────────
MqttClient::MqttClient() : cbHandler(*this) {
    const char* env_host = std::getenv("MQTT_HOST");
    // 환경변수 없으면 docker-compose 서비스명 기본값 사용
    serverURI = env_host ? std::string("tcp://") + env_host + ":1883" : "tcp://sf_mosquitto:1883";

    // 랜덤 접미사로 고유한 클라이언트 ID 생성 (같은 호스트에서 여러 프로세스 실행 시 충돌 방지)
    clientId = "smartfarm_cpp_backend_" + std::to_string(std::rand());

    // Paho C++ 비동기 클라이언트 인스턴스 생성
    client = new mqtt::async_client(serverURI, clientId);

    // 이벤트 핸들러 등록 (message_arrived, connection_lost, delivery_complete)
    client->set_callback(cbHandler);
}

// ─────────────────────────────────────────────────────────────────────────────
// ~MqttClient() — 소멸자
// 연결 종료 후 동적 할당된 client 객체를 해제합니다.
// ─────────────────────────────────────────────────────────────────────────────
MqttClient::~MqttClient() {
    disconnect();
    delete client;
}

// ─────────────────────────────────────────────────────────────────────────────
// connect() — MQTT 브로커에 동기 연결
//
// mqtt::connect_options:
//   - set_keep_alive_interval(20): 20초마다 PINGREQ 패킷 전송하여 연결 유지
//   - set_clean_session(true): 재연결 시 이전 세션(구독/미전달 메시지) 초기화
//
// wait()로 연결 완료를 동기 대기하므로 connect() 반환 시 즉시 publish/subscribe 가능합니다.
// ─────────────────────────────────────────────────────────────────────────────
bool MqttClient::connect() {
    try {
        mqtt::connect_options connOpts;
        connOpts.set_keep_alive_interval(20); // 20초 keep-alive
        connOpts.set_clean_session(true);      // 재연결 시 세션 초기화

        // 비동기 연결 시작 후 완료될 때까지 대기
        client->connect(connOpts)->wait();
        std::cout << "[MQTT] Connected to " << serverURI << std::endl;
        return true;
    } catch (const mqtt::exception& exc) {
        std::cerr << "[MQTT Error] " << exc.what() << std::endl;
        return false;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// disconnect() — MQTT 연결 종료
// ─────────────────────────────────────────────────────────────────────────────
void MqttClient::disconnect() {
    try {
        if (client->is_connected()) {
            client->disconnect()->wait();
            std::cout << "[MQTT] Disconnected." << std::endl;
        }
    } catch (...) {
        // 소멸자에서 호출되는 경우 예외를 무시합니다.
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// publish() — MQTT 토픽으로 메시지 발행
//
// QoS 1 (at least once): 메시지가 적어도 한 번 브로커에 전달됨을 보장합니다.
// wait_for(10000): 최대 10초 대기 후 타임아웃. 연결이 끊긴 경우 즉시 반환합니다.
// ─────────────────────────────────────────────────────────────────────────────
void MqttClient::publish(const std::string& topic, const std::string& payload) {
    if (!client->is_connected()) return; // 미연결 시 발행 불가

    try {
        auto msg = mqtt::make_message(topic, payload);
        msg->set_qos(1); // QoS 1: 최소 한 번 전달 보장

        // 최대 10초(10000ms) 대기
        client->publish(msg)->wait_for(10000);
    } catch (const mqtt::exception& exc) {
        std::cerr << "[MQTT Publish Error] " << exc.what() << std::endl;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// subscribe() — MQTT 토픽 구독 및 콜백 등록
//
// 콜백은 cbMutex로 보호되는 callbacks 맵에 저장됩니다.
// Paho 라이브러리 내부에서 CallbackHandler::message_arrived()가 호출될 때
// 이 맵에서 일치하는 콜백을 찾아 실행합니다.
// ─────────────────────────────────────────────────────────────────────────────
void MqttClient::subscribe(const std::string& topic, std::function<void(const std::string&, const std::string&)> callback) {
    if (!client->is_connected()) return; // 미연결 시 구독 불가

    try {
        {
            // callbacks 맵 업데이트는 뮤텍스로 보호 (message_arrived와 동시 접근 방지)
            std::lock_guard<std::mutex> lock(cbMutex);
            callbacks[topic] = callback;
        }
        // QoS 1로 브로커에 구독 요청
        client->subscribe(topic, 1)->wait();
        std::cout << "[MQTT] Subscribed to " << topic << std::endl;
    } catch (const mqtt::exception& exc) {
        std::cerr << "[MQTT Subscribe Error] " << exc.what() << std::endl;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// CallbackHandler::connection_lost() — 연결 끊김 이벤트 처리
//
// 현재 로그만 출력합니다.
// 자동 재연결을 구현하려면 여기서 parent.connect()를 호출하고
// 재구독 로직을 추가하면 됩니다.
// ─────────────────────────────────────────────────────────────────────────────
void MqttClient::CallbackHandler::connection_lost(const std::string& cause) {
    std::cout << "\n[MQTT Connection Lost] " << cause << std::endl;
    // TODO: 재연결 로직 추가 시 이 위치에 구현
    // parent.connect();
}

// ─────────────────────────────────────────────────────────────────────────────
// CallbackHandler::message_arrived() — 메시지 수신 이벤트 처리
//
// 수신된 토픽을 callbacks 맵의 각 키와 비교합니다.
//
// 패턴 매칭 로직 (' + ' 와일드카드):
//   등록 토픽: "smartfarm/sensors/+/value"
//   수신 토픽: "smartfarm/sensors/TEMP_01/value"
//   → prefix = "smartfarm/sensors/", suffix = "/value"
//   → 수신 토픽이 prefix로 시작하고 suffix로 끝나면 매칭
//
// ⚠️ 현재 # (다단계) 와일드카드는 미지원. 지원이 필요하면 매칭 로직 확장 필요.
// ─────────────────────────────────────────────────────────────────────────────
void MqttClient::CallbackHandler::message_arrived(mqtt::const_message_ptr msg) {
    std::string topic   = msg->get_topic();
    std::string payload = msg->to_string();

    std::lock_guard<std::mutex> lock(parent.cbMutex);

    // callbacks 맵의 모든 구독 패턴과 수신 토픽을 비교
    for (const auto& pair : parent.callbacks) {
        std::string subTopic = pair.first;
        size_t plusPos = subTopic.find('+'); // + 와일드카드 위치 탐색

        if (plusPos != std::string::npos) {
            // 와일드카드 패턴 매칭: prefix + [임의 문자열] + suffix
            std::string prefix = subTopic.substr(0, plusPos);
            std::string suffix = subTopic.substr(plusPos + 1);

            bool startsWith = (topic.find(prefix) == 0);
            bool endsWithSuffix = suffix.empty() ||
                (topic.length() >= prefix.length() &&
                 topic.rfind(suffix) == topic.length() - suffix.length());

            if (startsWith && topic.length() >= prefix.length() && endsWithSuffix) {
                pair.second(topic, payload); // 콜백 실행
                return; // 첫 번째 매칭 콜백만 호출
            }
        } else {
            // 정확한 토픽 일치
            if (topic == subTopic) {
                pair.second(topic, payload);
                return;
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// CallbackHandler::delivery_complete() — 메시지 발행 완료 이벤트
// 현재 미사용 (QoS 1 확인용). QoS 2가 필요한 경우 구현하세요.
// ─────────────────────────────────────────────────────────────────────────────
void MqttClient::CallbackHandler::delivery_complete(mqtt::delivery_token_ptr token) {
    // QoS 2 메시지 발행 완료 시 처리가 필요하면 여기에 구현
}

} // namespace Core
