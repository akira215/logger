#pragma once
/*
 * logger.hpp
 *
 * Written by Akira Shimahara
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <memory>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <ctime>

#include <mutex>
#include <chrono>
#include <cstring>
#include <thread>
#include <map>
#include <queue>
#include <atomic>
#include <cassert>
#include "log_policy.hpp"



/**
 * @brief log level definition
 * @brief macro defined to ease logger print call
 * @param debug debug message
 * @param info information (ex startup a service)
 * @param notice Nothing serious, but notably nevertheless
 * @param warning Nothing serious by itself but might indicate problems
 * @param error Error condition
 * @param critical Critical condition, should stop or abord
 */
enum class log_level
{
    debug = 1,
    info,
    notice,
    warning,
    error,
    critical      //6
};

/**
 * @brief macros. Prefered way to print using the logger
 * @brief logger->LOG_DEBUG("Locked here since ", 100, "days");
 */
#define LOG_DEBUG       print<log_level::debug>
#define LOG_INFO        print<log_level::info>
#define LOG_NOTICE      print<log_level::notice>
#define LOG_WARNING     print<log_level::warning>
#define LOG_ERROR       print<log_level::error>
#define LOG_CRITICAL    print<log_level::critical>

/**
 * @brief DEFAULT_PATTERN is the default header pattern when instancing
 * @brief a logger object
 */
#define DEFAULT_PATTERN "%d %t %l "

/**
 * @brief FORMAT_DELIMITER is the delimiter for date and time
 * @brief header format. It should follow immediately the 'd' or 't'
 * @brief delimiter, and should be added at the end of time / date format
 */
#define FORMAT_DELIMITER '&'

/**
 * @brief logger shall be instantiated with a specific log_policy
 * @brief by default a standard file log policy is set in the
 * @brief current directory. A static function
 * @brief logger::get_logger(const std::string& name);
 * @brief allow to retrieve a pointer to a specific logger
 * @brief by its name.
 * @brief 
 */
class logger
{
public:
    /** @brief logger constructor
     *  @brief each param has default value 
     *  @param policy log policy i.e.  where the log will be sent
     *  @param name of the logger. Wil be used for the file name
     *  @param path path of file, if no file in policy, unused
     */ 
    logger(log_policy_interface* policy = 
            (log_policy_interface*) new file_log_policy(),
            const std::string& name = "logger.log",
            const std::string& path = "./");

    /** @brief logger destructor
     *  @brief kill the daemon associated to the instance
     *  @brief remove the instance from map list
     *  @brief and call the close_out_stream() method of the policy
     */ 
    ~logger();


    /** @brief print the function to be called to log data.
     *  @brief Ex. logger->print<log_level::debug>(...)
     *  @brief variadics args combine all supported
     *  @brief std::stringstream << operator support
     *  @brief header with user pattern is prepared here
     */ 
    template< log_level severity , typename...Args >
    void print(Args&&...args);

    /** @brief set_thread_name()
     *  @brief set the thread name of the calling thread
     *  @brief that will be logged on each line (store in a map)
     *  @param name "name" of the thread
     */ 
    void set_thread_name(const std::string& name);

    /** @brief set_min_log_level()
     *  @param new_level minimum log level
     *  that will be displayed by this logger
     */ 
    void set_min_log_level(log_level new_level);

    /** @brief terminate_logger()
     *  @brief kill the thread
     */ 
    void terminate_logger();

   /** @brief get_logger static()
     *  @param name "name" of the logger
     *  @return logger* pointer to the logger
     *  @return nullptr if not found
     */ 
    static logger* get_logger(const std::string& name);

    /** @brief set_pattern()
     *  @brief Set the header pattern to be logged
     *  @param pattern is a string describing the format. String is composed by:
     *      * user characters which are used as delimiter and/or decorator. 
     *                      It can be any char except `%` char and sometimes `&`.
     *      * predefined fields which are informations that you 
     *                      wanted to be printed in each logging message. 
     *                      Each field is identified by only one char, 
     *                      preceded by the delimiter `%`. All predifined field are:
     *          `%d` : date field, see below to format it
     *          `%i` : (index) = line number. Be aware that line number
     *                  is incremented even if you don't show it in your log.
     *          `%l` : log level of the message (i.e. `CRITICAL`)
     *          `%n` : logger name, set up in the constructor 
     *                  (useless for file logging as it is the name of the file)
     *          `%t` : time field, see below to format it
     *          `%x` : thread name, as defined by calling `set_thread_name(name)`
     *           Any char out of this list will be scrapped (as well as the `%` delimiter)
     *  
     */ 
    void set_pattern(const std::string &pattern);

