#pragma once
#include <string>
#include <vector>

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

    static bool saveToFile(const std::string &filename, const User &user);
    static bool loadFromFile(const std::string &filename, User &user);

private:
    static std::string hashPassword(const std::string &password);
};