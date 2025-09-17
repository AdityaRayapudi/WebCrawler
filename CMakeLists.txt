cmake_minimum_required(VERSION 3.10)

project(crawler)

include_directories(cpp/include)

find_package(c-ares REQUIRED)

add_executable(${PROJECT_NAME} main.cpp cpp/src/WebScraper.cpp cpp/src/Frontier.cpp cpp/src/RedisReplyPtr.cpp cpp/src/DNS_Resolver.cpp)

target_link_libraries(crawler PRIVATE c-ares::cares hiredis)

configure_file(config.h.in config.h)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cpp/dependencies PUBLIC "${PROJECT_BINARY_DIR}")


