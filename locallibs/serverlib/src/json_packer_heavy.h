#pragma once

#include <assert.h>
#include <string.h>
#include <string>
#include <vector>
#include <set>
#include <list>
#include <map>
#include <functional>
#include <stddef.h>
#include <memory>
#include <type_traits>

#include "json_packer.h"
#include "string_cast.h"
#include "slogger_nonick.h"
#include "value_or_null.h"
#include "makename_defs.h"
#include "exception.h"
#include "expected.h"
#include "json_spirit.h"

namespace json_spirit
{

// ----   FORWARD DECLARATIONS BEGIN   ---- //
template<typename T> struct Traits< boost::optional<T> >;
template<typename U, typename V> struct Traits< std::pair<U, V> >;
template<typename T> struct Traits< std::vector<T> >;
template<typename T> struct Traits< std::set<T> >;
template<typename T> struct Traits< std::list<T> >;
template<typename U, typename V> struct Traits< std::map<U, V> >;
template<typename U, typename V> struct Traits< Expected<U, V> >;
template<typename T> struct Traits< ValOrNull<T> >;

template<typename T>
auto getDesc();
// ----   FORWARD DECLARATIONS END   ---- //

template<typename T>
struct Traits< boost::optional<T> >
{
    static mValue packInt(const boost::optional<T>& v) {
        return v ? Traits<T>::packInt(*v) : mValue();
    }
    static UnpackResult< boost::optional<T> > unpackInt(const mValue& v) {
        if (v.type() == json_spirit::null_type) {
            return boost::optional<T>();
        }
        const UnpackResult<T> o(Traits<T>::unpackInt(v));
        if (o) {
            return boost::optional<T>(*o);
        } else {
            return o.err();
        }
    }
    static UnpackResult< boost::optional<T> > unpackIntIgnoreBad(const mValue& v) {
        return Traits<T>::unpackInt(v);
    }
    static mValue packExt(const boost::optional<T>& v) {
        return v ? Traits<T>::packExt(*v) : mValue();
    }
    static UnpackResult< boost::optional<T> > unpackExt(const mValue& v) {
        if (v.type() == json_spirit::null_type) {
            return boost::optional<T>();
        }
        const UnpackResult<T> o(Traits<T>::unpackExt(v));
        if (o) {
            return boost::optional<T>(*o);
        } else {
            return o.err();
        }
    }
    static UnpackResult< boost::optional<T> > unpackExtIgnoreBad(const mValue& v) {
        return Traits<T>::unpackExt(v);
    }
};

template<typename T>
struct ContainerTraits
{
    template<typename V>
    struct Inserted {
        Inserted(V& vs, const typename V::value_type& v) {
            vs.push_back(v);
        }
        operator bool() const { return true; }
    };
    template<typename V>
    struct Inserted< std::set<V> > {
        Inserted(std::set<V>& vs, const V& v) {
            val = vs.insert(v).second;
        }
        operator bool() const { return val; }
        bool val;
    };
    template<typename V, typename U>
    struct Inserted< std::map<V, U> > {
        Inserted(std::map<V, U>& vs, const std::pair<const V, U>& v) {
            val = vs.insert(v).second;
        }
        operator bool() const { return val; }
        bool val;
    };
    template<typename V>
    static bool inserted(V& vs, const typename V::value_type& v) {
        return Inserted<V>(vs, v) == true;
    }

    template<typename V>
    static void prepare(V&, size_t) {}

    template<typename V>
    static void prepare(typename std::vector<V>& vs, size_t n) {
        vs.reserve(n);
    }

