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
#include <type_traits>
#include <pthread.h>

class TaskQueue {

    // pop operation is not thread-safe if copy/move construction can throw
    typedef std::packaged_task<void()> Task;
    static_assert(std::is_nothrow_move_constructible_v<Task>);

public:

    TaskQueue() = default;
    ~TaskQueue() = default;
    TaskQueue(const TaskQueue&) = delete;
    TaskQueue& operator=(const TaskQueue&) = delete;

    TaskQueue(TaskQueue&& other) {
        {
            std::lock_guard<std::mutex> lk(other.mut);
            queue = std::move(other.queue);
            done = other.done;
            other.done = true;
        }
        other.cv.notify_all();
    }

    TaskQueue& operator=(TaskQueue&& other) {
        if(&other == this)
            return *this;

        {
            std::scoped_lock lk{other.mut, mut};
            queue = std::move(other.queue);
            done = other.done;
            other.done = true;
        }

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

    void Restart() {
        std::lock_guard<std::mutex> lk(mut);
        done = false;
    }

    void Shutdown() {
        {
            std::lock_guard<std::mutex> lk(mut);
            done = true;
        }
        cv.notify_all();
    }

    void Push(Task&& task) {
        {
            std::lock_guard<std::mutex> lk(mut);
            queue.push(std::move(task));
        }
        cv.notify_one();
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

        if(done || queue.empty())
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

    void LaunchThread() {
        thread = std::thread([this]() {
            std::cout << "launching thread\n";
            while(!queue.IsDone()) {
                auto t = std::move(queue.WaitAndPop());
                if(t.valid()) {
                    t();
                }
            }
            std::cout << "thread done\n";
        });
    }

    void Shutdown() {
        queue.Shutdown();
        if (thread.joinable()) {
            thread.join();
        }
    }

    TaskManager()
    {
        LaunchThread();
    }

    // move: queue and thread handle is moved, copy is not available for threads
    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;

    TaskManager(TaskManager&& other) {
        other.Shutdown();

        queue = std::move(other.queue);
        queue.Restart();
        LaunchThread();
    }

    TaskManager& operator=(TaskManager&& other){
        if(&other == this)
            return *this;

        // Shutdown will block until thread is done and queue is marked as done (but possibly non-empty)
        // Elements in this queue will be lost
        this->Shutdown();
        other.Shutdown();

        queue = std::move(other.queue);
        queue.Restart();
        LaunchThread();

        return *this;
    }

    ~TaskManager() {
        Shutdown();
    }

    // PushTask can be used the same way std::bind is used
    // Just make sure f(args) evaluates to a function fo the form `void()`
    template<typename F, typename... Args>
    std::future<void> PushTask(F&& f, Args&&... args) {
        auto b = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        std::packaged_task<void()> pt(b);
        if(!pt.valid()) throw std::runtime_error("task not callable");
        auto fut = pt.get_future();
        queue.Push(std::move(pt));
        return fut;
    }

private:
    TaskQueue queue;
    std::thread thread;
};


#endif //TASKMANAGER_TASKMANAGER_H
