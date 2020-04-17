#include "logger.hpp"

int main(){
   logger *rogue_one =new logger(new file_log_policy(), 
                            "rpi-dev/execution.log");
   logger *rogue_two =new logger(new stdout_log_policy());

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
    rogue_two->set_pattern("#%i:[%d&%d-%m-%y& %t]-[%l]-[%x]:");
    
    rogue_two->LOG_DEBUG("This is the ", 1, "st test");
    rogue_two->LOG_INFO("On this computer");
    rogue_two->LOG_ERROR("Don't panic");
    rogue_two->LOG_CRITICAL("But", 67, " cast");

    delete rogue_one;
    delete rogue_two;
    std::cout << "That's all folks" << std::endl;

}
