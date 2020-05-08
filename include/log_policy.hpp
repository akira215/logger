#pragma once
/*
 * log_policy.hpp
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

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <ctime>

#define FLOAT_PRECISION 10

/** 
 * @brief log_policy for the logger
 * @brief log_policy for the logger
 */
class log_policy_interface
{
public:
    virtual ~log_policy_interface() = 0;
    virtual void open_out_stream(const std::string& name) = 0;
    virtual void close_out_stream() = 0;
    virtual void write(const std::string& msg) = 0;
};

inline log_policy_interface::~log_policy_interface(){}

/**
 * @brief Implementation which allow to write into a file
 */
class file_log_policy : public log_policy_interface
{
public:
    file_log_policy(){   }
    ~file_log_policy();
    void open_out_stream(const std::string& name);
    void close_out_stream();
    void write(const std::string& msg);
private:
    std::ofstream _out_stream;
};

/**
 * @brief Implementation to write to a file, limited by size
 */
class ringfile_log_policy : public log_policy_interface
{
public:
    /** @param max_size max size of the file in byte
     *  @param defaut value is 1MB
    */
    ringfile_log_policy(uintmax_t max_size = 1048576, 
                        uint16_t max_file_count = 2);
    ~ringfile_log_policy();
    void open_out_stream(const std::string& name);
    void close_out_stream();
    void write(const std::string& msg);
private:

    /** @brief get_next_filename
     *  @return the filename (without base path)
     */
    std::string get_next_filename();

    /** @brief rotate_file
     *  @brief rotate the current file and reset the size
     *  counter. Files are always smaller than _max_size
     *  we don't cut messages.
     */
    void rotate_file();

    /** @brief _out_stream :
     *  @brief the ofstream object
     */
    std::ofstream _out_stream;

    /** @brief _current_size :
     *  @brief current size of the log file
     */
    uintmax_t _current_size;
    
    /** @brief _max_size :
     *  @brief fmax file size in byte
     */
    uintmax_t _max_size;

    /** @brief _current_file_index :
     *  @brief file index we are writing int
     */
    uint16_t _current_file_index;

    /** @brief _max_index :
     *  @brief max file file count -1
     */
    uint16_t _max_index;

    /** @brief _name : name without numeric extension
     */
    std::string _name;

    /** @brief _path : path of the file
     */
    std::string _path;

};

/**
 * @brief Implementation to write to a file, rotating everyday
 */
class dailyfile_log_policy : public log_policy_interface
{
public:
    /** @param decimal_rotate_hour hour when log file will be rotated
     *          Value in decimal i.e. 12.25 represent 12h15'
     *  @param max_file_count will be evaluated as days. 
     *          Older files will be deleted
     * @param fmt format of date that will be appened in the filename.
    */
    dailyfile_log_policy(float decimal_rotate_hour = 0.0, 
                        uint16_t max_file_count = 30, 
                        const std::string &fmt = "%Y-%m-%d");
    ~dailyfile_log_policy();
    void open_out_stream(const std::string& name);
    void close_out_stream();
    void write(const std::string& msg);
private:

    /** @brief get_next_filename
     *  @brief based on the _next_rotate_time member
     *  @brief should be called after is_rotation_required(
     *  @brief the convention is that log day finished at 12:00
     *  @brief i.e. if deadline is set at 01:00, the extension
     *  @brief will be the day before from 00:00 to 01:00
     *  @return the filename (without base path)
     */
    std::string get_filename();

    /** @brief is_rotation_required()
     *  @brief check if the time exceed the deadline
     *  @brief if so, it will shift the member _next_rotate_time
     *  @brief to one day ahead
     *  @return true if rotation is required, false otherwise
     */
    bool is_rotation_required();

    /** @brief rotate_file
     *  @brief rotate the current file and reset the size
     *  counter. Files are always smaller than _max_size
     *  we don't cut messages.
     */
    void rotate_file();

    /** @brief delete_old_files()
     *  @brief Check the extension name regarding the registred
     *  @brief date format, and delete all file that are older than
     *  @brief _max_file_count days. Ignore files that are not matching
     *  @brief the pattern
     */
    void delete_old_files();

    /** @brief _out_stream :
     *  @brief the ofstream object
     */
    std::ofstream _out_stream;

    /** @brief _next_rotate_time :
     *  @brief next time point when we need
     *  @brief to rotate file (set by is_rotation_required)
     */
    time_t _next_rotate_time;
                        
    /** @brief _max_file_count :
     *  @brief max file kept on archive. Older ones will be deleted
     */
    uint16_t _max_file_count;

    /** @brief _date_format :
     *  @brief Date format that will be appened
     *  @brief to the log filename
     */
    std::string _date_format;

    /** @brief _name : name without numeric extension
     */
    std::string _name;

    /** @brief _path : path of the file
     */
    std::string _path;
};

/**
 * @brief Implementation log to stdout
 */
class stdout_log_policy : public log_policy_interface
{
public:
    stdout_log_policy() {  }
    ~stdout_log_policy() {  }
    void open_out_stream(const std::string& name){(void) name;}
    void close_out_stream() {  }
    void write(const std::string& msg);
};

/** 
 * @brief spread_log_policy 
 * @brief just spread messages to other loggers registred during construction
 * @brief take care as the name is the same for all policies, filename
 * @brief will be the same if nothing is done (ringfile <> file <> stdout)
 */
class spread_log_policy : public log_policy_interface
{
public:
    template<typename... Args>
    spread_log_policy(Args... args);
    
    ~spread_log_policy();
    void open_out_stream(const std::string& name);
    void close_out_stream();
    void write(const std::string& msg);
private:
    /** @brief initailize() is
     *  @brief the recursive variadic method
     *  @brief it fill the vector with the required policies
     */
    void initialize() { }

    template<typename... Args>
    void initialize(log_policy_interface* policy, Args... args);

    /** @brief _policy_list
     *  @brief is the vector storing pointers of all the 
     *  @brief policies we want to spread on
     */
    std::vector<log_policy_interface*> _policy_list;
};

template<typename... Args>
spread_log_policy::spread_log_policy(Args... args) {
    /* Don't call recursive contructor, it will create
     * several object....
     */
    initialize(args...);
}

template<typename... Args>
void spread_log_policy::initialize(log_policy_interface* policy, Args... args) {
    _policy_list.push_back(policy);
    initialize(args...);
}
