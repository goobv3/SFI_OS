/**
 * @file Router.cpp
 * @brief REST API 라우터 - 프론트엔드(React)와의 통신 관문
 *
 * 기존 파이썬 FastAPI 엔드포인트와 완전히 동일한 URL/메서드를 제공하여
 * 프론트엔드 코드 수정 없이 C++ 백엔드로 전환할 수 있습니다.
 */
#include "Router.h"
#include "../managers/HouseManager.h"
#include "../managers/SensorManager.h"
#include "../managers/ControlManager.h"
#include "../managers/WeatherManager.h"
#include "../managers/RuleEngine.h"
#include "../managers/SiteManager.h"
#include "../managers/SystemManager.h"
#include "../core/JwtUtils.h"
#include "../core/Database.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include "crow/middlewares/cors.h"

namespace API {

// ─────────────────────────────────────────────────────────────────────────────
// setupRoutes: 모든 HTTP 엔드포인트와 메소드별 처리 로직을 구현합니다.
//
// 내부적으로 requireAuth 람다를 선언해 JwtUtils::verifyToken()를 통한 권한 확인을 수행합니다.
// JSON 파싱은 nlohmann::json을 사용합니다.
// ─────────────────────────────────────────────────────────────────────────────
void Router::setupRoutes(crow::App<crow::CORSHandler>& app) {

    // JSON 응답을 위한 헬퍼 람다
    auto jsonResponse = [](const nlohmann::json& data, int status = 200) {
        crow::response res(status, data.dump());
        res.add_header("Content-Type", "application/json; charset=utf-8");
        return res;
    };

    // JWT 미들웨어 헬퍼: Authorization 헤더를 검증하고 TokenPayload 반환
    // /api/auth/* 경로는 제외. 유효하지 않으면 빈 payload (valid=false) 반환.
    auto requireAuth = [](const crow::request& req) -> Core::TokenPayload {
        std::string auth_header = req.get_header_value("Authorization");
        if (auth_header.empty()) return {};
        std::string token = Core::JwtUtils::extractBearerToken(auth_header);
        if (token.empty()) return {};
        return Core::JwtUtils::verifyToken(token);
    };

    // ─── Admin 비밀번호 초기화 ───────────────────────────────────────
    // 서버 시작 시 admin 계정의 password_hash를 실제 PBKDF2 값으로 갱신합니다.
    // (init.sql의 placeholder 해시를 실제 해시로 덮어씁니다)
    {
        auto& db = Core::Database::getInstance();
        auto rows = db.fetchAllPS(
            "SELECT password_salt FROM users WHERE username=?", {"admin"});
        if (!rows.empty()) {
            const std::string salt = rows[0].at("password_salt");
            std::string real_hash = Core::JwtUtils::hashPassword("admin1234", salt);
            db.executePS(
                "UPDATE users SET password_hash=? WHERE username=?",
                {real_hash, "admin"});
            std::cout << "[Router] admin password hash initialized." << std::endl;
        }
    }

    // =========================================================
    // 0. 인증 API (/api/auth/* — JWT 미들웨어 제외)
    // =========================================================

    // POST /api/auth/login
    CROW_ROUTE(app, "/api/auth/login").methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([&jsonResponse](const crow::request& req) {
        if (req.method == crow::HTTPMethod::Options) return jsonResponse({});
        try {
            auto body     = nlohmann::json::parse(req.body);
            std::string username = body.value("username", "");
            std::string password = body.value("password", "");
            if (username.empty() || password.empty())
                return jsonResponse({{"error","username and password required"}}, 400);

            auto& db = Core::Database::getInstance();
            auto rows = db.fetchAllPS(
                "SELECT password_hash, password_salt, role, is_active FROM users WHERE username=?",
                {username});

            if (rows.empty())
                return jsonResponse({{"error","Invalid credentials"}}, 401);

            const auto& row = rows[0];
            if (row.at("is_active") != "1" && row.at("is_active") != "true")
                return jsonResponse({{"error","Account disabled"}}, 403);

            if (!Core::JwtUtils::verifyPassword(password, row.at("password_salt"), row.at("password_hash")))
                return jsonResponse({{"error","Invalid credentials"}}, 401);

            // 마지막 로그인 시각 업데이트
            db.executePS("UPDATE users SET last_login_at=NOW() WHERE username=?", {username});

            int expiry = 24;
            const char* env_exp = std::getenv("JWT_EXPIRY_HOURS");
            if (env_exp) { try { expiry = std::stoi(env_exp); } catch (...) {} }

            std::string token = Core::JwtUtils::generateToken(username, row.at("role"), expiry);
            return jsonResponse({{
                "access_token", token},
                {"token_type", "bearer"},
                {"username", username},
                {"role", row.at("role")}
            });
        } catch (...) {
            return jsonResponse({{"error","invalid payload"}}, 400);
        }
    });

    // GET /api/auth/me
    CROW_ROUTE(app, "/api/auth/me").methods(crow::HTTPMethod::Get)([&jsonResponse, &requireAuth](const crow::request& req) {
        auto auth = requireAuth(req);
        if (!auth.valid)
            return jsonResponse({{"error","Unauthorized"}}, 401);
        return jsonResponse({{"username", auth.username}, {"role", auth.role}});
    });


    // =========================================================
    // 0.5. 사이트(농장) 관련 API
    // =========================================================

    // GET+POST /api/sites
    CROW_ROUTE(app, "/api/sites").methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post, crow::HTTPMethod::Options)([&jsonResponse, &requireAuth](const crow::request& req){
        if (req.method == crow::HTTPMethod::Options) return jsonResponse({});
        if (!requireAuth(req).valid) return jsonResponse({{"error","Unauthorized"}}, 401);
        if (req.method == crow::HTTPMethod::Post) {
            try {
                auto body = nlohmann::json::parse(req.body);
                std::string site_id = body.value("site_id", "");
                std::string name     = body.value("name", site_id);
                std::string location = body.value("location", "");
                bool ok = Managers::SiteManager::getInstance().createSite(site_id, name, location);
                return jsonResponse({{"status", ok ? "created" : "error"}}, ok ? 201 : 400);
            } catch (...) { return jsonResponse({{"error","invalid payload"}}, 400); }
        }
        return jsonResponse(Managers::SiteManager::getInstance().getSites());
    });