    typedef typename T::value_type value_type;
    enum { isContainer };
    static mValue packInt(const T& v) {
        mArray arr;
        for (typename T::const_iterator i = v.begin(); i != v.end(); ++i) {
            arr.push_back(Traits<value_type>::packInt(*i));
        }
        return arr;
    }
    static UnpackResult< T > unpackInt(const mValue& v) {
        return unpack__(v, false, false);
    }
    static UnpackResult< T > unpackIntIgnoreBad(const mValue& v) {
        return unpack__(v, false, true);
    }
    static mValue packExt(const T& v) {
        mArray arr;
        for (typename T::const_iterator i = v.begin(); i != v.end(); ++i) {
            arr.push_back(Traits<value_type>::packExt(*i));
        }
        return arr;
    }
    static UnpackResult< T > unpackExt(const mValue& v) {
        return unpack__(v, true, false);
    }
    static UnpackResult< T > unpackExtIgnoreBad(const mValue& v) {
        return unpack__(v, true, true);
    }
    static UnpackResult< T > unpack__(const mValue& v, bool ext, bool ignoreBadValues) {
        JSON_ASSERT_TYPE(T, v, json_spirit::array_type);
        const mArray& arr(v.get_array());
        T ts;
        prepare(ts, arr.size());
        for (size_t i = 0; i < arr.size(); ++i) {
            UnpackResult<value_type> t(ext
                    ? Traits<value_type>::unpackExt(arr[i])
                    : Traits<value_type>::unpackInt(arr[i]));
            if (!t) {
                if (ignoreBadValues) {
                    LogTrace1 << "skip bad value at index " << i;
                    continue;
                } else {
                    return t.err().propagate("[" + std::to_string(i) + "]");
                }
            }
            if (!inserted(ts, *t)) {
                return UnpackError{"NONSTOP", __FILE__, __LINE__, "insert failed", {"[" + std::to_string(i) + "]"}};
            }
        }
        return ts;
    }
};

template<typename T>
struct Traits< std::vector<T> > : public ContainerTraits< std::vector<T> >
{};

template<typename T>
struct Traits< std::set<T> > : public ContainerTraits< std::set<T> >
{};

template<typename T>
struct Traits< std::list<T> > : public ContainerTraits< std::list<T> >
{};

template<typename U, typename V>
struct Traits< std::map<U, V> > : public ContainerTraits< std::map<U, V> >
{};

template<typename U, typename V>
struct Traits< std::pair<U, V> >
{
    static mValue packInt(const std::pair<U, V>& v) {
        mArray arr;
        arr.push_back(Traits<U>::packInt(v.first));
        arr.push_back(Traits<V>::packInt(v.second));
        return arr;
    }
    static UnpackResult< std::pair<U, V> > unpackInt(const mValue& v) {
        JSON_ASSERT_TYPE(pair_t, v, json_spirit::array_type);
        const mArray& arr(v.get_array());
        if (arr.size() != 2) {
            return UnpackError{"NONSTOP", __FILE__, __LINE__, "invalid size: " + std::to_string(arr.size())};
        }
        const UnpackResult<typename std::decay<U>::type> of(Traits<U>::unpackInt(arr[0]));
        if (!of) {
            return of.err().propagate("first");
        }
        const UnpackResult<typename std::decay<V>::type> os(Traits<V>::unpackInt(arr[1]));
        if (!os) {
            return os.err().propagate("second");
        }
        return std::pair<U, V>(*of, *os);
    }
    static mValue packExt(const std::pair<U, V>& v) {
        mArray arr;
        arr.push_back(Traits<U>::packExt(v.first));
        arr.push_back(Traits<V>::packExt(v.second));
        return arr;
    }
    static UnpackResult< std::pair<U, V> > unpackExt(const mValue& v) {
        JSON_ASSERT_TYPE(pair_t, v, json_spirit::array_type);
        const mArray& arr(v.get_array());
        if (arr.size() != 2) {
            return UnpackError{"NONSTOP", __FILE__, __LINE__, "invalid size: " + std::to_string(arr.size())};
        }
        const UnpackResult<typename std::decay<U>::type> of(Traits<U>::unpackExt(arr[0]));
        if (!of) {
            return of.err().propagate("first");
        }
        const UnpackResult<typename std::decay<V>::type> os(Traits<V>::unpackExt(arr[1]));
        if (!os) {
            return os.err().propagate("second");
        }
        return std::pair<U, V>(*of, *os);
    }
};

template<typename U, typename V>
struct Traits< Expected<U,V> >
{
    static mValue packInt(const Expected<U,V>& v) {
        mObject ret;
        if (v.valid())
            ret["val"] = Traits<U>::packInt(*v);
        else
            ret["err"] = Traits<V>::packInt(v.err());
        return ret;
    }

