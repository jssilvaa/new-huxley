// HuxleyServer.h
#pragma once

#include <atomic>
#include <memory>
#include <queue>
#include <string>
#include <vector>
#include <pthread.h>

// Forward declarations
class WorkerThread;
class AuthManager;
class MessageRouter;
class StatusManager;
class CryptoEngine;
class ProtocolHandler;
class Database;

// Main orchestrator responsible for standing up shared services and
// dispatching accepted sockets to the worker thread pool.
class HuxleyServer {
public:
    HuxleyServer();
    ~HuxleyServer();

    bool start(int port);
    void stop();

private:
    void acceptLoop();
    static void* acceptThreadEntry(void* arg);
    void dispatchPendingClients();
    bool initializeServices(int port);
    void startWorkerPool(std::size_t threadCount); // changed here
    void stopWorkerPool();
    void shutdownServices();

    int listenFd {-1};
    std::atomic<bool> running {false}; // changed here
    pthread_t acceptThread {0};

    pthread_mutex_t queueMutex;
    pthread_cond_t queueCond;
    std::queue<int> socketQueue;

    std::vector<std::unique_ptr<WorkerThread>> workerThreads;

    std::unique_ptr<AuthManager> authManager;
    std::unique_ptr<MessageRouter> messageRouter;
    std::unique_ptr<StatusManager> statusManager;
    std::unique_ptr<CryptoEngine> cryptoEngine;
    std::unique_ptr<ProtocolHandler> protocolHandler;
    std::unique_ptr<Database> database;

    std::string databasePath;
    std::size_t nextWorkerIndex {0};
};
