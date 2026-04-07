/**
 * @file SensorManager.cpp
 * @brief SensorManager 구현 — 센서 수신·저장·알람·룰 평가
 */
#include "SensorManager.h"
#include "RuleEngine.h"               // 자동화 룰 평가
#include "../core/Database.h"         // DB 접근
#include "../core/EmailSender.h"      // CRITICAL 알람 이메일
#include <iostream>

namespace Managers {

// ─────────────────────────────────────────────────────────────────────────────
// getInstance — Meyers' Singleton
// ─────────────────────────────────────────────────────────────────────────────
SensorManager& SensorManager::getInstance() {
    static SensorManager instance;
    return instance;
}

// ─────────────────────────────────────────────────────────────────────────────
// processIncomingData — 센서 데이터 처리 메인 진입점
//
// MQTT 콜백(main.cpp) 또는 HTTP POST (/api/sensors)에서 호출됩니다.
//
// 처리 분기:
//   ① sensor_metadata에 등록된 센서
//     → sensors 테이블에 INSERT (데이터 이력 저장)
//     → 임계값 경보 검사 (checkAlarms)
//     → 자동화 룰 평가 (RuleEngine::evaluate)
//
//   ② 미등록 센서 (새 기기 첫 수신)
//     → discovered_devices 테이블에 UPSERT (기기 자동 감지)
//     → 관리자가 ManageMode에서 확인 후 등록 가능
// ─────────────────────────────────────────────────────────────────────────────
void SensorManager::processIncomingData(const std::string& sensor_id, double value) {
    auto& db = Core::Database::getInstance();

    // ── 1. 등록 여부 확인 ──
    // sensor_id는 외부 입력(MQTT 토픽, HTTP 바디)이므로 Prepared Statement 사용 필수
    auto res = db.fetchAllPS(
        "SELECT house_id FROM sensor_metadata WHERE sensor_id=?",
        {sensor_id});

    // ── 2. 미등록 센서: discovered_devices에 기록 ──
    if (res.empty()) {
        // ON DUPLICATE KEY UPDATE: 이미 있으면 last_seen만 갱신
        db.executePS(
            "INSERT INTO discovered_devices (device_id, device_type, payload) VALUES (?,?,'') "
            "ON DUPLICATE KEY UPDATE last_seen=CURRENT_TIMESTAMP, payload=VALUES(payload)",
            {sensor_id, "sensor"});

        // payload(측정값)는 double이므로 별도 UPDATE로 안전하게 저장
        // (문자열로 변환하여 PS 바인딩)
        db.executePS(
            "UPDATE discovered_devices SET payload=? WHERE device_id=?",
            {std::to_string(value), sensor_id});
        return; // 미등록 센서는 이후 처리 없음
    }

    // ── 3. 등록 센서: house_id 확인 ──
    std::string house_id = res[0]["house_id"];

    // ── 4. 센서 측정값 저장 (sensors 테이블) ──
    // sensors 테이블의 timestamp 컬럼은 DEFAULT CURRENT_TIMESTAMP이므로 생략 가능
    db.executePS(
        "INSERT INTO sensors (sensor_id, house_id, value) VALUES (?,?,?)",
        {sensor_id, house_id, std::to_string(value)});

    // ── 5. 경보 검사 (임계값 비교) ──
    checkAlarms(house_id, sensor_id, value);

    // ── 6. 자동화 룰 평가 ──
    // 이 센서 ID에 연결된 automation_rules를 조회하여 조건 충족 시 액추에이터 제어
    Managers::RuleEngine::getInstance().evaluate(sensor_id, value);
}

// ─────────────────────────────────────────────────────────────────────────────
// checkAlarms — 임계값 비교 및 알람 생성
//
// 판정 우선순위 (높은 순):
//   CRITICAL_HIGH → WARNING_HIGH → CRITICAL_LOW → WARNING_LOW
//
// 알람 생성 후 CRITICAL 등급이면 이메일 전송도 수행합니다.
// ─────────────────────────────────────────────────────────────────────────────
void SensorManager::checkAlarms(const std::string& house_id,
                                const std::string& sensor_id,
                                double value)
{
    auto& db = Core::Database::getInstance();

    // 센서 임계값 및 별칭 조회 (is_active=1인 센서만)
    auto limits = db.fetchAllPS(
        "SELECT warn_high, warn_low, crit_high, crit_low, alias "
        "FROM sensor_metadata WHERE sensor_id=? AND is_active=1",
        {sensor_id});
    if (limits.empty()) return; // 비활성 센서 또는 설정 없음

    auto row = limits[0];
    std::string type  = "";   // 알람 등급 문자열
    std::string msg   = "";   // 알람 설명
    // alias가 비어있으면 sensor_id 사용 (이메일 본문에 표시)
    std::string alias = row["alias"].empty() ? sensor_id : row["alias"];

    try {
        // 판정 순서: CRITICAL_HIGH > WARNING_HIGH > CRITICAL_LOW > WARNING_LOW
        if (!row["crit_high"].empty() && value >= std::stod(row["crit_high"])) {
            type = "CRITICAL_HIGH"; msg = "Critical High limit exceeded";
        } else if (!row["warn_high"].empty() && value >= std::stod(row["warn_high"])) {
            type = "WARNING_HIGH";  msg = "Warning High limit exceeded";
        } else if (!row["crit_low"].empty() && value <= std::stod(row["crit_low"])) {
            type = "CRITICAL_LOW";  msg = "Critical Low limit exceeded";
        } else if (!row["warn_low"].empty() && value <= std::stod(row["warn_low"])) {
            type = "WARNING_LOW";   msg = "Warning Low limit exceeded";
        }
    } catch (...) {
        return; // 임계값 파싱 오류 시 무시
    }

    // 알람이 발생한 경우에만 처리
    if (!type.empty()) {
        // alarms 테이블에 알람 레코드 INSERT
        db.executePS(
            "INSERT INTO alarms (house_id, sensor_id, alarm_type, value, message) "
            "VALUES (?,?,?,?,?)",
            {house_id, sensor_id, type, std::to_string(value), msg});

        // CRITICAL 등급이면 이메일 알림 전송
        // "CRITICAL"이 포함된 알람 등급(CRITICAL_HIGH, CRITICAL_LOW)에 반응
        if (type.find("CRITICAL") != std::string::npos) {
            Core::EmailSender::sendAlert(alias, type, value, msg);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// getAlarms — 미확인 알람 목록 반환
//
// is_acknowledged=0 (미확인) 상태의 알람만 최신순으로 조회합니다.
// ─────────────────────────────────────────────────────────────────────────────
nlohmann::json SensorManager::getAlarms() {
    auto& db = Core::Database::getInstance();
    // 조건절에 외부 입력이 없으므로 fetchAll() 사용 가능
    auto results = db.fetchAll(
        "SELECT id, house_id, sensor_id, alarm_type, value, message, created_at "
        "FROM alarms WHERE is_acknowledged=0 ORDER BY created_at DESC");

    nlohmann::json arr = nlohmann::json::array();
    for (const auto& row : results) {
        nlohmann::json obj;
        for (const auto& p : row) obj[p.first] = p.second;
        arr.push_back(obj);
    }
    return arr;
}

// ─────────────────────────────────────────────────────────────────────────────
// resolveAlarm — 알람 확인 처리
//
// alarm_id는 HTTP 경로 파라미터(/api/alarms/<id>/acknowledge)에서 오므로
// 외부 입력으로 간주하여 Prepared Statement를 사용합니다.
// ─────────────────────────────────────────────────────────────────────────────
bool SensorManager::resolveAlarm(int alarm_id) {
    auto& db = Core::Database::getInstance();
    return db.executePS(
        "UPDATE alarms SET is_acknowledged=1 WHERE id=?",
        {std::to_string(alarm_id)});
}

} // namespace Managers
