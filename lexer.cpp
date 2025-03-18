#include "lexer.h"
#include "utils.h"
#include <iostream>
#include <regex>
#include <map>
#include <vector>
#include <any>

#define ICASE std::regex_constants::icase

Lexer::Lexer() {}

std::string serializeColumns(const std::vector<std::map<std::string, std::string>>& columns) {
    std::string result;
    for (const auto& column : columns) {
        result += column.at("name") + " " + column.at("type");
        if (!column.at("constraints").empty()) {
            result += " " + column.at("constraints");
        }
        result += ";";
    }
    return result;
}

std::map<std::string, std::any> parseCreate(const std::string& sql) {
    std::map<std::string, std::any> result = { {"type", "CREATE"}, {"status", false} };

    std::regex createPattern(R"(CREATE\s+(DATABASE|TABLE)\s+(\w+)\s*(?:\(([\s\S]*?)\))?\s*;)", ICASE);
    std::smatch match;

    if (std::regex_search(sql, match, createPattern)) {
        result["status"] = true;
        result["object_type"] = match[1].str();
        result["object_name"] = match[2].str();

        if (result["object_type"] == "TABLE" && match[3].matched) {
            std::string columnsStr = match[3];
            std::regex columnPattern(R"(\s*(\w+)\s+(\w+)\s*([\s\S]*?)\s*(?:,|$))");
            std::smatch columnMatch;
            std::vector<std::map<std::string, std::string>> columns;

            auto columnBegin = std::sregex_iterator(columnsStr.begin(), columnsStr.end(), columnPattern);
            auto columnEnd = std::sregex_iterator();

            for (std::sregex_iterator it = columnBegin; it != columnEnd; ++it) {
                std::map<std::string, std::string> column;
                column["name"] = (*it)[1].str();
                column["type"] = (*it)[2].str();
                column["constraints"] = (*it)[3].str();
                columns.push_back(column);
            }
            result["columns"] = columns;
        }
    }
    return result;
}

std::map<std::string,std::any> parseDrop(const std::string& sql){
    std::map<std::string,std::any> result = { {"type","DROP"}, {"status",false}, {"restrict",true} };
    std::regex pattern(R"(DROP\s(DATABASE|TABLE)\s(\w+)\s*(RESTRICT|CASCADE)*;)",ICASE);
    std::smatch match;
    if(std::regex_search(sql,match,pattern)){
        result["status"]=true;
        result["object_type"]=match[1].str();
        result["object_name"]=match[2].str();
        
        if (match[3].matched) {
            result["restrict"] = (match[3].str() == "RESTRICT");
        }
    }
    return result;
}

std::map<std::string, std::any> parseInsert(const std::string& sql) {
    std::map<std::string, std::any> result = { {"type", "INSERT"}, {"status", false} };

    std::regex pattern(R"(^INSERT\s+INTO\s+(\w+)\s*\(([^)]+)\)\s*VALUES\s*(.+);)", std::regex::icase);
    std::smatch match;

    if (std::regex_search(sql, match, pattern)) {
        result["status"] = true;
        result["table"] = match[1].str();

        std::vector<std::string> columns;
        std::regex colPattern(R"(\w+)");
        auto colBegin = std::sregex_iterator(match[2].str().begin(), match[2].str().end(), colPattern);
        auto colEnd = std::sregex_iterator();

        for (auto it = colBegin; it != colEnd; ++it) {
            columns.push_back(it->str());
        }
        result["columns"] = columns;

        std::vector<std::vector<std::string>> valuesGroups;
        std::regex groupPattern(R"(\(([^()]+)\))");
        auto groupBegin = std::sregex_iterator(match[3].str().begin(), match[3].str().end(), groupPattern);
        auto groupEnd = std::sregex_iterator();

        for (auto it = groupBegin; it != groupEnd; ++it) {
            std::string groupValues = it->str(1);
            std::vector<std::string> values;

            std::regex valPattern(R"((?:'[^']*'|[^,]+))");
            auto valBegin = std::sregex_iterator(groupValues.begin(), groupValues.end(), valPattern);
            auto valEnd = std::sregex_iterator();

            for (auto valIt = valBegin; valIt != valEnd; ++valIt) {
                std::string val = valIt->str();
                val.erase(0, val.find_first_not_of(' '));
                val.erase(val.find_last_not_of(' ') + 1);
                values.push_back(val);
            }

            if (columns.size() == values.size()) {
                valuesGroups.push_back(values);
            }
        }

        result["values"] = valuesGroups;
    }

    return result;
}

