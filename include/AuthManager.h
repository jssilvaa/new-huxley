// AuthManager.h
#pragma once
#include <string>

// Forward declaration
class Database; 

class AuthManager {
public:
    // Dependency-injected database pointer. AuthManager does NOT own the database.
    explicit AuthManager(Database* db);

    bool registerUser(const std::string& username, const std::string& password);
    bool loginUser(const std::string& username, const std::string& password);
    bool verifySession(const std::string& username);

private:
    Database* database { nullptr };
    std::string hashPassword(const std::string& password) const; // simple placeholder hash
};

