# Лабораторная работа №3  
### Вариант: LR(0)  
**Студент**: Еделькин Герман, ИУ9-51Б  

---

## Компиляция программы  
Для компиляции используйте следующие команды:  
```bash
cd TFI_lab3
g++ -std=c++17 main.cpp grammar_reader.cpp -o program
```
---
## Запуск программы
Для запуска программы выполните:
```bash
./program
```
---
## Папка tests
В папке tests можно добавить файлы:
* Файл с грамматикой в формате .txt
Пример: grammar_2.txt
* Файл с тестовыми строками
Пример: strinsTocheck.txt

После добавления файлов необходимо изменить две строки в main.cpp для их использования:

* Строка 704:
    ```cpp
    ifstream file("tests/grammar_2.txt");
    ```

* Строка 743:
    ```cpp
    std::vector<StringBoolPair> data = readFileToVector("tests/strinsTocheck.txt");
    ```

## Результат работы

После выполнения программы генерируются:

1. PNG-файл с позиционным автоматом
2. PNG-файл с PDA
3. Вывод результата тестирования строк в консоль
    пример вывода:
    ```bash
    [OK] String: "aacbb" -> Result: 1
    [OK] String: "aaaaaaaacbc" -> Result: 0
    [OK] String: "acbb" -> Result: 1
    [OK] String: "cb" -> Result: 1
    [OK] String: "aaacbbba" -> Result: 0
    [OK] String: "oidfjoweir" -> Result: 0
    ```

