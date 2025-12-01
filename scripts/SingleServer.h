#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <thread>

class SingleWorker;
class AuthManager;
class MessageRouter;
class ProtocolHandler;
class CryptoEngine;
class Database;

// Minimal single-worker server: one listener thread dispatching
// all accepted sockets to a lone SingleWorker instance.
class SingleServer {
public:
    SingleServer();
    ~SingleServer();

    bool start(int port);
    void stop();

private:
    bool initializeServices(int port);
    void shutdownServices();
    void acceptLoop();

    int listenFd;
    std::atomic<bool> running;
    std::thread acceptThread;
    std::thread workerThread;

    std::unique_ptr<SingleWorker> worker;
    std::unique_ptr<AuthManager> authManager;
    std::unique_ptr<MessageRouter> messageRouter;
    std::unique_ptr<ProtocolHandler> protocolHandler;
    std::unique_ptr<CryptoEngine> cryptoEngine;
    std::unique_ptr<Database> database;

    std::string databasePath;
};
