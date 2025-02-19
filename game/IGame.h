#ifndef IGAME_H
#define IGAME_H

#include <string>
#include <vector>

// Represents a player connecting to a game.
// You could store a player ID, socket, display name, etc.
struct PlayerConnection {
    int socket;       // or any identifier for the player's connection
    std::string name; // optional: player's nickname, if needed
};

// IGame: minimal universal interface for any game
class IGame {
public:
    virtual ~IGame() = default;

    // Assign / retrieve a room code or unique identifier
    virtual void setRoomCode(const std::string &code) = 0;
    virtual std::string getRoomCode() const = 0;

    // Add / remove players
    virtual void addPlayer(const PlayerConnection &playerConn) = 0;
    virtual void removePlayer(int socket) = 0;

    // Check if the game is over
    virtual bool isGameOver() const = 0;
};

#endif // IGAME_H
