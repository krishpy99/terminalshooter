# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -pthread -std=c++17

# Default target: build the unified binary
all: TerminalShooter

# Link the executable from its object files
TerminalShooter: main.o server.o client.o
	$(CXX) $(CXXFLAGS) -o ShootClub main.o server.o client.o

# Compile main.cpp into main.o
main.o: main.cpp game.h
	$(CXX) $(CXXFLAGS) -c main.cpp

# Compile server.cpp into server.o
server.o: server.cpp game.h
	$(CXX) $(CXXFLAGS) -c server.cpp

# Compile client.cpp into client.o
client.o: client.cpp game.h
	$(CXX) $(CXXFLAGS) -c client.cpp

# Clean up build artifacts
clean:
	rm -f *.o ShootClub

