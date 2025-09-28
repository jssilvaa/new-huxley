// AuthManager.h
#pragma once
#include <string>

class Database;

class AuthManager {
public:
    AuthManager(Database* db);

    bool registerUser(const std::string& username, const std::string& password);
    bool loginUser(const std::string& username, const std::string& password);
    bool verifySession(const std::string& username);

private:
    Database* database;
};
