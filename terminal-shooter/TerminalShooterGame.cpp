#include "TerminalShooterGame.h"
#include <iostream>
#include <algorithm>
#include <random>
#include <sstream>
#include <chrono>
#include <cstdlib>
#include <ctime>

using namespace std;

TerminalShooterGame::TerminalShooterGame()
    : roomCode_(""), gameOver_(false), nextPlayerIndex_(1),
      lastPowerUpSpawn_(std::chrono::steady_clock::now())
{
    srand(time(nullptr));
}

void TerminalShooterGame::setRoomCode(const std::string &code) {
    lock_guard<mutex> lock(gameMutex_);
    roomCode_ = code;
}

std::string TerminalShooterGame::getRoomCode() const {
    lock_guard<mutex> lock(gameMutex_);
    return roomCode_;
}

void TerminalShooterGame::addPlayer(const PlayerConnection &playerConn) {
    lock_guard<mutex> lock(gameMutex_);
    ShooterPlayer sp;
    sp.socket  = playerConn.socket;
    sp.name    = playerConn.name;
    sp.health  = 10.0f;
    sp.bullets = 25;

    // Basic logic for first 2 players; additional players placed in center.
    if (players_.empty()) {
        sp.pos = Point(1, BOARD_COLS/2);
        sp.rep = '1';
    } else if (players_.size() == 1) {
        sp.pos = Point(BOARD_ROWS-2, BOARD_COLS/2);
        sp.rep = '2';
    } else {
        sp.pos = Point(BOARD_ROWS/2, BOARD_COLS/2);
        sp.rep = 'X';
    }

    players_[playerConn.socket] = sp;
    nextPlayerIndex_++;
    cout << "[TerminalShooterGame] Player " << sp.socket
         << " joined, health=" << sp.health << ", bullets=" << sp.bullets << endl;
}

void TerminalShooterGame::removePlayer(int socket) {
    lock_guard<mutex> lock(gameMutex_);
    players_.erase(socket);
    if (players_.empty()) {
        gameOver_ = true; // if no players remain, the game is over
    }
}

bool TerminalShooterGame::isGameOver() const {
    return gameOver_.load();
}

void TerminalShooterGame::handleMessage(int senderSocket, const std::string &msg) {
    lock_guard<mutex> lock(gameMutex_);
    auto it = players_.find(senderSocket);
    if (it == players_.end()) return;

    ShooterPlayer &p = it->second;

    // Single-char commands: movement and shooting
    if (msg == "w") {
        if (p.pos.x > 1) p.pos.x--;
    } else if (msg == "s") {
        if (p.pos.x < BOARD_ROWS-2) p.pos.x++;
    } else if (msg == "a") {
        if (p.pos.y > 1) p.pos.y--;
    } else if (msg == "d") {
        if (p.pos.y < BOARD_COLS-2) p.pos.y++;
    } else if (msg == " ") {
        // Shoot: decrease bullet count and add a bullet moving upward
        if (p.bullets > 0) {
            p.bullets--;
            Bullet b;
            b.ownerSocket = senderSocket;
            b.pos = p.pos;
            b.dir = Point(-1, 0);
            bullets_.push_back(b);
        }
    }
}

void TerminalShooterGame::moveBullets() {
    for (auto &b : bullets_) {
        b.pos.x += b.dir.x;
        b.pos.y += b.dir.y;
    }
    // Remove bullets that have gone off the board.
    bullets_.erase(
        remove_if(bullets_.begin(), bullets_.end(),
            [&](const Bullet &bu) {
                return (bu.pos.x <= 0 || bu.pos.x >= BOARD_ROWS-1 ||
                        bu.pos.y <= 0 || bu.pos.y >= BOARD_COLS-1);
            }),
        bullets_.end());
}

void TerminalShooterGame::checkCollisions() {
    // Check bullet collisions with players.
    for (auto it = bullets_.begin(); it != bullets_.end();) {
        bool hit = false;
        for (auto &kv : players_) {
            ShooterPlayer &pl = kv.second;
            if (pl.health > 0 && pl.pos.x == it->pos.x && pl.pos.y == it->pos.y) {
                pl.health -= 1.0f;
                hit = true;
                break;
            }
        }
        if (hit) {
            it = bullets_.erase(it);
        } else {
            ++it;
        }
    }
    // Check for player collision with power-ups.
    for (auto &kv : players_) {
        ShooterPlayer &pl = kv.second;
        if (pl.health <= 0) continue;
        for (auto puIt = powerUps_.begin(); puIt != powerUps_.end();) {
            if (pl.pos.x == puIt->pos.x && pl.pos.y == puIt->pos.y) {
                if (puIt->type == PowerUpType::BULLET) {
                    pl.bullets++;
                } else {
                    pl.health += 0.5f;
                    if (pl.health > 10.f) pl.health = 10.f;
                }
                puIt = powerUps_.erase(puIt);
            } else {
                ++puIt;
            }
        }
    }
}

