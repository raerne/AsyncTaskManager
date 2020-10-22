#include "gtest/gtest.h"

#include <iostream>
#include <TaskManager.h>
#include "DummyProcess.h"

constexpr int VECNUM = 20;

class TaskManagerTest : public ::testing::Test {
protected:
    TaskManagerTest() : p(VECNUM){}

public:
    DummyProcess p;
    TaskManager tm;
};

TEST_F(TaskManagerTest, Exception) {

    struct Test {
        void throw_exception() {
            throw std::runtime_error("test");
        }
    } t;

    auto exfut = tm.PushTask(&Test::throw_exception, &t);
    EXPECT_THROW(exfut.get(), std::runtime_error);

}


TEST_F(TaskManagerTest, PackagedTasks) {
    struct Test {
        int ret;
        void set(int i) { ret = i; }
    } t;

    auto f = tm.PushTask(&Test::set, &t,5);
    f.get();
    EXPECT_EQ(5, t.ret);
}


TEST_F (TaskManagerTest, Setup) {
    auto fut = tm.PushTask(&DummyProcess::set, &p, 5);
    fut.wait();

    p.double_nums();
    EXPECT_EQ((std::vector<int>(VECNUM, 10)), p.get());
}

TEST_F (TaskManagerTest, SyncAsync) {

    p.set(5); // syncronousely
    auto fut = tm.PushTask(&DummyProcess::set, &p, 10); // async

    fut.wait(); // block until async done
    p.double_nums();

    EXPECT_EQ((std::vector<int>(VECNUM, 20)), p.get());
}

TEST_F (TaskManagerTest, QueueSyncOrder) {

    tm.PushTask(&DummyProcess::set, &p, 10); // async
    tm.PushTask(&DummyProcess::set, &p, 20); // async
    tm.PushTask(&DummyProcess::set, &p, 30); // async
    tm.PushTask(&DummyProcess::set, &p, 40); // async
    auto fut = tm.PushTask(&DummyProcess::set, &p, 50); // async

    fut.wait(); // block until async done
    EXPECT_EQ((std::vector<int>(VECNUM, 50)), p.get());
}

TEST_F (TaskManagerTest, ExceptNoExcept) {

    auto exfut = tm.PushTask(&DummyProcess::throw_ex, &p); // async
    auto exfut2 = tm.PushTask(&DummyProcess::throw_ex, &p); // async
    auto fut = tm.PushTask(&DummyProcess::set, &p, 20); // async

    EXPECT_THROW(exfut.get(), std::runtime_error);
    EXPECT_NO_THROW(exfut2.wait()); // get would throw however we don't check the shared state
    EXPECT_NO_THROW(fut.get());
    EXPECT_EQ((std::vector<int>(VECNUM, 20)), p.get());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}