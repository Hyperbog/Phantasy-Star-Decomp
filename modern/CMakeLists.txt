cmake_minimum_required(VERSION 3.20)
project(ModernPhantasyStar LANGUAGES CXX)

add_library(modernps_core STATIC
    src/combat.cpp src/world.cpp src/character.cpp src/content.cpp)
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

    add_executable(modernps_world_tests tests/world_tests.cpp)
    target_link_libraries(modernps_world_tests PRIVATE modernps_core)
    add_test(NAME modernps_world COMMAND modernps_world_tests)

    add_executable(modernps_character_tests tests/character_tests.cpp)
    target_link_libraries(modernps_character_tests PRIVATE modernps_core)
    add_test(NAME modernps_character COMMAND modernps_character_tests)

    add_executable(modernps_content_tests tests/content_tests.cpp)
    target_link_libraries(modernps_content_tests PRIVATE modernps_core)
    target_compile_definitions(modernps_content_tests PRIVATE
        MODERNPS_CONTENT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/content")
    add_test(NAME modernps_content COMMAND modernps_content_tests)
endif()

if(BUILD_TESTING)
    add_executable(modernps_integration_tests tests/integration_tests.cpp)
    target_link_libraries(modernps_integration_tests PRIVATE modernps_core)
    target_compile_definitions(modernps_integration_tests PRIVATE
        MODERNPS_CONTENT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/content")
    add_test(NAME modernps_integration COMMAND modernps_integration_tests)
endif()
