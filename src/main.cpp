#include "HuxleyServer.h"

int main() {
    HuxleyServer server;
    if (server.start(8080)) {
        // TODO: loop or signal handling
    }
    server.stop();
    return 0;
}
