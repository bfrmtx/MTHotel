#ifndef MESSAGES_H
#define MESSAGES_H

#include <string>
#include <climits>
#include <sstream>
#include <cmath>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <filesystem>
#include <atomic>
#include "json.h"
/**
 * @file messages.h
 * @brief messages.h: defines messages to be send to a sqlite database or write to an file
 */

// **********************************  L O G  &  E R R O R  D A T A B A S E *****************************************************************

struct msg_to_sqlite
{
    int64_t timestamp = 0;          //!< to be filled in by the receiver; 64 bit is timestamp on new Linux systems
    int64_t ref = 0;                //!< reference to an index for a long explanation of the message inside a database of detailed explanations
    std::string sender;             //!< string id of the sender like Batt_1, GPS, ADC and so on
    std::string message;            //!< message string short
    int64_t severity =    0;        //!< 0 = info ...
    int64_t ival_first =  0;        //!< int value like 9 for satellites   ... or RANGE start
    int64_t ival_second = 0;        //!<                                   ... or RANGE stop
    double  dval_first =  0;        //!< 12.3 for battery ...              and LATITUDE  or RANGE start
    double  dval_second = 0;        //!<                               ... and LONGITUDE or RANGE stop
};

std::string msg_to_sqlite_sqlstr(const msg_to_sqlite &msg) {
    std::ostringstream sst;
    sst << "VALUES(";
    sst << msg.timestamp << ",";
    sst << msg.ref << ", ";
    sst << "'" << msg.sender << "', ";
    sst << "'" << msg.message << "', ";
    sst << msg.severity << ", ";
    sst << msg.ival_first << ", ";
    sst << msg.ival_second << ", ";
    if (trunc(msg.dval_first) == msg.dval_first) sst  << msg.dval_first << ", ";
    else sst  << std::setprecision(std::numeric_limits<double>::digits10) << std::scientific << msg.dval_first << ", ";
    if (trunc(msg.dval_second) == msg.dval_second) sst << msg.dval_second;
    else sst  << std::setprecision(std::numeric_limits<double>::digits10) << std::scientific << msg.dval_second;
    sst << ");";

    return sst.str();
}

static std::string create_sqlite_message_table("CREATE TABLE IF NOT EXISTS `logs` (`idx` INTEGER NOT NULL, `timestamp` INT8,  `ref` INT8,\
                                            `sender` TEXT, `message` TEXT, `severity` INT8, \
                                            `ival_first` INT8, `ival_second` INT8, \
                                            `dval_first` DOUBLE, `dval_second` DOUBLE, PRIMARY KEY('idx' AUTOINCREMENT));");

    // make a "insert_sqlite_message_table + msg_to_sqlite_sqlstr" to finally insert
    static std::string insert_sqlite_message_table("INSERT INTO `logs` (`timestamp`, `ref`, `sender`, `message`, `severity`, `ival_first`, `ival_second`, `dval_first`, `dval_second`) ");

static std::string create_sqlite_message_explanation_table("CREATE TABLE IF NOT EXISTS `help` (`key` INT8, `value` TEXT);");


std::atomic_int64_t write_json_message_file_written;

bool write_json_message_file(const std::filesystem::path &directory_path_only, const msg_to_sqlite &msg) {

    std::filesystem::path filepath(std::filesystem::canonical(directory_path_only));

    if (!std::filesystem::exists(filepath)) {
        std::filesystem::create_directory(filepath);
        std::cout << "creating " << filepath << std::endl;
        if (!std::filesystem::exists(filepath)) {
            std::cout << "creating " << filepath << " " << "failed!" <<  std::endl;
            std::ostringstream err_str(__func__, std::ios_base::ate);
            err_str << " can not create message in dir: " << filepath;
            throw err_str.str();
        }
    }
    std::ofstream file;
    filepath /= ("msg_" + std::to_string(write_json_message_file_written++) + ".json");
    file.open(filepath, std::fstream::out | std::fstream::trunc);
    if (!file.is_open()) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << " can not create message " << filepath;
        throw err_str.str();
    }

    nlohmann::ordered_json jmsg;                // use ordered because of readability
    jmsg["timestamp"]   = msg.timestamp;
    jmsg["ref"]         = msg.ref;
    jmsg["sender"]      = msg.sender;
    jmsg["message"]     = msg.message;
    jmsg["severity"]    = msg.severity;
    jmsg["ival_first"]  = msg.ival_first;
    jmsg["ival_second"] = msg.ival_second;
    jmsg["dval_first"]  = msg.dval_first;
    jmsg["dval_second"] = msg.dval_second;

    file << std::setw(2) << jmsg << std::endl;
    file.close();
    return true;
}





#endif // MESSAGES_H
