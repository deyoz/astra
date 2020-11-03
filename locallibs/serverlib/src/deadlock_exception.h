#ifndef SERVERLIB_DEADLOCK_EXCEPTION_H
#define SERVERLIB_DEADLOCK_EXCEPTION_H

#include <string>
#include "exception.h"

// Deadlock is special case
// it must interrupt running process immediately
// rollbacking all changes
class DeadlockException : public comtech::Exception
{
public:
    DeadlockException(const std::string &msg)
        : comtech::Exception(msg)
    {}
    virtual ~DeadlockException() throw() {}
};

#endif /* SERVERLIB_DEADLOCK_EXCEPTION_H */

