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

#define F_UNLOCK 64

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
    const enum mode {
        r = O_RDONLY,
        w = O_WRONLY,
        rw = O_RDWR
    };
    File(const std::string& file_path, mode_t file_perms = 0600);

    /// @brief Открывает файл
    /// @param flags Флаги режима открытия
    /// @return Дескриптор файла
    int fopen(int flags);

    int fopenR();

    int fopenW();

    int fopenRW();

    void fclose();

    /// @brief Читает файл.
    /// @param start Начальная позиция в файле с котрой начнется чтение
    /// @param length Длина читаемых данных
    /// @return Прочитанные данные
    std::string fread(size_t start = 0, size_t length = 0);

    /// @brief Читает файл с блокировкой LOCK_SH
    /// @param start Начальная позиция в файле с котрой начнется чтение
    /// @param length Длина читаемых данных
    /// @param unlock Снять блокировку после завершения операции
    /// @return Прочитанные данные
    std::string freadLock(size_t start = 0, size_t length = 0);

    /// @brief Записывает данные в файл
    /// @param data Данные для записи
    /// @param start Начальная позиция.
    /// @param length Длина записываемых данных
    /// @return
    ssize_t fwrite(const std::string& data, size_t start = 0, size_t length = 0);

    /// @brief Записывает данные в файл с блокировкой LOCK_EX
    /// @param data Данные для записи
    /// @param start Начальная позиция.
    /// @param length Длина записываемых данных
    /// @return
    ssize_t fwriteLock(const std::string& data, size_t start = 0, size_t length = 0);

    /// @brief Блокирует файл с флагами, указанными в аргументах
    /// @param flags 
    /// @return 
    bool lock(int flags);

    /// @brief Блокирует файл с флагом LOCK_EX
    /// @param lock_nb добавление флага LOCK_NB
    /// @return 
    bool lock_ex();

    /// @brief Блокирует файл с флагом LOCK_SH    
    /// @return 
    bool lock_sh();

    /// @brief Снимает все блокировки
    void unlock();

    // методы меняющие системные права доступа к файлу
    void setGroup(gid_t group_id);
    void setOwner(uid_t user_id);
    void setPerms(mode_t perms = 0600);
    void setPerms(const std::string& perms = "0600");
    void UserToReader(); // открыть текущему пользователю доступ для чтения
    void UserToWriter(); // открыть текущему пользователю доступ для записи

    unsigned long long int fsize();

    int fd();
    bool is_opened() const;
    bool is_locked() const;
    bool is_locked_ex() const;
    bool is_locket_sh() const;

    // проверка открытого! файла на флаги чтения
    bool isReadable() const;
    // проверка открытого! файла на флаги записи
    bool isWritable() const;
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
    mode_t stringToModeT(const std::string& modeStr);
};