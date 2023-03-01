cmake_minimum_required(VERSION 3.16.3)
project(server)

set(CMAKE_C_FLAGS "-g -O0 -Wall -Wextra -Wpedantic -Wno-unused-variable -Wno-unknown-pragmas -Wno-sign-compare -Wwrite-strings -Wno-unused ")
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDART_REQURED ON)
add_executable(server main.c)

find_library(EV_LIBRARY
        NAMES libev ev
        PATHS /usr/local/lib /usr/lib )

target_link_libraries(server PRIVATE ${EV_LIBRARY})

