#pragma once
#include "../shared/Protocol.h"
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <netinet/in.h>

// Minimal definitions for Bullet and Item:
struct Bullet {
    int x, y;
    int dx, dy;
};

struct Item {
    std::string type; // e.g., "food" or "bullet"
    int x, y;
};

class Room {
public:
    Room(const std::string &code);
    // Returns the player index (0 or 1) if added, or -1 if full.
    int addPlayer(int sock);
    void startThreads();
    void stopThreads();
    void handleInput(int playerIndex, const std::string &cmd);
    std::string getCode() const { return code; }
private:
    std::string code;
    int playerSockets[2];
    bool isPlayerSlotOccupied[2];
    int playerX[2], playerY[2];
    float playerHealth[2];
    int playerBullets[2];
    std::vector<Bullet> bullets;
    std::vector<Item> items;
    
    std::mutex roomMutex;
    std::thread bulletThread;
    std::thread itemThread;
    bool isActive;

    void bulletLoop();
    void itemLoop();
    bool moveAndCollideBullets();
    bool processCommand(int playerIndex, const std::string &cmd);
    void spawnItem();
    void broadcastState();
    // Utility function to send a message to a client.
    void sendToPlayer(int playerIndex, const std::string &msg);
};
