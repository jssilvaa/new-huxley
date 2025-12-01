#include "../include/MessageRouter.h"
#include "../include/ClientState.h"
#include "../include/DatabaseEngine.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

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

bool MessageRouter::isRegistered(const std::string& username)
{
    pthread_mutex_lock(&clientsMutex);
    const bool registered = activeClients.find(username) != activeClients.end();
    pthread_mutex_unlock(&clientsMutex);
    return registered;
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

std::vector<std::string> MessageRouter::listActiveUsers()
{
    std::vector<std::string> users;
    pthread_mutex_lock(&clientsMutex);
    users.reserve(activeClients.size());
    for (const auto& entry : activeClients) {
        users.push_back(entry.first);
    }
    pthread_mutex_unlock(&clientsMutex);
    return users;
}

namespace {
std::string isoTimestampNow()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif

    char buffer[32];
    if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm)) {
        return std::string(buffer);
    }
    return {};
}
} // namespace

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
    int messageId = 0;
    if (!database.insertMessage(senderId, recipientId, cipher.ciphertext, cipher.nonce, messageId)) {
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
    const std::string timestamp = isoTimestampNow();
    recipientState->queueIncomingMessage(sender, plaintext, timestamp, messageId);
    if (!database.markDelivered(messageId)) {
        database.logActivity("ERROR", "Realtime delivery persisted but markDelivered failed for message "
                                         + std::to_string(messageId));
        return false;
    }

    database.logActivity("INFO", "Queued realtime delivery: " + sender + " -> " + recipient);
    return true;
}
