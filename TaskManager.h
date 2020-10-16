//
// Created by dev on 15.10.20.
//

#ifndef TASKMANAGER_TASKMANAGER_H
#define TASKMANAGER_TASKMANAGER_H

#include <thread>
#include <future>
#include <vector>
#include <queue>
#include <chrono>
#include <iostream>
#include <functional>

//template<class Function>
//class Task {
//    // can be passed into a std::thread
//    explicit Task(std::packaged_task<Function>&& pt) : pt_(pt) {}
//
//private:
//    std::packaged_task<Function> pt_;
//};

class TaskQueue {
    typedef std::packaged_task<void()> Task;
public:
    // TODO: thread safety and all
    size_t Size() {
        return queue.size();
    }
    size_t Empty() {
        return queue.empty();
    }
    void AddTask(Task&& task) {
        queue.push(std::move(task));
    }

    template<typename F, typename... Args>
    std::future<void> AddTask(F&& f, Args&&... args) {
        auto b = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        Task pt(b);
        auto fut = pt.get_future();
        queue.push(std::packaged_task<void()>(b));
        return fut;
    }

    Task GetTask() {
        if(queue.empty()) return {};
        auto t = std::move(queue.front());
        queue.pop();
        return std::move(t);
    }

private:
    std::queue<Task> queue;
};

class TaskManager {
public:
    // simple version for now with single queue and single thread already existing
    void Launch() {
        active = true;
        thread = std::thread([this]() {
            while(active) {
                if(!queue.Empty()) {
                    auto t = std::move(queue.GetTask());
                    t();
                } else {
                    using namespace std::chrono_literals;
                    std::cout << "sleep for 1s...\n";
                    std::this_thread::sleep_for(100ms);
                }
            }
        });
    }

    void Shutdown() {
        std::cout << "shutdown\n";
        active = false;
        if(thread.joinable())
            thread.join();
    }

    TaskQueue& GetQueue() {
        return queue;
    }

private:

    TaskQueue queue;
    std::thread thread;
    bool active;
};


#endif //TASKMANAGER_TASKMANAGER_H
