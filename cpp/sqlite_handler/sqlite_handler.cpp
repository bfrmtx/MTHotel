#include "sqlite_handler.h"

static int callback(void *ptr, int argc, char **argv, char **col_name)
{
    {

        //fprintf(stderr, "%s: ", (const char*)data);
        std::vector<std::vector<std::string>>* table = static_cast<std::vector<std::vector<std::string>>*>(ptr);
        std::vector<std::string> header;
        std::vector<std::string> row;
        if (!table->size()) for (int i = 0; i < argc; i++) header.push_back(col_name[i]);
        for (int i = 0; i < argc; i++) {
            // printf("%s = %s\n", col_name[i], argv[i] ? argv[i] : "NULL");
            row.push_back(argv[i] ? argv[i] : "NULL");
        }
        // if (!table->size()) table->push_back(header);
        table->push_back(row);
        // printf("\n");
        return 0;
    }
}

sqlite_handler::sqlite_handler()
{

}

size_t sqlite_handler::sqlite_select(const std::filesystem::path &db_name, const std::string query)
{
    sqlite3* DB;
    int exit = 0;
    exit = sqlite3_open(db_name.string().c_str(), &DB);
    if (exit) {
        std::cerr << "Error open DB " << db_name << " " << sqlite3_errmsg(DB) << std::endl;
        return 0;
    }
    std::vector<std::vector<std::string>> stable;
    int rc = sqlite3_exec(DB, query.c_str(), callback, &stable, NULL);
    if (rc != SQLITE_OK)  std::cerr << "Error SELECT" << std::endl;
    sqlite3_close(DB);

    size_t i = 0;
    this->table.clear();
    this->header.clear();

    for (const auto &row : stable) {

     this->table.push_back(row);
    }

    return this->table.size();
}

void sqlite_handler::show_table() const
{
    if (this->table.size()) {
        for (const auto &row : this->table) {
            for (auto &col : row) {
                std::cout << col << " ";
            }
            std::cout << std::endl;
        }
    }
    else std::cout << "table is empty!" <<  std::endl;
}

void sqlite_handler::show_header() const
{
    if (this->header.size()) {
        for (const auto &col : this->header) {
            std::cout << col << " ";
        }
        std::cout << std::endl;
    }
    else std::cout << "header is empty!" <<  std::endl;
}

void sqlite_handler::clear()
{
    this->header.clear();
    this->table.clear();
}


