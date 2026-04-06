#include "ControlManager.h"
#include "../core/Database.h"
#include "../core/MqttClient.h"
#include <iostream>

namespace Managers {

ControlManager& ControlManager::getInstance() {
    static ControlManager instance;
    return instance;
}

bool ControlManager::processControlCommand(const std::string& actuator_id, const std::string& command, const std::string& user_id) {
    auto& db = Core::Database::getInstance();

    // actuator_id 가 외부 입력 → PS 사용
    auto res = db.fetchAllPS(
        "SELECT house_id FROM actuator_metadata WHERE actuator_id=?",
        {actuator_id});
    if (res.empty()) return false;

    db.executePS(
        "INSERT INTO control_logs (actuator_id, command, user_id, status) VALUES (?,?,?,'SUCCESS')",
        {actuator_id, command, user_id});

    std::string topic = "smartfarm/actuators/" + actuator_id + "/command";
    Core::MqttClient::getInstance().publish(topic, command);

    return true;
}
}
