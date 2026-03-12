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
    std::string metaQuery = "SELECT house_id FROM actuator_metadata WHERE actuator_id='" + actuator_id + "'";
    auto res = db.fetchAll(metaQuery);
    if (res.empty()) return false;
    
    std::string logQuery = "INSERT INTO control_logs (actuator_id, command, user_id, status) VALUES ('" + actuator_id + "', '" + command + "', '" + user_id + "', 'SUCCESS')";
    db.execute(logQuery);
    
    std::string topic = "smartfarm/actuators/" + actuator_id + "/command";
    Core::MqttClient::getInstance().publish(topic, command);
    
    return true;
}
}
