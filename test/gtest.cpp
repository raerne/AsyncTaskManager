#include "gtest/gtest.h"

#include <iostream>
#include <TaskManager.h>
#include "DummyProcess.h"

constexpr int VECNUM = 20;

class TaskManagerTest : public ::testing::Test {
protected:
    TaskManagerTest() : p(VECNUM) {}

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

    p.set(5); // synchronousely
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

TEST_F (TaskManagerTest, MoveQueue) {

    auto fut = tm.PushTask(&DummyProcess::set, &p, 20);
    TaskManager newtm = std::move(tm);

    auto fut2 = newtm.PushTask(&DummyProcess::double_nums, &p);

    EXPECT_NO_THROW(fut.wait());
    EXPECT_NO_THROW(fut2.wait());
    EXPECT_EQ((std::vector<int>(VECNUM, 40)), p.get());
}

TEST_F (TaskManagerTest, MoveTaskManager) {

    p.set(10);
    auto fut = tm.PushTask(&DummyProcess::double_nums, &p);
    std::this_thread::sleep_for(1ms);
    TaskManager newtm{std::move(tm)};

    auto fut2 = newtm.PushTask(&DummyProcess::double_nums, &p);
    EXPECT_NO_THROW(fut.wait());
    EXPECT_NO_THROW(fut2.wait());
    EXPECT_EQ((std::vector<int>(VECNUM, 40)), p.get());
}

TEST_F (TaskManagerTest, AssignTaskManager) {

    p.set(10);
    auto fut = tm.PushTask(&DummyProcess::double_nums, &p);

    TaskManager tmp = std::move(tm);
    auto fut2 = tmp.PushTask(&DummyProcess::double_nums, &p);

    TaskManager tmp2;
    tmp2 = std::move(tmp);

    auto fut3 = tmp2.PushTask(&DummyProcess::double_nums, &p);

    EXPECT_NO_THROW(fut.wait());
    EXPECT_NO_THROW(fut2.wait());
    EXPECT_NO_THROW(fut3.wait());

    EXPECT_EQ((std::vector<int>(VECNUM, 80)), p.get());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}