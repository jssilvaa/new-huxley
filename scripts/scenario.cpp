#include <iostream>
#include <string>
#include <sodium.h>
#include "scenario.hpp"

int main() {
    // Initialize libsodium
    if ( sodium_init() != 0 ) {
        fprintf(stderr, "error initializing sodium"); 
    }
    srand(static_cast<unsigned int>(time(nullptr)));   
    
    // Generate random username
    std::string username;
    random_username(username);
    std::cout << username << std::endl; 

    // Generate random password
    std::string password;
    random_password(password); 
    std::cout << password << std::endl; 

    // Load random message
    auto message = random_message(); 
    std::cout << "Random Message: " << std::endl;
    for (auto &msg : message) {
        std::cout << msg << std::endl;
    }

    // Create ScenarioUser
    ScenarioUser user(username, password);
    return 0;
}