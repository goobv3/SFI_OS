#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace Managers {
class SiteManager {
public:
    static SiteManager& getInstance();

    // ── 사이트 목록 및 현황 요약 ──
    nlohmann::json getSites();
    nlohmann::json getSiteOverview(const std::string& site_id);
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
