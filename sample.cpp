#include "File.h"
#include <thread>
#include <chrono>

int main()
{
    /*пример использования*/

    File f("test.txt", File::open_mode::rw); // создаем экземпляр класса, файл будет создан если не существует
    f.setPerms(0644);           // меняем права доступа к файлу

    std::cout << "Блокируем файл и записываем в него данные" << std::endl;
    std::cout << "Запустите sample.php" << std::endl;
    f.lock_ex(); // получаем эксклюзивную блокировку файла

    std::string text = "Файл заблокирован в приложении C++";
    f.fwrite(text); // записываем данные в файл
    std::this_thread::sleep_for(std::chrono::seconds(30)); // имитируем долгий процесс записи

    // до истечения времени задержки запустите sample.php и он будет ждать когда файл test.txt будет разблокирован

    f.unlock(); // после выполнения этого метода sample.php продолжит свою работу

    return 0;
}
