#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <list>
#include <memory>
#include <vector>
#include <thread>
#include <sstream>

#include <chrono>
#include "messages.h"
#include "sqlite_msg_thread.h"




int main() {

    std::filesystem::path sqlfile(std::filesystem::temp_directory_path() / "log_test.sql3");
    std::filesystem::path logdir(std::filesystem::temp_directory_path());
    std::unique_ptr<msg_sqlite_receiver> logger;

    try {
        logger = std::make_unique<msg_sqlite_receiver>(sqlfile);

    }
    catch (const std::string &error) {
        logger.reset();
        std::cerr << error << std::endl;
        return EXIT_FAILURE;
    }

    // create my messengers
    auto message_sender_1 =std::make_unique<msg_sender>(logger);
    auto message_sender_2 =std::make_unique<msg_sender>(logger);

    // start the receiver thread
    logger->run();

    // create message sender
    auto message_1 = msg_to_sqlite();
    auto message_2 = msg_to_sqlite();

    message_1.sender = "Batt_1";
    message_1.message = "V";
    message_1.dval_first = 12.3;

    message_2.sender = "GPS";
    message_2.message = "Sats tracked";
    message_2.ival_first = 2;

    size_t loops(10);           // simulate live

    for (size_t i= 0; i < loops; ++i) {
        message_sender_1->submit(message_1);
        message_sender_2->submit(message_2);

        write_json_message_file(logdir, message_1);
        write_json_message_file(logdir, message_2);

        message_1.ival_first +=1;
        message_2.ival_first += 1;
        std::cerr << i << std::endl;

    }

    // another function is called - here simuled by try & catch
    // example - a later created messenger will auto delete when leaving the scope
    try {
        auto msg3 =std::make_unique<msg_sender>(logger);
        auto sta3 = msg_to_sqlite();

        sta3.sender = "GPS";
        sta3.message = "position";
        sta3.dval_first = 39.0261966;
        sta3.dval_second = 29.12395333;

        msg3->submit(sta3);
    }
    catch (...) {
        std::cerr << "error in scope" << std::endl;
    }


    std::time_t s = std::time(nullptr);
    std::cout << std::endl << s << " seconds since the Epoch\n";
    // set datafile separator "|"
    // set xdata time
    // set timefmt "%s"
    // set format x "%m/%d/%Y %H:%M:%S"
    // plot '< sqlite3 log_test.sql3 "select timestamp, ival_first from logs WHERE sender=\"GPS\" and message like \"Sats tracked\" ";' using 1:2
    // select timestamp, ival_first from logs WHERE sender="GPS" AND message like "Sats tracked";

    return EXIT_SUCCESS;


}


