#pragma once 

#include <unordered_map>
#include <sys/epoll.h>
#include <memory>
#include <vector>

class AuthManager; 
class MessageRouter; 
class ProtocolHandler; 
class StatusManager; 
class ClientState; 
struct Command; 

// Simple single-thread worker thread 
// For starters, we'll go with a single worker thread model
// to simplify development and testing of module integration 
class SingleWorker {
public: 
        SingleWorker(int id, 
                    AuthManager& auth, 
                    MessageRouter& router,
                    ProtocolHandler& protocol); 
        ~SingleWorker(); 

        void start(); 
        void stop(); 
        void assignClient(int clientFd); 
        void notifyEvent(int clientFd); 

        int id() const noexcept { return workerId; };
private:
        void eventLoop(); 
        void handleReadEvent(int clientFd); 
        void handleWriteEvent(int clientFd); 
        void processCommand(ClientState& state, const Command& command); 
        void closeClient(int clientFd); 
        ClientState* getClient(int clientFd); 
        void removeClient(int clientFd); 
        
        int workerId;
        int epollFd; 
        int wakeupFd;
        volatile bool running; 
        
        std::unordered_map<int, std::unique_ptr<ClientState>> clientStates;
        
        AuthManager& authManager;
        MessageRouter& messageRouter; 
        ProtocolHandler& protocolHandler; 

        struct epoll_event event; 
}; 