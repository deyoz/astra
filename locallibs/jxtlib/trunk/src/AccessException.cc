
#include "AccessException.h"

AccessException::AccessException(const char *nick, const char *file, int line, const std::string& msg)
    : comtech::Exception(nick, file, line, "", msg)
{}

AccessException::~AccessException() throw()
{}


