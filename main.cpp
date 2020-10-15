#include <iostream>
#include "TaskManager.h"
#include <chrono>

using namespace std::chrono_literals;

int main() {

    TaskManager tm;
    auto& q = tm.GetQueue();
    tm.Launch();

    std::packaged_task<void(int)> task([](int a) { std::cout << "a: " << a << std::endl;});
    q.AddTask(std::move(task));

    // tear down
    std::this_thread::sleep_for(1s);


    std::packaged_task<void(int)> t2([](int b) { std::cout << "b: " << b << std::endl;});
    q.AddTask(std::move(t2));

    // tear down
    std::this_thread::sleep_for(1s);


    tm.Shutdown();

    return 0;
}
