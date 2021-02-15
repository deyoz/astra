#pragma once

#include <array>

#include "db_row.h"
#include "cursctl.h"

namespace OciCpp {

using BaseRow = dbcpp::BaseRow<OciCpp::CursCtl>;

template<typename... Args>
using Row = dbcpp::Row<OciCpp::CursCtl, Args...>;

template<typename... Args>
class Rows : public BaseRow
{
    static_assert(sizeof...(Args) > 0, "Useless OciCpp::Rows usage");

    template <typename T, size_t N>
    void define(CursCtl& cr) {
        dbcpp::details::DefHelper<T>::def(cr, std::get<N>(rows.front()), &indicators[N * rows.size()]);
    }

    template <std::size_t... Is>
    void define_with_index(CursCtl& cr, std::index_sequence<Is...>) {
        int d[] = { (define<Args, Is>(cr), 0)... };
        (void)d;
    }

    typedef std::tuple<Args...> row_type;
    typedef std::tuple<typename dbcpp::details::RowFieldTraits<Args>::placeholder_t...> internal_row_type;

    std::vector<internal_row_type> rows;
    std::vector<short> indicators;
    size_t size;
public:
    Rows(size_t n = 100)
        : rows(n), indicators(sizeof...(Args) * n, 0), size(0)
    {}

    virtual void def(CursCtl& cr) {
        cr.fetchLen(rows.size()).structSize(sizeof(internal_row_type));
        define_with_index(cr, std::index_sequence_for<Args...>());
    }

    int fetch(CursCtl& cr)
    {
        size = 0;
        int result = cr.err();
        if (!result) {
            cr.fen(rows.size());
            size = cr.rowcount_now();
        }
        return result;
    }

    class SingleRow
    {
        const internal_row_type* row;
        std::array<short, sizeof...(Args)> indicators;
    public:
        SingleRow(const internal_row_type* r, const std::array<short, sizeof...(Args)>& i)
            : row(r), indicators(i)
        {}

        template<int N>
        typename std::tuple_element<N, row_type>::type get() const {
            return dbcpp::details::GetHelper<typename std::tuple_element<N, row_type>::type>::get(std::get<N>(*row), indicators[N]);
        }
    };

    class Iterator
    {
        const std::vector<internal_row_type>& rows;
        const std::vector<short>& indicators;
        const size_t size;

        size_t pos;
        boost::optional<SingleRow> row;

        void set_row_content() {
            if (pos < size) {
                std::array<short, sizeof...(Args)> is;
                for (size_t i = 0; i < is.size(); ++i) {
                    is[i] = indicators[i * rows.size() + pos];
                }
                row = SingleRow(&rows[pos], is);
            } else {
                row.reset();
            }
        }

    public:
        typedef std::forward_iterator_tag   iterator_category;
        typedef SingleRow                   value_type;
        typedef std::ptrdiff_t              difference_type;
        typedef const SingleRow*            pointer;
        typedef const SingleRow&            reference;

        Iterator(const std::vector<internal_row_type>& r, const std::vector<short>& i, size_t n, size_t p)
            : rows(r), indicators(i), size(n), pos(p)
        {
            set_row_content();
        }

        Iterator& operator= (const Iterator& it) {
            rows = it.rows;
            indicators = it.indicators;
            size = it.size;
            pos = it.pos;
            set_row_content();
            return *this;
        }
        Iterator& operator++() {
            if (pos < size) {
                ++pos;
                set_row_content();
            }
            return *this;
        }
        Iterator operator++ (int) {
            Iterator out(*this);
            ++(*this);
            return out;
        }
        reference operator*() const {
            return row.get();
        }
        pointer operator->() const {
            return row.get_ptr();
        }
        bool operator== (const Iterator& it) const {
            assert(&rows == &it.rows);
            return pos == it.pos;
        }
        bool operator!= (const Iterator& it) {
            return !(*this == it);
        }
    };

    typedef Iterator const_iterator;
    typedef Iterator iterator;

    Iterator begin() const {
        return Iterator(rows, indicators, size, 0);
    }
    Iterator end() const {
        return Iterator(rows, indicators, size, size);
    }
};

} // OciCpp
