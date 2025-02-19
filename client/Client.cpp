#include "Client.h"
#include "../shared/Protocol.h"
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

Client::Client() : m_serverSocket(-1), m_inGame(false) {}

Client::~Client() {
    if(m_serverSocket != -1) close(m_serverSocket);
    if(receiverThread.joinable()) receiverThread.join();
}

bool Client::connectToServer(const std::string &ip, int port) {
    m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(m_serverSocket < 0) {
        std::cerr << "Socket creation error\n";
        return false;
    }
    sockaddr_in serv_addr;
    std::memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if(inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address\n";
        return false;
    }
    if(connect(m_serverSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed\n";
        return false;
    }
    return true;
}

void Client::mainMenu() {
    int choice;
    std::string input;
    std::cout << "1) CREATE GAME\n2) JOIN GAME\n3) EXIT\n";
    std::getline(std::cin, input);
    choice = std::atoi(input.c_str());
    if(choice == 1) {
        std::string msg = CMD_CREATE + "\n";
        write(m_serverSocket, msg.c_str(), msg.size());
        char buffer[1024];
        int len = read(m_serverSocket, buffer, sizeof(buffer)-1);
        if(len > 0) {
            buffer[len] = '\0';
            std::cout << "Server: " << buffer;
        }
        m_inGame = true;
        gameLoop();
    } else if(choice == 2) {
        std::cout << "Enter room code: ";
        std::string code;
        std::getline(std::cin, code);
        std::string msg = CMD_JOIN + " " + code + "\n";
        write(m_serverSocket, msg.c_str(), msg.size());
        char buffer[1024];
        int len = read(m_serverSocket, buffer, sizeof(buffer)-1);
        if(len > 0) {
            buffer[len] = '\0';
            std::string reply(buffer);
            std::cout << "Server: " << reply;
            if(reply.find(REPLY_JOIN_OK) != std::string::npos) {
                m_inGame = true;
                gameLoop();
            } else {
                std::cout << "Join failed\n";
            }
        }
    } else {
        std::cout << "Exiting...\n";
    }
}

void Client::gameLoop() {
    // Start receiver thread to get state updates.
    receiverThread = std::thread(&Client::receiverFunc, this);
    std::cout << "Enter commands (MOVE <W/A/S/D>, SHOOT, EXIT):\n";
    while(m_inGame) {
        std::string line;
        std::getline(std::cin, line);
        if(line == "EXIT") {
            m_inGame = false;
            break;
        }
        line += "\n";
        write(m_serverSocket, line.c_str(), line.size());
    }
    if(receiverThread.joinable())
        receiverThread.join();
}

void Client::receiverFunc() {
    char buffer[1024];
    while(m_inGame) {
        int len = read(m_serverSocket, buffer, sizeof(buffer)-1);
        if(len <= 0) break;
        buffer[len] = '\0';
        std::string line(buffer);
        if(line.find(REPLY_STATE) == 0) {
            displayState(line);
        } else if(line.find(REPLY_GAME_OVER) == 0) {
            std::cout << "Game Over: " << line << "\n";
            m_inGame = false;
            break;
        } else {
            std::cout << "Server: " << line;
        }
    }
}

void Client::displayState(const std::string &stateLine) {
    // For simplicity, just print the state line.
    std::cout << stateLine;
}
