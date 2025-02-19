#ifndef ROOMMANAGER_H
#define ROOMMANAGER_H

#include <string>
#include <map>
#include <memory>
#include <mutex>

class Room;

class RoomManager {
public:
    RoomManager();
    ~RoomManager();

    // Creates a new room and returns its unique room code.
    std::string createRoom();

    // Returns a pointer to an existing room by its code.
    std::shared_ptr<Room> joinRoom(const std::string &code);

    // Removes a room from the manager.
    void removeRoom(const std::string &code);

private:
    std::mutex roomsMutex;
    std::map<std::string, std::shared_ptr<Room>> rooms;
    std::string generateRoomCode();
};

#endif 