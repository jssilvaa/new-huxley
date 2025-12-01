#include "client.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <optional>
#include <vector>
#include <nlohmann/json.hpp>
#include <string>
#include <sstream>
#include <cstdint>
#include <array>
#include <algorithm>

using json = nlohmann::json;

void ProtocolClient::connect(const std::string &host, const int port)
{
    if (socketFd.has_value())
    {
        std::cerr << "Already connected" << std::endl;
        return;
    }

    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *result = nullptr;
    const std::string portStr = std::to_string(port);
    const int status = ::getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result);
    if (status != 0)
    {
        std::cerr << "Failed to resolve host " << host << ": " << ::gai_strerror(status) << std::endl;
        return;
    }

    int sock = -1;
    for (struct addrinfo *rp = result; rp != nullptr; rp = rp->ai_next)
    {
        sock = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock < 0)
        {
            continue;
        }
        if (::connect(sock, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            socketFd = sock;
            break;
        }
        ::close(sock);
        sock = -1;
    }

    ::freeaddrinfo(result);

    if (!socketFd.has_value())
    {
        std::cerr << "Unable to connect to " << host << ":" << port << std::endl;
        if (sock >= 0)
        {
            ::close(sock);
        }
    }
}

void ProtocolClient::close()
{
    if (socketFd.has_value())
    {
        ::close(socketFd.value());
        socketFd.reset();
    }
}

void ProtocolClient::send_command(const json &command) const
{
    if (!socketFd.has_value())
    {
        return;
    }

    const std::string payload = command.dump();
    std::uint32_t len = static_cast<std::uint32_t>(payload.size());
    std::uint32_t len_net = htonl(len);

    // Send 4-byte length header
    ::send(socketFd.value(), &len_net, sizeof(len_net), 0);
    // Send payload
    ::send(socketFd.value(), payload.data(), payload.size(), 0);
}

std::optional<json> ProtocolClient::receive_response() const
{
    if (!socketFd.has_value())
    {
        return std::nullopt;
    }

    // receive header
    const auto headerOpt = recv_exactly(4);
    if (!headerOpt.has_value())
    {
        return std::nullopt;
    }

    const std::string &header = headerOpt.value();
    std::uint32_t netlen = 0;
    std::memcpy(&netlen, header.data(), sizeof(netlen));
    const std::uint32_t len = ntohl(netlen);
    if (len == 0)
    {
        return std::nullopt;
    }

    const auto payloadOpt = recv_exactly(len);
    if (!payloadOpt.has_value())
    {
        return std::nullopt;
    }

    const std::string &payload = payloadOpt.value();
    try
    {
        auto response = json::parse(payload);
        return response;
    }
    catch (const json::parse_error &)
    {
        return std::nullopt;
    }
}

std::optional<std::string> ProtocolClient::recv_exactly(const size_t size) const
{
    if (!socketFd.has_value())
    {
        return std::nullopt;
    }

    std::string buffer;
    buffer.resize(size);

    size_t total_received = 0;
    while (total_received < size)
    {
        ssize_t received = ::recv(socketFd.value(), &buffer[total_received], size - total_received, 0);
        if (received <= 0)
        {
            return std::nullopt;
        }
        total_received += static_cast<size_t>(received);
    }

    return buffer;
}

std::optional<json> MessageClient::register_user(const std::string &username, const std::string &password) const
{
    json command;
    command["type"] = "register";
    command["username"] = username;
    command["password"] = password;
    protocolClient.send_command(command);
    return recv_command_response();
}

std::optional<json> MessageClient::login_user(const std::string &username, const std::string &password) const
{
    json command;
    command["type"] = "login";
    command["username"] = username;
    command["password"] = password;
    protocolClient.send_command(command);
    return recv_command_response();
}

std::optional<json> MessageClient::send_message(const std::string &recipient, const std::string &content) const
{
    json command;
    command["type"] = "send_message";
    command["recipient"] = recipient;
    command["content"] = content;
    protocolClient.send_command(command);
    return recv_command_response();
}

