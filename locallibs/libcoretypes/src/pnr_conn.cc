#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

#include "pnr_conn.h"

#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <serverlib/localtime.h>
#include <serverlib/dates.h>
#include <serverlib/exception.h>
#include <serverlib/enum.h>
#include <nsi/callbacks.h>

#define NICKNAME "D.ZERNOV"
#include <serverlib/slogger.h>

namespace
{

using namespace ct::connections;

// сегменты должны стыковаться по городу
static bool isGoodSegmentOrderByCity(const SegData& seg1, const SegData& seg2)
{
    ASSERT(seg1.arr && seg2.dep);
    return nsi::Point(*seg1.arr).cityId() == nsi::Point(*seg2.dep).cityId();
}

static bool isGoodSegmentOrderByDateTime(const SegData& seg1, const SegData& seg2, const CheckMode& mode)
{
    // для сравнения времена не переводятся в UTC, т.к. оба эти времени относятся к одному и тому же городу
    if (!seg2.depTime || !seg1.arrTime) {
        return true;
    }
    if ( seg2.depTime->time_of_day().fractional_seconds() > 0 || seg1.arrTime->time_of_day().fractional_seconds() > 0 ) {
        boost::gregorian::date_duration dateDuration = seg2.depTime->date() - seg1.arrTime->date();
        if (dateDuration.is_negative()) {
            return false;
        }
        return true;
    }
    // Проверка стыковки по времени. Города прилёта-вылета совпадают.
    const boost::posix_time::time_duration dockingTime = *seg2.depTime - *seg1.arrTime;
    if (dockingTime.is_negative()) {
        return false;
    }
    if (*seg2.dep != *seg1.arr) { // разные аэропорты
        if (dockingTime < mode.sameCityTime) {
            return false;
        }
    } else if (seg1.arrTerm && seg2.depTerm && seg2.depTerm != seg1.arrTerm) { // разные терминалы одного аэропорта
        if (dockingTime < mode.samePortTime) {
            return false;
        }
    } else { // один терминал одного аэропорта или терминалов нет
        if (dockingTime < mode.sameTermTime) {
            return false;
        }
    }
    return true;
}

boost::optional<Reason> validItinsConnection(const SegData& prevSeg, const SegData& seg, const CheckMode& mode)
{
    if (prevSeg.arnk || seg.arnk) { // fine - one of segs is ARNK
        return boost::none;
    }
    ASSERT(static_cast<bool>(seg.dep) && "not ARNK SegData without departure is invalid")

    if (!prevSeg.arr) { // fine - prev seg has no info about arrival
        return boost::none;
    }

    if (!isGoodSegmentOrderByCity(prevSeg, seg)) {
        return CR_BadCities;
    }

    if (mode.checkTime && !isGoodSegmentOrderByDateTime(prevSeg, seg, mode)) {
        return CR_BadTimes;
    }
    return boost::none;
}

}

namespace ct
{
namespace connections
{

SegData::SegData()
    : arnk(false)
{}

CheckMode::CheckMode()
    : sameCityTime(boost::posix_time::time_duration(0,0,0)),
      samePortTime(boost::posix_time::time_duration(0,0,0)),
      sameTermTime(boost::posix_time::time_duration(0,0,0)),
      checkTime(true)
{}

Error::Error(unsigned i_seg1, unsigned i_seg2, Reason reason)
    : seg1(i_seg1), seg2(i_seg2), r(reason)
{
    ASSERT(i_seg1 < i_seg2);
}

boost::optional<Error> check(const std::vector<SegData>& segs, const CheckMode& mode)
{
    for (auto seg = segs.begin(), prevSeg = segs.end(), end = segs.end(); seg != end; ++seg) {
        if (prevSeg != segs.end()) {
            boost::optional<Reason> connRes = validItinsConnection(*prevSeg, *seg, mode);
            if (connRes) {
                return Error(std::distance(segs.begin(), prevSeg),
                             std::distance(segs.begin(), seg),
                             *connRes);
            }
        }
        prevSeg = seg;
    }
    return boost::none;
}

} // connections
} // ct

#ifdef XP_TESTING
#include <serverlib/checkunit.h>

