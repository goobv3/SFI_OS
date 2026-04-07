/**
 * @file Database.cpp
 * @brief Database 클래스 구현 — MariaDB C API 기반 싱글톤 DB 래퍼
 *
 * ▶ 구현 패턴 설명
 *   - Meyers' Singleton: 정적 지역 변수를 사용하여 스레드-안전하게 단일 인스턴스를 생성합니다.
 *   - std::lock_guard<std::mutex>: 모든 DB 작업 함수 진입 시 뮤텍스를 획득하여 
 *     동시 접근으로 인한 데이터 레이스를 방지합니다.
 *   - Prepared Statement: MYSQL_STMT API를 사용하여 SQL Injection을 원천 차단합니다.
 *
 * ▶ 데이터 흐름
 *   [외부 호출] → connect() → MYSQL 핸들 생성 → mysql_real_connect()
 *   [쿼리 실행] → executePS(sql, params) → MYSQL_STMT 생성 → 파라미터 바인딩 → 실행
 *   [SELECT]   → fetchAllPS(sql, params) → 결과 바인딩 → 행별 map 변환 → 반환
 */
#include "Database.h"
#include <iostream>
#include <cstdlib>   // getenv, stoi
#include <cstring>   // memset

namespace Core {

// ─────────────────────────────────────────────────────────────────────────────
// getInstance — Meyers' Singleton
// 첫 호출 시 Database() 생성자를 통해 환경변수에서 접속 정보를 읽습니다.
// 이후 호출 시 동일 인스턴스를 반환합니다.
// ─────────────────────────────────────────────────────────────────────────────
Database& Database::getInstance() {
    static Database instance; // C++11 이후 초기화 스레드 안전 보장
    return instance;
}

// ─────────────────────────────────────────────────────────────────────────────
// Database() — 생성자
// 환경변수에서 DB 접속 정보를 읽습니다. 환경변수 미설정 시 docker-compose
// 기본값이 사용됩니다. 실제 연결은 connect()가 호출될 때 수행됩니다.
// ─────────────────────────────────────────────────────────────────────────────
Database::Database() : conn(nullptr) {
    // 환경변수 없으면 docker-compose 기본값 사용
    const char* env_host = std::getenv("DB_HOST");
    const char* env_user = std::getenv("DB_USER");
    const char* env_pass = std::getenv("DB_PASSWORD");
    const char* env_db   = std::getenv("DB_NAME");
    const char* env_port = std::getenv("DB_PORT");

    host   = env_host ? env_host : "sf_mariadb";
    user   = env_user ? env_user : "sf_user";
    pass   = env_pass ? env_pass : "sf_password";
    dbname = env_db   ? env_db   : "smartfarm_db";
    port   = env_port ? std::stoi(env_port) : 3306;
}

// ─────────────────────────────────────────────────────────────────────────────
// ~Database() — 소멸자
// 프로세스 종료 시 연결을 안전하게 닫습니다.
// ─────────────────────────────────────────────────────────────────────────────
Database::~Database() { disconnect(); }

// ─────────────────────────────────────────────────────────────────────────────
// connect() — DB 연결 수립
// 이미 연결되어 있으면 중복 연결을 시도하지 않고 즉시 true를 반환합니다.
// mysql_set_character_set으로 UTF-8(utf8mb4) 인코딩을 강제합니다.
// ─────────────────────────────────────────────────────────────────────────────
bool Database::connect() {
    std::lock_guard<std::mutex> lock(dbMutex); // 뮤텍스 획득 (범위 벗어나면 자동 해제)

    // 이미 연결된 경우 재연결 불필요
    if (conn) return true;

    // MYSQL 구조체 초기화
    conn = mysql_init(nullptr);
    if (!conn) return false;

    // 실제 TCP 연결 시도
    if (!mysql_real_connect(conn,
                            host.c_str(),   // 호스트
                            user.c_str(),   // 사용자명
                            pass.c_str(),   // 비밀번호
                            dbname.c_str(), // DB명
                            port,           // 포트 (기본 3306)
                            nullptr,        // 유닉스 소켓 (nullptr = TCP 사용)
                            0)) {           // 클라이언트 플래그 (없음)
        mysql_close(conn);
        conn = nullptr;
        return false;
    }

    // 한글 데이터 처리를 위한 UTF-8 캐릭터셋 설정
    mysql_set_character_set(conn, "utf8mb4");
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// disconnect() — DB 연결 종료
// ─────────────────────────────────────────────────────────────────────────────
void Database::disconnect() {
    std::lock_guard<std::mutex> lock(dbMutex);
    if (conn) {
        mysql_close(conn);
        conn = nullptr;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// isAlive() — DB 연결이 물리적으로 유효한지 확인 (ping)
// ─────────────────────────────────────────────────────────────────────────────
bool Database::isAlive() {
    std::lock_guard<std::mutex> lock(dbMutex);
    if (!conn) return false;
    return (mysql_ping(conn) == 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// execute() — 결과가 없는 일반 쿼리 실행 (외부 입력 不포함 시에만 사용)
//
// ⚠️ 주의: 이 함수는 SQL Injection 방어 기능이 없습니다.
//    외부 입력값(사용자 ID, URL 파라미터 등)이 포함된 쿼리에는 executePS()를 사용하세요.
// ─────────────────────────────────────────────────────────────────────────────
bool Database::execute(const std::string& query) {
    std::lock_guard<std::mutex> lock(dbMutex);
    if (!conn && !connect()) return false; // 연결 끊겼으면 재연결 시도
    if (mysql_query(conn, query.c_str())) return false;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// fetchAll() — 일반 쿼리로 결과 행 목록 반환 (외부 입력 不포함 시에만 사용)
//
// 각 행은 { "컬럼명": "값" } 형태의 map으로 반환됩니다.
// NULL 값은 빈 문자열("")로 대체됩니다.
//
// ⚠️ 주의: SQL Injection 방어 없음. 외부 입력 포함 시 fetchAllPS() 사용 필수.
// ─────────────────────────────────────────────────────────────────────────────
std::vector<std::map<std::string, std::string>> Database::fetchAll(const std::string& query) {
    std::lock_guard<std::mutex> lock(dbMutex);
    std::vector<std::map<std::string, std::string>> results;

    if (!conn && !connect()) return results;
    if (mysql_query(conn, query.c_str())) return results;

    // 결과셋 메모리에 적재
    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return results;

    int num_fields     = mysql_num_fields(res);  // 컬럼 수
    MYSQL_FIELD* fields = mysql_fetch_fields(res); // 컬럼 메타데이터 (이름 포함)

    MYSQL_ROW row;
    // 각 행을 이터레이션하며 map 으로 변환
    while ((row = mysql_fetch_row(res))) {
        std::map<std::string, std::string> row_map;
        for (int i = 0; i < num_fields; i++) {
            // NULL 컬럼은 빈 문자열로 처리 (row[i] == nullptr)
            row_map[fields[i].name] = row[i] ? row[i] : "";
        }
        results.push_back(row_map);
    }

    mysql_free_result(res); // 결과셋 메모리 해제 (반드시 필요)
    return results;
}

// ─────────────────────────────────────────────────────────────────────────────
// executePS() — Prepared Statement를 사용한 안전한 DML 실행
//
// 동작 순서:
//   1. MYSQL_STMT 핸들 생성 (mysql_stmt_init)
//   2. SQL 파싱 (mysql_stmt_prepare)
//   3. 파라미터 바인딩 — 모든 파라미터를 MYSQL_TYPE_STRING으로 처리
//   4. 쿼리 실행 (mysql_stmt_execute)
//   5. 리소스 해제 (mysql_stmt_close)
//
// ✅ SQL Injection 완전 차단: 파라미터가 이스케이프 처리되어 SQL 구조에 영향을 주지 않습니다.
// ─────────────────────────────────────────────────────────────────────────────
bool Database::executePS(const std::string& sql, const std::vector<std::string>& params) {
    std::lock_guard<std::mutex> lock(dbMutex);
    if (!conn && !connect()) return false;

    // 1. Prepared Statement 핸들 생성
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return false;

    // 2. SQL 파싱 (플레이스홀더 ? 개수와 params.size()가 일치해야 함)
    if (mysql_stmt_prepare(stmt, sql.c_str(), static_cast<unsigned long>(sql.size()))) {
        mysql_stmt_close(stmt);
        return false;
    }

    // 3. 파라미터 바인딩 (MYSQL_BIND 배열 구성)
    std::vector<MYSQL_BIND>       bind(params.size());
    std::vector<unsigned long>    lengths(params.size());
    std::memset(bind.data(), 0, sizeof(MYSQL_BIND) * params.size());

    for (std::size_t i = 0; i < params.size(); ++i) {
        lengths[i]            = static_cast<unsigned long>(params[i].size());
        bind[i].buffer_type   = MYSQL_TYPE_STRING;         // 문자열 타입으로 통일
        bind[i].buffer        = const_cast<char*>(params[i].c_str());
        bind[i].buffer_length = lengths[i];
        bind[i].length        = &lengths[i];
        bind[i].is_null       = nullptr; // NULL 아님
    }

    // 파라미터가 0개면 바인딩 생략 (WHERE 절 없는 쿼리 등)
    if (!params.empty() && mysql_stmt_bind_param(stmt, bind.data())) {
        mysql_stmt_close(stmt);
        return false;
    }

    // 4. 실행
    bool ok = (mysql_stmt_execute(stmt) == 0);
    mysql_stmt_close(stmt);
    return ok;
}

// ─────────────────────────────────────────────────────────────────────────────
// fetchAllPS() — Prepared Statement를 사용한 안전한 SELECT 실행
//
// 동작 순서:
//   1-4. executePS()와 동일 (stmt 생성 → 준비 → 입력 바인딩 → 실행)
//   5. 결과 메타데이터 조회 (mysql_stmt_result_metadata)
//   6. 출력 버퍼(512B/컬럼) 및 MYSQL_BIND 배열 구성
//   7. 결과 바인딩 (mysql_stmt_bind_result)
//   8. 결과 메모리 적재 (mysql_stmt_store_result)
//   9. 행별 fetch → map 변환
//  10. 리소스 해제
// ─────────────────────────────────────────────────────────────────────────────
std::vector<std::map<std::string, std::string>> Database::fetchAllPS(const std::string& sql, const std::vector<std::string>& params) {
    std::lock_guard<std::mutex> lock(dbMutex);
    std::vector<std::map<std::string, std::string>> results;
    if (!conn && !connect()) return results;

    // ── 1. Prepared Statement 초기화 ──
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return results;

    // ── 2. SQL 파싱 ──
    if (mysql_stmt_prepare(stmt, sql.c_str(), static_cast<unsigned long>(sql.size()))) {
        mysql_stmt_close(stmt);
        return results;
    }

    // ── 3. 입력 파라미터 바인딩 ──
    std::vector<MYSQL_BIND>    inBind(params.size());
    std::vector<unsigned long> inLengths(params.size());
    std::memset(inBind.data(), 0, sizeof(MYSQL_BIND) * params.size());

    for (std::size_t i = 0; i < params.size(); ++i) {
        inLengths[i]            = static_cast<unsigned long>(params[i].size());
        inBind[i].buffer_type   = MYSQL_TYPE_STRING;
        inBind[i].buffer        = const_cast<char*>(params[i].c_str());
        inBind[i].buffer_length = inLengths[i];
        inBind[i].length        = &inLengths[i];
        inBind[i].is_null       = nullptr;
    }

    if (!params.empty() && mysql_stmt_bind_param(stmt, inBind.data())) {
        mysql_stmt_close(stmt);
        return results;
    }

    // ── 4. 쿼리 실행 ──
    if (mysql_stmt_execute(stmt)) {
        mysql_stmt_close(stmt);
        return results;
    }

    // ── 5. 결과 컬럼 메타데이터 조회 ──
    MYSQL_RES* meta = mysql_stmt_result_metadata(stmt);
    if (!meta) { mysql_stmt_close(stmt); return results; }

    int num_fields      = mysql_num_fields(meta);
    MYSQL_FIELD* fields = mysql_fetch_fields(meta);

    // ── 6. 출력 버퍼 할당 (컬럼당 최대 512 바이트) ──
    // ⚠️ TEXT/BLOB 컬럼이 포함된 경우 BUF_SIZE를 늘려야 합니다.
    const unsigned long BUF_SIZE = 512;
    std::vector<std::vector<char>> buffers(num_fields, std::vector<char>(BUF_SIZE, 0));
    std::vector<unsigned long>     outLengths(num_fields, 0);
    std::vector<my_bool>           isNull(num_fields, 0);

    // ── 7. 출력 바인딩 구성 ──
    std::vector<MYSQL_BIND> outBind(num_fields);
    std::memset(outBind.data(), 0, sizeof(MYSQL_BIND) * num_fields);
    for (int i = 0; i < num_fields; ++i) {
        outBind[i].buffer_type   = MYSQL_TYPE_STRING;
        outBind[i].buffer        = buffers[i].data();
        outBind[i].buffer_length = BUF_SIZE;
        outBind[i].length        = &outLengths[i];
        outBind[i].is_null       = &isNull[i];
    }

    if (mysql_stmt_bind_result(stmt, outBind.data())) {
        mysql_free_result(meta);
        mysql_stmt_close(stmt);
        return results;
    }

    // ── 8. 전체 결과를 클라이언트 메모리에 적재 (이후 stmt_fetch로 순회) ──
    mysql_stmt_store_result(stmt);

    // ── 9. 행별 데이터 추출 ──
    while (mysql_stmt_fetch(stmt) == 0) {
        std::map<std::string, std::string> row_map;
        for (int i = 0; i < num_fields; ++i) {
            if (isNull[i]) {
                row_map[fields[i].name] = ""; // NULL → 빈 문자열
            } else {
                // outLengths[i] : 실제 데이터 바이트 수
                row_map[fields[i].name] = std::string(buffers[i].data(), outLengths[i]);
            }
        }
        results.push_back(row_map);
    }

    // ── 10. 리소스 해제 ──
    mysql_free_result(meta);
    mysql_stmt_close(stmt);
    return results;
}

} // namespace Core
