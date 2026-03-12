#include "SensorManager.h"
#include "../core/Database.h"
#include <iostream>

namespace Managers {

SensorManager& SensorManager::getInstance() {
    static SensorManager instance;
    return instance;
}

void SensorManager::processIncomingData(const std::string& sensor_id, double value) {
    auto& db = Core::Database::getInstance();
    
    std::string metaQuery = "SELECT house_id FROM sensor_metadata WHERE sensor_id='" + sensor_id + "'";
    auto res = db.fetchAll(metaQuery);
    
    if (res.empty()) {
        std::string upsertQuery = "INSERT INTO discovered_devices (device_id, device_type, payload) "
                                  "VALUES ('" + sensor_id + "', 'sensor', '" + std::to_string(value) + "') "
                                  "ON DUPLICATE KEY UPDATE last_seen=CURRENT_TIMESTAMP, payload=VALUES(payload)";
        db.execute(upsertQuery);
        return;
    }
    
    std::string house_id = res[0]["house_id"];
    
    std::string insertData = "INSERT INTO sensor_data (sensor_id, house_id, value) VALUES ('" + 
                             sensor_id + "', '" + house_id + "', " + std::to_string(value) + ")";
    db.execute(insertData);
    
    checkAlarms(house_id, sensor_id, value);
}

void SensorManager::checkAlarms(const std::string& house_id, const std::string& sensor_id, double value) {
    auto& db = Core::Database::getInstance();
    std::string q = "SELECT warn_high, warn_low, crit_high, crit_low FROM sensor_metadata WHERE sensor_id='" + sensor_id + "' AND is_active=1";
    auto limits = db.fetchAll(q);
    if (limits.empty()) return;
    
    auto row = limits[0];
    std::string type = "";
    std::string msg = "";
    
    try {
        if (!row["crit_high"].empty() && value >= std::stod(row["crit_high"])) { type = "CRITICAL_HIGH"; msg = "Critical High limit exceeded"; }
        else if (!row["warn_high"].empty() && value >= std::stod(row["warn_high"])) { type = "WARNING_HIGH"; msg = "Warning High limit exceeded"; }
        else if (!row["crit_low"].empty() && value <= std::stod(row["crit_low"])) { type = "CRITICAL_LOW"; msg = "Critical Low limit exceeded"; }
        else if (!row["warn_low"].empty() && value <= std::stod(row["warn_low"])) { type = "WARNING_LOW"; msg = "Warning Low limit exceeded"; }
    } catch (...) { return; }
    
    if (!type.empty()) {
        std::string alarmQ = "INSERT INTO alarms (house_id, sensor_id, alarm_type, value, message) VALUES ('" + 
                             house_id + "', '" + sensor_id + "', '" + type + "', " + std::to_string(value) + ", '" + msg + "')";
        db.execute(alarmQ);
    }
}

nlohmann::json SensorManager::getAlarms() {
    auto& db = Core::Database::getInstance();
    auto results = db.fetchAll("SELECT id, house_id, sensor_id, alarm_type, value, message, created_at FROM alarms WHERE is_acknowledged=0 ORDER BY created_at DESC");
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& row : results) {
        nlohmann::json obj;
        for(const auto& p : row) obj[p.first] = p.second;
        arr.push_back(obj);
    }
    return arr;
}

bool SensorManager::resolveAlarm(int alarm_id) {
    auto& db = Core::Database::getInstance();
    return db.execute("UPDATE alarms SET is_acknowledged=1 WHERE id=" + std::to_string(alarm_id));
}

}
