// AuthManager.h
#pragma once

#include <string>
#include <unordered_set>
#include <pthread.h>

class Database;

// Handles registration, authentication, and session validation.
class AuthManager {
public:
    explicit AuthManager(Database& db);
    ~AuthManager();

    bool registerUser(const std::string& username, const std::string& password);
    bool loginUser(const std::string& username, const std::string& password);
    bool logoutUser(const std::string& username);
    bool verifySession(const std::string& username) const;

private:
    std::string hashPassword(const std::string& password) const;
    bool verifyPassword(const std::string& password, const std::string& storedHash) const;

    Database& database;
    mutable pthread_mutex_t sessionMutex;
    std::unordered_set<std::string> activeUsers;
};