#ifndef __SIMPLE_TASK_H
#define __SIMPLE_TASK_H

#include "daemon_task.h"
#include "strong_types.h"

namespace ServerFramework {

    DEFINE_STRING_WITH_LENGTH_VALIDATION(TaskId, 1, 32, "TASK ID", "TASK ID");
    DEFINE_STRING_WITH_LENGTH_VALIDATION(TaskPrefix, 1, 30, "TASK PREFIX", "TASK PREFIX");
    DEFINE_NUMBER_WITH_RANGE_VALIDATION(TaskAttempt, long, 0, 99, "TASK ATTEMPT", "TASK ATTEMPT");

    void fallback_failed(const TaskId& id);

    struct Task {
        TaskId id;
        TaskPrefix prefix;
        TaskAttempt attempt;
        TaskAttempt max_attempt;
        boost::posix_time::ptime init_time;
    };

    Task read_task(const TaskId& id,
                   const TaskPrefix& prefix,
                   const TaskAttempt& max_attempt);
    void remove_task(const TaskId& id,
                     const TaskPrefix& prefix);
    void increase_attempt(const TaskId& id, const TaskPrefix& prefix);

    template<typename DataType>
    class SimpleTask : public CyclicDaemonTask<DataType> {
    public:
        SimpleTask(const DaemonTaskTraits& t) :
            CyclicDaemonTask<DataType>(t) {}

        virtual ~SimpleTask() {}

        virtual void callback(const DataType& data) const = 0;
        virtual void fallback(const DataType& data) const = 0;
        virtual TaskId id(const DataType& data) const = 0;
        virtual TaskPrefix prefix() const = 0;
        virtual TaskAttempt max_attempt() const { return TaskAttempt(3); }

    protected:
        virtual int run(const boost::posix_time::ptime& time,
                        const DataType& data,
                        bool);
    };

    template<typename DataType>
    int SimpleTask<DataType>::run(const boost::posix_time::ptime& /*time*/,
                                  const DataType& data,
                                  bool) {
        const Task task = read_task(id(data), prefix(), max_attempt());

        if (task.attempt.get() > task.max_attempt.get()) {
            fallback_failed(task.id);
            remove_task(task.id, prefix());
            return -1;
        }

        increase_attempt(task.id, task.prefix);

        if (task.attempt.get() == task.max_attempt.get()) {
            fallback(data);
            remove_task(task.id, prefix());
            return -1;
        }

        callback(data);
        remove_task(task.id, prefix());

        return 0;
    }

} /* ServerFramework */

#endif /* __SIMPLE_TASK_H */
