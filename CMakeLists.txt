cmake_minimum_required(VERSION 3.26)
project(kolos C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_VERBOSE_MAKEFILE 1)

add_executable(ser server.c)
add_executable(cli client.c)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(ser PRIVATE Threads::Threads rt)
target_link_libraries(cli PRIVATE Threads::Threads rt)
