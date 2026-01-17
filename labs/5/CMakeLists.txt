cmake_minimum_required(VERSION 3.10)
project(TemperatureMonitor C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Find required packages
find_package(Threads REQUIRED)

# Check for SQLite3
find_library(SQLITE3_LIBRARY sqlite3)
if(NOT SQLITE3_LIBRARY)
    message(STATUS "SQLite3 not found in system, will try to use pkg-config")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SQLITE3 REQUIRED sqlite3)
endif()

# Platform-specific settings
if(WIN32)
    set(PLATFORM_LIBS ws2_32)
else()
    set(PLATFORM_LIBS m)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files for main application
set(MAIN_SOURCES
    src/main.c
    src/database.c
    src/http_server.c
    src/serial_reader.c
)

# Source files for simulator
set(SIMULATOR_SOURCES
    src/simulator.c
)

# Main temperature monitor executable
add_executable(temp_monitor ${MAIN_SOURCES})

# Link libraries
target_link_libraries(temp_monitor
    ${CMAKE_THREAD_LIBS_INIT}
    ${SQLITE3_LIBRARY}
    ${PLATFORM_LIBS}
)

# Temperature simulator executable
add_executable(temp_simulator ${SIMULATOR_SOURCES})
target_link_libraries(temp_simulator ${PLATFORM_LIBS})

# Copy web files to build directory
file(COPY ${CMAKE_SOURCE_DIR}/web DESTINATION ${CMAKE_BINARY_DIR})

# Install targets
install(TARGETS temp_monitor temp_simulator
    RUNTIME DESTINATION bin
)

install(DIRECTORY web
    DESTINATION share/temp_monitor
)

# Print configuration summary
message(STATUS "")
message(STATUS "Temperature Monitor Configuration:")
message(STATUS "  CMake version: ${CMAKE_VERSION}")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  SQLite3 library: ${SQLITE3_LIBRARY}")
message(STATUS "  Platform libs: ${PLATFORM_LIBS}")
message(STATUS "")
