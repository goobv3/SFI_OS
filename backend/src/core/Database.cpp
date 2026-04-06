#include "Database.h"
#include <iostream>
#include <cstdlib>
#include <cstring>

namespace Core {
Database& Database::getInstance() {
    static Database instance;
    return instance;
}

Database::Database() : conn(nullptr) {
    const char* env_host = std::getenv("DB_HOST");
    const char* env_user = std::getenv("DB_USER");
    const char* env_pass = std::getenv("DB_PASSWORD");
    const char* env_db   = std::getenv("DB_NAME");
    const char* env_port = std::getenv("DB_PORT");
    host = env_host ? env_host : "sf_mariadb";
    user = env_user ? env_user : "sf_user";
    pass = env_pass ? env_pass : "sf_password";
    dbname = env_db ? env_db : "smartfarm_db";
    port = env_port ? std::stoi(env_port) : 3306;
}

Database::~Database() { disconnect(); }

bool Database::connect() {
    std::lock_guard<std::mutex> lock(dbMutex);
    if (conn) return true; 
    conn = mysql_init(nullptr);
    if (!conn) return false;
    if (!mysql_real_connect(conn, host.c_str(), user.c_str(), pass.c_str(), dbname.c_str(), port, nullptr, 0)) {
        mysql_close(conn);
        conn = nullptr;
        return false;
    }
    mysql_set_character_set(conn, "utf8mb4");
    return true;
}

void Database::disconnect() {
    std::lock_guard<std::mutex> lock(dbMutex);
    if (conn) {
        mysql_close(conn);
        conn = nullptr;
    }
}

bool Database::execute(const std::string& query) {
    std::lock_guard<std::mutex> lock(dbMutex);
    if (!conn && !connect()) return false;
    if (mysql_query(conn, query.c_str())) return false;
    return true;
}

std::vector<std::map<std::string, std::string>> Database::fetchAll(const std::string& query) {
    std::lock_guard<std::mutex> lock(dbMutex);
    std::vector<std::map<std::string, std::string>> results;
    if (!conn && !connect()) return results;
    if (mysql_query(conn, query.c_str())) return results;
    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return results;
    int num_fields = mysql_num_fields(res);
    MYSQL_FIELD* fields = mysql_fetch_fields(res);
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        std::map<std::string, std::string> row_map;
        for (int i = 0; i < num_fields; i++) {
            row_map[fields[i].name] = row[i] ? row[i] : "";
        }
        results.push_back(row_map);
    }
    mysql_free_result(res);
    return results;
}

bool Database::executePS(const std::string& sql, const std::vector<std::string>& params) {
    std::lock_guard<std::mutex> lock(dbMutex);
    if (!conn && !connect()) return false;

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return false;

    if (mysql_stmt_prepare(stmt, sql.c_str(), static_cast<unsigned long>(sql.size()))) {
        mysql_stmt_close(stmt);
        return false;
    }

    std::vector<MYSQL_BIND> bind(params.size());
    std::vector<unsigned long> lengths(params.size());
    std::memset(bind.data(), 0, sizeof(MYSQL_BIND) * params.size());

    for (std::size_t i = 0; i < params.size(); ++i) {
        lengths[i] = static_cast<unsigned long>(params[i].size());
        bind[i].buffer_type   = MYSQL_TYPE_STRING;
        bind[i].buffer        = const_cast<char*>(params[i].c_str());
        bind[i].buffer_length = lengths[i];
        bind[i].length        = &lengths[i];
        bind[i].is_null       = nullptr;
    }

    if (!params.empty() && mysql_stmt_bind_param(stmt, bind.data())) {
        mysql_stmt_close(stmt);
        return false;
    }

    bool ok = (mysql_stmt_execute(stmt) == 0);
    mysql_stmt_close(stmt);
    return ok;
}

std::vector<std::map<std::string, std::string>> Database::fetchAllPS(const std::string& sql, const std::vector<std::string>& params) {
    std::lock_guard<std::mutex> lock(dbMutex);
    std::vector<std::map<std::string, std::string>> results;
    if (!conn && !connect()) return results;

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return results;

    if (mysql_stmt_prepare(stmt, sql.c_str(), static_cast<unsigned long>(sql.size()))) {
        mysql_stmt_close(stmt);
        return results;
    }

    // ── 입력 파라미터 바인딩 ──
    std::vector<MYSQL_BIND> inBind(params.size());
    std::vector<unsigned long> inLengths(params.size());
    std::memset(inBind.data(), 0, sizeof(MYSQL_BIND) * params.size());

    for (std::size_t i = 0; i < params.size(); ++i) {
        inLengths[i] = static_cast<unsigned long>(params[i].size());
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

    if (mysql_stmt_execute(stmt)) {
        mysql_stmt_close(stmt);
        return results;
    }

    MYSQL_RES* meta = mysql_stmt_result_metadata(stmt);
    if (!meta) { mysql_stmt_close(stmt); return results; }

    int num_fields = mysql_num_fields(meta);
    MYSQL_FIELD* fields = mysql_fetch_fields(meta);

    // ── 출력 버퍼: 컬럼당 최대 512 바이트 (TEXT 등 대용량 컬럼 필요시 조정) ──
    const unsigned long BUF_SIZE = 512;
    std::vector<std::vector<char>> buffers(num_fields, std::vector<char>(BUF_SIZE, 0));
    std::vector<unsigned long> outLengths(num_fields, 0);
    std::vector<my_bool> isNull(num_fields, 0);

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

    mysql_stmt_store_result(stmt);

    while (mysql_stmt_fetch(stmt) == 0) {
        std::map<std::string, std::string> row_map;
        for (int i = 0; i < num_fields; ++i) {
            if (isNull[i]) {
                row_map[fields[i].name] = "";
            } else {
                row_map[fields[i].name] = std::string(buffers[i].data(), outLengths[i]);
            }
        }
        results.push_back(row_map);
    }

    mysql_free_result(meta);
    mysql_stmt_close(stmt);
    return results;
}
}
