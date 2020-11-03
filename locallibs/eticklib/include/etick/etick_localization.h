/*	2006 by Roman Kovalev 	*/
/*	roman@pike.dev.sirena2000.ru		*/
#ifndef _ETICK_LOCALIZATION_H_
#define _ETICK_LOCALIZATION_H_

#include <locale>
#include <boost/shared_ptr.hpp>
#include <string>
#include <boost/format.hpp>
#include "etick/lang.h"

namespace Ticketing
{

template<class A> inline void passArg( const std::string &errtext,
                                        boost::shared_ptr<boost::format> &pf,
                                        const A &a)
{
    if(!pf)
    {
        pf.reset( new boost::format(errtext) );
    }

    (*pf) % a;
}

class EtickLocalization
{
    std::string ErrCode;
    std::string ErrText;
    mutable boost::shared_ptr<boost::format> pFrmt;
    Language Lng;

protected:
    virtual const std::string Localize(std::string ErrCode, Language l);
public:
    EtickLocalization(const std::string &ErrC, Language l)
    :ErrCode(ErrC), ErrText(Localize(ErrC,l)), Lng(l)
    {
    }

    /**
     * Передать параметр в локализированный текст
     * @param p параметр
     * @return *this
     */
    template<class A> EtickLocalization & operator << (const A &p)
    {
        passArg(ErrText, pFrmt, p);
        return *this;
    }

    virtual operator std::string & ();
    virtual std::string str();

    virtual ~EtickLocalization() throw () {}
};

inline EtickLocalization EtTr(const std::string &ErrC, Language l)
{
    return EtickLocalization(ErrC, l);
}
}

#endif /*_ETICK_LOCALIZATION_H_*/
