#include "../shared/game.h"
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
#include <algorithm>
#include <map>
#include <semaphore> // C++20 semaphore header

using namespace std;

// ---------------------- Room Class ---------------------------
class Room {
public:
    string code;                      // Room code (a simple string)
    vector<Player> players;           // Up to 2 players
    vector<Bullet> bullets;
    vector<PowerUp> powerUps;
    Board board;
    vector<int> clientSockets;        // Sockets for all clients in this room

    // Separate mutexes for state and broadcasting
    mutex stateMutex;                 
    mutex broadcastMutex;

    bool gameOver;
    bool gameStarted;                 // Flag to ensure the game loop is started only once
    thread gameThread;
    // Semaphore to allow up to 2 players in the room.
    std::counting_semaphore<2> joinSemaphore{2};

    Room(const string &c) : code(c), gameOver(false), gameStarted(false) { }

    // Launch the game loop in its own thread.
    void startGame() {
        if (!gameStarted) {
            gameStarted = true;
            gameThread = thread(&Room::gameLoop, this);
        }
    }

    // Broadcast a message to all clients in this room using a dedicated mutex.
    void broadcastBoard(const string &state) {
        lock_guard<mutex> lock(broadcastMutex);
        for (int sock : clientSockets) {
            send(sock, state.c_str(), state.size(), 0);
        }
    }

    // Build a board string (with boundaries, entities, and player stats)
    string buildBoard() {
        board.clear();
        board.drawBoundaries();
        // Place power-ups
        for (auto &pu : powerUps) {
            char symbol = (pu.type == FOOD) ? 'F' : 'B';
            if (pu.pos.x >= 1 && pu.pos.x < BOARD_ROWS-1 &&
                pu.pos.y >= 1 && pu.pos.y < BOARD_COLS-1)
                board.grid[pu.pos.x][pu.pos.y] = symbol;
        }
        // Place bullets (if cell is empty)
        for (auto &b : bullets) {
            if (b.pos.x >= 1 && b.pos.x < BOARD_ROWS-1 &&
                b.pos.y >= 1 && b.pos.y < BOARD_COLS-1)
                if(board.grid[b.pos.x][b.pos.y] == ' ')
                    board.grid[b.pos.x][b.pos.y] = '|';
        }
        // Place players (they take precedence)
        for (auto &p : players) {
            if (p.pos.x >= 1 && p.pos.x < BOARD_ROWS-1 &&
                p.pos.y >= 1 && p.pos.y < BOARD_COLS-1)
                board.grid[p.pos.x][p.pos.y] = p.rep;
        }
        // Compose output string
        ostringstream oss;
        oss << board.serialize() << "\n";
        // Append player statistics
        for (auto &p : players) {
            oss << "Player " << p.id << " (" << p.rep << ") Health: " 
                << p.health << "   Bullets: " << p.bullets << "\n";
        }
        return oss.str();
    }

