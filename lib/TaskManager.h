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
    ~TaskQueue() {
        std::cout << "taskqueue deleted\n";
        done = true; // not threadsafe
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
//        std::cout << "IsDone on this: " << std::hex << this << std::endl;
//        pthread_mutex_t* m = mut.native_handle();
//        int err = pthread_mutex_lock(m);
////
//        std::cout << "pthread_mutex_lock(" << err << ")\n";
//        bool d = done;
//        err = pthread_mutex_unlock(m);
//        std::cout << "pthread_mutex_unlock(" << err << ")\n";
//        return d;
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
        // join existing thread first
        if(thread.joinable()) {
            std::cout << "join thread at launch: " << std::hex << thread.native_handle() << std::endl;
            thread.join();
        }

        thread = std::thread([this]() {
            while(!queue.IsDone()) {
                std::cout << "queue: " << std::hex << &queue << "\n";
                auto t = std::move(queue.WaitAndPop());
                if(t.valid()) {
                    t();
                }
            }
            std::cout << "thread done, queue: " << std::hex << &queue << "\n";
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
        queue = std::move(other.queue);
        thread = std::move(other.thread);
        // old thread will exit due to the moved queue marking the old one as done
        // then we will relaunch a new thread
        LaunchThread();
        std::cout << "move constructed TaskManager " << std::hex << this << "\n";
    }
//    TaskManager& operator=(TaskManager&& other) = default;
    TaskManager& operator=(TaskManager&& other){
        queue = std::move(other.queue);
        thread = std::move(other.thread);
        LaunchThread();
//        thread = std::move(other.thread);
        std::cout << "move assigned TaskManager\n";
        return *this;
    }

    ~TaskManager() {
        queue.Shutdown();
        std::cout << "shutdown called TaskManager\n";
        if (thread.joinable()) {
            std::cout << "join thread: " << std::hex << thread.native_handle() << " " << this << std::endl;
            thread.join();
        } else {
            std::cout << "nonjoinable thread: " << std::hex << thread.native_handle() << " " << this << std::endl;

        }
    }

    TaskQueue* GetQueue() {
        return &queue;
    }

private:
//    std::shared_ptr<TaskQueue> queue;
    TaskQueue queue;
    std::thread thread;
};


#endif //TASKMANAGER_TASKMANAGER_H
