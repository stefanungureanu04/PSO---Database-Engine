#include "UsersFile.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>

UsersFile *UsersFile::instance = nullptr;
std::string UsersFile::filename = "/tmp/.usersdb"; // fisier de stocare a utilizatorilor
std::mutex UsersFile::Mutex;

UsersFile &UsersFile::getInstance()
{
    std::lock_guard<std::mutex> lock(Mutex);

    if (instance == nullptr)
    {
        instance = new UsersFile();
        instance->loadFromFile();
    }

    return *instance;
}

void UsersFile::destroyInstance()
{
    std::lock_guard<std::mutex> lock(Mutex);
    delete instance;
    instance = nullptr;
}

void UsersFile::loadFromFile()
{
    users.clear();

    std::ifstream in(filename);
    if (!in.is_open())
        return;

    std::string line;
    while (std::getline(in, line))
    {
        if (!line.empty())
        {
            users.push_back(User::fromString(line));
        }
    }

    in.close();
}

void UsersFile::saveToFile() const
{
    std::ofstream out(filename, std::ios::trunc);
    for (const auto &u : users)
    {
        out << u.toString() << "\n";
    }
    out.close();
}

bool UsersFile::addUser(const std::string &username, const std::string &password)
{
    std::lock_guard<std::mutex> lock(Mutex);

    if (userExists(username))
        return false;

    users.emplace_back(username, password);
    saveToFile();
    return true;
}

bool UsersFile::authenticate(const std::string &username, const std::string &password)
{
    std::lock_guard<std::mutex> lock(Mutex);

    for (const auto &u : users)
        if (u.getUsername() == username && u.checkPassword(password))
            return true;

    return false;
}

bool UsersFile::userExists(const std::string &username) const
{
    for (const auto &u : users)
        if (u.getUsername() == username)
            return true;
    return false;
}

std::vector<User> UsersFile::getUsers() const
{
    return this->users;
}