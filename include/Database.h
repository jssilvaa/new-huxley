// Database.h
#pragma once
#include <string>

class Database {
public:
    Database(const std::string& filename);
    ~Database();

    bool insertUser(const std::string& username, const std::string& hash);
    bool findUser(const std::string& username, std::string& outHash);
    bool logActivity(const std::string& message);

private:
    void* dbHandle; // will be sqlite3*
};
