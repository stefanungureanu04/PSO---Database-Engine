#ifndef SERVER_H
#define SERVER_H

#include "Database.h"
#include <netinet/in.h>
#include <semaphore.h>
#include <thread>
#include <vector>

class Server {
private:
  int serverSocket;
  int port;
  struct sockaddr_in serverAddr;
  bool running;
  int user;

  // Concurrency limits
  sem_t connectionSem;

  void handleClient(int clientSocket);

public:
  Server(int port);
  ~Server();
  void start();
  void stop();
};

#endif // SERVER_H
