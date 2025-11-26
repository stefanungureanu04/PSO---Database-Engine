#include "Table.h"
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

Table::Table(std::string name, std::vector<Column> cols, std::string dbPath)
    : name(name), columns(cols) {
  filename = dbPath + "/" + name + ".db";

  // Create file if it doesn't exist
  int fd = open(filename.c_str(), O_CREAT | O_RDWR, 0644);
  if (fd != -1) {
    close(fd);
  }

  saveSchema();
}

std::string Table::getName() const { return name; }

std::string Table::insert(const std::vector<std::string> &values) {
  std::lock_guard<std::mutex> lock(tableMutex);

  if (values.size() != columns.size()) {
    return "ERROR: Column count mismatch";
  }

  // Validate types (basic validation)
  // ...

  std::stringstream ss;
  for (size_t i = 0; i < values.size(); ++i) {
    ss << values[i];
    if (i < values.size() - 1)
      ss << ",";
  }
  ss << "\n";
  std::string row = ss.str();

  int fd = open(filename.c_str(), O_WRONLY | O_APPEND);
  if (fd == -1) {
    return "ERROR: Could not open table file";
  }

  write(fd, row.c_str(), row.length());
  close(fd);

  return "OK: Row inserted";
}

std::string Table::select() {
  std::lock_guard<std::mutex> lock(tableMutex);

  int fd = open(filename.c_str(), O_RDONLY);
  if (fd == -1) {
    return "ERROR: Could not open table file";
  }

  std::string result = "";
  char buffer[1024];
  int bytesRead;
  while ((bytesRead = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
    buffer[bytesRead] = '\0';
    result += buffer;
  }
  close(fd);

  if (result.empty())
    return "Empty table";
  return result;
}

void Table::saveSchema() {
  std::string schemaPath =
      filename.substr(0, filename.find_last_of('.')) + ".schema";
  std::ofstream outFile(schemaPath);
  if (outFile.is_open()) {
    for (const auto &col : columns) {
      std::string typeStr;
      switch (col.type) {
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
      outFile << col.name << " " << typeStr << "\n";
    }
    outFile.close();
  }
}

Table *Table::loadFromSchema(std::string dbPath, std::string tableName) {
  std::string schemaPath = dbPath + "/" + tableName + ".schema";
  std::ifstream inFile(schemaPath);
  if (!inFile.is_open()) {
    return nullptr;
  }

  std::vector<Column> cols;
  std::string line;
  while (std::getline(inFile, line)) {
    std::stringstream ss(line);
    std::string colName, colTypeStr;
    ss >> colName >> colTypeStr;

    ColumnType type;
    if (colTypeStr == "INT")
      type = TYPE_INT;
    else if (colTypeStr == "VARCHAR")
      type = TYPE_VARCHAR;
    else if (colTypeStr == "DATE")
      type = TYPE_DATE;
    else
      continue; // Skip unknown types

    cols.push_back({colName, type});
  }

  return new Table(tableName, cols, dbPath);
}
