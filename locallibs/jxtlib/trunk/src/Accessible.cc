#define NICKNAME "NONSTOP"
#define NICKTRACE NONSTOP_TRACE
#include <serverlib/slogger.h>


#include "Accessible.h"

Accessible::~Accessible()
{
}

IAccessible::IAccessible(Accessible* acc)
    : acc_(acc)
{
}

IAccessible::~IAccessible()
{
    delete acc_;
}

bool IAccessible::isAccessible() const
{
    if (acc_)
        return acc_->state();
    return true;
}

