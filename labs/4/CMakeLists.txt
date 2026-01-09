cmake_minimum_required(VERSION 3.10)
project(lab4 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(lab4 4.c)

target_compile_options(lab4 PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(lab4 PRIVATE m)

add_executable(simulator simulator.c)

target_compile_options(simulator PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(simulator PRIVATE m)
