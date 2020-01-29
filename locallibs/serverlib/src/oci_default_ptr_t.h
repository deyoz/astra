#pragma once

#include <type_traits>
#include <utility>

namespace OciCpp {

template <class T> void default_ptr_t_fill(void *out, T const& m) { *static_cast<T*>(out) = m; }

class default_ptr_t // yeeeeah... a bit longer than just using buf_ptr_t = boost::shared_ptr
{
    // but uses nice and sexy Type Erasure Parent Style
    // and also _way_ easier to use
    struct concept_t {
        virtual void fill(void* out) const = 0;
        virtual ~concept_t() = default;
    };
    concept_t* ptr = nullptr;

    template <class T> struct fits_sbo
        : std::integral_constant<bool, sizeof(T) < sizeof(ptr) and std::is_trivially_copyable<T>::value> {};
    void set_pod(void const* t_ptr, unsigned sz) noexcept;

    default_ptr_t() = default;
  public:
    default_ptr_t(const default_ptr_t&) = delete;
    default_ptr_t(default_ptr_t&& x) { std::swap(ptr, x.ptr); }
    default_ptr_t& operator=(const default_ptr_t&) = delete;
    default_ptr_t& operator=(default_ptr_t&& x) noexcept { std::swap(ptr, x.ptr); return *this; }

    template <class T> explicit default_ptr_t(T&& t)
    {
        if /*constexpr*/ (fits_sbo<T>::value) {
            set_pod(&t, sizeof(t));
        } else {
            ptr = new model_t<std::decay_t<T>>(std::forward<T>(t));
        }
    }
    ~default_ptr_t();

    static default_ptr_t none() noexcept { return {}; }

    explicit operator bool () const noexcept { return ptr; }

    void fill(void* out) const noexcept;

  private:
    template <typename T> struct model_t : public concept_t
    {
        model_t() = default;
        model_t(const T& v) : m_data(v) {}
        model_t(T&& v) : m_data(std::move(v)) {}
        void fill(void* out) const override final { default_ptr_t_fill(out, m_data); }
        T m_data;
    };
};

} // namespace OciCpp
