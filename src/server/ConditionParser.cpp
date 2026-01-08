#include "ConditionParser.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <stack>

ConditionParser::ConditionParser(std::string whereClause, const std::vector<Column> &schema) : whereClause(whereClause), schema(schema) {}

std::string ConditionParser::trim(std::string s)
{

    const char *ws = " \t\r\n";
    s.erase(0, s.find_first_not_of(ws));
    s.erase(s.find_last_not_of(ws) + 1);

    if (s.size() >= 2 && s.front() == '(' && s.back() == ')')
    {
        int count = 0;
        bool matching = true;

        for (size_t i = 0; i < s.length() - 1; ++i)
        {
            if (s[i] == '('){
                count++;
            }
            else if (s[i] == ')'){
                count--;
            }

            if (count == 0)
            {
                matching = false;
                break;
            }
        }
        if (matching)
        {
            return trim(s.substr(1, s.length() - 2));
        }
    }
    return s;
}

int ConditionParser::getColIndex(std::string colName)
{
    for (size_t i = 0; i < schema.size(); ++i)
    {
        if (schema[i].name == colName){
            return i;
        }
    }

    return -1;
}

std::string ConditionParser::getValue(const std::vector<std::string> &row, std::string colName)
{
    int idx = getColIndex(colName);

    if (idx != -1 && idx < (int)row.size()){
        return row[idx];
    }

    return "";
}

bool ConditionParser::evaluate(const std::vector<std::string> &row)
{
    if (whereClause.empty()){
        return true;
    }

    return evalExpression(whereClause, row);
}

bool ConditionParser::evalExpression(std::string expr, const std::vector<std::string> &row)
{
    expr = trim(expr);

    std::string upperExpr = expr;
    std::transform(upperExpr.begin(), upperExpr.end(), upperExpr.begin(), ::toupper);

    int parenCount = 0;

    for (size_t i = 0; i < upperExpr.length(); ++i)
    {
        if (upperExpr[i] == '('){
            parenCount++;
        }
        else if (upperExpr[i] == ')'){
            parenCount--;
        }
        else if (parenCount == 0 && i + 2 < upperExpr.length() && upperExpr.substr(i, 4) == " OR "){
            return evalExpression(expr.substr(0, i), row) || evalExpression(expr.substr(i + 4), row);
        }
    }

    parenCount = 0;

    for (size_t i = 0; i < upperExpr.length(); ++i)
    {
        if (upperExpr[i] == '('){
            parenCount++;
        }
        else if (upperExpr[i] == ')'){
            parenCount--;
        }
        else if (parenCount == 0 && i + 3 < upperExpr.length() && upperExpr.substr(i, 5) == " AND "){
            return evalExpression(expr.substr(0, i), row) && evalExpression(expr.substr(i + 5), row);
        }
    }

    std::string op;
    size_t opPos = std::string::npos;

    std::vector<std::string> ops = {"!=", "<=", ">=", "=", "<", ">"};

    for (const auto &o : ops)
    {
        size_t pos = expr.find(o);
    
        if (pos != std::string::npos){
            op = o;
            opPos = pos;
            break;
        }
    }

    if (opPos == std::string::npos){
        return false;
    }

    std::string left = trim(expr.substr(0, opPos));
    std::string right = trim(expr.substr(opPos + op.length()));

    if (right.size() >= 2 && (right.front() == '\'' || right.front() == '"')){
        right = right.substr(1, right.size() - 2);
    }

    std::string leftVal = getValue(row, left);

    if (leftVal.size() >= 2 && (leftVal.front() == '\'' || leftVal.front() == '"')) {
        leftVal = leftVal.substr(1, leftVal.size() - 2);
    }

    if (leftVal.empty() && getColIndex(left) == -1)
    {
        leftVal = left;

        if (leftVal.size() >= 2 && (leftVal.front() == '\'' || leftVal.front() == '"')){
            leftVal = leftVal.substr(1, leftVal.size() - 2);
        }
    }

    bool isInt = false;
    int idx = getColIndex(left);
    
    if (idx != -1 && schema[idx].type == TYPE_INT){
        isInt = true;
    }

    if (isInt){

        try
        {
            int l = std::stoi(leftVal);
            int r = std::stoi(right);

            if (op == "=") { return l == r; }
            if (op == "!=") { return l != r; }
            if (op == "<")  { return l < r; }
            if (op == ">")  { return l > r; }
            if (op == "<=") { return l <= r; }
            if (op == ">=") { return l >= r; }
        }
        catch (...)
        {
            return false;
        }
    }
    else{
        if (op == "=") { return leftVal == right; }
        if (op == "!=") { return leftVal != right; }
        if (op == "<") { return leftVal < right; }
        if (op == ">") { return leftVal > right; }
        if (op == "<=") { return leftVal <= right; }
        if (op == ">=") { return leftVal >= right; }
    }

    return false;
}
