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

#define FLOAT_PRECISION 10

/** 
 * @brief log_policy for the logger
 * @brief log_policy for the logger
 */
class log_policy_interface
{
public:
    virtual ~log_policy_interface() = 0;
    virtual void		open_out_stream(const std::string& name) = 0;
    virtual void		close_out_stream() = 0;
    virtual void		write(const std::string& msg) = 0;
    
};

inline log_policy_interface::~log_policy_interface(){}

/**
 * @brief Implementation which allow to write into a file
 */
class file_log_policy : public log_policy_interface
{
public:
    file_log_policy(){   }
    ~file_log_policy(){}
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
    ~ringfile_log_policy(){}
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
 * @brief Implementation which allow to write to stdout
 */
class stdout_log_policy : public log_policy_interface
{
public:
    stdout_log_policy(){   }
    ~stdout_log_policy(){  }
    void open_out_stream(const std::string& name){(void) name;}
    void close_out_stream() {  }
    void write(const std::string& msg);
};