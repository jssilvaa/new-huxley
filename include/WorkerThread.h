#pragma once

#include <atomic>
#include <memory>
#include <unordered_map>
#include <vector>
#include <pthread.h>

class AuthManager;
class MessageRouter;
class ProtocolHandler;
class StatusManager;
class ClientState;
struct Command;

// Event-driven worker responsible for servicing a shard of client sockets.
class WorkerThread {
public:
    WorkerThread(int id,
                 AuthManager& auth,
                 MessageRouter& router,
                 ProtocolHandler& protocol,
                 StatusManager& status);
    ~WorkerThread();

    void start();
    void stop();
    void assignClient(int clientFd);
    void notifyEvent(int clientFd);

    int id() const { return workerId; }

private:
    static void* threadEntry(void* arg);
    void eventLoop();
    void handleReadEvent(int clientFd);
    void handleWriteEvent(int clientFd);
    void processCommand(ClientState& state, const Command& command);
    void closeClient(int clientFd);

    int workerId;
    int epollFd;
    int wakeupFd;
    std::atomic<bool> running;
    pthread_t threadHandle;

    pthread_mutex_t clientsMutex;
    std::unordered_map<int, std::unique_ptr<ClientState>> clientStates;

    AuthManager& authManager;
    MessageRouter& messageRouter;
    ProtocolHandler& protocolHandler;
    StatusManager& statusManager;

    std::vector<int> eventBuffer;
};
