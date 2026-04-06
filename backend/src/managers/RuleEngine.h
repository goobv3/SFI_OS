#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace Managers {

/**
 * @class RuleEngine
 * @brief 자동화 룰 엔진 (싱글톤)
 *
 * 센서 값이 수신될 때마다 evaluate()를 호출합니다.
 * 룰 조건이 충족되고 쿨다운이 지났으면 ControlManager를 통해 액추에이터 명령을 전송합니다.
 */
class RuleEngine {
public:
    static RuleEngine& getInstance();

    // ── 룰 엔진 핵심 ──
    void evaluate(const std::string& sensor_id, double value);

    // ── CRUD ──
    nlohmann::json getRules(const std::string& house_id);
    nlohmann::json createRule(const nlohmann::json& body);
    bool updateRule(long long id, const nlohmann::json& body);
    bool deleteRule(long long id);
    bool toggleRule(long long id, bool enabled);

private:
    RuleEngine() = default;
    ~RuleEngine() = default;
    RuleEngine(const RuleEngine&) = delete;
    RuleEngine& operator=(const RuleEngine&) = delete;
};

} // namespace Managers
