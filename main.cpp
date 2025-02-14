#include "game.h"
#include <iostream>
#include <thread>
#include <string>
#include <chrono>

using namespace std;

// Forward declarations of the functions that encapsulate the server and client logic.
void runServer();             // Defined in server.cpp
void runClient(const string &serverIP);  // Defined in client.cpp

// Utility: A simple function to clear the screen (platform-specific)
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// Utility: A function to get the host IP address (for display purposes)
// For actual network discovery, see the ifconfig implementation discussed earlier.
string getLocalIPAddress() {
    // For simplicity, we return the result from a proper implementation.
    // (You can replace this with the ifconfig popen() based implementation.)
    return "127.0.0.1";
}

int main() {
    bool exitProgram = false;
    while (!exitProgram) {
        clearScreen();
        cout << "=== ShootClub ===\n";
        cout << "1. Host Game\n";
        cout << "2. Join Game\n";
        cout << "3. Exit\n";
        cout << "Enter your choice: ";
        int choice;
        cin >> choice;

        if (choice == 1) {
            // Host Game mode
            bool playAgain = false;
            do {
                clearScreen();
                // Host mode: Start the server in a separate thread and then run the client.
                thread serverThread(runServer);

                // Give the server a moment to start up.
                this_thread::sleep_for(chrono::seconds(1));

                // Display the host's IP address for sharing.
                string localIP = getLocalIPAddress();
                cout << "Hosting game on IP: " << localIP << "\n";
                cout << "Waiting for guest to join...\n";

                // Run the client connecting to the local server.
                runClient("127.0.0.1");

                // Wait for the server thread to finish.
                serverThread.join();

                // When the game is over, display the replay menu.
                cout << "\nGame Over!\n";
                cout << "1. Play again in the same room\n";
                cout << "2. Back to main menu\n";
                cout << "Enter your choice: ";
                int replayChoice;
                cin >> replayChoice;
                playAgain = (replayChoice == 1);
            } while (playAgain);
        } else if (choice == 2) {
            // Join Game mode
            cout << "Enter the server IP address: ";
            string serverIP;
            cin >> serverIP;

            bool playAgain = false;
            do {
                clearScreen();
                runClient(serverIP);

                cout << "\nGame Over!\n";
                cout << "1. Play again in the same room\n";
                cout << "2. Back to main menu\n";
                cout << "Enter your choice: ";
                int replayChoice;
                cin >> replayChoice;
                playAgain = (replayChoice == 1);
            } while (playAgain);
        } else {
            exitProgram = true;
        }
    }
    cout << "Exiting...\n";
    return 0;
}
