#ifndef LIBNSI_EXCEPTION_H
#define LIBNSI_EXCEPTION_H

#include <serverlib/message.h>
#include <serverlib/exception.h>

namespace nsi
{

class Exception
    : public ServerFramework::Exception
{
public:
    Exception(const char* file, int line, const std::string&);
    virtual ~Exception() throw() {}
};


} // nsi

#endif /* LIBNSI_EXCEPTION_H */

