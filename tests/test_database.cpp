#include "Database.h"
#include "AuthManager.h"
#include <iostream>

int main() {
    Database db("huxley.db");
    AuthManager auth(&db);

    std::cout << "Register alice: "
              << auth.registerUser("alice", "1234") << "\n";

    std::cout << "Register alice again: "
              << auth.registerUser("alice", "1234") << "\n";

    std::cout << "Login alice (ok): "
              << auth.loginUser("alice", "1234") << "\n";

    std::cout << "Login alice (wrong pw): "
              << auth.loginUser("alice", "wrong") << "\n";

    return 0;
}
