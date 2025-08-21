cmake_minimum_required(VERSION 3.16)
project(cpp_logger VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(CTest)

find_package(Threads REQUIRED)

file(GLOB_RECURSE LOGGER_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(logger_static STATIC ${LOGGER_SOURCES})
target_include_directories(logger_static PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(logger_static PUBLIC Threads::Threads)
target_compile_features(logger_static PUBLIC cxx_std_17)

add_library(logger_shared SHARED ${LOGGER_SOURCES})
target_include_directories(logger_shared PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(logger_shared PUBLIC Threads::Threads)
target_compile_features(logger_shared PUBLIC cxx_std_17)

add_executable(log_app "${CMAKE_CURRENT_SOURCE_DIR}/apps/log_app.cpp")
target_link_libraries(log_app PRIVATE logger_static)

if (BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    FetchContent_MakeAvailable(googletest)

    file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/*_gtest.cpp")

    add_executable(logger_tests ${TEST_SOURCES})
    target_link_libraries(logger_tests PRIVATE
            logger_static
            GTest::gtest_main
            Threads::Threads)

    include(GoogleTest)
    gtest_discover_tests(logger_tests)
endif()

foreach(tgt IN ITEMS logger_static logger_shared log_app)
    target_compile_options(${tgt} PRIVATE -Wall -Wextra -Wpedantic)
endforeach()
add_executable(stats_collector apps/stats_collector.cpp src/stats.cpp)
target_include_directories(stats_collector PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(stats_collector PRIVATE logger_static Threads::Threads)
