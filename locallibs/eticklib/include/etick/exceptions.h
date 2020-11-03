/*	2006 by Roman Kovalev 	*/
/*	roman@pike.dev.sirena2000.ru		*/
#ifndef _ETICK_EXCEPTIONS_H_
#define _ETICK_EXCEPTIONS_H_

#include <string>
#include <list>
#include <serverlib/exception.h>
#include "etick/lang.h"
#include "etick/etick_msg_types.h"
namespace Ticketing{
inline Language toLang(int l)
{
    return static_cast<Language>(l);
}
namespace TickExceptions{

    class Exception : public comtech::Exception
    {
        std::string What;
    public:
        Exception(const std::string &wht)
            :comtech::Exception(wht), What(wht)
        {
        }

        virtual const char *what() const throw()
        {
            return What.c_str();
        }

        virtual ~Exception() throw() {}
    };

    class ClassIntegralityError : public Exception
    {
        //const char *Func;
    public:
        ClassIntegralityError(const char *func, const char *wht)
            :Exception(std::string("Class integrality: ") + func + ": " + wht)//,Func(func)
        {
        }
        virtual ~ClassIntegralityError() throw() {}
    };

#define ClassIntegralityErr(x) \
            Ticketing::TickExceptions::ClassIntegralityError(__FUNCTION__, (x));

    /// @class tick_exception базовый класс ticket exception"ов
    class tick_exception : public comtech::Exception
    {
    protected:
        std::string What;
        void setMessage(const std::string &m)
        {
            What = m;
        }
    private:
        ErrMsg_t ErrCode;
        std::list<std::string> ErrTextParams;


    public:
            tick_exception(const char *nickname, const char *file, int line,
                           const ErrMsg_t & err, short fatal);

            tick_exception(const char *nickname, const char *file, int line,
                           const ErrMsg_t & err, short fatal, const char *str);

            tick_exception(const char *nickname, const char *file, int line,
                           const ErrMsg_t & err, short fatal, const std::string &str);

            virtual const ErrMsg_t &errCode(void) const throw() {
                return ErrCode;
            }

            virtual const char *what() const throw()
            {
                return What.c_str();
            }

            /**
             * Передать параметр в локализируемый текст
             * @param p параметр
             * @return *this
             */
            virtual tick_exception & operator << (const std::string &p);

            virtual std::string errText(Language l = ENGLISH) const;

            virtual std::list<std::string> errTextParams() const { return ErrTextParams; }

            virtual ~tick_exception() throw() {}
    };

    class tick_soft_except : public tick_exception
    {
        tick_soft_except ();
        public:
            tick_soft_except(const char *nickname, const char *file,
                             int line, const ErrMsg_t & err)  throw();

            tick_soft_except(const char *nickname, const char *file,
                             int line, const ErrMsg_t & err, const char *format, ...) throw();

            virtual tick_soft_except & operator << (const std::string &p);
            virtual ~tick_soft_except()  throw() {}
    };

    class tick_fatal_except : public tick_exception
    {
        tick_fatal_except();
        public:
            tick_fatal_except(const char *nickname, const char *file,
                              int line, const ErrMsg_t & err) throw()
            :tick_exception(nickname,file,line,err,1){};

            tick_fatal_except(const char *nickname, const char *file,
                              int line, const ErrMsg_t & err, const char *format, ...) throw();

            virtual tick_fatal_except & operator << (const std::string &p);
            virtual ~tick_fatal_except()  throw() {}
    };
}// tick_exceptions
}//Ticketing

#endif /*_ETICK_EXCEPTIONS_H_*/

