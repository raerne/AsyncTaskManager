cmake_minimum_required(VERSION 3.17)
project(TaskManager)

set(CMAKE_CXX_STANDARD 17)
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)

add_library(taskmanager TaskManager.cpp TaskManager.h)
add_executable(TaskManagerExe main.cpp)
target_link_libraries(TaskManagerExe PRIVATE taskmanager Threads::Threads)

