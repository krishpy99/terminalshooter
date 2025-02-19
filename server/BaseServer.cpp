#include "BaseServer.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>

BaseServer::BaseServer()
    : serverSocket_(-1),
      port_(0),
      running_(false)
{}

BaseServer::~BaseServer() {
    stop();
}

void BaseServer::start(int port) {
    if (running_) {
        std::cout << "[BaseServer] Already running.\n";
        return;
    }
    port_ = port;
    running_ = true;

    setupSocket();
    if (serverSocket_ < 0) {
        std::cerr << "[BaseServer] Failed to set up socket.\n";
        running_ = false;
        return;
    }

    launchAcceptLoop();
    std::cout << "[BaseServer] Listening on port " << port_ << std::endl;
}

void BaseServer::stop() {
    if (!running_) return;
    running_ = false;

    // Close the server socket
    if (serverSocket_ != -1) {
        close(serverSocket_);
        serverSocket_ = -1;
    }

    // Wait for the accept loop to end
    if (acceptThread_.joinable()) {
        acceptThread_.join();
    }

    std::cout << "[BaseServer] Stopped.\n";
}

bool BaseServer::isRunning() const {
    return running_;
}

void BaseServer::setupSocket() {
    // Create the socket
    serverSocket_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        perror("socket");
        return;
    }

    // Reuse address
    int opt = 1;
    setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind
    sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    address.sin_port = htons(port_);

    if (::bind(serverSocket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind");
        close(serverSocket_);
        serverSocket_ = -1;
        return;
    }

    // Listen
    if (::listen(serverSocket_, 10) < 0) {
        perror("listen");
        close(serverSocket_);
        serverSocket_ = -1;
        return;
    }
}

void BaseServer::launchAcceptLoop() {
    acceptThread_ = std::thread([this]() {
        acceptLoop(); // call the derived class implementation
    });
}
