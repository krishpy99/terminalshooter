#include "TerminalShooterServer.h"
#include <iostream>

int main() {
    TerminalShooterServer server;
    server.start(12345); // start listening on port 12345

    std::cout << "Press Enter to stop the server...\n";
    std::cin.get(); // wait for user input

    server.stop();
    return 0;
}