    // GET /api/sites/<site_id>/overview
    CROW_ROUTE(app, "/api/sites/<string>/overview").methods(crow::HTTPMethod::Get)([&jsonResponse, &requireAuth](const crow::request& req, const std::string& site_id){
        if (!requireAuth(req).valid) return jsonResponse({{"error","Unauthorized"}}, 401);
        return jsonResponse(Managers::SiteManager::getInstance().getSiteOverview(site_id));
    });

    // GET /api/sites/<site_id>/houses
    CROW_ROUTE(app, "/api/sites/<string>/houses").methods(crow::HTTPMethod::Get)([&jsonResponse, &requireAuth](const crow::request& req, const std::string& site_id){
        if (!requireAuth(req).valid) return jsonResponse({{"error","Unauthorized"}}, 401);
        return jsonResponse(Managers::SiteManager::getInstance().getSiteHouses(site_id));
    });

    // PUT/DELETE /api/sites/<site_id>
    CROW_ROUTE(app, "/api/sites/<string>").methods(crow::HTTPMethod::Put, crow::HTTPMethod::Delete, crow::HTTPMethod::Options)([&jsonResponse, &requireAuth](const crow::request& req, const std::string& site_id){
        if (req.method == crow::HTTPMethod::Options) return jsonResponse({});
        if (!requireAuth(req).valid) return jsonResponse({{"error","Unauthorized"}}, 401);
        if (req.method == crow::HTTPMethod::Put) {
            try {
                auto body = nlohmann::json::parse(req.body);
                std::string name = body.value("name", site_id);
                std::string location = body.value("location", "");
                std::string timezone = body.value("timezone", "Asia/Seoul");
                bool ok = Managers::SiteManager::getInstance().updateSite(site_id, name, location, timezone);
                return jsonResponse({{"status", ok ? "updated" : "error"}}, ok ? 200 : 400);
            } catch (...) { return jsonResponse({{"error","invalid payload"}}, 400); }
        } else {
            bool ok = Managers::SiteManager::getInstance().deleteSite(site_id);
            return jsonResponse({{"status", ok ? "deleted" : "error"}});
        }
    });

