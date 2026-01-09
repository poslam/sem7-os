# Курсовка по C/C++ (CMake)

Лабораторные 1–6 и их сборка.

## Требования
- CMake 3.10+
- Компилятор C++17 (gcc/clang на Linux, gcc/MSVC на Windows)
- git, VS Code (по желанию), gdb/отладчик
- Для лаб 5–6: Node.js 18+ (Vite) и Qt/Qwt

# Windows: команды сборки по лабам
- Лаба 1 — Hello, World
```bash
cmake -S lab1 -B lab1/build -DCMAKE_BUILD_TYPE=Debug
cmake --build lab1/build --config Debug

cmake -S lab1 -B lab1/build-mingw -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build lab1/build-mingw
```
Выход: `lab1/build/bin/hello_cpp` (.exe).

- Лаба 2 — подпроцессы
```bash
cmake -S lab2 -B lab2/build -DCMAKE_BUILD_TYPE=Debug
cmake --build lab2/build --config Debug

cmake -S lab2 -B lab2/build-mingw -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build lab2/build-mingw
```
Выход: `lab2/build/bin/bg_test`.

- Лаба 3 — shared память
```bash
cmake -S lab3 -B lab3/build -DCMAKE_BUILD_TYPE=Debug
cmake --build lab3/build --config Debug

cmake -S lab3 -B lab3/build-mingw -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build lab3/build-mingw
```
Выход: `lab3/build/bin/lab3_main`.

- Лаба 4 — агрегаты/симулятор
```bash
cmake -S lab4 -B lab4/build -DCMAKE_BUILD_TYPE=Debug
cmake --build lab4/build --config Debug

cmake -S lab4 -B lab4/build-mingw -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build lab4/build-mingw
```
- Лаба 5 — HTTP + веб (React/Vite) + SQLite
1) Фронт (Node 18+):
   ```bash
   cd lab5/web
   npm install
   node node_modules/vite/bin/vite.js build
   ```
2) Бэкенд:
   ```bash
   cmake -S lab5 -B lab5/build -DCMAKE_BUILD_TYPE=Debug
   cmake --build lab5/build --config Debug

   cmake -S lab5 -B lab5/build-mingw -G "MinGW Makefiles" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH=C:/msys64/mingw64
   cmake --build lab5/build-mingw
   ```
3) Запуск: `./lab5/build/bin/lab5_main --simulate` (Windows: `.exe`).
   API (порт 8080): `/api/current`, `/api/stats?bucket=measurements|hourly|daily&start=<ms>&end=<ms>`.

- Лаба 6 — Qt/Qwt GUI с встроенным бэкендом (логика лаб5)
- Фронт: `lab6/src/frontend`, `lab6/include/frontend`.
- Бэк: `lab6/src/backend`, `lab6/include/backend` (HTTP на :8080 внутри GUI).
- Зависимости (Windows, MSVC): Qt 6.10.1 MSVC `C:\dev\Qt\6.10.1\msvc2022_64`, Qwt 6.3.0 `C:\Qwt-6.3.0`, SQLite3 (vcpkg).

Сборка (MSVC):
```cmd
cmake -S lab6 -B lab6/build-msvc -G "Visual Studio 17 2022" -A x64^
-DCMAKE_TOOLCHAIN_FILE=C:/dev/tools/vcpkg/scripts/buildsystems/vcpkg.cmake ^"-DCMAKE_PREFIX_PATH=C:/dev/Qt/6.10.1/msvc2022_64;C:/Qwt-6.3.0;C:/dev/tools/vcpkg/installed/x64-windows"
cmake --build lab6/build-msvc --config Release   # или Debug

cmake -S lab6 -B lab6/build-mingw -G "MinGW Makefiles" ^
  -DCMAKE_BUILD_TYPE=Release ^
  "-DCMAKE_PREFIX_PATH=C:/Qt/6.6.3/mingw_64;C:/Qwt-6.3.0-mingw;C:/msys64/mingw64"
cmake --build lab6/build-mingw
```

# Linux: команды сборки по лабам
- Лаба 1 — Hello, World
  ```bash
  cmake -S lab1 -B lab1/build-linux -DCMAKE_BUILD_TYPE=Release
  cmake --build lab1/build-linux --config Release
  ```
