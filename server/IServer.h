#ifndef ISERVER_H
#define ISERVER_H

class IServer {
public:
    virtual ~IServer() = default;

    // Start listening on the specified port
    virtual void start(int port) = 0;

    // Stop listening and shut down
    virtual void stop() = 0;

    // Check if the server is running
    virtual bool isRunning() const = 0;
};

#endif // ISERVER_H
