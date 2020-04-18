#include "log_policy.hpp"
#include <cassert>
#include <filesystem>

namespace fs = std::filesystem;

/**
* -----------------Implementation for file_log_policy-------------------------
*/
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