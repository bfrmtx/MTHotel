#ifndef SQLITE_HANDLER_H
#define SQLITE_HANDLER_H
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <cfloat>
#include <iomanip>
#include <climits>

#include <sqlite3.h>



/**
  * @file sqlite_handler.h
  * contains a most simple C++ interface without comfort - but does work with vectors
  *
*/
/*

int sqlite3_exec(
    sqlite3*,                                  // An open database
    const char *sql,                           // SQL to be evaluated
    int (*callback)(void*,int,char**,char**),  // Callback function
    void *,                                    // 1st argument to callback
    char **errmsg                              // Error msg written here
    );

*/


// does nothing - a prototype (not used)
    static int sqlite_handler_create_callback(void* ptr, int argc, char** argv, char** col_name) {

    for (int i = 0; i < argc; i++) {
        printf("%s = %s\n", col_name[i], argv[i] ? argv[i] : "NULL");
        // condition ? result_if_true : result_if_false
    }

    return SQLITE_OK;

}

    /*!
 * \brief sqlite_handler_str_callback
 * \param ptr std::vector<std::vector<std::string>> aka a table of strings
 * \param argc
 * \param argv
 * \param col_name
 * \return
 */
static int sqlite_handler_str_callback(void* ptr, int argc, char** argv, char** col_name) {

    std::vector<std::vector<std::string>>* table = static_cast<std::vector<std::vector<std::string>>*>(ptr);
    std::vector<std::string> row;
    row.reserve(size_t(argc));
    for (int i = 0; i < argc; i++) {
        // printf("%s = %s\n", col_name[i], argv[i] ? argv[i] : "NULL");
        // that is: condition ? result_if_true : result_if_false
        row.push_back(argv[i] ? argv[i] : "NULL");
    }
    table->push_back(row);
    return SQLITE_OK;

}

/*!
 * \brief sqlite_handler_scolumn_double_callback
 * \param ptr std::vector<double> with results only - as we use for frequencies
 * \param argc
 * \param argv
 * \param col_name
 * \return
 */
static int sqlite_handler_scolumn_double_callback(void* ptr, int argc, char** argv, char** col_name) {

    std::vector<double>* vec = static_cast<std::vector<double>*>(ptr);

    std::string snum = (argv[0]) ? (argv[0]) : "NULL";
    if (snum == "NULL") return SQLITE_ERROR;
    double num = 0;
    try {
        num = std::stod(std::string(argv[0]));

    } catch (std::invalid_argument const& ia) {
        std::cout << "sqlite_handler_scolumn_double_callback: " << ia.what() << std::endl;
        return SQLITE_ERROR;
    }
    catch (...) {
        std::cout << "sqlite_handler_scolumn_double_callback: out of range or other error" << std::endl;
        return SQLITE_ERROR;
    }

    vec->push_back(num);
    return SQLITE_OK;

}

/*!
 * \brief sqlite_handler_scolumn_int64_t_callback
 * \param ptr std::vector<int64_t> with results only - as we use for what?
 * \param argc
 * \param argv
 * \param col_name
 * \return
 */
static int sqlite_handler_scolumn_int64_t_callback(void* ptr, int argc, char** argv, char** col_name) {

    std::vector<std::int64_t>* vec = static_cast<std::vector<std::int64_t>*>(ptr);
    std::string snum = (argv[0]) ? (argv[0]) : "NULL";
    if (snum == "NULL") return SQLITE_ERROR;
    std::int64_t num = 0;
    try {
        num = std::stoll(std::string(argv[0]));

    }
    catch (std::invalid_argument const& ia) {
        std::cout << "sqlite_handler_scolumn_int64_t_callback: " << ia.what() << std::endl;
        return SQLITE_ERROR;
    }
    catch (...) {
        std::cout << "sqlite_handler_scolumn_int64_t_callback: out of range or other error" << std::endl;
        return SQLITE_ERROR;
    }

    vec->push_back(num);
    return SQLITE_OK;

}

