#ifndef SERVERCORE_H
#define SERVERCORE_H

#include <thread>
#include <map>
#include <mutex>
#include <atomic>
#include <memory>

class Room;       // Forward declaration
class RoomManager;

class ServerCore {
public:
    ServerCore(int port);
    ~ServerCore();

    // Start listening for clients.
    void start();

    // Stop the server.
    void stop();

private:
    int serverSocket;
    int port;
    std::atomic<bool> running;
    std::mutex clientsMutex;
    std::map<int, std::thread> clientThreads; // key: client socket
    std::unique_ptr<RoomManager> roomManager;

    // Accept incoming client connections.
    void acceptClients();

    // Handle communication with a connected client.
    void handleClient(int clientSocket);
};

#endif 