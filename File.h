#pragma once
#include <sys/file.h>
#include <unistd.h>
#include <string>
#include <iostream>

class File
{
private:
    const std::string _fpath;
    mode_t _perms;
    int _flags;
    int _fd = -1;
    bool _opened = false;
    bool _locked = false;
    int _errno = 0;
    std::string _last_error;

public:
    File(const std::string &file_path, mode_t file_perms = 0600);
    int fopen(int flags, mode_t file_perms = 0600);
    void fclose();
    std::string fread(int bytes = 0);
    bool lock(int flags);
    void unlock();
    int fd();
    bool opened();
    bool locked();
    int error_number();
    std::string last_error();
    void error_clear();
};