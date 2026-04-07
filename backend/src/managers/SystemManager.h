/**
 * @file SystemManager.h
 * @brief 서버 리소스(CPU, RAM, Disk) 및 서비스 상태 모니터링 매니저
 */
#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace Managers {

/**
 * @class SystemManager
 * @brief 서버 상태 모니터링 싱글톤 매니저
 */
class SystemManager {
public:
    static SystemManager& getInstance();

    /**
     * @brief 전체 시스템 상태(CPU, RAM, DB/MQTT 연결 등)를 JSON으로 반환합니다.
     */
    nlohmann::json getSystemStatus();

    /**
     * @brief 최근 N줄의 서버 로그를 가져옵니다.
     * @param lines 가져올 로그 라인 수
     */
    std::vector<std::string> getLatestLogs(int lines = 50);

    /**
     * @brief 서버 시작 시각(Uptime 계산용)
     */
    std::string getStartTime() const { return startTime; }

private:
    SystemManager();
    ~SystemManager() = default;
    SystemManager(const SystemManager&) = delete;
    SystemManager& operator=(const SystemManager&) = delete;

    std::string startTime;

    // 리소스 측정용 헬퍼
    double getCpuUsage();
    double getMemoryUsage();
    double getDiskUsage();
};

} // namespace Managers
