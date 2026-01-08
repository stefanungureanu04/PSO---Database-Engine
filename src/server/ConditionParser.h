#ifndef CONDITIONPARSER_H
#define CONDITIONPARSER_H

#include "Table.h"
#include <string>
#include <vector>
#include <map>

class ConditionParser {
private:
    std::string whereClause;
    std::vector<Column> schema;

    std::string getValue(const std::vector<std::string> &row, std::string colName);
    int getColIndex(std::string colName);

    bool evalExpression(std::string expr, const std::vector<std::string> &row);
    std::string trim(std::string s);

public:
    ConditionParser(std::string whereClause, const std::vector<Column> &schema);
    bool evaluate(const std::vector<std::string> &row);
};

#endif
