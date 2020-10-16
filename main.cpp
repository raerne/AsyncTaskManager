#include <iostream>
#include "TaskManager.h"
#include <chrono>
#include <functional>

using namespace std::chrono_literals;

class Process {
public:
    explicit Process(int n) : nums(n, 0) {}

    void set(int num) {
        std::lock_guard<std::mutex> lk(m);
        std::cout << "set\n";
        for(auto& n : nums) n = num;
    }

    void double_nums() {
        std::lock_guard<std::mutex> lk(m);
        std::cout << "double\n";
        for(auto& n : nums) {
            n *= 2;
        }
    }

    void double_nums_and_wait() {
        std::cout << "double and wait [1]...\n";
        std::this_thread::sleep_for(200ms);
        std::lock_guard<std::mutex> lk(m);
        std::cout << "double and wait [2]...\n";
        for(auto& n : nums) {
            n *= 2;
        }

    }

    void throw_ex() {
        throw std::runtime_error("test error");
    }

    void print() {
        std::lock_guard<std::mutex> lk(m);
        for(auto& n : nums) {
            std::cout << n << " ";
        }
        std::cout << std::endl;
    }

private:
    std::mutex m;
    std::vector<int> nums;
};

int main() {

//    std::packaged_task<void()> tt{};
//    tt();

    TaskManager tm;
    auto q = tm.GetQueue();

//    q.AddTask(std::move(task));
//
//    // tear down
//    std::this_thread::sleep_for(1s);
//
//
//    std::packaged_task<void()> t2([]() { std::cout << "process" << std::endl;});
//    q.AddTask(std::move(t2));
//
//    // tear down
//    std::this_thread::sleep_for(1s);

    Process p(20);

    auto exfut = q->AddTask(&Process::throw_ex, &p);
//    try {
//        exfut.get();
//    } catch(std::exception& e) {
//        std::cout << "caught exception: " << e.what() << std::endl;
//    }

    auto fut = q->AddTask(&Process::set, &p, 5);

//    std::packaged_task<void()> tp(std::bind(&Process::double_nums_and_wait, &p));
//    std::future<void> f;
//    f = tp.get_future();
//    q.AddTask(std::move(tp));



    std::cout << "block until ret..." << std::endl;
    fut.wait();
    std::cout << "done\n";

    p.double_nums();
    q->AddTask(&Process::double_nums_and_wait, &p);

//    p.set(1);
//    std::_Bind_helper<0, void (Process::*)(), Process *>::type bp;
//    bp = std::bind(&Process::double_nums, &p);
//    std::packaged_task<void()> tproc(&Process::double_nums, &p);
//    std::packaged_task<void()> tproc(bp);
//    q.AddTask(std::move(tproc));


    // tear down
    std::this_thread::sleep_for(1s);

    p.print();

    return 0;
}
