cmake_minimum_required(VERSION 3.15)
project(CrystalLyrics)

set(CMAKE_PREFIX_PATH "/Volumes/zdata_ssd/Program/Qt5.13.2/5.13.2/clang_64")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_PATH "./cmake-build-debug/CrystalLyrics_autogen/include")

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(CrystalLyrics main.cpp mainwindow.cpp mainwindow.h mainwindow.ui window.cpp window.h)

target_link_libraries(CrystalLyrics PRIVATE Qt5::Widgets)