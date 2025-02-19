#include "ServerCore.h"
#include "Room.h"
#include "RoomManager.h"
#include "../shared/NetworkUtils.h"
#include "../shared/Logger.h"
#include "../shared/Protocol.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <cstdlib>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <unistd.h>
#endif

ServerCore::ServerCore(int port) : serverSocket(-1), port(port), running(false) {
    roomManager = std::make_unique<RoomManager>();
}

ServerCore::~ServerCore() {
    stop();
#ifdef _WIN32
    closesocket(serverSocket);
#else
    if (serverSocket != -1) {
        close(serverSocket);
    }
#endif
}

void ServerCore::start() {
#ifdef _WIN32
    // Windows-specific initialization if needed.
#endif
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        Logger::logError("Failed to create server socket.");
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        Logger::logError("Failed to bind server socket.");
        return;
    }

    if (listen(serverSocket, 5) < 0) {
        Logger::logError("Failed to listen on server socket.");
        return;
    }

    running = true;
    Logger::logInfo("Server started, waiting for clients...");
    std::thread(&ServerCore::acceptClients, this).detach();

    // Main thread keeps the server running.
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void ServerCore::stop() {
    running = false;
    // Additional cleanup can be added here.
}

void ServerCore::acceptClients() {
    while (running) {
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            continue;
        }
        Logger::logInfo("New client connected.");

        std::lock_guard<std::mutex> lock(clientsMutex);
        clientThreads[clientSocket] = std::thread(&ServerCore::handleClient, this, clientSocket);
        clientThreads[clientSocket].detach();
    }
}

void ServerCore::handleClient(int clientSocket) {
    std::string line;
    // Send a welcome message.
    if (!NetworkUtils::sendLine(clientSocket, "Welcome to Terminal Shooter Server!")) {
        Logger::logError("Failed to send welcome message.");
    }

    // Handshake loop: client can send CREATE_ROOM or JOIN_ROOM.
    while (running && NetworkUtils::recvLine(clientSocket, line)) {
        auto tokens = Protocol::split(line, ' ');
        if (tokens.empty()) continue;

        if (tokens[0] == Protocol::CMD_CREATE_ROOM) {
            std::string roomCode = roomManager->createRoom();
            NetworkUtils::sendLine(clientSocket, Protocol::CMD_ROOM_CODE + " " + roomCode);
            // Auto-add client to the room.
            auto room = roomManager->joinRoom(roomCode);
            if (room) {
                if (room->addClient(clientSocket)) {
                    // Now the room will manage further messages.
                    break;
                } else {
                    NetworkUtils::sendLine(clientSocket, Protocol::CMD_ROOM_FULL);
                }
            }
        } else if (tokens[0] == Protocol::CMD_JOIN_ROOM) {
            if (tokens.size() >= 2) {
                std::string roomCode = tokens[1];
                auto room = roomManager->joinRoom(roomCode);
                if (room) {
                    if (room->addClient(clientSocket)) {
                        NetworkUtils::sendLine(clientSocket, Protocol::CMD_ROOM_JOINED);
                        break;
                    } else {
                        NetworkUtils::sendLine(clientSocket, Protocol::CMD_ROOM_FULL);
                    }
                } else {
                    NetworkUtils::sendLine(clientSocket, Protocol::CMD_NO_SUCH_ROOM);
                }
            }
        } else {
            NetworkUtils::sendLine(clientSocket, "Unrecognized command.");
        }
    }

    // After joining a room, the room is expected to handle game-related messages.
    while (running && NetworkUtils::recvLine(clientSocket, line)) {
        // Optionally, you can pass further messages to the room.
    }

    Logger::logInfo("Client disconnected.");
#ifdef _WIN32
    closesocket(clientSocket);
#else
    close(clientSocket);
#endif
} 