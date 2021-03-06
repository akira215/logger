# Logger
This is a small job around logger, composed by two component:
* logger class : a class that will act as an interface to the user
* logger_policy classes : they shall all inherit from the abstract class `log_policy_interface`, and it will deal the outuput of the log

The class support multiple loggers (with mutliple logging pocilicies), is thread safe and could be retrive from anywhere in your code.

# Changes
* Commit 08/05/2020 : improving CPU load, getting thread safe, adding default_logger facility to provide `lclog`,`lcout` and `lcerr` macros.


# Details on implementation and use

## Building
**Please note that C++11 or higher compiler is required for the logger**

**C++17 or higher compiler is required for the `ringfile_log_policy`**

To build this logger :
  * add `-std=c++17` directive to the compiler (or`-std=c++11`) if `ringfile_log_policy` is not required
  * add `-pthread` library to the compiler that will give to the linker
  * add `-lstdc++fs` library if `ringfile_log_policy` is required

Example of Makefile
```
CXX		  := /usr/bin/
CXX_FLAGS := -Wall -Wextra -std=c++17 -ggdb

BIN		:= bin
SRC		:= src
INCLUDE	:= include
LIB		:= lib

LIBRARIES	:= -pthread -lstdc++fs

EXECUTABLE	:= MyLogger


all: $(BIN)/$(EXECUTABLE)

run: clean all
	clear
	./$(BIN)/$(EXECUTABLE)

$(BIN)/$(EXECUTABLE): $(SRC)/*.cpp
	$(CXX) $(CXX_FLAGS) -I$(INCLUDE) -L$(LIB) $^ -o $@ $(LIBRARIES)

clean:
	-rm $(BIN)/*

```

## logger class
### Thread safe
`logger` class is thread safe, meaning that you can call the same logger in different thread, as you could see in the example below. However, if different logger log on same output, log lines order could be mixed up (but one log line will always stay consistent), because the logger class don't manage which thread will the first access to the `print` method.

### Construction
Contructor has default values on all the args, so object could be instanciante without any arguments
```
logger(log_policy_interface* policy = 
           (log_policy_interface*) new file_log_policy(),
           const std::string& name = "logger.log",
           const std::string& path = "./");
```
* `policy` : it should be a pointer to the policy class which should inherit from `log_policy_interface`
* `name` : the logger name. It will be used to retrieve the logger from anywhere using the static method. In addition, it will be the filename in case of file logging policy.
* `path` : path of the logging file in case of file policy, unused otherwise.
```
logger(log_policy_interface* policy = 
           (log_policy_interface*) new file_log_policy(),
           const std::string& name = "logger.log",
           const std::string& path = "./");
```
* `policy` : it should be a pointer to the policy class which should inherit from `log_policy_interface`
* `name` : the logger name. It will be used to retrieve the logger from anywhere using the static method. In addition, it will be the filename in case of file logging policy.
* `path` : path of the logging file in case of file policy, unused otherwise.

If you construct a logger with a `name` that already exist in the logger list,
it will replace the existing one. A static function is available to check whether a logger is already registred with a name: `bool loggername_exist(const std::string& name)`, it return true only if a logger with `name` is already registered.

### Destruction
By calling the destructor (using `delete` on a variable created on the heap, or getting out of scope for a stack variable), the corresponding logger will be deleted (its thread stopped and purged) and its entry will be removed from the logger list, so it will not be accessible again.

Note that if you delete the default logger, the first logger on the list (i.e. smallest key name) will be the new default logger.

If you miss names or handles to logger, a static function can be call to delete all the available loggers `logger::logger_killall()`. After this call, no logger are available and memory is completely purged.

### Retrieve logger
As soon as a `logger` object is instancied, it is registred in a static map using its name as the key.
From anywhere in your application, you can retrieve a pointer to the logger object using the static function `logger::get_logger`. For example, assuming that `your_logger_name` is the `name` in the constructor:
```
#include "logger.hpp"
...
logger* _plog = logger::get_logger("your_logger_name");
```
As this method is slow, it shall be implemented for example in the contructor of your class and the pointer shall be stored as a member.

### Thread name
You can log date from different thread. If you want to log each thread name with its associate message, you have to use the `set_thread_name` method, like the following example:
```
 _plog->set_thread_name("your_thread");
```
This will register the correspondance of your thread name and the thread id that call the method. Following this, each time you will send a message with this thread, it will automatically retrieve the name you affected to it, and it will log it if header has been set with that information.

