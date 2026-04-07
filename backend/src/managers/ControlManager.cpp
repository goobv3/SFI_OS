/**
 * @file ControlManager.cpp
 * @brief ControlManager 클래스 구현
 *
 * ▶ 처리 흐름
 *   1. REST API/룰엔진에서 processControlCommand(actuator_id, command, user_id) 호출
 *   2. Database 싱글톤을 가져옴
 *   3. actuator_metadata 테이블 조회 (유효한 actuator_id인지 확인)
 *      - prepared statement(fetchAllPS)로 SQL Injection 방어
 *   4. 유효하지 않으면 false 반환 후 즉시 종료
 *   5. control_logs 테이블에 실행 이력 INSERT (status='SUCCESS')
 *   6. MQTT 토픽(smartfarm/actuators/<id>/command) 조합
 *   7. MqttClient::publish()를 호출하여 비동기로 메시지 발행 (QoS 1)
 */
#include "ControlManager.h"
#include "../core/Database.h"
#include "../core/MqttClient.h"
#include <iostream>

namespace Managers {

// ─────────────────────────────────────────────────────────────────────────────
// getInstance — Meyers' Singleton
// ─────────────────────────────────────────────────────────────────────────────
ControlManager& ControlManager::getInstance() {
    static ControlManager instance;
    return instance;
}

// ─────────────────────────────────────────────────────────────────────────────
// processControlCommand — 액추에이터 제어 명령 검증, 로깅, MQTT 발행
//
// @param actuator_id 액추에이터 식별자 (예: "FAN_1")
// @param command     명령어 문자열 (예: "ON", "OFF")
// @param user_id     명령을 내린 주체 (사용자명, "AutoRule" 등)
// @return true 제어 명령 전송 완료, false 액추에이터 미등록
// ─────────────────────────────────────────────────────────────────────────────
bool ControlManager::processControlCommand(const std::string& actuator_id,
                                           const std::string& command,
                                           const std::string& user_id) {
    auto& db = Core::Database::getInstance();

    // ── 1. 액추에이터 유효성 검사 ──
    // actuator_metadata 테이블에 등록된 기기인지 확인
    // actuator_id가 외부에서 주입될 수 있으므로 반드시 PS 사용
    auto res = db.fetchAllPS(
        "SELECT house_id FROM actuator_metadata WHERE actuator_id=?",
        {actuator_id});
    
    // 미등록 액추에이터에 대한 제어 요청은 무시하고 false 반환
    if (res.empty()) return false;

    // ── 2. 제어 이력 데이터베이스 로깅 ──
    // 누가(user_id), 어떤 기기에(actuator_id), 어떤 명령(command)을 보냈는지 기록
    db.executePS(
        "INSERT INTO control_logs (actuator_id, command, user_id, status) "
        "VALUES (?,?,?,'SUCCESS')",
        {actuator_id, command, user_id});

    // ── 3. MQTT 메시지 발행 (Publish) ──
    // 해당 액추에이터가 구독(Subscribe)하고 있을 토픽 조합
    std::string topic = "smartfarm/actuators/" + actuator_id + "/command";
    
    // MqttClient 싱글톤을 이용해 메시지 페이로드(command)를 발행
    // 내부적으로 QoS 1을 사용하여 적어도 한 번 전송을 보장함
    Core::MqttClient::getInstance().publish(topic, command);

    return true;
}

} // namespace Managers
