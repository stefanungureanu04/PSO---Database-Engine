#include "Database.h"
#include "../common/Protocol.h"
#include "Saver.h"

#include <algorithm>
#include <dirent.h>
#include <fcntl.h>
#include <fcntl.h>
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

    std::string msg = "Owner: " + owner + "\n";
    write(STDOUT_FILENO, msg.c_str(), msg.length());

    pathToDb = ".data/" + owner;

    struct stat st;

    if (stat(pathToDb.c_str(), &st) == -1){
        mkdir(pathToDb.c_str(), 0700);
    }

    if (stat(".logs", &st) == -1){
        mkdir(".logs", 0700);
    }
}

Database::~Database()
{
    for (auto const &[name, table] : tables){
        delete table;
    }
}

std::string Database::toUpper(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);

    return s;
}

std::string Database::executeCommand(std::string cmd, std::string &currentDB, std::string username)
{
    std::string upperCmd = toUpper(cmd);

    logOperation(cmd, currentDB, username);

    if (upperCmd.rfind(CMD_CREATE_DB, 0) == 0){
        return createDatabase(cmd);
    }
    else if (upperCmd.rfind(CMD_DROP_DB, 0) == 0){
        return dropDatabase(cmd);
    }
    else if (upperCmd.rfind(CMD_USE, 0) == 0){
        return useDatabase(cmd, currentDB);
    }
    else if (upperCmd == CMD_PWDB){
        return currentDB.empty() ? "No database selected" : currentDB;
    }
    else if (upperCmd.rfind(CMD_SHOW, 0) == 0){
        return handleShow(cmd, currentDB);
    }
    else if (upperCmd == CMD_LOG){
        return handleLog(username);
    }
    else if (upperCmd.rfind(CMD_CREATE_TABLE, 0) == 0){
        
        if (currentDB.empty()){
            return MSG_NO_DB_SELECTED;
        }
        
        return createTable(cmd, currentDB);
    }
    else if (upperCmd.rfind(CMD_INSERT, 0) == 0) {
        
        if (currentDB.empty()){
            return MSG_NO_DB_SELECTED;
        }
        
        return handleInsert(cmd, currentDB);
    }
    else if (upperCmd.rfind(CMD_SELECT, 0) == 0){

        if (currentDB.empty()){
            return MSG_NO_DB_SELECTED;
        }
        if (currentDB.empty()){
            return MSG_NO_DB_SELECTED;
        }

        return handleSelect(cmd, currentDB);
    }
    else if (upperCmd.rfind(CMD_UPDATE, 0) == 0){
        
        if (currentDB.empty()){
            return MSG_NO_DB_SELECTED;
        }

        return handleUpdate(cmd, currentDB);
    }
    else if (upperCmd.rfind(CMD_DELETE, 0) == 0)
    {
        if (currentDB.empty()){
            return MSG_NO_DB_SELECTED;
        }

        return handleDelete(cmd, currentDB);
    }
    else if (upperCmd.rfind(CMD_DROP_TABLE, 0) == 0)
    {
        if (currentDB.empty()){
            return MSG_NO_DB_SELECTED;
        }
        
        return dropTable(cmd, currentDB);
    }
    else if (upperCmd.rfind(CMD_SAVE, 0) == 0){
        std::stringstream ss(cmd);
        std::string segment;
        std::vector<std::string> parts;
     
        while (ss >> segment){
            parts.push_back(segment);
        }

        if (parts.size() < 3){
            return "ERROR: Invalid SAVE command";
        }

        std::string option = toUpper(parts[1]);
        std::string filename = parts.back();

        if (option == OPT_CONFIG){
            return Saver::saveConfig(filename, currentDB, this->owner);
        }
        else if (option == OPT_LOGS){
            return Saver::saveLogs(filename,username);
        }
        else if (option == OPT_TABLE){

            if (parts.size() < 4){
                return "ERROR: Missing table name";
            }
            
            std::string tableName = parts[2];
            return Saver::saveTable(tableName, filename, currentDB, *this);
        }
        
        return "ERROR: Unknown SAVE option";
    }
    return MSG_UNKNOWN_CMD;
}