/*!
 * \brief sqlite_handler_scolumn_uint64_t_callback
 * \param ptr std::vector<std::uint64_t> with results only - as we use for indices
 * \param argc
 * \param argv
 * \param col_name
 * \return
 */
static int sqlite_handler_scolumn_uint64_t_callback(void* ptr, int argc, char** argv, char** col_name) {

    std::vector<std::uint64_t>* vec = static_cast<std::vector<std::uint64_t>*>(ptr);
    std::string snum = (argv[0]) ? (argv[0]) : "NULL";
    if (snum == "NULL") return SQLITE_ERROR;
    std::uint64_t num = 0;
    try {
        num = std::stoull(std::string(argv[0]));

    } catch (std::invalid_argument const& ia) {
        std::cout << "sqlite_handler_scolumn_uint64_t_callback: " << ia.what() << std::endl;
        return SQLITE_ERROR;
    }
    catch (...) {
        std::cout << "sqlite_handler_scolumn_uint64_t_callback: out of range or other error" << std::endl;
        return SQLITE_ERROR;
    }

    vec->push_back(num);
    return SQLITE_OK;

}

// sqlite3_open_v2("database.db", &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL );
// sqlite3_open_v2("database.db", &db, SQLITE_OPEN_READWRITE, NULL );
// sqlite3_open_v2("database.db", &db, SQLITE_OPEN_READONLY, NULL );


/*!
 * \brief The sqlite_handler class - trivial C++ interface
 */

class sqlite_handler
{
public:

    /*!
     * \brief sqlite_handler empty contructor
     */
    sqlite_handler() {;};

    /*!
     * \brief sqlite_handler empty destructor, no members, nothing to do
     */
    ~sqlite_handler() {;};


