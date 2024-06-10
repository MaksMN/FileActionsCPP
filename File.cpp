#include "File.h"
#include <cstring>

File::File(const std::string &file_path, mode_t file_perms) : _fpath(file_path), _perms(file_perms) {}

int File::fopen(int flags, mode_t file_perms)
{
    if (_opened)
        fclose();
    _fd = open(_fpath.c_str(), flags, file_perms);
    _opened_flags = flags;
    if (_fd == -1)
    {
        _opened = false;
        add_error("fopen()");
    }
    else
        _opened = true;
    return _fd;
}

void File::fclose()
{
    if (!_opened)
        return;
    unlock();
    close(_fd);
    _fd = -1;
    _opened = false;
    _opened_flags = 0;
}

std::string File::fread(int lock_flags, int start, int length)
{
    // Если файл открыт без флагов для чтения - переоткрываем с O_RDONLY и закрываем насовсем.
    // В этом случае снимаются все блокировки.
    // Чтобы не иметь проблем - открывайте файл с флагами для чтения. Иначе эта функция прервет все другие операции.

    // если файл не открыт, то закроем его после выполнения операции.

    // если файл открыт с флагами для чтения - оставляем его открытым.

    // если присутствуют флаги блокировки - выключить текущую блокировку и включить новую

    // если присутствует флаг LOCK_UN - убрать блокировку при завершении

    bool reopen = !_opened || (_opened && (_opened_flags & (O_RDONLY | O_RDWR) == 0));
    if (reopen)
    {
        fopen(O_RDWR | O_CREAT, _perms);
        if (_fd == -1)
        {
            add_error("fread()");
            return std::string();
        }
    }
    // Если на файл установлена блокировка LOCK_EX - снимаем блокировку насовсем.
    if (_locked && (_locked_flags & LOCK_EX))
        unlock();

    // блокируем файл если указаны флаги
    if (lock_flags & LOCK_SH)
    {
        if (_locked)
            unlock();
        lock_sh(lock_flags & LOCK_NB);
    }
    auto result = std::string();
    const auto file_size = fsize();
    if (file_size > 0)
    {
        if (start < 0)
            start = 0;
        if (start > file_size)
            start = file_size - 1;
        if ((start + length > file_size) || length == 0)
            length = file_size - start;
        if (start > 0 && start < file_size)
        {
            if (lseek(_fd, start, SEEK_SET) == -1)
            {
                add_error("fread()");
            }
        }
        char buffer[length];
        ssize_t bytesRead = read(_fd, buffer, sizeof(buffer));
        if (bytesRead == -1)
        {
            add_error("fread()");
        }
        else
        {
            result = buffer;
        }
    }

    // снимаем блокировку если указано
    if (lock_flags & LOCK_UN)
        unlock();
    if (reopen)
        fclose();

    return result;
}

unsigned long long int File::fsize()
{
    bool close = false; // если файл не открыт - закрыть после открытия.
    unsigned long long int result = 0;
    if (!_opened)
    {
        close = true;
        fopen(O_RDONLY | O_CREAT, _perms);
        if (_fd == -1)
        {
            add_error("fsize()");
            return 0;
        }
    }
    struct stat fileInfo;

    if (fstat(_fd, &fileInfo) == -1)
    {
        add_error("fsize()");
        result = 0;
    }
    else
    {
        result = (unsigned long long int)fileInfo.st_size;
    }
    if (close)
        fclose();
    return result;
}

bool File::lock(int flags)
{
    unlock();
    bool reopen = !_opened || (_opened && (_opened_flags & (O_WRONLY | O_RDWR) == 0));
    if (reopen)
    {
        fopen(O_RDWR | O_CREAT, _perms);
        if (_fd == -1)
        {
            add_error("lock()");
            return false;
        }
    }

    if (flock(_fd, flags) == -1)
    {
        add_error("lock()");
        return false;
    }
    _locked = true;
    _locked_flags = flags;
    return true;
}

bool File::lock_ex(bool lock_nb)
{
    int flags = lock_nb ? LOCK_EX | LOCK_NB : LOCK_EX;
    return lock(flags);
}

bool File::lock_sh(bool lock_nb)
{
    int flags = lock_nb ? LOCK_SH | LOCK_NB : LOCK_SH;
    return lock(flags);
}

void File::unlock()
{
    if (!_locked || !_opened)
        return;
    flock(_fd, LOCK_UN);
    _locked = false;
    _locked_flags = LOCK_UN;
}

int File::fd()
{
    return _fd;
}

bool File::is_opened() const
{
    return _opened;
}

bool File::is_locked() const
{
    return _locked;
}

bool File::is_locked_ex() const
{
    return _locked_flags & LOCK_EX;
}

bool File::is_locket_sh() const
{
    return _locked_flags & LOCK_SH;
}

bool File::is_open_readable() const
{
    return _opened && ((_opened_flags & O_WRONLY) == 0 || _opened_flags & (O_RDWR));
}

bool File::is_open_writable() const
{
    return _opened && ((_opened_flags & (O_WRONLY | O_RDWR)) > 0);
}

bool File::is_file_readable() const
{
    return false;
}

bool File::is_file_writable() const
{
    return false;
}

int File::error_number() const
{
    return _errno;
}

std::string File::error_message() const
{
    return _error_message;
}

void File::add_error(std::string prefix)
{
    _errno = errno;
    if (!prefix.empty())
        prefix += ": ";
    _error_message += prefix + "(" + std::to_string(errno) + ") " + std::strerror(errno) + "\n";
}

void File::error_clear()
{
    _errno = 0;
    _error_message = std::string();
}