std::string Database::createDatabase(std::string cmd)
{
    std::string name = cmd.substr(15);
    name.erase(0, name.find_first_not_of(" "));
    name.erase(name.find_last_not_of(" ") + 1);

    std::string path = pathToDb + "/" + name;

    if (mkdir(path.c_str(), 0700) == 0){
        return "OK: Database created";
    }
    else{
        return "ERROR: Could not create database (maybe exists?)";
    }
}

std::string Database::dropDatabase(std::string cmd)
{
    std::string name = cmd.substr(13);
    name.erase(0, name.find_first_not_of(" "));
    name.erase(name.find_last_not_of(" ") + 1);

    pid_t pid = fork();

    if (pid == 0){
        std::string path = pathToDb + "/" + name;
        execlp("rm", "rm", "-rf", path.c_str(), NULL);
        exit(EXIT_FAILURE);
    }
    else{
        int status;
        waitpid(pid, &status, 0);
     
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0){
            return "OK: Database dropped";
        }
    }

    return "ERROR: Failed to drop database";
}

std::string Database::useDatabase(std::string cmd, std::string &currentDB)
{
    std::string name = cmd.substr(3);
    name.erase(0, name.find_first_not_of(" "));
    name.erase(name.find_last_not_of(" ") + 1);

    struct stat st;
    std::string path = pathToDb + "/" + name;

    if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)){
        currentDB = name;
        loadTables(name);
        return "OK: Switched to database " + name;
    }

    return "ERROR: Database does not exist";
}

std::string Database::createTable(std::string cmd, std::string &currentDB)
{
    std::lock_guard<std::mutex> lock(dbMutex);
    size_t openParen = cmd.find('(');
    size_t closeParen = cmd.find(')');

    if (openParen == std::string::npos || closeParen == std::string::npos){
        return "ERROR: Invalid syntax";
    }

    std::string namePart = cmd.substr(13, openParen - 13);
    namePart.erase(0, namePart.find_first_not_of(" "));
    namePart.erase(namePart.find_last_not_of(" ") + 1);

    std::string tableKey = currentDB + "." + namePart;

    if (tables.find(tableKey) != tables.end()){
        return MSG_TABLE_EXISTS;
    }

    std::string colsPart = cmd.substr(openParen + 1, closeParen - openParen - 1);
    std::vector<Column> columns;
    std::stringstream ss(colsPart);
    std::string segment;

    while (std::getline(ss, segment, ','))
    {
        std::stringstream ss2(segment);
        std::string colName, colTypeStr;
        ss2 >> colName >> colTypeStr;

        std::string upperType = toUpper(colTypeStr);

        ColumnType type;
        if (upperType == TYPE_INT_STR){
            type = TYPE_INT;
        }
        else if (upperType.find(TYPE_VARCHAR_STR) == 0){
            type = TYPE_VARCHAR;
        }
        else if (upperType == TYPE_DATE_STR){
            type = TYPE_DATE;
        }
        else{
            return "ERROR: Unknown type " + colTypeStr;
        }

        columns.push_back({colName, type});
    }

    std::string dbPath = pathToDb + "/" + currentDB;
    tables[tableKey] = new Table(namePart, columns, dbPath);
    return "OK: Table created";
}

std::string Database::handleInsert(std::string cmd, std::string &currentDB)
{
    size_t valuesPos = toUpper(cmd).find(KEYWORD_VALUES);
    
    if (valuesPos == std::string::npos){
        return "ERROR: Syntax error";
    }

    std::string namePart = cmd.substr(12, valuesPos - 12);
    namePart.erase(0, namePart.find_first_not_of(" "));
    namePart.erase(namePart.find_last_not_of(" ") + 1);

    std::string tableKey = currentDB + "." + namePart;

    std::lock_guard<std::mutex> lock(dbMutex);

    if (tables.find(tableKey) == tables.end()){
        return MSG_TABLE_NOT_FOUND;
    }

    size_t openParen = cmd.find('(', valuesPos);
    size_t closeParen = cmd.find(')', openParen);
    std::string valPart = cmd.substr(openParen + 1, closeParen - openParen - 1);

    std::vector<std::string> values;
    std::stringstream ss(valPart);
    std::string val;

    while (std::getline(ss, val, ','))
    {
        val.erase(0, val.find_first_not_of(" '\""));
        val.erase(val.find_last_not_of(" '\"") + 1);
        values.push_back(val);
    }

    return tables[tableKey]->insert(values);
}

