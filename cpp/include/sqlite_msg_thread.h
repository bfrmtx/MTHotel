#ifndef SQLITE_MSG_THREAD_H
#define SQLITE_MSG_THREAD_H

#include <cfloat>
#include <thread>
#include <iostream>
#include <queue>
#include <mutex>
#include <algorithm>
#include <condition_variable>
#include <atomic>
#include <random>
#include <memory>
#include <utility>
#include <sstream>
#include <chrono>
#include <shared_mutex>

#include "messages.h"

#include <sqlite_handler.h>



/*!
 * \brief The msg_sqlite_receiver class running in a thread like  std::jthread xd(&msg_sqlite_receiver::run, logger.get());

 */
class msg_sqlite_receiver
{
public:
    msg_sqlite_receiver(std::filesystem::path sqlfile) {

        this->mtx = new std::mutex();
        this->act  = new std::atomic<int>(0);
        this->cond = new std::condition_variable();
        this->message_queue = new std::queue<msg_to_sqlite>;
        this->sqlfile = sqlfile;
        this->sql_log = std::make_unique<sqlite_handler>(this->sqlfile);

        // may throw exception; catch in main
        //this->sql_query = "CREATE TABLE IF NOT EXISTS `logs` ('idx'	INTEGER NOT NULL, `timestamp` INT8, `sender` TEXT, `message` TEXT, `severity` DOUBLE, `dval` DOUBLE, PRIMARY KEY('idx' AUTOINCREMENT))";
        this->sql_log->create_table(create_sqlite_message_table);
    }

    void reset_db() {
        if (this->sql_log != nullptr) this->sql_log.reset();
    }

    ~msg_sqlite_receiver() {
        if(tr != nullptr) {
            tr->join();
            delete tr;
        }
        delete this->mtx;
        delete this->act;
        delete this->cond;
        delete this->message_queue;
        if (this->sql_log != nullptr)this->sql_log->vacuum();
        if (this->sql_log != nullptr) this->sql_log->close();
        std::cout << counter <<  " messages cought" << std::endl;
    }

    void run() {
        tr =  new std::jthread([this]() {sub();} );
    }


    mutable std::mutex *mtx;
    std::atomic<int>  *act;
    std::condition_variable  *cond;
    std::queue<msg_to_sqlite> *message_queue;

private:
    std::string sql_query;
    std::unique_ptr<sqlite_handler> sql_log;
    std::filesystem::path sqlfile;
    uint64_t counter = 0;
    std::jthread *tr = nullptr;

    void sub() {

        while (true) {
            std::unique_lock lock(*this->mtx);
            cond->wait(lock, [this] { return !(*this->act) || !this->message_queue->empty(); }); // spurious wakeup protection, i.e:
            while (!message_queue->empty()) {
                std::time_t s = std::time(nullptr);
                this->message_queue->front().timestamp = s;
                //std::cerr << this->sql_query + msg_to_sqlite_sqlstr(this->message_queue->front()) << std::endl;
                try{
                    this->sql_log->insert((insert_sqlite_message_table + msg_to_sqlite_sqlstr(this->message_queue->front())), true);
                }
                catch (const std::string &error ) {
                    std::cerr << error <<std::endl;
                }
                ++this->counter;
                message_queue->pop();
            }
            if (*this->act == 0) {
                break;
            }
        }
    }
};



/*!
 * \brief The msg_sender class note: this class has no loop; it sends the message when ever you like
 */
class msg_sender
{
public:
    msg_sender(const std::unique_ptr<msg_sqlite_receiver> &msc) : mtx(msc->mtx), act(msc->act), cond(msc->cond), message_queue(msc->message_queue) {

        ++(*this->act);;      // increment the living sender
    }

    ~msg_sender() {
        --(*this->act);
        cond->notify_all();
    }


    void submit(const msg_to_sqlite &msg) {
        std::unique_lock lock(*mtx);
        this->message_queue->emplace(msg);
        cond->notify_all();
    }

private:
    mutable std::mutex *mtx;
    std::atomic<int>  *act;
    std::condition_variable  *cond;
    std::queue<msg_to_sqlite> *message_queue;


};




//multiple thread can get

//unsigned int get() const {
//    std::shared_lock lock(mutex_);
//    return value_;
//}

// Only one thread/writer can increment/write the counter's value.
//void increment() {
//    std::unique_lock lock(mutex_);
//    ++value_;
//}



#endif // SQLITE_MSG_THREAD_H
