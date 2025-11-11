#include "AuthManager.h"
#include "DatabaseEngine.h"
#include "HuxleyServer.h"

#include <iostream>

int main()
{
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
    if (!server.start(8080)) {
        std::cerr << "Server failed to start" << std::endl;
        return 1;
    }

    std::cout << "Server running on port 8080. Press Enter to stop." << std::endl;
    std::cin.get();

    server.stop();
    return 0;
}
