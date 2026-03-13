/**
 * @file main.cpp
 * @brief 스마트팜 C++ 백엔드 서버 진입점
 *
 * Python FastAPI → C++ Crow 마이그레이션 메인 파일입니다.
 * 1. MariaDB 연결 초기화
 * 2. MQTT 브로커 연결 및 센서 토픽 구독
 * 3. REST API 라우트 등록
 * 4. 멀티스레드 Crow 웹서버 시작 (포트 8000)
 */
#include <iostream>
#include <string>
#include <stdexcept>

#include "crow.h"
#include "api/Router.h"
#include "core/Database.h"
#include "core/MqttClient.h"
#include "managers/SensorManager.h"

int main() {
    std::cout << "=======================================================" << std::endl;
    std::cout << "  Smart Farm Intelligence OS - C++ Backend v1.0" << std::endl;
    std::cout << "  Powered by Crow + MariaDB + Paho MQTT" << std::endl;
    std::cout << "=======================================================" << std::endl;

    // --- 1. 데이터베이스 연결 ---
    std::cout << "[Boot] MariaDB 연결 시도..." << std::endl;
    int retries = 10;
    while (retries-- > 0) {
        if (Core::Database::getInstance().connect()) {
            std::cout << "[Boot] ✅ MariaDB 연결 성공" << std::endl;
            break;
        }
        std::cout << "[Boot] DB 연결 실패, " << retries << "회 재시도 중..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    // --- 2. MQTT 연결 및 센서 구독 ---
    std::cout << "[Boot] MQTT 브로커 연결 시도..." << std::endl;
    if (Core::MqttClient::getInstance().connect()) {
        std::cout << "[Boot] ✅ MQTT 연결 성공" << std::endl;

        // smartfarm/sensors/+/value 토픽 구독 (+ 는 모든 sensor_id)
        Core::MqttClient::getInstance().subscribe(
            "smartfarm/sensors/+/value",
            [](const std::string& topic, const std::string& payload) {
                // 토픽에서 sensor_id 추출: "smartfarm/sensors/TEMP_01/value" → "TEMP_01"
                std::string s = topic;
                size_t p1 = s.find('/', s.find('/') + 1); // 두 번째 '/'
                size_t p2 = s.rfind('/');
                if (p1 != std::string::npos && p2 != std::string::npos && p1 < p2) {
                    std::string sensor_id = s.substr(p1 + 1, p2 - p1 - 1);
                    try {
                        double value = std::stod(payload);
                        Managers::SensorManager::getInstance().processIncomingData(sensor_id, value);
                    } catch (const std::exception& e) {
                        std::cerr << "[MQTT] Payload 파싱 오류: " << e.what() << std::endl;
                    }
                }
            }
        );
    } else {
        std::cout << "[Boot] ⚠️ MQTT 연결 실패 - 계속 진행합니다." << std::endl;
    }

    // --- 3. Crow 웹서버 조립 (CORSHandler 미들웨어 포함) ---
    crow::App<crow::CORSHandler> app;

    // CORS 전역 정책: 모든 출처·메서드·헤더 허용
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors.global()
        .origin("*")
        .methods("GET,POST,PUT,DELETE,OPTIONS"_method)
        .headers("Content-Type, Authorization")
        .max_age(86400);

    // 모든 REST 라우트 등록
    API::Router::setupRoutes(app);

    std::cout << "[Boot] ✅ REST API 라우트 등록 완료" << std::endl;
    std::cout << "[Boot] 🚀 서버 시작 - 포트 8000 수신 대기 중..." << std::endl;

    // --- 4. 멀티스레드 서버 실행 ---
    app.port(8000)
       .multithreaded()
       .run();

    return 0;
}
