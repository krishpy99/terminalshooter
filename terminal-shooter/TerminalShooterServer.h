#ifndef TERMINALSHOOTERSERVER_H
#define TERMINALSHOOTERSERVER_H

#include "../server/BaseServer.h"
#include "TerminalShooterGame.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <string>
#include <vector>

// Enumeration to track per-client menu states.
enum class ClientMenuState {
    INITIAL,
    HOST_ROOM_CODE,
    JOIN_ROOM_CODE
};

enum class ServerState {
    MENU,
    IN_GAME,
    GAME_OVER
};

class TerminalShooterServer : public BaseServer {
public:
    TerminalShooterServer();
    virtual ~TerminalShooterServer();

protected:
    void acceptLoop() override;

private:
    // Single shooter game instance.
    TerminalShooterGame game_;

    // Global server state.
    ServerState state_;
    std::mutex stateMutex_;

    // Threads for broadcasting, bullet/collision updates, and power-up spawning.
    std::thread broadcastThread_;
    std::thread bulletThread_;
    std::thread powerUpThread_;
    std::atomic<bool> runningBroadcastLoop_;
    std::atomic<bool> runningBulletLoop_;
    std::atomic<bool> runningPowerUpLoop_;

    // Clients.
    std::vector<int> clients_;
    std::mutex clientsMutex_;

    // Per-client menu state.
    std::unordered_map<int, ClientMenuState> clientMenuStates_;
    std::mutex menuStateMutex_;

    // Game loop: broadcast board.
    void gameLoop();

    // Bullet update loop.
    void bulletLoop();

    // Power-up update loop.
    void powerUpLoop();

    // Accept input from each client.
    void clientHandler(int clientSocket);

    // Line-based partial buffer.
    std::unordered_map<int, std::string> partialBuf_;
    void processData(int clientSocket, const char* buf, int bytes);

    // Menu and game input handling.
    void handleMenuLine(int clientSocket, const std::string &line);
    void handleGameChar(int clientSocket, char c);

    // Broadcast.
	void broadcastGameStart();
    void broadcastBoard(const std::string &board);

    // Utility.
    void sendToClient(int clientSocket, const std::string &msg);
    void sendClearScreen(int clientSocket);

    void removeClient(int clientSocket);

    // Menu display functions.
    void showMenu(int clientSocket);
    void showHostRoomPrompt(int clientSocket);
    void showJoinRoomPrompt(int clientSocket);
    void showGameOverMenu(int clientSocket);

    // Helper to generate a random room code.
    std::string generateRandomRoomCode();
};

#endif
