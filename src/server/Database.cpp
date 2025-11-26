#include "Database.h"
#include "Protocol.h"
#include "Saver.h"
#include <algorithm>
#include <dirent.h>
#include <fcntl.h>
#include <iostream>
#include <pwd.h>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

Database::Database(const std::string &owner) 
{
  this->owner = owner;

  std::cout << "Owner: " << owner << std::endl;
  
  pathToDb = ".data/" + owner;

  struct stat st;
  if (stat(pathToDb.c_str(), &st) == -1) {
    mkdir(pathToDb.c_str(), 0700);
  }
  // Ensure logs directory exists
  if (stat(".logs", &st) == -1) {
    mkdir(".logs", 0700);
  }
}

Database::~Database() {
  for (auto const &[name, table] : tables) {
    delete table;
  }
}

// Helper to convert string to uppercase for comparison
std::string Database::toUpper(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), ::toupper);
  return s;
}

std::string Database::executeCommand(std::string cmd, std::string &currentDB,
                                     std::string username) {
  std::string upperCmd = toUpper(cmd);

  // Log operation
  logOperation(cmd, currentDB, username);

  if (upperCmd.rfind("CREATE DATABASE", 0) == 0) {
    return createDatabase(cmd);
  } else if (upperCmd.rfind("DROP DATABASE", 0) == 0) {
    return dropDatabase(cmd);
  } else if (upperCmd.rfind("USE", 0) == 0) {
    return useDatabase(cmd, currentDB);
  } else if (upperCmd == "PWDB") {
    return currentDB.empty() ? "No database selected" : currentDB;
  } else if (upperCmd.rfind("SHOW", 0) == 0) {
    return handleShow(cmd, currentDB);
  } else if (upperCmd == "LOG") {
    return handleLog(username);
  } else if (upperCmd.rfind("CREATE TABLE", 0) == 0) {
    if (currentDB.empty())
      return "ERROR: No database selected. Use USE <dbname> first.";
    return createTable(cmd, currentDB);
  } else if (upperCmd.rfind("INSERT INTO", 0) == 0) {
    if (currentDB.empty())
      return "ERROR: No database selected. Use USE <dbname> first.";
    return handleInsert(cmd, currentDB);
  } else if (upperCmd.rfind("SELECT", 0) == 0) {
    if (currentDB.empty())
      return "ERROR: No database selected. Use USE <dbname> first.";
    return handleSelect(cmd, currentDB);
  } else if (upperCmd ==
             toUpper(CMD_BACKUP)) { // CMD_BACKUP is a constant, convert to
                                    // upper for comparison
    return handleBackup();
  } else if (upperCmd.rfind("SAVE", 0) == 0) {
    // Format: SAVE -configuration <filename>
    //         SAVE -logs <filename>
    //         SAVE -table <tablename> <filename>
    std::stringstream ss(cmd);
    std::string segment;
    std::vector<std::string> parts;
    while (ss >> segment)
      parts.push_back(segment);

    if (parts.size() < 3)
      return "ERROR: Invalid SAVE command";

    std::string option = toUpper(parts[1]);
    std::string filename = parts.back();

    if (option == "-CONFIGURATION") {
      return Saver::saveConfig(filename, currentDB, this->owner);
    } else if (option == "-LOGS") {
      return Saver::saveLogs(filename);
    } else if (option == "-TABLE") {
      if (parts.size() < 4)
        return "ERROR: Missing table name";
      std::string tableName = parts[2];
      return Saver::saveTable(tableName, filename, currentDB, *this);
    }
    return "ERROR: Unknown SAVE option";
  }
  return "ERROR: Unknown command";
}

std::string Database::createDatabase(std::string cmd) {
  std::string name = cmd.substr(15);
  name.erase(0, name.find_first_not_of(" "));
  name.erase(name.find_last_not_of(" ") + 1);

  std::string path = pathToDb + "/" + name;

  if (mkdir(path.c_str(), 0700) == 0) {
    return "OK: Database created";
  } else {
    return "ERROR: Could not create database (maybe exists?)";
  }
}

std::string Database::dropDatabase(std::string cmd) {
  std::string name = cmd.substr(13);
  name.erase(0, name.find_first_not_of(" "));
  name.erase(name.find_last_not_of(" ") + 1);

  pid_t pid = fork();
  if (pid == 0) {
    std::string path = pathToDb + "/" + name;
    execlp("rm", "rm", "-rf", path.c_str(), NULL);
    exit(EXIT_FAILURE);
  } else {
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
      return "OK: Database dropped";
    }
  }
  return "ERROR: Failed to drop database";
}

