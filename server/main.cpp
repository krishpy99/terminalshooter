#include "Server.h"
#include <cstdlib>

int main(int argc, char* argv[]) {
    int port = 12345; // default port
    if (argc > 1) {
        port = std::atoi(argv[1]);
    }
    Server server(port);
    server.start();
    return 0;
}