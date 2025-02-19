CXX = g++
CXXFLAGS = -Wall -std=c++17 -Igame -Iserver -Iterminal-shooter

SOURCES = \
	main.cpp \
    ../server/BaseServer.cpp \
    TerminalShooterGame.cpp \
    TerminalShooterServer.cpp

OBJECTS = $(SOURCES:.cpp=.o)

all: TerminalShooterServer

TerminalShooterServer: $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o TerminalShooterServer $(OBJECTS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) TerminalShooterServer

