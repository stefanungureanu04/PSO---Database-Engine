#include "UsersFile.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>
#include <iostream>
#include <cerrno>
#include <algorithm>
#include <sys/file.h>

UsersFile *UsersFile::instance = nullptr;
std::string UsersFile::filename = "/tmp/.usersdb"; 
std::mutex UsersFile::Mutex;

UsersFile &UsersFile::getInstance()
{
    std::lock_guard<std::mutex> lock(Mutex);

    if (instance == nullptr)
    {
        instance = new UsersFile();
        instance->loadFromFile(); // incarcare useri din fisier
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

    int fd = open(filename.c_str(), O_RDONLY);
    if (fd == -1)
    {
        perror("Failed to open file");
        return;
    }

    if (flock(fd, LOCK_SH) == -1)
    {
        perror("Failed to lock file");
        close(fd);
        return;
    }

    char buffer[1024];
    ssize_t bytesRead;
    std::string fileContent;

    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0)
    {
        fileContent.append(buffer, bytesRead);
    }

    if (bytesRead == -1)
    {
        perror("Failed to read file");
    }

    size_t pos = 0;
    while ((pos = fileContent.find('\n')) != std::string::npos)
    {
        std::string line = fileContent.substr(0, pos);
        if (!line.empty())
        {
            users.push_back(User::fromString(line));
        }
        fileContent.erase(0, pos + 1);
    }

    flock(fd, LOCK_UN);
    close(fd);
}

void UsersFile::saveToFile() const
{
    int fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
        perror("Failed to open file");
        return;
    }

    if (flock(fd, LOCK_EX) == -1)
    {
        perror("Failed to lock file");
        close(fd);
        return;
    }

    for (const auto &user : users)
    {
        std::string userStr = user.toString() + "\n";
        ssize_t bytesWritten = write(fd, userStr.c_str(), userStr.size());
        if (bytesWritten == -1)
        {
            perror("Failed to write to file");
            flock(fd, LOCK_UN);
            close(fd);
            return;
        }
    }

    flock(fd, LOCK_UN);
    close(fd);
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

    for (const auto &user : users)
    {
        if (user.getUsername() == username && user.checkPassword(password))
            return true;
    }

    return false;
}

bool UsersFile::userExists(const std::string &username) const
{
    for (const auto &user : users)
    {
        if (user.getUsername() == username)
            return true;
    }
    return false;
}

std::vector<User> UsersFile::getUsers() const
{
    return this->users;
}