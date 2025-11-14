#include "../include/SingleWorker.h"

SingleWorker::SingleWorker(int id,
                           AuthManager& auth, 
                           MessageRouter& router, 
                           ProtocolHandler& protocol) 
            : workerId(id), 
              epollFd(-1), 
              wakeupFd(-1),
              running(false),
              eventBuffer(64),
              authManager(auth),
              messageRouter(router),
              protocolHandler(protocol)

{}

SingleWorker::~SingleWorker() 
{
    stop(); 
}

void SingleWorker::start() {
    
    // create epoll, wait file descriptors
    epollFd = epoll_create(16); // to issue/rcv from sockets
    wakeupFd = epoll_create(1); // to rcv a wake-up call from the main thread 

    // assign it to running
    running = true; 
    
    // jumpstart to program execution
    eventLoop(); 
}

void SingleWorker::eventLoop() {

    // rcv from socket or main 
    

}