std::string Database::handleSelect(std::string cmd, std::string &currentDB)
{
    std::string upperCmd = toUpper(cmd);
    size_t selectPos = upperCmd.find(CMD_SELECT);
    size_t fromPos = upperCmd.find(KEYWORD_FROM);

    if (fromPos == std::string::npos || selectPos == std::string::npos){
        return MSG_SYNTAX_ERROR;
    }

    std::string colsPart = cmd.substr(selectPos + 6, fromPos - (selectPos + 6));
    std::vector<std::string> columns;
    std::stringstream ss(colsPart);
    std::string col;

    while (std::getline(ss, col, ','))
    {
        col.erase(0, col.find_first_not_of(" "));
        col.erase(col.find_last_not_of(" ") + 1);

        if (col != "*"){
            columns.push_back(col);
        }
    }

    size_t wherePos = upperCmd.find(KEYWORD_WHERE);
    size_t orderByPos = upperCmd.find(KEYWORD_ORDER_BY);

    std::string tableName;
    std::string whereClause = "";
    std::string orderByCol = "";
    bool orderDesc = false;

    if (orderByPos != std::string::npos){

        std::string orderPart = cmd.substr(orderByPos + 9);
        std::stringstream ssOrder(orderPart);
        std::string dir;
        ssOrder >> orderByCol >> dir;

        orderByCol.erase(0, orderByCol.find_first_not_of(" "));
        orderByCol.erase(orderByCol.find_last_not_of(" ") + 1);

        std::string upperDir = toUpper(dir);
        if (upperDir == "DESC"){
            orderDesc = true;
        }
    }

    size_t endOfQuery = (orderByPos == std::string::npos) ? cmd.length() : orderByPos;

    if (wherePos == std::string::npos){
        tableName = cmd.substr(fromPos + 5, endOfQuery - (fromPos + 5));
    }
    else{

        if (wherePos > endOfQuery){
            return "ERROR: WHERE must precede ORDER BY";
        }

        tableName = cmd.substr(fromPos + 5, wherePos - (fromPos + 5));
        whereClause = cmd.substr(wherePos + 6, endOfQuery - (wherePos + 6));
    }

    tableName.erase(0, tableName.find_first_not_of(" "));
    tableName.erase(tableName.find_last_not_of(" ") + 1);

    whereClause.erase(0, whereClause.find_first_not_of(" "));
    whereClause.erase(whereClause.find_last_not_of(" ") + 1);

    std::string tableKey = currentDB + "." + tableName;

    std::lock_guard<std::mutex> lock(dbMutex);
    
    if (tables.find(tableKey) == tables.end()){
        return MSG_TABLE_NOT_FOUND;
    }

    return tables[tableKey]->select(columns, whereClause, orderByCol, orderDesc);
}

std::string Database::handleBackup()
{
    pid_t pid = fork();

    if (pid == -1){
        return "ERROR: Fork failed";
    }
    else if (pid == 0){
        mkdir("backup", 0700);
        execlp("cp", "cp", "-r", "data", "backup/", NULL);
        exit(EXIT_FAILURE);
    }
    else{
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0){
            return "OK: Backup completed";
        }

        else{
            return "ERROR: Backup failed";
        }
    }
}

