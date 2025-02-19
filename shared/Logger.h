#ifndef LOGGER_H
#define LOGGER_H

#include <string>

namespace Logger {
    void logInfo(const std::string &msg);
    void logError(const std::string &msg);
}

#endif 