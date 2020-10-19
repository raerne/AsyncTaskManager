cmake_minimum_required(VERSION 3.17)
project(TaskManager)

set(CMAKE_CXX_STANDARD 17)
set(THREADS_PREFER_PTHREAD_FLAG ON)

add_subdirectory(src)
add_subdirectory(test)

