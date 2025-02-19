#include "TerminalShooterGame.h"
#include <sstream>
#include <cstdlib>
#include <ctime>

TerminalShooterGame::TerminalShooterGame() : gameOver(false), spawnTimer(0.0f)
{
    // Initialize random seed.
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // Initialize two players:
    players.resize(2);
    players[0] = { 1, 1, 10.0f, 25 };
    players[1] = { gridWidth - 2, gridHeight - 2, 10.0f, 25 };
}

void TerminalShooterGame::processInput(int playerId, const std::string &input) {
    std::lock_guard<std::mutex> lock(gameMutex);
    if (gameOver) return;
    if (playerId < 0 || playerId >= static_cast<int>(players.size()))
        return;

    if (input.find("MOVE") == 0) {
        // Expected format: "MOVE <direction>"
        if (input.length() >= 7) {
            char direction = input[5];
            switch (direction) {
                case 'W': case 'w':
                    if (players[playerId].y > 0)
                        players[playerId].y--;
                    break;
                case 'A': case 'a':
                    if (players[playerId].x > 0)
                        players[playerId].x--;
                    break;
                case 'S': case 's':
                    if (players[playerId].y < gridHeight - 1)
                        players[playerId].y++;
                    break;
                case 'D': case 'd':
                    if (players[playerId].x < gridWidth - 1)
                        players[playerId].x++;
                    break;
                default:
                    break;
            }
            // After moving, check for pickup items.
            checkItemPickup();
        }
    } else if (input == "SHOOT") {
        if (players[playerId].bullets > 0) {
            players[playerId].bullets--;
            handleShooting(playerId);
        }
    }
}

void TerminalShooterGame::update(float deltaTime) {
    std::lock_guard<std::mutex> lock(gameMutex);
    if (gameOver) return;
    spawnTimer += deltaTime;
    if (spawnTimer >= 2.0f) { // Every 2 seconds, spawn an item.
        spawnItems();
        spawnTimer = 0.0f;
    }
    // Check game over conditions.
    if (players[0].health <= 0 && players[1].health <= 0) {
        gameOver = true;
    } else if (players[0].health <= 0 || players[1].health <= 0) {
        gameOver = true;
    }
}

std::string TerminalShooterGame::getCurrentState() {
    std::lock_guard<std::mutex> lock(gameMutex);
    std::ostringstream oss;
    oss << "P1(" << players[0].x << "," << players[0].y
        << ",health=" << players[0].health
        << ",bullets=" << players[0].bullets << ") ";
    oss << "P2(" << players[1].x << "," << players[1].y
        << ",health=" << players[1].health
        << ",bullets=" << players[1].bullets << ") ";
    oss << "Items(";
    for (size_t i = 0; i < items.size(); i++) {
        oss << "(" << items[i].x << "," << items[i].y << ",";
        oss << (items[i].type == ItemType::FOOD ? "FOOD" : "BULLET");
        oss << ")";
        if (i < items.size() - 1)
            oss << " ";
    }
    oss << ")";
    return oss.str();
}

bool TerminalShooterGame::isGameOver() const {
    return gameOver;
}

void TerminalShooterGame::spawnItems() {
    // Randomly spawn an item.
    int x = std::rand() % gridWidth;
    int y = std::rand() % gridHeight;
    Item newItem;
    newItem.x = x;
    newItem.y = y;
    newItem.type = (std::rand() % 2 == 0) ? ItemType::FOOD : ItemType::BULLET;
    items.push_back(newItem);
}

void TerminalShooterGame::checkItemPickup() {
    for (int p = 0; p < 2; p++) {
        for (auto it = items.begin(); it != items.end(); ) {
            if (players[p].x == it->x && players[p].y == it->y) {
                if (it->type == ItemType::FOOD) {
                    players[p].health += 0.5f;
                } else {
                    players[p].bullets += 1;
                }
                // Remove the item after pickup.
                it = items.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void TerminalShooterGame::handleShooting(int shooterId) {
    int targetId = (shooterId == 0) ? 1 : 0;
    // If opponent is in the same row or column, decrement health.
    if (players[shooterId].x == players[targetId].x ||
        players[shooterId].y == players[targetId].y) {
        players[targetId].health -= 1.0f;
    }
} 