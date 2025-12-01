#pragma once

#include <optional>
#include <string>

struct Command {
    enum class Type {
        Register,
        Login,
        SendMessage,
        Logout,
        Unknown
    };

    Type type {Type::Unknown};
    std::string username;
    std::string password;
    std::string recipient;
    std::string content;
};

struct Response {
    std::optional<bool> success;
    std::string command;
    std::string message;
    std::optional<std::string> payload;
    std::optional<std::string> sender;
    std::optional<std::string> recipient;
    std::optional<std::string> content;
    std::optional<std::string> timestamp;
};

// Responsible for translating protocol client and server side commands.
class ProtocolHandler {
public:
    Command parseCommand(const std::string& json) const;
    std::string serializeResponse(const Response& response) const;
};