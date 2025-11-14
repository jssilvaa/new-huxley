// MessageRouter.h
#pragma once

#include <map>
#include <string>
#include <vector>
#include <pthread.h>

#include "CryptoEngine.h"
#include "AuthManager.h"
#include "DatabaseEngine.h"

class ClientState;
class Database;

// Routes encrypted messages to online clients or persists them for later delivery.
class MessageRouter {
public:
    // MessageRouter(Database& db, CryptoEngine& crypto);
    MessageRouter(Database& db, CryptoEngine& crypto, AuthManager& authMgr);
    ~MessageRouter();

    void registerClient(const std::string& username, ClientState* state);
    void unregisterClient(const std::string& username);

    bool routeMessage(const std::string& sender,
                      const std::string& recipient,
                      const std::string& plaintext);
    bool deliverQueuedMessages(const std::string& username, ClientState& state);

private:
    ClientState* findActiveClient(const std::string& username);
    bool persistMessage(const std::string& sender,
                        const std::string& recipient,
                        const CryptoEngine::CipherMessage& cipher);

    Database& database;
    CryptoEngine& cryptoEngine;
    AuthManager& authManager;
    pthread_mutex_t clientsMutex;
    std::map<std::string, ClientState*> activeClients;
};
