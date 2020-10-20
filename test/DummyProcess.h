//
// Created by dev on 19.10.20.
//

#ifndef TASKMANAGER_DUMMYPROCESS_H
#define TASKMANAGER_DUMMYPROCESS_H

#include <vector>
#include <chrono>
#include <functional>
#include <mutex>
#include <thread>
#include <iostream>

using namespace std::chrono_literals;

class DummyProcess {

public:
    explicit DummyProcess(int n) : nums(n, 0) {}

    void set(int num) {
        std::lock_guard<std::mutex> lk(m);
        for(auto& n : nums) n = num;
    }

    std::vector<int> get() {
        return nums;
    }

    void double_nums() {
        std::lock_guard<std::mutex> lk(m);
        std::cout << "double...\n";
        for(auto& n : nums) {
            n *= 2;
        }
    }

    void double_nums_and_wait() {
//        std::cout << "double and wait [1]...\n";
        std::this_thread::sleep_for(200ms);
        std::lock_guard<std::mutex> lk(m);
//        std::cout << "double and wait [2]...\n";
        for(auto& n : nums) {
            n *= 2;
        }

    }

    void throw_ex() {
        throw std::runtime_error("test error");
    }

//    void print() {
//        std::lock_guard<std::mutex> lk(m);
//        for(auto& n : nums) {
//            std::cout << n << " ";
//        }
//        std::cout << std::endl;
//    }

private:
    std::mutex m;
    std::vector<int> nums;
};


#endif //TASKMANAGER_DUMMYPROCESS_H
