/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#ifndef __ctrlplane__task_trigger__
#define __ctrlplane__task_trigger__

#include <atomic>

#include <boost/function.hpp>

class TaskTrigger {
public:
    typedef boost::function<bool()> FunctionPtr;
    TaskTrigger(const FunctionPtr& func, int task_id, int task_instance);
    ~TaskTrigger();
    void Set();
    void Reset();
    // For Test only
    void set_disable() {
        bool current = disabled_.exchange(true);
        assert(!current);
    }
    void set_enable() {
        bool current = disabled_.exchange(false);
        assert(current);
        Set();
    }
    bool disabled() {
        return disabled_;
    }

    // For Test only
    void set_deferred() {
        bool current = deferred_.exchange(true);
        assert(!current);
    }
    void clear_deferred() {
        bool current = deferred_.exchange(false);
        assert(current);
    }
    bool deferred() const { return deferred_; }
    bool IsSet() const { return trigger_; }

private:
    class WorkerTask;

    FunctionPtr func_;
    int task_id_;
    int task_instance_;
    std::atomic<bool> trigger_;
    std::atomic<bool> disabled_;
    std::atomic<bool> deferred_;
};

#endif /* defined(__ctrlplane__task_trigger__) */
