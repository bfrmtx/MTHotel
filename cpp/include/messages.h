#ifndef MESSAGES_H
#define MESSAGES_H

#include "json.h"
#include <atomic>
#include <climits>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "json.h"
#include "json_text.h"
#include "strings_etc.h"

using jsn = nlohmann::ordered_json;

/*!
 * @brief message_to_sqlite is the class for messages adds SQL commands to the JSON message;
 * @details it is like a  R O W  in a  T A B L E  of a database; the json data is  P R I V A T E  and can not be changed from outside: when running the rows of a table are fixed
 */

class msg_to_sqlite {
public:
  msg_to_sqlite(const int &idx = 0) {
    msg["idx"] = idx;           //!< index of the message; needed for an update statement; incremented by receiver class act value
    msg["date"] = "1970-01-01"; //!< date as string, date of message
    msg["time"] = "00:00:00";   //!< time as string, time of message
    msg["ref_idx"] = 0;         //!< reference to an index for a long explanation of the message inside a database of detailed explanations
    msg["sender"] = "";         //!< string id of the sender like ch0, ch2, Batt_1, GPS, ADC, ADU and so on; so the SW component for the HW
    msg["key"] = "";            //!< like "battery_voltage" or "satellites" - YOU NEED TO DEFINE A KEY VECTOR FOR EACH SENDER - OTHERWISE THE STATUS TABLE CAN NOT BE UPDATED
    msg["value"] = "";          //!< like "12.3" or "9"
    msg["severity"] = 0;        //!< 0 = info ... warning = 1 ... error = 2
    msg["log_only"] = 0;        //!< true gives you the opportunity to log a message without updating the status table and be more verbose in the log table, I do not write this to the database
  }
  msg_to_sqlite(const int &idx, const std::string &date, const std::string &time, const int &ref_idx, const std::string &sender, const std::string &key, const std::string &value, const int severity = 0, const int log_only = 0) {
    msg["idx"] = idx;
    msg["date"] = date;
    msg["time"] = time;
    msg["ref_idx"] = ref_idx;
    msg["sender"] = sender;
    msg["key"] = key;
    msg["value"] = value;
    msg["severity"] = severity;
    msg["log_only"] = log_only;
  }

  msg_to_sqlite(const msg_to_sqlite &msg) {
    this->msg = msg.msg;
  }

  msg_to_sqlite(const std::unique_ptr<msg_to_sqlite> msg) {
    this->msg = msg->msg;
  }

  std::string key() const {
    return msg["key"].get<std::string>();
  }

  std::string value() const {
    return msg["value"].get<std::string>();
  }

  int is_log_only() const {
    return msg["log_only"];
  }

  int ivalue() const {
    return msg["value"].get<int>();
  }

  double dvalue() const {
    return msg["value"].get<double>();
  }

  int idx() const {
    return msg["idx"].get<int>();
  }

  jsn get_msg() const {
    return msg;
  }

  std::string create_table_status(const std::string &name) const {
    std::ostringstream sst;
    sst << "CREATE TABLE IF NOT EXISTS `" << name << "` (`idx` INTEGER NOT NULL UNIQUE, `ref_idx` INTEGER NOT NULL,";
    for (auto &it : msg.items()) {
      if (std::find(exclude_keys_create.begin(), exclude_keys_create.end(), it.key()) == exclude_keys_create.end()) {
        sst << "`" << it.key() << "` TEXT,";
      }
    }
    sst << "`severity` INTEGER NOT NULL, PRIMARY KEY('idx'));";
    return sst.str();
  }

  std::string create_table_log(const std::string &name) const {
    std::ostringstream sst;
    sst << "CREATE TABLE IF NOT EXISTS `" << name << "` (`idx` INTEGER NOT NULL, `ref_idx` INTEGER NOT NULL,";
    for (auto &it : msg.items()) {
      if (std::find(exclude_keys_create.begin(), exclude_keys_create.end(), it.key()) == exclude_keys_create.end()) {
        sst << "`" << it.key() << "` TEXT,";
      }
    }
    sst << "`severity` INTEGER NOT NULL, PRIMARY KEY('idx' AUTOINCREMENT));";
    return sst.str();
  }

  std::string insert_into_table_log(const std::string &table_name) const {
    std::ostringstream sst;
    sst << "INSERT INTO `" << table_name << "` (";
    sst << "`ref_idx`,";
    for (auto &it : msg.items()) {
      if (std::find(exclude_keys_create.begin(), exclude_keys_create.end(), it.key()) == exclude_keys_create.end()) {
        sst << "`" << it.key() << "`,";
      }
    }
    sst << "`severity`) VALUES (";
    sst << msg["ref_idx"] << ",";
    for (auto &it : msg.items()) {
      if (std::find(exclude_keys_create.begin(), exclude_keys_create.end(), it.key()) == exclude_keys_create.end()) {
        sst << "'" << to_text(it) << "', ";
      }
    }
    sst << msg["severity"] << ");";
    return sst.str();
  }

