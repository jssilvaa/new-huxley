#pragma once

#include <string>

class Database;
class CryptoEngine;
class ClientState;

// Helper invoked by worker threads to flush any queued offline messages for
// a specific user once authentication succeeds.
bool deliverOfflineMessages(Database& database,
                             CryptoEngine& crypto,
                             const std::string& username,
                             ClientState& state);
