cmake_minimum_required(VERSION 3.10)
project(ProcessLibrary C)

set(CMAKE_C_STANDARD 99)

# Библиотека для работы с процессами
add_library(process STATIC
    process.c
    process.h
)

# Тестовая программа для запуска в фоне
add_executable(test_program
    test_program.c
)

# Основная тестовая утилита
add_executable(test_code
    test_code.c
)

# Линковка библиотеки к тестовой утилите
target_link_libraries(test_code process)

# Установка опций компиляции
if(MSVC)
    target_compile_options(process PRIVATE /W4)
    target_compile_options(test_code PRIVATE /W4)
    target_compile_options(test_program PRIVATE /W4)
else()
    target_compile_options(process PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(test_code PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(test_program PRIVATE -Wall -Wextra -pedantic)
endif()

# Установка выходных директорий
set_target_properties(test_code test_program PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Для Windows может понадобиться копирование test_program в рабочую директорию test_code
if(WIN32)
    add_custom_command(TARGET test_code POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:test_program>"
        "$<TARGET_FILE_DIR:test_code>"
    )
endif()