std::map<std::string,std::any> parseUse(const std::string& sql){
    std::map<std::string, std::any> result = { {"type", "USE"}, {"status", false} };
    std::regex pattern(R"(USE\s+(\w)+;)",ICASE);
    std::smatch match;

    if(std::regex_search(sql,match,pattern)){
        result["status"] = true;
        result["name"] = match[1].str();
    }
    return result;
}

std::map<std::string, std::any> parseSelect(const std::string& sql) {
    std::map<std::string, std::any> result = { {"type", "SELECT"}, {"status", false} };
    std::regex pattern(R"(SELECT\s+(.*?)\s+FROM\s+(\w+)(?:\s+WHERE\s+(.*?))?(?:\s+GROUP\s+BY\s+(.*?))?(?:\s+HAVING\s+(.*?))?(?:\s+ORDER\s+BY\s+(.*?))?(?:\s+LIMIT\s+(.*?))?)", ICASE);
    std::smatch match;

    if (std::regex_search(sql, match, pattern)) {
        result["status"] = true;

        std::regex colRegex(R"(\w+(?:\s+AS\s+\w+)?)", ICASE);
        std::string matchStr = match[1].str();
        auto colBegin = std::sregex_iterator(matchStr.begin(), matchStr.end(), colRegex);
        auto colEnd = std::sregex_iterator();
        std::string columns;
        for (auto it = colBegin; it != colEnd; ++it) {
            columns += it->str() + ",";
        }
        if (!columns.empty()) columns.pop_back();
        result["columns"] = columns;

        result["table"] = match[2];
        if (match[3].matched) result["where"] = match[3];
        if (match[4].matched) result["group_by"] = match[4];
        if (match[5].matched) result["having"] = match[5];
        if (match[6].matched) result["order_by"] = match[6];
        if (match[7].matched) result["limit"] = match[7];
    }
    std::cout << result["status"] << std::endl;
    return result;
}

std::map<std::string, std::any> parseGrant(const std::string& sql) {
    std::map<std::string, std::any> result = { {"type", "GRANT"}, {"status", false} };
    std::regex pattern(R"(GRANT\s+([\w,\s]+)\s+ON\s+(\w+)\s+TO\s+(\w+))", ICASE);
    std::smatch match;
    if (std::regex_search(sql, match, pattern)) {
        result["status"] = "true";
        result["object"] = match[2].str();
        result["user"] = match[3].str();

        std::vector<std::string> rightsList;
        std::regex rightsRegex(R"(\w+)");
        std::string matchStr = match[1].str();
        auto rightsBegin = std::sregex_iterator(matchStr.begin(), matchStr.end(), rightsRegex);
        auto rightsEnd = std::sregex_iterator();
        for (auto it = rightsBegin; it != rightsEnd; ++it) {
            rightsList.push_back(it->str());
        }
        result["rights_list"] = "";
        for (const auto& right : rightsList) {
            result["rights_list"] += (right + ",");
        }
        if (!result["rights_list"].empty()) result["rights_list"].pop_back();

        result["rights"] = rightsList;
    }
    return result;
}

std::map<std::string, std::string> parseRevoke(const std::string& sql) {
    std::map<std::string, std::string> result = { {"type", "REVOKE"}, {"status", "false"} };
    std::regex pattern(R"(REVOKE\s+([\w,\s]+)\s+ON\s+(\w+)\s+FROM\s+(\w+))", ICASE);
    std::smatch match;
    if (std::regex_search(sql, match, pattern)) {
        result["status"] = "true";
        result["rights"] = match[1];
        result["object"] = match[2];
        result["user"] = match[3];
        // 拆分多个权限
        std::vector<std::string> rightsList;
        std::regex rightsRegex(R"(\w+)");
        std::string matchStr = match[1].str();
        auto rightsBegin = std::sregex_iterator(matchStr.begin(), matchStr.end(), rightsRegex);
        auto rightsEnd = std::sregex_iterator();
        for (auto it = rightsBegin; it != rightsEnd; ++it) {
            rightsList.push_back(it->str());
        }
        result["rights_list"] = "";
        for (const auto& right : rightsList) {
            result["rights_list"] += (right + ",");
        }
        if (!result["rights_list"].empty()) result["rights_list"].pop_back();
    }
    return result;
}

