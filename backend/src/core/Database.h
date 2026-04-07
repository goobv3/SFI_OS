/**
 * @file Database.h
 * @brief MariaDB 연결 및 쿼리 실행을 담당하는 싱글톤 데이터베이스 클래스
 *
 * ▶ 역할
 *   - 애플리케이션 생명주기 동안 하나의 DB 커넥션을 유지하는 싱글톤 패턴을 구현합니다.
 *   - 일반 쿼리(fetchAll / execute)와 SQL Injection을 방지하는
 *     Prepared Statement(fetchAllPS / executePS) 두 가지 API를 제공합니다.
 *   - 멀티스레드 환경(Crow 웹서버)에서 안전하게 동작하도록 std::mutex로 DB 작업을 직렬화합니다.
 *
 * ▶ 환경변수
 *   DB_HOST     : MariaDB 호스트명 (기본값: sf_mariadb)
 *   DB_USER     : DB 사용자명    (기본값: sf_user)
 *   DB_PASSWORD : DB 비밀번호    (기본값: sf_password)
 *   DB_NAME     : DB 이름       (기본값: smartfarm_db)
 *   DB_PORT     : DB 포트       (기본값: 3306)
 *
 * ▶ 의존성
 *   - libmariadb (C API: mysql.h)
 *   - C++17 표준 라이브러리 (string, vector, map, mutex)
 *
 * ▶ 커스터마이징 가이드
 *   1. 커넥션 풀이 필요한 경우: getInstance()를 커넥션 풀 클래스로 교체합니다.
 *   2. 대용량 TEXT 컬럼 조회 시: fetchAllPS 내 BUF_SIZE(현재 512)를 늘려야 합니다.
 *   3. 다중 DB 지원 시: Database를 복수 인스턴스로 관리하는 팩토리 패턴 적용을 권장합니다.
 */
#pragma once
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <mysql.h>

namespace Core {

/**
 * @class Database
 * @brief MariaDB 싱글톤 래퍼 클래스
 *
 * Meyers' Singleton 패턴으로 구현되었으며,
 * 복사 생성자 및 대입 연산자가 삭제되어 단일 인스턴스를 보장합니다.
 */
class Database {
public:
    /**
     * @brief 싱글톤 인스턴스를 반환합니다.
     * @return Database& 전역 유일 인스턴스 참조
     * @note 처음 호출 시 생성자가 실행되어 환경변수에서 DB 설정을 읽습니다.
     */
    static Database& getInstance();

    /**
     * @brief MariaDB 서버에 연결합니다.
     * @return true  연결 성공 (또는 이미 연결됨)
     * @return false 연결 실패 (호스트 미응답, 인증 오류 등)
     * @note main.cpp에서 최대 10회 재시도 루프와 함께 호출됩니다.
     */
    bool connect();

    /**
     * @brief DB 연결을 종료합니다.
     * @note 소멸자에서도 자동 호출됩니다.
     */
    void disconnect();
    bool isAlive();

    /**
     * @brief 결과가 없는 DML/DDL 쿼리(INSERT, UPDATE, DELETE)를 실행합니다.
     * @param query 실행할 SQL 문자열 (외부 입력 불포함 권장)
     * @return true  성공, false 실패
     * @warning SQL Injection 위험! 외부 입력값이 포함된 경우 반드시 executePS()를 사용하세요.
     */
    bool execute(const std::string& query);

    /**
     * @brief SELECT 쿼리를 실행하고 결과를 반환합니다.
     * @param query 실행할 SQL 문자열
     * @return 행(map<컬럼명, 값>) 목록. NULL 컬럼은 빈 문자열("")로 처리됩니다.
     * @warning SQL Injection 위험! 외부 입력값이 포함된 경우 반드시 fetchAllPS()를 사용하세요.
     */
    std::vector<std::map<std::string, std::string>> fetchAll(const std::string& query);

    /**
     * @brief Prepared Statement를 사용하여 DML 쿼리를 안전하게 실행합니다.
     * @param sql    ? 플레이스홀더가 포함된 SQL 문자열
     * @param params ? 에 바인딩할 매개변수 목록 (순서대로 대응)
     * @return true  성공, false 실패
     *
     * @example
     * @code
     * db.executePS("INSERT INTO users (name, age) VALUES (?, ?)", {"홍길동", "30"});
     * @endcode
     */
    bool executePS(const std::string& sql, const std::vector<std::string>& params);

    /**
     * @brief Prepared Statement를 사용하여 SELECT 쿼리를 안전하게 실행하고 결과를 반환합니다.
     * @param sql    ? 플레이스홀더가 포함된 SQL 문자열
     * @param params ? 에 바인딩할 매개변수 목록
     * @return 행(map<컬럼명, 값>) 목록
     * @note 컬럼당 버퍼 크기는 512바이트입니다. TEXT 등 대용량 컬럼은 BUF_SIZE 조정이 필요합니다.
     *
     * @example
     * @code
     * auto rows = db.fetchAllPS("SELECT * FROM sensors WHERE house_id=?", {house_id});
     * @endcode
     */
    std::vector<std::map<std::string, std::string>> fetchAllPS(const std::string& sql, const std::vector<std::string>& params);

private:
    Database();                               ///< 환경변수에서 DB 설정 읽기
    ~Database();                              ///< disconnect() 호출
    Database(const Database&) = delete;       ///< 복사 금지
    Database& operator=(const Database&) = delete; ///< 대입 금지

    MYSQL*      conn;      ///< libmariadb C API 연결 핸들
    std::mutex  dbMutex;   ///< 멀티스레드 동시 접근 보호용 뮤텍스

    // DB 접속 정보 (환경변수로부터 초기화됨)
    std::string host;
    std::string user;
    std::string pass;
    std::string dbname;
    int         port;
};

} // namespace Core
