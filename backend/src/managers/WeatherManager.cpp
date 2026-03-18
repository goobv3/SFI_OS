#include "WeatherManager.h"
#include "../core/Database.h"
#include <string>
#include <iostream>
#include <cpr/cpr.h>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <vector>
#include <algorithm>

namespace Managers {
WeatherManager& WeatherManager::getInstance() {
    static WeatherManager instance;
    return instance;
}

nlohmann::json WeatherManager::getLatestWeather(int hours_ahead) {
    auto& db = Core::Database::getInstance();
    nlohmann::json res;
    
    // 1. 농장 내 실시간 데이터 (항상 offset 0)
    std::string qFarm = "SELECT timestamp, wind_speed, wind_direction, rainfall, solar_radiation, temperature, humidity FROM weather_data WHERE source='FARM' AND forecast_offset=0 ORDER BY timestamp DESC LIMIT 1";
    auto fRes = db.fetchAll(qFarm);
    if (!fRes.empty()) {
        nlohmann::json fObj;
        for (const auto& p : fRes[0]) fObj[p.first] = (p.first == "timestamp" || p.first == "wind_direction") ? nlohmann::json(p.second) : (p.second.empty() ? nlohmann::json() : nlohmann::json(std::stod(p.second)));
        res["FARM"] = fObj;
    } else res["FARM"] = nullptr;
    
    // 2. 외부 기상 데이터 (KMA)
    if (hours_ahead > 0) {
        // [변경] 예보는 DB가 아닌 기상청 API에서 직접 실시간으로 가져옵니다.
        res["KMA"] = getLiveForecast(hours_ahead);
    } else {
        // 실시간 관측은 DB에서 가장 최근 것을 가져옵니다.
        std::string qKma = "SELECT timestamp, wind_speed, wind_direction, rainfall, solar_radiation, temperature, humidity FROM weather_data WHERE source='KMA' AND forecast_offset=0 ORDER BY timestamp DESC LIMIT 1";
        auto kRes = db.fetchAll(qKma);
        if (!kRes.empty()) {
            nlohmann::json kObj;
            for (const auto& p : kRes[0]) kObj[p.first] = (p.first == "timestamp" || p.first == "wind_direction") ? nlohmann::json(p.second) : (p.second.empty() ? nlohmann::json() : nlohmann::json(std::stod(p.second)));
            res["KMA"] = kObj;
        } else res["KMA"] = nullptr;
    }
    
    return res;
}


nlohmann::json WeatherManager::getLiveForecast(int hours_ahead) {
    const char* service_key_env = std::getenv("KMA_SERVICE_KEY");
    const char* lat_env = std::getenv("LAT");
    const char* lon_env = std::getenv("LON");
    
    if (!service_key_env) return nullptr;
    
    double lat = lat_env ? std::stod(lat_env) : 37.3390;
    double lon = lon_env ? std::stod(lon_env) : 127.4907;
    auto grid = convertGrid(lat, lon);
    
    // 현재 KST 시간 계산 (UTC + 9)
    std::time_t now = std::time(nullptr);
    std::time_t kst_now = now + 9 * 3600; 
    std::tm* kst_tm = std::gmtime(&kst_now); 
    
    // 기준 시간 결정 (45분 이전이면 1시간 전 데이터)
    int base_hour = kst_tm->tm_hour;
    if (kst_tm->tm_min < 45) {
        base_hour -= 1;
    }
    
    std::tm base_tm = *kst_tm;
    base_tm.tm_hour = base_hour;
    base_tm.tm_min = 0;
    base_tm.tm_sec = 0;
    // mktime을 쓰지 않고 직접 문자열 생성 (TZ 이슈 회피)
    char dateBuf[9], timeBuf[5];
    std::strftime(dateBuf, sizeof(dateBuf), "%Y%m%d", &base_tm);
    std::strftime(timeBuf, sizeof(timeBuf), "%H00", &base_tm);
    std::string base_date_str = dateBuf;
    std::string base_time_str = timeBuf;

    std::cout << "[WeatherManager] API Request -> base_date: " << base_date_str << ", base_time: " << base_time_str << " (KST Now: " << kst_tm->tm_hour << ":" << kst_tm->tm_min << ")" << std::endl;
    
    std::string baseUrl = "http://apis.data.go.kr/1360000/VilageFcstInfoService_2.0/getUltraSrtFcst";
    auto response = cpr::Get(cpr::Url{baseUrl},
                             cpr::Parameters{
                                 {"serviceKey", service_key_env},
                                 {"numOfRows", "100"},
                                 {"pageNo", "1"},
                                 {"dataType", "JSON"},
                                 {"base_date", base_date_str},
                                 {"base_time", base_time_str},
                                 {"nx", std::to_string(grid.first)},
                                 {"ny", std::to_string(grid.second)}
                             },
                             cpr::Timeout{5000});

    if (response.status_code == 200) {
        try {
            auto data = nlohmann::json::parse(response.text);
            if (data.contains("response") && data["response"]["header"]["resultCode"] == "00") {
                auto items = data["response"]["body"]["items"]["item"];
                
                // 대상 시간 계산 (base_hour + hours_ahead)
                int target_hour = base_hour + hours_ahead;
                std::tm target_tm = base_tm;
                target_tm.tm_hour = target_hour;
                // 시간 넘침 처리 (예: 25시 -> 다음날 1시)
                std::time_t target_t = std::mktime(&target_tm); // gmtime 기반이지만 mktime으로 정규화만 수행
                // mktime은 local TZ를 쓰므로, 다시 gmtime으로 정규화된 시/분/초를 얻어야 할 수 있으나
                // 단순하게 target_hour % 24 등으로 처리하는 것이 더 안전함.
                
                int final_target_hour = target_hour % 24;
                // 날짜가 바뀌는 경우는 일단 생략 (초단기예보는 최대 6시간이므로 오늘/내일만 고려하면 됨)
                // 하지만 KMA API 응답에 fcstDate가 명확히 있으므로, 
                // 우리는 base_date와 target_hour를 조합해 비교하면 됨.
                
                std::string tDate = base_date_str;
                if (target_hour >= 24) {
                    // 날짜 더하기 로직 (단순화: 24시간 더함)
                    std::time_t next_day = std::mktime(&base_tm) + 86400;
                    std::tm* next_tm = std::localtime(&next_day);
                    char nextDateBuf[9];
                    std::strftime(nextDateBuf, sizeof(nextDateBuf), "%Y%m%d", next_tm);
                    tDate = nextDateBuf;
                }
                char targetTimeBuf[5];
                sprintf(targetTimeBuf, "%02d00", final_target_hour);
                std::string tTime = targetTimeBuf;
                
                std::cout << "[WeatherManager] Searching forecast for: " << tDate << " " << tTime << std::endl;

                nlohmann::json res;
                res["timestamp"] = tDate + " " + tTime.substr(0, 2) + ":00:00";
                res["temperature"] = nullptr;
                res["humidity"] = nullptr;
                res["wind_speed"] = nullptr;
                res["wind_direction"] = nullptr;
                res["rainfall"] = nullptr;
                res["condition"] = "sunny"; // Default
                
                int sky = 1;
                int pty = 0;
                bool found = false;
                for (const auto& item : items) {
                    if (item["fcstDate"] == tDate && item["fcstTime"] == tTime) {
                        found = true;
                        std::string cat = item["category"];
                        std::string valStr = item["fcstValue"];
                        
                        auto parseVal = [&](const std::string& c, const std::string& v) -> nlohmann::json {
                            if (c == "RN1") {
                                if (v.find("강수없음") != std::string::npos) return 0.0;
                                if (v.find("미만") != std::string::npos) return 0.1;
                                try {
                                    std::string clean = v;
                                    clean.erase(std::remove(clean.begin(), clean.end(), 'm'), clean.end());
                                    return std::stod(clean);
                                } catch (...) { return 0.0; }
                            }
                            try { return std::stod(v); } catch (...) { return nullptr; }
                        };

                        if (cat == "T1H") res["temperature"] = parseVal(cat, valStr);
                        else if (cat == "REH") res["humidity"] = parseVal(cat, valStr);
                        else if (cat == "WSD") res["wind_speed"] = parseVal(cat, valStr);
                        else if (cat == "VEC") res["wind_direction"] = parseVal(cat, valStr);
                        else if (cat == "RN1") res["rainfall"] = parseVal(cat, valStr);
                        else if (cat == "SKY") sky = std::stoi(valStr);
                        else if (cat == "PTY") pty = std::stoi(valStr);
                    }
                }

                // Condition Mapping
                if (pty > 0) {
                    if (pty == 1 || pty == 4 || pty == 5) res["condition"] = "rainy";
                    else if (pty == 2 || pty == 3 || pty == 6 || pty == 7) res["condition"] = "snowy";
                } else {
                    if (sky == 1) res["condition"] = "sunny";
                    else if (sky == 3) res["condition"] = "cloudy";
                    else if (sky == 4) res["condition"] = "overcast";
                }

                if (!found) std::cout << "[WeatherManager] ⚠️ No items found in API result for " << tDate << " " << tTime << std::endl;
                return found ? res : nullptr;
            } else {
                std::cerr << "[WeatherManager] ❌ KMA API Error: " << data.dump() << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[WeatherManager] ❌ JSON Parse Error: " << e.what() << std::endl;
        }
    } else {
        std::cerr << "[WeatherManager] ❌ HTTP Error: " << response.status_code << " " << response.error.message << std::endl;
    }
    return nullptr;
}

std::pair<int, int> WeatherManager::convertGrid(double lat, double lon) {
    const double RE = 6371.00877;
    const double GRID = 5.0;
    const double SLAT1 = 30.0;
    const double SLAT2 = 60.0;
    const double OLON = 126.0;
    const double OLAT = 38.0;
    const double XO = 43;
    const double YO = 136;

    double DEGRAD = M_PI / 180.0;
    double re = RE / GRID;
    double slat1 = SLAT1 * DEGRAD;
    double slat2 = SLAT2 * DEGRAD;
    double olon = OLON * DEGRAD;
    double olat = OLAT * DEGRAD;

    double sn = std::tan(M_PI * 0.25 + slat2 * 0.5) / std::tan(M_PI * 0.25 + slat1 * 0.5);
    sn = std::log(std::cos(slat1) / std::cos(slat2)) / std::log(sn);
    double sf = std::tan(M_PI * 0.25 + slat1 * 0.5);
    sf = std::pow(sf, sn) * std::cos(slat1) / sn;
    double ro = std::tan(M_PI * 0.25 + olat * 0.5);
    ro = re * sf / std::pow(ro, sn);
    
    double ra = std::tan(M_PI * 0.25 + lat * DEGRAD * 0.5);
    ra = re * sf / std::pow(ra, sn);
    double theta = lon * DEGRAD - olon;
    if (theta > M_PI) theta -= 2.0 * M_PI;
    if (theta < -M_PI) theta += 2.0 * M_PI;
    theta *= sn;
    
    int nx = (int)std::floor(ra * std::sin(theta) + XO + 0.5);
    int ny = (int)std::floor(ro - ra * std::cos(theta) + YO + 0.5);
    return {nx, ny};
}

}
