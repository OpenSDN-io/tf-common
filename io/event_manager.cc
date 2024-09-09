/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#include <string>

#include "io/event_manager.h"
#include "base/logging.h"
#include "io/io_log.h"

using boost::asio::io_context;

SandeshTraceBufferPtr IOTraceBuf(SandeshTraceBufferCreate(IO_TRACE_BUF, 1000));

EventManager::EventManager() : shutdown_(false), running_(false) {
}

void EventManager::Shutdown() {
    shutdown_ = true;
    io_service_.stop();
}

void EventManager::Run() {
    Lock();
    io_context::work work(io_service_);
    do {
        if (shutdown_) break;
        boost::system::error_code ec;
        try {
            io_service_.run(ec);
            if (ec) {
                EVENT_MANAGER_LOG_ERROR("io_service run failed: " <<
                                        ec.message());
                break;
            }
        } catch (std::exception &except) {
            static std::string what = except.what();
            EVENT_MANAGER_LOG_ERROR("Exception caught in io_service run : " <<
                                    what);
            assert(false);
        } catch (...) {
            EVENT_MANAGER_LOG_ERROR("Exception caught in io_service run : "
                                    "bailing out");
            assert(false);
        }
    } while (true);
    Unlock();
}

size_t EventManager::RunOnce() {
    Lock();
    if (shutdown_) {
        Unlock();
        return 0;
    }
    boost::system::error_code err;
    size_t res = io_service_.run_one(err);
    if (res == 0)
        io_service_.reset();
    Unlock();
    return res;
}

size_t EventManager::Poll() {
    Lock();
    if (shutdown_) {
        Unlock();
        return 0;
    }
    boost::system::error_code err;
    size_t res = io_service_.poll(err);
    if (res == 0)
        io_service_.reset();
    Unlock();
    return res;
}

bool EventManager::IsRunning() const {
    return running_;
}

void EventManager::Lock() {
    tbb::spin_mutex::scoped_lock lock(guard_running_);
    assert(io_mutex_.try_lock());
    running_ = true;
}

void EventManager::Unlock() {
    tbb::spin_mutex::scoped_lock lock(guard_running_);
    io_mutex_.unlock();
    running_ = false;
}
