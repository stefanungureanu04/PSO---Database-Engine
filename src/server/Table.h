#ifndef TABLE_H
#define TABLE_H

#include <mutex>
#include <string>
#include <vector>

enum ColumnType {
    TYPE_INT,
    TYPE_VARCHAR,
    TYPE_DATE
};

struct Column {
    std::string name;
    ColumnType type;
};

class Table {
private:
    std::string name;
    std::vector<Column> columns;
    std::string filename;
    std::mutex tableMutex;

    void saveToFile();
    void loadFromFile();

public:
    Table(std::string name, std::vector<Column> cols, std::string dbPath);
    std::string getName() const;
    std::string insert(const std::vector<std::string> &values);
    std::string select(const std::vector<std::string> &columns, const std::string &whereClause, std::string orderByCol = "", bool orderDesc = false);
    void saveSchema();
    static Table *loadFromSchema(std::string dbPath, std::string tableName);
    std::string update(std::string setCol, std::string setVal, std::string whereClause);
    std::string deleteRows(std::string whereClause);
    int getColumnIndex(std::string colName);
};

#endif // TABLE_H
