cmake_minimum_required(VERSION 3.10)

project(lab4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(CMAKE_CONFIGURATION_TYPES)
    foreach(OUTPUTCONFIG IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER "${OUTPUTCONFIG}" OUTPUTCONFIG_UPPER)
        set("CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    endforeach()
endif()

find_package(Threads REQUIRED)

add_executable(lab4_main
    src/main.cpp
    src/common.cpp
    src/sample.cpp
    src/logging.cpp
    src/simulator.cpp
)

target_include_directories(lab4_main PRIVATE include)

target_link_libraries(lab4_main PRIVATE Threads::Threads)

if(WIN32)
    target_compile_definitions(lab4_main PRIVATE _WIN32_WINNT=0x0601)
endif()
