/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 * Copyright Contributors to the OpenSDN project
 */

#include "base/test/task_test_util.h"
#include "io/test/event_manager_test.h"
#include "testing/gunit.h"

using namespace std;

class EventManagerTest : public ::testing::Test {
protected:
    EventManagerTest() : thread_(&evm_) { }

    virtual void SetUp() {
        thread_.Start();
    }

    virtual void TearDown() {
        evm_.Shutdown();
        thread_.Join();
    }

    EventManager evm_;
    ServerThread thread_;
};

typedef EventManagerTest EventManagerDeathTest;

TEST_F(EventManagerDeathTest, Poll) {
    while (!evm_.IsRunning())
        usleep(1000);
    TASK_UTIL_EXPECT_EXIT(evm_.Poll(), ::testing::KilledBySignal(SIGABRT),
                          ".*Lock.*");
}

TEST_F(EventManagerDeathTest, RunOnce) {
    while (!evm_.IsRunning())
        usleep(1000);
    TASK_UTIL_EXPECT_EXIT(evm_.RunOnce(), ::testing::KilledBySignal(SIGABRT),
                          ".*Lock.*");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    int result = RUN_ALL_TESTS();
    TaskScheduler::GetInstance()->Terminate();
    return result;
}
