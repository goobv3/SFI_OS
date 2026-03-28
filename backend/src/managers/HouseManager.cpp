#include "HouseManager.h"
#include "../core/Database.h"

namespace Managers {

HouseManager& HouseManager::getInstance() {
    static HouseManager instance;
    return instance;
}

nlohmann::json HouseManager::getHouses() {
    auto& db = Core::Database::getInstance();
    auto results = db.fetchAll("SELECT house_id, name, display_order FROM houses ORDER BY display_order");
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& row : results) {
        nlohmann::json obj;
        obj["house_id"] = row.at("house_id");
        obj["name"] = row.at("name");
        obj["display_order"] = std::stoi(row.at("display_order"));
        
        // 동별 알람/기기 개수 등의 메타데이터 추가 가능
        std::string sensorQuery = "SELECT COUNT(*) as c FROM sensor_metadata WHERE house_id='" + obj["house_id"].get<std::string>() + "'";
        auto sCount = db.fetchAll(sensorQuery);
        obj["sensor_count"] = sCount.empty() ? 0 : std::stoi(sCount[0].at("c"));

        std::string actuatorQuery = "SELECT COUNT(*) as c FROM actuator_metadata WHERE house_id='" + obj["house_id"].get<std::string>() + "'";
        auto aCount = db.fetchAll(actuatorQuery);
        obj["actuator_count"] = aCount.empty() ? 0 : std::stoi(aCount[0].at("c"));

        arr.push_back(obj);
    }
    return arr;
}

nlohmann::json HouseManager::getHouseDevices(const std::string& house_id) {
    auto& db = Core::Database::getInstance();
    nlohmann::json result;
    
    auto sResults = db.fetchAll("SELECT * FROM sensor_metadata WHERE house_id='" + house_id + "' ORDER BY display_order");
    nlohmann::json sArr = nlohmann::json::array();
    for (const auto& row : sResults) {
        nlohmann::json obj;
        for (const auto& pair : row) obj[pair.first] = pair.second;
        sArr.push_back(obj);
    }
    result["sensors"] = sArr;

    auto aResults = db.fetchAll("SELECT * FROM actuator_metadata WHERE house_id='" + house_id + "'");
    nlohmann::json aArr = nlohmann::json::array();
    for (const auto& row : aResults) {
        nlohmann::json obj;
        for (const auto& pair : row) obj[pair.first] = pair.second;
        aArr.push_back(obj);
    }
    result["actuators"] = aArr;

    return result;
}

nlohmann::json HouseManager::getDiscoveredDevices() {
    auto& db = Core::Database::getInstance();
    auto results = db.fetchAll("SELECT device_id, device_type, first_seen, last_seen, payload FROM discovered_devices ORDER BY last_seen DESC");
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& row : results) {
        nlohmann::json obj;
        for (const auto& pair : row) obj[pair.first] = pair.second;
        arr.push_back(obj);
    }
    return arr;
}

// ── 하우스 CRUD ──

bool HouseManager::createHouse(const std::string& house_id, const std::string& name) {
    auto& db = Core::Database::getInstance();
    auto r = db.fetchAll("SELECT COALESCE(MAX(display_order),0)+1 as next_order FROM houses");
    int next_order = r.empty() ? 1 : std::stoi(r[0].at("next_order"));
    std::string sql = "INSERT IGNORE INTO houses (house_id, name, display_order) VALUES ('"
        + house_id + "','" + name + "'," + std::to_string(next_order) + ")";
    return db.execute(sql);
}

bool HouseManager::updateHouse(const std::string& house_id, const std::string& name, int display_order) {
    auto& db = Core::Database::getInstance();
    std::string sql = "UPDATE houses SET name='" + name + "', display_order=" + std::to_string(display_order) +
                      " WHERE house_id='" + house_id + "'";
    return db.execute(sql);
}

bool HouseManager::deleteHouse(const std::string& house_id) {
    auto& db = Core::Database::getInstance();
    db.execute("DELETE FROM sensor_metadata WHERE house_id='" + house_id + "'");
    db.execute("DELETE FROM actuator_metadata WHERE house_id='" + house_id + "'");
    return db.execute("DELETE FROM houses WHERE house_id='" + house_id + "'");
}

bool HouseManager::updateHousesOrder(const nlohmann::json& ordered_ids) {
    auto& db = Core::Database::getInstance();
    int order = 0;
    for (const auto& id : ordered_ids) {
        std::string sql = "UPDATE houses SET display_order=" + std::to_string(order)
            + " WHERE house_id='" + id.get<std::string>() + "'";
        if (!db.execute(sql)) return false;
        ++order;
    }
    return true;
}

// ── 센서 메타데이터 CRUD ──

