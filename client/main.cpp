#include "Client.h"
#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[]) {
    std::string serverIp = "127.0.0.1";
    int port = 12345;
    if(argc > 1) serverIp = argv[1];
    if(argc > 2) port = std::atoi(argv[2]);
    
    Client client;
    if(!client.connectToServer(serverIp, port)) {
        std::cerr << "Failed to connect to server\n";
        return EXIT_FAILURE;
    }
    client.mainMenu();
    return 0;
}