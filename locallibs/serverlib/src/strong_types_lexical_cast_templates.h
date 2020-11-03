#ifndef __STRONG_TYPES_LEXICAL_CAST_TEMPLATES_H_
#define __STRONG_TYPES_LEXICAL_CAST_TEMPLATES_H_

#include <boost/lexical_cast.hpp>

namespace ServerFramework {
namespace BoostLexicalCast {

template <class TypeName> TypeName strong_type_fromStr(const std::string& s) try
{
    return TypeName(boost::lexical_cast<typename TypeName::UnderlyingType>(s));
}
catch (const boost::bad_lexical_cast&)
{
    throw typename TypeName::ValidatorFailure(s);
}


template <class TypeName> bool strong_type_validateStr(const std::string& s) try
{
    return validate(boost::lexical_cast<typename TypeName::UnderlyingType>(s));
}
catch (const boost::bad_lexical_cast&)
{
    return false;
}

} // namespace BoostLexicalCast
} // namespace ServerFramework

#define MAKE_FROM_STR(T) T T::fromStr(const std::string& s) {  return ServerFramework::BoostLexicalCast::strong_type_fromStr<T>(s);  }

#endif // __STRONG_TYPES_LEXICAL_CAST_TEMPLATES_H_
