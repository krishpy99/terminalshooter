#ifndef GAME_H
#define GAME_H

#include <vector>
#include <mutex>
#include <sstream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <iostream>

using namespace std;

const int BOARD_ROWS = 20;
const int BOARD_COLS = 40;

enum PowerUpType {
    NONE,
    BULLET,
    FOOD
};

struct Point {
    int x, y;
    Point() : x(0), y(0) {}
    Point(int x, int y) : x(x), y(y) {}
};

struct Player {
    int id;
    char rep; // Representation (for example, a digit)
    Point pos;
    float health;   // MODIFICATION: changed to float, initial health will be set later
    int bullets;    // MODIFICATION: initial bullet count set to 25
};

struct Bullet {
    int playerId; // Shooter’s ID
    Point pos;
    Point dir;    // Direction vector (vertical only: up or down)
};

struct PowerUp {
    PowerUpType type;
    Point pos;
};

class Board {
public:
    vector<vector<char>> grid;
    mutex m;
    
    Board() {
        grid = vector<vector<char>>(BOARD_ROWS, vector<char>(BOARD_COLS, ' '));
    }
    
    void clear() {
        lock_guard<mutex> lock(m);
        for (int i = 0; i < BOARD_ROWS; i++) {
            for (int j = 0; j < BOARD_COLS; j++) {
                grid[i][j] = ' ';
            }
        }
    }
    
    void drawBoundaries() {
        lock_guard<mutex> lock(m);
        for (int i = 0; i < BOARD_ROWS; i++) {
            grid[i][0] = 'X';
            grid[i][BOARD_COLS-1] = 'X';
        }
        for (int j = 0; j < BOARD_COLS; j++) {
            grid[0][j] = 'X';
            grid[BOARD_ROWS-1][j] = 'X';
        }
    }
    
    // Serialize the board as a string so it can be sent to clients.
    // We later append the player stats as extra lines.
    string serialize() {
        lock_guard<mutex> lock(m);
        ostringstream oss;
        for (int i = 0; i < BOARD_ROWS; i++) {
            for (int j = 0; j < BOARD_COLS; j++) {
                oss << grid[i][j];
            }
            oss << "\n";
        }
        return oss.str();
    }
};

#endif // GAME_H