### Logging levels
The class define 6 logging level:
 * `debug` debug message
 * `info` information (ex startup a service)
 * `notice` Nothing serious, but notably nevertheless
 * `warning` Nothing serious by itself but might indicate problems
 * `error` Error condition
 * `critical` Critical condition, should stop or abord at least a process

Each message send to the logger shall have a logging level, it should be set like that :
 ```
 _plog->print<log_level::info>("Hello World !");
 ```

 Each logger has a mini logging level, which could be set by calling  :
 ```
 _plog->set_min_log_level(log_level::debug);
 ```


Messages sent to the logger with a inferior log level will be scrapped. By default, the log level is minimal (log_level::debug).

### Print macro

`logger.hpp` header file contains macro to ease your code.
 ```
 _plog->print<log_level::info>("Don't panic");
 ```
 is equivalent to:
  ```
 _plog->LOG_INFO("Don't panic");
 ```

Following macro are available:
```
#define LOG_DEBUG       print<log_level::debug>
#define LOG_INFO        print<log_level::info>
#define LOG_NOTICE      print<log_level::notice>
#define LOG_WARNING     print<log_level::warning>
#define LOG_ERROR       print<log_level::error>
#define LOG_CRITICAL    print<log_level::critical>
```
### Variadic print
`print` method has been implemented with variadic arguments. As it insert recursively the `args` in a `std::stringstream`, any type supported by the `<<`(insertion) operator could be used as type for `args` in the print method. As an example, you can write:
```
_plog->LOG_NOTICE("The result of ", 1, "divided by ", 2," is : ", 0.5);
```

### log macros
For convenience, 3 logging macros are available:
 * `lclog` which log on `log_level::debug`
 * `lcout` which log on `log_level::info`
 * `lcerr` which log on `log_level::error`

Note that these 3 macros logs on the defaut logger. Default logger is the first logger to be created by default. To change it, simply call the `set_default_logger` method.
Then to use the macro, you just have to include the header file and use it just as `std::clog`, `std::cout` and `std::cerr` streams:
```
#include "logger.hpp"

class myClass{
  myClass();
};

//myClass constructor
myClass::myClass() {
  lcout << "initializing myClass;
  lcout << 1 <<std::endl << "+" <<std::endl << "1";
}
```
Note that `std::endl` trigger a new log line (including header, ...), but is not required at the end of one << stream, it will be appened automaically.


