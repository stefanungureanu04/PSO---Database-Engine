#pragma once
#include <string>

class LogEntry {
public:
    enum class Level {
        INFO,
        ERROR
    };

private:
    std::string timestamp;
    std::string username;
    std::string message;
    Level level;

public:
    LogEntry(const std::string &username, const std::string &message, Level level);
    const std::string toString() const;

    const std::string getTimestamp() const;
    const std::string getUsername() const; 
    const std::string getMessage() const;
    const std::string getLevel() const;

    void setTimestamp(const std::string &timestamp);
    void setLevel(Level level);

    static std::string levelToString(Level lvl);
    static Level stringToLevel(const std::string &str);
};
