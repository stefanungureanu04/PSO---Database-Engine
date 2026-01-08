#include "Client.h"
#define PORT 8080
#define LOCAL_HOST "127.0.0.1"

int main(int argc, char *argv[])
{
    std::string ip = LOCAL_HOST;
    int port = PORT;

    if (argc > 1){
        ip = argv[1];
    }
    else if (argc > 2){
        port = std::stoi(argv[2]);
    }

    Client client(ip, port);

    if (client.connectToServer()){
        client.run();
    }

    return 0;
}
