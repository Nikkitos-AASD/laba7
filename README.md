## 1. Устройство окружения

Цель: установить и проверить инструменты для работы с Clang/LLVM и визуализации CFG.

1.1 Обновление индекса пакетов

```bash
sudo apt update
```

– получает актуальные метаданные репозиториев.

![image](https://github.com/user-attachments/assets/160b600e-5220-47e5-92b4-538574f0c830)


1.2 Установка Clang, LLVM, Opt и Graphviz

```bash
sudo apt install -y clang llvm opt graphviz
```

– `clang` для генерации AST и IR,
– `llvm` для библиотек и утилит (llc, llvm-as),
– `opt` для применения оптимизаций к IR,
– `graphviz` для конвертации DOT в изображения.

![image](https://github.com/user-attachments/assets/5be927a2-b852-42e7-adfc-816b51f739f8)


1.3 Проверка версий

```bash
clang --version
opt --version
dot -V
```

Убедился, что версии совместимы и без ошибок при запуске.

 ![image](https://github.com/user-attachments/assets/a21cf14f-0ead-4541-b42c-2cc9f6455194)



## 2. Исходный пример

Цель: получить простой код с константой и условием для демонстрации оптимизаций.

Файл `main.c`:

```c
#include <stdio.h>

const int LIMIT = 100;

int check(int x) {
    if (x < LIMIT)
        return x * 2;
    return x + LIMIT;
}

int main(void) {
    int value  = 42;
    int result = check(value);
    printf("Result = %d\n", result);
    return 0;
}
```

* `LIMIT` проверяется в оптимизациях `constprop`.
* `check` содержит две ветви (`<` и `>=`).
* В `main` фиксированное `value` позволяет предвычисления.

![image](https://github.com/user-attachments/assets/f4accec0-1ad9-45eb-b819-6a3d96cc126d)




## 3. Извлечение AST

Цель: вывести синтаксическое дерево и определить ключевые узлы.

Команда:

```bash
clang -Xclang -ast-dump -fsyntax-only main.c
```

* `-Xclang -ast-dump` – дамп AST.
* `-fsyntax-only` – только проверка, без сборки.

Обращаем внимание на:

* `VarDecl LIMIT` с `IntegerLiteral 100`.
* `FunctionDecl` для `check` и `main`.
* `IfStmt` в `check`.
* `CallExpr` для `printf`.

![image](https://github.com/user-attachments/assets/b45782de-fdfe-40e3-85e0-3df07f4c2c83)




## 4. Создание LLVM IR

Цель: получить IR без оптимизаций и изучить структуру.

Команда:

```bash
clang -S -emit-llvm main.c -o main_unopt.ll
```

* `-S` – текстовый файл `.ll`.
* `-emit-llvm` – LLVM IR.

В файле:

* `entry` блоки с `alloca` для `value` и `result`.
* Инструкции `store` и `load`.
* Функция `@check` разбита на `if.then`, `if.else`, `if.end`.

![image](https://github.com/user-attachments/assets/0cc75566-c3fd-481d-b82a-547190e3c37f)


## 5. Проведение оптимизаций

### 5.1 Уровень O0

```bash
clang -O0 -S -emit-llvm main.c -o main_O0.ll
```

* IR примерно как в `main_unopt.ll`.
* Сохраняются все `alloca`, `load`, `store`.
* `@check` как отдельная функция.

![image](https://github.com/user-attachments/assets/f58685e2-03a3-4c95-8aa8-32b661ae0b7e)


### 5.2 Уровень O2

```bash
clang -O2 -S -emit-llvm main.c -o main_O2.ll
```

Включает:

* `constprop` – подставляет `LIMIT=100`,
* `inline` – встраивает `check` в `main`,
* `mem2reg` – удаляет `alloca`, перевод в SSA,
* `instcombine`, `simplifycfg` и другие.

Результат:

1. Нет `alloca`, `load`, `store`.
2. `check` исчезает, её тело перенесено.
3. `printf` получает предвычисленное значение.

![image](https://github.com/user-attachments/assets/3ec4bbd2-fd05-45fe-98b4-0198c8aee3c4)

### 5.3 Сравнение O0 и O2

```bash
diff -u main_O0.ll main_O2.ll | head -n 25
```

* Удалены блоки операций памяти.
* Появился SSA‑код вместо `load`/`store`.
* Вызов `check` заменён арифметическими инструкциями.

![image](https://github.com/user-attachments/assets/2154cd31-3ae0-42db-bd3e-ac96ca9a0b2d)




## 6. Составление и просмотр CFG

Цель: отобразить и проанализировать граф потока управления после оптимизаций.

1. Генерация DOT:

   ```bash
   opt -dot-cfg -disable-output main_O2.ll
   ```

   Создаётся скрытый файл `.main.dot`.

   ![image](https://github.com/user-attachments/assets/2e266b83-7d03-45fd-9fc0-5406981d54f3)


2. Конвертация в PNG и просмотр:

   ```bash
   dot -Tpng .main.dot -o cfg_main.png
   xdg-open cfg_main.png
   ```

   * Если условие статично известно, CFG сводится к одному блоку.
   * Иначе: два блока (true/false).
    ![image](https://github.com/user-attachments/assets/7fa1969e-3231-45da-9f98-c17b4ee7da59)




## 7. Основные выводы

1. AST показывает исходную синтаксическую структуру, полезен для статического анализа.
2. IR в режиме `-O0` содержит явные операции памяти, что затрудняет трансформации.
3. `-O2` выполняет `constprop`, `inline`, `mem2reg` и другие, приводя к компактному SSA.
4. CFG после оптимизаций упрощается, уменьшается количество блоков и ветвлений.



## 8. Дополнительное задание: исследование оптимизаций (вариант 5)

### 8.1 Отдельное constprop

```bash
clang -S -emit-llvm main.c -o temp.ll
opt -S --passes="correlated-propagation" temp.ll -o main_constprop.ll
```

В `main_constprop.ll`:

* `LIMIT` заменён на `100`.
* Сохраняются `alloca`, `lo\ad`.


![image](https://github.com/user-attachments/assets/56f8b2f1-b352-4838-ba19-908d9d79f4a5)


### 8.2 Сравнение с O2

```bash
diff -u main_constprop.ll main_O2.ll | sed -n '1,20p'
```

* `constprop` лишь подставляет константы.
* `-O2` добавляет `mem2reg`, `inline`, `simplifycfg`.

![Uploading image.png…]()

Вывод: `constprop` заменяет литералы, но для полного удаления памяти и ветвлений нужен комплекс оптимизаций `-O2`.
