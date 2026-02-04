/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#ifndef ctrlplane_task_h
#define ctrlplane_task_h

#include <boost/scoped_ptr.hpp>
#include <boost/intrusive/list.hpp>
#include <map>
#include <shared_mutex>
#include <vector>
#include <mutex>
// TODO: change deprecated tbb::task to something else
#define TBB_SUPPRESS_DEPRECATED_MESSAGES 1
#include <tbb/task.h>
#include <tbb/task_scheduler_init.h>
#include "base/util.h"

class TaskGroup;
class TaskEntry;
class SandeshTaskScheduler;
class TaskTbbKeepAwake;
class EventManager;
class TaskMonitor;
class TaskScheduler;

/// @brief Task is a class to describe a computational task within OpenSDN
/// control plane applications. A task is a labelled sequence of instructions
/// (code) and data processed by them in a single thread. OpenSDN Task wraps
/// over tbb::task. Tasks are labelled using a pair of numbers:
/// - task code ID (or task ID), which corresponds to a version of code
/// to run into the task;
/// - task data ID, which corresponds to a version of data supplied
/// to the task and processed by code.
///
/// This labelling is used to apply execution policies determining
/// which tasks are allowed to be executed in parallel with others and
/// which tasks are forbidden to run in parallel. The labels are expressed as
/// <tcid, tdid>, where *tcid* is a task code ID and *tdid* is a task
/// data ID.
///
/// If a task with *tcid* has *tdid* equal to -1, then any number of
/// tasks with label <tcid,-1> can run at a time. If a task has task data ID
/// larger or equal to 0, then only one task with this
/// given label (i.e. <tcid,tdid>) can run at a time.
///
/// When there are multiple tasks ready to run, they are scheduled in their
/// order of enqueue.
///
/// Additionaly, parallel execution of tasks can be managed using task
/// execution policies.
/// Task execution policies are specified per a task with the given code
/// ID *tcid0* and arbitrary task data ID in a form of a list of task
/// labels:
/// <tcid0,-1> => <tcid1,tdid1>, <tcid2,tdid2>, ..., <tcidN,tdidN>.
///
/// This list specifies which tasks can't be executed in parallel with
/// a task having specified task code ID (tcid0).
/// Each label <tcidN, tdidN> in a policy (i.e. the list) is called task
/// exclusion because it specifies that the task with this task code ID and
/// task data ID cannot run in parallel with <tcid0, -1>.
/// When *tdid* is equal to -1 in a task exclusion, it corresponds to
/// wildcard (*), i.e. all possible values of the task data ID *tdid*.
///
/// For example, if we have a policy:
/// - <tcid0,-1> => <tcid1, -1> <tcid2, 2> <tcid3, 3>
///
/// The policy states that:
/// - Task <tcid0,-1> cannot run as long as <tcid1, -1> is running;
/// - Task <tcid0, 2> cannot run as long as task <tcid2, 2> is running;
/// - Task <tcid0, 3> cannot run as long as task <tcid3, 3> is running.
///.
/// Policy rules are symmetric. I.e., the previous example states also:
/// - Task <tcid1,-1> cannot run as long as <tcid0,-1> is running;
/// - Task <tcid2, 2> cannot run as long as task <tcid0, 2> is running;
/// - Task <tcid3, 3> cannot run as long as task <tcid0, 3> is running.
///
class Task {
public:

    /// @brief Task states.
    enum State {

        /// @brief A task was initialized.
        INIT,

        /// @brief A task is waiting in a queue.
        WAIT,

        /// @brief A task is being run.
        RUN
    };

    /// @brief Describes states of a task according to TBB library.
    enum TbbState {
        TBB_INIT,
        TBB_ENQUEUED,
        TBB_EXEC,
        TBB_DONE
    };

    /// @brief Specifies value for wildcard (any or *) task data ID.
    const static int kTaskInstanceAny = -1;

    /// @brief Creates a new task with the given values of
    /// task code ID and task data ID.
    Task(int task_id, int task_data_id);


    /// @brief Creates a new task with the given value of
    /// task code ID and wildcard for task data ID.
    Task(int task_id);

    /// @brief Destroys a task
    virtual ~Task() { };

    /// @brief Code to execute in a task.
    /// Returns true if task is completed. Return false to reschedule the task.
    virtual bool Run() = 0;

    /// @brief Called on task exit, if it is marked for cancellation.
    /// If the user wants to do any cleanup on task cancellation,
    /// then he/she can overload this function.
    virtual void OnTaskCancel() { };

    // Accessor methods

    /// @brief Returns a state value of a task.
    State state() const { return state_; };

    /// @brief Returns the code ID of this task.
    int task_code_id() const { return task_code_id_; };

