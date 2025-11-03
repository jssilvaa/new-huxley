// MessageRouter.h
#pragma once
#include <string>
#include <map>

// Forward definitions 
class CryptoEngine;
class ClientState;
class Database; 

class MessageRouter {
public:
    explicit MessageRouter(Database* db, CryptoEngine* crypto);

    void registerClient(const std::string& username, ClientState* state); 
    void unregisterClient(const std::string& username); 
    bool routeMessage(const std::string& sender, const std::string& recipient, const std::string& message);
    
private:
    CryptoEngine* cryptoEngine;
    pthread_mutex_t clientsMutex; 
    std::map<std::string, ClientState> activeClients; 
};
