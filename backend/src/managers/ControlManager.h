#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace Managers {
class ControlManager {
public:
    static ControlManager& getInstance();
    bool processControlCommand(const std::string& actuator_id, const std::string& command, const std::string& user_id);
private:
    ControlManager() = default;
    ~ControlManager() = default;
};
}
