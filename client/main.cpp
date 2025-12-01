#include "client.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char** argv)
{
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <host> <port> [timeout_ms]\n";
        return EXIT_FAILURE;
    }

    const std::string host = argv[1];
    const int port = std::stoi(argv[2]);
    const int timeout = (argc >= 4) ? std::stoi(argv[3]) : 5000;

    CliApp app(host, port, timeout);
    app.run();
    return EXIT_SUCCESS;
}
