// AuthManager.h
#pragma once
#include <string>

class Database;

// Handles registration, authentication, and session validation.
class AuthManager {
public:
    explicit AuthManager(Database& db);
    bool registerUser(const std::string& username, const std::string& password);
    bool loginUser(const std::string& username, const std::string& password);

private:
    std::string hashPassword(const std::string& password) const;

    Database& database;
};