    static UnpackResult< Expected<U,V> > unpackInt(const mValue& v) {
        JSON_ASSERT_TYPE(Type, v, json_spirit::obj_type);
        const mObject& o( v.get_obj() );

        mObject::const_iterator tmp, oend = o.end();
        tmp = o.find( "val" );
        if( tmp != oend) {
            const UnpackResult<U> of(Traits<U>::unpackInt(v));
            if (!of) {
                return of.err().propagate("val");
            }
            return *of;
        }
        tmp = o.find( "err" );
        if( tmp != oend) {
            const UnpackResult<V> of(Traits<V>::unpackInt(v));
            if (!of) {
                return of.err().propagate("err");
            }
            return *of;
        }
        return UnpackError{"NONSTOP", __FILE__, __LINE__, "no 'val' neither 'err' tags"};
    }

    static mValue packExt(const Expected<U,V>& v) {
        mObject ret;
        if (v.valid())
            ret["val"] = Traits<U>::packExt(*v);
        else
            ret["err"] = Traits<V>::packExt(v.err());
        return ret;
    }

    static UnpackResult< Expected<U,V> > unpackExt(const mValue& v) {
        JSON_ASSERT_TYPE(Type, v, json_spirit::obj_type);
        const mObject& o( v.get_obj() );

        mObject::const_iterator tmp, oend = o.end();
        tmp = o.find( "val" );
        if( tmp != oend) {
            const UnpackResult<U> of(Traits<U>::unpackExt(v));
            if (!of) {
                return of.err().propagate("val");
            }
            return *of;
        }
        tmp = o.find( "err" );
        if( tmp != oend) {
            const UnpackResult<V> of(Traits<V>::unpackExt(v));
            if (!of) {
                return of.err().propagate("err");
            }
            return *of;
        }
        return UnpackError{"NONSTOP", __FILE__, __LINE__, "no 'val' neither 'err' tags"};
    }
};

template<typename T>
struct Traits< ValOrNull<T> >
{
    static mValue packInt(const ValOrNull<T>& v) {
        if (!v.val) {
            return mValue("-");
        } else {
            return Traits<T>::packInt(*v.val);
        }
    }
    static UnpackResult< ValOrNull<T> > unpackInt(const mValue& v) {
        if (v.type() == json_spirit::str_type && v.get_str() == "-") {
            return ValOrNull<T>(); // empty, but existing
        }
        const auto t(Traits<T>::unpackInt(v));
        if (!t) {
            return t.err().propagate("val");
        }
        return ValOrNull<T>(*t);
    }
    static mValue packExt(const ValOrNull<T>& v) {
        if (!v.val) {
            return mValue("-");
        } else {
            return Traits<T>::packExt(*v.val);
        }
    }
    static UnpackResult< ValOrNull<T> > unpackExt(const mValue& v) {
        if (v.type() == json_spirit::str_type && v.get_str() == "-") {
            return ValOrNull<T>(); // empty, but existing
        }
        const auto t(Traits<T>::unpackExt(v));
        if (!t) {
            return t.err().propagate("val");
        }
        return ValOrNull<T>(*t);
    }
};

template<typename T>
void packField(mObject& ret, const std::string& tag, const T& val, const bool internal)
{
    mValue packed = internal ? Traits< T >::packInt(val) : Traits< T >::packExt(val);
    if (!packed.is_null()) {
        ret[ tag ] = packed;
    }
}

template<typename T>
boost::optional<UnpackError> unpackField(T& val, const std::string& tag, const mObject& obj,
                 const bool internal, const bool shouldExist = false)
{
    mObject::const_iterator it( obj.find( tag ) );
    if ( obj.end() == it ) {
        if ( shouldExist ) {
            return UnpackError{"NONSTOP", __FILE__, __LINE__, "tag '" + tag + "' not found"};
        }
        return boost::none;
    }
    const UnpackResult<T> value = internal
        ? Traits< T >::unpackInt( it->second )
        : Traits< T >::unpackExt( it->second );
    if (!value) {
        return value.err().propagate(tag);
    }
    val = *value;
    return boost::none;
}

#define TRY_UNPACK_FIELD(val, tag, obj, internal, shouldExist) \
if (const auto err = unpackField(val, tag, obj, internal, shouldExist)) { \
    return err->propagate(tag); \
}

template< typename T >
UnpackResult< T > unpackField( std::string const &tag, mObject const &obj, bool internal, std::false_type )
{
    mObject::const_iterator const it( obj.find( tag ) );
    if( obj.end() == it )
        return UnpackResult< T >( T() );

    UnpackResult< T > const value( internal
        ? Traits< T >::unpackInt( it->second )
        : Traits< T >::unpackExt( it->second ) );

    return value.valid() ? value : value.err().propagate( tag );
}

template< typename T >
UnpackResult< T > unpackField( std::string const &tag, mObject const &obj, bool internal, std::true_type )
{
    mObject::const_iterator const it( obj.find( tag ) );
    if( obj.end() == it )
        return UnpackError{ "NONSTOP", __FILE__, __LINE__, "tag '" + tag + "' not found" };

    UnpackResult< T > const value( internal
        ? Traits< T >::unpackInt( it->second )
        : Traits< T >::unpackExt( it->second ) );

    return value.valid() ? value : value.err().propagate( tag );
}

#define TRY_UNPACK_FIELD_VAR( type, vname, tag, obj, internal, shouldExist ) \
UnpackResult< type > const vname = unpackField< type >( tag, obj, internal, \
    std::integral_constant< bool, shouldExist >() ); \
if( !vname ) return vname.err();

namespace details {

template <typename T, typename Tuple, std::size_t... S>
static auto make_from_tuple(const Tuple& t, std::index_sequence<S...>)
{
    return T{*std::get<S>(t)...};
}

template <typename T>
static auto packHelper(bool intMode, const T& v)
{
    if (intMode) {
        return Traits<T>::packInt(v);
    } else {
        return Traits<T>::packExt(v);
    }
}

template <typename T>
static auto unpackHelper(bool intMode, const mValue& m)
{
    if (intMode) {
        return Traits<T>::unpackInt(m);
    } else {
        return Traits<T>::unpackExt(m);
    }
}

template<typename T, typename FieldType>
struct TagField;
template<typename T, typename FieldType>
struct TagFieldContainer;
template<typename T, typename FieldType>
struct TagFieldWithDefault;
template<typename T, typename FieldType>
struct TagFieldIgnoreBad;

template <typename T, typename FieldType>
UnpackResult<FieldType> unpackTag(bool intMode, const mObject& o,
        const TagFieldIgnoreBad<T, FieldType>& tagField)
{
    const auto& tag = tagField.tag;
    auto it = o.find(tag);
    if (it == o.end()) {
        return UnpackError{"JSON", __FILE__, __LINE__, "tag '" + tag + "' not found"};
    }
    return intMode
        ? json_spirit::Traits<FieldType>::unpackIntIgnoreBad(it->second)
        : json_spirit::Traits<FieldType>::unpackExtIgnoreBad(it->second);
}

template <typename T, typename FieldType>
UnpackResult<FieldType> unpackTag(bool intMode, const mObject& o,
        const TagField<T, FieldType>& tagField)
{
    const auto& tag = tagField.tag;
    auto it = o.find(tag);
    if (it == o.end()) {
        return UnpackError{"JSON", __FILE__, __LINE__, "tag '" + tag + "' not found"};
    }
    return unpackHelper<FieldType>(intMode, it->second);
}

template <typename T, typename FieldType>
UnpackResult<boost::optional<FieldType>> unpackTag(bool intMode, const mObject& o,
        const TagField<T, boost::optional<FieldType>>& tagField)
{
    const auto& tag = tagField.tag;
    auto it = o.find(tag);
    if (it == o.end()) {
        return boost::optional<FieldType>();
    }
    return unpackHelper<boost::optional<FieldType>>(intMode, it->second);
}

template <typename T, typename FieldType>
UnpackResult<FieldType> unpackTag(bool intMode, const mObject& o,
        const TagFieldContainer<T, FieldType>& tagField)
{
    const auto& tag = tagField.tag;
    auto it = o.find(tag);
    if (it == o.end()) {
        return FieldType{};
    }
    return unpackHelper<FieldType>(intMode, it->second);
}

template <typename T, typename FieldType>
UnpackResult<FieldType> unpackTag(bool intMode, const mObject& o,
        const TagFieldWithDefault<T, FieldType>& tagField)
{
    const auto& tag = tagField.tag;
    auto it = o.find(tag);
    if (it == o.end()) {
        return tagField.defaultVal;
    }
    return unpackHelper<FieldType>(intMode, it->second);
}

template<typename T, typename FieldType>
const FieldType* defaultValuePtr(const TagField<T, FieldType>&)
{
    return nullptr;
}

template<typename T, typename FieldType>
const FieldType* defaultValuePtr(const TagFieldIgnoreBad<T, FieldType>&)
{
    return nullptr;
}

template<typename T, typename FieldType>
const FieldType* defaultValuePtr(const TagFieldWithDefault<T, FieldType>& f)
{
    return &f.defaultVal;
}

template<typename Tuple, size_t ...S >
auto unpack_fields_impl(bool intMode, const json_spirit::mObject& o, Tuple&& t, std::index_sequence<S...>)
{
    // remove 'unused variable' warning when the tuple is empty
    static_cast<void>(intMode);

    return std::make_tuple(
        unpackTag(
            intMode,
            o,
            std::get<S>(t)
        )...);
}

template<typename Tuple>
auto unpack_fields(bool intMode, const json_spirit::mObject& o, Tuple&& t)
{
    std::size_t constexpr tSize = std::tuple_size<typename std::remove_reference<Tuple>::type>::value - 1;
    return unpack_fields_impl(intMode, o, std::forward<Tuple>(t), std::make_index_sequence<tSize>());
}

template<int N>
struct Checker;

template<>
struct Checker<0>
{
    template<typename Results, typename Fields>
    static boost::optional<UnpackError> check(const Results& results, const Fields& fields) {
        // do nothing
        return boost::none;
    }
};

template<int N>
struct Checker
{
    template<typename Results, typename Fields>
    static boost::optional<UnpackError> check(const Results& results, const Fields& fields) {
        if (!std::get<N - 1>(results)) {
            return std::get<N - 1>(results).err().propagate(std::get<N - 1>(fields).tag);
        }
        return Checker<N - 1>::check(results, fields);
    }
};

template<int N>
struct Packer;

template<>
struct Packer<0>
{
    template<typename T, typename Fields>
    static void pack(bool intMode, json_spirit::mObject& o, const Fields& fields, const T& v) {
        // do nothing
    }
};

template<typename T, typename FieldType>
bool needInsert(const FieldType&, const TagField<T, FieldType>&)
{
    return true;
}

template<typename T, typename FieldType>
bool needInsert(const boost::optional<FieldType>& val, const TagField<T, boost::optional<FieldType>>&)
{
    return val != boost::none;
}

template<typename T, typename FieldType>
bool needInsert(const FieldType& val, const TagFieldContainer<T, FieldType>&)
{
    return !val.empty();
}

template<typename T, typename FieldType>
bool needInsert(const FieldType& val, const TagFieldWithDefault<T, FieldType>& tf)
{
    return !(val == tf.defaultVal);
}

template<typename T, typename FieldType>
bool needInsert(const FieldType&, const TagFieldIgnoreBad<T, FieldType>&)
{
    return true;
}

template<int N>
struct Packer
{
    template<typename T, typename Fields>
    static void pack(bool intMode, json_spirit::mObject& o, const Fields& fields, const T& v) {
        const auto& tf(std::get<N - 1>(fields));
        if (needInsert(v.*tf.field, tf)) {
            const auto res(o.insert(std::make_pair(tf.tag, packHelper(intMode, v.*tf.field))));
            if (!res.second) {
                throw std::runtime_error("tag " + tf.tag + " insert failed");
            }
        }
        Packer<N - 1>::pack(intMode, o, fields, v);
    }
};

template<typename T, typename TupleT>
struct Desc
{
    typedef T result_t;
    typedef TupleT fields_t;

