// Database.h
#pragma once
#include <string>
#include <unordered_map>
#include <vector>

class Database {
public:
    explicit Database(const std::string& filename);
    ~Database();

    bool insertUser(const std::string& username, const std::string& hash);
    bool findUser(const std::string& username, std::string& outHash);
    bool insertMessage(int sender_id, int recipient_id); // missing encrypted blob
    std::vector<std::string> getQueuedMessages(int recipient_id); 
    bool markDelivered(int message_id); 
    bool logActivity(const std::string& log);
private:
    void* dbHandle; // will be sqlite3*
    std::unordered_map<std::string, std::string> users; // in-memory placeholder store
};
