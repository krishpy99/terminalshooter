#include "TerminalShooterRoom.h"
#include "../game/TerminalShooterGame.h"
#include "../shared/NetworkUtils.h"
#include "../shared/Logger.h"
#include "../shared/Protocol.h"
#include <chrono>
#include <thread>

TerminalShooterRoom::TerminalShooterRoom(const std::string &roomCode) : roomCode(roomCode), active(true) {
    game = std::make_unique<TerminalShooterGame>();
    // Start the game loop thread immediately.
    gameThread = std::thread(&TerminalShooterRoom::gameLoop, this);
}

TerminalShooterRoom::~TerminalShooterRoom() {
    active = false;
    if (gameThread.joinable())
        gameThread.join();
}

bool TerminalShooterRoom::addClient(int sock) {
    std::lock_guard<std::mutex> lock(roomMutex);
    if (clientSockets.size() >= MAX_PLAYERS)
        return false;
    clientSockets.push_back(sock);
    Logger::logInfo("Client added to room " + roomCode);
    return true;
}

void TerminalShooterRoom::handleClientMessage(int sock, const std::string &msg) {
    int playerId = getPlayerId(sock);
    if (playerId < 0)
        return;
    game->processInput(playerId, msg);
}

void TerminalShooterRoom::runGameLoop() {
    // Not used directly—the game loop runs in its own thread.
}

int TerminalShooterRoom::getPlayerId(int sock) {
    std::lock_guard<std::mutex> lock(roomMutex);
    for (int i = 0; i < static_cast<int>(clientSockets.size()); ++i) {
        if (clientSockets[i] == sock)
            return i;
    }
    return -1;
}

void TerminalShooterRoom::gameLoop() {
    const int tickRate = 30;
    const std::chrono::milliseconds tickDuration(1000 / tickRate);

    while (active && !game->isGameOver()) {
        auto start = std::chrono::steady_clock::now();
        game->update(1.0f / tickRate);
        std::string state = game->getCurrentState();

        // Broadcast the state to all clients.
        std::lock_guard<std::mutex> lock(roomMutex);
        for (int sock : clientSockets) {
            NetworkUtils::sendLine(sock, Protocol::CMD_STATE + " " + state);
        }

        auto end = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(tickDuration - (end - start));
    }

    // Notify clients that the game has ended.
    std::lock_guard<std::mutex> lock(roomMutex);
    for (int sock : clientSockets) {
        NetworkUtils::sendLine(sock, Protocol::CMD_GAME_END + " Game Over");
    }
    active = false;
} 