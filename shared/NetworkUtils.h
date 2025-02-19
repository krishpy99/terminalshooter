#ifndef NETWORKUTILS_H
#define NETWORKUTILS_H

#include <string>

#ifdef _WIN32
#include <winsock2.h>
typedef int socklen_t;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace NetworkUtils {

    // Sends a line (appends a newline) over the socket.
    bool sendLine(int sock, const std::string &line);

    // Receives a line (delimited by newline) from the socket.
    bool recvLine(int sock, std::string &line);
}

#endif 