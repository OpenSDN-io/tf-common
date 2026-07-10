/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#ifndef __EVENT_MANAGER_TEST_H__
#define __EVENT_MANAGER_TEST_H__

#include <atomic>
#include <memory>

#include "io/event_manager.h"

#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

#include "base/logging.h"
#include "base/task.h"

class ServerThread {
public:
    explicit ServerThread(EventManager *evm)
    : evm_(evm), tbb_scheduler_(nullptr) {
    }
    void Run() {
        tbb_scheduler_.reset(
            std::make_unique<oneapi::tbb::task_arena>(
                (TaskScheduler::GetThreadCount() + 1)).release());
        running_ = true;
        evm_->Run();
        running_ = false;
        tbb_scheduler_->terminate();
    }

    static void *ThreadRun(void *objp) {
        ServerThread *obj = reinterpret_cast<ServerThread *>(objp);
        obj->Run();
        return NULL;
    }

    void Start() {
        thread_.reset(std::make_unique<boost::thread>(
            &ServerThread::Run, this).release());
    }

    void Join() {
        thread_->join();
    }

private:
    std::unique_ptr<boost::thread> thread_;
    std::atomic<bool> running_;
    EventManager *evm_;
    std::unique_ptr<oneapi::tbb::task_arena> tbb_scheduler_;
};



#endif
