CXX = g++
CXXFLAGS = -std=c++17 -pthread -Wall -I../shared

SRCS = main.cpp ClientCore.cpp ../shared/NetworkUtils.cpp ../shared/Logger.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = client

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) 