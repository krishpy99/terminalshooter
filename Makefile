# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -pthread -std=c++17

# Default target when you run 'make'
all: server client

# Link the server executable from its object file(s)
server: server.o
	$(CXX) $(CXXFLAGS) -o server server.o

# Link the client executable from its object file(s)
client: client.o
	$(CXX) $(CXXFLAGS) -o client client.o

# Compile server.cpp into server.o
server.o: server.cpp game.h
	$(CXX) $(CXXFLAGS) -c server.cpp

# Compile client.cpp into client.o
client.o: client.cpp game.h
	$(CXX) $(CXXFLAGS) -c client.cpp

# Clean up build artifacts
clean:
	rm -f *.o server client