void init_connection_tests() {}

using namespace ct::connections;

static boost::optional<boost::posix_time::ptime> makeTime(const std::string& s)
{
    switch (s.size()) {
    case 0:
        return boost::none;
    case 8:
        return boost::posix_time::ptime(Dates::DateFromYYYYMMDD(s), boost::posix_time::time_duration(0, 0, 0, 1));
    case 13:
        return Dates::time_from_iso_string(s);
    }
    ASSERT(false && "invalid time string length");
    return boost::none;
}

static void makePoint(SegData& seg, bool isDep, const std::string& s)
{
    boost::optional<nsi::PointId>& p(isDep ? seg.dep : seg.arr);
    boost::optional<nsi::TermId>& t(isDep ? seg.depTerm : seg.arrTerm);
    switch (s.size()) {
    case 0: return;
    case 3: {
        p = nsi::Point(EncString::fromUtf(s)).id();
        return;
    }
    case 5: {
        p = nsi::Point(EncString::fromUtf(s.substr(0, 3))).id();
        t = nsi::TermId(s.substr(4));
        return;
    }
    }
    ASSERT(false && "invalid point string length");
}

static ct::connections::SegData toSegData(const std::string& from, const std::string& to,
                                   const std::string depTime, const std::string& arrTime)
{
    ct::connections::SegData seg;
    makePoint(seg, true, from);
    makePoint(seg, false, to);
    seg.depTime = makeTime(depTime);
    seg.arrTime = makeTime(arrTime);
    return seg;
}

static std::string makeErrText(const boost::optional<Error>& e)
{
    std::string s;
    if (!e) {
        return s;
    }
    s += (e->r == CR_BadCities ? "BadCities: " : "BadTimes: ");
    s += std::to_string(e->seg1) + "-" + std::to_string(e->seg2);
    return s;
}

#define CHECK_BAD_CONN(Err, Mode, ...) { \
    std::vector<SegData> segs = { __VA_ARGS__ }; \
    const std::string errText(makeErrText(check(segs, Mode))); \
    CHECK_EQUAL_STRINGS(errText, Err); \
}

#define CHECK_GOOD_CONN(Mode, ...) CHECK_BAD_CONN("", Mode, __VA_ARGS__)

START_TEST(pnrConnCities)
{
    CHECK_GOOD_CONN(CheckMode(),
            toSegData("SVO", "SVX", "", ""));
    CHECK_GOOD_CONN(CheckMode(),
            toSegData("SVO", "SVX", "", ""),
            toSegData("SVX", "OMS", "", ""));
    CHECK_GOOD_CONN(CheckMode(),
            toSegData("SVO", "SVX", "", ""),
            toSegData("SVX", "VOZ", "", ""),
            toSegData("VOZ", "DME", "", ""),
            toSegData("SVO", "OMS", "", ""),
            toSegData("OMS", "SVO", "", ""),
            toSegData("SVO", "SVX", "", ""));

    CHECK_BAD_CONN("BadCities: 0-1", CheckMode(),
            toSegData("SVO", "SVX", "", ""),
            toSegData("VOZ", "OMS", "", ""));
}
END_TEST

START_TEST(pnrConnCities_onePointArrival)
{
    CHECK_GOOD_CONN(CheckMode(),
            toSegData("SVO", "", "", ""));
    CHECK_GOOD_CONN(CheckMode(),
            toSegData("SVO", "", "", ""),
            toSegData("SVX", "OMS", "", ""));
    CHECK_GOOD_CONN(CheckMode(),
            toSegData("SVO", "SVX", "", ""),
            toSegData("SVX", "", "", ""),
            toSegData("SVX", "DME", "", ""));

    CHECK_BAD_CONN("BadCities: 0-1", CheckMode(),
            toSegData("SVO", "SVX", "", ""),
            toSegData("VOZ", "", "", ""));
}
END_TEST