    // =========================================================
    // 1. 하우스(동/온실) 단위 관리 API
    // =========================================================

    // GET+POST /api/houses
    CROW_ROUTE(app, "/api/houses").methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post, crow::HTTPMethod::Options)([&jsonResponse, &requireAuth](const crow::request& req){
        if (req.method == crow::HTTPMethod::Options) return jsonResponse({});
        if (!requireAuth(req).valid) return jsonResponse({{"error","Unauthorized"}}, 401);
        if (req.method == crow::HTTPMethod::Post) {
            try {
                auto body = nlohmann::json::parse(req.body);
                std::string house_id = body.value("house_id", "");
                std::string name     = body.value("name", house_id);
                bool ok = Managers::HouseManager::getInstance().createHouse(house_id, name);
                return jsonResponse({{"status", ok ? "created" : "error"}}, ok ? 201 : 400);
            } catch (...) { return jsonResponse({{"error","invalid payload"}}, 400); }
        }
        return jsonResponse(Managers::HouseManager::getInstance().getHouses());
    });

    // GET /api/houses/<house_id>/devices
    CROW_ROUTE(app, "/api/houses/<string>/devices").methods(crow::HTTPMethod::Get)([&jsonResponse, &requireAuth](const crow::request& req, const std::string& house_id){
        if (!requireAuth(req).valid) return jsonResponse({{"error","Unauthorized"}}, 401);
        return jsonResponse(Managers::HouseManager::getInstance().getHouseDevices(house_id));
    });

    // PUT/DELETE /api/houses/<house_id>
    CROW_ROUTE(app, "/api/houses/<string>").methods(crow::HTTPMethod::Put, crow::HTTPMethod::Delete, crow::HTTPMethod::Options)([&jsonResponse, &requireAuth](const crow::request& req, const std::string& house_id){
        if (req.method == crow::HTTPMethod::Options) return jsonResponse({});
        if (!requireAuth(req).valid) return jsonResponse({{"error","Unauthorized"}}, 401);
        if (req.method == crow::HTTPMethod::Put) {
            try {
                auto body = nlohmann::json::parse(req.body);
                std::string name = body.value("name", house_id);
                int display_order = body.value("display_order", 0);
                bool ok = Managers::HouseManager::getInstance().updateHouse(house_id, name, display_order);
                return jsonResponse({{"status", ok ? "updated" : "error"}}, ok ? 200 : 400);
            } catch (...) { return jsonResponse({{"error","invalid payload"}}, 400); }
        } else {
            bool ok = Managers::HouseManager::getInstance().deleteHouse(house_id);
            return jsonResponse({{"status", ok ? "deleted" : "error"}});
        }
    });

    // =========================================================
    // 2. 디바이스 자동감지 (Discovery)
    // =========================================================

    // GET /api/discovery
    CROW_ROUTE(app, "/api/discovery").methods(crow::HTTPMethod::Get)([&jsonResponse, &requireAuth](const crow::request& req){
        if (!requireAuth(req).valid) return jsonResponse({{"error","Unauthorized"}}, 401);
        return jsonResponse(Managers::HouseManager::getInstance().getDiscoveredDevices());
    });

    // =========================================================
    // 3. 알람 관련 API
    // =========================================================

    // GET /api/alarms
    CROW_ROUTE(app, "/api/alarms").methods(crow::HTTPMethod::Get)([&jsonResponse, &requireAuth](const crow::request& req){
        if (!requireAuth(req).valid) return jsonResponse({{"error","Unauthorized"}}, 401);
        return jsonResponse(Managers::SensorManager::getInstance().getAlarms());
    });

    // POST /api/alarms/<id>/acknowledge
    CROW_ROUTE(app, "/api/alarms/<int>/acknowledge").methods(crow::HTTPMethod::Post)([&jsonResponse, &requireAuth](const crow::request& req, int alarm_id){
        if (!requireAuth(req).valid) return jsonResponse({{"error","Unauthorized"}}, 401);
        bool ok = Managers::SensorManager::getInstance().resolveAlarm(alarm_id);
        return jsonResponse({{"status", ok ? "success" : "error"}});
    });

    // =========================================================
    // 4. 센서 데이터 수신 (HTTP fallback - MQTT가 주 경로)
    // =========================================================

    // POST /api/sensors
    CROW_ROUTE(app, "/api/sensors").methods(crow::HTTPMethod::Post)([&jsonResponse, &requireAuth](const crow::request& req){
        if (!requireAuth(req).valid) return jsonResponse({{"error","Unauthorized"}}, 401);
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string sensor_id = body.value("sensor_id", "");
            double value = body.value("value", 0.0);
            if (sensor_id.empty()) return jsonResponse({{"error","sensor_id required"}}, 400);
            Managers::SensorManager::getInstance().processIncomingData(sensor_id, value);
            return jsonResponse({{"status","ok"}});
        } catch (...) {
            return jsonResponse({{"error","invalid payload"}}, 400);
        }
    });

    // GET /api/sensors/<string>/history?start=&end=&resolution=auto
    CROW_ROUTE(app, "/api/sensors/<string>/history").methods(crow::HTTPMethod::Get)([&jsonResponse, &requireAuth](const crow::request& req, const std::string& sensors_str){
        if (!requireAuth(req).valid) return jsonResponse({{"error","Unauthorized"}}, 401);
        
        std::string start_str = req.url_params.get("start") ? req.url_params.get("start") : "";
        std::string end_str   = req.url_params.get("end") ? req.url_params.get("end") : "";
        std::string res_param = req.url_params.get("resolution") ? req.url_params.get("resolution") : "auto";
        
        std::string table = "sensors";
        std::string time_col = "timestamp";
        std::string val_col = "value";
        
        if (res_param == "auto") {
            struct tm t_start={0}, t_end={0};
            if(start_str.size()>=10) { sscanf(start_str.c_str(), "%4d-%2d-%2d", &t_start.tm_year, &t_start.tm_mon, &t_start.tm_mday); t_start.tm_year-=1900; t_start.tm_mon-=1; }
            if(end_str.size()>=10) { sscanf(end_str.c_str(), "%4d-%2d-%2d", &t_end.tm_year, &t_end.tm_mon, &t_end.tm_mday); t_end.tm_year-=1900; t_end.tm_mon-=1; }
            time_t time1 = mktime(&t_start);
            time_t time2 = mktime(&t_end);
            double diff_days = difftime(time2, time1) / (60*60*24);
            
            if (diff_days > 90) res_param = "daily";
            else if (diff_days > 7) res_param = "hourly";
            else res_param = "raw";
        }
        
        if (res_param == "daily") { table = "sensors_daily"; time_col = "day_date"; val_col = "avg_val"; }
        else if (res_param == "hourly") { table = "sensors_hourly"; time_col = "hour_ts"; val_col = "avg_val"; }
        
        auto& db = Core::Database::getInstance();
        std::string sql = "SELECT sensor_id, DATE_FORMAT(" + time_col + ", '%Y-%m-%dT%H:%i:%sZ') as time, " + val_col + " as value FROM " + table + 
                          " WHERE FIND_IN_SET(sensor_id, ?) AND " + time_col + " >= ? AND " + time_col + " <= ? ORDER BY time ASC";
                          
        auto rows = db.fetchAllPS(sql, {sensors_str, start_str, end_str});
        
        nlohmann::json result = nlohmann::json::array();
        std::map<std::string, nlohmann::json> time_map;
        for (const auto& row : rows) {
            std::string t = row.at("time");
            std::string s_id = row.at("sensor_id");
            if (row.at("value").empty()) continue;
            double v = std::stod(row.at("value"));
            time_map[t]["time"] = t;
            time_map[t][s_id] = v;
        }
        for (auto& pair : time_map) {
            result.push_back(pair.second);
        }
        
        return jsonResponse(result);
    });

    // =========================================================
    // 5. 제어 명령 API
    // =========================================================

    // POST /api/control
    CROW_ROUTE(app, "/api/control").methods(crow::HTTPMethod::Post)([&jsonResponse, &requireAuth](const crow::request& req){
        if (!requireAuth(req).valid) return jsonResponse({{"error","Unauthorized"}}, 401);
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string actuator_id = body.value("actuator_id", "");
            std::string command     = body.value("command", "");
            std::string user_id     = body.value("source", "UserDashboard");
            bool ok = Managers::ControlManager::getInstance().processControlCommand(actuator_id, command, user_id);
            return jsonResponse({{"status", ok ? "success" : "error"}});
        } catch (...) {
            return jsonResponse({{"error","invalid payload"}}, 400);
        }
    });

    // =========================================================
    // 6. 날씨 데이터 API
    // =========================================================

    // GET /api/weather/latest?hours_ahead=N
    CROW_ROUTE(app, "/api/weather/latest").methods(crow::HTTPMethod::Get)([&jsonResponse, &requireAuth](const crow::request& req){
        if (!requireAuth(req).valid) return jsonResponse({{"error","Unauthorized"}}, 401);
        int hours = 0;
        auto it = req.url_params.get("hours_ahead");
        if (it) { try { hours = std::stoi(it); } catch (...) {} }
        return jsonResponse(Managers::WeatherManager::getInstance().getLatestWeather(hours));
    });

    // =========================================================
    // 7. 메타데이터 CRUD API (/api/metadata/*)
    // =========================================================

    // POST /api/metadata/houses
    CROW_ROUTE(app, "/api/metadata/houses").methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([&jsonResponse, &requireAuth](const crow::request& req){
        if (req.method == crow::HTTPMethod::Options) return jsonResponse({});
        if (!requireAuth(req).valid) return jsonResponse({{"error","Unauthorized"}}, 401);
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string house_id = body.value("house_id", "");
            std::string name     = body.value("name", house_id);
            bool ok = Managers::HouseManager::getInstance().createHouse(house_id, name);
            return jsonResponse({{"status", ok ? "created" : "error"}}, ok ? 201 : 400);
        } catch (...) { return jsonResponse({{"error","invalid payload"}}, 400); }
    });

    // PUT/DELETE /api/metadata/houses/<string>
    CROW_ROUTE(app, "/api/metadata/houses/<string>").methods(crow::HTTPMethod::Put, crow::HTTPMethod::Delete, crow::HTTPMethod::Options)([&jsonResponse, &requireAuth](const crow::request& req, const std::string& house_id){
        if (req.method == crow::HTTPMethod::Options) return jsonResponse({});
        if (!requireAuth(req).valid) return jsonResponse({{"error","Unauthorized"}}, 401);
        if (req.method == crow::HTTPMethod::Put) {
            try {
                auto body = nlohmann::json::parse(req.body);
                std::string name = body.value("name", house_id);
                int display_order = body.value("display_order", 0);
                bool ok = Managers::HouseManager::getInstance().updateHouse(house_id, name, display_order);
                return jsonResponse({{"status", ok ? "updated" : "error"}}, ok ? 200 : 400);
            } catch (...) { return jsonResponse({{"error","invalid payload"}}, 400); }
        } else {
            bool ok = Managers::HouseManager::getInstance().deleteHouse(house_id);
            return jsonResponse({{"status", ok ? "deleted" : "error"}});
        }
    });

    // PUT /api/metadata/houses/reorder
    CROW_ROUTE(app, "/api/metadata/houses/reorder").methods(crow::HTTPMethod::Put, crow::HTTPMethod::Options)([&jsonResponse, &requireAuth](const crow::request& req){
        if (req.method == crow::HTTPMethod::Options) return jsonResponse({});
        if (!requireAuth(req).valid) return jsonResponse({{"error","Unauthorized"}}, 401);
        try {
            auto body = nlohmann::json::parse(req.body);
            bool ok = Managers::HouseManager::getInstance().updateHousesOrder(body["ordered_ids"]);
            return jsonResponse({{"status", ok ? "ok" : "error"}});
        } catch (...) { return jsonResponse({{"error","invalid payload"}}, 400); }
    });

    // POST /api/metadata/sensors
    CROW_ROUTE(app, "/api/metadata/sensors").methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([&jsonResponse, &requireAuth](const crow::request& req){
        if (req.method == crow::HTTPMethod::Options) return jsonResponse({});
        if (!requireAuth(req).valid) return jsonResponse({{"error","Unauthorized"}}, 401);
        try {
            auto body = nlohmann::json::parse(req.body);
            bool ok = Managers::HouseManager::getInstance().createSensor(body);
            return jsonResponse({{"status", ok ? "created" : "error"}}, ok ? 201 : 400);
        } catch (...) { return jsonResponse({{"error","invalid payload"}}, 400); }
    });

    // PUT/DELETE /api/metadata/sensors/<sensor_id>
    CROW_ROUTE(app, "/api/metadata/sensors/<string>").methods(crow::HTTPMethod::Put, crow::HTTPMethod::Delete, crow::HTTPMethod::Options)([&jsonResponse, &requireAuth](const crow::request& req, const std::string& sensor_id){
        if (req.method == crow::HTTPMethod::Options) return jsonResponse({});
        if (!requireAuth(req).valid) return jsonResponse({{"error","Unauthorized"}}, 401);
        if (req.method == crow::HTTPMethod::Put) {
            try {
                auto body = nlohmann::json::parse(req.body);
                bool ok = Managers::HouseManager::getInstance().updateSensor(sensor_id, body);
                return jsonResponse({{"status", ok ? "updated" : "error"}});
            } catch (...) { return jsonResponse({{"error","invalid payload"}}, 400); }
        } else {
            bool ok = Managers::HouseManager::getInstance().deleteSensor(sensor_id);
            return jsonResponse({{"status", ok ? "deleted" : "error"}});
        }
    });

    // POST /api/metadata/actuators
    CROW_ROUTE(app, "/api/metadata/actuators").methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([&jsonResponse, &requireAuth](const crow::request& req){
        if (req.method == crow::HTTPMethod::Options) return jsonResponse({});
        if (!requireAuth(req).valid) return jsonResponse({{"error","Unauthorized"}}, 401);
        try {
            auto body = nlohmann::json::parse(req.body);
            bool ok = Managers::HouseManager::getInstance().createActuator(body);
            return jsonResponse({{"status", ok ? "created" : "error"}}, ok ? 201 : 400);
        } catch (...) { return jsonResponse({{"error","invalid payload"}}, 400); }
    });

    // PUT/DELETE /api/metadata/actuators/<actuator_id>
    CROW_ROUTE(app, "/api/metadata/actuators/<string>").methods(crow::HTTPMethod::Put, crow::HTTPMethod::Delete, crow::HTTPMethod::Options)([&jsonResponse, &requireAuth](const crow::request& req, const std::string& actuator_id){
        if (req.method == crow::HTTPMethod::Options) return jsonResponse({});
        if (!requireAuth(req).valid) return jsonResponse({{"error","Unauthorized"}}, 401);
        if (req.method == crow::HTTPMethod::Put) {
            try {
                auto body = nlohmann::json::parse(req.body);
                bool ok = Managers::HouseManager::getInstance().updateActuator(actuator_id, body);
                return jsonResponse({{"status", ok ? "updated" : "error"}});
            } catch (...) { return jsonResponse({{"error","invalid payload"}}, 400); }
        } else {
            bool ok = Managers::HouseManager::getInstance().deleteActuator(actuator_id);
            return jsonResponse({{"status", ok ? "deleted" : "error"}});
        }
    });

    // DELETE /api/metadata/discovery/<device_id>
    CROW_ROUTE(app, "/api/metadata/discovery/<string>").methods(crow::HTTPMethod::Delete, crow::HTTPMethod::Options)([&jsonResponse, &requireAuth](const crow::request& req, const std::string& device_id){
        if (req.method == crow::HTTPMethod::Options) return jsonResponse({});
        if (!requireAuth(req).valid) return jsonResponse({{"error","Unauthorized"}}, 401);
        bool ok = Managers::HouseManager::getInstance().deleteDiscoveredDevice(device_id);
        return jsonResponse({{"status", ok ? "deleted" : "error"}});
    });

    // =========================================================
    // 8. 자동화 룰 엔진 API (센서 수치 연동 자동제어 조건 관리)
    // =========================================================

    // GET ?house_id= / POST /api/automation/rules
    CROW_ROUTE(app, "/api/automation/rules").methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post, crow::HTTPMethod::Options)([&jsonResponse, &requireAuth](const crow::request& req){
        if (req.method == crow::HTTPMethod::Options) return jsonResponse({});
        if (!requireAuth(req).valid) return jsonResponse({{"error","Unauthorized"}}, 401);
        if (req.method == crow::HTTPMethod::Post) {
            try {
                auto body = nlohmann::json::parse(req.body);
                auto result = Managers::RuleEngine::getInstance().createRule(body);
                int status = (result.value("status", "") == "created") ? 201 : 400;
                return jsonResponse(result, status);
            } catch (...) { return jsonResponse({{"error","invalid payload"}}, 400); }
        }
        std::string house_id;
        auto it = req.url_params.get("house_id");
        if (it) house_id = it;
        return jsonResponse(Managers::RuleEngine::getInstance().getRules(house_id));
    });

    // PUT / DELETE /api/automation/rules/<id>
    CROW_ROUTE(app, "/api/automation/rules/<int>").methods(crow::HTTPMethod::Put, crow::HTTPMethod::Delete, crow::HTTPMethod::Options)([&jsonResponse, &requireAuth](const crow::request& req, int id){
        if (req.method == crow::HTTPMethod::Options) return jsonResponse({});
        if (!requireAuth(req).valid) return jsonResponse({{"error","Unauthorized"}}, 401);
        if (req.method == crow::HTTPMethod::Put) {
            try {
                auto body = nlohmann::json::parse(req.body);
                bool ok = Managers::RuleEngine::getInstance().updateRule(static_cast<long long>(id), body);
                return jsonResponse({{"status", ok ? "updated" : "error"}}, ok ? 200 : 400);
            } catch (...) { return jsonResponse({{"error","invalid payload"}}, 400); }
        } else {
            bool ok = Managers::RuleEngine::getInstance().deleteRule(static_cast<long long>(id));
            return jsonResponse({{"status", ok ? "deleted" : "error"}});
        }
    });

    // PATCH /api/automation/rules/<id>/toggle
    CROW_ROUTE(app, "/api/automation/rules/<int>/toggle").methods(crow::HTTPMethod::Patch, crow::HTTPMethod::Options)([&jsonResponse, &requireAuth](const crow::request& req, int id){
        if (req.method == crow::HTTPMethod::Options) return jsonResponse({});
        if (!requireAuth(req).valid) return jsonResponse({{"error","Unauthorized"}}, 401);
        try {
            auto body = nlohmann::json::parse(req.body);
            bool enabled = body.value("is_enabled", true);
            bool ok = Managers::RuleEngine::getInstance().toggleRule(static_cast<long long>(id), enabled);
            return jsonResponse({{"status", ok ? "ok" : "error"}});
        } catch (...) { return jsonResponse({{"error","invalid payload"}}, 400); }
    });

    // =========================================================
    // 9. 헬스체크
    // =========================================================
    CROW_ROUTE(app, "/")([](){
        return "Smart Farm Intelligence OS - C++ Backend v1.0 [Optimized by Antigravity]";
    });

    // =========================================================
    // 9. 시스템 모니터링 및 로그 API
    // =========================================================

    // 시스템 상태 (CPU, RAM, Disk, Service Status)
    CROW_ROUTE(app, "/api/admin/system/status")
    .methods(crow::HTTPMethod::GET)([&](const crow::request& req) {
        auto payload = requireAuth(req);
        if (!payload.valid || (payload.role != "admin" && payload.role != "operator")) {
            return crow::response(403, "Admin/Operator role required");
        }
        
        return jsonResponse(Managers::SystemManager::getInstance().getSystemStatus());
    });

    // 서버 로그 조회 (최근 N줄)
    CROW_ROUTE(app, "/api/admin/system/logs")
    .methods(crow::HTTPMethod::GET)([&](const crow::request& req) {
        auto payload = requireAuth(req);
        if (!payload.valid || (payload.role != "admin" && payload.role != "operator")) {
            return crow::response(403, "Admin/Operator role required");
        }

        int lines = 50;
        if (req.url_params.get("lines")) {
            try { lines = std::stoi(req.url_params.get("lines")); } catch (...) {}
        }

        auto logs = Managers::SystemManager::getInstance().getLatestLogs(lines);
        nlohmann::json logArray = nlohmann::json::array();
        for (const auto& log : logs) logArray.push_back(log);

        return jsonResponse(logArray);
    });

    std::cout << "[Router] ✅ 모든 REST API 라우트 등록 완료." << std::endl;
}

} // namespace API
