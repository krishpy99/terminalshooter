CXX = g++
CXXFLAGS = -std=c++11 -pthread -Wall -O2
SRCS = main.cpp client.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = client

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -I../shared -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -I../shared -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)