- Лаба 2 — подпроцессы
  ```bash
  cmake -S lab2 -B lab2/build-linux -DCMAKE_BUILD_TYPE=Release
  cmake --build lab2/build-linux --config Release
  ```
- Лаба 3 — shared память
  ```bash
  cmake -S lab3 -B lab3/build-linux -DCMAKE_BUILD_TYPE=Release
  cmake --build lab3/build-linux --config Release
  ```
- Лаба 4 — агрегаты/симулятор
  ```bash
  cmake -S lab4 -B lab4/build-linux -DCMAKE_BUILD_TYPE=Release
  cmake --build lab4/build-linux --config Release
  ```
- Лаба 5 (Qt не нужен, но нужен sqlite3-dev; фронт отдельно npm run build в lab5/web):
  ```bash
  cmake -S lab5 -B lab5/build-linux -DCMAKE_BUILD_TYPE=Release
  cmake --build lab5/build-linux --config Release
  ./lab5/build-linux/bin/lab5_main --simulate
  ```
- Лаба 6 (Qt5+Qwt под Ubuntu 20.04):
  ```bash
  sudo apt install -y build-essential cmake ninja-build libsqlite3-dev qtbase5-dev qttools5-dev qttools5-dev-tools libqwt-qt5-dev
  cmake -S lab6 -B lab6/build-linux -G "Ninja" \
    -DCMAKE_BUILD_TYPE=Release \
    "-DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt5;/usr/lib/x86_64-linux-gnu/cmake/qwt"
  cmake --build lab6/build-linux --config Release
  ./lab6/build-linux/lab6_gui
  ```
# Kiosk GUI (Linux)

Полный минимальный сценарий киоска на Linux.

## 1. Сборка
```bash
cmake -S lab7 -B lab7/build-linux -G "Ninja" -DCMAKE_BUILD_TYPE=Release "-DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt5;/usr/lib/x86_64-linux-gnu/cmake/qwt"
cmake --build lab7/build-linux --config Release
```
Бинарь: `lab7/build-linux/lab7_gui.` Данные/логи: `lab7/db/lab7.db, lab7/logs.`

## 2. Создать пользователя kiosk (без sudo)
```bash
sudo adduser kiosk
```
# (опционально) запретить парольный вход
```bash
sudo passwd -l kiosk
```
## 3. Автологин на tty1
```bash
sudo systemctl edit getty@tty1.service
[Service]
ExecStart=
ExecStart=-/sbin/agetty --autologin kiosk --noclear %I $TERM

sudo systemctl daemon-reload
sudo systemctl restart getty@tty1
```
## 4. Файлы автозапуска под kiosk
```bash
sudo -u kiosk tee /home/kiosk/.bash_profile >/dev/null <<'EOF'
if [ -z "$DISPLAY" ] && [ "$(tty)" = "/dev/tty1" ]; then
  startx -- -nocursor
fi
EOF

sudo -u kiosk tee /home/kiosk/.xinitrc >/dev/null <<'EOF'
exec /home/soul/codeOS/lab7/build-linux/lab7_gui
EOF
```
Убедитесь, что X установлен: sudo apt-get install -y xorg xinit

## 5. Права на проект
```bash
sudo chown -R kiosk:kiosk /home/soul/codeOS/lab7
```
(чтобы БД/логи создавались под kiosk).

## 6. (Опционально) Маскировать лишние TTY
```bash
sudo systemctl mask getty@tty2.service getty@tty3.service getty@tty4.service getty@tty5.service getty@tty6.service
```

## 7. Запуск
Перезагрузить или выйти/войти на tty1: автологин под kiosk, startx поднимет X и запустит lab7_gui в полноэкранном киоске (рамки/курсор скрыты, закрытие и хоткеи блокированы). Сервисный доступ — по SSH под админом (ключи), внутри киоска выхода нет.
```bash
sudo systemctl set-default multi-user.target
```
или 
```bash
sudo systemctl restart getty@tty1
```
## 8. Закрытие киоска
Нажимаем `Ctrl+Alt+F2` вводим `login` и `password`, после этого `sudo systemctl set-default graphical.target` и далее `reboot`.