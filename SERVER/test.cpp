#include "logger/Logger.hpp"
#include <iostream>

int main() 
{
    Logger &log = Logger::getInstance();

    log.write(LogEntry("system", "Server started", LogEntry::Level::INFO));
    log.write(LogEntry("db", "Connection failed", LogEntry::Level::ERROR));
    log.write(LogEntry("alice", "User login success", LogEntry::Level::INFO));
    log.write(LogEntry("alice", "Failed INSERT query", LogEntry::Level::ERROR));

    std::cout << "\n--- All logs ---\n";
    for (const auto &e : log.getEntries())
        std::cout << e.toString() << "\n";

    std::cout << "\n--- Logs from user alice ---\n";
    for (const auto &e : log.getLogsFromUser("alice"))
        std::cout << e.toString() << "\n";

    std::cout << "\n--- Errors from user alice ---\n";
    for (const auto &e : log.getErrorsFromUser("alice"))
        std::cout << e.toString() << "\n";

    return 0;

}
