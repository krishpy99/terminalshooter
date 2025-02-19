#ifndef ROOM_H
#define ROOM_H

#include <string>

class Room {
public:
    virtual ~Room() {}
    // Runs the game loop for the room.
    virtual void runGameLoop() = 0;
    // Adds a client socket to the room; returns false if full.
    virtual bool addClient(int sock) = 0;
    // Handles a message sent by a client.
    virtual void handleClientMessage(int sock, const std::string &msg) = 0;
};

#endif 