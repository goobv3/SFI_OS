/**
 * @file HouseManager.cpp
 * @brief HouseManager 클래스 구현
 *
 * ▶ 데이터 조회 방식
 *   - getHouses()는 하우스 단위 정보와 함께 센서/액추에이터 갯수를 동적으로 카운트하여 반환
 *   - getHouseDevices()는 센서, 액추에이터 메타데이터를 분리하여 JSON 객체 내 배열 형태로 반환
 */
#include "HouseManager.h"
#include "../core/Database.h"

namespace Managers {

HouseManager& HouseManager::getInstance() {
    static HouseManager instance;
    return instance;
}

// ─────────────────────────────────────────────────────────────────────────────
// getHouses: 하우스 목록 조회 + 하우스별 센서/액추에이터 계수 반환
// ─────────────────────────────────────────────────────────────────────────────
nlohmann::json HouseManager::getHouses() {
    auto& db = Core::Database::getInstance();
    // 외부 입력 없는 상수 쿼리 → fetchAll 유지
    auto results = db.fetchAll("SELECT house_id, name, display_order FROM houses ORDER BY display_order");
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& row : results) {
        nlohmann::json obj;
        obj["house_id"] = row.at("house_id");
        obj["name"] = row.at("name");
        obj["display_order"] = std::stoi(row.at("display_order"));

        // house_id 가 외부 입력이므로 PS 사용
        std::string hid = obj["house_id"].get<std::string>();
        auto sCount = db.fetchAllPS(
            "SELECT COUNT(*) as c FROM sensor_metadata WHERE house_id=?",
            {hid});
        obj["sensor_count"] = sCount.empty() ? 0 : std::stoi(sCount[0].at("c"));

        auto aCount = db.fetchAllPS(
            "SELECT COUNT(*) as c FROM actuator_metadata WHERE house_id=?",
            {hid});
        obj["actuator_count"] = aCount.empty() ? 0 : std::stoi(aCount[0].at("c"));

        arr.push_back(obj);
    }
    return arr;
}

