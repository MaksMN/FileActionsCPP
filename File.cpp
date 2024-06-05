#include "File.h"

File::File(const std::string &file_path, mode_t file_perms) : _fpath(file_path), _perms(file_perms) {}

int File::fopen(int flags, mode_t file_perms)
{
    error_clear();
    if (_opened)
        return _fd;
    _fd = open(_fpath.c_str(), flags, file_perms);
    if (_fd == -1)
        _opened = false;
    return _fd;
}

void File::fclose()
{
    error_clear();
    if (!_opened)
        return;

    if (_locked)
    {
        flock(_fd, LOCK_UN);
    }
    close(_fd);
}

bool File::lock(int flags)
{
    error_clear();
    if (!_opened)
    {
        _locked = false;
        _errno = 1;
        _last_error = "Lock failed. File not opened.";
        return _locked;
    }
    if (_locked)
        return _locked;

    flags &= ~LOCK_UN;

    if ((flags & (LOCK_SH | LOCK_EX)) == 0)
    {
        _errno = 2;
        _last_error = "Lock failed. Flags set incorrectly.";
        _locked = false;
        return _locked;
    }

    int l = flock(_fd, flags);
    if (l = -1)
        _locked = false;
    else
        _locked = true;
    return _locked;
}

void File::unlock()
{
    if (!_locked || !_opened)
        return;
    flock(_fd, LOCK_UN);
    _locked = false;
}

int File::fd()
{
    return _fd;
}

bool File::opened()
{
    return _opened;
}

bool File::locked()
{
    return _locked;
}

int File::error_number()
{
    return _errno;
}

std::string File::last_error()
{
    return _last_error;
}

void File::error_clear()
{
    _errno = 0;
    _last_error = std::string();
}
