#ifndef HELPSYSTEM_H
#define HELPSYSTEM_H

#include <string>

class HelpSystem
{
private:
    static std::string loadFile(std::string filename);
    static std::string toLower(std::string s);

public:
    static std::string getHelp(std::string command);
};

#endif