START_TEST(pnrConnCities_CheckMode)
{
    CheckMode mode;
    mode.sameCityTime = boost::posix_time::minutes(180);
    mode.samePortTime = boost::posix_time::minutes(60);
    mode.sameTermTime = boost::posix_time::minutes(30);

    // same city
    CHECK_BAD_CONN("BadTimes: 0-1", mode,
            toSegData("LED", "SVO", "20140925T0600", "20140925T1000"),
            toSegData("DME", "VOZ", "20140925T1100", "20140925T1500"));
    CHECK_GOOD_CONN(mode,
            toSegData("LED", "SVO", "20140925T0600", "20140925T1000"),
            toSegData("DME", "VOZ", "20140925T1300", "20140925T1500"));
    // same port
    CHECK_BAD_CONN("BadTimes: 0-1", mode,
            toSegData("LED", "SVO/A", "20140925T0600", "20140925T1000"),
            toSegData("SVO/D", "VOZ", "20140925T1030", "20140925T1500"));
    CHECK_GOOD_CONN(mode,
            toSegData("LED", "SVO", "20140925T0600", "20140925T1000"),
            toSegData("SVO/D", "VOZ", "20140925T1030", "20140925T1500"));
    CHECK_GOOD_CONN(mode,
            toSegData("LED", "SVO/A", "20140925T0600", "20140925T1000"),
            toSegData("SVO", "VOZ", "20140925T1030", "20140925T1500"));
    CHECK_GOOD_CONN(mode,
            toSegData("LED", "SVO/A", "20140925T0600", "20140925T1000"),
            toSegData("SVO", "VOZ", "20140925T1100", "20140925T1500"));
    // same terminal
    CHECK_BAD_CONN("BadTimes: 0-1", mode,
            toSegData("LED", "SVO/D", "20140925T0600", "20140925T1000"),
            toSegData("SVO/D", "VOZ", "20140925T1010", "20140925T1500"));
    CHECK_BAD_CONN("BadTimes: 0-1", mode,
            toSegData("LED", "SVO", "20140925T0600", "20140925T1000"),
            toSegData("SVO", "VOZ", "20140925T1010", "20140925T1500"));
    CHECK_GOOD_CONN(mode,
            toSegData("LED", "SVO/A", "20140925T0600", "20140925T1000"),
            toSegData("SVO/A", "VOZ", "20140925T1030", "20140925T1500"));
    CHECK_GOOD_CONN(mode,
            toSegData("LED", "SVO", "20140925T0600", "20140925T1000"),
            toSegData("SVO", "VOZ", "20140925T1030", "20140925T1500"));
}
END_TEST

START_TEST(checkPnrConnDates_noDepDate_noDepDate)
{
    CHECK_GOOD_CONN(CheckMode(),
            toSegData("SVO", "SVX", "", "20140925T2000"),
            toSegData("SVX", "OMS", "", "20140925T2000"));
    CHECK_GOOD_CONN(CheckMode(),
            toSegData("SVO", "SVX", "", ""),
            toSegData("SVX", "OMS", "20140925T1600", "20140925T2000"));
    CHECK_GOOD_CONN(CheckMode(),
            toSegData("SVO", "SVX", "20140925T1600", "20140925T2000"),
            toSegData("SVX", "OMS", "", ""));
}
END_TEST

START_TEST(checkPnrConnDates_noDepDate_sameDate)
{
    //segTime не задан для обоих сегментов
    CHECK_GOOD_CONN(CheckMode(),
            toSegData("SVO", "LED", "", "20140925"),
            toSegData("LED", "VOZ", "", "20140925"));

    //segTime не задан для первого сегмента
    CHECK_GOOD_CONN(CheckMode(),
            toSegData("SVO", "LED", "", "20140925"),
            toSegData("LED", "VOZ", "20140925T1600", "20140925T2000"));

    //segTime не задан для второго сегмента
    CHECK_GOOD_CONN(CheckMode(),
            toSegData("SVO", "LED", "20140925T1600", "20140925T2000"),
            toSegData("LED", "VOZ", "20140925", "20140925"));

    //segTime задан для обоих сегментов -> connection check passes
    CHECK_GOOD_CONN(CheckMode(),
            toSegData("SVO", "LED", "20140925T1600", "20140925T1200"),
            toSegData("LED", "VOZ", "20140925T1400", "20140925T1800"));

    //segTime задан для обоих сегментов -> segments are not connected by time
    CHECK_BAD_CONN("BadTimes: 0-1", CheckMode(),
            toSegData("SVO", "LED", "20140925T0600", "20140925T1200"),
            toSegData("LED", "VOZ", "20140925T1000", "20140925T1500"));
}
END_TEST

