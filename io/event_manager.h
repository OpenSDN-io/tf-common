/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#ifndef SRC_IO_EVENT_MANAGER_H_
#define SRC_IO_EVENT_MANAGER_H_

#pragma once

#include <tbb/spin_mutex.h>
#include <boost/asio/io_service.hpp>

#include "base/util.h"

//
// Wrapper around boost::io_service.
//
// The mutex and the related assertions in Run, RunOnce and Poll are used
// to detect the case where multiple threads are running the EventManager.
// This typically happens in unit tests that inadvertently call RunOnce or
// Poll directly or indirectly after having started a ServerThread (which
// calls Run).
//
class EventManager {
public:
    EventManager();

    // Run until shutdown.
    void Run();

    // Run at most once.
    size_t RunOnce();

    // Run all ready handlers, without blocking.
    size_t Poll();

    void Shutdown();

    // Information whether mutex is already acquired
    bool IsRunning() const;

    boost::asio::io_context *io_service() { return &io_service_; }

private:
    // Atomic mutex lock operation and changing the running_ flag
    void Lock();

    // Atomic mutex unlock operation and changing the running_ flag
    void Unlock();

    boost::asio::io_context io_service_;
    bool shutdown_;
    tbb::spin_mutex io_mutex_;
    bool running_;
    tbb::spin_mutex guard_running_;

    DISALLOW_COPY_AND_ASSIGN(EventManager);
};

#endif  // SRC_IO_EVENT_MANAGER_H_
