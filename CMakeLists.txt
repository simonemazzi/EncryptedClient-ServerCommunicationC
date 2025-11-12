cmake_minimum_required(VERSION 3.30)
project(untitled4 C)

set(CMAKE_C_STANDARD 11)

add_executable(untitled4
        Client/client.c
        Client/client.h
        Server/server.c
        Server/server.h)
