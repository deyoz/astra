#pragma once

#include <boost/shared_array.hpp>
#include <oci.h>

namespace OciCpp {

namespace External
{
    enum type
    {
        pod,
        string,
        wrapper
    };
}

enum iValues
{
    iok,
    inull = -1,
    itruncate = 1
};

using indicator = sb2;

using buf_ptr_t = boost::shared_array<char>;
using ind_ptr_t = boost::shared_array<indicator>;

class default_ptr_t;

template <typename T, typename Dummy = void> struct OciSelector
{
   static_assert(Dummy::not_found_so_autogenerated, "no proper OciSelector<...> specialisation found");
};

} // namespace OciCpp