std::string Database::handleShow(std::string cmd, std::string &currentDB)
{
    std::string upperCmd = toUpper(cmd);
    std::string path;

    std::string result = "";
    DIR *dir;
    struct dirent *ent;

    if (upperCmd.find(OPT_DBS) != std::string::npos)
    {
        if ((dir = opendir(pathToDb.c_str())) != NULL){

            while ((ent = readdir(dir)) != NULL)
            {
                if (ent->d_type == DT_DIR){
                    std::string name = ent->d_name;
                    if (name != "." && name != ".."){
                        result += name + "\n";
                    }
                }
            }
            closedir(dir);
        }
        else{
            return "ERROR: Could not open DB directory";
        }
    }
    else if (upperCmd.find(OPT_TABLES) != std::string::npos)
    {
        if (currentDB.empty()){
            return "ERROR: No database selected";
        }

        path = pathToDb + "/" + currentDB;

        if ((dir = opendir(path.c_str())) != NULL){

            while ((ent = readdir(dir)) != NULL)
            {
                std::string fname = ent->d_name;
                if (fname.length() > 7 && fname.substr(fname.length() - 7) == ".schema")
                {
                    std::string tableName = fname.substr(0, fname.length() - 7);
                    result += tableName + "\n";

                    int fd = open((path + "/" + fname).c_str(), O_RDONLY);

                    if (fd != -1)
                    {
                        char buffer[4096];
                        std::string content;
                        ssize_t bytesRead;

                        while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0)
                        {
                            content.append(buffer, bytesRead);
                        }

                        close(fd);

                        std::stringstream schemaSS(content);
                        std::string line;

                        while (std::getline(schemaSS, line))
                        {
                            if (!line.empty()){
                                std::stringstream ss(line);
                                std::string colName, colType;
                                ss >> colName >> colType;
                                result += "     * " + colName + " (" + colType + ")\n";
                            }
                        }
                    }
                }
            }
            closedir(dir);
        }
        else{
            return "ERROR: Could not open database directory";
        }
    }
    else{
        return "ERROR: Invalid SHOW option. Use -databases or -tables";
    }

    if (result.empty()){
        return "No items found.";
    }
    if (result.back() == '\n'){
        result.pop_back();
    }

    return result;
}

void Database::logOperation(std::string cmd, std::string currentDB, std::string username)
{
    std::lock_guard<std::mutex> lock(dbMutex);
    std::string logFile = ".logs/server.log";

    int fd = open(logFile.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    
    if (fd != -1){

        time_t now = time(0);
        char *dt = ctime(&now);
        std::string timestamp(dt);
        timestamp.pop_back();

        std::string logEntry = "[" + timestamp + "] [" + username + "] [" +
                               (currentDB.empty() ? "NONE" : currentDB) + "] " +
                               cmd + "\n";

        write(fd, logEntry.c_str(), logEntry.length());
        close(fd);
    }
}

std::string Database::handleLog(std::string username)
{
    int pipefd[2];

    if (pipe(pipefd) == -1){
        return "ERROR: Pipe failed";
    }

    pid_t pid = fork();
    
    if (pid == -1){
        close(pipefd[0]);
        close(pipefd[1]);
        return "ERROR: Fork failed";
    }
    else if (pid == 0){
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        std::string logFile = ".logs/server.log";
        std::string pattern = "\\[" + username + "\\]";

        execlp("grep", "grep", pattern.c_str(), logFile.c_str(), NULL);
        exit(EXIT_FAILURE);
    }
    else{
        close(pipefd[1]);
        char buffer[1024];
        std::string result;
        int bytesRead;

        while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0)
        {
            buffer[bytesRead] = '\0';
            result += buffer;
        }

        close(pipefd[0]);
        wait(NULL);

        if (result.empty()){
            return "Log is empty. No entries for user";
        }
        
        return result;
    }
}

std::string Database::getTableContent(std::string tableName, std::string currentDB)
{
    std::string tableKey = currentDB + "." + tableName;
    std::lock_guard<std::mutex> lock(dbMutex);

    if (tables.find(tableKey) != tables.end()){
        return tables[tableKey]->select({}, "");
    }

    return "";
}

