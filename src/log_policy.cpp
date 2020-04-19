#include "log_policy.hpp"
#include <cassert>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <stdio.h>

namespace fs = std::filesystem;

/**
* -----------------Implementation for file_log_policy-------------------------
*/

file_log_policy::~file_log_policy() {
    close_out_stream();
}

void file_log_policy::open_out_stream(const std::string& name) {

    size_t found;
    std::string path;

    found = name.find_last_of("/\\");
    path = name.substr(0,found);

    /* Create dir if it is not existing */
    if (!fs::is_directory(path) || !fs::exists(path)) {
        fs::create_directory(path); // create folder
    }

    _out_stream.open(name.c_str(), std::ios_base::binary |
                        std::ios_base::out | std::ofstream::app);
    assert( _out_stream.is_open() == true );
    _out_stream.precision(FLOAT_PRECISION);
}

void file_log_policy::close_out_stream() {
    if( _out_stream )
    {
        _out_stream.close();
    }
}

void file_log_policy::write(const std::string& msg) {
    _out_stream<<msg<<std::endl<<std::flush;
}

/**
* ---------------Implementation for ringfile_log_policy-----------------------
*/

ringfile_log_policy::ringfile_log_policy(uintmax_t max_size, 
                        uint16_t max_file_count): 
                        _max_size(max_size),
                        _current_file_index(0) { 
    if (max_file_count > 1)
        _max_index = max_file_count -1;
    else
        _max_index = 1;

}
ringfile_log_policy::~ringfile_log_policy() {
    close_out_stream();
}
void ringfile_log_policy::open_out_stream(const std::string& name) {
    size_t found;
    fs::file_time_type last_time;
    long last_index = -1;
    std::string next_filename;
    uintmax_t filesize;

    /* fill the class attribute */
    found = name.find_last_of("/\\");
    _path = name.substr(0,found);
    _name = name.substr(found+1);

    /* Create dir if it is not existing */
    if (!fs::is_directory(_path) || !fs::exists(_path)) {
        fs::create_directory(_path); // create folder
    }

    /* Find the last modified file */
    for(const auto& p : 
        fs::directory_iterator(_path)) {

        if(p.path().stem() == _name)  { // ignore the .log file
            int num = -1; // tag the file without extension
            std::string ext = p.path().extension();
            ext = ext.substr(1); // removing '.'
            try {
                num = std::stoi(ext);
            } catch (...) { }
            if(num > -1) {
                auto current_time = fs::last_write_time(p.path());
                if (current_time > last_time){
                    last_index = num;
                    last_time = current_time;
                }
            }
        }
    }

    /* last_index is either the last modified file either -1 
     * get the file size */
    _current_file_index = last_index < 0 ? 0: last_index;
    next_filename = _path + "/" + _name + "." + 
                            std::to_string(_current_file_index);

    if (last_index < 0) { //need to create the file
        _out_stream.open(next_filename.c_str(), std::ios_base::binary |
                        std::ios_base::out | std::ofstream::trunc);
        if( _out_stream ) {
            _out_stream.close();
        }
    }

    filesize = fs::file_size(next_filename);

     /* Find the correct file to be written */
    if (filesize < _max_size) { // Open existing and append
        _current_size = filesize;
        _out_stream.open(next_filename.c_str(), std::ios_base::binary |
                        std::ios_base::out | std::ofstream::app);
    } else { // Open and truncate
        _current_size = 0;
        next_filename = _path + "/" + get_next_filename();
        _out_stream.open(next_filename.c_str(), std::ios_base::binary |
                        std::ios_base::out | std::ofstream::trunc);
    }
   
    assert( _out_stream.is_open() == true );
    _out_stream.precision(FLOAT_PRECISION);
}

std::string ringfile_log_policy::get_next_filename() {

    if (_current_file_index >= _max_index)
        _current_file_index = 0;
    else
        _current_file_index++;

    return _name + "." + std::to_string( _current_file_index);
}

void ringfile_log_policy::rotate_file() {
    if( _out_stream )
        _out_stream.close();

    std::string next_filename = _path + "/" + get_next_filename();

    _out_stream.open(next_filename.c_str(), std::ios_base::binary |
                        std::ios_base::out | std::ofstream::trunc);
    assert( _out_stream.is_open() == true );
    _current_size = 0;
    _out_stream.precision(FLOAT_PRECISION);
}

void ringfile_log_policy::close_out_stream() {
    if( _out_stream ) {
        _out_stream.close();
    }
}

void ringfile_log_policy::write(const std::string& msg) {
    if(_current_size + msg.length() > _max_size)
        rotate_file();

    _current_size += msg.length();

    _out_stream<<msg<<std::endl<<std::flush;
}

/**
* -------------------Implementation for stdout_log_policy---------------------
*/
void stdout_log_policy::write(const std::string& msg) {
    std::cout<<msg<<std::endl<<std::flush;
}

/**
* ---------------Implementation for dailyfile_log_policy-----------------------
*/

