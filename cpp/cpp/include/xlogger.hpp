#ifndef XLOGGER_HPP
#define XLOGGER_HPP

#include <chrono>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>

/// @brief xlogger implements a two-stage logging mechanism
/// - buffer: accumulates new messages since last print_last()
/// - store: permanent archive of all logged messages
/// - overload the << operator to log messages
/// - thread safe
/// - echo to console immediately
/// - print_last() displays buffer and moves it to store
class xlogger {
private:
  std::mutex log_mutex;
  std::deque<std::string> buffer; // new messages pending display
  std::deque<std::string> store;  // full message history

public:
  xlogger() = default;
  ~xlogger() = default;

  // operator << overload
  template <typename T>
  xlogger &operator<<(const T &msg) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::ostringstream out;
    out << msg;
    std::string log_entry = out.str();
    // add to buffer
    buffer.push_back(log_entry);
    // echo to console
    std::cout << log_entry << std::flush;
    return *this;
  }

  /// @brief clear both buffer and store
  void clear() {
    std::lock_guard<std::mutex> lock(log_mutex);
    buffer.clear();
    store.clear();
  }

  /// @brief print all new messages in buffer, then move buffer to store
  void print_last() {
    std::lock_guard<std::mutex> lock(log_mutex);
    if (buffer.empty()) {
      return;
    }
    // print buffer contents
    for (const auto &msg : buffer) {
      std::cout << msg << std::flush;
    }
    // move buffer to store (efficient bulk move)
    store.insert(store.end(), std::make_move_iterator(buffer.begin()), std::make_move_iterator(buffer.end()));
    buffer.clear();
  }

  /// @brief move all messages (buffer + store) to another logger
  void pop(xlogger &other) {
    std::lock_guard<std::mutex> lock(log_mutex);
    // Move store messages
    for (const auto &msg : store) {
      other << msg;
    }
    // Move buffer messages
    for (const auto &msg : buffer) {
      other << msg;
    }
    buffer.clear();
    store.clear();
  }
};
#endif // XLOGGER_HPP
