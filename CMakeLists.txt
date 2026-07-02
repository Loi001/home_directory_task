cmake_minimum_required(VERSION 3.15)
project(HomeDirectoryTask CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(nlohmann_json REQUIRED)
find_package(doctest REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(httplib REQUIRED)

add_executable(home_directory_task main.cpp)

target_link_libraries(home_directory_task PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(home_directory_task PRIVATE doctest::doctest)
target_link_libraries(home_directory_task PRIVATE spdlog::spdlog)
target_link_libraries(home_directory_task PRIVATE httplib::httplib)

enable_testing()

add_test(NAME validate_directory_path
         COMMAND home_directory_task --test=validate_directory_path --exit)

add_test(NAME walk_directory
         COMMAND home_directory_task --test=walk_directory --exit
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_test(NAME all_tests
         COMMAND home_directory_task --test=all --exit)
         
set_tests_properties(validate_directory_path walk_directory all_tests
    PROPERTIES TIMEOUT 30)