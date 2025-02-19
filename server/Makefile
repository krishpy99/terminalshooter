CXX = g++
CXXFLAGS = -std=c++17 -pthread -Wall

# Include additional source files that provide the missing symbols.
SRCS = main.cpp ServerCore.cpp RoomManager.cpp TerminalShooterRoom.cpp \
       ../game/TerminalShooterGame.cpp \
       ../shared/NetworkUtils.cpp ../shared/Logger.cpp ../shared/Protocol.cpp

OBJS = $(SRCS:.cpp=.o)
TARGET = server

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)