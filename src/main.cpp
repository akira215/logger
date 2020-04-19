#include "logger.hpp"

#include <thread>
#include <chrono>


int main(){
    logger *rogue_one = new logger(new file_log_policy(), 
                            "rpi-dev/execution.log");
    logger *rogue_two = new logger(new stdout_log_policy(),
                            "rpi-dev/rogue_two.log");

    /* Rotatinq on 3 files rogue_three.log.0,1 and 3, max size is 2048 byte */ 
    logger *rogue_three = new logger(new ringfile_log_policy(2048, 3),
                            "rpi-dev/rogue_three.log");

    /* Spread on one file and stdout output */ 
    logger *alpha_one = new logger(new spread_log_policy(new file_log_policy(),
                             new stdout_log_policy()),
                            "rpi-dev/alpha_one.log");
    
    /* A daily log file will be created each day at 17h15' */ 
    logger *alpha_bravo = new logger(new dailyfile_log_policy(17.25),
                            "rpi-dev/alpha_bravo.log");

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

    for(int i=0 ; i<300; i++) {
        alpha_bravo->LOG_DEBUG("This is the #", i, " record");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    

    delete rogue_one;
    delete rogue_two;
    delete rogue_three;
    std::cout << "That's all folks" << std::endl;
}
