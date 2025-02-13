#include "game.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <termios.h>
#include <cstring>
#include <cstdlib>

using namespace std;

int serverSocket;

// A getch implementation for non-blocking character input
char getch() {
    char buf = 0;
    struct termios old;
    memset(&old, 0, sizeof(old));
    if (tcgetattr(0, &old) < 0)
        perror("tcgetattr()");
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &old) < 0)
        perror("tcsetattr ICANON");
    if (read(0, &buf, 1) < 0)
        perror("read()");
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if (tcsetattr(0, TCSADRAIN, &old) < 0)
        perror("tcsetattr ~ICANON");
    return buf;
}

// Thread to capture user input (WASD and space) and send it to the server
void sendInput() {
    while (true) {
        char ch = getch();
        send(serverSocket, &ch, 1, 0);
        usleep(50000); // small delay to avoid flooding the server
    }
}

// Thread to receive the board state from the server and display it
void receiveBoard() {
    char buffer[4096];
    while (true) {
        int bytes = recv(serverSocket, buffer, sizeof(buffer)-1, 0);
        if (bytes <= 0) {
            cout << "Disconnected from server." << endl;
            exit(0);
        }
        buffer[bytes] = '\0';
        system("clear"); // or "cls" on Windows
        cout << buffer << endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: ./client <server_ip>" << endl;
        return 1;
    }
    
    struct sockaddr_in serv_addr;
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cout << "Socket creation error" << endl;
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(12345);
    
    // Convert the server address from text to binary form
    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0) {
        cout << "Invalid address/ Address not supported" << endl;
        return -1;
    }
    
    if (connect(serverSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        cout << "Connection Failed" << endl;
        return -1;
    }
    
    // Start threads for capturing input and receiving board updates
    thread inputThread(sendInput);
    thread receiveThread(receiveBoard);
    
    inputThread.join();
    receiveThread.join();
    
    close(serverSocket);
    return 0;
}