    /// @brief Returns the data ID of this task.
    int task_data_id() const { return task_data_id_; };

    /// @brief Returns the sequence number of this task.
    uint64_t seqno() const { return seqno_; };

    /// @brief Provides access to private members of a task for the
    /// output stream redirection operator.
    friend std::ostream& operator<<(std::ostream& out, const Task &task);

    /// @brief Returns a pointer to the current task the code is executing
    /// under.
    static Task *Running();

    /// @brief Returns true if the task has been canceled.
    bool task_cancelled() const { return task_cancel_; };

    /// @brief Gives a description of the task.
    virtual std::string Description() const  = 0;

    /// @brief Returns the time when the task was enqueued for execution.
    uint64_t enqueue_time() const { return enqueue_time_; }
    
    /// @brief Returns the time when the task execution was started.
    uint64_t schedule_time() const { return schedule_time_; }

    /// @brief Returns the threshold for the task execution duration.
    uint32_t execute_delay() const { return execute_delay_; }

    /// @brief Returns the time threshold for time difference between
    /// moments when the task was started and when it was enqueue.
    uint32_t schedule_delay() const { return schedule_delay_; }

private:

    /// @brief Gives access to private members for TaskEntry class.
    friend class TaskEntry;

    /// @brief Gives access to private members for TaskScheduler class.
    friend class TaskScheduler;

    /// @brief Gives access to private members for TaskImpl class.
    friend class TaskImpl;

    /// @brief Sets sequence number of the task
    void seqno(uint64_t seqno) {seqno_ = seqno;};

    /// @brief Sets a TBB state for the task
    void tbb_state(TbbState s) { tbb_state_ = s; };

    /// @brief Sets a state for this task
    void state(State s) { state_ = s; };

    /// @brief Marks this task for recycle
    void set_task_recycle() { task_recycle_ = true; };

    /// @brief Marks this task as completed (forbids recycling)
    void set_task_complete() { task_recycle_ = false; };

    /// @brief Starts execution of a task.
    void StartTask(TaskScheduler *scheduler);

    /// @brief The code path executed by the task.
    int                 task_code_id_;

    /// @brief The dataset id within a code path.
    int                 task_data_id_;

    /// @brief A pointer to an Intel TBB object storing
    /// low-level information to manage the task.
    tbb::task           *task_impl_;

    /// @brief Stores a state of the task.
    State               state_;

    /// @brief Stores a state of the TBB object.
    TbbState            tbb_state_;

    /// @brief Stores the sequence number.
    uint64_t            seqno_;

    /// @brief Determines if the task must be rescheduled (reused)
    /// after its completion.
    bool                task_recycle_;

    /// @brief Determines if the task's execution was canceled.
    bool                task_cancel_;

    /// @brief Contains the time when the task was enqueued
    /// for execution
    uint64_t            enqueue_time_;

    /// @brief Contains the time when the task was started.
    uint64_t            schedule_time_;

    /// @brief Sets threshold for the task's execution time.
    /// If the threshold is exceeded, the event is logged.
    uint32_t            execute_delay_;

    /// @brief Sets threshold for delay between enqueueing and execution.
    /// If the threshold is exceeded, the event is logged.
    uint32_t            schedule_delay_;

    // Hook in intrusive list for TaskEntry::waitq_
    boost::intrusive::list_member_hook<> waitq_hook_;

    DISALLOW_COPY_AND_ASSIGN(Task);
};

/// @brief The class is used to specify a Task label for formulating
/// a task exclusion list (an execution policy).
struct TaskExclusion {

    /// @brief Creates a new task exclusion from the given task code ID value
    /// and wildcard for task data ID.
    TaskExclusion(int task_code_id)
        : match_code_id(task_code_id), match_data_id(-1) {}

    /// @brief Creates a new task exclusion from the given task code ID and
    /// task data ID values.
    TaskExclusion(int task_code_id, int task_data_id)
        : match_code_id(task_code_id), match_data_id(task_data_id) {
    }

    /// @brief Specifies task code ID (must be a valid id >= 0)
    /// for a task execution policy.
    int match_code_id;

    /// @brief  Specifies task data ID for a task execution policy.
    /// The value of -1 corresponds to wildcard (any).
    int match_data_id;
};

/// @brief Defines a type to store an execution policy (a list of
/// task exclusions).
typedef std::vector<TaskExclusion> TaskPolicy;

/// The class is used to store various statistics associated with a
/// task or group of tasks
struct TaskStats {

    /// @brief Number of entries in waitq
    int wait_count_;

    /// @brief Number of entries currently running
    int run_count_;

    /// @brief Number of entries in deferq
    int defer_count_;

    /// @brief Number of tasks enqueued
    uint64_t enqueue_count_;

