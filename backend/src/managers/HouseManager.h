#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace Managers {
class HouseManager {
public:
    static HouseManager& getInstance();
    nlohmann::json getHouses();
    nlohmann::json getHouseDevices(const std::string& house_id);
    nlohmann::json getDiscoveredDevices();
private:
    HouseManager() = default;
    ~HouseManager() = default;
    HouseManager(const HouseManager&) = delete;
    HouseManager& operator=(const HouseManager&) = delete;
};
}
