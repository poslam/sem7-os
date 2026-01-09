cmake_minimum_required(VERSION 3.10)

project(bgproc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(CMAKE_CONFIGURATION_TYPES)
    foreach(OUTPUTCONFIG IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER "${OUTPUTCONFIG}" OUTPUTCONFIG_UPPER)
        set("CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    endforeach()
endif()

add_library(bgproc STATIC
    src/bg_process.cpp
)
target_include_directories(bgproc PUBLIC include)

if (WIN32)
    target_compile_definitions(bgproc PRIVATE _WIN32_WINNT=0x0601)
endif()

add_executable(bg_test
    src/bg_test.cpp
)
target_link_libraries(bg_test PRIVATE bgproc)
