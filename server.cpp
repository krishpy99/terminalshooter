#include "game.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <algorithm>

using namespace std;

// Global game state and synchronization primitives
Board board;
mutex playersMutex, bulletsMutex, powerUpsMutex, clientsMutex;
vector<Player> players;
vector<Bullet> bullets;
vector<PowerUp> powerUps;
vector<int> clientSockets;
int nextPlayerId = 1;
bool gameOver = false;  // MODIFICATION: track game over state

// Helper function to append player stats to the board display.
string appendPlayerStats(const string &boardState) {
    ostringstream oss;
    oss << boardState;
    oss << "\n";
    {
        lock_guard<mutex> lock(playersMutex);
        for (auto &p : players) {
            oss << "Player " << p.id << " ("
                << p.rep << ") Health: " << p.health 
                << "   Bullets: " << p.bullets << "\n";
        }
    }
    return oss.str();
}

// Send the current board state (plus stats) to every connected client
void broadcastBoard(const string &state) {
    lock_guard<mutex> lock(clientsMutex);
    for (int sock : clientSockets) {
        send(sock, state.c_str(), state.size(), 0);
    }
}

// Handle input from a single client (each client is one player)
void clientHandler(int clientSocket) {
    // MODIFICATION: Refuse a third connection.
    {
        lock_guard<mutex> lock(playersMutex);
        if (players.size() >= 2) {
            const char* msg = "Game is full. Only 2 players allowed.\n";
            send(clientSocket, msg, strlen(msg), 0);
            close(clientSocket);
            return;
        }
    }
    
    // Assign a new player to this connection
    Player p;
    {
        lock_guard<mutex> lock(playersMutex);
        p.id = nextPlayerId++;
        // Represent the player by a digit (cycles after 9)
        p.rep = '0' + (p.id % 10);
        // MODIFICATION: Set starting positions at opposite ends.
        if (players.empty()) {
            // First player: top center
            p.pos.x = 1;
            p.pos.y = BOARD_COLS / 2;
        } else {
            // Second player: bottom center
            p.pos.x = BOARD_ROWS - 2;
            p.pos.y = BOARD_COLS / 2;
        }
        p.health = 10.0f;   // MODIFICATION: set initial health to 10
        p.bullets = 25;     // MODIFICATION: set initial bullet count to 25
        players.push_back(p);
    }
    cout << "Player " << p.id << " connected." << endl;
    
    char buffer[1024];
    while (true) {
        int bytes = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0)
            break;
        buffer[bytes] = '\0';
        // Process a single-character command from the client
        char command = buffer[0];
        // Lock players when updating movement/shooting
        lock_guard<mutex> lock(playersMutex);
        for (auto &player : players) {
            if (player.id == p.id) {
                // Movement commands (WASD) remain similar; note that shooting is only vertical
                if (command == 'w' || command == 'W') {
                    if (player.pos.x > 1)
                        player.pos.x--;
                } else if (command == 's' || command == 'S') {
                    if (player.pos.x < BOARD_ROWS - 2)
                        player.pos.x++;
                }
                // Left/right movement can be kept if desired.
                else if (command == 'a' || command == 'A') {
                    if (player.pos.y > 1)
                        player.pos.y--;
                } else if (command == 'd' || command == 'D') {
                    if (player.pos.y < BOARD_COLS - 2)
                        player.pos.y++;
                }
                else if (command == ' ') { // Shooting command
                    // MODIFICATION: Only shoot if the player has bullets.
                    if (player.bullets <= 0)
                        break;  // Do nothing if no bullets.
                    
                    // Find the opponent (only 2 players exist)
                    Player opponent;
                    bool foundOpponent = false;
                    for (auto &p2 : players) {
                        if (p2.id != player.id) {
                            opponent = p2;
                            foundOpponent = true;
                            break;
                        }
                    }
                    // If opponent exists, determine shooting direction.
                    Point bulletDir;
                    if (foundOpponent) {
                        // Only vertical shooting allowed.
                        if (opponent.pos.x > player.pos.x)
                            bulletDir = Point(1, 0);   // shoot downward
                        else
                            bulletDir = Point(-1, 0);  // shoot upward
                    } else {
                        // Default direction (if no opponent yet)
                        bulletDir = Point(-1, 0);
                    }
                    
                    // Consume a bullet
                    player.bullets--;
                    
                    // Create the bullet if a bullet is available
                    {
                        lock_guard<mutex> bulletLock(bulletsMutex);
                        Bullet b;
                        b.playerId = player.id;
                        b.pos = player.pos;
                        b.dir = bulletDir;
                        bullets.push_back(b);
                    }
                }
                break;
            }
        }
    }
    
    // Remove player on disconnect
    {
        lock_guard<mutex> lock(playersMutex);
        players.erase(remove_if(players.begin(), players.end(),
                   [p](const Player &pl) { return pl.id == p.id; }), players.end());
    }
    {
        lock_guard<mutex> lock(clientsMutex);
        clientSockets.erase(remove(clientSockets.begin(), clientSockets.end(), clientSocket), clientSockets.end());
    }
    close(clientSocket);
    cout << "Player " << p.id << " disconnected." << endl;
}

