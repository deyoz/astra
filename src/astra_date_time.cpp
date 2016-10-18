#include "astra_date_time.h"
#include "astra_utils.h"
#include "date_time.h"

namespace ASTRA {
    namespace date_time {

        using namespace BASIC::date_time;

        TDateTime UTCToClient(TDateTime d, const std::string& region)
        {
          TReqInfo *reqInfo = TReqInfo::Instance();
          switch (reqInfo->user.sets.time)
          {
          case ustTimeUTC:
              return d;
          case ustTimeLocalDesk:
              return UTCToLocal(d,reqInfo->desk.tz_region);
          case ustTimeLocalAirp:
              return UTCToLocal(d,region);
          default:
              throw EXCEPTIONS::Exception("Unknown sets.time for user %s (user_id=%d)",reqInfo->user.login.c_str(),reqInfo->user.user_id);
          }
        }

        TDateTime ClientToUTC(TDateTime d, const std::string& region, int is_dst)
        {
            TReqInfo *reqInfo = TReqInfo::Instance();
            switch (reqInfo->user.sets.time)
            {
            case ustTimeUTC:
                return d;
            case ustTimeLocalDesk:
                return LocalToUTC(d,reqInfo->desk.tz_region,is_dst);
            case ustTimeLocalAirp:
                return LocalToUTC(d,region,is_dst);
            default:
                throw EXCEPTIONS::Exception("Unknown sets.time for user %s (user_id=%d)",reqInfo->user.login.c_str(),reqInfo->user.user_id);
            }
        }

        // Реализация класса season

        season::season(TDateTime timePoint) {
            int y = Year(timePoint);

            TDateTime spring = LastSunday(y, SummerPeriodFirstMonth) + changeTime;
            if(timePoint >= spring) {
                TDateTime autumn = LastSunday(y, SummerPeriodLastMonth) + changeTime;
                if(timePoint < autumn) {
                    beg_ = spring;
                    end_ = autumn;
                    summer_ = true;
                }
                else {
                    beg_ = autumn;
                    end_ = LastSunday(++y, SummerPeriodFirstMonth) + changeTime;
                    summer_ = false;
                }
            }
            else {
                beg_ = LastSunday(--y, SummerPeriodLastMonth) + changeTime;
                end_ = spring;
                summer_ = false;
            }
        }

        season& season::operator++() {
            beg_ = LastSunday(nextSeasonYear(), getOppositeMonth(dirFORWARD)) + changeTime;
            std::swap(beg_, end_);

            summer_ = !summer_;
            return *this;
        }

        season& season::operator--() {
            end_ = LastSunday(prevSeasonYear(), getOppositeMonth(dirBACKWARD)) + changeTime;
            std::swap(beg_, end_);
            summer_ = !summer_;
            return *this;
        }

        int season::nextSeasonYear() const {
            int year = Year(end_);
            return summer_ ? ++year : year;
        }

        int season::prevSeasonYear() const {
            int year = Year(beg_);
            return summer_ ? --year : year;
        }

        const TDateTime season::changeTime = 2./24;
        const int season::SummerPeriodFirstMonth = 3;
        const int season::SummerPeriodLastMonth  = 10;
    }
}
