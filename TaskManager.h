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

class TaskQueue {
    typedef std::packaged_task<void()> Task;
public:

    TaskQueue() = default;
    ~TaskQueue() = default;
    TaskQueue(const TaskQueue&) = delete;
    TaskQueue& operator=(const TaskQueue&) = delete;

    TaskQueue(TaskQueue&& other) {
        std::lock_guard<std::mutex> lk(other.mut);
        queue = std::move(other.queue);
        done = other.done;
        other.done = true;
        other.cv.notify_all();
    }

    TaskQueue& operator=(TaskQueue&& other) {
        std::lock_guard<std::mutex> lk(other.mut);
        queue = std::move(other.queue);
        done = other.done;
        other.done = true;
        other.cv.notify_all();
        return *this;
    }

    size_t Size() {
        std::lock_guard<std::mutex> lk(mut);
        return queue.size();
    }
    size_t Empty() {
        std::lock_guard<std::mutex> lk(mut);
        return queue.empty();
    }



    void Shutdown() {
        {
            std::lock_guard<std::mutex> lk(mut);
            done = true;
        }
        cv.notify_all();
    }

    // AddTask can be used the same way std::bind is used
    // Just make sure f(args) evaluates to a function fo the form `void()`, i.e. the created packaged task can
    // be evaluated using operator()
    //
    // Alternatively AddTask can be used by directly moving a std::packaged_task<void()> as argument
    void AddTask(Task&& task) {
        std::lock_guard<std::mutex> lk(mut);
        queue.push(std::move(task));
        cv.notify_one();
    }

    template<typename F, typename... Args>
    std::future<void> AddTask(F&& f, Args&&... args) {
        auto b = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        Task pt(b);
        auto fut = pt.get_future();
        AddTask(std::move(pt));
        return fut;
    }

    Task WaitAndGetTask() {
        std::unique_lock<std::mutex> lk(mut);
        std::cout << "waiting for task\n";
        cv.wait(lk, [this] { return done || !queue.empty(); });
        std::cout << "done waiting for task\n";
        if(done) return Task([](){std::cout << "done task\n"; });

        auto t = std::move(queue.front());
        queue.pop();
        lk.unlock();
        return std::move(t);
    }

    Task TryAndGetTask() {
        std::lock_guard<std::mutex> lk(mut);
        if(queue.empty()) return {};
        auto t = std::move(queue.front());
        queue.pop();
        return std::move(t);
    }

private:
    mutable std::mutex mut;
    std::condition_variable cv;
    std::queue<Task> queue;
    bool done{false};
};

class TaskManager {
public:
    // simple version for now with single queue and single thread already existing
    TaskManager() : queue(std::make_shared<TaskQueue>())
    {
        if(!thread_done) return;
        thread_done = false;

        thread = std::thread([this]() {
            while(!thread_done) {
                auto t = std::move(queue->WaitAndGetTask());
                t();
            }
        });
    }

    ~TaskManager() {
        std::cout << "shutdown\n";
        queue->Shutdown();

        if(!thread_done) {
            thread_done = true;
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    std::shared_ptr<TaskQueue> GetQueue() {
        return queue;
    }

private:
    std::shared_ptr<TaskQueue> queue;
    std::thread thread;

    std::atomic<bool> thread_done{true};
};


#endif //TASKMANAGER_TASKMANAGER_H