START_TEST(checkPnrConnDates_noDepDate_differentDates)
{
    //segTime не задан для обоих сегментов -> connection check passes
    CHECK_GOOD_CONN(CheckMode(),
            toSegData("SVO", "LED", "20140925", "20140925"),
            toSegData("LED", "VOZ", "20140926", "20140926"));

    //segTime не задан для обоих сегментов -> segments are not ordered by date
    CHECK_BAD_CONN("BadTimes: 0-1", CheckMode(),
            toSegData("SVO", "LED", "20140925", "20140925"),
            toSegData("LED", "VOZ", "20140924", "20140924"));

    //segTime не задан для первого сегмента -> connection check passes
    CHECK_GOOD_CONN(CheckMode(),
            toSegData("SVO", "LED", "20140924", "20140924"),
            toSegData("LED", "VOZ", "20140925T1600", "20140925T2000"));

    //segTime не задан для первого сегмента -> segments are not ordered by date
    CHECK_BAD_CONN("BadTimes: 0-1", CheckMode(),
            toSegData("SVO", "LED", "20140925", "20140925"),
            toSegData("LED", "VOZ", "20140924T1600", "20140924T2000"));

    //segTime не задан для второго сегмента -> connection check passes
    CHECK_GOOD_CONN(CheckMode(),
            toSegData("SVO", "LED", "20140925T1600", "20140925T2000"),
            toSegData("LED", "VOZ", "20140926", "20140926"));

    //segTime не задан для второго сегмента -> segments are not ordered by date
    CHECK_BAD_CONN("BadTimes: 0-1", CheckMode(),
            toSegData("SVO", "LED", "20140926T1600", "20140926T2000"),
            toSegData("LED", "VOZ", "20140925", "20140925"));

    //segTime не задан для второго сегмента -> time connection check passes
    CHECK_GOOD_CONN(CheckMode(),
            toSegData("SVO", "LED", "20140924T2300", "20140925T0500"),
            toSegData("LED", "VOZ", "20140925", "20140925"));

    //segTime задан для обоих сегментов, перехода через дату нет -> connection check passes
    CHECK_GOOD_CONN(CheckMode(),
            toSegData("SVO", "LED", "20140925T1000", "20140925T1500"),
            toSegData("LED", "VOZ", "20140926T0600", "20140926T1200"));

    //segTime задан для обоих сегментов, перехода через дату нет -> segments are not connected by time
    CHECK_BAD_CONN("BadTimes: 0-1", CheckMode(),
            toSegData("SVO", "LED", "20140925T1000", "20140925T1500"),
            toSegData("LED", "VOZ", "20140924T2000", "20140924T2300"));

    //segTime задан для обоих сегментов, переход через дату есть -> connection check passes
    CHECK_GOOD_CONN(CheckMode(),
            toSegData("SVO", "LED", "20140925T2300", "20140926T0300"),
            toSegData("LED", "VOZ", "20140926T0400", "20140926T0800"));

    //segTime задан для обоих сегментов, переход через дату есть -> segments are not connected by time
    CHECK_BAD_CONN("BadTimes: 0-1", CheckMode(),
            toSegData("SVO", "LED", "20140925T2300", "20140926T0400"),
            toSegData("LED", "VOZ", "20140926T0200", "20140926T0600"));
}
END_TEST

#define SUITENAME "coretypes"
TCASEREGISTER(nsi::setupTestNsi, 0)
{
    ADD_TEST(pnrConnCities);
    ADD_TEST(pnrConnCities_onePointArrival);
    ADD_TEST(pnrConnCities_CheckMode);
    ADD_TEST(checkPnrConnDates_noDepDate_noDepDate);
    ADD_TEST(checkPnrConnDates_noDepDate_sameDate);
    ADD_TEST(checkPnrConnDates_noDepDate_differentDates);
}
TCASEFINISH

#endif // XP_TESTING
