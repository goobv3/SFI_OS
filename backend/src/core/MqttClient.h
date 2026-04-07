/**
 * @file MqttClient.h
 * @brief MQTT 비동기 클라이언트 싱글톤 — 센서 데이터 수신 및 액추에이터 제어 명령 발행
 *
 * ▶ 역할
 *   - Paho MQTT C++ 라이브러리를 래핑하여 스마트팜 시스템의 MQTT 통신을 담당합니다.
 *   - 토픽 구독(subscribe)과 메시지 발행(publish) 기능을 제공합니다.
 *   - 와일드카드(+) 토픽 패턴을 지원하여 다수의 센서를 일괄 구독할 수 있습니다.
 *
 * ▶ 토픽 구조
 *   - 센서 데이터 수신: smartfarm/sensors/<sensor_id>/value
 *   - 액추에이터 제어: smartfarm/actuators/<actuator_id>/command
 *   - 농장 날씨 데이터: smartfarm/weather/farm
 *
 * ▶ 환경변수
 *   MQTT_HOST : MQTT 브로커 호스트명 (기본값: sf_mosquitto)
 *               자동으로 "tcp://<host>:1883" URI 조합
 *
 * ▶ 내부 구조
 *   - CallbackHandler (내부 클래스): mqtt::callback 인터페이스를 구현하여
 *     메시지 수신/연결 끊김/발행 완료 이벤트를 처리합니다.
 *   - callbacks 맵: 구독 토픽 → 콜백 함수 매핑을 저장합니다.
 *
 * ▶ QoS 정책
 *   - publish: QoS 1 (at least once) — 10초 타임아웃
 *   - subscribe: QoS 1
 *
 * ▶ 커스터마이징 가이드
 *   1. TLS/SSL 암호화 통신: mqtt::connect_options에 ssl_options 추가 후 "ssl://" URI 사용
 *   2. 사용자 인증(username/password): connOpts.set_user_name() / set_password()
 *   3. # 와일드카드 지원: message_arrived()의 패턴 매칭 로직 확장 필요
 *   4. 자동 재연결: connOpts.set_automatic_reconnect(true) 설정 및 connection_lost 콜백에서 재구독 처리
 */
#pragma once
#include <string>
#include <functional>
#include <mqtt/async_client.h>
#include <map>
#include <mutex>

namespace Core {

/**
 * @class MqttClient
 * @brief Paho MQTT 비동기 클라이언트 싱글톤 래퍼
 *
 * 메시지 수신은 CallbackHandler(내부 클래스)에서 비동기적으로 처리됩니다.
 * 등록된 콜백은 구독 토픽 패턴과 대조하여 일치하는 함수를 호출합니다.
 */
class MqttClient {
public:
    /**
     * @brief 싱글톤 인스턴스를 반환합니다.
     * @return MqttClient& 전역 유일 인스턴스 참조
     */
    static MqttClient& getInstance();

    /**
     * @brief MQTT 브로커에 연결합니다.
     * @return true  연결 성공, false 연결 실패
     * @note keep-alive 간격: 20초, clean session: true
     */
    bool connect();

    /**
     * @brief MQTT 브로커 연결을 종료합니다.
     */
    void disconnect();

    /**
     * @brief 지정 토픽으로 메시지를 발행합니다.
     * @param topic   발행할 MQTT 토픽 문자열 (예: "smartfarm/actuators/FAN_01/command")
     * @param payload 발행할 메시지 페이로드 (예: "ON", "OFF")
     * @note QoS 1, 최대 10초 대기 후 타임아웃
     */
    void publish(const std::string& topic, const std::string& payload);

    /**
     * @brief 토픽을 구독하고 메시지 수신 시 호출할 콜백을 등록합니다.
     * @param topic    구독할 MQTT 토픽 (+ 와일드카드 지원, 예: "smartfarm/sensors/+/value")
     * @param callback 메시지 수신 시 실행할 함수: void(const string& topic, const string& payload)
     * @note QoS 1로 구독합니다.
     *
     * @example
     * @code
     * MqttClient::getInstance().subscribe(
     *     "smartfarm/sensors/+/value",
     *     [](const std::string& topic, const std::string& payload) {
     *         // 센서 데이터 처리 로직
     *     }
     * );
     * @endcode
     */
    void subscribe(const std::string& topic, std::function<void(const std::string&, const std::string&)> callback);

private:
    MqttClient();
    ~MqttClient();
    MqttClient(const MqttClient&) = delete;
    MqttClient& operator=(const MqttClient&) = delete;

    std::string serverURI; ///< MQTT 브로커 URI (예: "tcp://sf_mosquitto:1883")
    std::string clientId;  ///< 클라이언트 ID (충돌 방지를 위해 랜덤 접미사 추가)
    mqtt::async_client* client; ///< Paho C++ 비동기 클라이언트 인스턴스

    /// 구독 토픽 → 콜백 함수 매핑 (+ 와일드카드 패턴 포함)
    std::map<std::string, std::function<void(const std::string&, const std::string&)>> callbacks;
    std::mutex cbMutex; ///< callbacks 맵 동시 접근 보호

    /**
     * @class CallbackHandler
     * @brief mqtt::callback 인터페이스 구현 — MQTT 이벤트 처리기
     *
     * Paho C++ 라이브러리의 이벤트 콜백을 MqttClient의 내부 로직과 연결합니다.
     * message_arrived()에서 수신된 토픽을 등록된 callbacks 맵과 대조합니다.
     */
    class CallbackHandler : public virtual mqtt::callback {
        MqttClient& parent; ///< 외부 MqttClient 인스턴스 참조
    public:
        explicit CallbackHandler(MqttClient& p) : parent(p) {}

        /**
         * @brief 브로커와 연결이 끊겼을 때 호출됩니다.
         * @param cause 연결 끊김 원인 메시지
         * @note 현재 로그만 출력합니다. 자동 재연결이 필요하면 이 함수에서 connect()를 호출하세요.
         */
        void connection_lost(const std::string& cause) override;

        /**
         * @brief 구독 토픽으로 메시지가 도착했을 때 호출됩니다.
         * @param msg 수신된 MQTT 메시지 (토픽 + 페이로드 포함)
         * @note 등록된 callbacks 맵에서 + 와일드카드 패턴 매칭으로 해당 콜백을 찾아 실행합니다.
         */
        void message_arrived(mqtt::const_message_ptr msg) override;

        /**
         * @brief 발행한 메시지의 전송이 완료되었을 때 호출됩니다.
         * @param token 발행 토큰 (QoS 확인에 사용)
         * @note 현재 미구현(빈 함수). QoS 2 확인 등이 필요하면 구현하세요.
         */
        void delivery_complete(mqtt::delivery_token_ptr token) override;
    };

    CallbackHandler cbHandler; ///< Paho C++ 라이브러리에 등록할 이벤트 핸들러 인스턴스
};

} // namespace Core
