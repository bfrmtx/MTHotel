#ifndef SQLITE_STATUS_THREAD_H
#define SQLITE_STATUS_THREAD_H

#include "messages.h"
#include "sqlite_handler.h"
#include <algorithm>
#include <atomic>
#include <cfloat>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <shared_mutex>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <utility>

#include "messages.h"

/*!
 * \brief The sqlite_receiver class running in a thread like  std::jthread xd(&sqlite_receiver::run, logger.get());
 */

// forward declaration because the receiver will create the sender
class sqlite_sender;

// make a template out of this for T message queue

class sqlite_receiver {
public:
  sqlite_receiver(std::filesystem::path sqlfile_status, std::filesystem::path sqlfile_log) : sqlfile_status(sqlfile_status), sqlfile_log(sqlfile_log) {

    this->mtx = new std::mutex();
    this->act = new std::atomic<int>(0); // thread can not run yet
    this->cond = new std::condition_variable();
    // create a message queue where senders can push messages to
    // this->message_queue = std::make_shared < std::queue < message_to_sqlite >>> ();
    // this->message_queue = std::make_shared<std::queue<message_to_sqlite>>();
    // write to two different databases; status has constant size, log is appended
    // status table has to be created by the sender; log may be continued
    std::filesystem::remove(this->sqlfile_status);
    this->sql_log = std::make_unique<sqlite_handler>(this->sqlfile_log);
    this->sql_stat = std::make_unique<sqlite_handler>(this->sqlfile_status);
    // queue to share with multiple senders
    this->message_queue = std::make_shared<std::queue<msg_to_sqlite>>();
  }

  void reset_db() {
    if (this->sql_log != nullptr)
      this->sql_log.reset();
    if (this->sql_stat != nullptr)
      this->sql_stat.reset();
  }

  std::unique_ptr<sqlite_sender> create_status_sender(const std::string &sender_name, const std::filesystem::path &json_file) {

    ++(*this->act);
    // create a sender and let it create the table
    // we have a forward declaration of sqlite_sender - all we can do is construct it
    auto multi = multiple_msg_to_sqlite(sender_name, json_file);
    // create the status in case it does not exist
    this->sql_stat->create_table(multi.create_status_table(sender_name));
    this->sql_log->create_table(multi.create_log_table("logs"));

    if (*this->act == 1)
      this->run();

    // we have a forward declaration of sqlite_sender - all we can do is construct it
    return std::make_unique<sqlite_sender>(this, sender_name, json_file);
  }

  ~sqlite_receiver() {
    // stop the thread
    if (tr != nullptr) {
      tr->join();
      delete tr;
    }
    delete this->mtx;
    delete this->act;
    delete this->cond;
    this->message_queue.reset();
    if (this->sql_log != nullptr) {
      this->sql_log->vacuum();
      this->sql_log->close();
      this->sql_log.reset();
      this->sql_stat->vacuum();
      this->sql_stat->close();
      this->sql_stat.reset();
    }

    std::cout << counter << " messages caught" << std::endl;
  }

  /*!
   * @brief starts the thread; e.g. you start the receiver thread first and then the sender threads
   */
  void run() {
    tr = new std::jthread([this]() { sub(); });
  }

  mutable std::mutex *mtx;
  std::atomic<int> *act;
  std::condition_variable *cond;
  std::shared_ptr<std::queue<msg_to_sqlite>> message_queue;

private:
  std::string sql_query;
  std::unique_ptr<sqlite_handler> sql_log;
  std::unique_ptr<sqlite_handler> sql_stat;
  std::filesystem::path sqlfile_status, sqlfile_log;
  uint64_t counter = 0;
  std::jthread *tr = nullptr;

