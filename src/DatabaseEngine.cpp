#include "DatabaseEngine.h"

#include <sqlite3.h>

#include <iostream>
#include <utility>

namespace {
bool exec(sqlite3* db, const char* sql)
{
    char* errMsg = nullptr;
    const int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        if (errMsg) {
            std::cerr << "SQLite error: " << errMsg << std::endl;
            sqlite3_free(errMsg);
        }
        return false;
    }
    return true;
}
} // namespace

Database::Database(const std::string& filename)
    : dbHandle(nullptr)
    , dbPath(filename)
{
}

Database::~Database()
{
    close();
}

bool Database::open()
{
    if (dbHandle) {
        return true;
    }

    if (sqlite3_open(dbPath.c_str(), &dbHandle) != SQLITE_OK) {
        std::cerr << "Failed to open database: " << sqlite3_errmsg(dbHandle) << std::endl;
        dbHandle = nullptr;
        return false;
    }

    if (!configurePragmas()) {
        return false;
    }
    if (!ensureSchema()) {
        return false;
    }
    return true;
}

void Database::close()
{
    if (dbHandle) {
        sqlite3_close(dbHandle);
        dbHandle = nullptr;
    }
}

bool Database::insertUser(const std::string& username, const std::string& passwordHash)
{
    if (!dbHandle && !open()) {
        return false;
    }

    static constexpr const char* sql =
        "INSERT INTO users (username, password_hash) VALUES (?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(dbHandle, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, passwordHash.c_str(), -1, SQLITE_TRANSIENT);

    const bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

bool Database::findUser(const std::string& username, std::string& outHash) const
{
    if (!dbHandle) {
        return false;
    }

    static constexpr const char* sql =
        "SELECT password_hash FROM users WHERE username = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(dbHandle, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    const int step = sqlite3_step(stmt);
    bool found = false;
    if (step == SQLITE_ROW) {
        const auto* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (text) {
            outHash.assign(text);
            found = true;
        }
    }
    sqlite3_finalize(stmt);
    return found;
}

bool Database::findUserId(const std::string& username, int& outId) const
{
    if (!dbHandle) {
        return false;
    }

    static constexpr const char* sql =
        "SELECT id FROM users WHERE username = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(dbHandle, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    const int step = sqlite3_step(stmt);
    const bool found = step == SQLITE_ROW;
    if (found) {
        outId = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return found;
}

bool Database::findUsername(int userId, std::string& outUsername) const
{
    if (!dbHandle) {
        return false;
    }

    static constexpr const char* sql =
        "SELECT username FROM users WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(dbHandle, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, userId);

    const int step = sqlite3_step(stmt);
    const bool found = step == SQLITE_ROW;
    if (found) {
        const auto* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (text) {
            outUsername.assign(text);
        }
    }
    sqlite3_finalize(stmt);
    return found;
}

bool Database::insertMessage(int senderId,
                              int recipientId,
                              const std::string& ciphertext,
                              const std::string& nonce,
                              const std::string& mac)
{
    if (!dbHandle) {
        return false;
    }

    static constexpr const char* sql =
        "INSERT INTO messages (sender_id, recipient_id, ciphertext, nonce, mac, delivered) "
        "VALUES (?, ?, ?, ?, ?, 0);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(dbHandle, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, senderId);
    sqlite3_bind_int(stmt, 2, recipientId);
    sqlite3_bind_blob(stmt, 3, ciphertext.data(), static_cast<int>(ciphertext.size()), SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 4, nonce.data(), static_cast<int>(nonce.size()), SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 5, mac.data(), static_cast<int>(mac.size()), SQLITE_TRANSIENT);

    const bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

std::vector<Database::StoredMessage> Database::getQueuedMessages(int recipientId) const
{
    std::vector<StoredMessage> messages;
    if (!dbHandle) {
        return messages;
    }

    static constexpr const char* sql =
        "SELECT id, sender_id, recipient_id, ciphertext, nonce, mac, timestamp "
        "FROM messages WHERE recipient_id = ? AND delivered = 0 ORDER BY id ASC;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(dbHandle, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return messages;
    }

    sqlite3_bind_int(stmt, 1, recipientId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        StoredMessage message{};
        message.id = sqlite3_column_int(stmt, 0);
        message.senderId = sqlite3_column_int(stmt, 1);
        message.recipientId = sqlite3_column_int(stmt, 2);

        const auto* cipherPtr = static_cast<const char*>(sqlite3_column_blob(stmt, 3));
        const int cipherSize = sqlite3_column_bytes(stmt, 3);
        if (cipherPtr && cipherSize > 0) {
            message.ciphertext.assign(cipherPtr, cipherSize);
        }

        const auto* noncePtr = static_cast<const char*>(sqlite3_column_blob(stmt, 4));
        const int nonceSize = sqlite3_column_bytes(stmt, 4);
        if (noncePtr && nonceSize > 0) {
            message.nonce.assign(noncePtr, nonceSize);
        }

        const auto* macPtr = static_cast<const char*>(sqlite3_column_blob(stmt, 5));
        const int macSize = sqlite3_column_bytes(stmt, 5);
        if (macPtr && macSize > 0) {
            message.mac.assign(macPtr, macSize);
        }

        const auto* tsPtr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        if (tsPtr) {
            message.timestamp.assign(tsPtr);
        }

        messages.emplace_back(std::move(message));
    }

    sqlite3_finalize(stmt);
    return messages;
}

bool Database::markDelivered(int messageId)
{
    if (!dbHandle) {
        return false;
    }

    static constexpr const char* sql =
        "UPDATE messages SET delivered = 1 WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(dbHandle, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, messageId);
    const bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

bool Database::logActivity(const std::string& level, const std::string& message)
{
    if (!dbHandle) {
        return false;
    }

    static constexpr const char* sql =
        "INSERT INTO logs (level, log) VALUES (?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(dbHandle, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, level.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, message.c_str(), -1, SQLITE_TRANSIENT);

    const bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

bool Database::configurePragmas()
{
    if (!dbHandle) {
        return false;
    }

    return exec(dbHandle, "PRAGMA journal_mode=WAL;")
        && exec(dbHandle, "PRAGMA synchronous=NORMAL;")
        && exec(dbHandle, "PRAGMA foreign_keys=ON;")
        && exec(dbHandle, "PRAGMA mmap_size=268435456;")
        && exec(dbHandle, "PRAGMA page_size=4096;");
}

bool Database::ensureSchema()
{
    if (!dbHandle) {
        return false;
    }

    static constexpr const char* usersSql =
        "CREATE TABLE IF NOT EXISTS users ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " username TEXT UNIQUE NOT NULL,"
        " password_hash TEXT NOT NULL,"
        " created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");";

    static constexpr const char* messagesSql =
        "CREATE TABLE IF NOT EXISTS messages ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " sender_id INTEGER NOT NULL,"
        " recipient_id INTEGER NOT NULL,"
        " ciphertext BLOB NOT NULL,"
        " nonce BLOB NOT NULL,"
        " mac BLOB NOT NULL,"
        " delivered INTEGER NOT NULL DEFAULT 0,"
        " timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
        " FOREIGN KEY(sender_id) REFERENCES users(id),"
        " FOREIGN KEY(recipient_id) REFERENCES users(id)"
        ");";

    static constexpr const char* logsSql =
        "CREATE TABLE IF NOT EXISTS logs ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " level TEXT NOT NULL,"
        " log TEXT NOT NULL,"
        " timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");";

    static constexpr const char* configSql =
        "CREATE TABLE IF NOT EXISTS config ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " memory_param INTEGER,"
        " iteration_param INTEGER,"
        " log_purge INTEGER"
        ");";

    static constexpr const char* idxUsername =
        "CREATE INDEX IF NOT EXISTS idx_username ON users(username);";
    static constexpr const char* idxRecipientDelivered =
        "CREATE INDEX IF NOT EXISTS idx_recipient_delivered ON messages(recipient_id, delivered);";
    static constexpr const char* idxSenderTimestamp =
        "CREATE INDEX IF NOT EXISTS idx_sender_timestamp ON messages(sender_id, timestamp);";

    return exec(dbHandle, usersSql)
        && exec(dbHandle, messagesSql)
        && exec(dbHandle, logsSql)
        && exec(dbHandle, configSql)
        && exec(dbHandle, idxUsername)
        && exec(dbHandle, idxRecipientDelivered)
        && exec(dbHandle, idxSenderTimestamp);
}
