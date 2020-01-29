#pragma once

#include <iosfwd>

#include "either.h"
#include "message.h"

template<typename T1, typename T2 = Message>
class Expected
{
public:
    Expected( T1 const &v ) : val_( v ) {}
    Expected( T2 const &v ) : val_( v ) {}

    bool valid() const noexcept
    {
        return val_.isLeft();
    }

    explicit operator bool() const noexcept
    {
        return valid();
    }

    T1* operator->() { return &val_.left(); }
    T1 const *operator->() const { return &val_.left(); }
    T1 &operator*() { return val_.left(); }
    T1 const &operator*() const { return val_.left(); }

    T2 const &err() const { return val_.right(); }

    bool operator==( Expected const &rhs ) const { return val_ == rhs.val_; }
    bool operator!=( Expected const &rhs ) const { return val_ != rhs.val_; }

    friend std::ostream& operator<<(std::ostream& os, Expected const & e) {
        if (e.valid())
            return os << *e;
        return os << e.err();
    }
private:
    Either< T1, T2 > val_;
};
/*
template<typename T1, typename T2>
std::ostream& operator<<(std::ostream& os, const Expected<T1, T2>& e)
{
    if (e.valid()) {
        os << *e;
    } else {
        os << e.err();
    }
    return os;
}*/

#define CALL_EXP_RET( var, expr ) \
    auto const var(expr); \
    if (!var) { return var.err(); }
