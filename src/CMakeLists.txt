cmake_minimum_required(VERSION 3.17)
project(TaskManagerExe)

set(CMAKE_CXX_STANDARD 17)
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)

add_executable(TaskManagerExe main.cpp)
target_include_directories(TaskManagerExe PRIVATE ../lib/)
target_link_libraries(TaskManagerExe PRIVATE taskmanager Threads::Threads)