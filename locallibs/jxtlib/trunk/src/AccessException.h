#ifndef ACCESSEXCEPTION_H
#define ACCESSEXCEPTION_H

#include <serverlib/exception.h>
class AccessException
    : public comtech::Exception
{
public:
    AccessException(const char *nick, const char *file, int line, const std::string& msg);
    virtual ~AccessException() throw();
};



#endif /* ACCESSEXCEPTION_H */

