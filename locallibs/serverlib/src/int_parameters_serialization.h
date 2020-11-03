#ifndef INT_PARAMETERS_SERIALIZATION_H
#define INT_PARAMETERS_SERIALIZATION_H

#include <boost/serialization/split_free.hpp>
#include <boost/serialization/nvp.hpp>

namespace ParInt { template<typename traits_t, typename base_t> class BaseIntParam; }

namespace boost {
namespace serialization {

template<class Archive, typename traits_t, typename base_t>
inline void save(Archive & ar, const ParInt::BaseIntParam<traits_t, base_t> &t, const unsigned int /* version */)
{
    auto val = *t.never_use_except_in_OciCpp();
    ar << BOOST_SERIALIZATION_NVP(val);
}

template<class Archive, typename traits_t, typename base_t>
inline void load(Archive & ar, ParInt::BaseIntParam<traits_t, base_t> &t, const unsigned int /* version */)
{
    base_t val = 0;
    ar >> BOOST_SERIALIZATION_NVP(val);
    t = ParInt::BaseIntParam<traits_t, base_t>(val);
    return;
}

template<class Archive, typename traits_t, typename base_t>
void serialize(Archive & ar, ParInt::BaseIntParam<traits_t, base_t> & par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}

} // namespace serialization
} // namespace boost



#endif /* INT_PARAMETERS_SERIALIZATION_H */

