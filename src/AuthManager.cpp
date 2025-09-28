// AuthManager.cpp
#include "AuthManager.h"
#include "Database.h"
#include <iostream>

AuthManager::AuthManager(Database* db) : database(db) {}

std::string AuthManager::hashPassword(const std::string& password) const {
    // VERY weak placeholder hash. Replace with proper salted hash (e.g., bcrypt / Argon2) later.
    return "hashed_" + password;
}

bool AuthManager::registerUser(const std::string& username, const std::string& password) {
    if (!database) {
        std::cerr << "AuthManager error: database not set" << std::endl;
        return false;
    }
    const std::string hash = hashPassword(password);
    if (database->insertUser(username, hash)) {
        database->logActivity("User registered: " + username);
        return true;
    }
    return false;
}

bool AuthManager::loginUser(const std::string& username, const std::string& password) {
    if (!database) {
        std::cerr << "AuthManager error: database not set" << std::endl;
        return false;
    }
    std::string storedHash;
    if (database->findUser(username, storedHash)) {
        const std::string hash = hashPassword(password);
        if (hash == storedHash) {
            database->logActivity("User logged in: " + username);
            return true;
        }
    }
    return false;
}

bool AuthManager::verifySession(const std::string& username) {
    if (!database) return false;
    std::string storedHash;
    // For now: session validity == user exists.
    return database->findUser(username, storedHash);
}
