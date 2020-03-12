cmake_minimum_required(VERSION 3.15)
project(CrystalLyrics)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

include_directories(CLyric)

set(QT5_DIR "C:/Qt/Qt5.14.0/5.14.0/msvc2017_64")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -isystem ${QT5_DIR}/include/QtCore -isystem ${QT5_DIR}/include/QtWidgets -isystem ${QT5_DIR}/include/QtNetwork" ${CMAKE_CXX_FLAGS})
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if ("${CMAKE_CXX_SIMULATE_ID}" STREQUAL "MSVC")
        set(CMAKE_CXX_FLAGS "/W4 /EHa \"/clang:-isystem ${QT5_DIR}/include/QtCore\" \"/clang:-isystem ${QT5_DIR}/include/QtWidgets\" \"/clang:-isystem ${QT5_DIR}/include/QtNetwork\"" ${CMAKE_CXX_FLAGS})
    else ()
        set(CMAKE_CXX_FLAGS "-Wall -Wextra -isystem ${QT5_DIR}/include/QtCore -isystem ${QT5_DIR}/include/QtWidgets -isystem ${QT5_DIR}/include/QtNetwork" ${CMAKE_CXX_FLAGS})
    endif ()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "/W4 /EHa /external:I ${QT5_DIR}/include/QtCore /external:I ${QT5_DIR}/include/QtWidgets /external:I ${QT5_DIR}/include/QtNetwork /external:W0" ${CMAKE_CXX_FLAGS})
else ()
    message(FATAL_ERROR "Unsupported compiler!")
endif ()

find_package(Qt5 COMPONENTS Core Widgets Network REQUIRED)

find_package(OpenCC REQUIRED)

include_directories(${Opencc_INCLUDE_PATH})

add_subdirectory(CLyric)

qt5_add_resources(QT_RESOURCES ui-qt/res/crystallyrics.qrc)

aux_source_directory(ui-qt CRYSTALLYRICS_SRC)

if (WIN32)
    add_executable(CrystalLyrics WIN32 ${CRYSTALLYRICS_SRC} ui-qt/res/CrystalLyrics.rc ${QT_RESOURCES})
else()
    add_executable(CrystalLyrics ${CRYSTALLYRICS_SRC} ${QT_RESOURCES})
endif()

target_link_libraries(CrystalLyrics PRIVATE Qt5::Core Qt5::Widgets Qt5::Network ${Opencc_LIBRARY} CLyric)