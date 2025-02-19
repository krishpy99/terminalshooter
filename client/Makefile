CXX = g++
CXXFLAGS = -Wall -std=c++17

all: client

client: client.o
	$(CXX) $(CXXFLAGS) -o client client.o

client.o: client.cpp
	$(CXX) $(CXXFLAGS) -c client.cpp

clean:
	rm -f client.o client
