#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <vector>

namespace Protocol {

    // Client -> Server commands
    const std::string CMD_CREATE_ROOM = "CREATE_ROOM";
    const std::string CMD_JOIN_ROOM   = "JOIN_ROOM";
    const std::string CMD_MOVE        = "MOVE";
    const std::string CMD_SHOOT       = "SHOOT";

    // Server -> Client commands
    const std::string CMD_ROOM_CODE   = "ROOM_CODE";
    const std::string CMD_ROOM_JOINED = "ROOM_JOINED";
    const std::string CMD_ROOM_FULL   = "ROOM_FULL";
    const std::string CMD_NO_SUCH_ROOM= "NO_SUCH_ROOM";
    const std::string CMD_STATE       = "STATE";
    const std::string CMD_GAME_END    = "GAME_END";
    const std::string CMD_GAME_TIE    = "GAME_TIE";
    const std::string CMD_DISCONNECTED= "DISCONNECTED";
    const std::string CMD_ERROR       = "ERROR";

    // Utility: Split a string into tokens given a delimiter.
    std::vector<std::string> split(const std::string &s, char delimiter);
}

#endif 