#include "../include/MessageRouter.h"
#include "../include/ClientState.h"
#include "../include/DatabaseEngine.h"

MessageRouter::MessageRouter(Database& db,
                             CryptoEngine& crypto)
    : database(db)
    , cryptoEngine(crypto)
{
    pthread_mutex_init(&clientsMutex, nullptr);
}

MessageRouter::~MessageRouter()
{
    pthread_mutex_destroy(&clientsMutex);
}

void MessageRouter::registerClient(const std::string& username, ClientState* state)
{
    pthread_mutex_lock(&clientsMutex);
    activeClients[username] = state;
    pthread_mutex_unlock(&clientsMutex);

    database.logActivity("INFO", "Client online: " + username);
}

void MessageRouter::unregisterClient(const std::string& username)
{
    pthread_mutex_lock(&clientsMutex);
    activeClients.erase(username);
    pthread_mutex_unlock(&clientsMutex);

    database.logActivity("INFO", "Client offline: " + username);
}

bool MessageRouter::routeMessage(const std::string& sender,
                                 const std::string& recipient,
                                 const std::string& plaintext)
{
    const auto cipher = cryptoEngine.encryptMessage(plaintext);
    int senderId = 0;
    int recipientId = 0;

    // check users exist
    if (!database.findUserId(sender, senderId) || !database.findUserId(recipient, recipientId)) {
        database.logActivity("WARN", "Failed to persist message - unknown user");
        return false;
    }

    // persist message in database
    if (!database.insertMessage(senderId, recipientId, cipher.ciphertext, cipher.nonce)) {
        return false;
    }

    ClientState* recipientState = nullptr;
    pthread_mutex_lock(&clientsMutex);
    auto it = activeClients.find(recipient);
    if (it != activeClients.end()) {
        recipientState = it->second;
    }
    pthread_mutex_unlock(&clientsMutex);

    // user offline, message stored
    if (!recipientState) {
        return true; // Stored for later delivery.
    }

    // user online, deliver in real-time
    recipientState->queueIncomingMessage(sender, plaintext);
    database.logActivity("INFO", "Queued realtime delivery: " + sender + " -> " + recipient);
    return true;
}