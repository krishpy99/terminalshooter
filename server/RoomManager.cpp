#include "RoomManager.h"
#include "TerminalShooterRoom.h"
#include "../shared/Logger.h"
#include <cstdlib>
#include <ctime>
#include <sstream>

RoomManager::RoomManager() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
}

RoomManager::~RoomManager() {}

std::string RoomManager::generateRoomCode() {
    std::ostringstream oss;
    for (int i = 0; i < 4; ++i) {
        char c = 'A' + std::rand() % 26;
        oss << c;
    }
    return oss.str();
}

std::string RoomManager::createRoom() {
    std::lock_guard<std::mutex> lock(roomsMutex);
    std::string code = generateRoomCode();
    // Create a new TerminalShooterRoom and store it.
    rooms[code] = std::make_shared<TerminalShooterRoom>(code);
    Logger::logInfo("Room created with code: " + code);
    return code;
}

std::shared_ptr<Room> RoomManager::joinRoom(const std::string &code) {
    std::lock_guard<std::mutex> lock(roomsMutex);
    auto it = rooms.find(code);
    if (it != rooms.end()) {
        return it->second;
    }
    return nullptr;
}

void RoomManager::removeRoom(const std::string &code) {
    std::lock_guard<std::mutex> lock(roomsMutex);
    rooms.erase(code);
} 