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
    std::ofstream out_stream;
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