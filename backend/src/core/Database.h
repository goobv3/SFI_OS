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
