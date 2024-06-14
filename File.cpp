#include "File.h"

File::File(const std::string& file_path, mode_t file_perms) : _fpath(file_path), _perms(file_perms)
{
    setPerms(_perms);
}

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
    {
        _opened = true;
        _perms = file_perms;
    }
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

std::string File::fread(size_t start, size_t length, int lock_flags)
{
    // Если файл открыт без флагов для чтения - переоткрываем с O_RDONLY и закрываем насовсем.
    // В этом случае снимаются все блокировки.
    // Чтобы не иметь проблем - открывайте файл с флагами для чтения. Иначе эта функция прервет все другие операции.
    // Если файл не открыт, то закроем его после выполнения операции.
    // Если файл открыт с флагами для чтения - оставляем его открытым.
    // Если у заранее открытого файла присутствуют флаги блокировки - разблокировать текущую блокировку и включить новую lock_flags.
    // Если присутствует флаг LOCK_UN - убрать блокировку при завершении.

    bool reopen = !is_readable(); // если открыт не для чтения или не открыт
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

    // снимаем блокировку если указано
    if (lock_flags & LOCK_UN)
        unlock();
    if (reopen)
        fclose();

    return result;
}

std::string File::fread_lock(size_t start, size_t length)
{
    return fread(start, length, LOCK_SH | LOCK_UN);
}

ssize_t File::fwrite(const std::string& data, size_t start, size_t length, int lock_flags)
{
    // Если файл открыт без флагов для записи - переоткрываем с O_WRONLY и закрываем насовсем.
    // В этом случае снимаются все блокировки.
    // Чтобы не иметь проблем - открывайте файл с флагами для записи. Иначе эта функция прервет все другие операции.
    // Если файл не открыт, то закроем его после выполнения операции.
    // Если файл открыт с флагами для записи - оставляем его открытым.
    // Если у заранее открытого файла присутствуют флаги блокировки - разблокировать текущую блокировку и включить новую lock_flags.
    // Если присутствует флаг LOCK_UN - убрать блокировку при завершении.
    bool reopen = !is_writable(); // если открыт не для записи или не открыт
    if (reopen)
    {
        fopen(O_RDWR | O_CREAT, _perms);
        if (_fd == -1)
        {
            add_error("fwrite()");
            return -1;
        }
    }

    // Если на файл установлена блокировка LOCK_SH - снимаем блокировку насовсем.
    if (_locked && (_locked_flags & LOCK_SH))
        unlock();

    // блокируем файл если указаны флаги
    if (lock_flags & LOCK_EX)
    {
        if (_locked)
            unlock();
        lock_ex(lock_flags & LOCK_NB);
    }

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
    return result == data.size();
}

ssize_t File::fwrite_lock(const std::string& data, size_t start, size_t length)
{
    return fwrite(data, start, length, LOCK_EX | LOCK_UN);
}

void File::setGroup(gid_t group_id)
{
    struct stat fileInfo;
    if (!fileExists())
    {
        fopen(O_RDONLY | O_CREAT, _perms);
        fclose();
    }
    if (fstat(_fd, &fileInfo) != 0)
    {
        add_error("setGroup()");
    }
    auto user_id = fileInfo.st_uid;
    if (chown(_fpath.c_str(), user_id, group_id) != 0)
    {
        add_error("setGroup()");
    }
}

void File::setOwner(uid_t user_id)
{
    struct stat fileInfo;
    if (!fileExists())
    {
        fopen(O_RDONLY | O_CREAT, _perms);
        fclose();
    }
    if (fstat(_fd, &fileInfo) != 0)
    {
        add_error("setGroup()");
    }
    auto group_id = fileInfo.st_gid;
    if (chown(_fpath.c_str(), user_id, group_id) != 0)
    {
        add_error("setGroup()");
    }
}

void File::setPerms(mode_t perms)
{
    if (!fileExists())
    {
        fopen(O_RDONLY | O_CREAT, perms);
        fclose();
        return;
    }
    if (chmod(_fpath.c_str(), perms) == -1)
    {
        add_error("setPerms()");
        return;
    }
    _perms = perms;
}

void File::setPerms(const std::string& perms)
{
    mode_t p = std::stoi(perms, nullptr, 8);
    setPerms(p);
}

void File::UserToReader()
{
    _perms |= S_IRUSR;
    setOwner(getuid());
}

void File::UserToWriter()
{
    _perms |= S_IWUSR;
    setOwner(getuid());
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

bool File::is_readable() const
{
    return _opened && (!(_opened_flags & O_WRONLY) || (_opened_flags & O_RDWR));
}

bool File::is_writable() const
{
    return _opened && (_opened_flags & (O_WRONLY | O_RDWR));
}

bool File::fileExists() const
{
    struct stat buffer;
    return (stat(_fpath.c_str(), &buffer) == 0);
}

bool File::is_UserInFileGroup() const
{
    auto currentUserID = getgid();
    struct stat fileStat;
    // Получение информации о файле
    if (stat(_fpath.c_str(), &fileStat) != 0)
    {
        return false;
    }

    // Получение информации о пользователе
    struct passwd* pw = getpwuid(currentUserID);
    if (!pw)
    {
        return false;
    }

    // Проверка основной группы пользователя
    if (fileStat.st_gid == pw->pw_gid)
    {
        return true;
    }

    // Получение списка дополнительных групп пользователя
    int ngroups = 0;
    getgrouplist(pw->pw_name, pw->pw_gid, nullptr, &ngroups); // Получаем количество групп
    std::vector<gid_t> groups(ngroups);
    getgrouplist(pw->pw_name, pw->pw_gid, groups.data(), &ngroups); // Заполняем список групп

    // Проверка принадлежности к группе файла
    for (gid_t gid : groups)
    {
        if (fileStat.st_gid == gid)
        {
            return true;
        }
    }

    return false;
}

bool File::is_UserFileOwner() const
{
    struct stat fileStat;

    // Получение информации о файле
    if (stat(_fpath.c_str(), &fileStat) != 0)
    {
        return false;
    }
    // Получение UID текущего пользователя
    uid_t currentUserID = getuid();

    // Проверка, является ли текущий пользователь владельцем файла
    return (fileStat.st_uid == currentUserID);
}

bool File::is_UserReadPerms() const
{
    if (!fileExists())
    {
        return false;
    }
    struct stat fileStat;
    if (stat(_fpath.c_str(), &fileStat) != 0)
    {
        return false;
    }
    if (is_UserFileOwner())
    {
        return fileStat.st_mode & S_IRUSR;
    }
    if (is_UserInFileGroup())
    {
        return fileStat.st_mode & S_IRGRP;
    }
    return fileStat.st_mode & S_IROTH;
}

bool File::is_UserWritePerms() const
{
    if (!fileExists())
    {
        return false;
    }
    struct stat fileStat;
    if (stat(_fpath.c_str(), &fileStat) != 0)
    {
        return false;
    }
    if (is_UserFileOwner())
    {
        return fileStat.st_mode & S_IWUSR;
    }
    if (is_UserInFileGroup())
    {
        return fileStat.st_mode & S_IWGRP;
    }
    return fileStat.st_mode & S_IWOTH;
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

mode_t File::stringToModeT(const std::string& modeStr)
{
    try
    {
        return static_cast<mode_t>(std::stoi(modeStr, nullptr, 8));
    }
    catch (const std::invalid_argument& e)
    {
        add_error("Invalid argument: " + std::string(e.what()));
    }
    catch (const std::out_of_range& e)
    {
        add_error("Out of range: " + std::string(e.what()));
    }
    return 0;
}