  /*!
   * \brief sub is the thread worker started via run()
   */
  void sub() {
    while (true) {
      std::unique_lock lock(*this->mtx);
      cond->wait(lock, [this] { return !(*this->act) || !this->message_queue->empty(); }); // spurious wakeup protection, i.e:
      while (!message_queue->empty()) {

        try {
          if (this->message_queue->front().is_log_only() == 0) {
            this->sql_stat->insert(this->message_queue->front().update_table_status(), true);
          }
          this->sql_log->insert(this->message_queue->front().insert_into_table_log("logs"), true);

        } catch (const std::runtime_error &error) {
          std::cerr << error.what() << std::endl;
          std::cerr << this->message_queue->front().insert_into_table_log("logs") << std::endl;
          std::cerr << this->message_queue->front().update_table_status() << std::endl;
        }
        ++this->counter;
        message_queue->pop();
      }
      if (*this->act == 0) { // ends when all senders are finished
        break;
      }
    }
  }
};

// ************************************************************* S T A T U S or M E S S A G E *****************************************************************

/*!
 * @brief class for status messages to SQLite database sender_name is also table; thin wrapper around sender class
 */
class sqlite_sender : public multiple_msg_to_sqlite {
public:
  // remember: let the receiver create the multiple_msg_to_sqlite!! We need a valid status_index!!
  sqlite_sender(const sqlite_receiver *msc, const std::string &sender_name, const std::vector<std::string> &keys) : mtx(msc->mtx), act(msc->act), cond(msc->cond), multiple_msg_to_sqlite(sender_name, keys) {

    this->message_queue = msc->message_queue; // shared pointer to message queue
  }

  sqlite_sender(const sqlite_receiver *msc, const std::string &sender_name, const std::filesystem::path &json_file) : mtx(msc->mtx), act(msc->act), cond(msc->cond), multiple_msg_to_sqlite(sender_name, json_file) {

    this->message_queue = msc->message_queue; // shared pointer to message queue
  }

  // use in try block please
  void set_key_value(const std::string &key, const auto &T, const int &severity = 0) {
    // call base class
    multiple_msg_to_sqlite::set_value(key, T, severity); // can  (shall) throw an exception
    std::lock_guard lock(*mtx);
    message_queue->emplace(this->get(key));
  }

  // use in try block please
  void log_only_message(const std::string &key, const auto &T, const int &severity = 0, const int &ref_idx = 0) {
    // call base class
    std::lock_guard lock(*mtx);
    message_queue->emplace(multiple_msg_to_sqlite::log_only_message(key, T, severity, ref_idx));
  }

  void watchdog() {
    std::cout << this->get_sender_name() << " watchdog started" << std::endl;
    this->threads.emplace_back(new std::jthread([this]() { sub_watcher(); }));
  }

  void sub_watcher() {
    bool clear = false;
    std::string key = "watchdog " + this->get_sender_name();
    std::ostringstream value;
    while (true) {
      std::this_thread::sleep_for(std::chrono::milliseconds(2000));
      // compare lists thread  vs thread_ids_faulty

      for (auto &faulty : this->thread_ids_faulty) {
        std::cout << "faulty thread " << faulty << std::endl;
        std::cout << "thread " << faulty << " not found in list" << std::endl;
        clear = true;
        // join thread
        for (auto &tr : this->threads) {
          if (tr->get_id() == faulty) {
            std::cout << "terminating thread " << this->get_sender_name() << " " << tr->get_id() << std::endl;
            value << "thread " << faulty << " terminated;";
            if (tr != nullptr) {
              // lock local
              std::lock_guard<std::mutex> lock(this->mll);
              if (tr->joinable())
                tr->join(); // calls also the destructor
              // tr.reset(); can not be called ... WHY?
              // it can also not be removed from the list ... WHY?
            }
          }
        }
        // remove from list
        // this->thread_ids_faulty.remove(faulty);
      }
      if (clear) {
        this->thread_ids_faulty.clear();
        clear = false;
        this->log_only_message(key, value.str(), 1);
      }
      if (!this->running_thread)
        break;
    }
  }

