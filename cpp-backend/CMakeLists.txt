cmake_minimum_required(VERSION 3.2)
project(epoll_server VERSION 1.0 LANGUAGES CXX)

set(CMAKE_C_COMPILER   "/usr/local/gcc-15.1/bin/gcc"  CACHE PATH "" FORCE)
set(CMAKE_CXX_COMPILER "/usr/local/gcc-15.1/bin/g++" CACHE PATH "" FORCE)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "15.0")
        message(FATAL_ERROR "GCC version must be at least 15. Detected: ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS    OFF)

find_path(MYSQLCPPCONN_INCLUDE_DIR
        NAMES cppconn/driver.h
        PATHS /usr/include /usr/include/mysql
)
find_library(MYSQLCPPCONN_LIBRARY
        NAMES mysqlcppconn
        PATHS /usr/lib /usr/lib/x86_64-linux-gnu
)

find_package(CURL REQUIRED)

add_executable(server main.cpp server/server.cpp http/http_session.cpp)

target_include_directories(server
        PRIVATE
        ${MySQLConnectorCPP_INCLUDE_DIRS}
)

target_link_libraries(server
        PRIVATE
        ${MySQLConnectorCPP_LIBRARIES}
)

target_link_libraries(server
        PRIVATE
        CURL::libcurl
)