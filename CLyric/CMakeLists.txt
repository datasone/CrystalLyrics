find_package(CURL CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

aux_source_directory(. CLYRIC_SRC)
add_library(CLyric ${CLYRIC_SRC})
target_link_libraries(CLyric PRIVATE CURL::libcurl nlohmann_json::nlohmann_json ZLIB::ZLIB)

enable_testing()

find_package(GTest CONFIG REQUIRED)
aux_source_directory(tests CLYRICTESTS_SRC)
add_executable(CLyricTests ${CLYRICTESTS_SRC})
target_link_libraries(CLyricTests PRIVATE GTest::gtest GTest::gtest_main CLyric)

add_test(
        NAME CLyricTests
        COMMAND CLyricTests
)