std::optional<json> MessageClient::logout_user() const
{
    json command;
    command["type"] = "logout";
    protocolClient.send_command(command);
    return recv_command_response();
}

std::optional<json> MessageClient::recv_command_response() const
{
    static const std::array<std::string, 3> ASYNC_COMMAND_TYPES = {"incoming_message", "incoming_message_response", "timeout"};

    while (true)
    {
        std::optional<json> response = protocolClient.receive_response();
        if (!response.has_value())
        {
            return std::nullopt;
        }

        const json &res = response.value();
        const std::string type = res.value("type", "unknown");
        const bool isAsync = std::find(ASYNC_COMMAND_TYPES.begin(), ASYNC_COMMAND_TYPES.end(), type) != ASYNC_COMMAND_TYPES.end();
        const bool hasSuccess = res.contains("success");

        if (isAsync || !hasSuccess)
        {
            if (notificationHandler.has_value())
            {
                notificationHandler.value()(res);
            }
            continue;
        }
        return response;
    }
}

inline std::vector<std::string> splitCommandLine(const std::string &cmdLine)
{
    std::istringstream iss(cmdLine);
    std::vector<std::string> tokens;
    std::string tok;
    while (iss >> tok)
    {
        tokens.push_back(tok);
    }
    return tokens;
}

bool CliApp::handle_command(const std::string &cmdline)
{
    const std::vector<std::string> tokens = splitCommandLine(cmdline);
    if (tokens.empty())
    {
        return true;
    }
    const std::string cmd = tokens[0];
    if (cmd == "/help")
    {
        print_help();
    }
    else if (cmd == "/exit")
    {
        return false;
    }
    else if (cmd == "/register" && tokens.size() == 3)
    {
        display_response(messageClient.register_user(tokens[1], tokens[2]));
    }
    else if (cmd == "/login" && tokens.size() == 3)
    {
        display_response(messageClient.login_user(tokens[1], tokens[2]));
    }
    else if (cmd == "/logout")
    {
        display_response(messageClient.logout_user());
    }
    else if (cmd == "/send" && tokens.size() >= 3)
    {
        const std::string recipient = tokens[1];
        const std::string content = cmdline.substr(cmdline.find(recipient) + recipient.size() + 1);
        try
        {
            display_response(messageClient.send_message(recipient, content));
        }
        catch (const std::exception &e)
        {
            std::cout << "Error sending message: " << e.what() << std::endl;
        }
    }
    else if (cmd == "/whoami")
    {
        std::cout << "Connected to " << host << ":" << port << std::endl;
    }
    else
    {
        std::cout << "Unknown command. Type /help for a list of commands." << std::endl;
    }
    return true;
}

void CliApp::display_response(std::optional<json> response)
{
    clear_prompt();
    if (!response.has_value())
    {
        std::cout << "No response from server." << std::endl;
        return;
    }
    const json &obj = response.value();
    std::string type = obj.value("type", "unknown");
    if (type == "ERROR")
    {
        std::cout << "Error: " << obj.value("message", "Unknown error") << "\n";
        return;
    }

    const std::string cmd = obj.value("command", "");
    const bool success = obj.value("success", false);

    if (cmd == "register")
    {
        std::cout << (success ? "[SUCCESS] User registered successfully.\n"
                              : "[FAILURE] Registration failed.\n");
    }
    else if (cmd == "login")
    {
        std::cout << (success ? "[SUCCESS] Logged in successfully.\n"
                              : "[FAILURE] Login failed.\n");
    }
    else if (cmd == "send_message")
    {
        std::cout << (success ? "[SUCCESS] Message sent.\n"
                              : "[FAILURE] Message sending failed.\n");
    }
    else if (cmd == "logout")
    {
        std::cout << (success ? "[SUCCESS] Logged out successfully.\n"
                              : "[FAILURE] Logout failed.\n");
    }
    else
    {
        std::cout << "Response: " << obj.dump() << "\n";
    }
}

