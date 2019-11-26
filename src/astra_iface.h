#pragma once

#include <jxtlib/JxtInterface.h>
#include <jxtlib/Accessible.h>

class AstraJxtIface: public JxtInterface
{
public:
    AstraJxtIface(const std::string& iface, Accessible* acc = NULL)
        : JxtInterface("", iface, acc)
    {}

    virtual ~AstraJxtIface() {}
};



namespace func_equal {

template<typename T, typename... U>
size_t getAddress(std::function<T(U...)> f) {
    typedef T(fnType)(U...);
    fnType ** fnPointer = f.template target<fnType*>();
    return (size_t) *fnPointer;
}

}
