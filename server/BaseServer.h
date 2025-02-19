#ifndef BASESERVER_H
#define BASESERVER_H

#include "IServer.h"
#include <thread>
#include <atomic>

class BaseServer : public IServer {
public:
    BaseServer();
    virtual ~BaseServer();

    // IServer interface
    void start(int port) override;
    void stop() override;
    bool isRunning() const override;

protected:
    // The server socket file descriptor
    int serverSocket_;
    // Port on which we listen
    int port_;
    // Thread that runs acceptLoop()
    std::thread acceptThread_;
    // Atomic flag indicating if the server is up
    std::atomic<bool> running_;

    // Setup the socket (create, bind, listen)
    virtual void setupSocket();

    // Launch a new thread calling acceptLoop()
    virtual void launchAcceptLoop();

    // The function that continuously accepts client connections.
    // This is pure virtual so that derived classes must implement their own logic.
    virtual void acceptLoop() = 0;
};

#endif