bool CliApp::consume_socket_events(const bool block, const std::optional<int> timeoutMs)
{
    std::optional<int> socketFd = protocolClient.getSocket();
    if (!socketFd.has_value())
    {
        return false;
    }

    bool rcv_any = false;
    bool first_wait = true;
    while (true)
    {
        std::optional<int> wait_timeout = std::nullopt;
        if (block && first_wait && timeoutMs.has_value())
        {
            wait_timeout = timeoutMs.value();
        }
        else
        {
            wait_timeout = 0;
        }
        first_wait = false;

        if (wait_timeout && wait_timeout.value() < 0)
        {
            wait_timeout = 0;
        }

        // wait for socket readiness with select
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(socketFd.value(), &readfds);
        struct timeval tv;
        struct timeval *tv_ptr = nullptr;
        if (wait_timeout.has_value())
        {
            tv.tv_sec = wait_timeout.value() / 1000;
            tv.tv_usec = (wait_timeout.value() % 1000) * 1000;
            tv_ptr = &tv;
        }
        int sel;
        do
        {
            sel = ::select(socketFd.value() + 1, &readfds, nullptr, nullptr, tv_ptr);
        } while (sel == -1 && errno == EINTR);
        if (sel == -1)
        {
            std::perror("select");
            break;
        }
        if (sel == 0)
        {
            break;
        }

        if (FD_ISSET(socketFd.value(), &readfds))
        {
            rcv_any = true;
            std::optional<json> response = protocolClient.receive_response();
            if (response.has_value())
            {
                std::string type = response.value().value("type", "unknown");
                if (type == "incoming_message" && messageClient.notificationHandler.has_value())
                {
                    messageClient.notificationHandler.value()(response.value());
                }
                else
                {
                    clear_prompt();
                    display_response(response);
                }
            }
        }
        else
        {
            break;
        }
    }

    return rcv_any;
}

void CliApp::run()
{
    try
    {
        protocolClient.connect(host, port);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Failed to connect to server: " << e.what() << std::endl;
        return;
    }

    std::cout << "Connected to " << host << ":" << port << ". Type /help for commands." << std::endl;

    while (true)
    {
        print_prompt();
        // wait for inputs from stdin and socket

        fd_set rfds;
        FD_ZERO(&rfds);
        int maxfd = 0;

        // stdin
        FD_SET(STDIN_FILENO, &rfds);
        if (STDIN_FILENO > maxfd)
        {
            maxfd = STDIN_FILENO;
        }

        // socket
        std::optional<int> sock = protocolClient.getSocket();
        if (!sock.has_value())
        {
            std::cerr << "Socket closed." << std::endl;
            break;
        }
        FD_SET(sock.value(), &rfds);
        if (sock.value() > maxfd)
        {
            maxfd = sock.value();
        }

        int ret;
        do
        {
            ret = ::select(maxfd + 1, &rfds, nullptr, nullptr, nullptr);
        } while (ret == -1 && errno == EINTR);
        if (ret == -1)
        {
            std::perror("select");
            break;
        }

        // give priority to socket events
        if (FD_ISSET(sock.value(), &rfds))
        {
            consume_socket_events(false, std::nullopt);
        }

        // handle stdin
        if (FD_ISSET(STDIN_FILENO, &rfds))
        {
            std::string line;
            if (!std::getline(std::cin, line))
            {
                std::cout << "Exiting." << std::endl;
                break;
            }

            if (line.empty())
            {
                continue;
            }

            if (!line.empty() && line[0] != '/')
            {
                std::cout << "Invalid command. Commands must start with '/'." << std::endl;
                continue;
            }

            if (!handle_command(line))
            {
                std::cout << "\nExiting." << std::endl;
                break;
            }

            consume_socket_events(false, std::nullopt);
        }
    }

    protocolClient.close();
}