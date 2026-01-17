cmake_minimum_required(VERSION 3.10)
project(TemperatureMonitorGUI C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Find required packages
find_package(PkgConfig REQUIRED)

# GTK3
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})
add_definitions(${GTK3_CFLAGS_OTHER})

# libcurl
pkg_check_modules(CURL REQUIRED libcurl)
include_directories(${CURL_INCLUDE_DIRS})
link_directories(${CURL_LIBRARY_DIRS})

# json-c
pkg_check_modules(JSON REQUIRED json-c)
include_directories(${JSON_INCLUDE_DIRS})
link_directories(${JSON_LIBRARY_DIRS})

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.c
    src/api_client.c
)

# Executable
add_executable(temp_monitor_gui ${SOURCES})

# Link libraries
target_link_libraries(temp_monitor_gui
    ${GTK3_LIBRARIES}
    ${CURL_LIBRARIES}
    ${JSON_LIBRARIES}
    m
)

# Install target
install(TARGETS temp_monitor_gui
    RUNTIME DESTINATION bin
)

# Print configuration summary
message(STATUS "")
message(STATUS "Temperature Monitor GUI Configuration:")
message(STATUS "  CMake version: ${CMAKE_VERSION}")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  GTK3 version: ${GTK3_VERSION}")
message(STATUS "  CURL version: ${CURL_VERSION}")
message(STATUS "  JSON-C version: ${JSON_VERSION}")
message(STATUS "")
