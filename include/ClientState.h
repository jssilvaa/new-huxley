// MessageRouter.h
#pragma once
#include <string>
#include <pthread.h> 
#include <time.h>
#include <queue>

class ClientState {
    pthread_t* owner;  
    int socketFd;
    std::string username; 
    bool authenticated; 
    time_t timestamp; 
    std::string recvBuffer; 
    std::queue<std::string> sendQueue; 
public: 
    bool isAuthenticated() {
        return authenticated; 
    }
    void queueResponse(const std::string& msg) {
        sendQueue.push(msg); 
    }
};
