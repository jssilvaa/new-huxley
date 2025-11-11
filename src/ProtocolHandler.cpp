#include "ProtocolHandler.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string_view>

namespace {
std::string trim(std::string_view text)
{
    auto begin = text.find_first_not_of(" \r\n\t");
    if (begin == std::string_view::npos) {
        return {};
    }
    auto end = text.find_last_not_of(" \r\n\t");
    return std::string{text.substr(begin, end - begin + 1)};
}

std::string extractStringField(std::string_view json, const std::string& key)
{
    const std::string pattern = '"' + key + '"';
    const auto keyPos = json.find(pattern);
    if (keyPos == std::string_view::npos) {
        return {};
    }

    const auto colonPos = json.find(':', keyPos + pattern.size());
    if (colonPos == std::string_view::npos) {
        return {};
    }

    const auto quoteStart = json.find('"', colonPos + 1);
    if (quoteStart == std::string_view::npos) {
        return {};
    }

    std::string value;
    bool escape = false;
    for (std::size_t i = quoteStart + 1; i < json.size(); ++i) {
        const char c = json[i];
        if (escape) {
            switch (c) {
            case 'n': value.push_back('\n'); break;
            case 't': value.push_back('\t'); break;
            case 'r': value.push_back('\r'); break;
            case '\\': value.push_back('\\'); break;
            case '"': value.push_back('"'); break;
            default: value.push_back(c); break;
            }
            escape = false;
            continue;
        }
        if (c == '\\') {
            escape = true;
            continue;
        }
        if (c == '"') {
            break;
        }
        value.push_back(c);
    }

    return value;
}

std::string escapeJson(const std::string& text)
{
    std::ostringstream oss;
    for (char c : text) {
        switch (c) {
        case '"': oss << "\\\""; break;
        case '\\': oss << "\\\\"; break;
        case '\n': oss << "\\n"; break;
        case '\t': oss << "\\t"; break;
        default: oss << c; break;
        }
    }
    return oss.str();
}
} // namespace

Command ProtocolHandler::parseCommand(const std::string& json) const
{
    Command command;
    const std::string body = trim(json);
    const std::string typeString = extractStringField(body, "type");

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

    command.username = extractStringField(body, "username");
    command.password = extractStringField(body, "password");
    command.recipient = extractStringField(body, "recipient");
    command.content = extractStringField(body, "content");

    return command;
}

std::string ProtocolHandler::serializeResponse(const Response& response) const
{
    std::ostringstream oss;
    oss << '{';
    oss << "\"success\":" << (response.success ? "true" : "false") << ',';
    oss << "\"command\":\"" << escapeJson(response.command) << "\",";
    oss << "\"message\":\"" << escapeJson(response.message) << "\"";
    if (response.payload) {
        oss << ",\"payload\":\"" << escapeJson(*response.payload) << "\"";
    }
    oss << "}\n";
    return oss.str();
}
