#pragma once
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <mysql.h>

namespace Core {
class Database {
public:
    static Database& getInstance();
    bool connect();
    void disconnect();
    bool execute(const std::string& query);
    std::vector<std::map<std::string, std::string>> fetchAll(const std::string& query);
    bool executePS(const std::string& sql, const std::vector<std::string>& params);
    std::vector<std::map<std::string, std::string>> fetchAllPS(const std::string& sql, const std::vector<std::string>& params);
private:
    Database();
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    MYSQL* conn;
    std::mutex dbMutex;
    std::string host, user, pass, dbname;
    int port;
};
}
