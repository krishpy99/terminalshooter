#ifndef TERMINALSHOOTERGAME_H
#define TERMINALSHOOTERGAME_H

#include "../game/IGame.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>

static const int BOARD_ROWS = 20;
static const int BOARD_COLS = 40;

enum class PowerUpType {
    BULLET,
    FOOD
};

struct Point {
    int x, y;
    Point() : x(0), y(0) {}
    Point(int xx, int yy) : x(xx), y(yy) {}
};

struct Bullet {
    int ownerSocket;
    Point pos;
    Point dir; // up or down
};

struct PowerUp {
    PowerUpType type;
    Point pos;
};

struct ShooterPlayer {
    int socket;
    std::string name;
    float health;
    int bullets;
    Point pos;
    char rep; // e.g. '1','2','X'
};

class TerminalShooterGame : public IGame {
public:
    TerminalShooterGame();
    virtual ~TerminalShooterGame() {}

    // IGame interface
    void setRoomCode(const std::string &code) override;
    std::string getRoomCode() const override;
    void addPlayer(const PlayerConnection &playerConn) override;
    void removePlayer(int socket) override;
    bool isGameOver() const override;

    // Handle single-char input (WASD + space)
    void handleMessage(int senderSocket, const std::string &msg);

    // Split update functionalities into dedicated functions:
    void updateBulletsAndCollisions();
    void updatePowerUps();

    // Original update method (no longer used)
    void update();

    void resetGame();
    
    // Build a textual representation of the board + player stats and room code
    std::string getSerializedBoard();

    // Return all sockets
    std::vector<int> getConnectedSockets();

private:
    mutable std::mutex gameMutex_;
    std::string roomCode_;
    std::atomic<bool> gameOver_;

    std::unordered_map<int, ShooterPlayer> players_;
    int nextPlayerIndex_;

    std::vector<Bullet> bullets_;
    std::vector<PowerUp> powerUps_;

    std::chrono::steady_clock::time_point lastPowerUpSpawn_;

    void moveBullets();
    void checkCollisions();
    void spawnPowerUpIfNeeded();
};

#endif
