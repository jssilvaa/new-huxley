// AuthManager.cpp
#include "AuthManager.h"
#include "DatabaseEngine.h"
#include <iostream>
#include <sodium.h> 

/* TODOS 
1. Compare moderate vs interactive OPS / MEMLIMITS
2. Ensure sodium_init() is called once at program start (maybe in the main() function)
3. Delete commented-out code if not needed: libsodium generates its own salt internally.
There's also no need for the constantTimeEquals function since libsodium handles that securely.

*/

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