    void create_table(const std::filesystem::path &db_name, const std::string query) {
        sqlite3* DB = nullptr;
        int exit = 0;
        exit = sqlite3_open_v2(db_name.string().c_str(), &DB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
        if (exit) {
            std::cerr << "Error open DB for CREATE TABLE " << db_name << " " << sqlite3_errmsg(DB) << std::endl;
            std::string err_str = __func__;
            err_str += std::string(sqlite3_errmsg(DB));
            if (DB != nullptr) sqlite3_close(DB);
            throw err_str;
        }

        int rc = sqlite3_exec(DB, query.c_str(), NULL, NULL, NULL);
        if (rc != SQLITE_OK) {
            sqlite3_close(DB);
            std::string err_str = __func__;
            err_str += "::Error CREATE ->";
            err_str += std::filesystem::absolute(db_name).string();
            throw err_str;
        }

        sqlite3_close(DB);
        if (!std::filesystem::exists(db_name)) {
            std::string err_str = __func__;
            err_str += ":: file not CREATED? -> ";
            err_str += std::filesystem::absolute(db_name).string();
            throw err_str;
        }

    }

    std::vector<std::vector<std::string>> sqlite_select_strs(const std::filesystem::path &db_name, const std::string query) {

        if (!std::filesystem::exists(db_name)) {
            std::string err_str = __func__;
            err_str += ":: file not found -> ";
            err_str += std::filesystem::absolute(db_name).string();
            throw err_str;
        }

        sqlite3* DB = nullptr;
        int exit = 0;
        exit = sqlite3_open_v2(db_name.string().c_str(), &DB, SQLITE_OPEN_READONLY, NULL);
        if (exit) {
            std::cerr << "Error open DB " << db_name << " " << sqlite3_errmsg(DB) << std::endl;
            std::string err_str = __func__;
            err_str += std::string(sqlite3_errmsg(DB));
            if (DB != nullptr) sqlite3_close(DB);
            throw err_str;
        }
        std::vector<std::vector<std::string>> stable;

        int rc = sqlite3_exec(DB, query.c_str(), sqlite_handler_str_callback, &stable, NULL);
        if (rc != SQLITE_OK) {
            sqlite3_close(DB);
            std::string err_str = __func__;
            err_str += "::Error SELECT -> ";
            err_str += std::filesystem::absolute(db_name).string();
            throw err_str;
        }
        sqlite3_close(DB);

        return stable;
    }

    std::vector<double> sqlite_vector_double(const std::filesystem::path &db_name, const std::string query) {

        if (!std::filesystem::exists(db_name)) {
            std::string err_str = __func__;
            err_str += ":: file not found -> ";
            err_str += std::filesystem::absolute(db_name).string();
            throw err_str;
        }


        sqlite3* DB = nullptr;
        int exit = 0;
        exit = sqlite3_open_v2(db_name.string().c_str(), &DB, SQLITE_OPEN_READONLY, NULL);
        if (exit) {
            std::cerr << "Error open DB " << db_name << " " << sqlite3_errmsg(DB) << std::endl;
            std::string err_str = __func__;
            err_str += std::string(sqlite3_errmsg(DB));
            if (DB != nullptr) sqlite3_close(DB);
            throw err_str;
        }
        std::vector<double> data;

        int rc = sqlite3_exec(DB, query.c_str(), sqlite_handler_scolumn_double_callback, &data, NULL);
        if (rc != SQLITE_OK) {
            sqlite3_close(DB);
            std::string err_str = __func__;
            err_str += "::Error SELECT ->";
            err_str += std::filesystem::absolute(db_name).string();
            throw err_str;
        }
        sqlite3_close(DB);
        return data;
    }

    std::vector<int64_t> sqlite_vector_int64_t(const std::filesystem::path &db_name, const std::string query) {

        if (!std::filesystem::exists(db_name)) {
            std::string err_str = __func__;
            err_str += ":: file not found ->";
            err_str += std::filesystem::absolute(db_name).string();
            throw err_str;
        }

        sqlite3* DB = nullptr;
        int exit = 0;
        exit = sqlite3_open_v2(db_name.string().c_str(), &DB, SQLITE_OPEN_READONLY, NULL);
        if (exit) {
            std::cerr << "Error open DB " << db_name << " " << sqlite3_errmsg(DB) << std::endl;
            std::string err_str = __func__;
            err_str += std::string(sqlite3_errmsg(DB));
            if (DB != nullptr) sqlite3_close(DB);
            throw err_str;
        }
        std::vector<int64_t> data;

        int rc = sqlite3_exec(DB, query.c_str(), sqlite_handler_scolumn_int64_t_callback, &data, NULL);
        if (rc != SQLITE_OK) {
            sqlite3_close(DB);
            std::string err_str = __func__;
            err_str += "::Error SELECT ->";
            err_str += std::filesystem::absolute(db_name).string();
            throw err_str;
        }
        sqlite3_close(DB);
        return data;
    }

    std::vector<uint64_t> sqlite_vector_uint64_t(const std::filesystem::path &db_name, const std::string query) {

        if (!std::filesystem::exists(db_name)) {
            std::string err_str = __func__;
            err_str += ":: file not found ->";
            err_str += std::filesystem::absolute(db_name).string();
            throw err_str;
        }

        sqlite3* DB = nullptr;
        int exit = 0;
        exit = sqlite3_open_v2(db_name.string().c_str(), &DB, SQLITE_OPEN_READONLY, NULL);
        if (exit) {
            std::cerr << "Error open DB " << db_name << " " << sqlite3_errmsg(DB) << std::endl;
            std::string err_str = __func__;
            err_str += std::string(sqlite3_errmsg(DB));
            if (DB != nullptr) sqlite3_close(DB);
            throw err_str;
        }
        std::vector<uint64_t> data;

        int rc = sqlite3_exec(DB, query.c_str(), sqlite_handler_scolumn_uint64_t_callback, &data, NULL);
        if (rc != SQLITE_OK) {
            sqlite3_close(DB);
            std::string err_str = __func__;
            err_str += "::Error SELECT ->";
            err_str += std::filesystem::absolute(db_name).string();
            throw err_str;
        }
        sqlite3_close(DB);
        return data;
    }


    void show_table(const std::vector<std::vector<std::string>> &table) {

        for (const auto &srow: table) {
            for (const auto &scol: srow) {
                std::cout << std::setw(3) << scol << " ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;

    }




};


#endif // SQLITE_HANDLER_H
