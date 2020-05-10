/*
 * logger.cpp
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

#include "logger.hpp"
#include <iomanip>
#include <chrono>

/*
* Thread functions
*/
void logger::logging_thread()
{
    std::unique_lock< std::mutex > writing_lock(_write_mutex ,std::defer_lock );
    std::string log_line;
    do{
        writing_lock.lock();  // shall be locked before wait call
        _data_available.wait_for(writing_lock,
                std::chrono::milliseconds(LOGGER_DELAY),
               [this]{ return (!_log_buffer.empty() || !_is_running.load()); });

        if( !_log_buffer.empty() ) {
            
            log_line = std::move(_log_buffer.front());
            _log_buffer.pop();
            writing_lock.unlock();

            _policy->write( log_line );
            log_line.clear();
        } else
            writing_lock.unlock();

    }while( _is_running.load() || !_log_buffer.empty());
    //Dump the log data if any before shutting down
}

/**
* Implementation for logger
*/

// static func
logger* logger::_default_logger = nullptr;
std::map<std::string, logger*> logger::_logger_list;

logger* logger::get_default_logger()
{
    if (_default_logger)
        return _default_logger;
    
    // if no default logger, map is empty
    // just build a defaut logger, it will be set
    // as default_logger in the constructor
    logger* new_logger = new logger();

    return new_logger;
}

logger* logger::get_logger(const std::string& name) {
    std::map<std::string, logger*>::iterator it;
    
    it = _logger_list.find(name);
    if (it != _logger_list.end())
        return it->second;

    return nullptr;
}

bool logger::loggername_exist(const std::string& name) {
     // Check if the same name exist
    for (auto const& lo : _logger_list)
    {
        if(lo.first == name)  // string (key)
            return true;
    }
    return false;
}

void logger::logger_killall() {
    // Copy the map because items will be 
    // deleted by the destructor
     std::map<std::string, logger*> list_cpy(_logger_list);
    for (auto const& lo : list_cpy)
        delete (lo.second);
}

// constructor

logger::logger(log_policy_interface* policy,
        const std::string& name): _policy(policy),
        _log_line_number(0), _filename(name)
{
    //remove the path for the logger name
    _name = _filename.substr(_filename.find_last_of("/\\") + 1);
    
    _min_log_level = log_level::debug;

    if (_logger_list.empty())
        set_default_logger();

    _logger_list.insert(std::pair<std::string, logger*>(_filename, this));
    set_pattern(DEFAULT_PATTERN);
    _date_format = "%d-%m-%Y";
    _time_format = "%H:%M:%S";

    _policy->open_out_stream(_filename);
    // avoid logging start here because pattern is not set

    //Set the running flag and spawn the daemon
    _is_running.store(true);
    _daemon = std::thread( &logger::logging_thread, this );
}

// destructor

logger::~logger()
{
    std::map<std::string, logger*>::iterator it;

    terminate_logger();

    // if the default_logger is deleted, reaffect to the 1st
    if (_default_logger == this)
        if(_logger_list.size() > 1)
            _default_logger = _logger_list.begin()->second;
        else
            _default_logger = nullptr;

    it = _logger_list.find(_filename);
    if (it != _logger_list.end())
         _logger_list.erase (it);

    _policy->close_out_stream();
    delete _policy;
}

void logger::set_default_logger()
{
    _default_logger = this;
}

void logger::terminate_logger()
{
    //Terminate the daemon activity
    LOG_INFO( "..............Logger activity terminated.............." );
    _is_running.store(false);
    _daemon.join();
}

void logger::set_thread_name(const std::string& name)
{
    _thread_name[ std::this_thread::get_id() ] = name;
}

void logger::set_min_log_level(log_level new_level)
{
    _min_log_level = new_level;
}

void logger::print_impl(std::stringstream&& log_stream)
{
    if(!log_stream.str().empty()) {
        if(log_stream.str().back() != '\n')
            log_stream << std::endl;

        std::scoped_lock<std::mutex> lock(_write_mutex);
        std::string todel(log_stream.str());
        _log_buffer.push(log_stream.str());
    }
    _data_available.notify_one();
}

std::string logger::get_line_number() {
    return std::to_string(_log_line_number);
}

std::string logger::get_thread_name() {
    return _thread_name[ std::this_thread::get_id() ];
}

std::string logger::get_log_level() {
    switch(_current_level)
    {
    case log_level::debug:
        return "DEBUG";
    case log_level::info:
        return "INFO";
    case log_level::notice:
        return "NOTICE";
    case log_level::warning:
        return "WARNING";
    case log_level::error:
        return "ERROR";
    case log_level::critical:
        return "CRITICAL";
    };
    return std::string();
}

std::string logger::get_date() {
    time_t t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;

    oss << std::put_time(&tm, _date_format.c_str());
    return oss.str();
}

std::string logger::get_time() {
    time_t t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;

    oss << std::put_time(&tm, _time_format.c_str());
    return oss.str();
}

std::string logger::get_logger_name() {
    return _name;
}

std::string logger::get_empty_string() {
    return std::string();
}

void logger::set_pattern(const std::string &pattern) {
    std::string userchar;
    headerElement format_elmt;

    // Clear previous pattern
    _header_pattern.clear();

    for (auto it = pattern.begin(); it < pattern.end(); ++it) {
        
        if (*it != '%')
            userchar.push_back(*it);
        else {
            // push the string in the map and reset the userchar
            format_elmt.first = userchar;
            userchar.clear();
            it++; // check the char after '%'
            switch (*it) {
            case 'd': //date
                format_elmt.second = (&logger::get_date);
                if(*(++it) == FORMAT_DELIMITER) {
                    _date_format.clear();
                    for(++it;*it != FORMAT_DELIMITER; ++it)
                        _date_format.push_back(*it);
                    it++;
                }
                it--;
                break;
            case 'i': //index = line number
                format_elmt.second = (&logger::get_line_number);
                break;
            case 'l': //Log level
                format_elmt.second = (&logger::get_log_level);
                break;
            case 'n': //logger name
                format_elmt.second = (&logger::get_logger_name);
                break;
            case 't': //time
                format_elmt.second = (&logger::get_time);
                if(*(++it) == FORMAT_DELIMITER) {
                    _time_format.clear();
                    for(++it;*it != FORMAT_DELIMITER; ++it)
                        _time_format.push_back(*it);
                    it++;
                }
                it--;
                break;
            case 'x': //thread name
                format_elmt.second = (&logger::get_thread_name);
                break;
            default:
                format_elmt.second = (&logger::get_empty_string);
            }
            _header_pattern.push_back(format_elmt);
        }
        
    }
    // fill the last user string if any
    if(!userchar.empty()) {
        format_elmt.first = userchar;
        userchar.clear();
        format_elmt.second = (&logger::get_empty_string); // Nothing after
        _header_pattern.push_back(format_elmt);
    }

}

void logger::set_date_format(const std::string &fmt){
    _date_format = fmt;
 
}

void logger::set_time_format(const std::string &fmt){
    _time_format = fmt;
}