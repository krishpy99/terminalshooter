CXX = g++
CXXFLAGS = -std=c++20 -pthread -Wall -O2
SRCS = server.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = server

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -I../shared -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)