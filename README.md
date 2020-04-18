# Logger
This is a small job around logger, composed by two component:
* logger class : a class that will act as an interface to the user
* logger_policy classes : they shall all inherit from the abstract class `log_policy_interface`, and it will deal the outuput of the log

## Building
**Please note that C++11 or higher compiler is required for the logger**
**C++17 or higher compiler is required for the `ringfile_log_policy`**
To build this logger :
  * add `-std=c++17` directive to the compiler (or`-std=c++11`) if `ringfile_log_policy`
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
  * `ringfile_log_policy`, which log on `n` rolling files, with a max size per file. At startup, the last modified file is selected. If there is enough space to logg data in this file, data, will be appened, if note, rotating process occured. 2 args in the constructor :
    * `max_size` max size of one file in bytes, default value is 1MB. Note that 1KB = 1 024 bytes (and 1MB = 1 024KB and so on), and that the policy don't break log messages, and always keep file size less than `max_size`. File will be rotate if the incoming message is too big regarding the actual file size.
    *  `max_file_count` is the max number of rotating file, default value is 2. Note that a number will be appened to the filename, starting by `0` and up to `max_file_count` - 1, so you will have for example `execution.log.0`, `execution.log.1`, ...
    * Please note that with 2 files, in the worst case, you will have `max_size` data logged, with 3 files worst case is 2*`max_size`, ...
  * `stdout_log_policy`, which send the log to stdout.
  * `spread_log_policy`, which spread log message to several log policy (which obviously all inherit from `log_policy_interface`). `spread_log_policy` has a variadic constructor, you should add as many as logging polcies as you want, just take care of the performance. Another caveat when using `spread_log_policy` is that all policies will have the same name, so the same filename. It is not a problem if one policy is only one policy is a `file_log_policy`. `stdout_log_policy` has no filename and `ringfile_log_policy` will append a number after the logger filename. Also keep in mind that you will have to set up the base policies before calling the `spread_log_policy` contructor (max file size, ...)

some policies may be developped:
  * daily file policy
  * policy distributor (send to 2 policies)

## Example
Herebelow a sample example to illustrate simple use of the logger :

In main.cpp
```
#include "logger.hpp"

int main(){
    logger *rogue_one =new logger(new file_log_policy(), 
                            "rpi-dev/execution.log");
    logger *rogue_two =new logger(new stdout_log_policy(),
                            "rpi-dev/rogue_two.log");

    /* Rotatinq on 3 files rogue_three.log.0,1 and 3, max size is 2048 byte */ 
    logger *rogue_three =new logger(new ringfile_log_policy(2048, 3),
                            "rpi-dev/rogue_three.log");

    /* spread on one file and stdout output */ 
    logger *alpha_one =new logger(new spread_log_policy(new file_log_policy(),
                             new stdout_log_policy()),
                            "rpi-dev/alpha_one.log");

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

    delete rogue_one;
    delete rogue_two;
    delete rogue_three;
    std::cout << "That's all folks" << std::endl;
}
```
To illustrate how to retrieve a pointer in another class : in far_away.h
```
#include "logger.hpp"

class far_away {
public:
    far_away();
private:
    logger* _p_one;
}

// Implementation of the contructor
far_away::far_away() {
  _p_one = logger::get_logger("execution.log");

  _p_one->LOG_INFO("I have got a pointer to the rogue_one logger !);
}
```


## License
Copyright (c) 2020 Akira Shimahara <akira215corp@gmail.com>

This program is free software; you can redistribute it and/or modify it under the therms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
 
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.


