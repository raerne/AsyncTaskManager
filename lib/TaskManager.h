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
#include<type_traits>

#include <pthread.h>

class TaskQueue {
    typedef std::packaged_task<void()> Task;
    // pop operation is not thread-safe if copy/move construction can throw
    static_assert(std::is_nothrow_move_constructible_v<Task>);

public:

    TaskQueue() = default;
//    ~TaskQueue() = default;

    // Always call Shutdown before destruction, otherwise undefined behaviour
    ~TaskQueue() {
        std::cout << "taskqueue deleted\n";
//        done = true; // not threadsafe
//        cv.notify_all();
    }
    TaskQueue(const TaskQueue&) = delete;
    TaskQueue& operator=(const TaskQueue&) = delete;

    TaskQueue(TaskQueue&& other) {
        std::cout << "move construct TaskQueue\n";
        {
            std::lock_guard<std::mutex> lk(other.mut);
            queue = std::move(other.queue);
            done = other.done;
            other.done = true;
            std::cout << "moved queue with " << queue.size() << " elements\n";
        }
        other.cv.notify_all();
    }

    TaskQueue& operator=(TaskQueue&& other) {
        std::cout << "move assign TaskQueue\n";
        if(&other == this)
            return *this;

        {
            std::scoped_lock lk{other.mut, mut};
            queue = std::move(other.queue);
            std::cout << "moved queue with " << queue.size() << " elements\n";
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

    Task WaitAndPop() {
        std::unique_lock<std::mutex> lk(mut);
        cv.wait(lk, [this] { std::cout << "wakeup with (done, empty) = (" << done << ", " << queue.empty() << ")" << std::endl; return done || !queue.empty(); });

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
            std::cout << "launching thread: " << std::hex << this << std::endl;
            while(!queue.IsDone()) {
                std::cout << "queue: " << std::hex << &queue << "\n";
                auto t = std::move(queue.WaitAndPop());
                if(t.valid()) {
                    t();
                }
            }
            std::cout << "thread done: " << std::hex << this << std::endl;
        });
    }

    // simple version for now with single queue and single thread already existing
    TaskManager() //: queue(std::make_shared<TaskQueue>())
    {
        LaunchThread();
    }

    // move: queue and thread handle is moved, copy is not available for threads
    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;
//    TaskManager(TaskManager&& other) = default;
    TaskManager(TaskManager&& other) {

        // TODO a bit much custom code here
        other.Shutdown();
        this->Shutdown();

        queue = std::move(other.queue);
        queue.Restart();
        LaunchThread();

        std::cout << "move constructed TaskManager " << std::hex << this << "\n";
    }
//    TaskManager& operator=(TaskManager&& other) = default;
    TaskManager& operator=(TaskManager&& other){
        if(&other == this)
            return *this;

        // Move shutdown queue (so no thread is waiting) and restart a new (not moved) thread on moved queue
        this->Shutdown();
        other.Shutdown();

        queue = std::move(other.queue);
        queue.Restart();

        LaunchThread();

        std::cout << "move assigned TaskManager\n";
        return *this;
    }

    void Shutdown() {
        queue.Shutdown();
        if (thread.joinable()) {
            thread.join();
        }
    }

    ~TaskManager() {
        Shutdown();
    }

    TaskQueue* GetQueue() {
        return &queue;
    }

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
//    std::shared_ptr<TaskQueue> queue;
    TaskQueue queue;
    std::thread thread;
};


#endif //TASKMANAGER_TASKMANAGER_H
