#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace Managers {
class HouseManager {
public:
    static HouseManager& getInstance();

    // ── 조회 ──
    nlohmann::json getHouses();
    nlohmann::json getHouseDevices(const std::string& house_id);
    nlohmann::json getDiscoveredDevices();

    // ── 하우스 CRUD ──
    bool createHouse(const std::string& house_id, const std::string& name);
    bool deleteHouse(const std::string& house_id);
    bool updateHousesOrder(const nlohmann::json& ordered_ids);

    // ── 센서 메타데이터 CRUD ──
    bool createSensor(const nlohmann::json& body);
    bool updateSensor(const std::string& sensor_id, const nlohmann::json& body);
    bool deleteSensor(const std::string& sensor_id);

    // ── 액추에이터 메타데이터 CRUD ──
    bool createActuator(const nlohmann::json& body);
    bool updateActuator(const std::string& actuator_id, const nlohmann::json& body);
    bool deleteActuator(const std::string& actuator_id);

    // ── 감지기기 삭제 ──
    bool deleteDiscoveredDevice(const std::string& device_id);

private:
    HouseManager() = default;
    ~HouseManager() = default;
    HouseManager(const HouseManager&) = delete;
    HouseManager& operator=(const HouseManager&) = delete;
};
}
