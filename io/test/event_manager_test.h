/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#ifndef __EVENT_MANAGER_TEST_H__
#define __EVENT_MANAGER_TEST_H__

#include <atomic>

#include "io/event_manager.h"

#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

#include "base/logging.h"
#include "base/task.h"

class ServerThread {
public:
    explicit ServerThread(EventManager *evm)
    : evm_(evm), tbb_scheduler_(NULL) {
    }
    void Run() {
        tbb_scheduler_.reset(
            new tbb::task_scheduler_init(TaskScheduler::GetThreadCount() + 1));
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
        thread_.reset(new boost::thread(&ServerThread::Run, this));
    }

    void Join() {
        thread_->join();
    }

private:
    boost::scoped_ptr<boost::thread> thread_;
    std::atomic<bool> running_;
    EventManager *evm_;
    boost::scoped_ptr<tbb::task_scheduler_init> tbb_scheduler_;
};


#endif
