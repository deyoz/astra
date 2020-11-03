//
// C++ Interface: edi_except
//
// Description:
//
//
// Author:  <rom@sirena2000.ru>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef _EDILIB_EDI_EXCEPT_H_
#define _EDILIB_EDI_EXCEPT_H_
#include <string>
#include <serverlib/exception.h>

namespace edilib{

class EdiExcept : public comtech::Exception
{
    std::string Message;
    public:
        EdiExcept(const std::string &msg):
            comtech::Exception(msg), Message(msg)
        {
        }

        virtual void setMessage(const std::string &msg)
        {
            Message = msg;
        }

        virtual const char * what() const throw ()
        {
            return Message.c_str();
        }

        virtual ~EdiExcept() throw ()
        {
        }
};

class ParseExcept : public EdiExcept
{
public:
    ParseExcept(const std::string &msg)
    :
        EdiExcept(msg)
    {
    }
    virtual ~ParseExcept() throw () {}
};

} // namespace edilib
#endif /*_EDILIB_EDI_EXCEPT_H_*/
