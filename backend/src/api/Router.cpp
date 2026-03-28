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
#include "crow/middlewares/cors.h"

namespace API {

void Router::setupRoutes(crow::App<crow::CORSHandler>& app) {

    // JSON 응답을 위한 헬퍼 람다 (CORS 헤더는 CORSHandler가 처리하므로 최소화)
    auto jsonResponse = [](const nlohmann::json& data, int status = 200) {
        crow::response res(status, data.dump());
        res.add_header("Content-Type", "application/json; charset=utf-8");
        return res;
    };

    // =========================================================
    // 1. 하우스(동) 관련 API
    // =========================================================

    // GET+POST /api/houses - 전체 하우스 목록 조회 및 생성
    CROW_ROUTE(app, "/api/houses").methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post, crow::HTTPMethod::Options)([&jsonResponse](const crow::request& req){
        if (req.method == crow::HTTPMethod::Options) return jsonResponse({});
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

    // GET /api/houses/{house_id}/devices - 특정 하우스 기기 목록
    CROW_ROUTE(app, "/api/houses/<string>/devices").methods(crow::HTTPMethod::Get)([&jsonResponse](const std::string& house_id){
        return jsonResponse(Managers::HouseManager::getInstance().getHouseDevices(house_id));
    });

    // PUT/DELETE /api/houses/<house_id> - 하우스 수정/삭제 (레거시 경로 호환)
    CROW_ROUTE(app, "/api/houses/<string>").methods(crow::HTTPMethod::Put, crow::HTTPMethod::Delete, crow::HTTPMethod::Options)([&jsonResponse](const crow::request& req, const std::string& house_id){
        if (req.method == crow::HTTPMethod::Options) return jsonResponse({});
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
    // 7. 메타데이터 CRUD API (/api/metadata/*)
    // =========================================================

    // POST /api/metadata/houses - 재배동 생성
    CROW_ROUTE(app, "/api/metadata/houses").methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([&jsonResponse](const crow::request& req){
        if (req.method == crow::HTTPMethod::Options) return jsonResponse({});
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string house_id = body.value("house_id", "");
            std::string name     = body.value("name", house_id);
            bool ok = Managers::HouseManager::getInstance().createHouse(house_id, name);
            return jsonResponse({{"status", ok ? "created" : "error"}}, ok ? 201 : 400);
        } catch (...) { return jsonResponse({{"error","invalid payload"}}, 400); }
    });

    // PUT/DELETE /api/metadata/houses/<string> - 재배동 수정 및 삭제
    CROW_ROUTE(app, "/api/metadata/houses/<string>").methods(crow::HTTPMethod::Put, crow::HTTPMethod::Delete, crow::HTTPMethod::Options)([&jsonResponse](const crow::request& req, const std::string& house_id){
        if (req.method == crow::HTTPMethod::Options) return jsonResponse({});
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

    // PUT /api/metadata/houses/reorder - 재배동 순서 저장
    CROW_ROUTE(app, "/api/metadata/houses/reorder").methods(crow::HTTPMethod::Put, crow::HTTPMethod::Options)([&jsonResponse](const crow::request& req){
        if (req.method == crow::HTTPMethod::Options) return jsonResponse({});
        try {
            auto body = nlohmann::json::parse(req.body);
            bool ok = Managers::HouseManager::getInstance().updateHousesOrder(body["ordered_ids"]);
            return jsonResponse({{"status", ok ? "ok" : "error"}});
        } catch (...) { return jsonResponse({{"error","invalid payload"}}, 400); }
    });

    // POST /api/metadata/sensors - 센서 등록
    CROW_ROUTE(app, "/api/metadata/sensors").methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([&jsonResponse](const crow::request& req){
        if (req.method == crow::HTTPMethod::Options) return jsonResponse({});
        try {
            auto body = nlohmann::json::parse(req.body);
            bool ok = Managers::HouseManager::getInstance().createSensor(body);
            return jsonResponse({{"status", ok ? "created" : "error"}}, ok ? 201 : 400);
        } catch (...) { return jsonResponse({{"error","invalid payload"}}, 400); }
    });

    // PUT/DELETE /api/metadata/sensors/<sensor_id> - 센서 상태 수정 및 삭제
    CROW_ROUTE(app, "/api/metadata/sensors/<string>").methods(crow::HTTPMethod::Put, crow::HTTPMethod::Delete, crow::HTTPMethod::Options)([&jsonResponse](const crow::request& req, const std::string& sensor_id){
        if (req.method == crow::HTTPMethod::Options) return jsonResponse({});
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

    // POST /api/metadata/actuators - 액추에이터 등록
    CROW_ROUTE(app, "/api/metadata/actuators").methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([&jsonResponse](const crow::request& req){
        if (req.method == crow::HTTPMethod::Options) return jsonResponse({});
        try {
            auto body = nlohmann::json::parse(req.body);
            bool ok = Managers::HouseManager::getInstance().createActuator(body);
            return jsonResponse({{"status", ok ? "created" : "error"}}, ok ? 201 : 400);
        } catch (...) { return jsonResponse({{"error","invalid payload"}}, 400); }
    });

    // PUT/DELETE /api/metadata/actuators/<actuator_id> - 액추에이터 수정 및 삭제
    CROW_ROUTE(app, "/api/metadata/actuators/<string>").methods(crow::HTTPMethod::Put, crow::HTTPMethod::Delete, crow::HTTPMethod::Options)([&jsonResponse](const crow::request& req, const std::string& actuator_id){
        if (req.method == crow::HTTPMethod::Options) return jsonResponse({});
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

    // DELETE /api/metadata/discovery/<device_id> - 감지기기 삭제/무시
    CROW_ROUTE(app, "/api/metadata/discovery/<string>").methods(crow::HTTPMethod::Delete, crow::HTTPMethod::Options)([&jsonResponse](const crow::request& req, const std::string& device_id){
        if (req.method == crow::HTTPMethod::Options) return jsonResponse({});
        bool ok = Managers::HouseManager::getInstance().deleteDiscoveredDevice(device_id);
        return jsonResponse({{"status", ok ? "deleted" : "error"}});
    });

    // =========================================================
    // 8. 헬스체크
    // =========================================================
    CROW_ROUTE(app, "/")([](){
        return "Smart Farm Intelligence OS - C++ Backend v1.0 [Optimized by Antigravity]";
    });

    std::cout << "[Router] ✅ 모든 REST API 라우트 등록 완료." << std::endl;
}

} // namespace API
