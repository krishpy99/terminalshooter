#include "TerminalShooterServer.h"
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <random>
#include <mutex>
#include <thread>

using namespace std;

TerminalShooterServer::TerminalShooterServer()
    : BaseServer(),
      state_(ServerState::MENU),
      runningBulletLoop_(true),
      runningPowerUpLoop_(true)
{
    // Removed broadcastThread_ since we now broadcast on state changes.
}

TerminalShooterServer::~TerminalShooterServer() {
    stop();
    runningBulletLoop_ = false;
    runningPowerUpLoop_ = false;
    if (bulletThread_.joinable()) {
        bulletThread_.join();
    }
    if (powerUpThread_.joinable()) {
        powerUpThread_.join();
    }
}

void TerminalShooterServer::acceptLoop() {
    // Launch the bullet and power-up threads.
    bulletThread_ = thread([this]() { bulletLoop(); });
    powerUpThread_ = thread([this]() { powerUpLoop(); });

    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    while (isRunning()) {
        int newSock = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientLen);
        if (newSock < 0) {
            this_thread::sleep_for(chrono::milliseconds(100));
            continue;
        }
        {
            lock_guard<mutex> lock(clientsMutex_);
            clients_.push_back(newSock);
        }
        {
            lock_guard<mutex> lock(menuStateMutex_);
            // For clients connecting in MENU state, set initial menu state.
            if (state_ == ServerState::MENU) {
                clientMenuStates_[newSock] = ClientMenuState::INITIAL;
                showMenu(newSock);
            } else if (state_ == ServerState::IN_GAME) {
                // For clients joining an already running game.
                clientMenuStates_[newSock] = ClientMenuState::JOIN_ROOM_CODE;
                showJoinRoomPrompt(newSock);
            } else if (state_ == ServerState::GAME_OVER) {
                showGameOverMenu(newSock);
            }
        }
        thread t(&TerminalShooterServer::clientHandler, this, newSock);
        t.detach();
    }
    // Signal threads to stop.
    runningBulletLoop_ = false;
    runningPowerUpLoop_ = false;
    if (bulletThread_.joinable()) bulletThread_.join();
    if (powerUpThread_.joinable()) powerUpThread_.join();
}

void TerminalShooterServer::bulletLoop() {
    while (isRunning() && runningBulletLoop_) {
        if (state_ == ServerState::IN_GAME) {
            game_.updateBulletsAndCollisions();
            // Broadcast board immediately after bullet updates.
            broadcastBoard(game_.getSerializedBoard());
        }
        this_thread::sleep_for(chrono::milliseconds(10));
    }
}

void TerminalShooterServer::powerUpLoop() {
    while (isRunning() && runningPowerUpLoop_) {
        if (state_ == ServerState::IN_GAME) {
            game_.updatePowerUps();
            // Broadcast board immediately after power-up updates.
            broadcastBoard(game_.getSerializedBoard());
        }
        this_thread::sleep_for(chrono::milliseconds(50));
    }
}

void TerminalShooterServer::clientHandler(int clientSocket) {
    cout << "[TerminalShooterServer] Client " << clientSocket << " connected.\n";
    char buf[256];
    while (true) {
        memset(buf, 0, sizeof(buf));
        int bytes = recv(clientSocket, buf, 255, 0);
        if (bytes <= 0) {
            removeClient(clientSocket);
            break;
        }
        processData(clientSocket, buf, bytes);
    }
}

void TerminalShooterServer::processData(int clientSocket, const char* buf, int bytes) {
    lock_guard<mutex> lock(stateMutex_);
    if (state_ == ServerState::MENU || state_ == ServerState::GAME_OVER) {
        auto &partial = partialBuf_[clientSocket];
        for (int i = 0; i < bytes; i++) {
            char c = buf[i];
            if (c == '\n') {
                handleMenuLine(clientSocket, partial);
                partial.clear();
            } else if (c == '\r') {
                // ignore carriage return
            } else {
                partial.push_back(c);
            }
        }
    } else if (state_ == ServerState::IN_GAME) {
        {
            lock_guard<mutex> lockMenu(menuStateMutex_);
            if (clientMenuStates_.find(clientSocket) != clientMenuStates_.end() &&
                (clientMenuStates_[clientSocket] == ClientMenuState::HOST_ROOM_CODE ||
                 clientMenuStates_[clientSocket] == ClientMenuState::JOIN_ROOM_CODE)) {
                auto &partial = partialBuf_[clientSocket];
                for (int i = 0; i < bytes; i++) {
                    char c = buf[i];
                    if (c == '\n') {
                        handleMenuLine(clientSocket, partial);
                        partial.clear();
                    } else if (c == '\r') {
                        // ignore
                    } else {
                        partial.push_back(c);
                    }
                }
                return;
            }
        }
        // Process in-game single-char commands.
        for (int i = 0; i < bytes; i++) {
            char c = buf[i];
            if (c == '\n' || c == '\r') continue;
            handleGameChar(clientSocket, c);
        }
    }
}

