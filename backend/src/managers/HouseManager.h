/**
 * @file HouseManager.h
 * @brief 하우스(온실) 및 디바이스 메타데이터 관리를 담당하는 매니저
 *
 * ▶ 역할
 *   - 하우스 동별로 어떤 센서, 어떤 액추에이터가 있는지 (sensor_metadata, actuator_metadata) 관리
 *   - 프론트엔드 UI를 위해 표시 순서(display_order) 업데이트 로직 제공
 *   - 새롭게 발견된 알 수 없는 디바이스(discovered_devices) 목록 확인 및 관리
 *
 * ▶ 커스터마이징 가이드
 *   1. 모델명, 시리얼 번호 등의 속성을 센서에 추가하려면 createSensor() 등에 컬럼 삽입 로직을 추가해야 함.
 */
#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace Managers {
class HouseManager {
public:
    static HouseManager& getInstance();

    // ── 조회 관련 (하우스 및 포함된 디바이스) ──
    nlohmann::json getHouses();
    nlohmann::json getHouseDevices(const std::string& house_id);
    nlohmann::json getDiscoveredDevices();

    // ── 하우스 기본 정보 CRUD ──
    bool createHouse(const std::string& house_id, const std::string& name);
    bool updateHouse(const std::string& house_id, const std::string& name, int display_order);
    bool deleteHouse(const std::string& house_id);
    bool updateHousesOrder(const nlohmann::json& ordered_ids);

    // ── 센서 메타데이터 CRUD (하우스 소속) ──
    bool createSensor(const nlohmann::json& body);
    bool updateSensor(const std::string& sensor_id, const nlohmann::json& body);
    bool deleteSensor(const std::string& sensor_id);

    // ── 액추에이터 메타데이터 CRUD (하우스 소속) ──
    bool createActuator(const nlohmann::json& body);
    bool updateActuator(const std::string& actuator_id, const nlohmann::json& body);
    bool deleteActuator(const std::string& actuator_id);

    // ── 감지기기 삭제 (ManageMode에서 무시하기 처리) ──
    bool deleteDiscoveredDevice(const std::string& device_id);

private:
    HouseManager() = default;
    ~HouseManager() = default;
    HouseManager(const HouseManager&) = delete;
    HouseManager& operator=(const HouseManager&) = delete;
};
}
