#pragma once
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <iostream>

class File
{
private:
    const std::string _fpath;
    mode_t _perms;
    int _fd = -1;
    bool _opened = false;
    int _opened_flags = 0;
    bool _locked = false;
    int _locked_flags = LOCK_UN;
    int _errno = 0;
    std::string _error_message;

public:
    File(const std::string &file_path, mode_t file_perms = 0600);
    int fopen(int flags, mode_t file_perms = 0600);
    void fclose();
    std::string fread(int lock_flags = LOCK_SH | LOCK_UN, int start = 0, int length = 0);
    bool fwrite(int start = 0, int length = 0);
    unsigned long long int fsize();
    bool lock(int flags);
    bool lock_ex(bool lock_nb = false);
    bool lock_sh(bool lock_nb = false);
    void unlock();
    int fd();
    bool is_opened() const;
    bool is_locked() const;
    bool is_locked_ex() const;
    bool is_locket_sh() const;
    bool is_open_readable() const;
    bool is_open_writable() const;
    bool is_file_readable() const;
    bool is_file_writable() const;
    int error_number() const;
    std::string error_message() const;
    void add_error(std::string prefix = std::string());
    void error_clear();
};