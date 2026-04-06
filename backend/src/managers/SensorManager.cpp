#include "SensorManager.h"
#include "RuleEngine.h"
#include "../core/Database.h"
#include "../core/EmailSender.h"
#include <iostream>

namespace Managers {

SensorManager& SensorManager::getInstance() {
    static SensorManager instance;
    return instance;
}

void SensorManager::processIncomingData(const std::string& sensor_id, double value) {
    auto& db = Core::Database::getInstance();

    // sensor_id 가 외부 입력(MQTT 토픽 등) → PS 사용
    auto res = db.fetchAllPS(
        "SELECT house_id FROM sensor_metadata WHERE sensor_id=?",
        {sensor_id});

    if (res.empty()) {
        db.executePS(
            "INSERT INTO discovered_devices (device_id, device_type, payload) VALUES (?,?,'') "
            "ON DUPLICATE KEY UPDATE last_seen=CURRENT_TIMESTAMP, payload=VALUES(payload)",
            {sensor_id, "sensor"});
        // ※ payload(value)는 double이므로 별도 UPDATE로 안전하게 삽입
        db.executePS(
            "UPDATE discovered_devices SET payload=? WHERE device_id=?",
            {std::to_string(value), sensor_id});
        return;
    }

    std::string house_id = res[0]["house_id"];

    db.executePS(
        "INSERT INTO sensors (sensor_id, house_id, value) VALUES (?,?,?)",
        {sensor_id, house_id, std::to_string(value)});

    checkAlarms(house_id, sensor_id, value);
    Managers::RuleEngine::getInstance().evaluate(sensor_id, value);
}

void SensorManager::checkAlarms(const std::string& house_id, const std::string& sensor_id, double value) {
    auto& db = Core::Database::getInstance();

    // sensor_id 가 외부 입력 → PS 사용
    auto limits = db.fetchAllPS(
        "SELECT warn_high, warn_low, crit_high, crit_low, alias FROM sensor_metadata WHERE sensor_id=? AND is_active=1",
        {sensor_id});
    if (limits.empty()) return;

    auto row = limits[0];
    std::string type = "";
    std::string msg  = "";
    std::string alias = row["alias"].empty() ? sensor_id : row["alias"];

    try {
        if (!row["crit_high"].empty() && value >= std::stod(row["crit_high"])) { type = "CRITICAL_HIGH"; msg = "Critical High limit exceeded"; }
        else if (!row["warn_high"].empty() && value >= std::stod(row["warn_high"])) { type = "WARNING_HIGH"; msg = "Warning High limit exceeded"; }
        else if (!row["crit_low"].empty() && value <= std::stod(row["crit_low"]))  { type = "CRITICAL_LOW"; msg = "Critical Low limit exceeded"; }
        else if (!row["warn_low"].empty() && value <= std::stod(row["warn_low"]))  { type = "WARNING_LOW";  msg = "Warning Low limit exceeded"; }
    } catch (...) { return; }

    if (!type.empty()) {
        db.executePS(
            "INSERT INTO alarms (house_id, sensor_id, alarm_type, value, message) VALUES (?,?,?,?,?)",
            {house_id, sensor_id, type, std::to_string(value), msg});
            
        // CRITICAL 알람인 경우 이메일 전송
        if (type.find("CRITICAL") != std::string::npos) {
            Core::EmailSender::sendAlert(alias, type, value, msg);
        }
    }
}

nlohmann::json SensorManager::getAlarms() {
    auto& db = Core::Database::getInstance();
    // 외부 입력 없는 상수 쿼리 → fetchAll 유지
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
    // alarm_id 는 int 형이나, 외부 API에서 받은 값 → PS 처리
    return db.executePS(
        "UPDATE alarms SET is_acknowledged=1 WHERE id=?",
        {std::to_string(alarm_id)});
}

}