std::string Database::useDatabase(std::string cmd, std::string &currentDB) {
  std::string name = cmd.substr(3);
  name.erase(0, name.find_first_not_of(" "));
  name.erase(name.find_last_not_of(" ") + 1);

  struct stat st;
  std::string path = pathToDb + "/" + name;
  if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
    currentDB = name;
    loadTables(name);
    return "OK: Switched to database " + name;
  }
  return "ERROR: Database does not exist";
}

std::string Database::createTable(std::string cmd, std::string &currentDB) {
  std::lock_guard<std::mutex> lock(dbMutex);
  size_t openParen = cmd.find('(');
  size_t closeParen = cmd.find(')');
  if (openParen == std::string::npos || closeParen == std::string::npos) {
    return "ERROR: Invalid syntax";
  }

  std::string namePart = cmd.substr(13, openParen - 13);
  namePart.erase(0, namePart.find_first_not_of(" "));
  namePart.erase(namePart.find_last_not_of(" ") + 1);

  std::string tableKey = currentDB + "." + namePart;

  if (tables.find(tableKey) != tables.end()) {
    return "ERROR: Table already exists";
  }

  std::string colsPart = cmd.substr(openParen + 1, closeParen - openParen - 1);
  std::vector<Column> columns;
  std::stringstream ss(colsPart);
  std::string segment;

  while (std::getline(ss, segment, ',')) {
    std::stringstream ss2(segment);
    std::string colName, colTypeStr;
    ss2 >> colName >> colTypeStr;

    std::string upperType = toUpper(colTypeStr);

    ColumnType type;
    if (upperType == "INT")
      type = TYPE_INT;
    else if (upperType.find("VARCHAR") == 0)
      type = TYPE_VARCHAR;
    else if (upperType == "DATE")
      type = TYPE_DATE;
    else
      return "ERROR: Unknown type " + colTypeStr;

    columns.push_back({colName, type});
  }

  std::string dbPath = pathToDb + "/" + currentDB;
  tables[tableKey] = new Table(namePart, columns, dbPath);
  return "OK: Table created";
}

std::string Database::handleInsert(std::string cmd, std::string &currentDB) {
  size_t valuesPos = toUpper(cmd).find("VALUES");
  if (valuesPos == std::string::npos)
    return "ERROR: Syntax error";

  std::string namePart = cmd.substr(12, valuesPos - 12);
  namePart.erase(0, namePart.find_first_not_of(" "));
  namePart.erase(namePart.find_last_not_of(" ") + 1);

  std::string tableKey = currentDB + "." + namePart;

  std::lock_guard<std::mutex> lock(dbMutex);
  if (tables.find(tableKey) == tables.end()) {
    return "ERROR: Table not found";
  }

  size_t openParen = cmd.find('(', valuesPos);
  size_t closeParen = cmd.find(')', openParen);
  std::string valPart = cmd.substr(openParen + 1, closeParen - openParen - 1);

  std::vector<std::string> values;
  std::stringstream ss(valPart);
  std::string val;
  while (std::getline(ss, val, ',')) {
    val.erase(0, val.find_first_not_of(" '\""));
    val.erase(val.find_last_not_of(" '\"") + 1);
    values.push_back(val);
  }

  return tables[tableKey]->insert(values);
}

std::string Database::handleSelect(std::string cmd, std::string &currentDB) {
  size_t fromPos = toUpper(cmd).find("FROM");
  if (fromPos == std::string::npos)
    return "ERROR: Syntax error";

  std::string namePart = cmd.substr(fromPos + 5);
  namePart.erase(0, namePart.find_first_not_of(" "));
  namePart.erase(namePart.find_last_not_of(" ") + 1);

  std::string tableKey = currentDB + "." + namePart;

  std::lock_guard<std::mutex> lock(dbMutex);
  if (tables.find(tableKey) == tables.end()) {
    return "ERROR: Table not found";
  }

  return tables[tableKey]->select();
}

