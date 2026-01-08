#include "Table.h"
#include "ConditionParser.h"
#include <algorithm>
#include <fcntl.h>

#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

Table::Table(std::string name, std::vector<Column> cols, std::string dbPath) : name(name), columns(cols)
{
    filename = dbPath + "/" + name + ".db";

    int fd = open(filename.c_str(), O_CREAT | O_RDWR, 0644);

    if (fd != -1)
    {
        close(fd);
    }

    saveSchema();
}

std::string Table::getName() const 
{ 
    return name; 
}

std::string Table::removeQuotes(std::string str)
{
    str.erase(std::remove(str.begin(), str.end(), '\''), str.end());
    
    return str;
}

bool Table::isValidDate(std::string strdate)
{
    if (strdate.length() != 10){
        return false;
    } 

    if (strdate[4] != '-' || strdate[7] != '-'){
        return false;
    } 

    for (int i = 0; i < 10; i++) {
        
        if (i == 4 || i == 7) {
            continue;
        }
        if (!isdigit(strdate[i])) {
            return false;
        }
    }

    int year, month, day;

    try {
        year = std::stoi(strdate.substr(0, 4));
        month = std::stoi(strdate.substr(5, 2));
        day = std::stoi(strdate.substr(8, 2));
    } catch (...) {
        return false;
    }

    if (month < 1 || month > 12){
        return false;
    }

    if (year < 1600 || year > 2200) {
        return false;
    }

    std::vector<int> daysInMonth = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    bool isLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    
    if (isLeap) {
        daysInMonth[1] = 29;
    }

    if (day < 1 || day > daysInMonth[month - 1]) {
        return false;
    }

    return true;
}

std::string Table::insert(const std::vector<std::string> &values)
{
    std::lock_guard<std::mutex> lock(tableMutex);

    if (values.size() != columns.size()){
        return "ERROR: Column count mismatch";
    }

    std::stringstream ss;

    for (size_t i = 0; i < values.size(); ++i)
    {
        if (columns[i].type == TYPE_DATE) { 
            if (!isValidDate(values[i])) {
                return "ERROR: Invalid date format in column '" + columns[i].name + "'. Expected YYYY-MM-DD.";
            }
        }

        ss << values[i];
    
        if (i < values.size() - 1){
            ss << ",";            
        }
    }

    ss << "\n";
    
    std::string row = ss.str();

    int fd = open(filename.c_str(), O_WRONLY | O_APPEND);
    
    if (fd == -1){
        return "ERROR: Could not open table file";
    }

    write(fd, row.c_str(), row.length());
    close(fd);

    return "OK: Row inserted";
}

