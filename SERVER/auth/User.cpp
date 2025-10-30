#include "User.hpp"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>

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