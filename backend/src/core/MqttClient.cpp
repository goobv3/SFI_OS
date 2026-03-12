#include "MqttClient.h"
#include <iostream>
#include <cstdlib>

namespace Core {

MqttClient& MqttClient::getInstance() {
    static MqttClient instance;
    return instance;
}

MqttClient::MqttClient() : cbHandler(*this) {
    const char* env_host = std::getenv("MQTT_HOST");
    serverURI = env_host ? std::string("tcp://") + env_host + ":1883" : "tcp://sf_mosquitto:1883";
    clientId = "smartfarm_cpp_backend_" + std::to_string(std::rand());
    client = new mqtt::async_client(serverURI, clientId);
    client->set_callback(cbHandler);
}

MqttClient::~MqttClient() {
    disconnect();
    delete client;
}

bool MqttClient::connect() {
    try {
        mqtt::connect_options connOpts;
        connOpts.set_keep_alive_interval(20);
        connOpts.set_clean_session(true);
        client->connect(connOpts)->wait();
        std::cout << "[MQTT] Connected to " << serverURI << std::endl;
        return true;
    } catch (const mqtt::exception& exc) {
        std::cerr << "[MQTT Error] " << exc.what() << std::endl;
        return false;
    }
}

void MqttClient::disconnect() {
    try {
        if (client->is_connected()) {
            client->disconnect()->wait();
            std::cout << "[MQTT] Disconnected." << std::endl;
        }
    } catch (...) {}
}

void MqttClient::publish(const std::string& topic, const std::string& payload) {
    if (!client->is_connected()) return;
    try {
        auto msg = mqtt::make_message(topic, payload);
        msg->set_qos(1);
        client->publish(msg)->wait_for(10000);
    } catch (const mqtt::exception& exc) {
        std::cerr << "[MQTT Publish Error] " << exc.what() << std::endl;
    }
}

void MqttClient::subscribe(const std::string& topic, std::function<void(const std::string&, const std::string&)> callback) {
    if (!client->is_connected()) return;
    try {
        {
            std::lock_guard<std::mutex> lock(cbMutex);
            callbacks[topic] = callback;
        }
        client->subscribe(topic, 1)->wait();
        std::cout << "[MQTT] Subscribed to " << topic << std::endl;
    } catch (const mqtt::exception& exc) {
        std::cerr << "[MQTT Subscribe Error] " << exc.what() << std::endl;
    }
}

void MqttClient::CallbackHandler::connection_lost(const std::string& cause) {
    std::cout << "\n[MQTT Connection Lost] " << cause << std::endl;
}

void MqttClient::CallbackHandler::message_arrived(mqtt::const_message_ptr msg) {
    std::string topic = msg->get_topic();
    std::string payload = msg->to_string();
    
    std::lock_guard<std::mutex> lock(parent.cbMutex);
    
    // 단순 패턴 매칭 (실제 구현에서는 + 와 # 와일드카드 처리가 필요합니다)
    for (const auto& pair : parent.callbacks) {
        std::string subTopic = pair.first;
        size_t plusPos = subTopic.find('+');
        if (plusPos != std::string::npos) {
            std::string prefix = subTopic.substr(0, plusPos);
            std::string suffix = subTopic.substr(plusPos + 1);
            if (topic.find(prefix) == 0 && topic.length() >= prefix.length() && 
                (suffix.empty() || topic.rfind(suffix) == topic.length() - suffix.length())) {
                pair.second(topic, payload);
                return;
            }
        } else if (topic == subTopic) {
            pair.second(topic, payload);
            return;
        }
    }
}

void MqttClient::CallbackHandler::delivery_complete(mqtt::delivery_token_ptr token) {}

}
