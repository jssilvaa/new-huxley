// DatabaseEngine.h
#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct sqlite3;

// Thin wrapper around the SQLite persistence layer.
class Database {
public:
    struct StoredMessage {
        int id;
        int senderId;
        int recipientId;
        std::string ciphertext;
        std::string nonce;
        std::string mac;
        std::string timestamp;
    };

    explicit Database(const std::string& filename);
    ~Database();

    bool open();
    void close();

    bool insertUser(const std::string& username, const std::string& passwordHash);
    bool findUser(const std::string& username, std::string& outHash) const;
    bool findUserId(const std::string& username, int& outId) const;
    bool findUsername(int userId, std::string& outUsername) const;

    bool insertMessage(int senderId,
                       int recipientId,
                       const std::string& ciphertext,
                       const std::string& nonce,
                       const std::string& mac);
    std::vector<StoredMessage> getQueuedMessages(int recipientId) const;
    bool markDelivered(int messageId);

    bool logActivity(const std::string& level, const std::string& message);

private:
    bool configurePragmas();
    bool ensureSchema();

    sqlite3* dbHandle;
    std::string dbPath;
};
