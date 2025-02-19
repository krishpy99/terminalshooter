#include "../shared/game.h"
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

// getch implementation for non-blocking single-character input
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

// Thread that captures keystrokes and sends them immediately to the server.
void sendInput() {
    while (true) {
        char ch = getch();
        send(serverSocket, &ch, 1, 0);
        usleep(50000); // slight delay to prevent flooding
    }
}

// Thread that receives board state updates from the server and displays them.
void receiveBoard() {
    char buffer[4096];
    while (true) {
        int bytes = recv(serverSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            cout << "Disconnected from server." << endl;
            exit(0);
        }
        buffer[bytes] = '\0';
        // Clear the terminal using ANSI escape codes instead of system("clear")
        cout << "\033[H\033[J";
        cout << buffer << flush;
    }
}

// Run the client: connect to the server and perform the initial room command.
void runClient(const string &serverIP) {
    struct sockaddr_in serv_addr;
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cout << "Socket creation error" << endl;
        return;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(12345);
    
    if (inet_pton(AF_INET, serverIP.c_str(), &serv_addr.sin_addr) <= 0) {
        cout << "Invalid address/ Address not supported" << endl;
        return;
    }
    
    if (connect(serverSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        cout << "Connection Failed" << endl;
        return;
    }
    
    // Ask user whether to create or join a room.
    cout << "Enter 'C' to create a room or 'J' to join: ";
    char choice;
    cin >> choice;
    string roomCmd;
    if (choice == 'C' || choice == 'c') {
        roomCmd = "C\n";
    } else if (choice == 'J' || choice == 'j') {
        cout << "Enter room code: ";
        string code;
        cin >> code;
        roomCmd = "J " + code + "\n";
    } else {
        cout << "Invalid option." << endl;
        return;
    }
    // Send the room command.
    send(serverSocket, roomCmd.c_str(), roomCmd.size(), 0);
    
    // Wait for server reply.
    char reply[1024];
    int len = recv(serverSocket, reply, sizeof(reply)-1, 0);
    if (len > 0) {
        reply[len] = '\0';
        cout << reply;
        if (string(reply).find("FAIL") != string::npos) {
            close(serverSocket);
            return;
        }
    }
    
    // Start threads to send input and receive board state.
    thread inputThread(sendInput);
    thread receiveThread(receiveBoard);
    inputThread.join();
    receiveThread.join();
    
    close(serverSocket);
}