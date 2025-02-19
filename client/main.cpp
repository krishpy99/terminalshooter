#include "ClientCore.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: client <server ip> <port>" << std::endl;
        return 1;
    }
    ClientCore client;
    std::string ip = argv[1];
    int port = std::stoi(argv[2]);
    if (!client.connectToServer(ip, port)) {
        return 1;
    }
    client.mainLoop();
    return 0;
} 