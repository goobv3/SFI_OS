/**
 * @file RuleEngine.h
 * @brief 자동화 룰 엔진 — 센서값 기반 액추에이터 자동 제어
 *
 * ▶ 역할
 *   - 센서값이 수신될 때마다 evaluate()를 호출하여 활성화된 자동화 룰을 평가합니다.
 *   - 조건이 충족되고 쿨다운(cooldown) 시간이 경과했으면 ControlManager를 통해
 *     액추에이터에 MQTT 명령을 전송합니다.
 *   - automation_rules 테이블의 CRUD 및 활성화/비활성화(toggle) 기능을 제공합니다.
 *
 * ▶ 룰 구조 (automation_rules 테이블)
 *   - trigger_sensor_id : 감시할 센서 ID
 *   - condition_type    : 조건 연산자 (GT / LT / GTE / LTE)
 *   - threshold_value   : 비교 임계값
 *   - actuator_id       : 조건 충족 시 제어할 액추에이터 ID
 *   - action_command    : 전송할 명령 (예: "ON", "OFF", "50%")
 *   - cooldown_minutes  : 연속 발동 방지 대기 시간 (분)
 *   - last_triggered_at : 마지막 발동 시각 (쿨다운 계산에 사용)
 *   - is_enabled        : 룰 활성화 여부
 *
 * ▶ 조건 연산자 정의
 *   - GT  : 초과 (value > threshold)
 *   - LT  : 미만 (value < threshold)
 *   - GTE : 이상 (value >= threshold)
 *   - LTE : 이하 (value <= threshold)
 *
 * ▶ 커스터마이징 가이드
 *   1. 복합 조건 룰: condition_type에 "AND", "OR" 로직 추가, 복수 센서 비교 지원
 *   2. 시간 조건 추가: active_start/active_end 컬럼 추가 후 evaluate()에서 시간 범위 검사
 *   3. 임계값 달성 지속 시간 조건: 최초 임계 초과 시각을 기록하여 지속 시간이 X분 이상일 때만 발동
 *   4. 사용자 알람 연동: evaluate()에서 룰 발동 시 alarms 테이블에 룰 관련 이벤트 기록
 */
#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace Managers {

/**
 * @class RuleEngine
 * @brief 자동화 룰 엔진 싱글톤 클래스
 */
class RuleEngine {
public:
    /**
     * @brief 싱글톤 인스턴스를 반환합니다.
     * @return RuleEngine& 전역 유일 인스턴스
     */
    static RuleEngine& getInstance();

    // ── 룰 엔진 핵심 ──────────────────────────────────────────────────────────

    /**
     * @brief 지정된 센서의 값으로 활성화된 모든 룰을 평가합니다.
     * @param sensor_id 측정값을 수신한 센서 ID
     * @param value     수신된 측정값
     *
     * @details 처리 흐름:
     *   1. DB에서 trigger_sensor_id=sensor_id, is_enabled=1인 룰 전체 조회
     *   2. 각 룰에 대해 condition_type/threshold_value로 조건 판정
     *   3. 쿨다운 검사 (last_triggered_at + cooldown_minutes > now)
     *   4. 조건 통과: ControlManager::processControlCommand() 호출
     *   5. last_triggered_at=NOW()로 갱신
     */
    void evaluate(const std::string& sensor_id, double value);

    // ── CRUD 메서드 ───────────────────────────────────────────────────────────

    /**
     * @brief 룰 목록을 조회합니다.
     * @param house_id 하우스 ID (빈 문자열이면 전체 룰 반환)
     * @return JSON 배열 [{id, name, trigger_sensor_id, condition_type, threshold_value,
     *                    actuator_id, action_command, cooldown_minutes, is_enabled,
     *                    sensor_alias, actuator_alias}, ...]
     */
    nlohmann::json getRules(const std::string& house_id);

    /**
     * @brief 새 자동화 룰을 생성합니다.
     * @param body JSON 객체 {name, house_id, trigger_sensor_id, condition_type,
     *                        threshold_value, actuator_id, action_command, cooldown_minutes}
     * @return JSON {status: "created", id: "<new_id>"} 또는 {status: "error", message: "..."}
     * @note trigger_sensor_id, actuator_id, action_command는 필수 항목입니다.
     */
    nlohmann::json createRule(const nlohmann::json& body);

    /**
     * @brief 기존 룰을 수정합니다.
     * @param id   수정할 룰의 PK (id 컬럼)
     * @param body JSON 객체 (수정할 필드만 포함 가능, 부분 업데이트 지원)
     * @return true 성공, false 실패
     */
    bool updateRule(long long id, const nlohmann::json& body);

    /**
     * @brief 룰을 삭제합니다.
     * @param id 삭제할 룰의 PK
     * @return true 성공, false 실패
     */
    bool deleteRule(long long id);

    /**
     * @brief 룰의 활성화/비활성화 상태를 전환합니다.
     * @param id      토글할 룰의 PK
     * @param enabled true=활성화, false=비활성화
     * @return true 성공, false 실패
     */
    bool toggleRule(long long id, bool enabled);

private:
    RuleEngine() = default;
    ~RuleEngine() = default;
    RuleEngine(const RuleEngine&) = delete;
    RuleEngine& operator=(const RuleEngine&) = delete;
};

} // namespace Managers