void TerminalShooterServer::handleMenuLine(int clientSocket, const std::string &line) {
    lock_guard<mutex> lock(menuStateMutex_);
    if (clientMenuStates_.find(clientSocket) == clientMenuStates_.end()) {
        clientMenuStates_[clientSocket] = ClientMenuState::INITIAL;
    }
    ClientMenuState currentState = clientMenuStates_[clientSocket];

    if (currentState == ClientMenuState::INITIAL) {
        if (line == "1") {
            clientMenuStates_[clientSocket] = ClientMenuState::HOST_ROOM_CODE;
            showHostRoomPrompt(clientSocket);
        } else if (line == "2") {
            clientMenuStates_[clientSocket] = ClientMenuState::JOIN_ROOM_CODE;
            showJoinRoomPrompt(clientSocket);
        } else {
            sendClearScreen(clientSocket);
            sendToClient(clientSocket, "Invalid choice. Try again.");
            showMenu(clientSocket);
        }
    } else if (currentState == ClientMenuState::HOST_ROOM_CODE) {
        string roomCode = line;
        if (roomCode.empty()) {
            roomCode = generateRandomRoomCode();
        }
        game_.setRoomCode(roomCode);
        // Add the hosting player.
        PlayerConnection pc;
        pc.socket = clientSocket;
        pc.name = "Host";
        game_.addPlayer(pc);
        clientMenuStates_.erase(clientSocket);
        state_ = ServerState::IN_GAME;
        sendClearScreen(clientSocket);
        sendToClient(clientSocket, "Hosting game with Room Code: " + roomCode);
        broadcastGameStart();
        // Broadcast the updated board.
        broadcastBoard(game_.getSerializedBoard());
    } else if (currentState == ClientMenuState::JOIN_ROOM_CODE) {
        string enteredCode = line;
        string activeCode = game_.getRoomCode();
        if (enteredCode == activeCode) {
            // Add the joining player.
            PlayerConnection pc;
            pc.socket = clientSocket;
            pc.name = "Guest";
            game_.addPlayer(pc);
            clientMenuStates_.erase(clientSocket);
            sendClearScreen(clientSocket);
            sendToClient(clientSocket, "Joined game with Room Code: " + enteredCode);
            broadcastGameStart();
            broadcastBoard(game_.getSerializedBoard());
        } else {
            sendClearScreen(clientSocket);
            sendToClient(clientSocket, "No such room exists. Please enter a valid Room Code:");
            // Remain in JOIN_ROOM_CODE state.
        }
    }
}

void TerminalShooterServer::handleGameChar(int clientSocket, char c) {
    string cmd(1, c);
    // Log the key press.
    cout << "[TerminalShooterServer] Player " << clientSocket << " pressed \"" << cmd << "\"\n";
    // Process the input in the game.
    game_.handleMessage(clientSocket, cmd);
    // Broadcast the updated board immediately after processing the key.
    broadcastBoard(game_.getSerializedBoard());
    // Optionally, if a player types 'x', remove them.
    if (c == 'x') {
        removeClient(clientSocket);
    }
}

void TerminalShooterServer::broadcastBoard(const std::string &board) {
    // Use a simple repositioning of the cursor to refresh the client's view.
    static const char* clearCmd = "\033[H";
    lock_guard<mutex> lock(clientsMutex_);
    for (int sock : clients_) {
        send(sock, clearCmd, strlen(clearCmd), 0);
        send(sock, board.c_str(), board.size(), 0);
    }
}

void TerminalShooterServer::showMenu(int clientSocket) {
    std::string m =
        "=== Terminal Shooter ===\n"
        "1) Host Game\n"
        "2) Join Game\n"
        "Enter choice: ";
    sendToClient(clientSocket, m);
}

void TerminalShooterServer::showHostRoomPrompt(int clientSocket) {
    std::string m =
        "Enter room code (optional, leave blank for random): ";
    sendToClient(clientSocket, m);
}

void TerminalShooterServer::showJoinRoomPrompt(int clientSocket) {
    std::string m =
        "Enter room code (required): ";
    sendToClient(clientSocket, m);
}

void TerminalShooterServer::showGameOverMenu(int clientSocket) {
    std::string m =
        "Game Over!\n"
        "1) Play again\n"
        "2) Return to main menu\n"
        "Enter choice: ";
    sendToClient(clientSocket, m);
}

void TerminalShooterServer::sendToClient(int clientSocket, const std::string &msg) {
    ::send(clientSocket, msg.c_str(), msg.size(), 0);
    ::send(clientSocket, "\n", 1, 0);
}

void TerminalShooterServer::sendClearScreen(int clientSocket) {
    static const char* clr = "\033[H";
    ::send(clientSocket, clr, strlen(clr), 0);
}

void TerminalShooterServer::removeClient(int clientSocket) {
    {
        lock_guard<mutex> lock(clientsMutex_);
        clients_.erase(
            remove(clients_.begin(), clients_.end(), clientSocket),
            clients_.end()
        );
    }
    game_.removePlayer(clientSocket);
    {
        lock_guard<mutex> lock(menuStateMutex_);
        clientMenuStates_.erase(clientSocket);
    }
    close(clientSocket);
    cout << "[TerminalShooterServer] Client " << clientSocket << " disconnected.\n";
}

std::string TerminalShooterServer::generateRandomRoomCode() {
    const char alphanum[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int len = 6;
    string code;
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, sizeof(alphanum) - 2);
    for (int i = 0; i < len; ++i) {
        code.push_back(alphanum[dis(gen)]);
    }
    return code;
}

void TerminalShooterServer::broadcastGameStart() {
    lock_guard<mutex> lock(clientsMutex_);
    for (int sock : clients_) {
        sendToClient(sock, "GAME_START");
    }
}