void Database::loadTables(std::string dbName)
{
    std::string path = pathToDb + "/" + dbName;

    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir(path.c_str())) != NULL){

        while ((ent = readdir(dir)) != NULL)
        {
            std::string filename = ent->d_name;
            
            if (filename.length() > 7 && filename.substr(filename.length() - 7) == ".schema"){

                std::string tableName = filename.substr(0, filename.length() - 7);
                std::string tableKey = dbName + "." + tableName;

                std::lock_guard<std::mutex> lock(dbMutex);

                if (tables.find(tableKey) == tables.end()){
                
                    Table *t = Table::loadFromSchema(path, tableName);
                
                    if (t)
                    {
                        tables[tableKey] = t;
                    }
                }
            }
        }
        closedir(dir);
    }
}

std::string Database::handleUpdate(std::string cmd, std::string &currentDB)
{
    std::string upperCmd = toUpper(cmd);
    size_t setPos = upperCmd.find(KEYWORD_SET);
    size_t wherePos = upperCmd.find(KEYWORD_WHERE);

    if (setPos == std::string::npos || wherePos == std::string::npos){
        return "ERROR: Invalid syntax (UPDATE table SET col=val WHERE ...)";
    }

    std::string tableName = cmd.substr(7, setPos - 7);
    tableName.erase(0, tableName.find_first_not_of(" "));
    tableName.erase(tableName.find_last_not_of(" ") + 1);

    std::string setPart = cmd.substr(setPos + 3, wherePos - (setPos + 3));
    std::string whereClause = cmd.substr(wherePos + 6);

    size_t setEq = setPart.find('=');
    
    if (setEq == std::string::npos){
        return "ERROR: Invalid SET clause";
    }

    std::string setCol = setPart.substr(0, setEq);
    std::string setVal = setPart.substr(setEq + 1);

    setCol.erase(0, setCol.find_first_not_of(" "));
    setCol.erase(setCol.find_last_not_of(" ") + 1);
    setVal.erase(0, setVal.find_first_not_of(" '\""));
    setVal.erase(setVal.find_last_not_of(" '\"") + 1);

    std::string tableKey = currentDB + "." + tableName;
    std::lock_guard<std::mutex> lock(dbMutex);
    
    if (tables.find(tableKey) == tables.end()){
        return MSG_TABLE_NOT_FOUND;
    }

    return tables[tableKey]->update(setCol, setVal, whereClause);
}

std::string Database::handleDelete(std::string cmd, std::string &currentDB)
{
    std::string upperCmd = toUpper(cmd);
    size_t fromPos = upperCmd.find(KEYWORD_FROM);
    size_t wherePos = upperCmd.find(KEYWORD_WHERE);

    if (fromPos == std::string::npos || wherePos == std::string::npos){
        return "ERROR: Invalid syntax (DELETE FROM table WHERE ...)";
    }

    std::string tableName = cmd.substr(fromPos + 4, wherePos - (fromPos + 4));
    tableName.erase(0, tableName.find_first_not_of(" "));
    tableName.erase(tableName.find_last_not_of(" ") + 1);

    std::string whereClause = cmd.substr(wherePos + 6);

    std::string tableKey = currentDB + "." + tableName;
    std::lock_guard<std::mutex> lock(dbMutex);

    if (tables.find(tableKey) == tables.end()){
        return MSG_TABLE_NOT_FOUND;
    }

    return tables[tableKey]->deleteRows(whereClause);
}

std::string Database::dropTable(std::string cmd, std::string &currentDB)
{
    std::string upperCmd = toUpper(cmd);
    std::string tableName = cmd.substr(10);
    tableName.erase(0, tableName.find_first_not_of(" "));
    tableName.erase(tableName.find_last_not_of(" ") + 1);

    std::string tableKey = currentDB + "." + tableName;
    std::lock_guard<std::mutex> lock(dbMutex);

    auto it = tables.find(tableKey);

    if (it == tables.end()){
        return MSG_TABLE_NOT_FOUND;
    }

    std::string dbPath = pathToDb + "/" + currentDB;
    std::string dataFile = dbPath + "/" + tableName + ".db";
    std::string schemaFile = dbPath + "/" + tableName + ".schema";

    unlink(dataFile.c_str());
    unlink(schemaFile.c_str());

    delete it->second;
    tables.erase(it);

    return "OK: Table dropped";
}