void TerminalShooterGame::spawnPowerUpIfNeeded() {
    if (powerUps_.size() >= 5) return;
    auto now = chrono::steady_clock::now();
    auto diff = chrono::duration_cast<chrono::seconds>(now - lastPowerUpSpawn_);
    if (diff.count() < 5) return;
    lastPowerUpSpawn_ = now;

    PowerUp pu;
    pu.pos.x = rand() % (BOARD_ROWS - 2) + 1;
    pu.pos.y = rand() % (BOARD_COLS - 2) + 1;
    pu.type = (rand() % 2 == 0) ? PowerUpType::BULLET : PowerUpType::FOOD;
    powerUps_.push_back(pu);
}

void TerminalShooterGame::updateBulletsAndCollisions() {
    lock_guard<mutex> lock(gameMutex_);
    moveBullets();
    checkCollisions();
}

void TerminalShooterGame::updatePowerUps() {
    lock_guard<mutex> lock(gameMutex_);
    spawnPowerUpIfNeeded();
}

void TerminalShooterGame::update() {
    // This method is kept for interface compatibility but is no longer used.
    // Functionality has been split into updateBulletsAndCollisions and updatePowerUps.
}

std::string TerminalShooterGame::getSerializedBoard() {
    lock_guard<mutex> lock(gameMutex_);
    // Create a 2D board initialized with spaces.
    vector<string> lines(BOARD_ROWS, string(BOARD_COLS, ' '));
    // Draw boundaries.
    for (int r = 0; r < BOARD_ROWS; r++) {
        lines[r][0] = 'X';
        lines[r][BOARD_COLS - 1] = 'X';
    }
    for (int c = 0; c < BOARD_COLS; c++) {
        lines[0][c] = 'X';
        lines[BOARD_ROWS - 1][c] = 'X';
    }
    // Place players on the board.
    for (auto &kv : players_) {
        ShooterPlayer &pl = kv.second;
        if (pl.health > 0) {
            lines[pl.pos.x][pl.pos.y] = pl.rep;
        }
    }
    // Place bullets.
    for (auto &b : bullets_) {
        if (b.pos.x >= 0 && b.pos.x < BOARD_ROWS &&
            b.pos.y >= 0 && b.pos.y < BOARD_COLS) {
            lines[b.pos.x][b.pos.y] = '|';
        }
    }
    // Place power-ups.
    for (auto &pu : powerUps_) {
        if (pu.pos.x > 0 && pu.pos.x < BOARD_ROWS - 1 &&
            pu.pos.y > 0 && pu.pos.y < BOARD_COLS - 1) {
            char c = (pu.type == PowerUpType::BULLET) ? 'B' : 'F';
            lines[pu.pos.x][pu.pos.y] = c;
        }
    }
    // Build the board string.
    ostringstream oss;
    for (int r = 0; r < BOARD_ROWS; r++) {
        oss << lines[r] << "\n";
    }
    oss << "\n";
    // Append player stats.
    for (auto &kv : players_) {
        ShooterPlayer &pl = kv.second;
        oss << "Player " << pl.socket << " (" << pl.rep << ")"
            << " HP=" << pl.health
            << " Bullets=" << pl.bullets << "\n";
    }
    // Always show the room code.
    oss << "Room Code: " << roomCode_ << "\n";
    return oss.str();
}

std::vector<int> TerminalShooterGame::getConnectedSockets() {
    lock_guard<mutex> lock(gameMutex_);
    vector<int> out;
    for (auto &kv : players_) {
        out.push_back(kv.first);
    }
    return out;
}

void TerminalShooterGame::resetGame() {
    lock_guard<mutex> lock(gameMutex_);
    gameOver_ = false;
    players_.clear();
    bullets_.clear();
    powerUps_.clear();
    nextPlayerIndex_ = 1;
    lastPowerUpSpawn_ = std::chrono::steady_clock::now();
}
