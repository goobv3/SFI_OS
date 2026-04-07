/**
 * @file SiteManager.cpp
 * @brief SiteManager 클래스 구현
 *
 * ▶ 복합 쿼리 설명
 *   - getSites() 에서는 LEFT JOIN 과 서브쿼리를 사용해 하우스 수와 액티브 알람 갯수를 사이트 단위로 집계합니다.
 *   - getSiteOverview() 에서는 최근 10분간의 센서 데이터 평균을 도출합니다.
 */
#include "SiteManager.h"
#include "../core/Database.h"
#include <iostream>

namespace Managers {

SiteManager& SiteManager::getInstance() {
    static SiteManager instance;
    return instance;
}

// ─────────────────────────────────────────────────────────────────────────────
// getSites: 사이트 단위 집계 현황 반환 (하우스 개수, 알람 개수 포함)
// ─────────────────────────────────────────────────────────────────────────────
nlohmann::json SiteManager::getSites() {
    auto& db = Core::Database::getInstance();
    std::string sql = R"(
        SELECT s.site_id, s.name, s.location, s.timezone, s.created_at,
               COUNT(DISTINCT h.house_id) as house_count,
               (SELECT COUNT(*) FROM alarms a 
                JOIN sensor_metadata sm ON a.sensor_id = sm.sensor_id 
                JOIN houses h2 ON sm.house_id = h2.house_id 
                WHERE h2.site_id = s.site_id AND a.is_acknowledged = FALSE) as active_alarm_count
        FROM sites s
        LEFT JOIN houses h ON s.site_id = h.site_id
        GROUP BY s.site_id
        ORDER BY s.created_at ASC
    )";
    auto rows = db.fetchAllPS(sql, {});
    nlohmann::json result = nlohmann::json::array();
    for (const auto& row : rows) {
        nlohmann::json site = {
            {"site_id", row.at("site_id")},
            {"name", row.at("name")},
            {"location", row.at("location")},
            {"timezone", row.at("timezone")},
            {"created_at", row.at("created_at")}
        };
        try { site["house_count"] = std::stoi(row.at("house_count")); } catch (...) { site["house_count"] = 0; }
        try { site["active_alarm_count"] = std::stoi(row.at("active_alarm_count")); } catch (...) { site["active_alarm_count"] = 0; }
        result.push_back(site);
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// getSiteOverview: 사이트에 속한 각 센서 타입(온도, 습도 등)의 최근 평균 반환
// ─────────────────────────────────────────────────────────────────────────────
nlohmann::json SiteManager::getSiteOverview(const std::string& site_id) {
    auto& db = Core::Database::getInstance();
    std::string sql = R"(
        SELECT sm.type, AVG(sd.value) as avg_value
        FROM houses h
        JOIN sensor_metadata sm ON h.house_id = sm.house_id
        JOIN (
            SELECT sensor_id, value 
            FROM sensor_data 
            WHERE created_at >= NOW() - INTERVAL 10 MINUTE
        ) sd ON sm.sensor_id = sd.sensor_id
        WHERE h.site_id = ?
        GROUP BY sm.type
    )";
    auto rows = db.fetchAllPS(sql, {site_id});
    nlohmann::json overview = nlohmann::json::object();
    for (const auto& row : rows) {
        std::string type = row.at("type");
        try {
            overview[type] = std::stod(row.at("avg_value"));
        } catch (...) {
            overview[type] = 0.0;
        }
    }
    return overview;
}

// ─────────────────────────────────────────────────────────────────────────────
// getSiteHouses: 사이트 소속 하우스 목록 반환
// ─────────────────────────────────────────────────────────────────────────────
nlohmann::json SiteManager::getSiteHouses(const std::string& site_id) {
    auto& db = Core::Database::getInstance();
    std::string sql = "SELECT house_id, name, display_order FROM houses WHERE site_id = ? ORDER BY display_order ASC, name ASC";
    auto rows = db.fetchAllPS(sql, {site_id});
    nlohmann::json result = nlohmann::json::array();
    for (const auto& row : rows) {
        nlohmann::json house = {
            {"house_id", row.at("house_id")},
            {"name", row.at("name")}
        };
        try { house["display_order"] = std::stoi(row.at("display_order")); } catch (...) { house["display_order"] = 0; }
        result.push_back(house);
    }
    return result;
}

// ── 단순 CRUD (생성, 수정, 삭제) ──
bool SiteManager::createSite(const std::string& site_id, const std::string& name, const std::string& location) {
    auto& db = Core::Database::getInstance();
    std::string sql = "INSERT INTO sites (site_id, name, location) VALUES (?, ?, ?)";
    return db.executePS(sql, {site_id, name, location});
}

bool SiteManager::updateSite(const std::string& site_id, const std::string& name, const std::string& location, const std::string& timezone) {
    auto& db = Core::Database::getInstance();
    std::string sql = "UPDATE sites SET name = ?, location = ?, timezone = ? WHERE site_id = ?";
    return db.executePS(sql, {name, location, timezone, site_id});
}

bool SiteManager::deleteSite(const std::string& site_id) {
    auto& db = Core::Database::getInstance();
    return db.executePS("DELETE FROM sites WHERE site_id = ?", {site_id});
}

}
