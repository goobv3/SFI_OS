/**
 * @file RuleEngine.cpp
 * @brief RuleEngine 클래스 구현 — 자동화 룰 평가 및 CRUD
 */
#include "RuleEngine.h"
#include "ControlManager.h"
#include "../core/Database.h"
#include <iostream>
#include <ctime>
#include <cstring>

namespace Managers {

// ─────────────────────────────────────────────────────────────────────────────
// getInstance — Meyers' Singleton
// ─────────────────────────────────────────────────────────────────────────────
RuleEngine& RuleEngine::getInstance() {
    static RuleEngine instance;
    return instance;
}

// ─────────────────────────────────────────────────────────────────────────────
// evaluate: 센서값이 들어올 때마다 호출. 해당 sensor_id의 활성 룰을 평가.
//
// 동작 절차:
//  1. Trigger 조건이 주어진 sensor_id이고, is_enabled=1 인 룰 조회
//  2. 룰의 임계값(threshold_value)과 수신된 value를 condition_type(GTE, LT 등)에 맞춰 비교
//  3. 조건이 맞으면 쿨다운 검사 (현재시간 - last_triggered_at > cooldown_minutes)
//  4. 쿨다운이 지났으면 ControlManager를 통해 액추에이터 제어 명령 전송
//  5. 룰의 마지막 실행 시간(last_triggered_at) 갱신
// ─────────────────────────────────────────────────────────────────────────────
void RuleEngine::evaluate(const std::string& sensor_id, double value) {
    auto& db = Core::Database::getInstance();

    // 해당 센서의 is_enabled=true 룰 전체 조회
    auto rules = db.fetchAllPS(
        "SELECT id, condition_type, threshold_value, actuator_id, action_command, "
        "cooldown_minutes, last_triggered_at "
        "FROM automation_rules "
        "WHERE trigger_sensor_id=? AND is_enabled=1",
        {sensor_id});

    for (const auto& rule : rules) {
        // ── 조건 평가 ──
        double threshold = 0.0;
        try { threshold = std::stod(rule.at("threshold_value")); } catch (...) { continue; }

        const std::string& ctype = rule.at("condition_type");
        bool conditionMet = false;
        
        // condition_type에 따른 비교 연산
        if      (ctype == "GT")  conditionMet = (value >  threshold);
        else if (ctype == "LT")  conditionMet = (value <  threshold);
        else if (ctype == "GTE") conditionMet = (value >= threshold);
        else if (ctype == "LTE") conditionMet = (value <= threshold);

        if (!conditionMet) continue; // 조건 미달 시 다음 룰로

        // ── 쿨다운 체크 ──
        int cooldown_minutes = 5; // 기본값
        try { cooldown_minutes = std::stoi(rule.at("cooldown_minutes")); } catch (...) {}

        const std::string& last_triggered = rule.at("last_triggered_at");
        if (!last_triggered.empty()) {
            // last_triggered_at 파싱 (MySQL DATETIME: "YYYY-MM-DD HH:MM:SS")
            struct tm tm_val{};
#ifdef _WIN32
            sscanf_s(last_triggered.c_str(), "%d-%d-%d %d:%d:%d",
                &tm_val.tm_year, &tm_val.tm_mon, &tm_val.tm_mday,
                &tm_val.tm_hour, &tm_val.tm_min, &tm_val.tm_sec);
#else
            strptime(last_triggered.c_str(), "%Y-%m-%d %H:%M:%S", &tm_val);
#endif
            tm_val.tm_year -= 1900;
            tm_val.tm_mon  -= 1;
            tm_val.tm_isdst = -1;

            time_t last_time = mktime(&tm_val);
            time_t now_time  = std::time(nullptr);
            double elapsed_minutes = std::difftime(now_time, last_time) / 60.0;

            // 아직 쿨다운 시간이 지나지 않았으면 제어명령 스킵
            if (elapsed_minutes < static_cast<double>(cooldown_minutes)) {
                std::cout << "[RuleEngine] Rule " << rule.at("id")
                          << " is in cooldown (" << elapsed_minutes << " min elapsed)." << std::endl;
                continue;
            }
        }

        // ── 조건 통과 및 쿨다운 지남: 제어 명령 전송 ──
        const std::string& actuator_id    = rule.at("actuator_id");
        const std::string& action_command = rule.at("action_command");

        std::cout << "[RuleEngine] Rule " << rule.at("id")
                  << " fired → " << actuator_id << " : " << action_command << std::endl;

        ControlManager::getInstance().processControlCommand(actuator_id, action_command, "AutoRule");

        // ── last_triggered_at 업데이트 ──
        db.executePS(
            "UPDATE automation_rules SET last_triggered_at=NOW() WHERE id=?",
            {rule.at("id")});
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// getRules: 특정 하우스 또는 전체의 룰 목록을 반환
//
// LEFT JOIN을 이용해 센서 및 액추에이터의 alias(별명)를 같이 가져와 프론트엔드 표시에 씁니다.
// ─────────────────────────────────────────────────────────────────────────────
nlohmann::json RuleEngine::getRules(const std::string& house_id) {
    auto& db = Core::Database::getInstance();
    std::vector<std::map<std::string,std::string>> rows;

    if (house_id.empty()) {
        rows = db.fetchAll(
            "SELECT r.*, sm.alias AS sensor_alias, am.alias AS actuator_alias "
            "FROM automation_rules r "
            "LEFT JOIN sensor_metadata sm ON r.trigger_sensor_id = sm.sensor_id "
            "LEFT JOIN actuator_metadata am ON r.actuator_id = am.actuator_id "
            "ORDER BY r.id");
    } else {
        rows = db.fetchAllPS(
            "SELECT r.*, sm.alias AS sensor_alias, am.alias AS actuator_alias "
            "FROM automation_rules r "
            "LEFT JOIN sensor_metadata sm ON r.trigger_sensor_id = sm.sensor_id "
            "LEFT JOIN actuator_metadata am ON r.actuator_id = am.actuator_id "
            "WHERE r.house_id=? "
            "ORDER BY r.id",
            {house_id});
    }

    nlohmann::json arr = nlohmann::json::array();
    for (const auto& row : rows) {
        nlohmann::json obj;
        for (const auto& [k, v] : row) obj[k] = v;
        arr.push_back(obj);
    }
    return arr;
}

// ─────────────────────────────────────────────────────────────────────────────
// createRule: 새로운 룰 삽입
// ─────────────────────────────────────────────────────────────────────────────
nlohmann::json RuleEngine::createRule(const nlohmann::json& body) {
    auto& db = Core::Database::getInstance();

    std::string name              = body.value("name", "");
    std::string house_id          = body.value("house_id", "");
    std::string trigger_sensor_id = body.value("trigger_sensor_id", "");
    std::string condition_type    = body.value("condition_type", "GT");
    double threshold_value        = body.value("threshold_value", 0.0);
    std::string actuator_id       = body.value("actuator_id", "");
    std::string action_command    = body.value("action_command", "");
    int cooldown_minutes          = body.value("cooldown_minutes", 5);

    if (trigger_sensor_id.empty() || actuator_id.empty() || action_command.empty()) {
        return {{"status", "error"}, {"message", "Required fields missing"}};
    }

    bool ok = db.executePS(
        "INSERT INTO automation_rules "
        "(name, house_id, trigger_sensor_id, condition_type, threshold_value, "
        "actuator_id, action_command, cooldown_minutes) "
        "VALUES (?,?,?,?,?,?,?,?)",
        {name, house_id, trigger_sensor_id, condition_type,
         std::to_string(threshold_value), actuator_id, action_command,
         std::to_string(cooldown_minutes)});

    if (!ok) return {{"status", "error"}, {"message", "DB insert failed"}};

    // 새로 생성된 룰의 ID를 조회 (LAST_INSERT_ID)
    auto res = db.fetchAll("SELECT LAST_INSERT_ID() as id");
    std::string new_id = res.empty() ? "" : res[0].at("id");
    return {{"status", "created"}, {"id", new_id}};
}

// ─────────────────────────────────────────────────────────────────────────────
// updateRule: 룰 필드 수정사항에 대한 동적 쿼리 생성
// ─────────────────────────────────────────────────────────────────────────────
bool RuleEngine::updateRule(long long id, const nlohmann::json& body) {
    auto& db = Core::Database::getInstance();

    std::vector<std::string> parts;
    std::vector<std::string> vals;

    // 동적 쿼리 구성을 위한 람다 (문자열 및 숫자)
    auto addStr = [&](const char* col, const std::string& key) {
        if (body.contains(key)) {
            parts.push_back(std::string(col) + "=?");
            vals.push_back(body[key].get<std::string>());
        }
    };
    auto addNum = [&](const char* col, const std::string& key) {
        if (body.contains(key)) {
            parts.push_back(std::string(col) + "=?");
            if (body[key].is_number()) vals.push_back(std::to_string(body[key].get<double>()));
            else vals.push_back(body[key].get<std::string>());
        }
    };

    addStr("name",              "name");
    addStr("house_id",          "house_id");
    addStr("trigger_sensor_id", "trigger_sensor_id");
    addStr("condition_type",    "condition_type");
    addNum("threshold_value",   "threshold_value");
    addStr("actuator_id",       "actuator_id");
    addStr("action_command",    "action_command");
    addNum("cooldown_minutes",  "cooldown_minutes");

    if (parts.empty()) return true;

    std::string sql = "UPDATE automation_rules SET ";
    for (size_t i = 0; i < parts.size(); ++i) {
        sql += parts[i];
        if (i + 1 < parts.size()) sql += ",";
    }
    sql += " WHERE id=?";
    vals.push_back(std::to_string(id));

    return db.executePS(sql, vals);
}

// ─────────────────────────────────────────────────────────────────────────────
// deleteRule
// ─────────────────────────────────────────────────────────────────────────────
bool RuleEngine::deleteRule(long long id) {
    return Core::Database::getInstance().executePS(
        "DELETE FROM automation_rules WHERE id=?",
        {std::to_string(id)});
}

// ─────────────────────────────────────────────────────────────────────────────
// toggleRule: 룰의 활성 상태(is_enabled) 전환
// ─────────────────────────────────────────────────────────────────────────────
bool RuleEngine::toggleRule(long long id, bool enabled) {
    return Core::Database::getInstance().executePS(
        "UPDATE automation_rules SET is_enabled=? WHERE id=?",
        {enabled ? "1" : "0", std::to_string(id)});
}

} // namespace Managers
