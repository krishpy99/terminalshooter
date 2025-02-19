#ifndef GAME_H
#define GAME_H

#include <string>
#include <vector>
#include <sstream>
#include <mutex>
using namespace std;

const int BOARD_ROWS = 20;
const int BOARD_COLS = 40;

struct Point {
    int x, y;
    Point(int _x=0, int _y=0) : x(_x), y(_y) {}
};

struct Player {
    int id;         // Player id (e.g., 1 or 2)
    char rep;       // Character representation ('1' or '2')
    float health;   // Health (starting at 10)
    int bullets;    // Number of bullets available
    Point pos;      // Current position on board
};

struct Bullet {
    int playerId;   // Shooter’s id
    Point pos;      // Current position
    Point dir;      // Direction (for simplicity, vertical only)
};

enum PowerUpType { BULLET, FOOD };

struct PowerUp {
    Point pos;
    PowerUpType type;
};

class Board {
public:
    vector<string> grid;
    Board() { grid.resize(BOARD_ROWS, string(BOARD_COLS, ' ')); }
    void clear() {
        for(auto &line : grid)
            line.assign(BOARD_COLS, ' ');
    }
    void drawBoundaries() {
        grid[0] = string(BOARD_COLS, 'X');
        grid[BOARD_ROWS-1] = string(BOARD_COLS, 'X');
        for (int i = 1; i < BOARD_ROWS-1; i++) {
            grid[i][0] = 'X';
            grid[i][BOARD_COLS-1] = 'X';
        }
    }
    string serialize() {
        ostringstream oss;
        for(auto &line : grid)
            oss << line << "\n";
        return oss.str();
    }
};

#endif