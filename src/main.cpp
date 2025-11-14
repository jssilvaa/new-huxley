#include "AuthManager.h"
#include "DatabaseEngine.h"
#include "HuxleyServer.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

namespace {
std::atomic<bool> gKeepRunning {true};

void handleSignal(int)
{
    gKeepRunning.store(false);
}

void printUsage(const char* prog)
{
    std::cout << "Usage: " << prog << " [--port <port>] [--duration <seconds>] [--no-block]" << std::endl;
    std::cout << "       --port <port>        TCP port to bind (default: 8080)" << std::endl;
    std::cout << "       --duration <seconds> Run headless for N seconds then exit" << std::endl;
    std::cout << "       --no-block          Run headless until SIGINT/SIGTERM" << std::endl;
}
} // namespace

int main(int argc, char** argv)
{
    int port = 8080;
    bool waitForEnter = true;
    std::optional<int> durationSeconds;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--duration" && i + 1 < argc) {
            durationSeconds = std::stoi(argv[++i]);
            waitForEnter = false;
        } else if (arg == "--no-block") {
            waitForEnter = false;
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    Database database("huxley.db");
    if (!database.open()) {
        std::cerr << "Failed to open database" << std::endl;
        return 1;
    }

    AuthManager auth(database);
    auth.registerUser("alice", "password123");
    if (auth.loginUser("alice", "password123")) {
        std::cout << "Authentication OK" << std::endl;
    }

    HuxleyServer server;
    if (!server.start(port)) {
        std::cerr << "Server failed to start" << std::endl;
        return 1;
    }

    if (waitForEnter) {
        std::cout << "Server running on port " << port << ". Press Enter to stop." << std::endl;
        std::cin.get();
    } else if (durationSeconds) {
        std::cout << "Server running on port " << port << " for " << *durationSeconds << " seconds." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(*durationSeconds));
    } else {
        std::cout << "Server running on port " << port << ". Send SIGINT (Ctrl+C) to stop." << std::endl;
        std::signal(SIGINT, handleSignal);
        std::signal(SIGTERM, handleSignal);
        while (gKeepRunning.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }

    server.stop();
    return 0;
}
