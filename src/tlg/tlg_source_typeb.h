#pragma once

#include "tlg_source.h"


namespace TlgHandling {

class TlgSourceTypeB: public TlgSourceTypified
{
public:
    TlgSourceTypeB(const TlgSource& src);

    virtual const char *name() const { return "TPB"; }
    virtual const char *nameToDB(bool out) const { return out?"OUTB":"INB"; }
    virtual tlg_type_t type() const { return TlgType::airimp; }

    /**
     * ������ � ����
     */
    virtual void write();

    static bool isItYours(const std::string &txt);
};

std::ostream & operator << (std::ostream& os, const TlgSourceTypeB &tlg);

}//namespace TlgHandling
