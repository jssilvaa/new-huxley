#include "OfflineDelivery.h"

#include "ClientState.h"
#include "CryptoEngine.h"
#include "DatabaseEngine.h"

#include <string>

bool deliverOfflineMessages(Database& database,
                            CryptoEngine& crypto,
                            const std::string& username,
                            ClientState& state)
{
    int recipientId = 0;
    if (!database.findUserId(username, recipientId)) {
        database.logActivity("WARN", "Offline delivery aborted - unknown user " + username);
        return false;
    }

    auto messages = database.getQueuedMessages(recipientId);
    if (messages.empty()) {
        return true;
    }

    for (const auto& stored : messages) {
        CryptoEngine::CipherMessage cipher { stored.nonce, stored.ciphertext };
        std::string plaintext;
        if (!crypto.decryptMessage(cipher, plaintext)) {
            database.logActivity("ERROR", "Failed to decrypt stored message " + std::to_string(stored.id));
            continue;
        }

        std::string senderName;
        if (!database.findUsername(stored.senderId, senderName)) {
            senderName = "unknown";
        }

        state.queueIncomingMessage(senderName, plaintext);
        database.markDelivered(stored.id);
    }

    database.logActivity("INFO", "Delivered queued messages to " + username);
    return true;
}