    /** @brief set_date_format()
     *   @param fmt the required format, according to std::put_time() format
     */ 
    void set_date_format(const std::string &fmt);

    /** @brief set_time_format()
     *   @param fmt the required format, according to std::put_time() format
     */ 
    void set_time_format(const std::string &fmt);

    /** @brief logging_daemon()
     *  @brief the dameon func that will be called by the
     *  thread
     */ 
    friend void logging_daemon( logger* logger );

    /** @brief Formatting functions
     *  @brief public as they may be interesting for others
     */
    std::string get_line_number();
    std::string get_date();
    std::string get_time();
    std::string get_thread_name();
    std::string get_log_level();
    std::string get_logger_name();
    std::string get_empty_string();

private:
    /** @brief print_impl core printing method
     *  @brief will be called once the overloaded recursive
     *  @brief will reach the end. It push the stream to the 
     *  @brief log buffer which will be exploited by the deamon 
     */
    void print_impl(std::stringstream&&);

    /** @brief print_impl overloaded variadic
     *  @brief it will be called by public print method
     */
    template<typename First, typename...Rest>
    void print_impl(std::stringstream&&,First&& parm1,Rest&&...parm);

    /** @brief _header_pattern store the user pattern
     *  @brief it is a vector of pair 
     *  @brief pair first are user char (separator, decorator,...)
     *  @brief pair second are pointer to member function that will return the info
     *  @brief first could be empty string and second could be a function
     *  @brief that will return empty string (logger::get_empty_string)
     */
    std::vector<std::pair<std::string, std::string (logger::*)()>> _header_pattern;

    /* min log level, message with a inferior level will not be printed */
    log_level _min_log_level;
    log_level _current_level; // only way to pass the level to the function
  
    /** @brief _date_format and _time_format
     *  @brief are captured in set_pattern when FORMAT_DELIMITER
     *  @brief char directly follow date or time tag. If FORMAT_DELIMITER is '&'
     *  @brief %d&date_format& where date_format is directly compliant with 
     *  @brief std::put_time format
     */
    std::string _date_format;
    std::string _time_format;

    /** @brief _daemon is the daemon that perform the output operations
     */
    std::thread _daemon;

    /** @brief _thread_name is the map where correspondence
     *  @brief thread_id <=> thread_name is stored
     */
    std::map<std::thread::id,std::string> _thread_name;

    /** @brief _the write mutex of the logger
     */
    std::timed_mutex _write_mutex;

    /** @brief _log_buffer is the media between the current user 
     *  @brief input operations and the daemon thread that perform
     *  @brief output operations
     */
    std::vector< std::string > _log_buffer;

    /** @brief _policy pointer to the policy class which shall
     *  @brief inherit from log_policy_interface
     */
    log_policy_interface* _policy;

    /** @brief _is_still_running flag for the running daemon
     */
    std::atomic_flag _is_still_running = ATOMIC_FLAG_INIT;

    /** @brief _log_line_number is incremented each time
     *  @brief logger::print method is called, even if it is
     *  @brief not printed in the user pattern
     */
    unsigned int _log_line_number;

    /** @brief path of the log file
     *  @brief unused in case of non file logger
     */ 
    std::string _path;

    /** @brief _name of the logger
     *  @brief will be used as filename for file logger
     */ 
    std::string _name;

    /** @brief satic _logger_list
     *  @brief map all available logger instance
     *  @param key string "name" of the logger
     *  @param T logger* pointer to the logger
     */ 
    static std::map<std::string, logger*> _logger_list;

};

template< log_level severity ,typename...Args >
void logger::print(Args&&...args)
{
    if(severity < _min_log_level){
        return;//Level too low
    }
    std::stringstream log_stream;

    _current_level = severity; // Pass arg to the funct called latter
    _log_line_number++; // Even if no output, increment line number

    /* Build the header by pushing user char 
     * and calling func stored in _header pattern
     */
    for (auto it_header = _header_pattern.begin(); 
            it_header < _header_pattern.end(); ++it_header) 
        log_stream << it_header->first << (this->*(it_header->second))();
  
    print_impl(std::forward<std::stringstream>(log_stream),
               std::move(args)...);
}

template< typename First, typename...Rest >
void logger::print_impl(std::stringstream&& log_stream,
                                      First&& parm1,Rest&&...parm)
{
    log_stream << parm1;
    print_impl(std::forward<std::stringstream>(log_stream),
               std::move(parm)...);
}

