#ifndef TERMINALSHOOTERROOM_H
#define TERMINALSHOOTERROOM_H

#include "Room.h"
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <memory>
#include "../game/IGame.h"

class TerminalShooterRoom : public Room, public std::enable_shared_from_this<TerminalShooterRoom> {
public:
    TerminalShooterRoom(const std::string &roomCode);
    ~TerminalShooterRoom();

    // Adds a client to this room. Returns false if the room is full.
    bool addClient(int sock) override;

    // Called to pass client input commands to the game.
    void handleClientMessage(int sock, const std::string &msg) override;

    // Runs the game loop (not directly called because we run our own thread).
    void runGameLoop() override;

private:
    const int MAX_PLAYERS = 2;
    std::string roomCode;
    std::vector<int> clientSockets;
    std::mutex roomMutex;
    std::thread gameThread;
    std::atomic<bool> active;
    std::unique_ptr<IGame> game;

    // The main game loop running at a fixed tick-rate.
    void gameLoop();

    // Returns the player ID corresponding to a client socket.
    int getPlayerId(int sock);
};

#endif 