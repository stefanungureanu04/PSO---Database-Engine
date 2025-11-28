#ifndef SAVER_H
#define SAVER_H

#include "Database.h"
#include <string>
#include <vector>

class Saver
{
public:
	static std::string saveConfig(std::string filename, std::string currentDB, std::string owner);
	static std::string saveLogs(std::string filename);
	static std::string saveTable(std::string tableName, std::string filename, std::string currentDB, Database &db);
};

#endif