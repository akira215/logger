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

#include <string>
#include <sstream>
#include <map>
#include <queue>
#include <vector>

#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <utility>

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
 * @brief LOGGER_DELAY is the sleep time in ms for the logger thead
 */
#define LOGGER_DELAY 10

/**
 * @brief DEFAULT_LOGGER_NAME is the default name is arg is
 * not specified in the constructor
 */
#define DEFAULT_LOGGER_NAME "./logger.log"

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
     *  @param name of the logger. Will be used for the file name
     *  by removing the path
     */ 
    logger(log_policy_interface* policy = 
            (log_policy_interface*) new stdout_log_policy(),
            const std::string& name = DEFAULT_LOGGER_NAME);

    /** @brief logger destructor
     *  @brief kill the daemon associated to the instance
     *  @brief remove the instance from map list
     *  @brief and call the close_out_stream() method of the policy
     */ 
    ~logger();

    /** @brief print the function to be called to log data.
     *  @brief variadics args combine all supported
     *  @brief std::stringstream << operator support
     *  @brief header with user pattern is prepared here
     */ 
    template< typename...Args >
    void print(log_level severity, Args&&...args);

    /** @brief print the function to be called to log data.
     *  @brief Ex. logger->print<log_level::debug>(...)
     *  @brief just here to have the macro LOG_INFO, ...
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

    /** @brief set_default_logger()
     *  @brief set the current logger
     *  as the default logger, which will be
     *  used by lout and lerr macros
     */ 
    void set_default_logger();

   /** @brief get_default_logger()
     * @return return a pointer to 
     * the default logger.
     */ 
    static logger* get_default_logger();

   /** @brief get_logger static()
     *  @param name "filename" of the logger
     *  @return logger* pointer to the logger
     *  @return nullptr if not found
     */ 
    static logger* get_logger(const std::string& name);

    /** @brief loggername_exist()
     *  @param name "name" to be tested
     *  @return true if a logger with the same name exist
     *  @return false otherwise
     */ 
    static bool loggername_exist(const std::string& name);

    /** @brief logger_killall()
     *  @brief destroy all the logger created and still
     *  @brief alive.
     */ 
    static void logger_killall();

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
    /** @brief terminate_logger()
     *  @brief kill the thread
     * it is called by the class destructor
     */ 
    void terminate_logger();

    /** @brief logging_thread()
     *  @brief this thread push the logging queue
     *  to the write policy (ies)
     */ 
    void logging_thread();

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

    /** @brief _print_mutex to protect print access
     * if multi threaded app call the logger.
     */
    std::mutex _print_mutex;

    /** @brief The first logger to be registred or
     * the one set by set_default_logger();
     * default_logger will be used when calling
     * lcout, lcerr and lclog macros
     */
    static logger* _default_logger;

    typedef std::pair<std::string, std::string (logger::*)()> headerElement;
    /** @brief _header_pattern store the user pattern
     *  @brief it is a vector of pair 
     *  @brief pair first are user char (separator, decorator,...)
     *  @brief pair second are pointer to member function that will return the info
     *  @brief first could be empty string and second could be a function
     *  @brief that will return empty string (logger::get_empty_string)
     */
    std::vector<headerElement> _header_pattern;

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
    std::mutex _write_mutex;

    /** @brief _data_available is notify by the print method of this class
     * it will wake up the logging thread that will process log datas.
     */
    std::condition_variable _data_available;

    /** @brief _log_buffer is the media between the current user 
     *  @brief input operations and the daemon thread that perform
     *  @brief output operations
     */
    std::queue< std::string > _log_buffer;

    /** @brief _policy pointer to the policy class which shall
     *  @brief inherit from log_policy_interface
     */
    log_policy_interface* _policy;

    /** @brief _is_running flag for the running daemon
     */
    std::atomic<bool> _is_running;

    /** @brief _log_line_number is incremented each time
     *  @brief logger::print method is called, even if it is
     *  @brief not printed in the user pattern
     */
    unsigned int _log_line_number;

    /** @brief filename of log file
     *  @brief i.e. path + file
     */ 
    std::string _filename;

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
    print(severity, std::move(args)...);
}

template< typename...Args >
void logger::print(log_level severity, Args&&...args)
{
    if(severity < _min_log_level){
        return;//Level too low
    }
    std::stringstream log_stream;

    // Acquire the mutex to protect header mixing in case of multithreaded app
    std::scoped_lock<std::mutex> guard(_print_mutex);

    _current_level = severity; // Pass arg to the header built funct
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


/** @brief Macro to log data direclty to 
 * _default_logger. Include this header and
 * just call lcout << "something to log" << std:endl
 */
#define lclog log_stream(log_level::debug)
#define lcout log_stream(log_level::info)
#define lcerr log_stream(log_level::error)

/** @brief Class log_stream is a logging stream that send
 * the stream to logger each time std::endl is pushed
 * to this stream, and when it is destructed. If std::endl
 * is not present at the end of the stream <<operator, object
 * is destroy and the logOutput is called, so that a line is
 * displayed in the the logs.
 * To be used with dedicated macros lclog, lcout an lcerr
 */
class log_stream: public std::ostream
{
    // Stream buffer that send data to logger class each time std::endl
    // is triggered
    class logStreamBuf: public std::stringbuf
    {
        public:
            logStreamBuf(log_level loglevel)
                :_loglevel(loglevel)
            { }
            ~logStreamBuf() {
                if (pbase() != pptr()) {
                    logOutput();
                }
            }

            // When we sync the stream with the output. 
            // 1) print on the logger (thread safe)
            // 2) Reset the buffer
            virtual int sync() {
                logOutput();
                return 0;
            }
            void logOutput() {
                // Called by destructor.
                // destructor can not call virtual methods.
                logger::get_default_logger()->print(_loglevel,str());
                str("");
            }
        private:
            log_level _loglevel;
    };

    // log_stream just uses the logStreamBuf
    logStreamBuf buffer;
    public:
        log_stream(log_level loglevel)
            :std::ostream(&buffer), buffer(loglevel)
        {
        }
};