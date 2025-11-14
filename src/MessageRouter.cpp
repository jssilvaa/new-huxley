#include "../include/MessageRouter.h"
#include "../include/ClientState.h"
#include "../include/AuthManager.h"
#include "../include/DatabaseEngine.h"
#include <sstream>

namespace {
std::string buildInboundMessagePayload(const std::string& sender, const std::string& content)
{
    std::ostringstream oss;
    oss << "{\"command\":\"incoming_message\",";
    oss << "\"sender\":\"" << sender << "\",";
    oss << "\"content\":\"";
    for (char c : content) {
        if (c == '\\' || c == '"') {
            oss << '\\';
        }
        if (c == '\n') {
            oss << "\\n";
        } else {
            oss << c;
        }
    }
    oss << "\"}";
    oss << '\n';
    return oss.str();
}
} // namespace

MessageRouter::MessageRouter(Database& db, CryptoEngine& crypto, AuthManager& authMgr)
    : database(db)
    , cryptoEngine(crypto)
    , authManager(authMgr) 
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

ClientState* MessageRouter::findActiveClient(const std::string& username)
{
    pthread_mutex_lock(&clientsMutex);
    auto it = activeClients.find(username);
    ClientState* state = it != activeClients.end() ? it->second : nullptr;
    pthread_mutex_unlock(&clientsMutex);
    return state;
}

bool MessageRouter::persistMessage(const std::string& sender,
                                   const std::string& recipient,
                                   const CryptoEngine::CipherMessage& cipher)
{
    int senderId = 0;
    int recipientId = 0;
    if (!database.findUserId(sender, senderId) || !database.findUserId(recipient, recipientId)) {
        database.logActivity("WARN", "Failed to persist message - unknown user");
        return false;
    }

    return database.insertMessage(senderId, recipientId, cipher.ciphertext, cipher.nonce);
}

bool MessageRouter::routeMessage(const std::string& sender,
                                 const std::string& recipient,
                                 const std::string& plaintext)
{
    const auto cipher = cryptoEngine.encryptMessage(plaintext);
    if (!persistMessage(sender, recipient, cipher)) {
        return false;
    }

    ClientState* recipientState = findActiveClient(recipient);
    if (!recipientState) {
        return true; // Stored for later delivery.
    }

    recipientState->queueFramedResponse(buildInboundMessagePayload(sender, plaintext));
    database.logActivity("INFO", "Queued realtime delivery: " + sender + " -> " + recipient);
    return true;
}

bool MessageRouter::deliverQueuedMessages(const std::string& username, ClientState& state)
{
    int recipientId = 0;
    if (!database.findUserId(username, recipientId)) {
        return false;
    }

    auto messages = database.getQueuedMessages(recipientId);
    if (messages.empty()) {
        return true;
    }

    for (const auto& stored : messages) {
        CryptoEngine::CipherMessage cipher { stored.nonce, stored.ciphertext };
        std::string plaintext;
        if (!cryptoEngine.decryptMessage(cipher, plaintext)) {
            database.logActivity("ERROR", "Failed to decrypt stored message " + std::to_string(stored.id));
            continue;
        }

        std::string senderName;
        if (!database.findUsername(stored.senderId, senderName)) {
            senderName = "unknown";
        }

        state.queueFramedResponse(buildInboundMessagePayload(senderName, plaintext));
        database.markDelivered(stored.id);
    }

    database.logActivity("INFO", "Delivered queued messages to " + username);
    return true;
}