    Desc(const fields_t& f)
        : fields(f)
    {
    }

    virtual ~Desc() = default;

    json_spirit::mValue packInt(const T& v)
    {
        HEADER_ASSERT(allow_packInt<T>::allow && "packInt is disabled for this type");
        return pack(true, v);
    }

    UnpackResult<result_t> unpackInt(const json_spirit::mValue& v)
    {
        HEADER_ASSERT(allow_unpackInt<T>::allow && "unpackInt is disabled for this type");
        return unpack(true, v);
    }

    json_spirit::mValue packExt(const T& v)
    {
        HEADER_ASSERT(allow_packExt<T>::allow && "packExt is disabled for this type");
        return pack(false, v);
    }

    UnpackResult<result_t> unpackExt(const json_spirit::mValue& v)
    {
        HEADER_ASSERT(allow_unpackExt<T>::allow && "unpackExt is disabled for this type");
        return unpack(false, v);
    }

    json_spirit::mValue pack(bool intMode, const T& v) {
        json_spirit::mObject o;
        Packer<std::tuple_size<fields_t>::value - 1>::pack(intMode, o, fields, v);
        return o;
    }

    virtual UnpackResult<result_t> unpack(bool intMode, const json_spirit::mValue& m) {
        if (m.type() != json_spirit::obj_type) {
            return UnpackError{"JSON", __FILE__, __LINE__, "invalid type: object expected"};
        }
        const auto res = unpack_fields(intMode, m.get_obj(), fields);
        if (boost::optional<UnpackError> err = Checker<std::tuple_size<decltype(res)>::value>::check(res, fields)) {
            return *err;
        }
        return make_from_tuple<result_t>(res, std::make_index_sequence<std::tuple_size<decltype(res)>::value>{});
    }

