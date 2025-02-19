#ifndef TERMINALSHOOTERGAME_H
#define TERMINALSHOOTERGAME_H

#include "IGame.h"
#include <vector>
#include <string>
#include <mutex>

struct PlayerState {
    int x;
    int y;
    float health;
    int bullets;
};

enum class ItemType {
    FOOD,
    BULLET
};

struct Item {
    int x;
    int y;
    ItemType type;
};

class TerminalShooterGame : public IGame {
public:
    TerminalShooterGame();

    // Process input from a player (commands like "MOVE W" or "SHOOT")
    void processInput(int playerId, const std::string &input) override;

    // Update game logic (movement, collisions, spawn items).
    void update(float deltaTime) override;

    // Get game state as a string to broadcast to clients.
    std::string getCurrentState() override;

    // Check if the game is over.
    bool isGameOver() const override;

private:
    std::vector<PlayerState> players;
    std::vector<Item> items;
    bool gameOver;
    std::mutex gameMutex;
    const int gridWidth = 20;
    const int gridHeight = 20;
    float spawnTimer; // Timer used for item spawning.

    void spawnItems();
    void checkItemPickup();
    void handleShooting(int shooterId);
};

#endif 