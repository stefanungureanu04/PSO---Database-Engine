#ifndef DATABASE_H
#define DATABASE_H

#include "Table.h"
#include <map>
#include <mutex>
#include <string>
#include <vector>

class Database
{
private:
	std::string owner;
	std::string pathToDb;

	std::map<std::string, Table *> tables;
	std::mutex dbMutex;

	std::string createTable(std::string cmd, std::string &currentDB);
	std::string handleInsert(std::string cmd, std::string &currentDB);
	std::string handleSelect(std::string cmd, std::string &currentDB);
	std::string handleBackup();

	std::string createDatabase(std::string cmd);
	std::string dropDatabase(std::string cmd);
	std::string useDatabase(std::string cmd, std::string &currentDB);
	void loadTables(std::string dbName);

	std::string handleShow(std::string cmd, std::string &currentDB);
	std::string handleLog(std::string username);
	void logOperation(std::string cmd, std::string currentDB, std::string username);

public:
	Database(const std::string &owner);
	~Database();
	std::string executeCommand(std::string cmd, std::string &currentDB, std::string username);
	static std::string toUpper(std::string s);
	std::string getTableContent(std::string tableName, std::string currentDB);
};

#endif