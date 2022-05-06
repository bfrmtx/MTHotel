#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <sqlite3.h>

using namespace std;

static int callback(void* ptr, int argc, char** argv, char** col_name)
{

    //fprintf(stderr, "%s: ", (const char*)data);
    vector<vector<string>>* table = static_cast<vector<vector<string>>*>(ptr);
    vector<string> header;
    vector<string> row;
    if (!table->size()) for (int i = 0; i < argc; i++) header.push_back(col_name[i]);
    for (int i = 0; i < argc; i++) {
        // printf("%s = %s\n", col_name[i], argv[i] ? argv[i] : "NULL");
        row.push_back(argv[i] ? argv[i] : "NULL");
    }
    if (!table->size()) table->push_back(header);
    table->push_back(row);
    // printf("\n");
    return 0;
}

int main(int argc, char** argv)
{
    sqlite3* DB;
    int exit = 0;
    exit = sqlite3_open("info.sql3", &DB);

    string sql_query("SELECT * FROM sensortypes;");
    if (exit) {
        std::cerr << "Error open DB " << sqlite3_errmsg(DB) << std::endl;
        return (-1);
    }
    else
        std::cout << "Opened Database Successfully!" << std::endl;
    vector<vector<string>> table;
    int rc = sqlite3_exec(DB, sql_query.c_str(), callback, &table, NULL);

    if (rc != SQLITE_OK)
        cerr << "Error SELECT" << endl;
    else {
        cout << "Operation OK!" << endl;
    }
    for (auto &row : table) {
        for (auto &col : row) {
            cout << col << " ";
        }
        cout << std::endl;
    }
    sqlite3_close(DB);

    return (0);
}
