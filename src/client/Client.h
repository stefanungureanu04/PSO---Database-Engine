#ifndef CLIENT_H
#define CLIENT_H

#include <netinet/in.h>
#include <string>

class Client
{
private:
	int sock;
	struct sockaddr_in serverAddr;
	bool connected;

public:
	Client(std::string ip, int port);
	~Client();
	bool connectToServer();
	void run();
};

#endif