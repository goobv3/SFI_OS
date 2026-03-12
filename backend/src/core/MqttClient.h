#pragma once
#include <string>
#include <functional>
#include <mqtt/async_client.h>
#include <map>
#include <mutex>

namespace Core {
class MqttClient {
public:
    static MqttClient& getInstance();
    bool connect();
    void disconnect();
    void publish(const std::string& topic, const std::string& payload);
    void subscribe(const std::string& topic, std::function<void(const std::string&, const std::string&)> callback);

private:
    MqttClient();
    ~MqttClient();
    MqttClient(const MqttClient&) = delete;
    MqttClient& operator=(const MqttClient&) = delete;

    std::string serverURI;
    std::string clientId;
    mqtt::async_client* client;
    
    std::map<std::string, std::function<void(const std::string&, const std::string&)>> callbacks;
    std::mutex cbMutex;

    class CallbackHandler : public virtual mqtt::callback {
        MqttClient& parent;
    public:
        CallbackHandler(MqttClient& p) : parent(p) {}
        void connection_lost(const std::string& cause) override;
        void message_arrived(mqtt::const_message_ptr msg) override;
        void delivery_complete(mqtt::delivery_token_ptr token) override;
    };
    CallbackHandler cbHandler;
};
}
