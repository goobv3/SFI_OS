#include "WeatherManager.h"
#include "../core/Database.h"
#include <string>
#include <iostream> // [추가] std::cerr 사용을 위해 필요

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

void WeatherManager::recordFarmWeather(const nlohmann::json& data) {
    auto& db = Core::Database::getInstance();
    
    // --- [로직] 로컬 기상대 데이터 저장 ---
    // MQTT를 통해 들어온 JSON 데이터를 파싱하여 weather_data 테이블에 'FARM' 소스로 저장합니다.
    try {
        std::string wind_speed = data.contains("wind_speed") ? std::to_string(data["wind_speed"].get<double>()) : "NULL";
        std::string wind_dir   = data.contains("wind_direction") ? "'" + data["wind_direction"].get<std::string>() + "'" : "NULL";
        std::string rainfall   = data.contains("rainfall") ? std::to_string(data["rainfall"].get<double>()) : "NULL";
        std::string solar      = data.contains("solar_radiation") ? std::to_string(data["solar_radiation"].get<double>()) : "NULL";
        std::string temp       = data.contains("temperature") ? std::to_string(data["temperature"].get<double>()) : "NULL";
        std::string hum        = data.contains("humidity") ? std::to_string(data["humidity"].get<double>()) : "NULL";

        std::string q = "INSERT INTO weather_data (source, forecast_offset, wind_speed, wind_direction, rainfall, solar_radiation, temperature, humidity) "
                        "VALUES ('FARM', 0, " + wind_speed + ", " + wind_dir + ", " + rainfall + ", " + solar + ", " + temp + ", " + hum + ")";
        
        db.execute(q);
    } catch (const std::exception& e) {
        std::cerr << "[WeatherManager] 로컬 기상대 데이터 처리 오류: " << e.what() << std::endl;
    }
}

}
