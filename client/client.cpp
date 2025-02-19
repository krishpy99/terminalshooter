#include <iostream>
#include <thread>
#include <mutex>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <termios.h>
#include <csignal>
#include <memory>

// RAII class to enable raw terminal mode persistently.
class TerminalRaw {
public:
    TerminalRaw() {
        tcgetattr(STDIN_FILENO, &old_);
        new_ = old_;
        new_.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &new_);
    }
    ~TerminalRaw() {
        tcsetattr(STDIN_FILENO, TCSANOW, &old_);
    }
private:
    struct termios old_;
    struct termios new_;
};

static int g_sock = -1;
static bool g_running = true;
static bool g_inGame = false; // True when we should be reading single-char inputs

void handleSigInt(int) {
    g_running = false;
    if (g_sock != -1) {
        close(g_sock);
    }
}

// Instead of clearing the entire screen, reposition the cursor at the top.
void clearScreen() {
    std::cout << "\033[H";
}

// Thread to receive data from the server.
void receiveThreadFunc() {
    const int BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    while (g_running) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(g_sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) {
            std::cerr << "Server disconnected.\n";
            g_running = false;
            break;
        }
        buffer[bytes] = '\0';
        std::string msg(buffer);

        // Check for game mode signals.
        if (msg.find("GAME_START") != std::string::npos) {
            g_inGame = true;
            continue;
        }
        if (msg.find("GAME_END") != std::string::npos) {
            g_inGame = false;
            std::cout << std::flush;
            continue;
        }

        // For in-game board updates, reposition cursor at top.
        if (g_inGame) {
            clearScreen();
            std::cout << msg << std::flush;
        } else {
            // In menu mode, just print the message.
            std::cout << msg << std::flush;
        }
    }
}

// Thread to handle user input.
void inputThreadFunc() {
    // Use a unique_ptr to manage our raw mode object.
    std::unique_ptr<TerminalRaw> rawMode = nullptr;
    while (g_running) {
        if (g_inGame) {
            // If not already in raw mode, enable it.
            if (!rawMode) {
                rawMode.reset(new TerminalRaw());
            }
            char ch;
            if (read(STDIN_FILENO, &ch, 1) <= 0) {
                g_running = false;
                break;
            }
            // Ignore newline characters.
            if (ch == '\n' || ch == '\r') continue;
            send(g_sock, &ch, 1, 0);
            usleep(30000); // slight delay to avoid spamming
        } else {
            // If raw mode is active, disable it.
            if (rawMode) {
                rawMode.reset();
            }
            std::string line;
            if (!std::getline(std::cin, line)) {
                g_running = false;
                break;
            }
            line.push_back('\n');
            send(g_sock, line.c_str(), line.size(), 0);
        }
    }
}

int main(int argc, char* argv[]) {
    signal(SIGINT, handleSigInt);

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> [port]\n";
        return 1;
    }
    std::string serverIP = argv[1];
    int port = 12345;
    if (argc > 2) {
        port = std::stoi(argv[2]);
    }

    g_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (g_sock < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port   = htons(port);

    if (inet_pton(AF_INET, serverIP.c_str(), &servAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address.\n";
        close(g_sock);
        return 1;
    }

    if (connect(g_sock, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
        perror("connect");
        close(g_sock);
        return 1;
    }

    std::cout << "Connected to " << serverIP << ":" << port << "\n";

    std::thread tRecv(receiveThreadFunc);
    std::thread tInput(inputThreadFunc);

    tRecv.join();
    tInput.join();

    if (g_sock != -1) {
        close(g_sock);
    }
    std::cout << "Client exiting.\n";
    return 0;
}
