#include "LogEntry.hpp"
#include <ctime>
#include <iomanip>
#include <sstream>

LogEntry::LogEntry(const std::string &username, const std::string &message, Level level)
{
    this->username = username;
    this->message = message;
    this->level = level;
   
    std::time_t current_time = std::time(nullptr);
    std::tm *local_time = std::localtime(&current_time);

    std::ostringstream oss;
    oss << std::put_time(local_time, "%Y-%m-%d %H:%M:%S");
    
    timestamp = oss.str();
}

const std::string LogEntry::toString() const 
{
    return "[" + this->timestamp + "] " +  "[" + this->getLevel() + "] " + this->username + ": " + this->message;
}

const std::string LogEntry::getTimestamp() const
{
    return this->timestamp;
}

const std::string LogEntry::getUsername() const
{
    return this->username;
}

const std::string LogEntry::getMessage() const
{
    return this->message;
}

const std::string LogEntry::getLevel() const
{
    std::string s_level{};

    if(this->level == Level::INFO){
        s_level = "INFO";
    }
    else if(this->level == Level::ERROR){
        s_level = "ERROR";
    }

    return s_level;
}

void LogEntry::setTimestamp(const std::string &timestamp)
{
    this->timestamp = timestamp;
} 

void LogEntry::setLevel(Level level)
{
    this->level = level;
}

std::string LogEntry::levelToString(Level level)
{
    std::string s_level{};

    if(level == Level::INFO) {
        s_level = "INFO";
    }
    else if(level == Level::ERROR) {
        s_level = "ERROR";
    }

    return s_level;
}

LogEntry::Level LogEntry::stringToLevel(const std::string &s_level) 
{
    if (s_level == "INFO") {
        return Level::INFO;
    }
    else if (s_level == "ERROR") {
        return Level::ERROR;
    }

    return Level::INFO;
}

