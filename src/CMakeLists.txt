project(TaskManagerExe)

find_package(Threads REQUIRED)

add_executable(TaskManagerExe main.cpp)
target_include_directories(TaskManagerExe PRIVATE ../lib/)
target_link_libraries(TaskManagerExe PRIVATE taskmanager Threads::Threads)