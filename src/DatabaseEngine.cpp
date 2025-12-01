#include "../include/DatabaseEngine.h"

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

// Create binding policies
template<typename T> 
void bindParam(sqlite3_stmt* stmt, int index, const T&) = delete; 

// int 
template<>
inline void bindParam<int>(sqlite3_stmt* stmt, int index, const int& value) 
{
    sqlite3_bind_int(stmt, index, value); 
}

// string
template<>
inline void bindParam<std::string>(sqlite3_stmt* stmt, int index, const std::string& value)
{
    sqlite3_bind_text(stmt, index, value.c_str(), -1, SQLITE_TRANSIENT); 
}

// const char* 
template<>
inline void bindParam<const char*>(sqlite3_stmt* stmt, int index, const char* const& value)
{
    sqlite3_bind_text(stmt, index, value, -1, SQLITE_TRANSIENT); 
}

// Generic extractor — fallback deleted.
template <typename T>
void extractColumn(sqlite3_stmt* stmt, int col, T&) = delete;

// Extract int
template <>
inline void extractColumn<int>(sqlite3_stmt* stmt, int col, int& out) {
    out = sqlite3_column_int(stmt, col);
}

// Extract std::string
template <>
inline void extractColumn<std::string>(sqlite3_stmt* stmt, int col, std::string& out) {
    const auto* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
    out = text ? text : "";
}
} // namespace

// helps bind cached statement and sql binding string to 
// implement findUser, findUserId, findUsername in FP style
template <typename Input, typename Output>
bool Database::singleColumnQuery(sqlite3_stmt*& cachedStmt,
                       const char* sql,
                       const Input& input,
                       Output& out) const
{
    if (!dbHandle) {
        return false;
    }

    auto guard = makeStatementGuard(cachedStmt, sql);
    sqlite3_stmt* stmt = guard.get();
    if (!stmt) {
        return false;
    }

    bindParam(stmt, 1, input);

    const int rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        return false;
    }

    extractColumn(stmt, 0, out);
    return true;
}

Database::Database(const std::string& filename)
    : dbHandle(nullptr)
    , dbPath(filename)
{
    if (sqlite3_open(dbPath.c_str(), &dbHandle) != SQLITE_OK) {
        std::cerr << "Failed to open database: "
                  << (dbHandle ? sqlite3_errmsg(dbHandle) : "unknown error")
                  << std::endl;
        if (dbHandle) {
            sqlite3_close(dbHandle);
            dbHandle = nullptr;
        }
        return;
    }

    if (!configurePragmas() || !ensureSchema()) {
        std::cerr << "Failed to initialize database schema" << std::endl;
        teardown();
    }
}

Database::~Database()
{
    teardown();
}

bool Database::isOpen() const noexcept
{
    return dbHandle != nullptr;
}

bool Database::insertUser(const std::string& username, const std::string& passwordHash)
{
    static constexpr const char* sql =
        "INSERT INTO users (username, password_hash) VALUES (?, ?);";

    auto stmtGuard = makeStatementGuard(insertUserStmt, sql);
    sqlite3_stmt* stmt = stmtGuard.get();
    if (!stmt) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, passwordHash.c_str(), -1, SQLITE_TRANSIENT);

    return sqlite3_step(stmt) == SQLITE_DONE;
}

bool Database::findUser(const std::string& username, std::string& outHash) const {
    static constexpr const char* sql =
        "SELECT password_hash FROM users WHERE username = ?;";
    return singleColumnQuery(findUserStmt, sql, username, outHash);
}

bool Database::findUserId(const std::string& username, int& outId) const {
    static constexpr const char* sql =
        "SELECT id FROM users WHERE username = ?;";
    return singleColumnQuery(findUserIdStmt, sql, username, outId);
}

bool Database::findUsername(int userId, std::string& outUsername) const {
    static constexpr const char* sql =
        "SELECT username FROM users WHERE id = ?;";
    return singleColumnQuery(findUsernameStmt, sql, userId, outUsername);
}

// Store encrypted message in database
bool Database::insertMessage(int senderId,
                              int recipientId,
                              const std::string& ciphertext,
                              const std::string& nonce,
                              int& outMessageId)
{
    if (!dbHandle) {
        return false;
    }

    // SQL to insert message
    static constexpr const char* sql =
        "INSERT INTO messages (sender_id, recipient_id, ciphertext, nonce, delivered) "
        "VALUES (?, ?, ?, ?, 0);";

    // Prepare statement for inserting message
    auto stmtGuard = makeStatementGuard(insertMessageStmt, sql);
    sqlite3_stmt* stmt = stmtGuard.get();
    if (!stmt) {
        return false;
    }
    // Bind parameters to SQL statement
    sqlite3_bind_int(stmt, 1, senderId);
    sqlite3_bind_int(stmt, 2, recipientId);
    sqlite3_bind_blob(stmt, 3, ciphertext.data(), static_cast<int>(ciphertext.size()), SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 4, nonce.data(), static_cast<int>(nonce.size()), SQLITE_TRANSIENT);

    // Execute insertion
    const int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to insert message: "
                  << (dbHandle ? sqlite3_errmsg(dbHandle) : "database closed")
                  << std::endl;
        return false;
    }

    outMessageId = static_cast<int>(sqlite3_last_insert_rowid(dbHandle));
    return true;
}

