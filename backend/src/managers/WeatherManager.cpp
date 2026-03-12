#include "WeatherManager.h"
#include "../core/Database.h"
#include <string>

namespace Managers {
WeatherManager& WeatherManager::getInstance() {
    static WeatherManager instance;
    return instance;
}

nlohmann::json WeatherManager::getLatestWeather(int hours_ahead) {
    auto& db = Core::Database::getInstance();
    nlohmann::json res;
    
    std::string qFarm = "SELECT timestamp, wind_speed, wind_direction, rainfall, solar_radiation, temperature, humidity FROM weather_data WHERE source='FARM' AND forecast_offset=0 ORDER BY timestamp DESC LIMIT 1";
    auto fRes = db.fetchAll(qFarm);
    if (!fRes.empty()) {
        nlohmann::json fObj;
        for (const auto& p : fRes[0]) fObj[p.first] = (p.first == "timestamp" || p.first == "wind_direction") ? nlohmann::json(p.second) : (p.second.empty() ? nlohmann::json() : nlohmann::json(std::stod(p.second)));
        res["FARM"] = fObj;
    } else res["FARM"] = nullptr;
    
    std::string qKma;
    if (hours_ahead > 0) {
        qKma = "SELECT timestamp, wind_speed, wind_direction, rainfall, solar_radiation, temperature, humidity FROM weather_data WHERE source='KMA' AND forecast_offset=" + std::to_string(hours_ahead) + " ORDER BY id DESC LIMIT 1";
    } else {
        qKma = "SELECT timestamp, wind_speed, wind_direction, rainfall, solar_radiation, temperature, humidity FROM weather_data WHERE source='KMA' AND forecast_offset=0 ORDER BY timestamp DESC LIMIT 1";
    }
    auto kRes = db.fetchAll(qKma);
    if (!kRes.empty()) {
        nlohmann::json kObj;
        for (const auto& p : kRes[0]) kObj[p.first] = (p.first == "timestamp" || p.first == "wind_direction") ? nlohmann::json(p.second) : (p.second.empty() ? nlohmann::json() : nlohmann::json(std::stod(p.second)));
        res["KMA"] = kObj;
    } else res["KMA"] = nullptr;
    
    return res;
}
}
