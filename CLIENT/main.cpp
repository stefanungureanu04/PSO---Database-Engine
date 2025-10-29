#include "../network/SocketChannel.hpp"
#include <iostream>
#include <string>

int main(int argc, char **argv) {
    std::string ip = "127.0.0.1";
    unsigned short port = 5000;

    if (argc > 1) ip = argv[1];
    if (argc > 2) port = static_cast<unsigned short>(std::stoi(argv[2]));

    SocketChannel client;
    if (!client.connect(ip, port)) {
        std::cerr << "Failed to connect to " << ip << ":" << port << "\n";
        return 1;
    }

    std::cout << "[CLIENT] Connected to " << ip << ":" << port << "\n";
    std::cout << "Type messages (EXIT to quit)\n";

    std::string input;
    while (true) {
        std::getline(std::cin, input);
        if (input == "EXIT") break;

        client.send(input);

        std::string response;
        if (client.receive(response))
            std::cout << "[SERVER] " << response << "\n";
        else
            std::cout << "Server disconnected.\n";
    }

    return 0;
}
