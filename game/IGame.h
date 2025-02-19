#ifndef IGAME_H
#define IGAME_H

#include <string>

struct GameState {
    std::string stateInfo; // Can be extended to structured data.
};

class IGame {
public:
    virtual ~IGame() {}
    virtual void processInput(int playerId, const std::string &input) = 0;
    virtual void update(float deltaTime) = 0;
    virtual std::string getCurrentState() = 0;
    virtual bool isGameOver() const = 0;
};

#endif