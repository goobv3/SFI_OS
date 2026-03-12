#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace Managers {
class SensorManager {
public:
    static SensorManager& getInstance();
    void processIncomingData(const std::string& sensor_id, double value);
    nlohmann::json getAlarms();
    bool resolveAlarm(int alarm_id);
private:
    SensorManager() = default;
    ~SensorManager() = default;
    SensorManager(const SensorManager&) = delete;
    SensorManager& operator=(const SensorManager&) = delete;
    
    void checkAlarms(const std::string& house_id, const std::string& sensor_id, double value);
};
}
