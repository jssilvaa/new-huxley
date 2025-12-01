#include "SingleServer.h"

#include "AuthManager.h"
#include "CryptoEngine.h"
#include "DatabaseEngine.h"
#include "MessageRouter.h"
#include "ProtocolHandler.h"
#include "SingleWorker.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <system_error>

namespace {
constexpr const char* kDefaultDatabasePath = "huxley.db";

bool setReusable(int fd)
{
    int opt = 1;
    return ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == 0;
}
}

SingleServer::SingleServer()
    : listenFd(-1)
    , running(false)
    , workerThread()
    , databasePath(kDefaultDatabasePath)
{
}

SingleServer::~SingleServer()
{
    stop();
}

bool SingleServer::start(int port)
{
    if (running.load()) {
        return true;
    }

    if (!initializeServices(port)) {
        return false;
    }

    worker = std::make_unique<SingleWorker>(0,
                                            *authManager,
                                            *messageRouter,
                                            *protocolHandler,
                                            *database,
                                            *cryptoEngine);

    running.store(true);

    try {
        workerThread = std::thread([this] {
            worker->start();
        });
    } catch (const std::system_error& err) {
        std::cerr << "Failed to start worker thread: " << err.what() << std::endl;
        running.store(false);
        worker.reset();
        shutdownServices();
        return false;
    }

    worker->waitUntilReady();
    if (!worker->isReady()) {
        std::cerr << "Worker failed to initialize" << std::endl;
        running.store(false);
        if (workerThread.joinable()) {
            workerThread.join();
        }
        worker.reset();
        shutdownServices();
        return false;
    }

    try {
        acceptThread = std::thread(&SingleServer::acceptLoop, this);
    } catch (const std::system_error& err) {
        std::cerr << "Failed to start accept thread: " << err.what() << std::endl;
        running.store(false);
        if (worker) {
            worker->stop();
        }
        if (workerThread.joinable()) {
            workerThread.join();
        }
        worker.reset();
        shutdownServices();
        return false;
    }

    return true;
}

void SingleServer::stop()
{
    if (!running.exchange(false)) {
        return;
    }

    if (listenFd != -1) {
        ::shutdown(listenFd, SHUT_RDWR);
        ::close(listenFd);
        listenFd = -1;
    }

    if (acceptThread.joinable()) {
        acceptThread.join();
    }

    if (worker) {
        worker->stop();
    }

    if (workerThread.joinable()) {
        workerThread.join();
    }

    worker.reset();
    shutdownServices();
}

bool SingleServer::initializeServices(int port)
{
    database = std::make_unique<Database>(databasePath);
    if (!database->isOpen()) {
        std::cerr << "Failed to open database" << std::endl;
        database.reset();
        return false;
    }

    cryptoEngine = std::make_unique<CryptoEngine>();
    protocolHandler = std::make_unique<ProtocolHandler>();
    authManager = std::make_unique<AuthManager>(*database);
    messageRouter = std::make_unique<MessageRouter>(*database, *cryptoEngine);

    listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd == -1) {
        std::perror("socket");
        return false;
    }

    setReusable(listenFd);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (::bind(listenFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        std::perror("bind");
        ::close(listenFd);
        listenFd = -1;
        return false;
    }

    if (::listen(listenFd, SOMAXCONN) == -1) {
        std::perror("listen");
        ::close(listenFd);
        listenFd = -1;
        return false;
    }

    return true;
}

void SingleServer::shutdownServices()
{
    worker.reset();
    messageRouter.reset();
    authManager.reset();
    cryptoEngine.reset();
    protocolHandler.reset();

    if (database) {
        database.reset();
    }
}

void SingleServer::acceptLoop()
{
    while (running.load()) {
        sockaddr_in clientAddr{};
        socklen_t addrLen = sizeof(clientAddr);
        const int clientFd = ::accept(listenFd, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
        if (clientFd == -1) {
            if (errno == EINTR) {
                continue;
            }
            if (!running.load()) {
                break;
            }
            if (errno == EBADF || errno == EINVAL) {
                break;
            }
            std::perror("accept");
            continue;
        }

        if (!worker) {
            ::close(clientFd);
            continue;
        }

        worker->assignClient(clientFd);
    }
}
