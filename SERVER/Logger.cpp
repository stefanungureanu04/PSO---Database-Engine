#include "Logger.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>

Logger* Logger::instance = nullptr;

std::string Logger::filename = "/tmp/.dblog";

std::mutex Logger::Mutex;

Logger& Logger::getInstance() 
{
    std::lock_guard<std::mutex> lock(Mutex);

    if (Logger::instance == nullptr) {
        Logger::instance = new Logger();
        Logger::instance->loadFromFile();
    }

    return *instance;
}

void Logger::destroyInstance() 
{
    std::lock_guard<std::mutex> lock(Mutex);

    if(Logger::instance != nullptr) {
        delete Logger::instance;
    }

    Logger::instance = nullptr;
}

void Logger::write(const LogEntry &entry) 
{
    std::lock_guard<std::mutex> lock(Mutex);

    this->entries.push_back(entry);

    if (!std::filesystem::exists(filename)) {
        std::ofstream create(filename);
        create.close();
    }

    std::ofstream out(filename, std::ios::app);
    if (!out.is_open()) {
        return;
    }

    out << entry.toString() << "\n";
    out.close();
}

std::vector<LogEntry> Logger::getEntries() const 
{
    std::lock_guard<std::mutex> lock(Mutex);
    
    return this->entries;
}

void Logger::flush() 
{
    std::lock_guard<std::mutex> lock(Mutex);

    std::ofstream out(filename, std::ios::trunc);

    out.close();

    entries.clear();
}

void Logger::loadFromFile() 
{
    this->entries.clear();

    std::ifstream in(filename);
    
    if (!in.is_open())
        return;

    std::string line;
    
    while (std::getline(in, line)) 
    {
        size_t open1 = line.find('[');
        size_t close1 = line.find(']');
        size_t open2 = line.find('[', close1);
        size_t close2 = line.find(']', open2);
        size_t colon = line.find(':', close2);

        if (open1 == std::string::npos || close1 == std::string::npos ||
            open2 == std::string::npos || close2 == std::string::npos ||
            colon == std::string::npos)
            continue;

        std::string timestamp = line.substr(open1 + 1, close1 - open1 - 1);
        std::string levelStr = line.substr(open2 + 1, close2 - open2 - 1);
        std::string username = line.substr(close2 + 2, colon - close2 - 2);
        std::string message = line.substr(colon + 2);

        LogEntry::Level lvl = LogEntry::stringToLevel(levelStr);
        LogEntry entry(username, message, lvl);
        entry.setTimestamp(timestamp);

        entries.push_back(entry);
    }
    
    in.close();
}

std::vector<LogEntry> Logger::getLogsFromDate(const std::string &date) 
{
    std::lock_guard<std::mutex> lock(Mutex);

    std::vector<LogEntry> result;
 
    for (const auto &entry : entries) {
        if (entry.getTimestamp().find(date) != std::string::npos) {
            result.push_back(entry);
        }
    }

    return result;
}

std::vector<LogEntry> Logger::getLogsFromUser(const std::string &username) 
{
    std::lock_guard<std::mutex> lock(Mutex);

    std::vector<LogEntry> result;

    for (const auto &entry : entries) {
        if (entry.getUsername() == username) {
            result.push_back(entry);
        }
    }

    return result;
}

std::vector<LogEntry> Logger::getErrorsFromUser(const std::string &username) 
{
    std::lock_guard<std::mutex> lock(Mutex);

    std::vector<LogEntry> result;

    for (const auto &entry : entries) {
        if (entry.getUsername() == username && entry.getLevel() == "ERROR") {
            result.push_back(entry);
        }
    }

    return result;
}
