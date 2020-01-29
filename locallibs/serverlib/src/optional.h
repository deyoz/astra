#ifndef SERVERLIB_OPTIONAL_H
#define SERVERLIB_OPTIONAL_H

#include <stdexcept>

namespace boost {  namespace serialization {  class access;  }  }

namespace ServerFramework
{

template<typename T>
class optional {
private:
    template<typename OptType>
    struct optional_traits
    {
        typedef OptType& ref;
        typedef const OptType& const_ref;
        typedef OptType* ptr;
        typedef const OptType* const_ptr;
        static ptr clone(const_ptr p) {
            return new OptType(*p);
        }
    };
    template<typename OptType>
    struct optional_traits<OptType&>
    {
        typedef OptType& ref;
        typedef const OptType& const_ref;
        typedef OptType* ptr;
        typedef const OptType* const_ptr;
        static ptr clone(const_ptr p) {
            return new OptType(*p);
        }
    };
    template<typename OptType>
    struct optional_traits<const OptType&>
    {
        typedef OptType& ref;
        typedef const OptType& const_ref;
        typedef OptType* ptr;
        typedef const OptType* const_ptr;
        static ptr clone(const_ptr p) {
            return new OptType(*p);
        }
    };

    typedef void (optional::*bool_type)() const;
public:
    typedef optional_traits<T> traits;
    optional() : p_(NULL) {}
    optional(typename traits::const_ref p) {
        p_ = NULL;
        reset__(&p);
    }
    optional(const optional<T>& rhp) {
        p_ = NULL;
        if (rhp) {
            reset__(rhp.get_ptr());
        }
    }
    ~optional() {
        if (p_)
            delete p_;
    }
    optional& operator=(typename traits::const_ref rhp) {
        reset__(&rhp);
        return *this;
    }
    optional& operator=(const optional<T>& rhp) {
        if (this != &rhp) {
            if (rhp) {
                reset__(rhp.get_ptr());
            } else {
                reset__(NULL);
            }
        }
        return *this;
    }
    void reset() {
        this->reset__(NULL);
    }
    void reset(typename traits::const_ref p) {
        this->reset__(&p);
    }
    explicit operator bool() const {  return p_ != nullptr;  }
    typename traits::ref get() { assert__(); return *p_; }
    typename traits::const_ref get() const { assert__(); return *p_; }
    typename traits::ref operator*() { assert__(); return *p_; }
    typename traits::const_ref operator*() const { assert__(); return *p_; }
    typename traits::ptr operator->() { assert__(); return p_; }
    typename traits::const_ptr operator->() const { assert__(); return p_; }
    
private:
    void assert__() const { if (!p_) throw std::runtime_error("empty optional"); }
    void reset__(typename traits::const_ptr p) {
        if (p) {
            delete p_;
            p_ = traits::clone(p);
        } else {
            delete p_;
            p_ = NULL;
        }
    }
    //T* get_ptr() { assert__(); return p_; }
    typename traits::const_ptr get_ptr() const { assert__(); return p_; }

    typename traits::ptr p_;
};

#define OptionalForSimpleTypes(TTT) \
template<> class optional<TTT> \
{ \
    typedef TTT T; \
    typedef optional<T> ThisType; \
    bool set; \
    T value; \
    void assert__() const {  if(not set) throw std::runtime_error("empty simple optional");  } \
    friend class boost::serialization::access; \
  public: \
    optional() : set(false), value(0) {} \
    optional(T p) {  reset(p);  } \
    optional(const ThisType& p) : set(p.set), value(p.value) {} \
    ThisType& operator=(T p) {  reset(p); return *this; } \
    ThisType& operator=(const ThisType& p) { if(p) { set = true; value = p.value; } else set = false; return *this; } \
    explicit operator bool() const { return set; } \
    void reset() {  set=false;  } \
    void reset(T p) {  set = true; value = p;  } \
    T& get() { assert__(); return value; } \
    const T& get() const { assert__(); return value; } \
    T& operator*() { assert__(); return value;  } \
    const T& operator*() const { assert__(); return value; } \
    T& operator->() { assert__(); return value; } \
    const T& operator->() const { assert__(); return value; } \
    template<class A> void serialize(A& ar, unsigned int /*version*/) {  ar & set & value;  } \
}

OptionalForSimpleTypes(int);
OptionalForSimpleTypes(size_t);

/*******************************************************************************
 * deep compare
 ******************************************************************************/

template<typename T>
inline bool operator==(const optional<T>& a, const T& b)
{
    return a && (*a == b);
}

template<typename T>
inline bool operator==(const optional<T>& a, const optional<T>& b)
{
    if (!a && !b)    return true;
    else if (a && b) return *a == *b;
    else             return false;
}

template<typename T>
inline bool operator==(const T& b, const optional<T>& a)
{
    return a == b;
}

template<typename T>
inline bool operator!=(const optional<T>& a, const T& b)
{
    return !(a == b);
}

template<typename T>
inline bool operator!=(const optional<T>& a, const optional<T>& b)
{
    return !(a == b);
}

template<typename T>
inline bool operator!=(const T& b, const optional<T>& a)
{
    return !(a == b);
}

} // namespace ServerFramework
namespace comtech = ServerFramework;
#endif /* SERVERLIB_OPTIONAL_H */

