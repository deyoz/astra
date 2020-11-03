#ifndef SERVERLIB_ENUM_H
#define SERVERLIB_ENUM_H

#include <set>
#include <map>
#include <vector>
#include <string>
#include <cassert>

#include "makename_defs.h"

template<typename EnumType>
class EnumNamesHelper
{
    typedef std::map<EnumType, const char*> map_t;
public:
    EnumNamesHelper() {}

    EnumNamesHelper& operator()(EnumType v, const char* name) {
        // do not use HEADER_ASSERT here to avoid cyclic header dependencies
        assert(names_.emplace(v, name).second == true);
        return *this;
    }

    // returns string representation of enum valid v
    // returns NULL when nothing found
    const char* enumToStr(EnumType v) const {
        const typename map_t::const_iterator it(names_.find(v));
        return it == names_.end() ? NULL : it->second;
    }

    // returns true and fills v if str is valid enum string representation
    // returns false otherwise
    bool enumFromStr(EnumType& v, const std::string& str) const {
        for (typename map_t::const_iterator it(names_.begin()); it != names_.end(); ++it) {
            if (str.compare(it->second) == 0) {
                v = it->first;
                return true;
            }
        }
        return false;
    }

    // returns valid enum value if str is valid enum string representation
    // returns defVal otherwise
    EnumType enumFromStr2(const std::string& str, EnumType defVal) const {
        EnumType v(defVal);
        enumFromStr(v, str);
        return v;
    }

    // returns sorted enum values
    std::vector<EnumType> sorted() const {
        std::set<EnumType> tmp;
        for (typename map_t::const_iterator it(names_.begin()); it != names_.end(); ++it) {
            tmp.insert(it->first);
        }
        std::vector<EnumType> res(tmp.size());
        std::copy(tmp.begin(), tmp.end(), res.begin());
        return res;
    }
private:
    map_t names_;
};

#define ENUM_NAMES_DECL__HELPER(Enum) \
const char* enumToStr(Enum v);\
const char* enumToStr2(Enum v);\
bool enumFromStr(Enum& v, const std::string&);\
Enum enumFromStr2(const std::string&, Enum def);\
template<typename T> std::vector<T> enumSorted();\
template<> std::vector<Enum> enumSorted<Enum>();

#define ENUM_NAMES_BEGIN__HELPER(Enum, VarName) \
static const EnumNamesHelper<Enum> VarName = EnumNamesHelper<Enum>()

#define ENUM_NAMES_END__HELPER(Enum, VarName) ;\
const char* enumToStr(Enum v) { const char *ret = VarName.enumToStr(v);\
    if (!ret) { \
        std::string errText("Invalid value for enum " #Enum " v="); \
        errText += std::to_string(static_cast<int>(v)); \
        throw comtech::AssertionFailed(NICKNAME, __FILE__, __LINE__, errText); \
    } \
    return ret; } \
bool enumFromStr(Enum& v, const std::string& str) { return VarName.enumFromStr(v, str); }

#define ENUM_NAMES_END__HELPER_AUX(Enum, VarName) ;\
const char* enumToStr2(Enum v) { return VarName.enumToStr(v); } \
Enum enumFromStr2(const std::string& str, Enum defVal) { return VarName.enumFromStr2(str, defVal); } \
template<> std::vector<Enum> enumSorted<Enum>() { return VarName.sorted(); }

#define ENUM_NAMES_DECL(Enum) \
    ENUM_NAMES_DECL__HELPER(Enum)

#define ENUM_NAMES_BEGIN(Enum) \
    ENUM_NAMES_BEGIN__HELPER(Enum, MakeName2(names, Enum))

#define ENUM_NAMES_END(Enum) \
    ENUM_NAMES_END__HELPER(Enum, MakeName2(names, Enum))

#define ENUM_NAMES_END_AUX(Enum)  ENUM_NAMES_END__HELPER_AUX(Enum, MakeName2(names, Enum))

#define ENUM_NAMES_DECL2(Class, Enum) \
    ENUM_NAMES_DECL__HELPER(Class::Enum)

#define ENUM_NAMES_BEGIN2(Class, Enum) \
    ENUM_NAMES_BEGIN__HELPER(Class::Enum, MakeName3(names, Class, Enum))

#define ENUM_NAMES_END2(Class, Enum) \
    ENUM_NAMES_END__HELPER(Class::Enum, MakeName3(names, Class, Enum)) \
    ENUM_NAMES_END__HELPER_AUX(Class::Enum, MakeName3(names, Class, Enum))

template<typename T>
T enumFromStrThrow(const std::string & str)
{
    T result;
    if(!enumFromStr(result, str))
        throw std::runtime_error("unknown enum value " + str);
    return result;
}

#endif /* SERVERLIB_ENUM_H */

