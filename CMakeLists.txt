cmake_minimum_required(VERSION 3.15)

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum OS X deployment version")
endif()

project(CrystalLyrics)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

include_directories(CLyric)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -isystem ${QT_DIR}/include/QtCore -isystem ${QT_DIR}/include/QtWidgets -isystem ${QT_DIR}/include/QtNetwork ${CMAKE_CXX_FLAGS}")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    if ("${CMAKE_CXX_SIMULATE_ID}" STREQUAL "MSVC")
        set(CMAKE_CXX_FLAGS "/W4 /EHa \"/clang:-isystem ${QT_DIR}/include/QtCore\" \"/clang:-isystem ${QT_DIR}/include/QtWidgets\" \"/clang:-isystem ${QT_DIR}/include/QtNetwork\" ${CMAKE_CXX_FLAGS}")
    else ()
        set(CMAKE_CXX_FLAGS "-Wall -Wextra -isystem ${QT_DIR}/include/QtCore -isystem ${QT_DIR}/include/QtWidgets -isystem ${QT_DIR}/include/QtNetwork ${CMAKE_CXX_FLAGS}")
    endif ()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "/W4 /EHa /external:I ${QT_DIR}/include/QtCore /external:I ${QT_DIR}/include/QtWidgets /external:I ${QT_DIR}/include/QtNetwork /external:W0 ${CMAKE_CXX_FLAGS}")
else ()
    message(FATAL_ERROR "Unsupported compiler!")
endif ()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_link_options("-L/usr/local/lib")
    add_link_options("-L/usr/local/opt/llvm/lib")
    add_link_options("-Wl,-rpath,/usr/local/opt/llvm/lib")
endif()

find_package(Qt6 COMPONENTS Core Widgets Network REQUIRED)

find_package(OpenCC REQUIRED)

include_directories(${Opencc_INCLUDE_PATH})

add_subdirectory(CLyric)

qt6_add_resources(QT_RESOURCES ui-qt/res/crystallyrics.qrc)

aux_source_directory(ui-qt CRYSTALLYRICS_SRC)

if (CMAKE_BUILD_TYPE MATCHES Rel* AND WIN32)
    add_executable(CrystalLyrics WIN32 ${CRYSTALLYRICS_SRC} ui-qt/res/CrystalLyrics.rc ${QT_RESOURCES})
elseif (APPLE)
    add_executable(CrystalLyrics MACOSX_BUNDLE ${CRYSTALLYRICS_SRC} ${QT_RESOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/ui-qt/res/icon.icns ${OPENCC_RESOURCES})

    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/ui-qt/res/icon.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

    file(GLOB_RECURSE OPENCC_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/opencc-res/*")
    foreach(RES_FILE ${OPENCC_RESOURCES})
        set_source_files_properties(${RES_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/opencc-files")
    endforeach(RES_FILE)

    set(MACOSX_BUNDLE_BUNDLE_NAME CrystalLyrics)
    set(MACOSX_BUNDLE_BUNDLE_VERSION 1)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER moe.datasone.crystallyrics)
    set(MACOSX_BUNDLE_ICON_FILE icon)
    set(MACOSX_BUNDLE_INFO_STRING CrystalLyrics)
    set(MACOSX_BUNDLE_LONG_VERSION_STRING 0.0.1)
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING 0.0.1)
    set_target_properties(CrystalLyrics PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/ui-qt/res/Info.plist.in)
else ()
    add_executable(CrystalLyrics ${CRYSTALLYRICS_SRC} ${QT_RESOURCES})
endif ()

target_link_libraries(CrystalLyrics PRIVATE Qt6::Core Qt6::Widgets Qt6::Network ${Opencc_LIBRARY} CLyric)

if (APPLE)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_custom_command(TARGET CrystalLyrics POST_BUILD
                COMMAND /opt/Qt/current/bin/macdeployqt ${CMAKE_CURRENT_BINARY_DIR}/CrystalLyrics.app -no-strip)
    else()
        add_custom_command(TARGET CrystalLyrics POST_BUILD
                COMMAND /opt/Qt/current/bin/macdeployqt ${CMAKE_CURRENT_BINARY_DIR}/CrystalLyrics.app)
    endif()
    add_custom_command(TARGET CrystalLyrics POST_BUILD
            COMMAND /bin/cp -R ${CMAKE_SOURCE_DIR}/opencc-res ${CMAKE_CURRENT_BINARY_DIR}/CrystalLyrics.app/Contents/Resources/opencc-files)
endif()

if (WIN32)
    message("${CMAKE_CURRENT_BINARY_DIR}/windeployqt" "${CMAKE_CURRENT_BINARY_DIR}/CrystalLyrics.exe")

    add_custom_command(TARGET CrystalLyrics POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/windeployqt"
            COMMAND "${QT_DIR}/bin/windeployqt.exe" --dir "${CMAKE_CURRENT_BINARY_DIR}/windeployqt" "${CMAKE_CURRENT_BINARY_DIR}/CrystalLyrics.exe")
endif()