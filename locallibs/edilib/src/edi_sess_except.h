//
// C++ Interface: edi_sess_except
//
// Description:
//
//
// Author:  <rom@sirena2000.ru>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef _EDI_SESS_EXCEPT_H_
#define _EDI_SESS_EXCEPT_H_
#include <serverlib/logger.h>
#include "edilib/edi_except.h"

namespace edilib
{

class EdiSessExcept : public edilib::EdiExcept
{
public:
    EdiSessExcept(const char *nick, const char *file, int line,
                  const char *msg)
    : EdiExcept(msg)
    {
        WriteLog(nick, file, line, "%s", msg);
    }
};

class EdiSessFatal : public EdiExcept
{
public:
    EdiSessFatal(const char *nick, const char *file, int line,
                  const char *msg)
    : EdiExcept(msg)
    {
        ProgError(nick, file, line, "%s", msg);
    }
};

class EdiSessDup : public EdiSessExcept
{
public:
    EdiSessDup(const char *n, const char *f, int l, const char *m)
     :EdiSessExcept(n, f, l, m)
    {
    }
};

class EdiSessNotFound : public EdiSessExcept
{
public:
    EdiSessNotFound(const char *n, const char *f, int l, const char *m)
    :EdiSessExcept(n, f, l, m)
    {
    }
};

class EdiSessLocked : public EdiSessExcept
{
public:
    EdiSessLocked(const char *n, const char *f, int l, const char *m)
    :EdiSessExcept(n, f, l, m)
    {
    }
};

} // namespace edilib

#endif /*_EDI_SESS_EXCEPT_H_*/
