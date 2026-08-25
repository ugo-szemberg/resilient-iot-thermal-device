cmake_minimum_required(VERSION 3.16)
project(gateway)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(gateway "main.c")

if(WIN32)
    target_link_libraries(gateway ws2_32)
endif()