  /*!
   * @brief when finished, wake up others in case they are waiting
   */
  ~sqlite_sender() {
    --(*this->act);
    cond->notify_all();
    this->running_thread = false;
    // stop the threads
    for (auto &tr : threads) {
      if (tr != nullptr) {
        // if tr is joinable
        if (tr->joinable()) {
          std::cout << "terminating thread " << this->get_sender_name() << " " << tr->get_id() << std::endl;
          tr->join();
        } else {
          std::cout << "thread " << tr->get_id() << " not joinable - or terminated" << std::endl;
        }
      }
    }
    // the list will be destroyed automatically, and therefore the unique pointers will be reset
  }

  /*!
   * @brief simulate a thread that is changing the value of a key - double version
   * @param key
   * @param lower_bound
   * @param upper_bound
   * @param sleep_ms
   */
  void run_double(const std::string key, const double &lower_bound, const double &upper_bound, const int &sleep_ms) {

    threads.emplace_back(std::make_unique<std::jthread>([this, key, upper_bound, lower_bound, sleep_ms]() { sub_T(key, lower_bound, upper_bound, sleep_ms); }));
    // tr = new std::jthread([this]() { sub_double(key); });
  }

  void run_int(const std::string key, const int &lower_bound, const int &upper_bound, const int &sleep_ms) {

    // capture string key
    threads.emplace_back(std::make_unique<std::jthread>([this, key, upper_bound, lower_bound, sleep_ms]() { sub_T(key, lower_bound, upper_bound, sleep_ms); }));
    // tr = new std::jthread([this]() { sub_double(key); });
  }

  template <typename T>
  void sub_T(const std::string key, const T &lower_bound, const T &upper_bound, const int &sleep_ms) {
    try {

      std::unique_ptr<std::uniform_int_distribution<int>> disti;
      std::unique_ptr<std::uniform_real_distribution<double>> distd;

      // compare if T is double type
      if (std::is_same<T, double>::value) {
        distd = std::make_unique<std::uniform_real_distribution<double>>(lower_bound, upper_bound);
      } else if (std::is_same<T, int>::value) {
        disti = std::make_unique<std::uniform_int_distribution<int>>(lower_bound, upper_bound);
      }

      // in case we fail - throw an exception
      if ((disti == nullptr) && (distd == nullptr)) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << " thread nullptpr exception in thread " << std::this_thread::get_id() << " for key " << key << " lower " << lower_bound << " upper " << upper_bound << " sleep " << sleep_ms << " ms";
        throw std::runtime_error(err_str.str());
      }

      // in case we not fail - set the running flag
      this->running_thread = true;
      std::default_random_engine re;
      T random;
      while (true) {
        if (distd != nullptr) {
          random = distd->operator()(re);
        } else if (disti != nullptr) {
          random = disti->operator()(re);
        }
        std::cout << this->get_sender_name() << "::" << key << " " << random << std::endl;
        // scope lock mutex local
        {
          std::lock_guard<std::mutex> lock(this->mll);
          this->set_key_value(key, random);
        }
        if (!this->running_thread) // used in destructor
          break;
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
      }
    }
    // end of try block
    catch (const std::runtime_error &e) {
      std::cerr << e.what() << '\n';
      std::cerr << "create / run thread error in thread " << std::this_thread::get_id() << std::endl;
      { // scope lock
        std::lock_guard<std::mutex> lock(this->mll);
        this->thread_ids_faulty.emplace_back(std::this_thread::get_id()); // mark the thread as faulty
        // this makes only sense if we have a watchdog thread, where we want to join the faulty thread
      }
    }
  }

private:
  mutable std::mutex *mtx;
  std::atomic<int> *act;
  std::condition_variable *cond;
  std::shared_ptr<std::queue<msg_to_sqlite>> message_queue;
  std::list<std::unique_ptr<std::jthread>> threads; // for a simulation we can make a thread where values are changing
  std::mutex mll;                                   // mutex lock local

  std::atomic<bool> running_thread = false;

  std::atomic<size_t> tid = SIZE_MAX;

  std::list<std::thread::id> thread_ids_faulty;
};

#endif // SQLITE_STATUS_THREAD_H