    fields_t fields;
};

template <typename T1, typename T2, typename TupleT, typename ConvFunc>
struct Desc2: public Desc<T1, TupleT>
{
    Desc2(const TupleT& f, ConvFunc&& convFunc)
        : Desc<T1, TupleT>(f), conv(convFunc)
    {
    }

    UnpackResult<T1> unpack(bool intMode, const json_spirit::mValue& m) override
    {
        const auto res1 = Desc<T1, TupleT>::unpack(intMode, m);
        if (res1) {
            return *res1;
        }

        const auto res2 = unpackHelper<T2>(intMode, m);
        if (!res2) {
            return res2.err();
        }

        const auto value = conv(*res2);
        if (value) return *value;
        return UnpackError{"NONSTOP", __FILE__, __LINE__, "convertion failed"};
    }

    ConvFunc conv;
};

template<typename T, typename FieldType>
struct TagField
{
    using obj_t = T;
    using field_t = FieldType;

    std::string tag;
    FieldType T::* field;

    TagFieldWithDefault<T, FieldType> setDefault(const FieldType&);
};

template<typename T, typename FieldType>
struct TagFieldContainer
{
    using obj_t = T;
    using field_t = FieldType;

    std::string tag;
    FieldType T::* field;

    TagFieldWithDefault<T, FieldType> setDefault(const FieldType&);
};

template<typename T, typename FieldType>
struct TagFieldWithDefault
{
    using obj_t = T;
    using field_t = FieldType;