    /// @brief Number of total tasks ran
    uint64_t total_tasks_completed_;

    ///@brief Number of time stamp of latest exist
    uint64_t last_exit_time_;
};

/// @brief The TaskScheduler keeps track of what tasks are currently
/// schedulable.
/// When a task is enqueued it is added to the run queue or the pending queue
/// depending as to whether there is a runable or pending task ahead of it
/// that violates the mutual exclusion policies.
/// When tasks exit the scheduler re-examines the tasks on the pending queue
/// which may now be runnable. It is important that this process is efficient
/// such that exit events do not scan tasks that are not waiting on a
/// particular task id or task instance to have a 0 count.
class TaskScheduler {
public:
    typedef boost::function<void(const char *file_name, uint32_t line_no,
                                 const Task *task, const char *description,
                                 uint64_t delay)> LogFn;

    /// @brief TaskScheduler constructor.
    /// TBB assumes it can use the "thread" invoking tbb::scheduler can be used
    /// for task scheduling. But, in our case we dont want "main" thread to be
    /// part of tbb. So, initialize TBB with one thread more than its default.
    TaskScheduler(int thread_count = 0);

    /// @brief Frees up the task_entry_db_ allocated for scheduler.
    ~TaskScheduler();

    static void Initialize(uint32_t thread_count = 0, EventManager *evm = NULL);
    static TaskScheduler *GetInstance();

    /// @brief Enqueues a task for running. Starts task if all policy rules
    /// are met else puts task in waitq. Enqueueing may may result in the 
    /// task being immedietly
    /// added to the run queue or to a pending queue. Tasks may not be added
    /// to the run queue in violation of their exclusion policy.
    void Enqueue(Task *task);

    void EnqueueUnLocked(Task *task);

    enum CancelReturnCode {
        CANCELLED,
        FAILED,
        QUEUED,
    };

    /// @brief Cancels a Task that can be in RUN/WAIT state.
    /// The caller needs to ensure that the task exists when Cancel()
    /// is invoked.
    CancelReturnCode Cancel(Task *task);

    /// @brief Sets the task exclusion policy.
    /// Adds policy entries for the task
    /// Examples:
    /// - Policy <tid0> => <tid1, -1> <tid2, inst2> will result in following:
    ///    - task_db_[tid0] : Rule <tid1, -1> is added to policyq
    ///    - task_group_db_[tid0, inst2] : Rule <tid2, inst2> is added to policyq
    /// - The symmetry of policy will result in following additional rules,
    ///    - task_db_[tid1] : Rule <tid0, -1> is added to policyq
    ///    - task_group_db_[tid2, inst2] : Rule <tid0, inst2> is added to policyq
    void SetPolicy(int task_id, TaskPolicy &policy);

    bool GetRunStatus() { return running_; };
    int GetTaskId(const std::string &name);
    std::string GetTaskName(int task_id) const;

    TaskStats *GetTaskGroupStats(int task_id);
    TaskStats *GetTaskStats(int task_id);
    TaskStats *GetTaskStats(int task_id, int instance_id);
    void ClearTaskGroupStats(int task_id);
    void ClearTaskStats(int task_id);
    void ClearTaskStats(int task_id, int instance_id);

    /// @brief Get TaskGroup for a task_id. Grows task_entry_db_ if necessary
    TaskGroup *GetTaskGroup(int task_id);

    /// @brief Query TaskGroup for a task_id.Assumes valid entry is present for
    /// task_id
    TaskGroup *QueryTaskGroup(int task_id);

    /// @brief Check if there are any Tasks in the given TaskGroup.
    /// Assumes that all task ids are mutually exclusive with bgp::Config.
    bool IsTaskGroupEmpty(int task_id) const;

    /// @brief Get TaskGroup for a task_id. Grows task_entry_db_ if necessary
    TaskEntry *GetTaskEntry(int task_id, int instance_id);

    /// @brief Query TaskEntry for a task-id and task-instance
    TaskEntry *QueryTaskEntry(int task_id, int instance_id);

    /// @brief Method invoked on exit of a Task.
    /// Exit of a task can potentially start tasks in pendingq.
    void OnTaskExit(Task *task);

    /// @brief Stops scheduling of all tasks
    void Stop();

    /// @brief Starts scheduling of all tasks
    void Start();

    /// @brief Debug print routine
    void Print();

    /// @brief Returns true if there are no tasks running and/or enqueued
    /// If running_only is true, enqueued tasks are ignored i.e. return true if
    /// there are no running tasks. Ignore TaskGroup or TaskEntry if it is
    /// disabled.
    bool IsEmpty(bool running_only = false);

    void Terminate();

    int HardwareThreadCount() { return hw_thread_count_; }

