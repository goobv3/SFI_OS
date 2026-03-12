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
#include <nlohmann/json.hpp>
#include <iostream>

namespace API {

void Router::setupRoutes(crow::SimpleApp& app) {

    // JSON 응답을 CORS 헤더와 함께 생성하는 헬퍼 람다
    auto jsonResponse = [](const nlohmann::json& data, int status = 200) {
        crow::response res(status, data.dump());
        res.add_header("Content-Type", "application/json; charset=utf-8");
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        return res;
    };

    // --- OPTIONS 사전 요청 처리 (CORS Preflight) ---
    CROW_ROUTE(app, "/api/<path>").methods(crow::HTTPMethod::Options)([&jsonResponse](const crow::request&, const std::string&){
        return jsonResponse({});
    });

    // =========================================================
    // 1. 하우스(동) 관련 API
    // =========================================================

    // GET /api/houses - 전체 하우스 목록
    CROW_ROUTE(app, "/api/houses").methods(crow::HTTPMethod::Get)([&jsonResponse](){
        return jsonResponse(Managers::HouseManager::getInstance().getHouses());
    });

    // GET /api/houses/{house_id}/devices - 특정 하우스 기기 목록
    CROW_ROUTE(app, "/api/houses/<string>/devices").methods(crow::HTTPMethod::Get)([&jsonResponse](const std::string& house_id){
        return jsonResponse(Managers::HouseManager::getInstance().getHouseDevices(house_id));
    });

    // =========================================================
    // 2. 디바이스 자동감지 (Discovery)
    // =========================================================

    // GET /api/discovery - 미등록 발견 기기 목록
    CROW_ROUTE(app, "/api/discovery").methods(crow::HTTPMethod::Get)([&jsonResponse](){
        return jsonResponse(Managers::HouseManager::getInstance().getDiscoveredDevices());
    });

    // =========================================================
    // 3. 알람 관련 API
    // =========================================================

    // GET /api/alarms - 미확인 알람 목록
    CROW_ROUTE(app, "/api/alarms").methods(crow::HTTPMethod::Get)([&jsonResponse](){
        return jsonResponse(Managers::SensorManager::getInstance().getAlarms());
    });

    // POST /api/alarms/{id}/acknowledge
    CROW_ROUTE(app, "/api/alarms/<int>/acknowledge").methods(crow::HTTPMethod::Post)([&jsonResponse](int alarm_id){
        bool ok = Managers::SensorManager::getInstance().resolveAlarm(alarm_id);
        return jsonResponse({{"status", ok ? "success" : "error"}});
    });

    // =========================================================
    // 4. 센서 데이터 수신 (HTTP fallback - MQTT가 주 경로)
    // =========================================================

    // POST /api/sensors - HTTP로 센서 데이터 수신
    CROW_ROUTE(app, "/api/sensors").methods(crow::HTTPMethod::Post)([&jsonResponse](const crow::request& req){
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

    // =========================================================
    // 5. 제어 명령 API
    // =========================================================

    // POST /api/control - 액추에이터 제어 명령
    CROW_ROUTE(app, "/api/control").methods(crow::HTTPMethod::Post)([&jsonResponse](const crow::request& req){
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
    CROW_ROUTE(app, "/api/weather/latest").methods(crow::HTTPMethod::Get)([&jsonResponse](const crow::request& req){
        int hours = 0;
        auto it = req.url_params.get("hours_ahead");
        if (it) {
            try { hours = std::stoi(it); } catch (...) {}
        }
        return jsonResponse(Managers::WeatherManager::getInstance().getLatestWeather(hours));
    });

    // =========================================================
    // 7. 헬스체크
    // =========================================================
    CROW_ROUTE(app, "/")([](){
        return "Smart Farm Intelligence OS - C++ Backend v1.0 [Optimized by Antigravity]";
    });

    std::cout << "[Router] ✅ 모든 REST API 라우트 등록 완료." << std::endl;
}

} // namespace API
