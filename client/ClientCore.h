#ifndef CLIENTCORE_H
#define CLIENTCORE_H

#include <string>

class ClientCore {
public:
    ClientCore();
    ~ClientCore();

    // Connect to the server given an IP and port.
    bool connectToServer(const std::string &ip, int port);

    // Main loop: sends user input and receives server messages.
    void mainLoop();

private:
    int serverSocket;
    bool connected;
    void handleServerMessage(const std::string &msg);
};

#endif 