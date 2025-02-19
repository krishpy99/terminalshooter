#pragma once
#include "Room.h"
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

class Server {
public:
    Server(int port);
    void start();
    Room* createRoom();
    Room* getRoom(const std::string &code);
    void removeRoom(const std::string &code);
private:
    int m_port;
    int m_serverSocket;
    std::mutex m_roomsMutex;
    std::map<std::string, std::unique_ptr<Room>> m_rooms;
    void acceptLoop();
    void handleClient(int clientSocket);
    // Utility function to generate a random room code.
    std::string generateRoomCode();
};