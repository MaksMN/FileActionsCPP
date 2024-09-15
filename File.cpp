#include "File.h"

File::File(const std::string& file_path, open_mode open_mode, mode_t file_perms) : _fpath(file_path), _open_mode(open_mode), _perms(file_perms)
{
    int o_mode = fileExists() ? _open_mode : _open_mode | O_CREAT;
    _fd = open(_fpath.c_str(), o_mode, _perms);

    if (_fd == -1)
    {
        throw std::runtime_error("File open: " + errorMessage());
    }
}

std::string File::fread(size_t start, size_t length)
{
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
                throw std::runtime_error("File read: " + errorMessage());
            }
        }
        char buffer[length];
        size_t l = sizeof(buffer);
        ssize_t bytesRead = read(_fd, buffer, l);
        if (bytesRead == -1)
        {
            throw std::runtime_error("File read: " + errorMessage());
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
    lock_sh();
    std::string result = fread();
    unlock();
    return result;
}

ssize_t File::fwrite(const std::string& data, size_t start, size_t length)
{
    if (lseek(_fd, start, SEEK_SET) == -1)
    {
        throw std::runtime_error("File write: " + errorMessage());
    }
    if (length == 0)
    {
        length = data.size();
    }
    auto result = write(_fd, data.c_str(), length);
    if (result == -1)
    {
        throw std::runtime_error("File write: " + errorMessage());
    }
    return result;
}

ssize_t File::fwriteLock(const std::string& data, size_t start, size_t length)
{
    lock_ex();
    auto result = fwrite(data, start, length);
    unlock();
    return result;
}

ssize_t File::append(const std::string& data)
{
    auto start = fsize();
    return fwrite(data, start);
}

ssize_t File::appendNew(const std::string& data)
{
    return append("\n" + data);
}

ssize_t File::appendLock(const std::string& data)
{
    auto start = fsize();
    return fwriteLock(data, start);
}

ssize_t File::appendNewLock(const std::string& data)
{
    return appendNewLock("\n" + data);
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
        throw std::runtime_error("File lock: " + errorMessage());
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
        throw std::runtime_error("File unlock: " + errorMessage());
    }
    _locked = false;
    _locked_flags = LOCK_UN;
}

void File::setGroup(gid_t group_id)
{
    struct stat fileInfo;

    if (fstat(_fd, &fileInfo) != 0)
    {
        throw std::runtime_error("File set group: " + errorMessage());
    }
    auto user_id = fileInfo.st_uid;
    if (chown(_fpath.c_str(), user_id, group_id) != 0)
    {
        throw std::runtime_error("File set group: " + errorMessage());
    }
}

void File::setOwner(uid_t user_id)
{
    struct stat fileInfo;

    if (fstat(_fd, &fileInfo) != 0)
    {
        throw std::runtime_error("File set owner: " + errorMessage());
    }
    auto group_id = fileInfo.st_gid;
    if (chown(_fpath.c_str(), user_id, group_id) != 0)
    {
        throw std::runtime_error("File set owner: " + errorMessage());
    }
}

void File::setPerms(mode_t perms)
{
    if (chmod(_fpath.c_str(), perms) == -1)
    {
        throw std::runtime_error("File set permissions: " + errorMessage());
    }
    _perms = perms;
}

void File::setPerms(const std::string& perms)
{
    try
    {
        auto p = stringToModeT(perms);
        setPerms(p);
    }
    catch (const std::exception& e)
    {
        throw;
    }
}

void File::UserToReader(uid_t uid)
{
    uid = uid == 0 ? getuid() : uid;
    _perms |= S_IRUSR;
    setOwner(uid);
    setPerms(_perms);
}

void File::UserToWriter(uid_t uid)
{
    uid = uid == 0 ? getuid() : uid;
    _perms |= S_IWUSR;
    setOwner(uid);
    setPerms(_perms);
}

std::string File::errorMessage() const
{
    return "(" + std::to_string(errno) + ") " + std::strerror(errno) + "\n";
}

mode_t File::stringToModeT(const std::string& modeStr)
{
    try
    {
        return static_cast<mode_t>(std::stoi(modeStr, nullptr, 8));
    }
    catch (const std::invalid_argument& e)
    {
        throw;
    }
    catch (const std::out_of_range& e)
    {
        throw;
    }
}

bool File::fileExists() const
{
    struct stat buffer;
    return (stat(_fpath.c_str(), &buffer) == 0);
}

bool File::isUserInFileGroup(uid_t uid) const
{
    uid = uid == 0 ? getuid() : uid;
    struct stat fileStat;
    // Получение информации о файле
    if (stat(_fpath.c_str(), &fileStat) != 0)
    {
        return false;
    }

    // Получение информации о пользователе
    struct passwd* pw = getpwuid(uid);
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

bool File::isUserFileOwner(uid_t uid) const
{
    struct stat fileStat;

    // Получение информации о файле
    if (stat(_fpath.c_str(), &fileStat) != 0)
    {
        return false;
    }
    // Получение UID текущего пользователя
    uid = uid == 0 ? getuid() : uid;

    // Проверка, является ли текущий пользователь владельцем файла
    return (fileStat.st_uid == uid);
}

bool File::isUserReadPerms(uid_t uid) const
{
    uid = uid == 0 ? getuid() : uid;
    struct stat fileStat;
    if (stat(_fpath.c_str(), &fileStat) != 0)
    {
        return false;
    }
    if (isUserFileOwner(uid))
    {
        return fileStat.st_mode & S_IRUSR;
    }
    if (isUserInFileGroup(uid))
    {
        return fileStat.st_mode & S_IRGRP;
    }
    return fileStat.st_mode & S_IROTH;
}

bool File::isUserWritePerms(uid_t uid) const
{
    uid = uid == 0 ? getuid() : uid;
    struct stat fileStat;
    if (stat(_fpath.c_str(), &fileStat) != 0)
    {
        return false;
    }
    if (isUserFileOwner(uid))
    {
        return fileStat.st_mode & S_IWUSR;
    }
    if (isUserInFileGroup(uid))
    {
        return fileStat.st_mode & S_IWGRP;
    }
    return fileStat.st_mode & S_IWOTH;
}

unsigned long long int File::fsize()
{
    struct stat fileInfo;

    if (fstat(_fd, &fileInfo) == -1)
    {
        throw std::runtime_error("File get size: " + errorMessage());
    }
    else
    {
        return (unsigned long long int)fileInfo.st_size;
    }

}

bool File::isLocked() const
{
    return _locked;
}

bool File::isLockedEX() const
{
    return _locked_flags & LOCK_EX;
}

bool File::isLocketSH() const
{
    return _locked_flags & LOCK_SH;
}

bool File::isReadable() const
{
    return _open_mode == open_mode::r || _open_mode == open_mode::rw;
}

bool File::isWritable() const
{
    return _open_mode == open_mode::w || _open_mode == open_mode::rw;
}