    std::string tag;
    FieldType T::* field;
    FieldType defaultVal;
};

template <typename T, typename FieldType>
struct TagFieldIgnoreBad
{
    using obj_t = typename TagField<T, FieldType>::obj_t;
    using field_t = typename TagField<T, FieldType>::field_t;

    std::string tag;
    FieldType T::* field;
};

template<typename T, typename FieldType>
TagFieldWithDefault<T, FieldType> TagField<T, FieldType>::setDefault(const FieldType& d)
{
    return TagFieldWithDefault<T, FieldType>{tag, field, d};
}

template<typename T, typename FieldType>
TagFieldWithDefault<T, FieldType> TagFieldContainer<T, FieldType>::setDefault(const FieldType& d)
{
    return TagFieldWithDefault<T, FieldType>{tag, field, d};
}

template<typename T, typename FieldType, typename = decltype(std::declval<Traits<FieldType>>().isContainer)>
auto declTag(std::string&& t, FieldType T::* field)
{
    return TagFieldContainer<T, FieldType>{t, field};
}

template<typename T, typename FieldType, typename... Ignored>
auto declTag(std::string&& t, FieldType T::* field, Ignored const&...)
{
    return TagField<T, FieldType>{t, field};
}

template <typename T, typename FieldType>
auto declTagIgnoreBad(std::string&& t, FieldType T::* field)
{
    static_assert(std::is_base_of<ContainerTraits<FieldType>, Traits<FieldType>>::value,
        "don't use NEW_DESC_TYPE_FIELD_IGNORE_BAD without a container");
    return TagFieldIgnoreBad<T, FieldType>{t, field};
}

template <typename T, typename TupleT>
auto makeDesc(const TupleT& tp)
{
    return Desc<T, TupleT>(tp);
}

template <typename T, typename T2, typename TupleT, typename ConvFunc>
auto makeDesc2(ConvFunc&& conv, const TupleT& tp)
{
    static_assert(!std::is_same<T, T2>::value, "don't use NEW_DESC_TYPE_FIELD2 with T == T2");
    return Desc2<T, T2, TupleT, ConvFunc>(tp, conv);
}

} // details

} // json_spirit