std::vector<Database::StoredMessage> Database::getQueuedMessages(int recipientId) const
{
    std::vector<StoredMessage> messages;
    if (!dbHandle) {
        return messages;
    }

    static constexpr const char* sql =
        "SELECT id, sender_id, recipient_id, ciphertext, nonce, timestamp "
        "FROM messages WHERE recipient_id = ? AND delivered = 0 ORDER BY id ASC;";

    auto stmtGuard = makeStatementGuard(queuedMessagesStmt, sql);
    sqlite3_stmt* stmt = stmtGuard.get();
    if (!stmt) {
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

        const auto* tsPtr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        if (tsPtr) {
            message.timestamp.assign(tsPtr);
        }

        messages.emplace_back(std::move(message));
    }

    return messages;
}

bool Database::markDelivered(int messageId)
{
    if (!dbHandle) {
        return false;
    }

    static constexpr const char* sql =
        "UPDATE messages SET delivered = 1 WHERE id = ?;";

    auto stmtGuard = makeStatementGuard(markDeliveredStmt, sql);
    sqlite3_stmt* stmt = stmtGuard.get();
    if (!stmt) {
        return false;
    }
    sqlite3_bind_int(stmt, 1, messageId);
    const int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to mark message " << messageId
                  << " as delivered: "
                  << (dbHandle ? sqlite3_errmsg(dbHandle) : "database closed")
                  << std::endl;
        return false;
    }
    return true;
}

bool Database::logActivity(const std::string& level, const std::string& message)
{
    if (!dbHandle) {
        return false;
    }

    static constexpr const char* sql =
        "INSERT INTO logs (level, log) VALUES (?, ?);";

    auto stmtGuard = makeStatementGuard(logActivityStmt, sql);
    sqlite3_stmt* stmt = stmtGuard.get();
    if (!stmt) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, level.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, message.c_str(), -1, SQLITE_TRANSIENT);

    return sqlite3_step(stmt) == SQLITE_DONE;
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

Database::StatementGuard::StatementGuard(const Database& db, sqlite3_stmt* stmt) noexcept
    : database(&db)
    , statement(stmt)
{
}

Database::StatementGuard::StatementGuard(StatementGuard&& other) noexcept
    : database(other.database)
    , statement(other.statement)
{
    other.statement = nullptr;
}

Database::StatementGuard& Database::StatementGuard::operator=(StatementGuard&& other) noexcept
{
    if (this != &other) {
        if (statement && database) {
            database->resetStatement(statement);
        }
        database = other.database;
        statement = other.statement;
        other.statement = nullptr;
    }
    return *this;
}

Database::StatementGuard::~StatementGuard()
{
    if (statement && database) {
        database->resetStatement(statement);
    }
}

sqlite3_stmt* Database::getStatement(sqlite3_stmt*& stmt, const char* sql) const
{
    if (!dbHandle) {
        return nullptr;
    }

    if (!stmt) {
        if (sqlite3_prepare_v2(dbHandle, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(dbHandle) << std::endl;
            stmt = nullptr;
            return nullptr;
        }
    }

    return stmt;
}

void Database::resetStatement(sqlite3_stmt* stmt) const
{
    if (!stmt) {
        return;
    }

    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
}

void Database::finalizeStatements()
{
    auto finalize = [](sqlite3_stmt*& stmt) {
        if (stmt) {
            sqlite3_finalize(stmt);
            stmt = nullptr;
        }
    };

    finalize(insertUserStmt);
    finalize(findUserStmt);
    finalize(findUserIdStmt);
    finalize(findUsernameStmt);
    finalize(insertMessageStmt);
    finalize(queuedMessagesStmt);
    finalize(markDeliveredStmt);
    finalize(logActivityStmt);
}

void Database::teardown()
{
    finalizeStatements();
    if (dbHandle) {
        sqlite3_close(dbHandle);
        dbHandle = nullptr;
    }
}

Database::StatementGuard Database::makeStatementGuard(sqlite3_stmt*& stmt, const char* sql) const
{
    return StatementGuard(*this, getStatement(stmt, sql));
}
