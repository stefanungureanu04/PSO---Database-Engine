all: server client

CXX = g++
CXXFLAGS = -Wall -Wextra -pthread -std=c++17 -Isrc/common -Isrc/server -Isrc/client

SERVER_SRCS = src/server/main.cpp src/server/Server.cpp src/server/Database.cpp src/server/Table.cpp src/server/Saver.cpp src/server/ConditionParser.cpp
CLIENT_SRCS = src/client/main.cpp src/client/Client.cpp src/client/HelpSystem.cpp

SERVER_OBJS = $(SERVER_SRCS:.cpp=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.cpp=.o)

server: $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o server $(SERVER_OBJS)
	rm -f src/server/*.o

client: $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o client $(CLIENT_OBJS)
	rm -f src/client/*.o

.PHONY: all clean

clean:
	rm -f server client
	rm -f src/common/*.o
