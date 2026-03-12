#pragma once
#include <nlohmann/json.hpp>

namespace Managers {
class WeatherManager {
public:
    static WeatherManager& getInstance();
    nlohmann::json getLatestWeather(int hours_ahead);
private:
    WeatherManager() = default;
};
}
