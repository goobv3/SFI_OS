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

}
