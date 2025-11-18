#pragma once
#include <map>
#include <string>
#include <pthread.h>
#include "CryptoEngine.h"
#include "DatabaseEngine.h"
#include "ClientState.h"

// Routes encrypted messages to online clients or persists them for later delivery.
class MessageRouter {
public:
    MessageRouter(Database& db,
                  CryptoEngine& crypto);
    ~MessageRouter();

    void registerClient(const std::string& username, ClientState* state);
    void unregisterClient(const std::string& username);

    bool routeMessage(const std::string& sender,
                      const std::string& recipient,
                      const std::string& plaintext);

private:
    Database& database;
    CryptoEngine& cryptoEngine;
    pthread_mutex_t clientsMutex;
    std::map<std::string, ClientState*> activeClients;
};
