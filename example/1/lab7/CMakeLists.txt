cmake_minimum_required(VERSION 3.16)

project(lab7_gui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt: ищем сначала Qt6, если нет — Qt5
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Gui Core Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Gui Core Network)

find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

# Qwt: Qt5 (Linux) / Qt6 (Windows)
if (UNIX AND NOT APPLE)
    find_path(QWT_INCLUDE_DIR qwt_plot.h
        PATHS /usr/include /usr/include/qwt /usr/include/qt5/qwt)
    find_library(QWT_LIBRARY
        NAMES qwt-qt5 qwt
        PATHS /usr/lib /usr/lib/x86_64-linux-gnu)
else()
    set(QWT_ROOT "C:/Qwt-6.3.0" CACHE PATH "Qwt install prefix")
    find_path(QWT_INCLUDE_DIR qwt_plot.h
        HINTS "${QWT_ROOT}/include" "${QWT_ROOT}")
    find_library(QWT_LIBRARY_RELEASE qwt
        HINTS "${QWT_ROOT}/lib" "${QWT_ROOT}")
    find_library(QWT_LIBRARY_DEBUG qwtd
        HINTS "${QWT_ROOT}/lib" "${QWT_ROOT}")
    if (QWT_LIBRARY_RELEASE)
        set(QWT_LIBRARY "${QWT_LIBRARY_RELEASE}")
    endif()
endif()

if (NOT QWT_INCLUDE_DIR OR NOT (QWT_LIBRARY OR QWT_LIBRARY_RELEASE OR QWT_LIBRARY_DEBUG))
    message(FATAL_ERROR "Qwt not found. Install libqwt-qt5-dev on Linux or set QWT_ROOT on Windows.")
endif()

add_library(Qwt::Qwt UNKNOWN IMPORTED)
set_target_properties(Qwt::Qwt PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${QWT_INCLUDE_DIR}")
if (QWT_LIBRARY_RELEASE)
    set_property(TARGET Qwt::Qwt APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
    set_target_properties(Qwt::Qwt PROPERTIES IMPORTED_LOCATION_RELEASE "${QWT_LIBRARY_RELEASE}")
endif()
if (QWT_LIBRARY_DEBUG)
    set_property(TARGET Qwt::Qwt APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
    set_target_properties(Qwt::Qwt PROPERTIES IMPORTED_LOCATION_DEBUG "${QWT_LIBRARY_DEBUG}")
endif()
if (NOT QWT_LIBRARY_RELEASE AND NOT QWT_LIBRARY_DEBUG)
    set_property(TARGET Qwt::Qwt PROPERTY IMPORTED_LOCATION "${QWT_LIBRARY}")
endif()

add_executable(lab7_gui
    src/frontend/main.cpp
    src/frontend/ApiClient.cpp
    src/frontend/MainWindow.cpp
    src/backend/backend.cpp
    src/backend/server_main.cpp
    src/backend/common.cpp
    src/backend/sample.cpp
    src/backend/logging.cpp
    src/backend/simulator.cpp
    src/backend/db.cpp
    src/backend/http_server.cpp
    include/frontend/ApiClient.h
    include/frontend/MainWindow.h
    include/backend/backend.h
    include/backend/common.h
    include/backend/sample.h
    include/backend/logging.h
    include/backend/simulator.h
    include/backend/db.h
    include/backend/http_server.h
)

target_include_directories(lab7_gui PRIVATE
    include/frontend
    include/backend
)

target_link_libraries(lab7_gui
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Network
        Qwt::Qwt
        SQLite::SQLite3
        Threads::Threads
)

if (WIN32)
    target_compile_definitions(lab7_gui PRIVATE _WIN32_WINNT=0x0601)
    target_link_libraries(lab7_gui PRIVATE ws2_32)
endif()
