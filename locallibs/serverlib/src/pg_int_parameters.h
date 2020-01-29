#pragma once
#ifdef ENABLE_PG
#include "int_parameters.h"
#include "pg_cursctl.h"

namespace PgCpp
{
namespace details
{

template<typename traits_t, int N>
class DefaultValueHolder<ParInt::BaseIntParam<traits_t, std::string>, char[N]> : public IDefaultValueHolder
{
public:
    static constexpr bool isConstructibe = true;
    DefaultValueHolder(const char* d)
        : defaultValue_(d)
    {}
    void setValue(void* v) override {
        auto* p = reinterpret_cast<ParInt::BaseIntParam<traits_t, std::string>*>(v);
        *p = defaultValue_;
    }
private:
    ParInt::BaseIntParam<traits_t, std::string> defaultValue_;
};

template<typename traits_t>
struct PgTraits<ParInt::BaseIntParam<traits_t, std::string>>
{
    typedef ParInt::BaseIntParam<traits_t, std::string> this_type;
    static const PgOid oid = PgTraits<std::string>::oid;
    static const int format = 0;

    static int length(const this_type&) { return 0; }
    static bool setNull(char* value) { return false; }
    static void fillBindData(std::vector<char>& dst, const this_type& v) {
        PgTraits<std::string>::fillBindData(dst, v.get());
    }
    static bool setValue(char* value, const char* data, int) {
        auto tmp = this_type::create(data);
        if (!tmp) {
            return false;
        }
        this_type* v = reinterpret_cast<this_type*>(value);
        *v = this_type(*tmp);
        return true;
    }
};

template<typename traits_t, typename base_t>
struct PgTraits<ParInt::BaseIntParam<traits_t, base_t>>
{
    typedef ParInt::BaseIntParam<traits_t, base_t> this_type;
    static const PgOid oid = PgTraits<base_t>::oid;
    static const int format = 1;

    static int length(const this_type&) { return sizeof(base_t); }
    static bool setNull(char* value) {
        this_type* v = reinterpret_cast<this_type*>(value);
        *v = this_type();
        return true;
    }
    static void fillBindData(std::vector<char>& dst, const this_type& v) {
        PgTraits<base_t>::fillBindData(dst, v.get());
    }
    static bool setValue(char* value, const char* data, int l) {
        base_t tmp1 = {};
        if (!PgTraits<base_t>::setValue(reinterpret_cast<char*>(&tmp1), data, l)) {
            return false;
        }
        this_type* v = reinterpret_cast<this_type*>(value);
        *v = this_type(tmp1);
        return true;
    }
};
} // details
} // PgCpp
#endif // ENABLE_PG
