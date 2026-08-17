cmake_minimum_required(VERSION 3.20)
project(proteus_backend_sangchi VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE PROTEUS_SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_library(proteus_backend STATIC ${PROTEUS_SOURCES})
target_include_directories(proteus_backend PUBLIC include)
target_compile_options(proteus_backend PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Werror>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>)

add_executable(proteus_demo examples/demo.cpp)
target_link_libraries(proteus_demo PRIVATE proteus_backend)

enable_testing()
add_executable(proteus_tests tests/test_main.cpp)
target_link_libraries(proteus_tests PRIVATE proteus_backend)
add_test(NAME proteus_tests COMMAND proteus_tests)
