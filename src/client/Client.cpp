#include "Client.h"
#include "HelpSystem.h"
#include "../common/Protocol.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

#define MSG_HELP_TOPIC_NOT_FOUND "ERROR: Help topic not found."

Client::Client(std::string ip, int port) : connected(false)
{
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0){
        perror(MSG_SOCK_ERR);
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0){
        perror(MSG_ADDR_ERR);
        exit(EXIT_FAILURE);
    }
}

Client::~Client()
{
    if (connected){
        close(sock);
    }
}

bool Client::connectToServer()
{
    if (connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0){
        perror(MSG_CONNECT_FAIL);
        return false;
    }

    connected = true;

    write(STDOUT_FILENO,MSG_CONNECT_SUCCESS,strlen(MSG_CONNECT_SUCCESS));

    char *user = getenv("USER");

    if (user == nullptr){
        user = (char *)"user";
    }

    std::string username(user);
    send(sock, username.c_str(), username.length(), 0);

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    read(sock, buffer, BUFFER_SIZE);

    return true;
}

void Client::run()
{
    char buffer[BUFFER_SIZE];
    std::string input;

    char *user = getenv("USER");

    if (user == nullptr){
        user = (char *)"user";
    }

    std::string prompt = "db@" + std::string(user) + " > ";

    while (connected)
    {
        write(STDOUT_FILENO,prompt.c_str(),strlen(prompt.c_str()));

        if (!std::getline(std::cin, input)){
            break;
        }

        if (input.empty()){
            continue;
        }

        if (input == "clear")
        {

            pid_t pid = fork();

            if (pid == 0)
            {
                execlp("clear", "clear", NULL);
                exit(EXIT_FAILURE);
            }
            else
            {
                wait(NULL);
            }

            continue;
        }
        else if (input.rfind("help", 0) == 0)
        {
            std::string help = HelpSystem::getHelp(input.substr(4));
            if (help == MSG_HELP_NOT_FOUND)
            {
                std::string helpMsg = help + "\n";
                write(STDOUT_FILENO, helpMsg.c_str(), helpMsg.length());
            } 
            else 
            {
                 std::string helpMsg = help + "\n";
                 write(STDOUT_FILENO, helpMsg.c_str(), helpMsg.length());
            }
            continue;
        }

        if (send(sock, input.c_str(), input.length(), 0) < 0)
        {
            perror(MSG_SEND_FAIL);
            break;
        }

        if (strcasecmp(input.c_str(), CMD_EXIT) == 0)
        {
            break;
        }

        memset(buffer, 0, BUFFER_SIZE);
        int bytesRead = read(sock, buffer, BUFFER_SIZE);
        buffer[bytesRead] = '\0';

        if (bytesRead > 0)
        {
            write(STDOUT_FILENO,buffer,strlen(buffer));
            write(STDOUT_FILENO,"\n",1);
        }
        else
        {
            write(STDOUT_FILENO,MSG_SERVER_DISC,strlen(MSG_SERVER_DISC));
            break;
        }
    }
}
