#include "HelpSystem.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

std::string HelpSystem::toLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

std::string HelpSystem::loadFile(std::string filename)
{
    std::ifstream file(filename);

    if (!file.is_open())
    {
        return "ERROR: Help topic not found.";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

std::string HelpSystem::getHelp(std::string command)
{
    std::string topic = toLower(command);

    topic.erase(0, topic.find_first_not_of(" "));

    if (topic.empty())
    {
        topic = "all";
    }

    std::string path = "src/client/help/" + topic + ".txt";

    if (access(path.c_str(), F_OK) != 0)
    {
        return "ERROR: Help topic not found.";
    }

    pid_t pid = fork();

    if (pid == 0)
    {
        execlp("less", "less", "-P(END)", path.c_str(), NULL);
        exit(EXIT_FAILURE);
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
    }

    return loadFile(path);
}