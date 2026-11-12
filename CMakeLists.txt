cmake_minimum_required(VERSION 3.20)

project(avl_lab VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if(ENABLE_SANITIZERS)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "ENABLE_SANITIZERS requires GCC or Clang")
    endif()
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer
                        -fno-sanitize-recover=all)
    add_link_options(-fsanitize=address,undefined)
endif()

add_library(avl_tree INTERFACE)
target_include_directories(avl_tree INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(avl_tree INTERFACE cxx_std_20)

add_executable(avl_console main.cpp)
target_link_libraries(avl_console PRIVATE avl_tree)

add_executable(avl_benchmark benchmarks/main.cpp)
target_link_libraries(avl_benchmark PRIVATE avl_tree)

include(CTest)
if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)

    add_executable(avl_tests tests/build_smoke_test.cpp)
    target_link_libraries(avl_tests PRIVATE avl_tree GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(avl_tests)
endif()
