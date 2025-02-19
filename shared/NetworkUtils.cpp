#include "NetworkUtils.h"
#include <cstring>

#ifdef _WIN32
// Windows-specific initialization may be added here.
#else
#include <errno.h>
#endif

namespace NetworkUtils {

bool sendLine(int sock, const std::string &line) {
    std::string data = line + "\n";
    size_t totalSent = 0;
    while (totalSent < data.size()) {
        int sent = send(sock, data.c_str() + totalSent, data.size() - totalSent, 0);
        if (sent <= 0) {
            return false;
        }
        totalSent += sent;
    }
    return true;
}

bool recvLine(int sock, std::string &line) {
    line.clear();
    char ch;
    while (true) {
        int bytesReceived = recv(sock, &ch, 1, 0);
        if (bytesReceived <= 0)
            return false;
        if (ch == '\n')
            break;
        line.push_back(ch);
    }
    return true;
}

} 