dailyfile_log_policy::dailyfile_log_policy(float decimal_rotate_hour, 
                        uint16_t max_file_count, const std::string &fmt): 
                        _date_format(fmt) {
    if (max_file_count > 1)
        _max_file_count = max_file_count;
    else
        _max_file_count = 2;
    
    /* Set the correct swap hour, but not the correct day ! 
     * the correct day is set up by is_rotation_required
     */
    time_t t = std::time(nullptr);      // Get time now
    std::tm t_rot = *std::localtime(&t);   // When we need to rotate file

    while(decimal_rotate_hour > 24.0)   // set the arg to 0.0..24.0 range
        decimal_rotate_hour -= 24;

    // Set the rotating hour
    t_rot.tm_hour = (unsigned int) decimal_rotate_hour;
    t_rot.tm_min =(unsigned int) (60 * (decimal_rotate_hour - 
                                    (unsigned int)decimal_rotate_hour));
    t_rot.tm_sec = 0;

    time_t epoch = mktime(&t_rot);
    
    if (t > epoch)   // add one day to the rotate target time
        epoch += (60*60*24); 
       
    _next_rotate_time = epoch;
}

dailyfile_log_policy::~dailyfile_log_policy() {
    close_out_stream();
}

void dailyfile_log_policy::open_out_stream(const std::string& name) {
    size_t found;

    /* fill the class attribute */
    found = name.find_last_of("/\\");
    _path = name.substr(0,found);
    _name = name.substr(found+1);

    /* Create dir if it is not existing */
    if (!fs::is_directory(_path) || !fs::exists(_path)) {
        fs::create_directory(_path); // create folder
    }

    /* align the _next_rotate_time to get the correct filename */
    is_rotation_required();

    rotate_file();  // open the stream
}

bool dailyfile_log_policy::is_rotation_required() {
    time_t t = std::time(nullptr);

    if (t > _next_rotate_time)  {
        _next_rotate_time += (60*60*24);
        return true;
    }

    return false;
}

std::string dailyfile_log_policy::get_filename() {
    std::tm t_ext = *std::localtime(&_next_rotate_time);
    time_t ext_time = _next_rotate_time;
    if(t_ext.tm_hour < 12) // The name of the file is switching at 12h00
        ext_time -= 60 * 60 * 24;

    t_ext = *std::localtime(&ext_time);
    std::stringstream ss;

    ss << std::put_time(&t_ext, _date_format.c_str());

    return _path + "/" + _name + "." + ss.str();
}

void dailyfile_log_policy::close_out_stream() {
    if( _out_stream ) {
        _out_stream.close();
    }
}

void dailyfile_log_policy::rotate_file() {
    if( _out_stream ) {
        _out_stream.close();
    }
    
    /* get_filename return the correct file name if is_rotation_required
     * has been called before
     */
    std::string filename = get_filename();

    _out_stream.open(filename.c_str(), std::ios_base::binary |
                        std::ios_base::out | std::ofstream::app);
    assert( _out_stream.is_open() == true );
    _out_stream.precision(FLOAT_PRECISION);

    delete_old_files();
}

void dailyfile_log_policy::delete_old_files() {
    /* Find the files olders than _max_file_count days */
    time_t t_file ;
    time_t t_now = std::time(nullptr);      // Get time now
    std::tm tm_file = {};
    std::stringstream ss;

    for(const auto& p : fs::directory_iterator(_path)) {

        if(p.path().stem() == _name)  { // ignore the .log file    
            std::string ext = p.path().extension();
            ext = ext.substr(1); // removing '.'
            std::stringstream().swap(ss); // clear stream state
            ss << ext;
            ss >> std::get_time(&tm_file, _date_format.c_str());

            if (((ss.rdstate() & std::ifstream::failbit ) == 0) &&
                ((ss.rdstate() & std::ifstream::badbit ) == 0)) {

                t_file = mktime(&tm_file);
                int diff_days = (t_now - t_file) / (60 * 60 * 24);
                if (diff_days > _max_file_count)
                    remove(p);
            }

        }
    }
}

void dailyfile_log_policy::write(const std::string& msg) {
    
    if(is_rotation_required())
        rotate_file();

    _out_stream<<msg<<std::endl<<std::flush;
}

/**
* -----------------Implementation for spread_log_policy-------------------------
*/

spread_log_policy::~spread_log_policy() {
    //delete the log_policy_interface objects
    for(auto it=_policy_list.begin(); it!=_policy_list.end(); ++it) {
	    delete *it;
        *it = nullptr;
    }
    _policy_list.clear();
}

void spread_log_policy::open_out_stream(const std::string& name) {
    for(auto it=_policy_list.begin(); it!=_policy_list.end(); ++it) {
	    (*it)->open_out_stream(name);
    }
}

void spread_log_policy::close_out_stream() {
    for(auto it=_policy_list.begin(); it!=_policy_list.end(); ++it) {
	    (*it)->close_out_stream();
    }
}

void spread_log_policy::write(const std::string& msg) {
    for(auto it=_policy_list.begin(); it!=_policy_list.end(); ++it) {
	    (*it)->write(msg);
    }
}




