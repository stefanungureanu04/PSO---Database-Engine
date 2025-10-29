#pragma once
#include <SFML/Network.hpp>
#include <string>
#include <memory>
#include <iostream>

class SocketChannel {
private:
    sf::TcpListener listener;
    std::unique_ptr<sf::TcpSocket> socket;
    bool isListener = false;

public:
    SocketChannel();
    ~SocketChannel();

    std::string getRemoteIP() const;
    unsigned short getRemotePort() const;

    bool connect(const std::string &ip, unsigned short port);
    bool bind(unsigned short port);
    std::unique_ptr<SocketChannel> accept();

    bool send(const std::string &message);
    bool receive(std::string &message);

    void addToSelector(sf::SocketSelector &selector);
    static bool waitSelector(sf::SocketSelector &selector, float timeout = 0.5f);
    bool isReady(sf::SocketSelector &selector);
    void removeFromSelector(sf::SocketSelector &selector);

};
