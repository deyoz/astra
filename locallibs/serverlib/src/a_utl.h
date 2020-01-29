#ifndef __SERVERLIB_A_UTL_H_
#define __SERVERLIB_A_UTL_H_

#include <sstream>

namespace ServerFramework {

namespace utl {


template <class Endpoint> std::string stringize(const Endpoint& endpoint)
{
    std::ostringstream s;
    s<<endpoint.address().to_string()<<':'<<endpoint.port();
    return s.str();
    //return endpoint.address().to_string() + ':' + std::to_string(endpoint.port());
}

} // namespace utl

} // namespace ServerFramework

#endif // __SERVERLIB_A_UTL_H_
