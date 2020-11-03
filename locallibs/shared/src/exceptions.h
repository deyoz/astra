#ifndef _EXCEPTIONS_H_
#define _EXCEPTIONS_H_
#include <string>
#include <iostream>
#include <sstream>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "setup.h"
#include "msg.h"
#ifdef ASTRA2007
#include "serverlib/exception.h"
#endif

/* for ASTRA2007 declared classes:
   Exception
   UserException */

namespace EXCEPTIONS
{
#ifdef ASTRA2007
class Exception:public ServerFramework::Exception
{
  protected:
    char Message[500];
  public:
    Exception(const std::string &msg): ServerFramework::Exception(msg, true) {
        *Message=0;
    };
    Exception(const char *format, ...): ServerFramework::Exception("", true) {
    	va_list ap;
    	va_start(ap, format);
    	vsnprintf(Message, sizeof(Message), format, ap);
        Message[sizeof(Message)-1]=0;
    	va_end(ap);
    };
    virtual ~Exception() throw(){};
    virtual const char* what() const throw() {
    	if (*Message==0)
    	  return ServerFramework::Exception::what();
    	else
    	  return Message;
    };
    void setMessage(const std::string &msg)
    {
        snprintf(Message,500,"%s",msg.c_str());
    }
};
#else
class Exception
{
  public:
    char Message[500];

    Exception(const char *format, ...) {
    	va_list ap;
    	va_start(ap, format);
    	vsnprintf(Message, sizeof(Message), format, ap);
        Message[sizeof(Message)-1]=0;
    	va_end(ap);
    }

    Exception(const std::string msg) {
      snprintf(Message, sizeof(Message), "%s", msg.c_str());
      Message[sizeof(Message)-1]=0;
    }
    const char *what() { return Message; }
};
#endif

class EMemoryError:public Exception
{
  public:
    EMemoryError(const char* msg):Exception(msg) {};
};

class EConvertError:public Exception
{
  public:
    EConvertError(const char *format, ...):Exception("")
    {
      va_list ap;
      va_start(ap, format);
      vsnprintf(Message, sizeof(Message), format, ap);
      Message[sizeof(Message)-1]=0;
      va_end(ap);
    };

    EConvertError(std::string msg):Exception(msg) {};
};

class EParserError:public Exception
{
  public:
    EParserError(const char* msg):Exception(msg) {};
};



#ifndef ASTRA2007
class UserException:public Exception
{
  public:
    int X, Y;
    UserException(const std::string msg, int x=0, int y=0): Exception(msg), X(x), Y(y) { };
};

class UserExceptionFmt: public UserException
{
  public:
	UserExceptionFmt(const char *format, ...): UserException("") {
    	va_list ap;
    	va_start(ap, format);
    	vsnprintf(Message, sizeof(Message), format, ap);
        Message[sizeof(Message)-1]=0;
    	va_end(ap);
    };
};

class FormatException:public UserException
{
  public:
    FormatException( ):UserException( "НЕКОРРЕКТНЫЙ ФОРМАТ ЗАПРОСА" ) { };
    FormatException(const char* msg, int x=0, int y=0 ):UserException(msg,x,y) { };
};

class UserEAccessError: public UserException {
  public:
    UserEAccessError(): UserException("Недостаточно прав для выполнения операции") {};
};

#endif /* ASTRA2007 */

class CustomException:public Exception
{
  public:
    CustomException():Exception("Ошибка программы - "
                                "обратитесь к разработчикам") {};
};

class EInvalidRequest: public Exception {
  public:
    EInvalidRequest(): Exception("Неправильный запрос") {};
    EInvalidRequest(const std::string msg): Exception("Неправильный запрос: " + msg) {};
    EInvalidRequest(const std::string request, const std::string elem):
      Exception("Неправильный запрос: " + request + "\nОтсутствует элемент: " + elem) {};
    EInvalidRequest(const std::string request, const std::string elem, const std::string attrib):
      Exception("Неправильный запрос: " + request + "\nУ элемента " + elem + " отсутствует атрибут " + attrib) {};
};

class EAccessError: public Exception {
  public:
    EAccessError(): Exception("Недостаточно прав для выполнения операции") {};
};

class ELockError: public Exception {
  public:
    ELockError(): Exception("Запись заблокирована другим пользователем") {};
};

class Error: public Exception {
    ErrorCodes Code;
    public:
        Error(ErrorCodes code): Exception(err_msg[code]), Code(code) {};
        ErrorCodes getCode() { return Code; };
};

#ifndef ASTRA2007
class AccessException: public Exception {
    std::string btn;
    public:
        AccessException(std::string btn): Exception("Нет доступа!") { this->btn = btn; };
        std::string getBtn() { return btn; };
};
class UserError: public UserException {
    ErrorCodes Code;
    public:
        UserError(ErrorCodes code): UserException(err_msg[code]), Code(code) {};
        ErrorCodes getCode() { return Code; };
};
#endif /* ASTRA2007 */

class ExceptionStream : public Exception
{
    std::ostringstream *Stream;
    mutable std::string What;
public:
    template < class T >
            ExceptionStream & operator << (const T &t)
    {
        *Stream << t;
        return *this;
    }
    virtual const char * what() const throw();
    virtual ~ExceptionStream() throw();
    ExceptionStream(const ExceptionStream&);
protected:
    ExceptionStream(const char *nick, const char *file, unsigned line);
    ExceptionStream();
    friend ExceptionStream ExceptionFmt (const char *nick, const char *file, unsigned line);
    friend ExceptionStream ExceptionFmt ();
};

inline ExceptionStream ExceptionFmt (const char *nick, const char *file, unsigned line)
{
    return ExceptionStream(nick, file, line);
}

inline ExceptionStream ExceptionFmt ()
{
    return ExceptionStream();
}



} //namespace EXCEPTIONS
#endif