bool HouseManager::createSensor(const nlohmann::json& body) {
    auto& db = Core::Database::getInstance();
    std::string sensor_id = body.value("sensor_id", "");
    std::string house_id  = body.value("house_id", "");
    std::string alias     = body.value("alias", sensor_id);
    std::string type      = body.value("type", "temperature");
    std::string unit      = body.value("unit", "");
    int display_order     = body.value("display_order", 0);
    if (sensor_id.empty() || house_id.empty()) return false;
    std::string sql = "INSERT IGNORE INTO sensor_metadata "
        "(sensor_id, house_id, alias, type, unit, display_order) VALUES ('"
        + sensor_id + "','" + house_id + "','" + alias + "','" + type + "','" + unit
        + "'," + std::to_string(display_order) + ")";
    return db.execute(sql);
}

bool HouseManager::updateSensor(const std::string& sensor_id, const nlohmann::json& body) {
    auto& db = Core::Database::getInstance();
    // NULL / 숫자 / 문자열을 SQL 값으로 변환하는 헬퍼
    auto toSqlVal = [](const nlohmann::json& v) -> std::string {
        if (v.is_null()) return "NULL";
        if (v.is_number()) return std::to_string(v.get<double>());
        if (v.is_string() && v.get<std::string>() == "") return "NULL";
        return "'" + v.get<std::string>() + "'";
    };
    std::vector<std::string> parts;
    if (body.contains("alias"))         parts.push_back("alias='"         + body["alias"].get<std::string>()         + "'");
    if (body.contains("type"))          parts.push_back("type='"          + body["type"].get<std::string>()          + "'");
    if (body.contains("unit"))          parts.push_back("unit='"          + body["unit"].get<std::string>()          + "'");
    if (body.contains("display_order")) parts.push_back("display_order="  + std::to_string(body["display_order"].get<int>()));
    if (body.contains("is_active")) {
        bool active = false;
        if (body["is_active"].is_boolean()) active = body["is_active"].get<bool>();
        else if (body["is_active"].is_string()) active = (body["is_active"].get<std::string>() == "1" || body["is_active"].get<std::string>() == "true");
        else if (body["is_active"].is_number()) active = (body["is_active"].get<int>() != 0);
        parts.push_back(std::string("is_active=") + (active ? "1" : "0"));
    }
    if (body.contains("warn_high"))     parts.push_back("warn_high="      + toSqlVal(body["warn_high"]));
    if (body.contains("warn_low"))      parts.push_back("warn_low="       + toSqlVal(body["warn_low"]));
    if (body.contains("crit_high"))     parts.push_back("crit_high="      + toSqlVal(body["crit_high"]));
    if (body.contains("crit_low"))      parts.push_back("crit_low="       + toSqlVal(body["crit_low"]));
    if (parts.empty()) return true;
    std::string sql = "UPDATE sensor_metadata SET ";
    for (size_t i = 0; i < parts.size(); ++i) { sql += parts[i]; if (i+1 < parts.size()) sql += ","; }
    sql += " WHERE sensor_id='" + sensor_id + "'";
    return db.execute(sql);
}

bool HouseManager::deleteSensor(const std::string& sensor_id) {
    return Core::Database::getInstance().execute(
        "DELETE FROM sensor_metadata WHERE sensor_id='" + sensor_id + "'");
}

// ── 액추에이터 메타데이터 CRUD ──

bool HouseManager::createActuator(const nlohmann::json& body) {
    auto& db = Core::Database::getInstance();
    std::string actuator_id = body.value("actuator_id", "");
    std::string house_id    = body.value("house_id", "");
    std::string alias       = body.value("alias", actuator_id);
    std::string type        = body.value("type", "GENERIC");
    if (actuator_id.empty() || house_id.empty()) return false;
    std::string sql = "INSERT IGNORE INTO actuator_metadata "
        "(actuator_id, house_id, alias, type) VALUES ('"
        + actuator_id + "','" + house_id + "','" + alias + "','" + type + "')";
    return db.execute(sql);
}

bool HouseManager::updateActuator(const std::string& actuator_id, const nlohmann::json& body) {
    auto& db = Core::Database::getInstance();
    std::vector<std::string> parts;
    if (body.contains("alias")) parts.push_back("alias='" + body["alias"].get<std::string>() + "'");
    if (body.contains("type"))  parts.push_back("type='"  + body["type"].get<std::string>()  + "'");
    if (parts.empty()) return true;
    std::string sql = "UPDATE actuator_metadata SET ";
    for (size_t i = 0; i < parts.size(); ++i) { sql += parts[i]; if (i+1 < parts.size()) sql += ","; }
    sql += " WHERE actuator_id='" + actuator_id + "'";
    return db.execute(sql);
}

bool HouseManager::deleteActuator(const std::string& actuator_id) {
    return Core::Database::getInstance().execute(
        "DELETE FROM actuator_metadata WHERE actuator_id='" + actuator_id + "'");
}

// ── 감지기기 삭제 ──

bool HouseManager::deleteDiscoveredDevice(const std::string& device_id) {
    return Core::Database::getInstance().execute(
        "DELETE FROM discovered_devices WHERE device_id='" + device_id + "'");
}

} // namespace Managers
