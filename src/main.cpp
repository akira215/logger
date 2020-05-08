#include "logger.hpp"

#include <thread>
#include <chrono>

 std::atomic<bool> thread_run;

void write_thread(const log_level loglevel, const std::string& thread_name)
{
    logger* log = logger::get_default_logger();
    log->set_thread_name(thread_name);
    std::string line;
    int i=0;
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        log->print(loglevel,thread_name, " -- ", i);

        i++;
               
    } while( thread_run.load() || i<10 );
}

int main(){
    logger *rogue_one = new logger(new file_log_policy(), 
                            "logs/execution.log");
    logger *rogue_two = new logger(new stdout_log_policy(),
                            "logs/rogue_two.log");
    
    bool exist = logger::loggername_exist("logs/rogue_two.log");

    /* Rotatinq on 3 files rogue_three.log.0,1 and 3, max size is 2048 byte */ 
    logger *rogue_three = new logger(new ringfile_log_policy(2048, 3),
                            "logs/rogue_three.log");

    /* Spread on one file and stdout output */ 
    logger *alpha_one = new logger(new spread_log_policy(new file_log_policy(),
                             new stdout_log_policy()),
                            "logs/alpha_one.log");
    
    /* A daily log file will be created each day at 17h15' */ 
    logger *alpha_bravo = new logger(new dailyfile_log_policy(17.25),
                            "logs/alpha_bravo.log");

    rogue_one->set_thread_name("computer");
    rogue_one->set_min_log_level(log_level::info);
    rogue_one->set_pattern("#%i:[%d&%a %d-%B-%y& %t]-[%l]-[%x]:");

    rogue_one->LOG_DEBUG("I can't print this"); // will be scrapped
    rogue_one->LOG_INFO("because min log level has been set higher"); 
    rogue_one->LOG_NOTICE("This is due to bad coding guy"); 
    rogue_one->LOG_WARNING("I think he is going crazy"); 
    rogue_one->LOG_ERROR("He started to hit the computer");
    rogue_one->LOG_CRITICAL("I'm about to explode !");

    rogue_two->set_thread_name("log_two");
    rogue_two->set_pattern("#%i:[%d&%d-%m-%y& %t]-[%l]-[%n]:");
    
    rogue_two->LOG_DEBUG("This is the ", 1, "st test");
    rogue_two->LOG_INFO("On this computer");
    rogue_two->LOG_ERROR("Don't panic");
    rogue_two->LOG_CRITICAL("But ", 0.5, " cast");

    for(int i=0 ; i<10000; i++)
        rogue_three->LOG_DEBUG("This is the #", i, " record");

    alpha_one->LOG_DEBUG("Spreading log on ", 2, " outputs");
    alpha_one->LOG_INFO(1, "st one is a file log");
    alpha_one->LOG_INFO(2, "nd one is a stdout log");
    alpha_one->LOG_NOTICE("you can add as many output as you want");
    alpha_one->LOG_WARNING("But take care of the performance");

/* This test take more than 300s i.e. more than 5 minutes
 * in order to test the daily rotating file behavior
    for(int i=0 ; i<300; i++) {
        alpha_bravo->LOG_DEBUG("This is the #", i, " record");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
*/

    rogue_two->set_default_logger();
    thread_run.store(true);
    std::thread t1 = std::thread( &write_thread,
                                        log_level::error, "t1");
    std::thread t2 = std::thread( &write_thread,
                                        log_level::info, "t2");
    
    thread_run.store(false);
    t1.join();
    t2.join();

    lcout << "This will be log by " << " the default logger";
    lcerr << "idem, but with different log level";
    lcout << "std:endl return a new log line" << std::endl << "as you can see";
    lcout << "but its not require at the end of a line";
    lcout << 1 << "+" << 1 << " = " << 2 << " not " << "11";

    delete rogue_one;
    delete rogue_two;   // When deleting rogue_two, default logger will
    delete rogue_three; // be changed, ...

    // logger_killall() will destroy all available loggers, even if
    // you don't know their name and you don't have any pointer to it
    // In this case, it will destroy alpha_one and alpha_bravo
    logger::logger_killall();

    std::cout << "That's all folks" << std::endl;
}
