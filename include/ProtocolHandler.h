#pragma once

#include <optional>
#include <string>
#include <nlohmann/json.hpp>

struct Command {
    enum class Type {
        Register,
        Login,
        SendMessage,
        Logout,
        ListUsers,
        ListOnline,
        GetHistory,
        Unknown
    };

    Type type {Type::Unknown};
    std::string username;
    std::string password;
    std::string recipient;
    std::string content;
    std::string targetUser;
    int limit {50};
    int offset {0};
};

struct Response {
    std::optional<bool> success;
    std::string command;
    std::string message;
    std::optional<nlohmann::json> payload;
    std::optional<int> id;
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
