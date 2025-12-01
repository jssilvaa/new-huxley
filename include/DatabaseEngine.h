// DatabaseEngine.h
#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct sqlite3;
struct sqlite3_stmt;

// Database Wrapper around the SQLite persistence layer.
class Database {
public:
    struct StoredMessage {
        int id;
        int senderId;
        int recipientId;
        std::string ciphertext;
        std::string nonce;
        std::string timestamp;
    };

    struct UserSummary {
        int id;
        std::string username;
    };

    explicit Database(const std::string& filename);
    ~Database();

    bool isOpen() const noexcept;
    template <typename Input, typename Output>
    bool singleColumnQuery(sqlite3_stmt*& cachedStmt,
                           const char* sql,
                           const Input& input,
                           Output& out) const;

    bool insertUser(const std::string& username, const std::string& passwordHash);
    bool findUser(const std::string& username, std::string& outHash) const;
    bool findUserId(const std::string& username, int& outId) const;
    bool findUsername(int userId, std::string& outUsername) const;

    bool insertMessage(int senderId,
                       int recipientId,
                       const std::string& ciphertext,
                       const std::string& nonce,
                       int& outMessageId);
    std::vector<StoredMessage> getQueuedMessages(int recipientId) const;
    std::vector<UserSummary> listAllUsers() const;
    std::vector<StoredMessage> getConversation(int userA,
                                               int userB,
                                               int limit,
                                               int offset) const;
    bool markDelivered(int messageId);

    bool logActivity(const std::string& level, const std::string& message);

private:
    class StatementGuard {
    public:
        StatementGuard(const Database& db, sqlite3_stmt* stmt) noexcept;
        StatementGuard(StatementGuard&& other) noexcept;
        StatementGuard& operator=(StatementGuard&& other) noexcept;
        StatementGuard(const StatementGuard&) = delete;
        StatementGuard& operator=(const StatementGuard&) = delete;
        ~StatementGuard();

        sqlite3_stmt* get() const noexcept { return statement; }
        explicit operator bool() const noexcept { return statement != nullptr; }

    private:
        const Database* database;
        sqlite3_stmt* statement;
    };

    bool configurePragmas();
    bool ensureSchema();
    sqlite3_stmt* getStatement(sqlite3_stmt*& stmt, const char* sql) const;
    void resetStatement(sqlite3_stmt* stmt) const;
    void finalizeStatements();
    void teardown();
    StatementGuard makeStatementGuard(sqlite3_stmt*& stmt, const char* sql) const;

    sqlite3* dbHandle;
    std::string dbPath;

    mutable sqlite3_stmt* insertUserStmt {nullptr};
    mutable sqlite3_stmt* findUserStmt {nullptr};
    mutable sqlite3_stmt* findUserIdStmt {nullptr};
    mutable sqlite3_stmt* findUsernameStmt {nullptr};
    mutable sqlite3_stmt* insertMessageStmt {nullptr};
    mutable sqlite3_stmt* queuedMessagesStmt {nullptr};
    mutable sqlite3_stmt* listUsersStmt {nullptr};
    mutable sqlite3_stmt* conversationStmt {nullptr};
    mutable sqlite3_stmt* markDeliveredStmt {nullptr};
    mutable sqlite3_stmt* logActivityStmt {nullptr};
};
