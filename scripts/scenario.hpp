#include <string> 
#include <sodium.h> 
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream> 
#define FILE_PATH std::filesystem::current_path().string() + "/conversations.json" 

const size_t sz = 32; 
const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
using json = nlohmann::json;

// == Class Definitions == 
class ScenarioUser {
public:
    ScenarioUser(const std::string& user, const std::string& pass) 
        : username(user), password(pass) 
    {}

    std::string getUsername() const {
        return username; 
    }

private: 
    std::string username; 
    std::string password; 
};

// == Helper functions == 
void random_username(std::string& outUsername) 
{   
    const size_t max_index = sizeof(charset) - 1;
    std::string username;
    for (size_t i = 0; i < sz; i++) {
        username += charset[rand() % max_index];
    }
    outUsername.assign(username);
}

void random_password(std::string& outPassword) {
    const size_t max_index = sizeof(charset) - 1; 
    std::string password; 
    for (size_t i = 0; i < sz; i++) {
        password += charset[rand() % max_index]; 
    }
    
    char hashed_password[crypto_pwhash_STRBYTES]; 
    if (crypto_pwhash_str(hashed_password, password.c_str(), sz, crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
        fprintf(stderr, "error hashing");
    }
    outPassword.assign(static_cast<std::string>(hashed_password)); 
}

std::vector<json> load_conversations() {
    std::ifstream i(FILE_PATH);
    json j;
    i >> j; 

    std::vector<json> conversations;
    for (auto& element : j.items()) {
        conversations.push_back(element.value()); 
    }
    return conversations;
}

std::vector<json> random_message() {
    std::vector<json> conversations = load_conversations();
    json convo = conversations[rand() % conversations.size()];
    auto& message = convo["messages"]; 

    return message.get<std::vector<json>>();
}

