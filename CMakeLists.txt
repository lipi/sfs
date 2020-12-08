cmake_minimum_required(VERSION 3.10)

project(fts)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories(src/)

add_executable(server src/server.c src/common.c)
add_executable(client src/client.c src/common.c)

target_link_libraries(server PRIVATE Threads::Threads)
