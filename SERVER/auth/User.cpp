#include "User.hpp"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>

User::User(const std::string &username, const std::string &password)
{
    this->username = username;
    this->passwordHash = hashPassword(password);
}

const std::string User::getUsername() const
{
    return this->username;
}

const std::string User::getPasswordHash() const
{
    return this->passwordHash;
}

bool User::checkPassword(const std::string &password) const
{
    return this->passwordHash == hashPassword(password);
}

const std::string User::toString() const
{
    return this->username + ":" + this->passwordHash;
}

User User::fromString(const std::string &line)
{
    size_t colon = line.find(':');
    if (colon == std::string::npos)
        return {};

    std::string username = line.substr(0, colon);
    std::string hash = line.substr(colon + 1);

    User u;
    u.username = username;
    u.passwordHash = hash;

    return u;
}

std::string User::hashPassword(const std::string &password)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(password.c_str()), password.size(), hash);

    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return oss.str();
}

bool User::saveToFile(const std::string &filename, const User &user)
{
    int fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
        perror("Failed to open file");
        return false;
    }

    std::string userStr = user.toString() + "\n";
    ssize_t bytesWritten = write(fd, userStr.c_str(), userStr.size());
    if (bytesWritten == -1)
    {
        perror("Failed to write to file");
        close(fd);
        return false;
    }

    close(fd);
    return true;
}

bool User::loadFromFile(const std::string &filename, User &user)
{
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd == -1)
    {
        perror("Failed to open file");
        return false;
    }

    char buffer[1024];
    ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    if (bytesRead == -1)
    {
        perror("Failed to read from file");
        close(fd);
        return false;
    }

    buffer[bytesRead] = '\0';
    user = User::fromString(std::string(buffer));

    close(fd);
    return true;
}