#include "Client.h"
#include "../common/Protocol.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

Client::Client(std::string ip, int port) : connected(false) {
  sock = socket(AF_INET, SOCK_STREAM, 0);

  if (sock < 0) {
    perror("Socket creation error");
    exit(EXIT_FAILURE);
  }

  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);

  if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0) {
    perror("Invalid address/ Address not supported");
    exit(EXIT_FAILURE);
  }
}

Client::~Client() {
  if (connected) {
    close(sock);
  }
}

bool Client::connectToServer() {
  if (connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
    std::cerr << "Connection failed" << std::endl;
    return false;
  }

  connected = true;
  std::cout << "Connected to server. Type 'EXIT' to quit." << std::endl;

  char *user = getenv("USER");

  if (user == nullptr) {
    user = (char *)"user";
  }

  std::string username(user);
  send(sock, username.c_str(), username.length(), 0);

  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);
  read(sock, buffer, BUFFER_SIZE);

  return true;
}

void Client::run() {
  char buffer[BUFFER_SIZE];
  std::string input;

  char *user = getenv("USER");

  if (user == nullptr) {
    user = (char *)"user";
  }

  std::string prompt = "db@" + std::string(user) + " > ";

  while (connected) {

    std::cout << prompt;

    if (!std::getline(std::cin, input))
      break;

    if (input.empty())
      continue;

    if (input == "clear") {

      pid_t pid = fork();

      if (pid == 0) {
        execlp("clear", "clear", NULL);
        exit(EXIT_FAILURE);
      } else {
        wait(NULL);
      }

      continue;
    }

    if (send(sock, input.c_str(), input.length(), 0) < 0) {
      perror("Send failed");
      break;
    }

    if (input == CMD_EXIT_UPPERCASE || input == CMD_EXIT_LOWERCASE) {
      break;
    }

    memset(buffer, 0, BUFFER_SIZE);
    int bytesRead = read(sock, buffer, BUFFER_SIZE);

    if (bytesRead > 0) {
      std::cout << buffer << std::endl;
    } else {
      std::cout << "Server disconnected" << std::endl;
      break;
    }
  }
}
