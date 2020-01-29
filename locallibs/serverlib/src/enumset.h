#pragma once

// Type-safe replacement for bitmask flags
// Typical usage:
//
// enum class E { A, B, C };
// EnumSet<E> s { E::A, E::B };
// s.size() // 2
// s.count(E::A) // 1
// s.count(E::C) // 0
// s.insert(E::C)
// s.erase(E::A)
//
// For many more examples see unit tests in enumset.cc

#include <bitset>
#include <cassert>
#include <iterator>
#include <string>
#include <initializer_list>

// Maximum enum value must be < CAPACITY
template<class EnumT, size_t CAPACITY = 64>
class EnumSet
{
    typedef std::bitset<CAPACITY> BitSet_t;
public:
    EnumSet() {}
    EnumSet(std::initializer_list<EnumT> lst) { for (EnumT item : lst) insert(item); };
    template<class IteratorT>
    EnumSet(const IteratorT& first, const IteratorT& last)
        { for (IteratorT it = first; it != last; ++it) insert(*it); }

    // warning: valid only for limited capacity!
    unsigned long long serialize_ull() const { return bits_.to_ullong(); }
    static EnumSet deserialize_ull(unsigned long long bits) { return EnumSet(BitSet_t(bits)); }

    std::string serialize_str() const { return bits_.to_string(); }
    static EnumSet deserialize_str(const std::string& str) { return EnumSet(BitSet_t(str)); }

    EnumSet& insert(EnumT item) { bits_.set(bitpos(item)); return *this; }
    EnumSet& insert(const EnumSet& s) { bits_ |= s.bits_; return *this; }
    template<class IteratorT>
    EnumSet& insert(const IteratorT& first, const IteratorT& last)
        { return insert(EnumSet(first, last)); }

    EnumSet& erase(EnumT item) { bits_.reset(bitpos(item)); return *this; }
    EnumSet& erase(const EnumSet& s) { bits_ &= ~s.bits_; return *this; }
    template<class IteratorT>
    EnumSet& erase(const IteratorT& first, const IteratorT& last)
        { return erase(EnumSet(first, last)); }

    EnumSet& clear() { bits_.reset(); return *this; }

    bool empty() const { return bits_.none(); }
    size_t size() const { return bits_.count(); }
    bool contains(EnumT item) const { return bits_.test(bitpos(item)); }
    size_t count(const EnumSet& s) const { return set_intersection(*this, s).size(); }

    bool is_equal_to(const EnumSet& s) const { return bits_ == s.bits_; }
    bool is_subset_of(const EnumSet& s) const { return s.is_superset_of(*this); }
    bool is_superset_of(const EnumSet& s) const { return count(s) == s.size(); }

    static EnumSet set_union(const EnumSet& a, const EnumSet& b)
        { return EnumSet(a.bits_ | b.bits_); }
    static EnumSet set_intersection(const EnumSet& a, const EnumSet& b)
        { return EnumSet(a.bits_ & b.bits_); }
    static EnumSet set_difference(const EnumSet& a, const EnumSet& b)
        { return EnumSet(a.bits_ & ~b.bits_); }

    bool operator<(const EnumSet& rhs) const
    {
        return CAPACITY <= sizeof(unsigned long long) * 8 ?
            (bits_.to_ullong() < rhs.bits_.to_ullong()) :
            (bits_.to_string() < rhs.bits_.to_string());
    }
    bool operator==(const EnumSet& rhs) const { return is_equal_to(rhs); }
    bool operator!=(const EnumSet& rhs) const { return !is_equal_to(rhs); }

    class const_iterator: public std::iterator<std::forward_iterator_tag, const EnumT>
    {
    public:
        const_iterator(): bits_(nullptr), pos_(0) {}

        const_iterator operator++() { return increment(); }
        const_iterator operator++(int) { return increment(); }
        EnumT operator*() const { return dereference(); }

        bool operator==(const const_iterator& rhs) const
        {
            return bits_ == rhs.bits_ && pos_ == rhs.pos_;
        }
        bool operator!=(const const_iterator& rhs) const { return !(*this == rhs); }
    private:
        friend EnumSet;

        const_iterator& increment()
        {
            assert(bits_);
            assert(pos_ < CAPACITY);
            do {
                ++pos_;
            } while (pos_ < CAPACITY && !bits_->test(pos_));
            return *this;
        }

        EnumT dereference() const
        {
            assert(bits_);
            assert(pos_ < CAPACITY);
            assert(bits_->test(pos_));
            return static_cast<EnumT>(pos_);
        }

        const_iterator(const EnumSet::BitSet_t* bits, size_t pos): bits_(bits), pos_(pos) { }
        const EnumSet::BitSet_t* bits_;
        size_t pos_;
    };

    const_iterator begin() const
    {
        size_t pos = 0;
        while (pos < CAPACITY && !bits_.test(pos))
            ++pos;
        return const_iterator(&bits_, pos);
    }

    const_iterator end() const { return const_iterator(&bits_, CAPACITY); }
private:
    friend const_iterator;

    EnumSet(const BitSet_t& bits): bits_(bits) {}

    static size_t bitpos(EnumT item)
    {
        const size_t pos = static_cast<size_t>(item);
        assert(pos < CAPACITY);
        return pos;
    }

    BitSet_t bits_;
};
