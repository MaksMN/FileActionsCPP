#include "../File.h"
#include <thread>
#include <chrono>

int main()
{
    /*пример использования*/

    File f("Example/test.txt"); // создаем экземпляр класса, файл будет создан если не существует
    f.setPerms(0644);           // меняем права доступа к файлу

    f.lock_ex(); // получаем эксклюзивную блокировку файла
    std::string text = "It's a locked file";

    f.fwrite(text, 0, text.size(), 0);                     // записываем данные в файл
    std::this_thread::sleep_for(std::chrono::seconds(30)); // имитируем долгий процесс записи

    // до истечения времени задержки запустите sample.php и он будет ждать когда файл test.txt будет разблокирован

    f.unlock(); // после выполнения этого метода sample.php продолжит свою работу
    f.fclose();
    return 0;
}
