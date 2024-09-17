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
public:
    enum open_mode {
        r = O_RDONLY,
        w = O_WRONLY,
        rw = O_RDWR
    };
private:
    const std::string _fpath;
    const open_mode _open_mode;
    mode_t _perms;
    int _fd = -1;
    bool _locked = false;
    int _locked_flags = LOCK_UN;

public:

    File(const std::string& file_path, open_mode open_mode = open_mode::r, mode_t file_perms = 0600);

    /// @brief Читает файл.
    /// @param start Начальная позиция в файле с котрой начнется чтение
    /// @param length Длина читаемых данных
    /// @return Прочитанные данные
    std::string fread(size_t start = 0, size_t length = 0);

    /// @brief Читает файл с блокировкой LOCK_SH
    /// @param start Начальная позиция в файле с котрой начнется чтение
    /// @param length Длина читаемых данных
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

    /// @brief Добавляет контент в конец файла
    /// @param data 
    /// @return 
    ssize_t append(const std::string& data);

    /// @brief Добавляет контент в новую строку
    /// @param data 
    /// @return 
    ssize_t appendNew(const std::string& data);

    /// @brief Добавляет контент с блокировкой файла
    /// @param data 
    /// @return 
    ssize_t appendLock(const std::string& data);

    /// @brief Добавляет в новую строку с блокировкой файла
    /// @param data 
    /// @return 
    ssize_t appendNewLock(const std::string& data);

    /// @brief Блокирует файл с флагами, указанными в аргументах
    bool lock(int flags);

    /// @brief Блокирует файл с флагом LOCK_EX
    bool lock_ex();

    /// @brief Блокирует файл с флагом LOCK_SH    
    bool lock_sh();

    /// @brief Снимает все блокировки
    void unlock();

    // методы меняющие системные права доступа к файлу

    /// @brief Задает группу файла
    /// @param group_id 
    void setGroup(gid_t group_id);

    /// @brief Задает владельца файла
    /// @param user_id 
    void setOwner(uid_t user_id);

    /// @brief Изменяет права доступа
    /// @param perms 
    void setPerms(mode_t perms = 0600);

    /// @brief Изменяет права доступа заданные строкой
    /// @param perms 
    void setPerms(const std::string& perms = "0600");

    /// @brief Дает право чтения указанному пользователю
    /// @param uid Если равен нулю права будут установлены для пользователя процесса
    void UserToReader(uid_t uid = 0);

    /// @brief Дает право записи указанному пользователю
    /// @param uid Если равен нулю права будут установлены для пользователя процесса
    void UserToWriter(uid_t uid = 0); // открыть текущему пользователю доступ для записи

    /// @brief Размер файла
    /// @return 
    unsigned long long int fsize();

    /// @brief Проверяет наложена ли любая блокировка
    /// @return 
    bool isLocked() const;
    /// @brief Проверяет эксклюзивную блокировку
    /// @return 
    bool isLockedEX() const;
    /// @brief Проверяет разделяемую блокировку
    /// @return 
    bool isLocketSH() const;

    // проверка открытого! файла на флаги чтения
    bool isReadable() const;
    // проверка открытого! файла на флаги записи
    bool isWritable() const;
    // проверка существования файла
    bool fileExists() const;
    // Проверка входит ли файл в группу текущего пользователя
    bool isUserInFileGroup(uid_t uid = 0) const;
    // Проверка является ли текущий пользователь владельцем
    bool isUserFileOwner(uid_t uid = 0) const;
    // Проверка имеет ли текущий пользователь права на чтение
    bool isUserReadPerms(uid_t uid = 0) const;
    // Проверка имеет ли текущий пользователь права на запись
    bool isUserWritePerms(uid_t uid = 0) const;
private:

    std::string errorMessage() const;
    mode_t stringToModeT(const std::string& modeStr);
};