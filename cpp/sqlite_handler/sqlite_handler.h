#ifndef SQLITE_HANDLER_H
#define SQLITE_HANDLER_H
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <filesystem>

#include <sqlite3.h>




class sqlite_handler
{
public:
    sqlite_handler();

    std::vector<std::vector<std::string>> sqlite_select(const std::filesystem::path &db_name, const std::string query);

    std::vector<std::string> header;
    std::vector<std::vector<std::string>> table;

    void show_table() const;
    void show_header() const;

    void clear();
private:

    static int callback(void* ptr, int argc, char** argv, char** col_name);


};

#endif // SQLITE_HANDLER_H
