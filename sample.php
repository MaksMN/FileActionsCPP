<?php

/* 
Пример получения разделяемой блокировки на файл, 
который используется другим процессом

1. Скомпилируйте sample.cpp и запустите его.
2. Запустите этот скрипт, пока процесс из sample.cpp удерживает эксклюзивную блокировку.
*/
$fname = __DIR__ . '/test.txt';

echo "Читаем файл $fname... \n";

$locked_file = fopen($fname, 'r');
flock($locked_file, LOCK_SH);
echo fread($locked_file, filesize($fname)) . "\n"; // эта строчка будет выполнена как только sample.cpp снимет блокировку 
fclose($locked_file);
