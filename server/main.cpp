#include "ServerCore.h"
#include <iostream>  // For std::cerr
#include <string>

int main(int argc, char *argv[]) {
    // Ensure that there is one command line argument for the port.
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }
    
    // Convert the command-line argument to an integer.
    int port = std::stoi(argv[1]);
    
    // Initialize your server components using the port.
    ServerCore server(port);
    server.start();  // Assuming start() starts the server's main loop
    return 0;
}