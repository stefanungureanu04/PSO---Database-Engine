#include "SocketChannel.hpp"

SocketChannel::SocketChannel() : socket(std::make_unique<sf::TcpSocket>()) 
{
    this->isListener = false;
}

SocketChannel::~SocketChannel() 
{
    if(this->isListener) {
        this->listener.close();
    }
    else {
        this->socket->disconnect();
    }
}

std::string SocketChannel::getRemoteIP() const 
{
    return this->socket->getRemoteAddress().toString();
}

unsigned short SocketChannel::getRemotePort() const 
{
    return this->socket->getRemotePort();
}

bool SocketChannel::connect(const std::string &ip, unsigned short port) 
{
    if (this->socket->connect(ip, port) != sf::Socket::Done) {
        return false;
    }

    return true;
}

bool SocketChannel::bind(unsigned short port) 
{
    if (this->listener.listen(port) != sf::Socket::Done) {
        return false;
    }

    this->isListener = true;
    
    return true;
}

std::unique_ptr<SocketChannel> SocketChannel::accept() 
{    
    if (!this->isListener) {
        return nullptr;
    }

    auto client = std::make_unique<SocketChannel>();
 
    if (this->listener.accept(*client->socket) != sf::Socket::Done) {
        return nullptr;
    }
    
    client->isListener = false;

    return client;
}

bool SocketChannel::send(const std::string &message) 
{
    sf::Packet packet;
    packet << message;

    return this->socket->send(packet) == sf::Socket::Done;
}

bool SocketChannel::receive(std::string &message) 
{
    sf::Packet packet;

    auto status = this->socket->receive(packet);

    if (status == sf::Socket::Disconnected) {
        return false;
    }
    else if (status != sf::Socket::Done) {
        return false;
    }

    packet >> message;
    return true;
}

void SocketChannel::addToSelector(sf::SocketSelector &selector) 
{
    if (this->isListener) {
        selector.add(this->listener);
    }
    else {
        selector.add(*(this->socket));
    }
}

bool SocketChannel::waitSelector(sf::SocketSelector &selector, float timeout) 
{
    return selector.wait(sf::seconds(timeout));
}

bool SocketChannel::isReady(sf::SocketSelector &selector)
{
    if (this->isListener) {
        return selector.isReady(this->listener);
    }
    else {
        return selector.isReady(*(this->socket));
    }
}

void SocketChannel::removeFromSelector(sf::SocketSelector &selector) 
{
    if (this->isListener) {
        selector.remove(this->listener);
    }
    else {
        selector.remove(*(this->socket));
    }
}