    // The room game loop: update bullets, check collisions and power-up pickups,
    // spawn power-ups periodically, update the board, and broadcast the new state.
    void gameLoop() {
        srand(time(0));
        int powerUpTimer = 0;
        while (!gameOver) {
            string state;
            {
                // Lock stateMutex only during game state modifications.
                lock_guard<mutex> lock(stateMutex);
                // Update bullet positions
                for (auto &b : bullets) {
                    b.pos.x += b.dir.x;
                    b.pos.y += b.dir.y;
                }
                // Remove bullets that leave the board
                bullets.erase(remove_if(bullets.begin(), bullets.end(), [](const Bullet &b) {
                    return (b.pos.x <= 0 || b.pos.x >= BOARD_ROWS-1 ||
                            b.pos.y <= 0 || b.pos.y >= BOARD_COLS-1);
                }), bullets.end());
                // Check bullet collisions with players
                for (auto it = bullets.begin(); it != bullets.end(); ) {
                    bool hit = false;
                    Bullet b = *it;
                    for (auto &p : players) {
                        if (b.pos.x == p.pos.x && b.pos.y == p.pos.y) {
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
                // Check if players collect power-ups
                for (auto &p : players) {
                    for (auto it = powerUps.begin(); it != powerUps.end(); ) {
                        if (p.pos.x == it->pos.x && p.pos.y == it->pos.y) {
                            if (it->type == BULLET) {
                                p.bullets += 1;
                            } else if (it->type == FOOD) {
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
                // Check for game over (if any player's health is <= 0)
                for (auto &p : players) {
                    if (p.health <= 0.0f) {
                        gameOver = true;
                        ostringstream oss;
                        oss << "\nGame Over! Player " << ((p.id==1)?2:1) << " wins!\n";
                        state = oss.str();
                        broadcastBoard(state);
                        goto end_loop;
                    }
                }
                // Periodically spawn a power-up (every ~50 iterations)
                powerUpTimer++;
                if (powerUpTimer >= 50) {
                    powerUpTimer = 0;
                    if (powerUps.size() < 5) {
                        PowerUp pu;
                        pu.pos.x = rand() % (BOARD_ROWS - 2) + 1;
                        pu.pos.y = rand() % (BOARD_COLS - 2) + 1;
                        pu.type = (rand() % 2 == 0) ? BULLET : FOOD;
                        powerUps.push_back(pu);
                    }
                }
                // Build the board state string while still under lock.
                state = buildBoard();
            }
            // Broadcast the updated board state outside the state lock.
            broadcastBoard(state);
            usleep(100000);
        }
    end_loop:
        ;
    }
};

// ---------------------- Global Server State ---------------------------
mutex roomsMutex;
map<string, Room*> rooms;  // key: room code, value: Room pointer

// Utility: Generate a simple 4-digit room code.
string generateRoomCode() {
    int num = rand() % 9000 + 1000;
    return to_string(num);
}

// ---------------------- Client Handler ---------------------------
// Each client (player) is assigned to a room. The first message from the client is
// expected to be either "C" (create room) or "J <code>" (join room).
void clientHandler(int clientSocket) {
    char buffer[1024];
    int bytes = recv(clientSocket, buffer, sizeof(buffer)-1, 0);
    if (bytes <= 0) {
        close(clientSocket);
        return;
    }
    buffer[bytes] = '\0';
    istringstream iss(buffer);
    char mode;
    iss >> mode;
    Room *room = nullptr;
    string roomCode;
    {
        lock_guard<mutex> lock(roomsMutex);
        if (mode == 'C') { // Create room
            roomCode = generateRoomCode();
            room = new Room(roomCode);
            rooms[roomCode] = room;
            // Send the room code back to the client.
            string msg = "ROOM " + roomCode + "\n";
            send(clientSocket, msg.c_str(), msg.size(), 0);
        } else if (mode == 'J') { // Join room
            iss >> roomCode;
            if (rooms.find(roomCode) != rooms.end()) {
                room = rooms[roomCode];
                string msg = "JOIN_OK\n";
                send(clientSocket, msg.c_str(), msg.size(), 0);
            } else {
                string msg = "JOIN_FAIL\n";
                send(clientSocket, msg.c_str(), msg.size(), 0);
                close(clientSocket);
                return;
            }
        } else {
            close(clientSocket);
            return;
        }
    }
    
    // Attempt to acquire a join slot using the semaphore.
    if (!room->joinSemaphore.try_acquire()) {
        const char* msg = "Room full.\n";
        send(clientSocket, msg, strlen(msg), 0);
        close(clientSocket);
        return;
    }
    
    // Flag to indicate that the player was successfully added.
    bool playerAdded = false;
    
    {
        // Lock stateMutex when modifying game state.
        lock_guard<mutex> stateLock(room->stateMutex);
        // Double-check that the room isn't full.
        if (room->players.size() >= 2) {
            const char* msg = "Room full.\n";
            send(clientSocket, msg, strlen(msg), 0);
            close(clientSocket);
            room->joinSemaphore.release();
            return;
        }
        cout << "Acquired join slot for a player.\n";
        Player p;
        p.id = (room->players.empty() ? 1 : 2);
        p.rep = (p.id == 1 ? '1' : '2');
        if (p.id == 1) {
            p.pos = Point(1, BOARD_COLS / 2);
            p.bullets = 6;
        } else {
            p.pos = Point(BOARD_ROWS - 2, BOARD_COLS / 2);
            p.bullets = 25;
        }
        p.health = 10.0f;
        room->players.push_back(p);
        // Lock broadcastMutex while updating the clientSockets.
        {
            lock_guard<mutex> broadcastLock(room->broadcastMutex);
            room->clientSockets.push_back(clientSocket);
        }
        // Start the game loop if not already started.
        if (!room->gameStarted) {
            room->startGame();
        }
        playerAdded = true;
    }

    // Continuously read single-character commands from this client.
    while (true) {
        bytes = recv(clientSocket, buffer, 1, 0);
        if (bytes <= 0)
            break;
        char command = buffer[0];
        {
            lock_guard<mutex> stateLock(room->stateMutex);
            cout << "Received command from client socket " << clientSocket << ": " << command << endl;
            // Process movement commands (W, A, S, D) or shoot (' ')
            for (size_t i = 0; i < room->players.size(); i++) {
                if (room->clientSockets[i] == clientSocket) {
                    Player &p = room->players[i];
                    if (command == 'w' || command == 'W') {
                        if (p.pos.x > 1)
                            p.pos.x--;
                    } else if (command == 's' || command == 'S') {
                        if (p.pos.x < BOARD_ROWS - 2)
                            p.pos.x++;
                    } else if (command == 'a' || command == 'A') {
                        if (p.pos.y > 1)
                            p.pos.y--;
                    } else if (command == 'd' || command == 'D') {
                        if (p.pos.y < BOARD_COLS - 2)
                            p.pos.y++;
                    } else if (command == ' ') { // Shooting command
                        if (p.bullets > 0) {
                            Point dir;
                            if (room->players.size() == 2) {
                                Player &opponent = (p.id == 1 ? room->players[1] : room->players[0]);
                                dir = (opponent.pos.x > p.pos.x) ? Point(1, 0) : Point(-1, 0);
                            } else {
                                dir = Point(-1, 0);
                            }
                            p.bullets--;
                            Bullet b;
                            b.playerId = p.id;
                            b.pos = p.pos;
                            b.dir = dir;
                            room->bullets.push_back(b);
                        }
                    }
                    break;
                }
            }
        }
    }
    
    // When the client disconnects, remove its entries from the room.
    {
        lock_guard<mutex> stateLock(room->stateMutex);
        lock_guard<mutex> broadcastLock(room->broadcastMutex);
        for (size_t i = 0; i < room->clientSockets.size(); i++) {
            if (room->clientSockets[i] == clientSocket) {
                room->clientSockets.erase(room->clientSockets.begin() + i);
                room->players.erase(room->players.begin() + i);
                cout << "Player with socket " << clientSocket << " removed from room." << endl;
                break;
            }
        }
    }
    close(clientSocket);
    // Release the join slot.
    if (playerAdded) {
        room->joinSemaphore.release();
    }
}

// ---------------------- Main Server Loop ---------------------------
int main() {
    srand(time(0));
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
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
    
    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }
        // Launch a handler thread for each client connection.
        thread t(clientHandler, new_socket);
        t.detach();
    }
    
    close(server_fd);
    return 0;
}
