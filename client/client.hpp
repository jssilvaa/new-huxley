#include <string> 
#include <optional>
#include <nlohmann/json.hpp>
#include <iostream> 
#include <functional> 
using json = nlohmann::json;

class ProtocolClient {
public:
    ProtocolClient(const int32_t timeout, const std::optional<int> fd)
        : timeout(timeout), socketFd(fd)
    {}
    ~ProtocolClient() = default;

    void connect(const std::string& host, const int port); 
    void close(); 
    std::optional<int> getSocket() const noexcept { return socketFd; }
    void send_command(const json& command) const;
    std::optional<json> receive_response() const;
    
private:
    int32_t timeout; 
    std::optional<int> socketFd; 
    std::optional<std::string> recv_exactly(const size_t size) const;
};

class MessageClient {
public: 
    using Handler = std::function<void(const json&)>; 

    MessageClient(ProtocolClient& client)
        : notificationHandler(std::nullopt), protocolClient(client)
    {}
    ~MessageClient() = default;
    
    std::optional<json> register_user(const std::string& username, const std::string& password) const; 
    std::optional<json> login_user(const std::string& username, const std::string& password) const; 
    std::optional<json> send_message(const std::string& recipient, const std::string& content) const; 
    std::optional<json> logout_user () const; 
    std::optional<Handler> notificationHandler; 
    

private:
    std::optional<json> recv_command_response() const;
    ProtocolClient& protocolClient;
}; 


class CliApp {
public:
    CliApp(const std::string& host_, const int port_, const int timeoutMs_)
        : host(host_), port(port_), timeout(timeoutMs_)
    {}
    ~CliApp() = default;

    void run();

private: 
    std::string host; 
    int port;
    int timeout; 
    ProtocolClient protocolClient{ timeout, std::nullopt }; 
    MessageClient messageClient{ protocolClient };

    bool handle_command(const std::string& command);
    bool consume_socket_events(const bool block, const std::optional<int> timeoutMs);
    void display_response(std::optional<json> response); 
    void print_help() const noexcept
    {
        std::cout << "Available commands:" << std::endl;
        std::cout << "  register <username> <password>  - Register a new user" << std::endl;
        std::cout << "  login <username> <password>     - Login as a user" << std::endl;
        std::cout << "  send <recipient> <message>      - Send a message to a recipient" << std::endl;
        std::cout << "  logout                          - Logout the current user" << std::endl;
        std::cout << "  help                            - Show this help message" << std::endl;
        std::cout << "  exit                            - Exit the application" << std::endl;
    }
    void print_prompt() const noexcept
    {
        std::cout << "> " << std::flush;
    }
    void clear_prompt() const noexcept
    {
        std::cout << "\033[A\33[2K\r" << std::flush;
    }
}; 