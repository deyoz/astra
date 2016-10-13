#pragma once

#include "tlg_source.h"


namespace TlgHandling {

class TlgSourceTypeB: public TlgSourceTypified
{
public:
    TlgSourceTypeB(const TlgSource& src);

    virtual const char *name() const { return "TPB"; }
    virtual tlg_type_t type() const { return TlgType::airimp; }

    /**
     * Запись в базу
     */
    virtual void write();

    static bool isItYours(const std::string &txt);
};

std::ostream & operator << (std::ostream& os, const TlgSourceTypeB &tlg);

}//namespace TlgHandling
