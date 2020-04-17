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
#include <cassert>
#include <thread>
#include <iomanip>
#include <ctime>
#include <sstream>

/*
* Thread function
*/
void logging_daemon( logger* logger )
{
    //Dump the log data if any
    std::unique_lock< std::timed_mutex > writing_lock{logger->_write_mutex,std::defer_lock};
    do{
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
        if( logger->_log_buffer.size() )
        {
            writing_lock.lock();
            for( auto& elem : logger->_log_buffer )
            {
                logger->_policy->write( elem );
            }
            logger->_log_buffer.clear();
            writing_lock.unlock();
        }
    }while( logger->_is_still_running.test_and_set() || logger->_log_buffer.size() );
    logger->_policy->write( "-Terminating the logger daemon!-" );
}



/**
* Implementation for logger
*/

// static func
std::map<std::string, logger*> logger::_logger_list;

logger* logger::get_logger(const std::string& name) {
    std::map<std::string, logger*>::iterator it;
    
    it = _logger_list.find(name);
    if (it != _logger_list.end())
        return it->second;
    else
        return nullptr;
}

// constructor

logger::logger(log_policy_interface* policy,
                const std::string& name,
                const std::string& path):
    _policy(policy), _log_line_number(0), _path(path), _name(name)
   
{
    _min_log_level = log_level::debug;
    _logger_list.insert(std::pair<std::string, logger*>(name, this));
    set_pattern(DEFAULT_PATTERN);
    _date_format = "%d-%m-%Y";
    _time_format = "%H:%M:%S";

    _policy->open_out_stream( _path + _name );

    //Set the running flag and spawn the daemon
    _is_still_running.test_and_set();
    _daemon = std::move( std::thread{ logging_daemon, this } );
}

// destructor

logger::~logger()
{
    std::map<std::string, logger*>::iterator it;

    terminate_logger();
      
    it = _logger_list.find(_name);
    if (it != _logger_list.end())
         _logger_list.erase (it);

    _policy->write( "-Logger activity terminated-" );
    _policy->close_out_stream();
}

void logger::terminate_logger()
{
    //Terminate the daemon activity
    _is_still_running.clear();
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
    std::lock_guard< std::timed_mutex > lock( _write_mutex );
    _log_buffer.push_back(log_stream.str());
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
    std::pair <std::string, std::string (logger::*)()> format_elmt;

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


