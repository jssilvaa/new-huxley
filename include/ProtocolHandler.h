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
    bool success {false};
    std::string command;
    std::string message;
    std::optional<std::string> payload;
};

// Responsible for translating protocol frames to/from strongly typed commands.
class ProtocolHandler {
public:
    Command parseCommand(const std::string& json) const;
    std::string serializeResponse(const Response& response) const;
};