    /// @brief Get number of tbb worker threads.
    /// For testing purposes only. Limit the number of tbb worker threads.
    static int GetThreadCount(int thread_count = 0);
    static bool ShouldUseSpawn();

    static int GetDefaultThreadCount();

    uint64_t enqueue_count() const { return enqueue_count_; }
    uint64_t done_count() const { return done_count_; }
    uint64_t cancel_count() const { return cancel_count_; }
    
    /// @brief Force number of threads
    void SetMaxThreadCount(int n);
    void GetSandeshData(SandeshTaskScheduler *resp, bool summary);
    void Log(const char *file_name, uint32_t line_no, const Task *task,
             const char *description, uint64_t delay);
    void RegisterLog(LogFn fn);

    void SetTrackRunTime(bool value) { track_run_time_ = value; }
    bool track_run_time() const { return track_run_time_; }

    /// @brief Enable logging of tasks exceeding configured latency
    void EnableLatencyThresholds(uint32_t execute, uint32_t schedule);
    uint32_t schedule_delay() const { return schedule_delay_; }
    uint32_t execute_delay() const { return execute_delay_; }

    bool measure_delay() const { return measure_delay_; }
    void SetLatencyThreshold(const std::string &name, uint32_t execute,
                             uint32_t schedule);
    uint32_t schedule_delay(Task *task) const;
    uint32_t execute_delay(Task *task) const;
    void set_event_manager(EventManager *evm);

    void DisableTaskGroup(int task_id);
    void EnableTaskGroup(int task_id);
    void DisableTaskEntry(int task_id, int instance_id);
    void EnableTaskEntry(int task_id, int instance_id);

    void ModifyTbbKeepAwakeTimeout(uint32_t timeout);

    /// @brief Enable Task monitoring
    void EnableMonitor(EventManager *evm, uint64_t tbb_keepawake_time_msec,
                       uint64_t inactivity_time_msec,
                       uint64_t poll_interval_msec);
    const TaskMonitor *task_monitor() const { return task_monitor_; }
    const TaskTbbKeepAwake *tbb_awake_task() const { return tbb_awake_task_; }
    bool use_spawn() const { return use_spawn_; }

    /// @brief following function allows one to increase max num of threads used by
    /// TBB
    static void SetThreadAmpFactor(int n);

private:
    friend class ConcurrencyScope;
    typedef std::vector<TaskGroup *> TaskGroupDb;
    typedef std::map<std::string, int> TaskIdMap;

    static const int        kVectorGrowSize = 16;
    static boost::scoped_ptr<TaskScheduler> singleton_;

    // XXX
    // Following two methods are only for Unit Testing to control
    // current running task. Usage of this method would result in
    // unexpected behavior.

    /// @brief This function should not be called in production code.
    /// It is only for unit testing to control current running task
    /// This function modifies the running task as specified by the input
    void SetRunningTask(Task *);
    void ClearRunningTask();
    void WaitForTerminateCompletion();

    /// @brief Platfrom-dependent subroutine in Linux and FreeBSD
    /// implementations, used only in 
    /// TaskScheduler::WaitForTerminateCompletion().
    /// In Linux, make sure that all the [tbb] threads launched have completely
    /// exited. We do so by looking for the Threads count of this process in
    /// /proc/<pid>/status.
    ///
    /// In FreeBSD use libprocstat to check how many threads is running
    /// in specific process.
    int CountThreadsPerPid(pid_t pid);

    /// @brief Use spawn() to run a tbb::task instead of enqueue()
    bool                    use_spawn_;
    TaskEntry               *stop_entry_;

    tbb::task_scheduler_init task_scheduler_;
    mutable std::mutex      mutex_;
    bool                    running_;
    uint64_t                seqno_;
    TaskGroupDb             task_group_db_;

    std::shared_mutex       id_map_mutex_;
    TaskIdMap               id_map_;
    int                     id_max_;

    LogFn                   log_fn_;
    int                     hw_thread_count_;

    bool                    track_run_time_;
    bool                    measure_delay_;

    /// @brief Log if time between enqueue and task-execute exceeds the delay
    uint32_t                schedule_delay_;

    /// @brief Log if time taken to execute exceeds the delay
    uint32_t                execute_delay_;

    uint64_t                enqueue_count_;
    uint64_t                done_count_;
    uint64_t                cancel_count_;
    EventManager            *evm_;

    /// @brief following variable allows one to increase max num of threads used by
    /// TBB
    static int ThreadAmpFactor_;

    TaskTbbKeepAwake *tbb_awake_task_;
    TaskMonitor      *task_monitor_;
    DISALLOW_COPY_AND_ASSIGN(TaskScheduler);
};

#endif
