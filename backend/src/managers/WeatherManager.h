#pragma once
#include <nlohmann/json.hpp>

namespace Managers {
class WeatherManager {
public:
    static WeatherManager& getInstance();
    nlohmann::json getLatestWeather(int hours_ahead);
    
    /**
     * @brief 농장 내 로컬 기상대 데이터를 DB에 기록합니다.
     * @param data JSON 형식의 날씨 데이터 (wind_speed, temperature 등 포함)
     */
    void recordFarmWeather(const nlohmann::json& data);
private:
    WeatherManager() = default;
};
}
