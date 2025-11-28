#include "Server.h"
#include "Protocol.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
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
	if (serverSocket < 0)
	{
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}
	
	int opt = 1;
	if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

	if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
	{
		perror("Bind failed");
		exit(EXIT_FAILURE);
	}

	if (listen(serverSocket, 3) < 0)
	{
		perror("Listen failed");
		exit(EXIT_FAILURE);
	}

	running = true;
	std::cout << "Server listening on port " << port << std::endl;

	while (running)
	{
		struct sockaddr_in clientAddr;
		socklen_t addrLen = sizeof(clientAddr);

		sem_wait(&connectionSem);

		int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrLen);
		if (clientSocket < 0)
		{
			perror("Accept failed");
			sem_post(&connectionSem);
			continue;
		}

		std::cout << "New connection: " << inet_ntoa(clientAddr.sin_addr) << std::endl;

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

	// citire username pentru a decide cine se conecteaza la baza de date
	// pentru a rula cu privilegiile altui user, punem in comanda sudo -u username -E ./client

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

	std::cout << "User connected: " << username << std::endl;

	std::string welcome = "Welcome\n";
	write(clientSocket, welcome.c_str(), welcome.length());

	std::string currentDB = "";

	while (connected)
	{
		memset(buffer, 0, BUFFER_SIZE);
		bytesRead = read(clientSocket, buffer, BUFFER_SIZE);

		if (bytesRead <= 0)
		{
			connected = false;
		}
		else
		{
			std::string command(buffer);
			if (!command.empty() && command.back() == '\n')
				command.pop_back();
			if (!command.empty() && command.back() == '\r')
				command.pop_back();

			std::cout << "Received: " << command << std::endl;

			if (db->toUpper(command) == "EXIT")
			{
				connected = false;
			}
			else
			{
				std::string response = db->executeCommand(command, currentDB, username);
				write(clientSocket, response.c_str(), response.length());
			}
		}
	}
	delete db;
	close(clientSocket);
	std::cout << "Client disconnected" << std::endl;
	sem_post(&connectionSem);
}
