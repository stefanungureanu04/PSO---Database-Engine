#include "Server.h"
#include "../common/Protocol.h"
#include <arpa/inet.h>
#include <cstring>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_CLIENTS 10

Server::Server(int port)
{
    this->port = port;
    this->running = false;
    sem_init(&connectionSem, 0, MAX_CLIENTS);
}

Server::~Server()
{
    stop();
    sem_destroy(&connectionSem);
}

void Server::start()
{
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;

    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0){
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, 3) < 0){
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    running = true;
    running = true;

    std::string msg = std::string(MSG_SERVER_LISTENING) + std::to_string(port) + "\n";
    write(STDOUT_FILENO, msg.c_str(), msg.length());

    while (running)
    {
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);

        sem_wait(&connectionSem);

        int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrLen);
        
        if (clientSocket < 0){
            perror("Accept failed");
            sem_post(&connectionSem);
            continue;
        }

        msg = std::string(MSG_NEW_CONNECTION) + inet_ntoa(clientAddr.sin_addr) + "\n";
        write(STDOUT_FILENO, msg.c_str(), msg.length());

        std::thread clientThread(&Server::handleClient, this, clientSocket);
        clientThread.detach();
    }
}

void Server::stop()
{
    running = false;
    close(serverSocket);
}

void Server::handleClient(int clientSocket)
{
    char buffer[BUFFER_SIZE];
    bool connected = true;

    memset(buffer, 0, BUFFER_SIZE);
    int bytesRead = read(clientSocket, buffer, BUFFER_SIZE);

    if (bytesRead <= 0)
    {
        close(clientSocket);
        sem_post(&connectionSem);
        return;
    }

    std::string username(buffer);

    Database *db = new Database(username);

    std::string msg = std::string(MSG_USER_CONNECTED) + username + "\n";
    write(STDOUT_FILENO, msg.c_str(), msg.length());

    std::string welcome = MSG_WELCOME;
    write(clientSocket, welcome.c_str(), welcome.length());

    std::string currentDB = "";

    while (connected)
    {
        memset(buffer, 0, BUFFER_SIZE);
        bytesRead = read(clientSocket, buffer, BUFFER_SIZE);

        if (bytesRead <= 0){
            connected = false;
        }
        else{
            
            std::string command(buffer);
            
            if (!command.empty() && command.back() == '\n'){
                command.pop_back();
            }
            if (!command.empty() && command.back() == '\r'){
                command.pop_back();

            }

            msg = "Received: " + command + "\n";
            write(STDOUT_FILENO, msg.c_str(), msg.length());

            if (db->toUpper(command) == CMD_EXIT){
                connected = false;
            }
            else{
                std::string response = db->executeCommand(command, currentDB, username);
                write(clientSocket, response.c_str(), response.length());
            }
        }
    }

    delete db;
    close(clientSocket);
    
    msg = std::string(MSG_CLIENT_DISCONNECTED) + "\n";
    write(STDOUT_FILENO, msg.c_str(), msg.length());
    sem_post(&connectionSem);
}
