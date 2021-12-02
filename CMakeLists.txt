cmake_minimum_required(VERSION 3.20)
project(systemsoftware2 C)

set(CMAKE_C_STANDARD 99)

add_executable(systemsoftware2 server.c client.c constants.h)
