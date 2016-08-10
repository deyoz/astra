#ifndef ASTRA_DATE_TIME_H
#define ASTRA_DATE_TIME_H

#include <string>
#include "limits.h"

#include "date_time.h"

namespace ASTRA {
    namespace date_time {

        using BASIC::date_time::TDateTime;

        TDateTime UTCToClient(TDateTime d, const std::string& region);
        TDateTime ClientToUTC(TDateTime d, const std::string& region, int is_dst = INT_MIN);

        class season {
            bool summer_;

            TDateTime beg_;
            TDateTime end_;

            static const TDateTime changeTime;
            static const int SummerPeriodFirstMonth;
            static const int SummerPeriodLastMonth;

            season(bool isSummer, TDateTime begin, TDateTime end) :
                 summer_(isSummer), beg_(begin), end_(end) {}

        public:
            season(TDateTime timePoint);
            season& operator++();
            season& operator--();

            season operator++(int) {
                season tmp = *this;
                (*this)++;
                return tmp;
            }

            season operator--(int) {
                season tmp = *this;
                (*this)--;
                return tmp;
            }

            bool isSummer() const {
                return summer_;
            }

            TDateTime begin() const { return beg_; }
            TDateTime end() const { return end_; }

        private:
            int nextSeasonYear() const;

            int prevSeasonYear() const;
            int getOppositeMonth() const {
                return summer_ ?
                         season::SummerPeriodLastMonth :
                         season::SummerPeriodFirstMonth;
            }
        };
    }
}

#endif // ASTRA_DATE_TIME_H