std::map<std::string, std::string> parseAlter(const std::string& sql) {
    std::map<std::string, std::string> result = { {"type", "ALTER"}, {"status", "false"} } ;
    std::regex pattern(R"(ALTER\s+TABLE\s+(\w+)\s+(ADD|MODIFY|DROP)\s+(.*))", ICASE);
    std::smatch match;
    if (std::regex_search(sql, match, pattern)) {
        result["status"] = "true";
        result["table"] = match[1];
        result["action"] = match[2];
        std::string columnsStr = match[3];

        std::vector<std::map<std::string, std::string>> columns;
        std::regex columnPattern(R"((\w+)\s+(\w+)\s*([^,]*))");
        auto columnBegin = std::sregex_iterator(columnsStr.begin(), columnsStr.end(), columnPattern);
        auto columnEnd = std::sregex_iterator();
        for (std::sregex_iterator it = columnBegin; it != columnEnd; ++it) {
            std::map<std::string, std::string> column;
            column["name"] = (*it)[1];
            column["type"] = (*it)[2];
            column["constraints"] = (*it)[3];
            columns.push_back(column);
        }
        result["columns"] = serializeColumns(columns);
    }
    return result;
}

std::map<std::string, std::string> parseShow(const std::string& sql) {
    std::map<std::string, std::string> result = { {"type", "SHOW"}, {"status", "false"} };
    std::regex pattern(R"(SHOW\s+(TABLES|DATABASES)(?:\s+FROM\s+(\w+))?)", ICASE);
    std::smatch match;
    if (std::regex_search(sql, match, pattern)) {
        result["status"] = "true";
        result["item"] = match[1];
        if (match[2].matched) result["database"] = match[2];
    }
    return result;
}

std::map<std::string, std::any> parseUpdate(const std::string& sql) {
    std::map<std::string, std::any> result = { {"type", "UPDATE"}, {"status", false} };
    std::regex pattern(R"(UPDATE\s+(\w+)\s+SET\s+(\w+)\s*=\s*(\w+)(?:\s+WHERE\s+(.+))?)", ICASE);
    std::smatch match;
    if (std::regex_search(sql, match, pattern)) {
        result["status"] = true;
        result["table"] = match[1];
        result["column"] = match[2];
        result["value"] = match[3];
        if (match[4].matched) result["condition"] = match[4];
    }
    return result;
}

std::map<std::string, std::any> parseDelete(const std::string& sql) {
    std::map<std::string, std::any> result = { {"type", "DELETE"}, {"status", false} };
    std::regex pattern(R"(DELETE\s+FROM\s+(\w+)(?:\s+WHERE\s+(.+))?)", ICASE);
    std::smatch match;
    if (std::regex_search(sql, match, pattern)) {
        result["status"] = true;
        result["table"] = match[1];
        if (match[2].matched) result["condition"] = match[2];
    }
    return result;
}

std::map<std::string, std::any> parseDescribe(const std::string& sql) {
    std::map<std::string, std::any> result = { {"type", "DESCRIBE"}, {"status", false} };
    std::regex pattern(R"(DESCRIBE\s+(\w+)(?:\s+(\w+))?)", ICASE);
    std::smatch match;
    if (std::regex_search(sql, match, pattern)) {
        result["status"] = true;
        result["table"] = match[1];
        if (match[2].matched) result["column"] = match[2];
    }
    return result;
}

using ParseFunc = std::map<std::string, std::string>(*)(const std::string&);
std::map<std::string, std::string> parseSQL(const std::string& sql) {
    std::vector<std::pair<std::regex, ParseFunc>> patterns = {
        {std::regex(R"(^CREATE\s)", ICASE), parseCreate},
        {std::regex(R"(^INSERT\s)", ICASE), parseInsert},
        {std::regex(R"(^SELECT\s)", ICASE), parseSelect},
        {std::regex(R"(^USE\s)", ICASE), parseUse},
        {std::regex(R"(^DROP\s)", ICASE), parseDrop},
        {std::regex(R"(^GRANT\s)", ICASE), parseGrant},
        {std::regex(R"(^REVOKE\s)", ICASE), parseRevoke},
        {std::regex(R"(^ALTER\s)", ICASE), parseAlter},
        {std::regex(R"(^SHOW\s)", ICASE), parseShow},
        {std::regex(R"(^UPDATE\s)", ICASE), parseUpdate},
        {std::regex(R"(^DELETE\s)", ICASE), parseDelete},
        {std::regex(R"(^DESCRIBE\s)", ICASE), parseDescribe},
    };
    for (const auto& [pattern, func] : patterns) {
        if (std::regex_search(sql, pattern)) {
            return func(sql);
        }
    }
    return { {"type", "UNKNOWN"}, {"status", "false"} };
}

void Lexer::handleRawSQL(QString rawSql){
    std::vector<std::string> sqls=utils::split(rawSql.toStdString(),";");
    for(std::string sql:sqls){
        std::map<std::string,std::string> result = parseSQL(sql);
        std::cout << "Type: " << result["type"] << ", Status: " << result["status"] << std::endl;
    }
}

