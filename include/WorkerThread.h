#pragma once 
#include <map>

// Forward declarations
class ClientState; 
struct Command; 

class WorkerThread {
    int epollFd; 
    int wakeupFd; 
    std::map<int, ClientState*> clientStates; 
    void processCommand(ClientState* state, Command cmd); 

public:
    static void* run(void); 
    void notifyEvent(int fd);
    void handleReadEvent(int fd); 
    void handleWriteEvent(int fd); 
};