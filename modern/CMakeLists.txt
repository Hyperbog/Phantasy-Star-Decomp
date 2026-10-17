cmake_minimum_required(VERSION 3.20)
project(ModernPhantasyStar LANGUAGES CXX)

add_library(modernps_core STATIC src/combat.cpp)
target_include_directories(modernps_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(modernps_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(modernps_core PRIVATE /W4 /WX)
else()
    target_compile_options(modernps_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(modernps_combat_tests tests/combat_tests.cpp)
    target_link_libraries(modernps_combat_tests PRIVATE modernps_core)
    add_test(NAME modernps_combat COMMAND modernps_combat_tests)
endif()
