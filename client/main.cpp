#include "../shared/game.h"
#include <iostream>
#include <thread>
#include <string>
#include <chrono>
#include <cstdlib>

using namespace std;

// Forward declaration of runClient (implemented in client.cpp)
void runClient(const string &serverIP);

// Utility function to clear the screen.
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// For demonstration, we assume the local IP is 127.0.0.1.
string getLocalIPAddress() {
    return "127.0.0.1";
}

int main(int argc, char* argv[]) {
    string serverIP = argc > 1 ? argv[1] : getLocalIPAddress();
    runClient(serverIP);
    return 0;
}