std::string Database::handleBackup() {
  pid_t pid = fork();

  if (pid == -1) {
    return "ERROR: Fork failed";
  } else if (pid == 0) {
    // Child process
    // Create backup directory
    mkdir("backup", 0700);

    // Execute copy command
    // cp -r data/* backup/
    execlp("cp", "cp", "-r", "data", "backup/", NULL);

    // If execlp returns, it failed
    exit(EXIT_FAILURE);
  } else {
    // Parent process
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
      return "OK: Backup completed";
    } else {
      return "ERROR: Backup failed";
    }
  }
}

std::string Database::handleShow(std::string cmd, std::string &currentDB) {
  std::string upperCmd = toUpper(cmd);
  std::string path;

  if (upperCmd.find("-DATABASES") != std::string::npos) {
    path = pathToDb;
    std::cout << path << std::endl;
  } else if (upperCmd.find("-TABLES") != std::string::npos) {
    if (currentDB.empty())
      return "ERROR: No database selected";
    path = pathToDb + "/" + currentDB;
  } else {
    return "ERROR: Invalid SHOW option. Use -databases or -tables";
  }

  int pipefd[2];
  if (pipe(pipefd) == -1)
    return "ERROR: Pipe failed";

  pid_t pid = fork();
  if (pid == -1) {
    close(pipefd[0]);
    close(pipefd[1]);
    return "ERROR: Fork failed";
  } else if (pid == 0) {
    // Child
    close(pipefd[0]);
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]);
    execlp("ls", "ls", path.c_str(), NULL);
    exit(EXIT_FAILURE);
  } else {
    // Parent
    close(pipefd[1]);
    char buffer[1024];
    std::string result;
    int bytesRead;
    while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
      buffer[bytesRead] = '\0';
      result += buffer;
    }
    close(pipefd[0]);
    wait(NULL);
    if (result.empty())
      return "Empty";
    return result;
  }
}

void Database::logOperation(std::string cmd, std::string currentDB,
                            std::string username) {
  std::lock_guard<std::mutex> lock(dbMutex);
  std::string logFile = ".logs/server.log";
  int fd = open(logFile.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
  if (fd != -1) {
    time_t now = time(0);
    char *dt = ctime(&now);
    std::string timestamp(dt);
    timestamp.pop_back(); // Remove newline

    std::string logEntry = "[" + timestamp + "] [" + username + "] [" +
                           (currentDB.empty() ? "NONE" : currentDB) + "] " +
                           cmd + "\n";
    write(fd, logEntry.c_str(), logEntry.length());
    close(fd);
  }
}

std::string Database::handleLog(std::string username) {
  int pipefd[2];
  if (pipe(pipefd) == -1)
    return "ERROR: Pipe failed";

  pid_t pid = fork();
  if (pid == -1) {
    close(pipefd[0]);
    close(pipefd[1]);
    return "ERROR: Fork failed";
  } else if (pid == 0) {
    // Child
    close(pipefd[0]);
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]);
    std::string logFile = ".logs/server.log";
    std::string pattern = "\\[" + username + "\\]";
    execlp("grep", "grep", pattern.c_str(), logFile.c_str(), NULL);
    exit(EXIT_FAILURE);
  } else {
    // Parent
    close(pipefd[1]);
    char buffer[1024];
    std::string result;
    int bytesRead;
    while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
      buffer[bytesRead] = '\0';
      result += buffer;
    }
    close(pipefd[0]);
    wait(NULL);
    if (result.empty())
      return "Log is empty or no entries for user";
    return result;
  }
}

std::string Database::getTableContent(std::string tableName,
                                      std::string currentDB) {
  std::string tableKey = currentDB + "." + tableName;
  std::lock_guard<std::mutex> lock(dbMutex);
  if (tables.find(tableKey) != tables.end()) {
    return tables[tableKey]->select();
  }
  return "";
}

void Database::loadTables(std::string dbName) {

  std::string path = pathToDb + "/" + dbName;

  DIR *dir;
  struct dirent *ent;

  if ((dir = opendir(path.c_str())) != NULL) {
    while ((ent = readdir(dir)) != NULL) {
      std::string filename = ent->d_name;
      if (filename.length() > 7 &&
          filename.substr(filename.length() - 7) == ".schema") {
        std::string tableName = filename.substr(0, filename.length() - 7);
        std::string tableKey = dbName + "." + tableName;

        std::lock_guard<std::mutex> lock(dbMutex);
        if (tables.find(tableKey) == tables.end()) {
          Table *t = Table::loadFromSchema(path, tableName);
          if (t) {
            tables[tableKey] = t;
          }
        }
      }
    }
    closedir(dir);
  }
}