// Main game loop: updates bullets, handles collisions and power-up collection,
// refreshes the board, broadcasts the updated board state, and checks for game over.
void gameLoop() {
    while (!gameOver) {
        // Update bullet positions
        {
            lock_guard<mutex> lock(bulletsMutex);
            for (auto &b : bullets) {
                b.pos.x += b.dir.x;
                b.pos.y += b.dir.y;
            }
            // Remove bullets that leave the board
            bullets.erase(remove_if(bullets.begin(), bullets.end(), [](const Bullet &b) {
                return (b.pos.x <= 0 || b.pos.x >= BOARD_ROWS - 1 ||
                        b.pos.y <= 0 || b.pos.y >= BOARD_COLS - 1);
            }), bullets.end());
        }
        
        // Check for bullet collisions with players (excluding the shooter)
        {
            lock_guard<mutex> lock1(playersMutex);
            lock_guard<mutex> lock2(bulletsMutex);
            for (auto it = bullets.begin(); it != bullets.end();) {
                bool hit = false;
                for (auto &p : players) {
                    if (p.id != it->playerId && p.pos.x == it->pos.x && p.pos.y == it->pos.y) {
                        // MODIFICATION: reduce health by 1.0 for each hit.
                        p.health -= 1.0f;
                        hit = true;
                        break;
                    }
                }
                if (hit)
                    it = bullets.erase(it);
                else
                    ++it;
            }
        }
        
        // Check if any players collect power-ups automatically
        {
            lock_guard<mutex> lock1(playersMutex);
            lock_guard<mutex> lock2(powerUpsMutex);
            for (auto &p : players) {
                for (auto it = powerUps.begin(); it != powerUps.end();) {
                    if (p.pos.x == it->pos.x && p.pos.y == it->pos.y) {
                        if (it->type == BULLET) {
                            // MODIFICATION: bullet power-up gives +1 bullet
                            p.bullets += 1;
                        } else if (it->type == FOOD) {
                            // MODIFICATION: food power-up gives +0.5 health (cap at 10)
                            p.health += 0.5f;
                            if (p.health > 10.0f)
                                p.health = 10.0f;
                        }
                        it = powerUps.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
        }
        
        // Update the board
        board.clear();
        board.drawBoundaries();
        {
            lock_guard<mutex> lock(playersMutex);
            for (auto &p : players) {
                board.grid[p.pos.x][p.pos.y] = p.rep;
            }
        }
        {
            lock_guard<mutex> lock(bulletsMutex);
            for (auto &b : bullets) {
                board.grid[b.pos.x][b.pos.y] = '|';
            }
        }
        {
            lock_guard<mutex> lock(powerUpsMutex);
            for (auto &pu : powerUps) {
                if (pu.type == BULLET)
                    board.grid[pu.pos.x][pu.pos.y] = 'B';
                else if (pu.type == FOOD)
                    board.grid[pu.pos.x][pu.pos.y] = 'F';
            }
        }
        
        // Append player stats to the board display.
        string state = appendPlayerStats(board.serialize());
        broadcastBoard(state);
        
        // Check for game over (if any player's health is <= 0)
        {
            lock_guard<mutex> lock(playersMutex);
            if (players.size() == 2) {
                for (auto &p : players) {
                    if (p.health <= 0.0f) {
                        gameOver = true;
                        // Identify the winner (the other player)
                        int winnerId = 0;
                        for (auto &other : players) {
                            if (other.id != p.id)
                                winnerId = other.id;
                        }
                        string msg = "\nGame Over! Player " + to_string(winnerId) + " wins!\n";
                        broadcastBoard(msg);
                        return;
                    }
                }
            }
        }
        
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

// Periodically spawn a power-up (either bullet or food) at a random position
void spawnPowerUps() {
    while (!gameOver) {
        this_thread::sleep_for(chrono::seconds(5));
        {
            lock_guard<mutex> lock(powerUpsMutex);
            // MODIFICATION: Only spawn if there are less than 5 power-ups on the board.
            if (powerUps.size() >= 5)
                continue;
        }
        PowerUp pu;
        pu.pos.x = rand() % (BOARD_ROWS - 2) + 1;
        pu.pos.y = rand() % (BOARD_COLS - 2) + 1;
        pu.type = (rand() % 2 == 0) ? BULLET : FOOD;
        {
            lock_guard<mutex> lock(powerUpsMutex);
            powerUps.push_back(pu);
        }
    }
}

void runServer() {
    srand(time(0));
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    // Create the server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(12345);
    
    if (::bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    cout << "Server started on port 12345." << endl;
    
    // Start the game update and power-up spawning threads
    thread gameThread(gameLoop);
    thread powerUpThread(spawnPowerUps);
    
    // Main loop to accept new client connections
    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
                                 (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }
        {
            lock_guard<mutex> lock(clientsMutex);
            clientSockets.push_back(new_socket);
        }
        thread t(clientHandler, new_socket);
        t.detach();
    }
    
    gameThread.join();
    powerUpThread.join();
    close(server_fd);
}
