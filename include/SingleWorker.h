#pragma once 

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <sys/epoll.h>

#include "ClientNotifier.h"

class AuthManager; 
class MessageRouter; 
class ProtocolHandler; 
class StatusManager; 
class ClientState; 
struct Command; 

// Simple single-thread worker thread 
// For starters, we'll go with a single worker thread model
// to simplify development and testing of module integration 
class SingleWorker : public ClientNotifier {
public: 
        SingleWorker(int id, 
                    AuthManager& auth, 
                    MessageRouter& router,
                    ProtocolHandler& protocol); 
        ~SingleWorker(); 

        void start(); 
        void stop(); 
        void assignClient(int clientFd); 
        void notifyEvent(int clientFd) override; 
        void waitUntilReady();
        bool isReady() const noexcept { return ready.load(std::memory_order_acquire); }
        bool hasInitFailed() const noexcept { return initFailed.load(std::memory_order_acquire); }

        int id() const noexcept { return workerId; };
private:
        void signalInitFailure();
        void eventLoop(); 
        void handleReadEvent(int clientFd); 
        void handleWriteEvent(int clientFd); 
        void processCommand(ClientState& state, const Command& command); 
        void closeClient(int clientFd); 
        ClientState* getClient(int clientFd); 
        void removeClient(int clientFd); 
        void drainPendingClients();
        void closePendingClients();
        void wakeWorker();
        
        int workerId;
        int epollFd; 
        int wakeupFd;
        std::atomic<bool> running; 
        std::atomic<bool> ready;
        std::atomic<bool> initFailed;
        mutable std::mutex readyMutex;
        std::condition_variable readyCv;
        
        std::unordered_map<int, std::unique_ptr<ClientState>> clientStates;
        std::vector<epoll_event> eventBuffer; 
        std::mutex pendingMutex;
        std::vector<int> pendingClients;
        
        AuthManager& authManager;
        MessageRouter& messageRouter; 
        ProtocolHandler& protocolHandler; 

}; 
