#include "Room.h"
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <unistd.h>

Room::Room(const std::string &code)
    : code(code), isActive(true) {
    for (int i = 0; i < 2; i++) {
        playerSockets[i] = -1;
        isPlayerSlotOccupied[i] = false;
        playerX[i] = rand() % 20;
        playerY[i] = rand() % 20;
        playerHealth[i] = 10.0f;
        playerBullets[i] = 25;
    }
}

int Room::addPlayer(int sock) {
    std::lock_guard<std::mutex> lk(roomMutex);
    for (int i = 0; i < 2; i++) {
        if (!isPlayerSlotOccupied[i]) {
            playerSockets[i] = sock;
            isPlayerSlotOccupied[i] = true;
            return i;
        }
    }
    return -1;
}

void Room::startThreads() {
    bulletThread = std::thread(&Room::bulletLoop, this);
    itemThread   = std::thread(&Room::itemLoop, this);
}

void Room::stopThreads() {
    isActive = false;
    if (bulletThread.joinable())
        bulletThread.join();
    if (itemThread.joinable())
        itemThread.join();
}

void Room::bulletLoop() {
    while (isActive) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        bool changed = false;
        {
            std::lock_guard<std::mutex> lk(roomMutex);
            changed = moveAndCollideBullets();
            if (changed) {
                broadcastState();
            }
        }
    }
}

void Room::itemLoop() {
    while (isActive) {
        int spawnDelay = rand() % 3000 + 2000; // 2-5 seconds
        std::this_thread::sleep_for(std::chrono::milliseconds(spawnDelay));
        {
            std::lock_guard<std::mutex> lk(roomMutex);
            spawnItem();
            broadcastState();
        }
    }
}

bool Room::moveAndCollideBullets() {
    bool changed = false;
    for (size_t i = 0; i < bullets.size();) {
        // Move bullet
        bullets[i].x += bullets[i].dx;
        bullets[i].y += bullets[i].dy;
        // Check for out-of-bound (assuming map size 20x20)
        if(bullets[i].x < 0 || bullets[i].x >= 20 || bullets[i].y < 0 || bullets[i].y >= 20) {
            bullets.erase(bullets.begin() + i);
            changed = true;
            continue;
        }
        // Check collision with players
        for (int p = 0; p < 2; p++) {
            if(isPlayerSlotOccupied[p] && bullets[i].x == playerX[p] && bullets[i].y == playerY[p]) {
                playerHealth[p] -= 1.0f;
                bullets.erase(bullets.begin() + i);
                changed = true;
                // End game if player's health falls below or equals 0
                if(playerHealth[p] <= 0) {
                    broadcastState();
                    // Inform both players that game is over
                    for (int j = 0; j < 2; j++) {
                        if(isPlayerSlotOccupied[j])
                            sendToPlayer(j, REPLY_GAME_OVER + " Player " + std::to_string(p) + " died\n");
                    }
                    isActive = false;
                }
                goto next_bullet;
            }
        }
        i++;
    next_bullet:
        ;
    }
    return changed;
}

bool Room::processCommand(int playerIndex, const std::string &cmd) {
    bool changed = false;
    std::istringstream iss(cmd);
    std::string token;
    iss >> token;
    if (token == CMD_MOVE) {
        std::string dir;
        iss >> dir;
        if(dir == "W") { playerY[playerIndex] = std::max(0, playerY[playerIndex]-1); changed = true; }
        else if(dir == "S") { playerY[playerIndex] = std::min(19, playerY[playerIndex]+1); changed = true; }
        else if(dir == "A") { playerX[playerIndex] = std::max(0, playerX[playerIndex]-1); changed = true; }
        else if(dir == "D") { playerX[playerIndex] = std::min(19, playerX[playerIndex]+1); changed = true; }
    } else if (token == CMD_SHOOT) {
        if(playerBullets[playerIndex] > 0) {
            // For simplicity, shoot in a fixed direction (e.g., upward)
            Bullet b;
            b.x = playerX[playerIndex];
            b.y = playerY[playerIndex];
            b.dx = 0;
            b.dy = -1;
            bullets.push_back(b);
            playerBullets[playerIndex]--;
            changed = true;
        }
    }
    return changed;
}

void Room::handleInput(int playerIndex, const std::string &cmd) {
    std::lock_guard<std::mutex> lk(roomMutex);
    bool changed = processCommand(playerIndex, cmd);
    if(changed) {
        broadcastState();
    }
}

void Room::spawnItem() {
    // Randomly place an item if there is no item at that location.
    Item it;
    it.x = rand() % 20;
    it.y = rand() % 20;
    // For simplicity alternate between two types.
    it.type = (rand() % 2 == 0) ? "food" : "bullet";
    items.push_back(it);
}

void Room::broadcastState() {
    // Compose state message:
    std::ostringstream oss;
    oss << REPLY_STATE << " ";
    for (int p = 0; p < 2; p++) {
        if(isPlayerSlotOccupied[p])
            oss << "P" << p << "(x=" << playerX[p] 
                << ",y=" << playerY[p] 
                << ",HP=" << playerHealth[p]
                << ",bullets=" << playerBullets[p] << ") ";
    }
    oss << "Items:";
    for (auto &it : items) {
        oss << " (type=" << it.type << ",x=" << it.x << ",y=" << it.y << ")";
    }
    oss << " Bullets:";
    for (auto &b : bullets) {
        oss << " (x=" << b.x << ",y=" << b.y << ")";
    }
    oss << "\n";
    std::string stateMsg = oss.str();
    // Send state to both players
    for (int p = 0; p < 2; p++) {
        if(isPlayerSlotOccupied[p])
            sendToPlayer(p, stateMsg);
    }
}

void Room::sendToPlayer(int playerIndex, const std::string &msg) {
    if(playerSockets[playerIndex] != -1) {
        write(playerSockets[playerIndex], msg.c_str(), msg.size());
    }
}