cmake_minimum_required(VERSION 3.10)

project(crawler)

include_directories(include)

find_package(c-ares REQUIRED)

add_executable(${PROJECT_NAME} main.cpp src/Frontier.cpp src/RedisReplyPtr.cpp src/DNS_Resolver.cpp)

target_link_libraries(crawler PRIVATE c-ares::cares PRIVATE hiredis)

configure_file(config.h.in config.h)

target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_BINARY_DIR}")


