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

    // Push can be used the same way std::bind is used
    // Just make sure f(args) evaluates to a function fo the form `void()`, i.e. the created packaged task can
    // be evaluated using operator()
    //
    // Alternatively Push can be used by directly by using a std::packaged_task<void()> as argument
    void Push(Task&& task) {
        std::lock_guard<std::mutex> lk(mut);
        queue.push(std::move(task));
        cv.notify_one();

    }

    template<typename F, typename... Args>
    std::future<void> PushTask(F&& f, Args&&... args) {
        auto b = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        Task pt(b);
        if(!pt.valid()) throw std::runtime_error("task not callable");
        auto fut = pt.get_future();
        Push(std::move(pt));
        return fut;
    }

    Task WaitAndPop() {
        std::unique_lock<std::mutex> lk(mut);
        cv.wait(lk, [this] { return done || !queue.empty(); });

        if(done)
            return Task([]{});

        auto t = std::move(queue.front());
        queue.pop();
        lk.unlock();

        return std::move(t);
    }

    Task TryAndPop() {
        std::lock_guard<std::mutex> lk(mut);
        if(queue.empty())
            return Task([]{});

        auto t = std::move(queue.front());
        queue.pop();

        return std::move(t);
    }

    bool IsDone() {
        std::lock_guard<std::mutex> lk(mut);
        return done;
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
        thread = std::thread([this]() {
            while(!queue->IsDone()) {
                auto t = std::move(queue->WaitAndPop());
                if(t.valid()) {
                    t();
                }
            }
        });
    }

    // move: queue and thread handle is moved, copy is not available for threads
    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;
    TaskManager(TaskManager&& other) = default;
    TaskManager& operator=(TaskManager&& other) = default;

    ~TaskManager() {
        queue->Shutdown();
        if (thread.joinable())
            thread.join();
    }

    std::shared_ptr<TaskQueue> GetQueue() {
        return queue;
    }

private:
    std::shared_ptr<TaskQueue> queue;
    std::thread thread;
};


#endif //TASKMANAGER_TASKMANAGER_H
