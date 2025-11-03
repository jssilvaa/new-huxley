// HuxleyServer.h
#pragma once
#include <vector>
#include <memory>
#include <queue>

// Forward declarations 
class ClientConnection;
class AuthManager;
class MessageRouter;
class StatusManager;
class Database; 
class HuxleyServer {
public:
    HuxleyServer();
    ~HuxleyServer();

    bool start(int port);
    void stop();

private:
    void acceptLoop();
    pthread_mutex_t queueMutex; 
    std::queue<int> socketQueue;
    std::priority_queue<int> workerThreadSize; // Used for least load thread dispatching, improve load balance, watch out memory overhead in embedded apps
    void dispatchLogic(); 

    std::unique_ptr<AuthManager> authManager;
    std::unique_ptr<MessageRouter> messageRouter;
    std::unique_ptr<StatusManager> statusManager;
    std::unique_ptr<Database> db; 
};
