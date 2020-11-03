#include "exceptions.h"

using namespace EXCEPTIONS;

ExceptionStream::~ ExceptionStream() throw()
{
    delete Stream;
}

ExceptionStream::ExceptionStream(const char * nick, const char * file, unsigned line)
    :Exception("")
{
    Stream = new std::ostringstream;
    *Stream << nick << ":" << file << ":" << line << " ";
    setMessage(Stream->str());
}

ExceptionStream::ExceptionStream()
        :Exception("")
{
    Stream = new std::ostringstream;
}

ExceptionStream::ExceptionStream(const ExceptionStream &e)
    :Exception("")
{
    Stream = new std::ostringstream;
    *Stream << e.Stream->str();
}

const char * ExceptionStream::what() const throw ()
{
    if(What.empty())
    {
        What.assign(Stream->str());
    }
    return What.c_str();
}






