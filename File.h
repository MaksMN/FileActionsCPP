#pragma once
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <string>
#include <iostream>
#include <cstring>
#include <vector>

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

    /// @brief Открывает файл
    /// @param flags Флаги режима открытия
    /// @param file_perms Права доступа если файл имеет флаг O_CREAT
    /// @return Дескриптор файла
    int fopen(int flags = O_RDWR | O_CREAT, mode_t file_perms = 0600);
    void fclose();

    /// @brief Читает файл.
    /// @param start Начальная позиция в файле с котрой начнется чтение
    /// @param length Длина читаемых данных
    /// @param lock_flags Флаги блокировки. Допустимые LOCK_SH | LOCK_UN. Если ноль, то блокировки не будет. Ели включен флаг LOCK_UN, то после чтения файл будет разблокирован.
    /// @return Прочитанные данные
    std::string fread(size_t start = 0, size_t length = 0, int lock_flags = 0);

    /// @brief Читает файл с блокировкой LOCK_SH | LOCK_UN
    /// @param start Начальная позиция в файле с котрой начнется чтение
    /// @param length Длина читаемых данных
    /// @return Прочитанные данные
    std::string fread_lock(size_t start = 0, size_t length = 0);

    /// @brief Записывает данные в файл
    /// @param data Данные для записи
    /// @param start Начальная позиция.
    /// @param length Длина записываемых данных
    /// @param lock_flags Флаги блокировки. Допустимые LOCK_EX | LOCK_UN. Если ноль, то блокировки не будет. Ели включен флаг LOCK_UN, то после записи файл будет разблокирован.
    /// @return
    ssize_t fwrite(const std::string &data, size_t start = 0, size_t length = 0, int lock_flags = 0);
    /// @brief Записывает данные в файл с блокировкой LOCK_EX | LOCK_UN
    /// @param data Данные для записи
    /// @param start Начальная позиция.
    /// @param length Длина записываемых данных
    /// @return
    ssize_t fwrite_lock(const std::string &data, size_t start = 0, size_t length = 0);

    // методы меняющие системные права доступа к файлу
    void setGroup(gid_t group_id);
    void setOwner(uid_t user_id);
    void setPerms(mode_t perms = 0600);
    void setPerms(const std::string &perms = "0600");
    void UserToReader(); // открыть текущему пользователю доступ для чтения
    void UserToWriter(); // открыть текущему пользователю доступ для записи

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

    // проверка открытого! файла на флаги чтения
    bool is_readable() const;
    // проверка открытого! файла на флаги записи
    bool is_writable() const;
    // проверка существования файла
    bool fileExists() const;
    // Проверка входит ли файл в группу текущего пользователя
    bool is_UserInFileGroup() const;
    // Проверка является ли текущий пользователь владельцем
    bool is_UserFileOwner() const;
    // Проверка имеет ли текущий пользователь права на чтение
    bool is_UserReadPerms() const;
    // Проверка имеет ли текущий пользователь права на запись
    bool is_UserWritePerms() const;

    int error_number() const;
    std::string error_message() const;
    void error_clear();

private:
    void add_error(std::string prefix = std::string());
    mode_t stringToModeT(const std::string &modeStr);
};