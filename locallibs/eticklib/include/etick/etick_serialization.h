#ifndef _ETICK_SERIALIZATION_H_
#define _ETICK_SERIALIZATION_H_

#include <boost/serialization/split_free.hpp>
#include "tick_doctype.h"
#include "tick_data.h"
#include "emd_data.h"


namespace {

template<class Archive, class EtickType>
void save_(Archive& ar, const EtickType& t, const unsigned int/* version */)
{
    std::string tstr = t ? t->code() : std::string();
    ar & tstr;
}

template<class Archive, class EtickType>
void load_(Archive& ar, EtickType& t, const unsigned int/* version */)
{
    std::string tstr;
    ar & tstr;
    t = !tstr.empty() ? EtickType(tstr) : EtickType();
}

}//namespace

/////////////////////////////////////////////////////////////////////////////////////////

#define BOOST_ETICK_SERIALIZATION(T)            \
namespace boost { namespace serialization {     \
                                                \
template<class Archive>                         \
inline void save(Archive& ar,                   \
                 const T& t,                    \
                 const unsigned int version)    \
{                                               \
    save_(ar, t, version);                      \
}                                               \
                                                \
template<class Archive>                         \
inline void load(Archive& ar,                   \
                 T& t,                          \
                 const unsigned int version)    \
{                                               \
    load_(ar, t, version);                      \
}                                               \
                                                \
}}                                              \
BOOST_SERIALIZATION_SPLIT_FREE(T)

/////////////////////////////////////////////////////////////////////////////////////////

BOOST_ETICK_SERIALIZATION(Ticketing::DocType);
BOOST_ETICK_SERIALIZATION(Ticketing::RficType);
BOOST_ETICK_SERIALIZATION(Ticketing::PassengerType);
BOOST_ETICK_SERIALIZATION(Ticketing::SubClass);
BOOST_ETICK_SERIALIZATION(Ticketing::CouponStatus);

#undef BOOST_ETICK_SERIALIZATION

#endif/*_ETICK_SERIALIZATION_H_*/
