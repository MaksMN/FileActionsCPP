#include "File.h"

File::File(const std::string& file_path, mode_t file_perms) : _fpath(file_path), _perms(file_perms)
{
    setPerms(_perms);
}

int File::fopen(int flags)
{
    if (_opened)
        fclose();
    _fd = open(_fpath.c_str(), flags, _perms);
    _opened_flags = flags;
    if (_fd == -1)
    {
        _opened = false;
        add_error("fopen()");
    }
    else
    {
        _opened = true;
    }
    return _fd;
}

int File::fopenR()
{
    if (isReadable())
        return _fd;
    else {
        fopen(O_RDONLY | O_CREAT);
        if (_fd == -1)
        {
            add_error("fopenR()");
            return -1;
        }
    }
}

int File::fopenW()
{
    if (isWritable())
        return _fd;
    else {
        fopen(O_WRONLY | O_CREAT);
        if (_fd == -1)
        {
            add_error("fopenW()");
            return -1;
        }
    }
}

int File::fopenRW()
{
    if (isReadable() && isWritable())
        return _fd;
    else {
        fclose();
        fopen(O_RDWR | O_CREAT);
        if (_fd == -1)
        {
            add_error("fopenRW()");
            return -1;
        }
    }
}

void File::fclose()
{
    if (!_opened)
        return;
    close(_fd);
    _fd = -1;
    _opened = false;
    _opened_flags = 0;
    _locked_flags = LOCK_UN;
}

std::string File::fread(size_t start, size_t length)
{
    // Файл должен быть открыт для чтения
    // Блокировки не обрабатываются
    // Чтобы не иметь проблем - предварительно открывайте файл с флагами для чтения.
    // Файл остается открытым, закрывайте после вызова.
    // В случае сбоев вернет пустую строку    
    std::string result = std::string();
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
        size_t l = sizeof(buffer);
        ssize_t bytesRead = read(_fd, buffer, l);
        if (bytesRead == -1)
        {
            add_error("fread()");
        }
        else
        {
            result = std::string(buffer, length);
        }
    }
    return result;
}

std::string File::freadLock(size_t start, size_t length)
{
    fopenR();
    lock_sh();
    std::string result = fread();
    unlock();
}

ssize_t File::fwrite(const std::string& data, size_t start, size_t length)
{
    // Файл должен быть открыт для записи
    // Блокировки не обрабатываются
    // Чтобы не иметь проблем - предварительно открывайте файл с флагами для записи.
    // Файл остается открытым, закрывайте после вызова.

    // запись с lseek за пределами размера файла создает  "разреженный файл" (sparse file)
    // start должен быть просто больше нуля.
    if (lseek(_fd, start, SEEK_SET) == -1)
    {
        add_error("fwrite()");
    }
    if (length == 0)
    {
        length = data.size();
    }
    auto result = write(_fd, data.c_str(), length);
    if (result == -1)
    {
        add_error("fwrite()");
        return result;
    }
    return result;
}

ssize_t File::fwriteLock(const std::string& data, size_t start, size_t length)
{
    fopenW();
    lock_ex();
    auto result = fwrite(data, start, length);
    unlock();
    return result;
}

bool File::lock(int flags)
{
    // если такая же блокировка уже наложена то ничего не делаем
    // если блокировка с другими флагами - снимаем её
    if (_locked && _locked_flags == flags)
        return true;
    else if (_locked)
        unlock();

    if (flock(_fd, flags) == -1)
    {
        _locked = false;
        _locked_flags = LOCK_UN;
        add_error("lock()");
        return false;
    }
    _locked = flags != LOCK_UN;
    _locked_flags = flags;
    return true;
}

bool File::lock_ex()
{
    return lock(LOCK_EX);
}

bool File::lock_sh()
{
    return lock(LOCK_SH);
}

void File::unlock()
{
    if (flock(_fd, LOCK_UN) == -1)
    {
        add_error("lock()");
    }
    _locked = false;
    _locked_flags = LOCK_UN;
}
