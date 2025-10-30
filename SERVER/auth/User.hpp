#pragma once
#include <string>

class User {
private:
    std::string username;
    std::string passwordHash;

public:
    User() = default;
    User(const std::string &username, const std::string &password);

    const std::string getUsername() const;
    const std::string getPasswordHash() const;

    bool checkPassword(const std::string &password) const;

    const std::string toString() const;
    static User fromString(const std::string &line);

private:
    static std::string hashPassword(const std::string &password);
};