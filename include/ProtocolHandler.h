#pragma once 
#include <string> 

struct Command{
    std::string type; 
    std::string payload; 
};

class ProtocolHandler{
public:
    Command parseCommand(const std::string& json); 
    std::string serializeResponse(Command result); 
}; 