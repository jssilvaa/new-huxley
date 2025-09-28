#include "Database.h"
#include <sqlite3.h>
#include <iostream>

Database::Database(const std::string& filename) : dbHandle(nullptr) {
    if (sqlite3_open(filename.c_str(), reinterpret_cast<sqlite3**>(&dbHandle)) != SQLITE_OK) {
        std::cerr << "Error opening DB: " << sqlite3_errmsg(reinterpret_cast<sqlite3*>(dbHandle)) << "\n";
        dbHandle = nullptr;
    } else {
        // Initialize schema
        const char* createUsers =
            "CREATE TABLE IF NOT EXISTS users ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "username TEXT UNIQUE NOT NULL, "
            "password TEXT NOT NULL);";

        const char* createLogs =
            "CREATE TABLE IF NOT EXISTS logs ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "message TEXT NOT NULL, "
            "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);";

        char* errMsg = nullptr;
        if (sqlite3_exec(reinterpret_cast<sqlite3*>(dbHandle), createUsers, nullptr, nullptr, &errMsg) != SQLITE_OK) {
            std::cerr << "Error creating users table: " << errMsg << "\n";
            sqlite3_free(errMsg);
        }
        if (sqlite3_exec(reinterpret_cast<sqlite3*>(dbHandle), createLogs, nullptr, nullptr, &errMsg) != SQLITE_OK) {
            std::cerr << "Error creating logs table: " << errMsg << "\n";
            sqlite3_free(errMsg);
        }
    }
}

Database::~Database() {
    if (dbHandle) {
        sqlite3_close(reinterpret_cast<sqlite3*>(dbHandle));
    }
}

bool Database::insertUser(const std::string& username, const std::string& hash) {
    const char* sql = "INSERT INTO users (username, password) VALUES (?, ?);";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(reinterpret_cast<sqlite3*>(dbHandle), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    return success;
}

bool Database::findUser(const std::string& username, std::string& outHash) {
    const char* sql = "SELECT password FROM users WHERE username = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(reinterpret_cast<sqlite3*>(dbHandle), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* text = sqlite3_column_text(stmt, 0);
        if (text) {
            outHash = reinterpret_cast<const char*>(text);
            found = true;
        }
    }
    sqlite3_finalize(stmt);
    return found;
}

bool Database::logActivity(const std::string& message) {
    const char* sql = "INSERT INTO logs (message) VALUES (?);";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(reinterpret_cast<sqlite3*>(dbHandle), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, message.c_str(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    return success;
}