// ─────────────────────────────────────────────────────────────────────────────
// getHouseDevices: 특정 하우스에 종속된 모든 센서와 액추에이터 메타데이터 조회
// ─────────────────────────────────────────────────────────────────────────────
nlohmann::json HouseManager::getHouseDevices(const std::string& house_id) {
    auto& db = Core::Database::getInstance();
    nlohmann::json result;

    auto sResults = db.fetchAllPS(
        "SELECT * FROM sensor_metadata WHERE house_id=? ORDER BY display_order",
        {house_id});
    nlohmann::json sArr = nlohmann::json::array();
    for (const auto& row : sResults) {
        nlohmann::json obj;
        for (const auto& pair : row) obj[pair.first] = pair.second;
        sArr.push_back(obj);
    }
    result["sensors"] = sArr;

    auto aResults = db.fetchAllPS(
        "SELECT * FROM actuator_metadata WHERE house_id=?",
        {house_id});
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
    // 외부 입력 없는 상수 쿼리 → fetchAll 유지
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

// ─────────────────────────────────────────────────────────────────────────────
// createHouse: 하우스 생성 시 display_order를 기존 최대값 + 1 로 자동 계산
// ─────────────────────────────────────────────────────────────────────────────
bool HouseManager::createHouse(const std::string& house_id, const std::string& name) {
    auto& db = Core::Database::getInstance();
    // COALESCE 는 외부 입력 없음 → fetchAll 유지
    auto r = db.fetchAll("SELECT COALESCE(MAX(display_order),0)+1 as next_order FROM houses");
    int next_order = r.empty() ? 1 : std::stoi(r[0].at("next_order"));
    return db.executePS(
        "INSERT IGNORE INTO houses (house_id, name, display_order) VALUES (?,?,?)",
        {house_id, name, std::to_string(next_order)});
}

bool HouseManager::updateHouse(const std::string& house_id, const std::string& name, int display_order) {
    auto& db = Core::Database::getInstance();
    return db.executePS(
        "UPDATE houses SET name=?, display_order=? WHERE house_id=?",
        {name, std::to_string(display_order), house_id});
}

bool HouseManager::deleteHouse(const std::string& house_id) {
    auto& db = Core::Database::getInstance();
    db.executePS("DELETE FROM sensor_metadata WHERE house_id=?",   {house_id});
    db.executePS("DELETE FROM actuator_metadata WHERE house_id=?", {house_id});
    return db.executePS("DELETE FROM houses WHERE house_id=?",     {house_id});
}

bool HouseManager::updateHousesOrder(const nlohmann::json& ordered_ids) {
    auto& db = Core::Database::getInstance();
    int order = 0;
    for (const auto& id : ordered_ids) {
        if (!db.executePS(
                "UPDATE houses SET display_order=? WHERE house_id=?",
                {std::to_string(order), id.get<std::string>()}))
            return false;
        ++order;
    }
    return true;
}

// ── 센서 메타데이터 CRUD ──

// ─────────────────────────────────────────────────────────────────────────────
// createSensor: 새 센서 메타데이터 정보를 DB에 저장
// ─────────────────────────────────────────────────────────────────────────────
bool HouseManager::createSensor(const nlohmann::json& body) {
    auto& db = Core::Database::getInstance();
    std::string sensor_id = body.value("sensor_id", "");
    std::string house_id  = body.value("house_id", "");
    std::string alias     = body.value("alias", sensor_id);
    std::string type      = body.value("type", "temperature");
    std::string unit      = body.value("unit", "");
    int display_order     = body.value("display_order", 0);
    if (sensor_id.empty() || house_id.empty()) return false;
    return db.executePS(
        "INSERT IGNORE INTO sensor_metadata "
        "(sensor_id, house_id, alias, type, unit, display_order) VALUES (?,?,?,?,?,?)",
        {sensor_id, house_id, alias, type, unit, std::to_string(display_order)});
}

// ─────────────────────────────────────────────────────────────────────────────
// updateSensor: 변경된 필드만 동적으로 UPDATE 쿼리를 생성하여 반영
// ─────────────────────────────────────────────────────────────────────────────
bool HouseManager::updateSensor(const std::string& sensor_id, const nlohmann::json& body) {
    auto& db = Core::Database::getInstance();
    // NULL / 숫자 / 문자열을 SQL 값으로 변환하는 헬퍼 (수치 컬럼은 PS 바인딩 불가 이슈 없음)
    auto toSqlVal = [](const nlohmann::json& v) -> std::string {
        if (v.is_null()) return "NULL";
        if (v.is_number()) return std::to_string(v.get<double>());
        if (v.is_string() && v.get<std::string>() == "") return "NULL";
        return "'" + v.get<std::string>() + "'";
    };
    // SET 절은 기존 방식 유지(동적 컬럼 목록), WHERE의 sensor_id 만 PS 바인딩
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
    sql += " WHERE sensor_id=?";
    return db.executePS(sql, {sensor_id});
}

bool HouseManager::deleteSensor(const std::string& sensor_id) {
    return Core::Database::getInstance().executePS(
        "DELETE FROM sensor_metadata WHERE sensor_id=?", {sensor_id});
}

// ── 액추에이터 메타데이터 CRUD ──

bool HouseManager::createActuator(const nlohmann::json& body) {
    auto& db = Core::Database::getInstance();
    std::string actuator_id = body.value("actuator_id", "");
    std::string house_id    = body.value("house_id", "");
    std::string alias       = body.value("alias", actuator_id);
    std::string type        = body.value("type", "GENERIC");
    if (actuator_id.empty() || house_id.empty()) return false;
    return db.executePS(
        "INSERT IGNORE INTO actuator_metadata (actuator_id, house_id, alias, type) VALUES (?,?,?,?)",
        {actuator_id, house_id, alias, type});
}

bool HouseManager::updateActuator(const std::string& actuator_id, const nlohmann::json& body) {
    auto& db = Core::Database::getInstance();
    std::vector<std::string> parts;
    if (body.contains("alias")) parts.push_back("alias='" + body["alias"].get<std::string>() + "'");
    if (body.contains("type"))  parts.push_back("type='"  + body["type"].get<std::string>()  + "'");
    if (parts.empty()) return true;
    std::string sql = "UPDATE actuator_metadata SET ";
    for (size_t i = 0; i < parts.size(); ++i) { sql += parts[i]; if (i+1 < parts.size()) sql += ","; }
    sql += " WHERE actuator_id=?";
    return db.executePS(sql, {actuator_id});
}

bool HouseManager::deleteActuator(const std::string& actuator_id) {
    return Core::Database::getInstance().executePS(
        "DELETE FROM actuator_metadata WHERE actuator_id=?", {actuator_id});
}

// ── 감지기기 삭제 ──

bool HouseManager::deleteDiscoveredDevice(const std::string& device_id) {
    return Core::Database::getInstance().executePS(
        "DELETE FROM discovered_devices WHERE device_id=?", {device_id});
}

} // namespace Managers