#define JSON_BEGIN_DESC_TYPE(Type) template<> auto getDesc<Type>() {\
using obj_t = Type;\
static auto p = details::makeDesc<Type>(std::make_tuple(

#define JSON_BEGIN_DESC_TYPE2(Type1, Type2, convFunc) template<> auto getDesc<Type1>() {\
using obj_t = Type1;\
static auto p = details::makeDesc2<Type1, Type2>(convFunc, std::make_tuple(

#define JSON_END_DESC_TYPE(Type) 0));\
return &p;\
}\
mValue Traits<Type>::packInt(const Type& v) { return getDesc<Type>()->packInt(v); }\
UnpackResult<Type> Traits<Type>::unpackInt(const mValue& v) { return getDesc<Type>()->unpackInt(v); }\
mValue Traits<Type>::packExt(const Type& v) { return getDesc<Type>()->packExt(v); }\
UnpackResult<Type> Traits<Type>::unpackExt(const mValue& v) { return getDesc<Type>()->unpackExt(v); }

#define JSON_END_DESC_TYPE_WITH_AFTER_UNPACK(Type, AfterUnpack) 0));\
return &p;\
}\
mValue Traits<Type>::packInt(const Type& v) { return getDesc<Type>()->packInt(v); }\
UnpackResult<Type> Traits<Type>::unpackInt(const mValue& v) { \
auto res = getDesc<Type>()->unpackInt(v); \
if (res) AfterUnpack(*res, v, true); \
return res; \
}\
mValue Traits<Type>::packExt(const Type& v) { return getDesc<Type>()->packExt(v); }\
UnpackResult<Type> Traits<Type>::unpackExt(const mValue& v) { \
auto res = getDesc<Type>()->unpackExt(v); \
if (res) AfterUnpack(*res, v, false); \
return res; \
}

#define DESC_TYPE_FIELD(tag, fld) details::declTag(tag, &obj_t::fld),
#define DESC_TYPE_FIELD2(tag, fld, def) details::declTag(tag, &obj_t::fld).setDefault(def),
#define DESC_TYPE_FIELD_IGNORE_BAD(tag, fld) details::declTagIgnoreBad(tag, &obj_t::fld),

// ParInt
#define JSON_PARINT_PACK_INT(ParIntType) \
mValue Traits< ParIntType >::packInt(const ParIntType& t) { \
    return Traits< ParIntType::base_type >::packInt(ParInt::getRef(t)); \
} \
UnpackResult< ParIntType > Traits< ParIntType >::unpackInt(const mValue& v) { \
    const UnpackResult< ParIntType::base_type > opt(Traits< ParIntType::base_type >::unpackInt(v)); \
    if (opt) { \
        return ParIntType(*opt); \
    } else { \
        return opt.err(); \
    } \
}

#define JSON_PARINT_PACK_EXT(ParIntType) \
mValue Traits< ParIntType >::packExt(const ParIntType& t){ \
    return Traits< ParIntType::base_type >::packExt(ParInt::getRef(t)); \
} \
UnpackResult< ParIntType > Traits< ParIntType >::unpackExt(const mValue& v) { \
    const UnpackResult< ParIntType::base_type > opt(Traits< ParIntType::base_type >::unpackExt(v)); \
    if (opt) { \
        return ParIntType(*opt); \
    } else { \
        return opt.err(); \
    } \
}

#define JSON_PARINT_PACK(ParIntType) \
    JSON_PARINT_PACK_INT(ParIntType) \
    JSON_PARINT_PACK_EXT(ParIntType) \

// Rip
#define JSON_RIP_PACK_INT(RipType) \
mValue Traits< RipType >::packInt(const RipType& t) { \
    return Traits< RipType::base_type >::packInt(t.get()); \
} \
UnpackResult< RipType > Traits< RipType >::unpackInt(const mValue& v) { \
    const UnpackResult< RipType::base_type > opt(Traits< RipType::base_type >::unpackInt(v)); \
    if (opt) { \
        if (!RipType::validate(*opt)) { \
            return UnpackError{STDLOG, "invalid " #RipType "(" + HelpCpp::string_cast(*opt) + ")"}; \
        } \
        return RipType(*opt); \
    } \
    return opt.err(); \
}

#define JSON_RIP_PACK_EXT(RipType) \
mValue Traits< RipType >::packExt(const RipType& t){ \
    return Traits< RipType::base_type >::packExt(t.get()); \
} \
UnpackResult< RipType > Traits< RipType >::unpackExt(const mValue& v) { \
    const UnpackResult< RipType::base_type > opt(Traits< RipType::base_type >::unpackExt(v)); \
    if (opt) { \
        if (!RipType::validate(*opt)) { \
            return UnpackError{STDLOG, "invalid " #RipType "(" + HelpCpp::string_cast(*opt) + ")"}; \
        } \
        return RipType(*opt); \
    } \
    return opt.err(); \
}

#define JSON_RIP_PACK(RipType) \
    JSON_RIP_PACK_INT(RipType) \
    JSON_RIP_PACK_EXT(RipType) \

// Enums
#define JSON_PACK_UNPACK_ENUM(Type) \
mValue Traits< Type >::packInt(const Type& s) { \
    return mValue(static_cast<int>(s)); \
} \
mValue Traits< Type >::packExt(const Type& s) { \
    return mValue(enumToStr(s)); \
} \
UnpackResult< Type > Traits< Type >::unpackInt(const mValue& v) { \
    JSON_ASSERT_TYPE( Type, v, json_spirit::int_type ); \
    const UnpackResult<int> val(json_spirit::Traits<int>::unpackInt(v)); \
    if (!val) { \
        return val.err(); \
    } \
    if (enumToStr2( static_cast< Type >(*val) ) == nullptr) { \
        return UnpackError{STDLOG, "unknown enum value " + std::to_string(*val)}; \
    } \
    return static_cast< Type >(*val); \
} \
UnpackResult< Type > Traits<Type>::unpackExt(const mValue& v) { \
    JSON_ASSERT_TYPE( Type, v, json_spirit::str_type ); \
    Type e; \
    if (!enumFromStr(e, v.get_str())) { \
        return UnpackError{STDLOG, "unknown enum value [" + v.get_str() + ']'}; \
    } \
    return e; \
}
