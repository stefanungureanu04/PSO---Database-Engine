#pragma once
#include <string>
#include <vector>
#include <mutex>
#include "LogEntry.hpp"

class Logger {
private:
    static Logger* instance;

    static std::string filename;
    static std::mutex Mutex;
    std::vector<LogEntry> entries;

public:
    static Logger& getInstance();
    static void destroyInstance();

    void writeEntry(const LogEntry& entry);
    std::vector<LogEntry> getEntries() const;
    void flush();

    std::vector<LogEntry> getLogsFromDate(const std::string& date);
    std::vector<LogEntry> getLogsFromUser(const std::string& username);
    std::vector<LogEntry> getErrorsFromUser(const std::string& username);

private:
    void loadFromFile();
};