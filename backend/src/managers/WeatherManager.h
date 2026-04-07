/**
 * @file WeatherManager.h
 * @brief 농장 로컬 기상 및 기상청(KMA) 데이터 처리를 위한 매니저
 *
 * ▶ 역할
 *   - 기상청 단기/초단기 예보 API 통신 (HTTP)
 *   - 로컬 농장 기상센서 데이터 수신 및 DB 저장 (FARM 소스)
 *   - 실시간 관측 데이터와 미래의 예보 데이터를 병합하여 반환
 *
 * ▶ 외부 API 통신
 *   - 기상청 공공데이터 포털의 초단기/단기 예보 API를 cpr 라이브러리를 통해 직접 조회
 *   - convertGrid()를 통해 WGS84 위도/경도를 기상청 XY 격자로 변환하여 요청
 *
 * ▶ 관련 테이블
 *   - weather_data: FARM 소스로 수집되는 농장 로컬 기상 데이터 저장
 *
 * ▶ 커스터마이징 가이드
 *   1. 캐시 기능 추가: 외부 API 요청 횟수를 줄이기 위해 Redis 또는 메모리에 결과 캐싱
 *   2. 농업 기상 모델 연동: 단순 예보가 아닌 증발산량, 이슬점 연산 함수 등을 추가
 */
#pragma once
#include <nlohmann/json.hpp>

namespace Managers {

/**
 * @class WeatherManager
 * @brief 기상 데이터 관리 싱글톤 클래스
 */
class WeatherManager {
public:
    static WeatherManager& getInstance();

    /**
     * @brief 가장 최근의 기상 데이터를 조회합니다.
     * @param hours_ahead 예보 오프셋(0=실시간, 양수=미래 예보)
     * @return JSON 형태로 묶인 FARM 및 KMA 데이터 반환
     */
    nlohmann::json getLatestWeather(int hours_ahead);
    
    /**
     * @brief 농장 내 로컬 기상대 데이터를 DB에 기록합니다.
     * @param data JSON 형식의 날씨 데이터 (wind_speed, temperature 등 포함)
     */
    void recordFarmWeather(const nlohmann::json& data);

private:
    WeatherManager() = default;

    /**
     * @brief 기상청 초단기 예보 API를 호출하여 미래 날씨 데이터를 가져옵니다.
     * @param hours_ahead 현재 기준 몇 시간 뒤의 데이터를 가져올지 지정
     * @return 기상 파라미터가 정리된 JSON 객체
     */
    nlohmann::json getLiveForecast(int hours_ahead);

    /**
     * @brief 위경도를 기상청 격자 x, y로 변환합니다. (기상청 변환 공식)
     * @param lat 위도 (Latitude)
     * @param lon 경도 (Longitude)
     * @return std::pair<int, int> {X, Y} 격자 좌표
     */
    std::pair<int, int> convertGrid(double lat, double lon);
};
}
