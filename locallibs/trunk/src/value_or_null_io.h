#pragma once

#include <iostream>
#include "value_or_null.h"

template<typename T>
std::ostream& operator<<(std::ostream& os, const ValOrNull<T>& vof)
{
    if (vof.val) {
        return os << *vof.val;
    } else {
        return os << "none";
    }
}

