// AuthManager.cpp
#include "AuthManager.h"

#include "DatabaseEngine.h"

// #include <algorithm>
// #include <array>
#include <chrono>
// #include <iomanip>
#include <iostream>
#include <random>
// #include <sstream>
#include <sodium.h> 

// namespace {
// std::string generateSalt()
// {
//     std::array<unsigned char, 16> salt{};
//     std::random_device rd;
//     for (auto& byte : salt) {
//         byte = static_cast<unsigned char>(rd());
//     }

//     std::ostringstream oss;
//     oss << std::hex;
//     for (unsigned char byte : salt) {
//         oss << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
//     }
//     return oss.str();
// }

// bool constantTimeEquals(const std::string& lhs, const std::string& rhs)
// {
//     if (lhs.size() != rhs.size()) {
//         return false;
//     }
//     unsigned char diff = 0;
//     for (std::size_t i = 0; i < lhs.size(); ++i) {
//         diff |= static_cast<unsigned char>(lhs[i] ^ rhs[i]);
//     }
//     return diff == 0;
// }
// } // namespace

AuthManager::AuthManager(Database& db)
    : database(db)
{
    pthread_mutex_init(&sessionMutex, nullptr);
}

AuthManager::~AuthManager()
{
    pthread_mutex_destroy(&sessionMutex);
}

std::string AuthManager::hashPassword(const std::string& password) const
{
    // Sodium.h hashing 
    char hash[crypto_pwhash_STRBYTES];
    if (crypto_pwhash_str(hash, password.c_str(), password.size(),
                          crypto_pwhash_OPSLIMIT_INTERACTIVE,
                          crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
        throw std::runtime_error("Password hashing failed");
    }
    return std::string(hash);
}

bool AuthManager::verifyPassword(const std::string& password, const std::string& storedHash) const
{
    return crypto_pwhash_str_verify(storedHash.c_str(), password.c_str(), password.size()) == 0;
}

bool AuthManager::registerUser(const std::string& username, const std::string& password)
{
    if (username.empty() || password.empty()) {
        return false;
    }
    const std::string hash = hashPassword(password);
    if (!database.insertUser(username, hash)) {
        return false;
    }
    database.logActivity("INFO", "Registered user: " + username);
    return true;
}

bool AuthManager::loginUser(const std::string& username, const std::string& password)
{
    std::string storedHash;
    if (!database.findUser(username, storedHash)) {
        return false;
    }

    if (!verifyPassword(password, storedHash)) {
        return false;
    }

    pthread_mutex_lock(&sessionMutex);
    activeUsers.insert(username);
    pthread_mutex_unlock(&sessionMutex);

    database.logActivity("INFO", "User login: " + username);
    return true;
}

void AuthManager::logoutUser(const std::string& username)
{
    pthread_mutex_lock(&sessionMutex);
    activeUsers.erase(username);
    pthread_mutex_unlock(&sessionMutex);

    database.logActivity("INFO", "User logout: " + username);
}

bool AuthManager::verifySession(const std::string& username) const
{
    pthread_mutex_lock(&sessionMutex);
    const bool active = activeUsers.find(username) != activeUsers.end();
    pthread_mutex_unlock(&sessionMutex);
    return active;
}
