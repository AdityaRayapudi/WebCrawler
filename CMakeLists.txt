cmake_minimum_required(VERSION 3.10)

project(crawler)

find_package(c-ares REQUIRED)

add_executable(${PROJECT_NAME} Frontier.cpp)

target_link_libraries(crawler PRIVATE c-ares::cares PRIVATE hiredis)

configure_file(config.h.in config.h)

target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_BINARY_DIR}")


