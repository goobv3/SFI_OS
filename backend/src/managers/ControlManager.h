/**
 * @file ControlManager.h
 * @brief 액추에이터 제어 명령 처리 — MQTT 발행 + 로그 기록
 *
 * ▶ 역할
 *   - REST API(/api/control) 또는 RuleEngine에서 액추에이터 제어 요청이 오면
 *     MQTT 토픽으로 명령을 발행하고, DB에 제어 이력(control_logs)을 기록합니다.
 *
 * ▶ MQTT 발행 토픽 형식
 *   smartfarm/actuators/<actuator_id>/command
 *   페이로드 예: "ON", "OFF", "50%", "OPEN", "CLOSE"
 *
 * ▶ 데이터 흐름
 *   [HTTP POST /api/control] 또는 [RuleEngine::evaluate()]
 *        → processControlCommand(actuator_id, command, user_id)
 *        → actuator_metadata에서 actuator_id 유효성 검사
 *        → control_logs INSERT
 *        → MqttClient::publish("smartfarm/actuators/<id>/command", command)
 *
 * ▶ 커스터마이징 가이드
 *   1. HTTP REST 제어 추가: publish() 대신 HTTP 요청을 보내는 분기 추가
 *   2. 명령 유효성 검사: actuator_metadata.type에 따라 허용 명령 목록 검증
 *   3. 상태 피드백 수신: MQTT "smartfarm/actuators/<id>/status" 토픽 구독 추가
 *   4. 사용자 권한 검사: user_id 파라미터 활용, admin/operator 역할만 허용
 */
#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace Managers {

/**
 * @class ControlManager
 * @brief 액추에이터 제어 명령 처리 싱글톤 클래스
 */
class ControlManager {
public:
    /**
     * @brief 싱글톤 인스턴스를 반환합니다.
     * @return ControlManager& 전역 유일 인스턴스
     */
    static ControlManager& getInstance();

    /**
     * @brief 지정 액추에이터에 제어 명령을 전송합니다.
     * @param actuator_id 제어할 액추에이터 ID (예: "FAN_01", "PUMP_ZONE_A")
     * @param command     전송할 명령 페이로드 (예: "ON", "OFF", "50%")
     * @param user_id     명령 실행자 ID ("admin", "AutoRule" 등 — 로그용)
     * @return true  전송 성공 (actuator_metadata에 등록된 기기인 경우)
     * @return false 미등록 액추에이터이거나 MQTT 미연결
     *
     * @details
     * 1. actuator_metadata에서 actuator_id 조회 (미등록이면 false 반환)
     * 2. control_logs 테이블에 명령 이력 INSERT (status='SUCCESS')
     * 3. MQTT 토픽 "smartfarm/actuators/<id>/command"으로 command 발행
     */
    bool processControlCommand(const std::string& actuator_id,
                               const std::string& command,
                               const std::string& user_id);

private:
    ControlManager() = default;
    ~ControlManager() = default;
    ControlManager(const ControlManager&) = delete;
    ControlManager& operator=(const ControlManager&) = delete;
};

} // namespace Managers
