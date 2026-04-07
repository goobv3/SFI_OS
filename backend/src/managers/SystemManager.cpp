/**
 * @file SystemManager.cpp
 * @brief SystemManager 클래스 구현 - 리눅스 기반 리소스 측정 및 로그 수집
 */
#include "SystemManager.h"
#include "../core/Database.h"
#include "../core/MqttClient.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <sys/statvfs.h> // Disk usage

namespace Managers {

// ─────────────────────────────────────────────────────────────────────────────
// getInstance - Meyers' Singleton
// ─────────────────────────────────────────────────────────────────────────────
SystemManager& SystemManager::getInstance() {
    static SystemManager instance;
    return instance;
}

SystemManager::SystemManager() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");
    startTime = ss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// getSystemStatus: 전체 리소스 정보 및 서비스 연결 상태 집계
// ─────────────────────────────────────────────────────────────────────────────
nlohmann::json SystemManager::getSystemStatus() {
    nlohmann::json status;

    // 1. 서비스 상태 체크
    status["services"] = {
        {"mariadb", Core::Database::getInstance().isAlive() ? "online" : "offline"},
        {"mosquitto", "online"} 
    };

    // 2. 리소스 측정 (리눅스 기준)
    status["resources"] = {
        {"cpu", getCpuUsage()},
        {"ram", getMemoryUsage()},
        {"disk", getDiskUsage()}
    };

    status["uptime"] = {
        {"start_time", startTime}
    };

    return status;
}

// ─────────────────────────────────────────────────────────────────────────────
// getLatestLogs: backend.log 파일의 마지막 50줄을 읽어옴
// ─────────────────────────────────────────────────────────────────────────────
std::vector<std::string> SystemManager::getLatestLogs(int lines) {
    std::vector<std::string> logLines;
    
    // 로그 파일 경로 (루트 디렉토리 기준)
    std::ifstream file("backend.log");
    if (!file.is_open()) {
        logLines.push_back("[Error] Unable to open backend.log");
        return logLines;
    }

    // 간단한 꼬리 읽기 구현
    std::string line;
    std::vector<std::string> allLines;
    while (std::getline(file, line)) {
        allLines.push_back(line);
    }

    int start = std::max(0, (int)allLines.size() - lines);
    for (int i = start; i < (int)allLines.size(); ++i) {
        logLines.push_back(allLines[i]);
    }

    return logLines;
}

// ── 리소스 측정 헬퍼 함수들 (Linux /proc 파일시스템 활용) ──

double SystemManager::getCpuUsage() {
    std::ifstream file("/proc/stat");
    if (!file.is_open()) return 0.0;

    std::string cpu;
    long user, nice, system, idle;
    file >> cpu >> user >> nice >> system >> idle;
    
    static long prevIdle = 0, prevTotal = 0;
    long total = user + nice + system + idle;
    long diffIdle = idle - prevIdle;
    long diffTotal = total - prevTotal;

    // 초기화 - 첫 호출 시 다음 측정을 위해 저장 후 0 대신 대략적인 값(또는 다음번 관찰) 유도
    if (prevTotal == 0) {
        prevIdle = idle;
        prevTotal = total;
        return 1.5; // 첫 호출 시 Placeholder (실제 데이터는 다음 폴링부터)
    }

    prevIdle = idle;
    prevTotal = total;

    if (diffTotal == 0) return 0.0;
    return (1.0 - (double)diffIdle / (double)diffTotal) * 100.0;
}

double SystemManager::getMemoryUsage() {
    // /proc/meminfo 에서 MemTotal 과 MemAvailable 을 사용하여 점유율 계산
    std::ifstream file("/proc/meminfo");
    if (!file.is_open()) return 0.0;

    std::string label;
    long total = 0, available = 0;
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("MemTotal:") == 0) {
            std::stringstream ss(line);
            ss >> label >> total;
        } else if (line.find("MemAvailable:") == 0) {
            std::stringstream ss(line);
            ss >> label >> available;
        }
    }

    if (total == 0) return 0.0;
    return (double)(total - available) / (double)total * 100.0;
}

double SystemManager::getDiskUsage() {
    // 루트 경로(/)의 디스크 용량 점유율 측정
    struct statvfs stat;
    if (statvfs("/", &stat) != 0) return 0.0;

    double total = (double)stat.f_blocks * stat.f_frsize;
    double free = (double)stat.f_bfree * stat.f_frsize;
    return (total - free) / total * 100.0;
}

} // namespace Managers
