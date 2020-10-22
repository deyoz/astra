#pragma once

#include <vector>
#include <set>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include "freq.h"

struct Period;
typedef std::vector<Period> Periods;

struct Period
{
    boost::gregorian::date start, end;
    bool biweekly;
    Freq freq; 

    Period();
    Period(const boost::gregorian::date& d1, const boost::gregorian::date& d2, const Freq & fr) : 
            start(d1), end(d2), biweekly(false), freq(fr) {}
    Period(const boost::gregorian::date& d1, const boost::gregorian::date& d2, const std::string& fr = "1234567");
    explicit Period(const boost::gregorian::date&);
    static boost::optional<Period> create(const boost::gregorian::date & d1, const boost::gregorian::date & d2,
                                            const Freq & freq, bool biweekly);
    bool empty() const;
    bool full() const;
    void changeStart(const boost::gregorian::date &);
    void biweekInvert();

    Period shift(boost::gregorian::days) const;

    static Period join(const Period&, const Period&, bool allowBiweek = true);
    static Period normalize(const Period &p);
    static Periods normalize(const Periods &ps);
    static Periods grain(const Periods &lhs, const Periods &rhs);
    static Periods intersect(const Periods &lhs, const Periods &rhs);
    static Periods difference( Periods const &lhs, Periods const &rhs );
    static Periods combine(const Periods&, const Periods&);
    static Period bounds(const Periods&);
    std::string periodForMask() const; 
    friend bool operator<(const Period&, const Period&);
    friend bool operator==(const Period&, const Period&);
    friend bool operator!=(const Period&, const Period&);
    friend Periods operator-(const Period& lp, const Period& rp);
    friend Period operator&(const Period&, const Period&);
    friend std::ostream& operator<<(std::ostream& str, const Period& p);
};


bool dateFromPeriod(const Period& p, const boost::gregorian::date& dt, bool checkBiweek = true);
bool isOneDayFreqForBiweeklyPeriods();

Periods joinByDates(const Periods&, bool allowBiweek);
Periods makePeriods(const std::set<boost::gregorian::date>&, bool allowBiweek);
Periods splitBiweekPeriod(const Period&);

class PeriodStl
{
    public:
        class Iterator
        {
            public:
                typedef ptrdiff_t                   difference_type;
                typedef std::forward_iterator_tag   iterator_category;
                typedef boost::gregorian::date          value_type;
                typedef boost::gregorian::date const *  pointer;
                typedef boost::gregorian::date const &  reference;

            public:
                Iterator() : p_( 0 ) {}
                Iterator( Period const *p, Period const &np )
                    : p_( p ), np_( np ), dt_( np.start )
                {}
                Iterator( Period const *p )
                    : p_( p ), np_(), dt_( boost::gregorian::date() )
                {}
                Iterator( Iterator const &it ) { *this = it; }

                Iterator &operator=( Iterator const &it );
                Iterator &operator++();
                Iterator operator++( int ); 
                reference operator*() const { return dt_; }
                pointer operator->() const { return &dt_; }
                bool operator==( Iterator const &it ) const;
                bool operator!=( Iterator const &it ) { return !( *this == it ); }

            private:
                Period const *p_;
                Period np_;
                boost::gregorian::date dt_;
        };

        typedef PeriodStl::Iterator const_iterator;
        typedef PeriodStl::Iterator iterator;

        PeriodStl( Period const &p ) : p_( p ), np_( Period::normalize( p ) ) {} 

        Iterator begin() const & { return p_.empty() ? end() : Iterator( &p_, np_ ); }
        Iterator end() const & { return Iterator( &p_ ); }

    private:
        Period const &p_;
        Period np_;
};


class period
{
    boost::gregorian::date beg, end;
    frequency fr;
  public:
    period(const boost::gregorian::date& b, const boost::gregorian::date& e, const frequency& f);
    period() {}
    period intersection(const period& p) const;
    bool intersects(const period& p) const;
    const boost::gregorian::date& beg_date() const {  return beg;  }
    const boost::gregorian::date& end_date() const {  return end;  }
    const frequency& freq() const {  return fr;  }
};

struct DayRange
{
    boost::gregorian::date start;
    boost::gregorian::date end;
    explicit DayRange(const boost::gregorian::date&);
    DayRange(const boost::gregorian::date&, const boost::gregorian::date&);
    friend bool operator==(const DayRange&, const DayRange&);
    friend bool operator<(const DayRange&, const  DayRange&);
    friend std::ostream& operator<<(std::ostream&, const DayRange&);
};
