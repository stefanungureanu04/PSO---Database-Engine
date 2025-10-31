#include "../network/SocketChannel.hpp"
#include <SFML/Network.hpp>
#include <iostream>
#include <vector>
#include <memory>
#include "logger/Logger.hpp"

int numclients=0;

int main() {
    SocketChannel server;
    if (!server.bind(5000)) {
        std::cerr << "Failed to bind server on port 5000\n";
        return 1;
    }

    sf::SocketSelector selector;
    server.addToSelector(selector);

    std::vector<std::unique_ptr<SocketChannel>> clients;

    std::cout << "[SERVER] Listening on port 5000...\n";

    while (true) {
        if (!SocketChannel::waitSelector(selector))
            continue;

        if (server.isReady(selector)) {
            auto client = server.accept();
            if (client) {
                std::cout << "[+] New client: " 
                          << client->getRemoteIP() << ":" 
                          << client->getRemotePort() << "\n";
                 
                          numclients++;
                
                std::cout << "Num clients: " << numclients << "\n";

                client->addToSelector(selector);
                clients.push_back(std::move(client));
            }
        }

        for (auto it = clients.begin(); it != clients.end(); ) {
            auto &client = *it;
            if (client->isReady(selector)) {
                std::string msg;
                if (!client->receive(msg)) {
                    std::cout << "[-] Client disconnected: " 
                              << client->getRemoteIP() << "\n";
                    client->removeFromSelector(selector);
                    it = clients.erase(it);

                    numclients--;
                    std::cout << "Num clients: " << numclients << "\n";

                    continue;
                }

                std::cout << "[Client " << client->getRemoteIP() << "] " << msg << "\n";
                client->send("Server echo: " + msg);
            }
            ++it;
        }
    }

    return 0;
}
