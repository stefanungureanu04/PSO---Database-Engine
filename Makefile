CXX = g++
CXXFLAGS = -Wall -Wextra -pthread -std=c++17 -Isrc/common -Isrc/server -Isrc/client

SERVER_SRCS = src/server/main.cpp src/server/Server.cpp src/server/Database.cpp src/server/Table.cpp src/server/Saver.cpp
CLIENT_SRCS = src/client/main.cpp src/client/Client.cpp

SERVER_OBJS = $(SERVER_SRCS:.cpp=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.cpp=.o)

all: server client

server: $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o server $(SERVER_OBJS)

client: $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o client $(CLIENT_OBJS)

clean:
	rm -f server client src/server/*.o src/client/*.o src/common/*.o

.PHONY: all clean
