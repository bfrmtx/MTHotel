#ifndef SQLITE_HANDLER_H
#define SQLITE_HANDLER_H
#include <cfloat>
#include <climits>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

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
//    static int sqlite_handler_create_callback(void* ptr, int argc, char** argv, char** col_name) {

//    for (int i = 0; i < argc; i++) {
//        printf("%s = %s\n", col_name[i], argv[i] ? argv[i] : "NULL");
//        // condition ? result_if_true : result_if_false
//    }

//    return SQLITE_OK;

//}

/*!
 * \brief sqlite_handler_str_callback
 * \param ptr std::vector<std::vector<std::string>> aka a table of strings
 * \param argc
 * \param argv
 * \param col_name
 * \return
 */
static int sqlite_handler_str_callback(void *ptr, int argc, char **argv, char **col_name) {

  std::vector<std::vector<std::string>> *table = static_cast<std::vector<std::vector<std::string>> *>(ptr);
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
static int sqlite_handler_scolumn_double_callback(void *ptr, int argc, char **argv, char **col_name) {

  std::vector<double> *vec = static_cast<std::vector<double> *>(ptr);

  std::string snum = (argv[0]) ? (argv[0]) : "NULL";
  if (snum == "NULL")
    return SQLITE_ERROR;
  double num = 0;
  try {
    num = std::stod(std::string(argv[0]));

  } catch (std::invalid_argument const &ia) {
    std::cout << "sqlite_handler_scolumn_double_callback: " << ia.what() << std::endl;
    return SQLITE_ERROR;
  } catch (...) {
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
static int sqlite_handler_scolumn_int64_t_callback(void *ptr, int argc, char **argv, char **col_name) {

  std::vector<int64_t> *vec = static_cast<std::vector<int64_t> *>(ptr);
  std::string snum = (argv[0]) ? (argv[0]) : "NULL";
  if (snum == "NULL")
    return SQLITE_ERROR;
  int64_t num = 0;
  try {
    num = std::stoll(std::string(argv[0]));

  } catch (std::invalid_argument const &ia) {
    std::cout << "sqlite_handler_scolumn_int64_t_callback: " << ia.what() << std::endl;
    return SQLITE_ERROR;
  } catch (...) {
    std::cout << "sqlite_handler_scolumn_int64_t_callback: out of range or other error" << std::endl;
    return SQLITE_ERROR;
  }

  vec->push_back(num);
  return SQLITE_OK;
}

/*!
 * \brief sqlite_handler_scolumn_uint64_t_callback
 * \param ptr std::vector<uint64_t> with results only - as we use for indices
 * \param argc
 * \param argv
 * \param col_name
 * \return
 */
static int sqlite_handler_scolumn_uint64_t_callback(void *ptr, int argc, char **argv, char **col_name) {

  std::vector<uint64_t> *vec = static_cast<std::vector<uint64_t> *>(ptr);
  std::string snum = (argv[0]) ? (argv[0]) : "NULL";
  if (snum == "NULL")
    return SQLITE_ERROR;
  uint64_t num = 0;
  try {
    num = std::stoull(std::string(argv[0]));

  } catch (std::invalid_argument const &ia) {
    std::cout << "sqlite_handler_scolumn_uint64_t_callback: " << ia.what() << std::endl;
    return SQLITE_ERROR;
  } catch (...) {
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

class sqlite_handler {
public:
  /*!
   * \brief sqlite_handler constructor
   */
  sqlite_handler(const std::filesystem::path &db_name) {
    this->db_name = db_name;
  };

  /*!
   * \brief sqlite_handler destructor
   */
  ~sqlite_handler() {
    if (this->DB != nullptr)
      sqlite3_close_v2(DB);
  };

  void create_table(const std::string query, const bool close_after_read = true) {

    if (this->open_mode != SQLITE_OPEN_CREATE) {
      this->close();
      this->open(SQLITE_OPEN_CREATE);
    }

    this->exec_query_error(sqlite3_exec(this->DB, query.c_str(), NULL, NULL, NULL));
    if (close_after_read)
      this->close();
  }

  void insert(const std::string query, const bool close_after_read = true) {

    if (this->open_mode != SQLITE_OPEN_READWRITE) {
      this->close();
      this->open(SQLITE_OPEN_READWRITE);
    }

    this->exec_query_error(sqlite3_exec(this->DB, query.c_str(), NULL, NULL, NULL));
    if (close_after_read)
      this->close();
  }

  std::vector<std::vector<std::string>> sqlite_select_strs(const std::string query, const bool close_after_read = true) {

    if (this->open_mode != SQLITE_OPEN_READONLY) {
      this->close();
      this->open(SQLITE_OPEN_READONLY);
    }
    std::vector<std::vector<std::string>> stable;

    this->exec_query_error(sqlite3_exec(this->DB, query.c_str(), sqlite_handler_str_callback, &stable, NULL));

    if (close_after_read)
      this->close();
    return stable;
  }

  std::vector<double> sqlite_vector_double(const std::string query, const bool close_after_read = true) {

    if (this->open_mode != SQLITE_OPEN_READONLY) {
      this->close();
      this->open(SQLITE_OPEN_READONLY);
    }
    std::vector<double> data;

    this->exec_query_error(sqlite3_exec(this->DB, query.c_str(), sqlite_handler_scolumn_double_callback, &data, NULL));
    if (close_after_read)
      this->close();
    return data;
  }

  std::vector<int64_t> sqlite_vector_int64_t(const std::string query, const bool close_after_read = true) {

    if (this->open_mode != SQLITE_OPEN_READONLY) {
      this->close();
      this->open(SQLITE_OPEN_READONLY);
    }
    std::vector<int64_t> data;

    this->exec_query_error(sqlite3_exec(this->DB, query.c_str(), sqlite_handler_scolumn_int64_t_callback, &data, NULL));
    if (close_after_read)
      this->close();
    return data;
  }

  std::vector<uint64_t> sqlite_vector_uint64_t(const std::string query, const bool close_after_read = true) {

    if (this->open_mode != SQLITE_OPEN_READONLY) {
      this->close();
      this->open(SQLITE_OPEN_READONLY);
    }
    std::vector<uint64_t> data;

    this->exec_query_error(sqlite3_exec(this->DB, query.c_str(), sqlite_handler_scolumn_uint64_t_callback, &data, NULL));
    if (close_after_read)
      this->close();
    return data;
  }

  void show_table(const std::vector<std::vector<std::string>> &table) {

    for (const auto &srow : table) {
      for (const auto &scol : srow) {
        std::cout << std::setw(3) << scol << " ";
      }
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }

  void close() {
    if (this->DB != nullptr)
      sqlite3_close_v2(DB);
    this->open_mode = -1;
    this->exit = SQLITE_ERROR;
  }

  void exec_query_error(const int &rc) {
    if (rc != SQLITE_OK) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::Error SELECT / INSERT -> " << this->db_name;
      if (this->DB != nullptr)
        err_str << " " << std::string(sqlite3_errmsg(this->DB));
      this->close();
      throw std::runtime_error(err_str.str());
    }
  }

  void open(const int &open_mode) {

    this->open_mode = open_mode;
    if (this->open_mode == SQLITE_OPEN_READONLY) {
      if (!std::filesystem::exists(this->db_name)) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << ":: DB RO file not found -> " << std::filesystem::absolute(db_name);
        throw std::runtime_error(err_str.str());
      }
      this->exit = sqlite3_open_v2(this->db_name.string().c_str(), &this->DB, SQLITE_OPEN_READONLY, NULL);
      if (this->exit) {
        std::cerr << "Error open DB RO" << this->db_name << " " << sqlite3_errmsg(this->DB) << std::endl;
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << ":: " << this->db_name;
        if (this->DB != nullptr)
          err_str << " " << std::string(sqlite3_errmsg(this->DB));
        this->close();
        throw std::runtime_error(err_str.str());
      }
    }
    if (this->open_mode == SQLITE_OPEN_READWRITE) {
      if (!std::filesystem::exists(this->db_name)) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << ":: DB RW file not found ->" << std::filesystem::absolute(db_name);
        throw std::runtime_error(err_str.str());
      }
      this->exit = sqlite3_open_v2(this->db_name.string().c_str(), &this->DB, SQLITE_OPEN_READWRITE, NULL);
      if (this->exit) {
        std::cerr << "Error open DB RW" << this->db_name << " " << sqlite3_errmsg(this->DB) << std::endl;
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << " " << this->db_name;
        if (this->DB != nullptr)
          err_str << " " << std::string(sqlite3_errmsg(this->DB));
        this->close();
        throw std::runtime_error(err_str.str());
      }
    }

    if (this->open_mode == SQLITE_OPEN_CREATE) {

      this->exit = sqlite3_open_v2(this->db_name.string().c_str(), &this->DB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
      if (this->exit) {
        std::cerr << "Error open DB / Create " << this->db_name << " " << sqlite3_errmsg(this->DB) << std::endl;
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << " " << this->db_name.string();
        if (this->DB != nullptr)
          err_str << " " << std::string(sqlite3_errmsg(this->DB));
        this->close();
        throw std::runtime_error(err_str.str());
      }
    }
  }

  void vacuum() {
    sqlite3_exec(this->DB, "VACUUM", 0, 0, 0);
  }

  sqlite3 *DB = nullptr;         //!< the database
  int exit = SQLITE_ERROR;       //!< sqlite return open value
  int open_mode = -1;            //!< SQLITE_OPEN_READONLY 1, SQLITE_OPEN_READWRITE 2, SQLITE_OPEN_CREATE 4
  std::filesystem::path db_name; //!< database file
};

#endif // SQLITE_HANDLER_H
