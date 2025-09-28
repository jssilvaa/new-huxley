#include "HuxleyServer.h"
#include "Database.h"
#include "AuthManager.h"
#include <iostream> 

// Test authmanager
int main() {
    // HuxleyServer server;
    // if (server.start(8080)) {
    //     // TODO: loop or signal handling
    // }
    // server.stop();
    // return 0;

    Database db("huxley.db");
    AuthManager authManager(&db);
    authManager.registerUser("testuser", "testpassword");
    if (authManager.loginUser("testuser", "testpassword")) {
        std::cout << "Login successful!" << std::endl;
    } else {
        std::cout << "Login failed!" << std::endl;
    }

    return 0;
}
