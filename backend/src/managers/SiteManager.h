/**
 * @file SiteManager.h
 * @brief 사이트(농장) 생성, 삭제, 조회 등 메타데이터 관리 매니저
 *
 * ▶ 역할
 *   - 여러 농장(Site)을 그룹화하고, 사이트 단위의 종합적인 현황(알람 갯수 등)을 조회합니다.
 *   - Site 아래에는 여러 개의 House(동/온실)가 포함됩니다.
 */
#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace Managers {
class SiteManager {
public:
    static SiteManager& getInstance();

    // ── 사이트 목록 및 현황 요약 ──
    /**
     * @brief 현재 등록된 모든 사이트 목록과, 포함된 하우스 수 및 활성 알람 수를 반환합니다.
     */
    nlohmann::json getSites();

    /**
     * @brief 특정 사이트 소속 센서들의 최근 10분간 평균 데이터를 종류별로 반환합니다.
     */
    nlohmann::json getSiteOverview(const std::string& site_id);

    /**
     * @brief 해당 사이트에 속한 하우스 목록을 반환합니다.
     */
    nlohmann::json getSiteHouses(const std::string& site_id);

    // ── 사이트 메타데이터 CRUD ──
    bool createSite(const std::string& site_id, const std::string& name, const std::string& location);
    bool updateSite(const std::string& site_id, const std::string& name, const std::string& location, const std::string& timezone);
    bool deleteSite(const std::string& site_id);

private:
    SiteManager() = default;
    ~SiteManager() = default;
    SiteManager(const SiteManager&) = delete;
    SiteManager& operator=(const SiteManager&) = delete;
};
}
