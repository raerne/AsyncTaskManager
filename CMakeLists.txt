cmake_minimum_required(VERSION 3.17)
project(TaskManager)

set(CMAKE_CXX_STANDARD 17)
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)

add_executable(TaskManager main.cpp TaskManager.cpp TaskManager.h)
target_link_libraries(TaskManager PRIVATE Threads::Threads)

