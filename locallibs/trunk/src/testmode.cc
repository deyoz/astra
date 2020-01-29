#if HAVE_CONFIG_H
#endif

#include "testmode.h"

static int test_mode_ = 0;

void initTestMode()
{
    test_mode_ = 1;
}

int inTestMode()
{
    return (test_mode_ != 0);
}

