cmake_minimum_required(VERSION 3.15)
project(HomeDirectoryTask CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(nlohmann_json REQUIRED)

add_executable(home_directory_task main.cpp)
target_link_libraries(home_directory_task PRIVATE nlohmann_json::nlohmann_json)