### Header pattern
Header of each log message could be parametred using the `set_pattern` method. It takes a `std::string` as argument, wich is basically composed by:
  * **user characters** which are used as delimiter and/or decorator. It can be any char except `%` char and sometimes `&`.
  * **predefined fields** which are informations that you wanted to be printed in each logging message. Each field is identified by only one char, preceded by the delimiter `%` (you know understand why you can't use it as a decorator). All predifined field are:
    * `%d` : date field, see below to format it
    * `%i` : (index) = line number. Be aware that line number is incremented even if you don't show it in your log.
    * `%l` : log level of the message (i.e. `CRITICAL`)
    * `%n` : logger name, set up in the constructor (useless for file logging as it is the name of the file)
    * `%t` : time field, see below to format it
    * `%x` : thread name, as defined by calling `set_thread_name(name)`
    * any char out of this list will be scrapped (as well as the `%` delimiter)
    You can add several time the same field (even if it seems to be useless).

Date and time pattern have a default formatting, respectively `"%d-%m-%Y"` and `"%H:%M:%S"`. You can change this by two ways
  * Calling the methods `set_date_format` and/or `set_time_format`. The argument of both method is a `std::string` that describe the date/time format, according to `std::put_time format` (refer to C++11 ore greater documentation). You can write a time format in the date field and vice versa. The 2 fields are made for convenience, as only one format is supported for each field (meaning that in you patter you can add as many %d %d %d %d ... as you want, the date format for each field will always be the same).
  * You can include directly in the pattern of `set_pattern` method. To do so, immedialely after the `%d` or `%t` mark, you should put your date/time format, enclosed by `&` char.

Herebelow some examples:
```
_plog->set_pattern("[%d&%d-%m-%y& %t&%H:%M:%S&]-{%l}+i ");
_plog->LOG_ERROR("computer is about to blow up");
// Output :
[17-04-20 00:14:11]-{ERROR}+1 computer is about to blow up

// Other example
_plog->set_thread_name("marvin");
_plog->set_min_log_level(log_level::info);
_plog->set_pattern("#%i:[%d&%a %d-%B-%y& %t]-[%l]-[%x]:");
_plog->LOG_INFO("Don't panic");
// Output :
#1:[Fri 17-April-20 22:13:45]-[INFO]-[marvin]:Don't panic
```
## log policy classes
log policy are the target of the logger. They all shall inherit from `log_policy_interface` abstract class:
```
class log_policy_interface
{
public:
    virtual ~log_policy_interface() = 0;
    virtual void		open_out_stream(const std::string& name) = 0;
    virtual void		close_out_stream() = 0;
    virtual void		write(const std::string& msg) = 0;
    
};
```
The name of the members to be implemented explain by itself the purpose of the method.

### Existing policies
For now only two policies are implemented:
  * `file_log_policy`, which basically log into a file
  * `ringfile_log_policy`, which log on `n` rolling files, with a max size per file. At startup, the last modified file is selected. If there is enough space to logg data in this file, data, will be appened, if not, rotating process occured. 2 args in the constructor :
    * `max_size` max size of one file in bytes, default value is 1MB. Note that 1KB = 1 024 bytes (and 1MB = 1 024KB and so on), and that the policy don't break log messages, and always keep file size less than `max_size`. File will be rotate if the incoming message is too big regarding the actual file size.
    *  `max_file_count` is the max number of rotating file, default value is 2. Note that a number will be appened to the filename, starting by `0` and up to `max_file_count` - 1, so you will have for example `execution.log.0`, `execution.log.1`, ...
    * Please note that with 2 files, in the worst case, you will have `max_size` data logged, with 3 files worst case is 2*`max_size`, ...
  * `dailyfile_log_policy`, which log on a file that change everyday. At startup, the file of the day is selected and log data appened. 3 args in the constructor :
    * `decimal_rotate_hour` the hour when the file rotation shall occured, in decimal (i.e. 12.25 represent 12h15', you can get the conversion from hour to decimal by dividing minutes per 60). Default value is 0.0. Note that a convention is that the day x start the latter at 12:00 and finish at 12:00 day x+1. If your rotate point is 5:00, the file will have the name of the day x from day x 5:00 to day x+1 5:00. But if rotate point is 17:00, the file will have the name of day x-1 from day x-1 17:00 to day x 17:00.
    *  `max_file_count` is the max number of rotating file, default value is 30. It will be interpreted as a number of day from today. Files with the correct name will be scanned, and the date will be determined by the extension (note exploiting the file system date), using the `fmt` format. All file older than `max_file_count` days will be deleted. Files that doesn't match the pattern are ignored. The check operation is done each time a new file is created. 
    *  `fmt` is the date format that will be used as an extension to the log filename. For example, the default format will generate `execution.log.2020-04-17`, `execution.log.2020-04-18`, ... You can tweak the format checking `std::put_time` from `<iomanip>` documentation.
  * `stdout_log_policy`, which send the log to stdout.
  * `spread_log_policy`, which spread log message to several log policy (which obviously all inherit from `log_policy_interface`). `spread_log_policy` has a variadic constructor, you should add as many as logging polcies as you want, just take care of the performance. Another caveat when using `spread_log_policy` is that all policies will have the same name, so the same filename. It is not a problem if one policy is only one policy is a `file_log_policy`. `stdout_log_policy` has no filename and `ringfile_log_policy` will append a number after the logger filename. Also keep in mind that you will have to set up the base policies before calling the `spread_log_policy` contructor (max file size, ...)

You can easily developp new policies, by inheriting from the abstract class `log_policy_interface`. You basically only have to implement 3 methods:
  * `void open_out_stream(const std::string& name)`
  * `void close_out_stream()`
  * `void write(const std::string& msg)`

## Example
Herebelow a sample example to illustrate simple use of the logger :

**Be aware that the last for loop will take 5 minutes to run (300 iterations @ 1s)**

In main.cpp
```
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
```


## License
Copyright (c) 2020 Akira Shimahara <akira215corp@gmail.com>

This program is free software; you can redistribute it and/or modify it under the therms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
 
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.


