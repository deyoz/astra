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