  std::string insert_into_table_status(const std::string &table_name = "") const {
    std::ostringstream sst;
    std::string local_table_name = table_name;
    size_t i = 0;
    if (!table_name.size())
      local_table_name = msg["sender"];
    sst << "INSERT INTO `" << local_table_name << "` (";
    for (auto &it : msg.items()) {
      if (it.key() != always_exclude_key) {
        sst << "`" << it.key() << "`, ";
      }
    }
    // remove last comma
    sst.seekp(-2, std::ios_base::end);

    sst << ") VALUES (";
    for (auto &it : msg.items()) {
      if (it.key() != always_exclude_key) {
        sst << "'" << to_text(it) << "', ";
      }
    }
    // remove last comma
    sst.seekp(-2, std::ios_base::end);
    sst << ");";
    return sst.str();
  }

  std::string update_table_status(const std::string &table_name = "") const {
    std::ostringstream sst;
    std::string local_table_name = table_name;
    if (!table_name.size())
      local_table_name = msg["sender"];
    sst << "UPDATE `" << local_table_name << "` SET ";
    for (auto &it : msg.items()) {
      if ((it.key() != always_exclude_key) && (it.key() != "idx")) { // idx is the primary key, do not update
        sst << "`" << it.key() << "` = '" << to_text(it) << "', ";
      }
    }
    // remove last comma
    sst.seekp(-2, std::ios_base::end);
    sst << " WHERE `idx` = " << msg["idx"] << ";";
    return sst.str();
  }

  void set_date_time(const std::string &date, const std::string &time) {
    msg["date"] = date;
    msg["time"] = time;
  }

  void set_timestamp(const std::time_t &timestamp) {
    std::string date, time;
    mstr::date_and_time(timestamp, date, time);
    msg["date"] = date;
    msg["time"] = time;
  }

  void set_ref_idx(const int &ref_idx) {
    msg["ref_idx"] = ref_idx;
  }

  void set_key(const std::string &key, const int &severity = 0) {
    msg["key"] = key;
    msg["severity"] = severity;
  }

  void set_sender(const std::string &sender) {
    msg["sender"] = sender;
  }

  void set_severity(const int &severity) {
    msg["severity"] = severity;
  }

  void set_log_only() {
    msg["log_only"] = 1;
  }

  // set value --- unless the key is not changed you update the value
  void set_value(const auto &T, const int &severity = 0, const int &log_only = 0) {
    msg["severity"] = severity;
    msg["value"] = T;
    msg["log_only"] = log_only;
  }

  // needed for latitudes and longitudes ONLY
  void set_value_precise(const double &value, const int &severity = 0, const int &log_only = 0) {
    msg["severity"] = severity;
    std::ostringstream sst;
    sst << std::setprecision(std::numeric_limits<double>::digits10) << std::scientific << value;
    msg["value"] = sst.str();
    msg["log_only"] = log_only;
  }

  // key value --- you can change the key and the value
  void set_key_value(const std::string &key, const auto &T, const int &severity = 0, const int &log_only = 0) {
    msg["key"] = key;
    msg["severity"] = severity;
    msg["value"] = T;
    msg["log_only"] = log_only;
  }

  // needed for latitudes and longitudes ONLY
  void set_key_value_precise(const std::string &key, const double &value, const int &severity = 0, const int &log_only = 0) {
    msg["key"] = key;
    msg["severity"] = severity;
    std::ostringstream sst;
    sst << std::setprecision(std::numeric_limits<double>::digits10) << std::scientific << value;
    msg["value"] = sst.str();
    msg["log_only"] = log_only;
  }

private:
  jsn msg;
  std::list<std::string> exclude_keys_create = {"idx", "ref_idx", "severity", "log_only"}; //!< keys are handled differently
  std::string always_exclude_key = "log_only";                                             //!< key shall not appear in the database
};

// **********************************  M U L T I P L E  M E S S A G E S  *****************************************************************

/*!
 * @brief multiple_msg_to_sqlite is the class for multiple messages - e.g. for a  T A B L E  with multiple keys
 * @details a sender can / will contain  M U L T I P L E  keys for multiple messages; reminder: a message is a  R O W  in a  T A B L E  of a database
 * for status we have F I X E D  keys and a fixed table size; for log we append messages to the table and increment the index
 */

class multiple_msg_to_sqlite {
public:
  multiple_msg_to_sqlite(const std::string &sender_name, const std::vector<std::string> &keys) : sender_name(sender_name) {
    int i = 1;
    std::string date, time;
    std::time_t timestamp = std::time(nullptr);
    mstr::date_and_time(timestamp, date, time);

    // create all keys - and I let you not change the keys later, v  is private
    // a running status table will crash if you change the keys and there index
    v.reserve(keys.size());
    for (const auto &key : keys) {
      v.emplace_back(msg_to_sqlite(i++, date, time, 0, sender_name, key, "", 0, false));
    }
  }