std::string Table::select(const std::vector<std::string> &reqColumns, const std::string &whereClause, std::string orderByCol, bool orderDesc)
{
    std::lock_guard<std::mutex> lock(tableMutex);

    std::vector<int> colIndices;

    if (reqColumns.empty()){
        for (size_t i = 0; i < columns.size(); ++i){
            colIndices.push_back(i);
        }
    }

    else{
        for (const auto &col : reqColumns)
        {
            int idx = getColumnIndex(col);

            if (idx == -1){
                return "ERROR: Column " + col + " not found";
            }

            colIndices.push_back(idx);
        }
    }

    ConditionParser parser(whereClause, columns);

    int fd = open(filename.c_str(), O_RDONLY);

    if (fd == -1)
    {
        return "ERROR: Could not open table file";
    }

    std::string fileContent = "";
    char buffer[1024];
    int bytesRead;

    while ((bytesRead = read(fd, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[bytesRead] = '\0';
        fileContent += buffer;
    }

    close(fd);

    if (fileContent.empty()){
        return "Empty table";
    }

    std::stringstream ss(fileContent);
    std::string line;
    bool firstRow = true;
    std::vector<std::vector<std::string>> filteredRows;

    while (std::getline(ss, line))
    {
        if (line.empty() && !firstRow) continue;
        
        firstRow = false;

        if (line.empty()) continue;

        std::stringstream ssRow(line);
        std::string cell;
        std::vector<std::string> row;
        
        while (std::getline(ssRow, cell, ',')){
            row.push_back(cell);
        }

        if (parser.evaluate(row)){
            filteredRows.push_back(row);
        }
    }

    if (!orderByCol.empty())
    {
        int sortIdx = getColumnIndex(orderByCol);

        if (sortIdx == -1){
            return "ERROR: Order by column " + orderByCol + " not found";
        }

        ColumnType type = columns[sortIdx].type;

        std::sort(filteredRows.begin(), filteredRows.end(), [sortIdx, sortDesc = orderDesc, type](const std::vector<std::string> &a, const std::vector<std::string> &b)
        {
          if (a.size() <= (size_t)sortIdx || b.size() <= (size_t)sortIdx){
            return false;
          }
          
          bool less = false;
          
          if (type == TYPE_INT) {
              try {
                  int valA = std::stoi(a[sortIdx]);
                  int valB = std::stoi(b[sortIdx]);
                  less = valA < valB;
              } catch (...) {
                  less = a[sortIdx] < b[sortIdx];
              }
          } else {
              less = a[sortIdx] < b[sortIdx];
          }

          if (sortDesc) return !less && (a[sortIdx] != b[sortIdx]);
          return less; });
    }

    std::string result = "";
    const size_t colWidth = 25; 

    for (size_t i = 0; i < colIndices.size(); ++i)
    {
        int idx = colIndices[i];
        std::string colName = columns[idx].name; 

        result += colName;

        if (colName.length() < colWidth) {
            result += std::string(colWidth - colName.length(), ' ');
        }

        if (i < colIndices.size() - 1) {
            result += " | ";
        }
    }
    result += "\n";

    for (size_t i = 0; i < colIndices.size(); ++i)
    {
        result += std::string(colWidth, '-');
        if (i < colIndices.size() - 1) {
            result += "-+-";
        }
    }
    result += "\n";

    for (size_t r = 0; r < filteredRows.size(); ++r)
    {
        const auto &row = filteredRows[r];
    
        for (size_t i = 0; i < colIndices.size(); ++i)
        {
            int idx = colIndices[i];
            std::string cell = "";

            if (idx < (int)row.size()) {
                cell = removeQuotes(row[idx]);
            }

            result += cell;

            if (cell.length() < colWidth) {
                result += std::string(colWidth - cell.length(), ' ');
            }
    
            if (i < colIndices.size() - 1){
                result += " | ";
            }
        }
        result += "\n";
    }

    if (filteredRows.empty()){
        return "No entries found!";
    }

    return result;
}

void Table::saveSchema()
{
    std::string schemaPath = filename.substr(0, filename.find_last_of('.')) + ".schema";
    
    int fd = open(schemaPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    
    if (fd != -1)
    {
        for (const auto &col : columns)
        {
            std::string typeStr;
            switch (col.type)
            {
            case TYPE_INT:
                typeStr = "INT";
                break;
            case TYPE_VARCHAR:
                typeStr = "VARCHAR";
                break;
            case TYPE_DATE:
                typeStr = "DATE";
                break;
            }
    
            std::string line = col.name + " " + typeStr + "\n";
            write(fd, line.c_str(), line.length());
        }
        close(fd);
    }
}

Table *Table::loadFromSchema(std::string dbPath, std::string tableName)
{
    std::string schemaPath = dbPath + "/" + tableName + ".schema";
    
    int fd = open(schemaPath.c_str(), O_RDONLY);
    
    if (fd == -1)
    {
        return nullptr;
    }

    std::string content;
    char buffer[4096];
    ssize_t bytesRead;
    
    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0)
    {
        content.append(buffer, bytesRead);
    }
    
    close(fd);

    std::vector<Column> cols;
    std::stringstream ssContent(content);
    std::string line;
    
    while (std::getline(ssContent, line))
    {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string colName, colTypeStr;
        ss >> colName >> colTypeStr;

        ColumnType type;
        
        if (colTypeStr == "INT"){
            type = TYPE_INT;
        }
        else if (colTypeStr == "VARCHAR"){
            type = TYPE_VARCHAR;
        }
        else if (colTypeStr == "DATE"){
            type = TYPE_DATE;
        }
        else{
            continue;
        }

        cols.push_back({colName, type});
    }

    return new Table(tableName, cols, dbPath);
}

int Table::getColumnIndex(std::string colName)
{
    for (size_t i = 0; i < columns.size(); ++i){
        if (columns[i].name == colName){
            return i;
        }
    }
    return -1;
}

std::string Table::update(std::string setCol, std::string setVal, std::string whereClause)
{
    std::lock_guard<std::mutex> lock(tableMutex);

    int setIdx = getColumnIndex(setCol);

    if (setIdx == -1){
        return "ERROR: Column " + setCol + " not found";
    }

    if (columns[setIdx].type == TYPE_DATE && !isValidDate(setVal)) {
        return "ERROR: Invalid date format";
    }

    ConditionParser parser(whereClause, columns);

    int fd = open(filename.c_str(), O_RDONLY);
    
    if (fd == -1){
        return "ERROR: Could not open table file";
    }

    std::string content;
    char buffer[4096];
    ssize_t bytesRead;
    
    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0)
    {
        content.append(buffer, bytesRead);
    }
    
    close(fd);

    std::vector<std::string> lines;
    std::string line;
    std::stringstream ssContent(content);
    int updatedCount = 0;

    while (std::getline(ssContent, line))
    {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string cell;
        std::vector<std::string> row;

        while (std::getline(ss, cell, ','))
        {
            row.push_back(cell);
        }

        if (parser.evaluate(row)){
            if (setIdx < (int)row.size()){
                row[setIdx] = setVal;
                updatedCount++;
            }
        }

        std::string newLine;
        for (size_t i = 0; i < row.size(); ++i){
            newLine += row[i];

            if (i < row.size() - 1){
                newLine += ",";
            }
        }
        lines.push_back(newLine);
    }

    fd = open(filename.c_str(), O_WRONLY | O_TRUNC);

    if (fd != -1)
    {
        for (const auto &l : lines){
            std::string lineOut = l + "\n";
            write(fd, lineOut.c_str(), lineOut.length());
        }
        close(fd);
    }

    return "OK: Updated " + std::to_string(updatedCount) + " rows";
}

std::string Table::deleteRows(std::string whereClause)
{
    std::lock_guard<std::mutex> lock(tableMutex);

    ConditionParser parser(whereClause, columns);

    int fd = open(filename.c_str(), O_RDONLY);
    
    if (fd == -1){
        return "ERROR: Could not open table file";
    }

    std::string content;
    char buffer[4096];
    ssize_t bytesRead;

    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0)
    {
        content.append(buffer, bytesRead);
    }
    
    close(fd);

    std::vector<std::string> lines;
    std::string line;
    std::stringstream ssContent(content);
    int deletedCount = 0;

    while (std::getline(ssContent, line))
    {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string cell;
        std::vector<std::string> row;

        while (std::getline(ss, cell, ','))
        {
            row.push_back(cell);
        }

        if (parser.evaluate(row))
        {
            deletedCount++;
            continue;
        }

        lines.push_back(line);
    }

    fd = open(filename.c_str(), O_WRONLY | O_TRUNC);
    
    if (fd != -1)
    {
        for (const auto &l : lines){
            std::string lineOut = l + "\n";
            write(fd, lineOut.c_str(), lineOut.length());
        }
        close(fd);
    }

    return "OK: Deleted " + std::to_string(deletedCount) + " rows";
}
