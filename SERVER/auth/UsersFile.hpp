#pragma once
#include <string>
#include <vector>
#include <mutex>
#include "User.hpp"

class UsersFile {
private:
    static UsersFile* instance;

    static std::string filename;
    static std::mutex Mutex;
    std::vector<User> users;

    UsersFile() = default;

    void loadFromFile();
    void saveToFile() const;

public:
    static UsersFile& getInstance();
    static void destroyInstance();

    bool addUser(const std::string &username, const std::string &password);
    bool authenticate(const std::string &username, const std::string &password);
    bool userExists(const std::string &username) const;

    std::vector<User> getUsers() const;
};