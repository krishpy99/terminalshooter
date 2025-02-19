#include "ClientCore.h"
#include "../shared/NetworkUtils.h"
#include "../shared/Logger.h"
#include <iostream>
#include <thread>
#include <atomic>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <unistd.h>
#endif

ClientCore::ClientCore() : serverSocket(-1), connected(false) {}

ClientCore::~ClientCore() {
    if (connected) {
#ifdef _WIN32
        closesocket(serverSocket);
#else
        close(serverSocket);
#endif
    }
}

bool ClientCore::connectToServer(const std::string &ip, int port) {
#ifdef _WIN32
    // Windows-specific socket initialization would go here.
#endif
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        Logger::logError("Failed to create socket.");
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
#ifdef _WIN32
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());
#else
    if(inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0) {
        Logger::logError("Invalid address or address not supported.");
        return false;
    }
#endif

    if (connect(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        Logger::logError("Connection to server failed.");
        return false;
    }
    connected = true;
    return true;
}

void ClientCore::handleServerMessage(const std::string &msg) {
    std::cout << "Server: " << msg << std::endl;
}

void ClientCore::mainLoop() {
    std::atomic<bool> running(true);
    std::thread recvThread([&](){
        std::string line;
        while (running) {
            if (NetworkUtils::recvLine(serverSocket, line)) {
                handleServerMessage(line);
            } else {
                Logger::logError("Disconnected from server.");
                running = false;
            }
        }
    });

    std::string input;
    while (running) {
        std::getline(std::cin, input);
        if (input.empty()) continue;
        if (!NetworkUtils::sendLine(serverSocket, input)) {
            Logger::logError("Failed to send message to server.");
            running = false;
            break;
        }
    }
    running = false;
    if (recvThread.joinable())
        recvThread.join();
} 