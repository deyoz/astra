/*	2006 by Roman Kovalev 	*/
/*	roman@pike.dev.sirena2000.ru		*/
#ifndef _ETICK_ERR_MSG_TYPES_H_
#define _ETICK_ERR_MSG_TYPES_H_

#include <string>
#include <map>
#include "etick/lang.h"

#define REGERR(x) static const ErrMsg_t x

#define DEF_ERR__(x) static const Ticketing::ErrMsg_t x

#define REG_ERR__(s, x, e,r) const Ticketing::ErrMsg_t s::x=""#s"::"#x;\
    namespace {\
    Ticketing::ErrMsgs x (s::x, e,r);\
}


namespace Ticketing
{
    typedef std::string ErrMsg_t;
    typedef std::map<ErrMsg_t, std::string> LocalizationMap;

    LocalizationMap &getLocalizMap(Language l);

    class ErrMsgs{
        public:
            ErrMsgs(const ErrMsg_t &err, const char *eng, const char *rus)
            {
                if(rus)
                    getLocalizMap(RUSSIAN)[err] = rus;

                if(eng)
                    getLocalizMap(ENGLISH)[err] = eng;
            }
            static std::string Localize(const ErrMsg_t &key, Language l);
    };
}// namespace Ticketing

//#undef REGERR
#endif /*_ETICK_ERR_MSG_TYPES_H_*/
