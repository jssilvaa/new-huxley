#include "ProtocolHandler.h"
#include <nlohmann/json.hpp>

Command ProtocolHandler::parseCommand(const std::string& json) const
{
    Command command;
    nlohmann::json payload;
    try {
        payload = nlohmann::json::parse(json);
    } catch (const nlohmann::json::exception&) {
        command.type = Command::Type::Unknown;
        return command;
    }

    const std::string typeString = payload.value("type", std::string{});
    std::string upperType = typeString;
    std::transform(upperType.begin(), upperType.end(), upperType.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });

    if (upperType == "REGISTER") {
        command.type = Command::Type::Register;
    } else if (upperType == "LOGIN") {
        command.type = Command::Type::Login;
    } else if (upperType == "SEND_MESSAGE") {
        command.type = Command::Type::SendMessage;
    } else if (upperType == "LOGOUT") {
        command.type = Command::Type::Logout;
    } else {
        command.type = Command::Type::Unknown;
    }

    command.username  = payload.value("username", std::string{});
    command.password  = payload.value("password", std::string{});
    command.recipient = payload.value("recipient", std::string{});
    command.content   = payload.value("content", std::string{});

    return command;
}

std::string ProtocolHandler::serializeResponse(const Response& response) const
{
    nlohmann::json jsonResponse {
        {"success", response.success},
        {"command", response.command},
        {"message", response.message}
    };

    if (response.payload) {
        jsonResponse["payload"] = *response.payload;
    }

    return jsonResponse.dump() + "\n";
}
