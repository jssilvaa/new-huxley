#include "WorkerThread.h"

#include "AuthManager.h"
#include "ClientState.h"
#include "MessageRouter.h"
#include "ProtocolHandler.h"
#include "StatusManager.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>

WorkerThread::WorkerThread(int id, 
                           AuthManager& auth, 
                           MessageRouter& router, 
                           ProtocolHandler& protocol, 
                           StatusManager& status) 
    : workerId(id), 
      epollFd(-1),
      wakeupFd(-1),
      running(false),
      threadHandle(0),
      clientStates(),
      authManager(auth),
      messageRouter(router),
      protocolHandler(protocol),
      statusManager(status) 
{
    pthread_mutex_init(&clientsMutex, nullptr); 
}

