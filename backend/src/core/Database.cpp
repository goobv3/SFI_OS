#include "Database.h"
#include <iostream>
#include <cstdlib>

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
}
