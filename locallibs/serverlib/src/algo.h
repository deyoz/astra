#pragma once

/*******************************************************************************
 * Reference manual and concepts:
 * http://redmine.komtex/projects/testmigrate/wiki/Serverlibalgoh_reference
 * ...and a lot of examples in algo.cc
 *
 * Please make yourself familiar with algo.h concepts
 * before trying to add/modify the code
 *******************************************************************************/

//!!!!! PLEASE DO NOT ADD ADDITIONAL DEPENDENCIES !!!!!
// ... even a standard library headers like <vector>, <map>, <set> etc
// use metaprogramming instead (e.g. compile-time checking for a class method)
// to write specializations
#include <iterator>
#include <utility>
#include <type_traits>
#include <algorithm>
#include <functional>

namespace algo {
namespace details {

// due to Walter E. Brown
// "Modern Template Metaprogramming: A Compendium, Part II"
// cppcon 2014
//
// template<typename...>
// using void_t = void;
//
// Until CWG 1558 (a C++14 defect), unused parameters in alias templates
// were not guaranteed to ensure SFINAE and could be ignored, so earlier
// compilers require a more complex definition of void_t
//
// should be in C++17 standard library
template<typename...> struct make_void { using type = void; };
template<typename... Ts> using void_t = typename make_void<Ts...>::type;

// Not defined; for use with decltype only.
// decltype(front(container)) allows to get a contained data type,
// given that std::begin can be applied.
// Does not require value_type in a container class, so it can
// be used with plain arrays etc.
template<typename Container>
auto front(Container&& container) ->
    typename std::decay<decltype(*std::begin(container))>::type;

// get a contained data type
template<typename Container>
using value_type = decltype(front(std::declval<Container&>()));
template<typename Container>
using key_type = typename Container::key_type;
template<typename Container>
using mapped_type = typename Container::mapped_type;

// check if a container looks like a map
template<typename Container, typename = void>
struct is_map: public std::false_type {};
template<typename Container>
struct is_map<Container, void_t<mapped_type<Container> > >
    : public std::true_type {};

// check presence of Container::push_back(value_type)
template<typename Container>
using push_back_method_t = decltype(std::declval<Container>().push_back(
            std::declval<value_type<Container> >()));
template<typename Container, typename = void>
struct provides_push_back: public std::false_type {};
template<typename Container>
struct provides_push_back<Container, void_t<push_back_method_t<Container> > >
    : public std::true_type {};

// check presence of Container::find(value_type)
template<typename Container, typename T>
using find_method_t = decltype(
        std::end(std::declval<Container>())
        == std::declval<Container>().find(std::declval<T>()));
template<typename Container, typename T, typename = void>
struct provides_find: public std::false_type {};
template<typename Container, typename T>
struct provides_find<Container, T, void_t<find_method_t<Container, T> > >
    : public std::true_type {};

// call reserve() if a such method is available in
// a given dst container and a src container has size()

template<typename DstContainer, typename SrcContainer>
inline auto reserve_same_size_inner(DstContainer* dst, const SrcContainer* src) ->
    decltype(dst->reserve(0), src->size(), (void)0)
{
    dst->reserve(src->size());
}

// SFINAE fallback
inline void reserve_same_size_inner(...) {}

template<typename DstContainer, typename SrcContainer>
inline void reserve_same_size(DstContainer& dst, const SrcContainer& src)
{
    reserve_same_size_inner(&dst, &src);
}

// std::back_inserter or just std::inserter, depending on
// ability to push_back()

template<typename Container>
inline auto end_inserter(Container& container) ->
    typename std::enable_if<
        provides_push_back<Container>::value,
        decltype(std::back_inserter(container))
    >::type
{
    return std::back_inserter(container);
}

template<typename Container>
inline auto end_inserter(Container& container) ->
    typename std::enable_if<
        !provides_push_back<Container>::value,
        decltype(std::inserter(container, std::end(container)))
    >::type
{
    return std::inserter(container, std::end(container));
}

// some meaningful description
template<typename DstContainer, typename SrcContainer>
inline auto reserve_or_resize(DstContainer& dst, const SrcContainer& src) ->
    decltype(dst->resize(src->size()), dst->begin())
{
    dst.resize(src.size());
    return dst.begin();
}

template<typename DstContainer, typename SrcContainer>
inline auto reserve_or_resize(DstContainer& dst, const SrcContainer& src)
{
    reserve_same_size_inner(&dst, &src);
    return end_inserter(dst);
}

//
// overloaded find() (for internal usage only)
//

// general-purpose find via linear search
//
// WARNING:
// returning a pointer to an element inside a const container is
// very dangerous; for internal use only
template<typename Container, typename T>
inline auto find(const Container& container, const T& val) ->
    typename std::enable_if<
        !provides_find<Container, T>::value,
        const decltype(front(container))*
    >::type
{
    const auto it = std::find(std::begin(container), std::end(container), val);
    return it != std::end(container) ? &*it : nullptr;
}

// works for std::set and similar containers, given that
// find(value) mathod is provided
//
// WARNING:
// returning a pointer to an element inside a const container is
// very dangerous; for internal use only
template<typename Container, typename T>
inline auto find(const Container& container, const T& val) ->
    typename std::enable_if<
        provides_find<Container, T>::value && !is_map<Container>::value,
        const decltype(front(container))*
    >::type
{
    const auto it = container.find(val);
    return it != std::end(container) ? &*it : nullptr;
}

// works for std::map (and similar types)
//
// WARNING:
// returning a pointer to an element inside a const container is
// very dangerous; for internal use only
template<typename Map>
inline const mapped_type<Map>* find(const Map& container, const key_type<Map>& key)
{
    const auto it = container.find(key);
    return it != std::end(container) ? &it->second : nullptr;
}

} // namespace details

/*******************************************************************************
 * functional-style analog of std::transform
 ******************************************************************************/

namespace details {

template< typename OutputContainer, typename InputContainer, typename UnaryFunction >
inline OutputContainer transform(const InputContainer& in, UnaryFunction&& func)
{
    OutputContainer result;
    auto output_iterator = details::reserve_or_resize(result, in);
    std::transform(std::begin(in), std::end(in), output_iterator, std::forward<UnaryFunction>(func));
    return result;
}

template< template<typename...> typename OutputContainer, typename InputContainer, typename UnaryFunction >
inline auto transform(const InputContainer& in, UnaryFunction func)
{
    using std::begin;
    using OutputContainer_ = OutputContainer<std::decay_t<decltype(std::invoke(func, *begin(in)))>>;
    OutputContainer_ result;
    auto output_iterator = details::reserve_or_resize(result, in);
    std::transform(std::begin(in), std::end(in), output_iterator, [&](auto&& i){ return std::invoke(func, i); });
    return result;
}

} // namespace details

// explicitly specified output container with explicitly specified value type
// may handle somewhat tricky cases like std::map -> std::map
template<
    typename OutputContainer,
    typename InputContainer,
    typename UnaryFunction
    >
inline OutputContainer transform(const InputContainer& in, UnaryFunction&& func)
{
    return details::transform<OutputContainer>(in, std::forward<UnaryFunction>(func));
}

// no change in container and value type
template<typename Container, typename UnaryFunction>
inline Container transform(const Container& in, UnaryFunction&& func)
{
    return details::transform<Container>(in, std::forward<UnaryFunction>(func));
}

// explicitly specified output container with deduced value type
template<
    template<typename...> class OutputContainer,
    typename InputContainer,
    typename UnaryFunction
    >
inline auto transform(const InputContainer& in, UnaryFunction func)
{
    return details::transform<OutputContainer, InputContainer, UnaryFunction>(in, func);
}

// output container = input container (with deduced output value type)
template<
    template<typename...> class InputContainer,
    typename InputElement,
    typename UnaryFunction
    >
inline auto transform(const InputContainer<InputElement>& in, UnaryFunction func)
{
    return details::transform<InputContainer, InputContainer<InputElement>, UnaryFunction>(in, func);
}

// overload for initializer_list
template<
    template<typename...> class OutputContainer,
    typename InputElement,
    typename UnaryFunction
    >
inline auto transform(const std::initializer_list<InputElement>& in, UnaryFunction&& func)
{
    return details::transform<OutputContainer, std::initializer_list<InputElement>, UnaryFunction>(in, func);
}

/*******************************************************************************
 * functional-style analog of std::copy_if
 ******************************************************************************/

namespace details {
template<
    typename OutputContainer,
    typename InputContainer,
    typename UnaryPredicate
    >
inline OutputContainer filter(const InputContainer& in, UnaryPredicate&& pred)
{
    OutputContainer result;
    std::copy_if(std::begin(in), std::end(in), details::end_inserter(result), std::forward<UnaryPredicate>(pred));
    return result;
}
} // namespace details

// explicitly specified output container
template<
    typename OutputContainer,
    typename InputContainer,
    typename UnaryPredicate
    >
inline OutputContainer filter(const InputContainer& in, UnaryPredicate&& pred)
{
    return details::filter<OutputContainer>(in, std::forward<UnaryPredicate>(pred));
}

// output container = input container
template<
    typename InputContainer,
    typename UnaryPredicate
    >
inline InputContainer filter(const InputContainer& in, UnaryPredicate&& pred)
{
    return details::filter<InputContainer>(in, std::forward<UnaryPredicate>(pred));
}

// explicitly specified output container with deduced value type
template<
    template<typename...> class OutputContainer,
    typename InputContainer,
    typename UnaryPredicate
    >
inline auto filter(const InputContainer& in, UnaryPredicate&& pred)
    -> OutputContainer<decltype(details::front(in))>
{
    using OutputContainer_ = OutputContainer<decltype(details::front(in))>;
    return details::filter<OutputContainer_>(in, std::forward<UnaryPredicate>(pred));
}

// overload for initializer_list
template<
    template<typename...> class OutputContainer,
    typename InputElement,
    typename UnaryPredicate
    >
inline OutputContainer<typename std::remove_const<InputElement>::type>
filter(const std::initializer_list<InputElement>& in, UnaryPredicate&& pred)
{
    using OutputContainer_ = OutputContainer<typename std::remove_const<InputElement>::type>;
    return details::filter<OutputContainer_>(in, std::forward<UnaryPredicate>(pred));
}

/*******************************************************************************
 * Simple wrappers to avoid passing (begin, end)
 ******************************************************************************/

template<typename Container, typename UnaryPredicate>
inline bool all_of(const Container& container, UnaryPredicate&& pred)
{
    return std::all_of(std::begin(container), std::end(container), std::forward<UnaryPredicate>(pred));
}

template<typename Container, typename UnaryPredicate>
inline bool any_of(const Container& container, UnaryPredicate&& pred)
{
    return std::any_of(std::begin(container), std::end(container), std::forward<UnaryPredicate>(pred));
}

template<typename Container, typename UnaryPredicate>
inline bool none_of(const Container& container, UnaryPredicate&& pred)
{
    return std::none_of(std::begin(container), std::end(container), std::forward<UnaryPredicate>(pred));
}

template<typename Container, typename T>
inline auto count(const Container& container, const T& val)
    -> typename std::iterator_traits<decltype(std::begin(container))>::difference_type
{
    return std::count(std::begin(container), std::end(container), val);
}

template<typename Container, typename UnaryPredicate>
inline auto count_if(const Container& container, UnaryPredicate&& pred)
    -> typename std::iterator_traits<decltype(std::begin(container))>::difference_type
{
    return std::count_if(std::begin(container), std::end(container), std::forward<UnaryPredicate>(pred));
}

template<typename Container, typename T>
inline T accumulate(const Container& container, T&& init)
{
    for (const auto& elem : container)
        init += elem;
    return std::forward<T>(init);
}

template<typename Container, typename T, typename BinaryOperation>
inline T accumulate(const Container& container, T init, BinaryOperation&& op)
{
    for (const auto& elem : container)
        init = op(std::move(init), elem);
    return init;
}

template <typename Iterator, typename UnaryOperation, typename BinaryOperation>
auto lfold(Iterator it, Iterator end, UnaryOperation&& mutation, BinaryOperation&& crossover)
    -> std::invoke_result_t<UnaryOperation, typename Iterator::value_type> // [[ expects : it < end ]]
{
    if(it == end)
        return {};
    auto init = std::invoke(mutation, *it);
    for(++it; it != end; ++it)
        init = crossover(std::move(init), std::invoke(mutation, *it));
    return init;
}

template <typename Container, typename UnaryOperation, typename BinaryOperation>
auto lfold(Container const& c, UnaryOperation&& mutation, BinaryOperation&& crossover)
{
    return lfold(begin(c), end(c), std::forward<UnaryOperation>(mutation), std::forward<BinaryOperation>(crossover));
}

template <class Iterator, class BinaryOperation> typename Iterator::value_type lfold(Iterator it, Iterator end, BinaryOperation&& op)
{
    return lfold(it, end, [](auto obj){ return obj; }, std::forward<BinaryOperation>(op));
}

template <class Container, class BinaryOperation> typename Container::value_type lfold(Container const& c, BinaryOperation&& op)
{
    return lfold(begin(c), end(c), [](auto obj){ return obj; }, std::forward<BinaryOperation>(op));
}

template<typename Container>
inline bool is_sorted(const Container& container)
{
    return std::is_sorted(std::begin(container), std::end(container));
}

template<typename Container, typename Compare>
inline bool is_sorted(const Container& container, Compare&& comp)
{
    return std::is_sorted(std::begin(container), std::end(container), std::forward<Compare>(comp));
}

/*******************************************************************************
 * contains
 ******************************************************************************/

template<typename Container, typename T>
inline bool contains(const Container& container, const T& val)
{
    return details::find(container, val) != nullptr;
}

template<typename T>
inline bool contains(const std::initializer_list<T>& container, const T& val)
{
    return contains<std::initializer_list<T> >(container, val);
}

/*******************************************************************************
 * find/find_if with optional return type (e.g. boost::optional)
 ******************************************************************************/

template<
    template<typename...> class Optional,
    typename Container,
    typename T
    >
inline auto find_opt(const Container& container, const T& what)
    -> Optional<typename std::decay<decltype(*details::find(container, what))>::type>
{
    using ValueType = typename std::decay<decltype(*details::find(container, what))>::type;
    const auto ptr = details::find(container, what);
    return ptr ? Optional<ValueType>(*ptr) : Optional<ValueType>();
}

template<
    template<typename...> class Optional,
    typename Container,
    typename UnaryPredicate
    >
inline auto find_opt_if(const Container& container, UnaryPredicate pred)
    -> Optional<decltype(details::front(container))>
{
    using ValueType = decltype(details::front(container));
    const auto it = std::find_if(std::begin(container), std::end(container), pred);
    return it != std::end(container) ? Optional<ValueType>(*it) : Optional<ValueType>();
}

/*******************************************************************************
 * append a sequence to a container
 * just a wrapper for container.insert()
 ******************************************************************************/

// this overload handles src lvalue references
// (and sometimes rvalue references too, if it's more efficient)
template<typename DstContainer, typename SrcContainer>
inline void append(DstContainer& dst, const SrcContainer& src)
{
    dst.insert(std::end(dst), std::begin(src), std::end(src));
}

template<typename DstContainer, typename SrcContainer, typename E = std::enable_if_t<std::is_rvalue_reference<SrcContainer>::value && std::is_same<DstContainer, std::decay_t<SrcContainer>>::value>>
inline auto append(DstContainer& dst, SrcContainer&& src)
{
    /*if constexpr (std::is_same_v<DstContainer, std::decay_t<SrcContainer>>)*/ {
    if (dst.empty()) {
        dst = std::move(src);
        return dst.begin();
    } }
    return dst.insert(dst.end(), std::make_move_iterator(begin(src)), std::make_move_iterator(end(src)));
}

/*******************************************************************************
 * functional-style concatenation of two identical containers
 ******************************************************************************/

namespace details {
// recursion terminator
template<typename ResultContainer, typename Container>
inline void combine_inner(ResultContainer& lhs, Container&& rhs)
{
    append(lhs, std::forward<Container>(rhs));
}
// recursive parameter pack handling
template<typename ResultContainer, typename Container, typename... Rest>
inline void combine_inner(ResultContainer& lhs, Container&& rhs, Rest&&... rest)
{
    append(lhs, std::forward<Container>(rhs));
    combine_inner(lhs, std::forward<Rest>(rest)...);
}
} // namespace details

template<typename ResultContainer, typename... Rest>
inline ResultContainer combine(ResultContainer lhs, Rest&&... rest)
{
    details::combine_inner(lhs, std::forward<Rest>(rest)...);
    return lhs;
}

/*******************************************************************************
 * functional-style sort
 ******************************************************************************/

template<typename Container>
inline Container sort(Container container)
{
    std::sort(std::begin(container), std::end(container));
    return container;
}

template<typename Container, typename Compare>
inline Container sort(Container container, Compare comp)
{
    std::sort(std::begin(container), std::end(container), comp);
    return container;
}

} // namespace algo
