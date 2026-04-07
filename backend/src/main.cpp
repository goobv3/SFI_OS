/**
 * @file main.cpp
 * @brief 스마트팜 C++ 백엔드 서버 진입점 (EntryPoint)
 *
 * ▶ 시스템 아키텍처 (Python FastAPI → C++ Crow 전환)
 * 1. MariaDB 연결 초기화: Database 클래스로 DB 싱글톤 객체 준비
 * 2. MQTT 브로커 연결 및 센서/날씨 토픽 와일드카드(+) 구독 처리
 * 3. REST API 라우트 조립: API::Router::setupRoutes() 활용
 * 4. 멀티스레드 기반 Crow 웹서버(포트 8000) 구동 완료
 *
 * 이 파일은 백엔드 서버의 뼈대 역할을 하며 컴파일과 서버 시작의 중심이 됩니다.
 */
#include <iostream>
#include <string>
#include <stdexcept>

#include "crow.h"
#include "api/Router.h"
#include "core/Database.h"
#include "core/MqttClient.h"
#include "managers/SensorManager.h"
#include "managers/WeatherManager.h" // [추가] 기상 데이터 관리를 위한 헤더


int main() {
    // [보정] 모든 로그를 backend.log 파일로 리다이렉트하여 웹 모니터링 콘솔에 표시 가능하게 함
    std::freopen("backend.log", "w", stdout);
    std::setvbuf(stdout, NULL, _IOLBF, 0); // 라인 버퍼링 설정 (실시간 기록성 향상)

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

        // --- [추가] 로컬 기상대(FARM) MQTT 구독 ---
        // 'smartfarm/weather/farm' 토픽으로 들어오는 JSON 데이터를 수신합니다.
        Core::MqttClient::getInstance().subscribe(
            "smartfarm/weather/farm",
            [](const std::string& topic, const std::string& payload) {
                try {
                    auto data = nlohmann::json::parse(payload);
                    Managers::WeatherManager::getInstance().recordFarmWeather(data);
                } catch (const std::exception& e) {
                    std::cerr << "[MQTT] 기상 데이터 파싱 오류: " << e.what() << std::endl;
                }
            }
        );
    } else {
        std::cout << "[Boot] ⚠️ MQTT 연결 실패 - 계속 진행합니다." << std::endl;
    }

    // --- 3. Crow 웹서버 조립 (CORSHandler 미들웨어 포함) ---
    crow::App<crow::CORSHandler> app;

    // CORS 전역 정책: 모든 출처·헤더 허용
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors.global()
        .origin("*")
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
