cmake_minimum_required(VERSION 3.10)

project(lab3 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(CMAKE_CONFIGURATION_TYPES)
    foreach(OUTPUTCONFIG IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER "${OUTPUTCONFIG}" OUTPUTCONFIG_UPPER)
        set("CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    endforeach()
endif()

find_package(Threads REQUIRED)

add_executable(lab3_main
    src/main.c
    src/app.c
    src/platform.c
    src/shared.c
)

target_include_directories(lab3_main PRIVATE include)

target_link_libraries(lab3_main PRIVATE Threads::Threads)

if(WIN32)
    target_compile_definitions(lab3_main PRIVATE _WIN32_WINNT=0x0601)
endif()
