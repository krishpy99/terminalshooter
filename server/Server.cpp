#include "Server.h"
#include "../shared/Protocol.h"
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

Server::Server(int port) : m_port(port), m_serverSocket(-1) {}

std::string Server::generateRoomCode() {
    // simple 4-digit code
    int num = rand() % 9000 + 1000;
    return std::to_string(num);
}

Room* Server::createRoom() {
    std::lock_guard<std::mutex> lock(m_roomsMutex);
    std::string code = generateRoomCode();
    // Ensure unique code:
    while(m_rooms.find(code) != m_rooms.end()){
        code = generateRoomCode();
    }
    m_rooms[code] = std::unique_ptr<Room>(new Room(code));
    m_rooms[code]->startThreads();
    return m_rooms[code].get();
}

Room* Server::getRoom(const std::string &code) {
    std::lock_guard<std::mutex> lock(m_roomsMutex);
    if(m_rooms.find(code) != m_rooms.end()){
        return m_rooms[code].get();
    }
    return nullptr;
}

void Server::removeRoom(const std::string &code) {
    std::lock_guard<std::mutex> lock(m_roomsMutex);
    auto it = m_rooms.find(code);
    if(it != m_rooms.end()){
        it->second->stopThreads();
        m_rooms.erase(it);
    }
}

void Server::start() {
    m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(m_serverSocket < 0) {
        std::cerr << "Socket creation failed\n";
        exit(EXIT_FAILURE);
    }
    
    int opt = 1;
    setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(m_port);
    
    if(bind(m_serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Bind failed\n";
        exit(EXIT_FAILURE);
    }
    
    if(listen(m_serverSocket, 5) < 0) {
        std::cerr << "Listen failed\n";
        exit(EXIT_FAILURE);
    }
    
    std::cout << "Server listening on port " << m_port << "\n";
    acceptLoop();
}

void Server::acceptLoop() {
    while (true) {
        sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        int clientSocket = accept(m_serverSocket, (struct sockaddr*)&clientAddr, &addrLen);
        if(clientSocket < 0) {
            std::cerr << "Accept failed\n";
            continue;
        }
        std::thread(&Server::handleClient, this, clientSocket).detach();
    }
}

void Server::handleClient(int clientSocket) {
    char buffer[1024];
    // Read initial command: either "CREATE" or "JOIN <code>"
    int len = read(clientSocket, buffer, sizeof(buffer)-1);
    if(len <= 0) {
        close(clientSocket);
        return;
    }
    buffer[len] = '\0';
    std::istringstream iss(buffer);
    std::string command;
    iss >> command;
    
    Room* room = nullptr;
    int playerIndex = -1;
    if(command == CMD_CREATE) {
        room = createRoom();
        playerIndex = room->addPlayer(clientSocket);
        if(playerIndex >= 0) {
            // reply with room code
            std::string reply = REPLY_ROOM + " " + room->getCode() + "\n";
            write(clientSocket, reply.c_str(), reply.size());
        }
    } else if(command == CMD_JOIN) {
        std::string code;
        iss >> code;
        room = getRoom(code);
        if(room) {
            playerIndex = room->addPlayer(clientSocket);
            if(playerIndex >= 0) {
                std::string reply = REPLY_JOIN_OK + "\n";
                write(clientSocket, reply.c_str(), reply.size());
            } else {
                std::string reply = REPLY_JOIN_FAIL + "\n";
                write(clientSocket, reply.c_str(), reply.size());
                close(clientSocket);
                return;
            }
        } else {
            std::string reply = REPLY_JOIN_FAIL + "\n";
            write(clientSocket, reply.c_str(), reply.size());
            close(clientSocket);
            return;
        }
    } else {
        close(clientSocket);
        return;
    }
    
    // Main client loop: read commands and forward them to the room.
    while (true) {
        len = read(clientSocket, buffer, sizeof(buffer)-1);
        if(len <= 0) break;
        buffer[len] = '\0';
        std::string cmd(buffer);
        // Remove potential trailing newline.
        cmd.erase(cmd.find_last_not_of("\r\n")+1);
        room->handleInput(playerIndex, cmd);
    }
    close(clientSocket);
    // For simplicity, we leave room removal to later (or when game ends).
}
