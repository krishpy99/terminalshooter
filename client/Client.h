#pragma once
#include <string>
#include <thread>

class Client {
public:
    Client();
    ~Client();
    bool connectToServer(const std::string &ip, int port);
    void mainMenu();
    void gameLoop();
private:
    int m_serverSocket;
    bool m_inGame;
    std::thread receiverThread;
    void receiverFunc();
    void displayState(const std::string &stateLine);
};