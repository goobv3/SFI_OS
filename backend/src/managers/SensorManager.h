/**
 * @file SensorManager.h
 * @brief 센서 데이터 수신·저장·경보 판정을 담당하는 매니저 싱글톤
 *
 * ▶ 역할
 *   1. MQTT 또는 HTTP POST로 센서 데이터가 수신될 때 processIncomingData()를 실행합니다.
 *   2. 등록된 센서이면 sensor_data 테이블에 값을 저장합니다.
 *   3. 미등록 센서이면 discovered_devices 테이블에 자동 기록합니다(자동 감지 기능).
 *   4. checkAlarms()로 warn/crit 임계값과 비교하여 알람을 생성합니다.
 *   5. CRITICAL 알람 발생 시 EmailSender::sendAlert()를 호출합니다.
 *   6. RuleEngine::evaluate()를 호출하여 자동화 룰을 평가합니다.
 *
 * ▶ 데이터 흐름 (센서값 수신 시)
 *   MQTT 브로커 → main.cpp 콜백 → processIncomingData()
 *              → [등록 센서] sensor_data 저장 → checkAlarms() → RuleEngine.evaluate()
 *              → [미등록]   discovered_devices 기록
 *
 * ▶ 알람 등급
 *   - WARNING_HIGH  : 경고 상한값 초과
 *   - WARNING_LOW   : 경고 하한값 미달
 *   - CRITICAL_HIGH : 위험 상한값 초과 (이메일 발송)
 *   - CRITICAL_LOW  : 위험 하한값 미달 (이메일 발송)
 *
 * ▶ 커스터마이징 가이드
 *   - 알람 방법 추가(SMS, 슬랙 등): checkAlarms() 내 CRITICAL 분기에 추가 알림 호출
 *   - 알람 중복 방지: 동일 알람의 acknowledge 여부를 확인하는 로직 추가 가능
 *   - 센서 타입별 처리: processIncomingData()에서 타입을 조회하여 분기 처리
 */
#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace Managers {

/**
 * @class SensorManager
 * @brief 센서 데이터 수신·경보 관리 싱글톤 클래스
 */
class SensorManager {
public:
    /**
     * @brief 싱글톤 인스턴스를 반환합니다.
     * @return SensorManager& 전역 유일 인스턴스
     */
    static SensorManager& getInstance();

    /**
     * @brief MQTT 또는 HTTP를 통해 수신된 센서 데이터를 처리합니다.
     * @param sensor_id 센서 ID (예: "TEMP_01", "HUM_B2")
     * @param value     측정값 (부동소수점)
     *
     * @details
     * 1. sensor_metadata 테이블에서 sensor_id 조회
     * 2. 미등록 센서: discovered_devices 자동 등록 후 반환
     * 3. 등록 센서: sensors 테이블에 INSERT
     * 4. checkAlarms() 호출
     * 5. RuleEngine::evaluate() 호출 (자동화 룰 평가)
     */
    void processIncomingData(const std::string& sensor_id, double value);

    /**
     * @brief 미확인(is_acknowledged=0) 알람 전체 목록을 반환합니다.
     * @return JSON 배열 [{id, house_id, sensor_id, alarm_type, value, message, created_at}, ...]
     */
    nlohmann::json getAlarms();

    /**
     * @brief 지정 알람을 확인 처리(acknowledged)합니다.
     * @param alarm_id 알람 PK (id 컬럼)
     * @return true 성공, false 실패
     */
    bool resolveAlarm(int alarm_id);

private:
    SensorManager() = default;
    ~SensorManager() = default;
    SensorManager(const SensorManager&) = delete;
    SensorManager& operator=(const SensorManager&) = delete;

    /**
     * @brief 수신된 센서값을 임계값과 비교하여 알람을 생성합니다.
     * @param house_id  센서가 속한 하우스 ID
     * @param sensor_id 센서 ID
     * @param value     측정값
     *
     * @details 판정 우선순위: CRITICAL_HIGH > WARNING_HIGH > CRITICAL_LOW > WARNING_LOW
     *          CRITICAL 알람 발생 시 Core::EmailSender::sendAlert() 추가 호출
     */
    void checkAlarms(const std::string& house_id, const std::string& sensor_id, double value);
};

} // namespace Managers