  /*!
   * @brief create a multiple message from a json file
   * @param sender_name like GPS, ch0 ... chX, ADU, ADC, Batt_1, Batt_2 and so on
   * @param json_file - same as we use in PHP on the webserver
   */
  multiple_msg_to_sqlite(const std::string &sender_name, const std::filesystem::path &json_file) : sender_name(sender_name) {
    int i = 1;
    std::string date, time;
    std::time_t timestamp = std::time(nullptr);
    mstr::date_and_time(timestamp, date, time);

    if (!std::filesystem::exists(json_file))
      throw std::runtime_error("json file " + json_file.string() + " does not exist");

    // create all keys - and I let you not change the keys later, v  is private
    // a running status table will crash if you change the keys and there index
    std::ifstream ifs(json_file);
    jsn j;
    ifs >> j;
    ifs.close();
    v.reserve(j.size());
    for (auto &it : j.items()) {
      std::cerr << it.key() << " " << it.value() << std::endl;
      // std::string value = it.value().get<std::string>();
      msg_to_sqlite m(i++);
      m.set_date_time(date, time);
      m.set_sender(sender_name);
      m.set_ref_idx(0);
      m.set_key_value(it.key(), it.value());
      m.set_severity(0);
      v.emplace_back(m);
    }
  }

  std::string get_sender_name() const {
    return sender_name;
  }

  void set_date_time(const std::string &key, const std::string &date, const std::string &time) {
    for (auto &msg : v) {
      if (msg.key() == key) {
        msg.set_date_time(date, time);
        return;
      }
    }
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << " key " << key << " not found in sender " << sender_name;
    throw std::runtime_error(err_str.str());
  }

  void set_timestamp(const std::string &key) {
    std::string date, time;
    std::time_t timestamp = std::time(nullptr);
    mstr::date_and_time(timestamp, date, time);
    for (auto &msg : v) {
      if (msg.key() == key) {
        msg.set_date_time(date, time);
        return;
      }
    }
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << " key " << key << " not found in sender " << sender_name;
    throw std::runtime_error(err_str.str());
  }

  void set_ref_idx(const std::string &key, const int &ref_idx) {
    for (auto &msg : v) {
      if (msg.key() == key) {
        msg.set_ref_idx(ref_idx);
        return;
      }
    }
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << " key " << key << " not found in sender " << sender_name;
    throw std::runtime_error(err_str.str());
  }

  void set_value(const std::string &key, const auto &T, const int &severity = 0, const int &log_only = 0) {
    for (auto &msg : v) {
      if (msg.key() == key) {
        msg.set_value(T, severity, log_only);
        msg.set_timestamp(std::time(nullptr));
        return;
      }
    }
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << " key " << key << " not found in sender " << sender_name;
    throw std::runtime_error(err_str.str());
  }

  void set_value_precise(const std::string &key, const double &value, const int &severity = 0, const int &log_only = 0) {
    for (auto &msg : v) {
      if (msg.key() == key) {
        msg.set_value_precise(value, severity, log_only);
        return;
      }
    }
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << " key " << key << " not found in sender " << sender_name;
    throw std::runtime_error(err_str.str());
  }

  msg_to_sqlite log_only_message(const std::string &key, const auto &T, const int &severity = 0, const int &ref_idx = 0) {
    msg_to_sqlite l(0);
    l.set_key_value(key, T, severity, 1);
    l.set_timestamp(std::time(nullptr));
    l.set_sender(this->sender_name);
    l.set_ref_idx(ref_idx);
    // std::cerr << "log only message " << l.get_msg() << std::endl;
    return l;
  }

  msg_to_sqlite get(const std::string &key) const {
    for (auto &msg : v) {
      if (msg.key() == key) {
        return msg;
      }
    }
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << " key " << key << " not found in sender " << sender_name;
    throw std::runtime_error(err_str.str());
  }

  std::string create_status_table(const std::string table_name) const {
    if (!v.size())
      throw std::runtime_error("no keys defined");

    std::ostringstream sst;
    sst << v[0].create_table_status(table_name);
    for (auto msg : v) {
      sst << msg.insert_into_table_status(table_name);
    }
    return sst.str();
  }

  std::string create_log_table(const std::string table_name) const {
    if (!v.size())
      throw std::runtime_error("no keys defined");

    std::ostringstream sst;
    sst << v[0].create_table_log(table_name);
    return sst.str();
  }

private:
  std::vector<msg_to_sqlite> v; //!< list of messages to be written to the database
  const std::string sender_name;
};

/**
 * @file messages.h
 * @brief messages.h: defines messages to be send to a sqlite database or write to an file
 */

// adu senders example for my web test
std::vector<std::string> adu_keys = {
    "start_date",
    "start_time",
    "duration",
    "sampling_rate",
    "min_voltage",
    "grid_mode",
    "utc_offset",
    "dst",
    "safe_mode",
    "lab_mode",
    "bat 1",
    "bat 2",
    "temp"};

// gps sender - example for my web test
std::vector<std::string> gps_keys = {
    "gps",
    "galileo",
    "glonass",
    "beidou",
    "tracked",
    "in_use",
    "latitude",
    "longitude",
    "elevation"};

// channel senders - example for my web test
std::vector<std::string>
    channel_keys = {"channel", "sensor", "chopper", "gain", "serial", "angle", "tilt"};

#endif // MESSAGES_H
