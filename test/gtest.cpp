#include "gtest/gtest.h"

#include <iostream>
#include "../src/TaskManager.h"
#include "DummyProcess.h"

constexpr int VECNUM = 20;

class TaskManagerTest : public ::testing::Test {
protected:
    TaskManagerTest() : p(VECNUM){
        queue = tm.GetQueue();
    }

public:
    DummyProcess p;
    TaskManager tm;
    std::shared_ptr<TaskQueue> queue;
};

TEST_F(TaskManagerTest, PackagedTasksEmpty) {

    std::packaged_task<void()> tt([]{});
    auto f = tt.get_future();
    queue->Push(std::move(tt));

    EXPECT_NO_THROW(
            f.get();
    );

    auto f2 = queue->PushTask([]{});

    EXPECT_NO_THROW(
            f2.get();
    );

}

TEST_F(TaskManagerTest, Exception) {

    struct Test {
        void throw_exception() {
            throw std::runtime_error("test");
        }
    } t;

    auto exfut = queue->PushTask(&Test::throw_exception, &t);
    EXPECT_THROW(exfut.get(), std::runtime_error);

}


TEST_F(TaskManagerTest, PackagedTasks) {
    struct Test {
        int ret;
        void set(int i) { ret = i; }
    } t;

    auto f = queue->PushTask(&Test::set, &t,5);
    f.get();
    EXPECT_EQ(5, t.ret);
}


TEST_F (TaskManagerTest, Setup) {
    auto fut = queue->PushTask(&DummyProcess::set, &p, 5);
    fut.wait();

    p.double_nums();
    EXPECT_EQ((std::vector<int>(VECNUM, 10)), p.get());
}

TEST_F (TaskManagerTest, SyncAsync) {

    p.set(5); // syncronousely
    auto fut = queue->PushTask(&DummyProcess::set, &p, 10); // async

    fut.wait(); // block until async done
    p.double_nums();

    EXPECT_EQ((std::vector<int>(VECNUM, 20)), p.get());
}

TEST_F (TaskManagerTest, QueueSyncOrder) {

    queue->PushTask(&DummyProcess::set, &p, 10); // async
    queue->PushTask(&DummyProcess::set, &p, 20); // async
    queue->PushTask(&DummyProcess::set, &p, 30); // async
    queue->PushTask(&DummyProcess::set, &p, 40); // async
    auto fut = queue->PushTask(&DummyProcess::set, &p, 50); // async

    fut.wait(); // block until async done
    EXPECT_EQ((std::vector<int>(VECNUM, 50)), p.get());
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}