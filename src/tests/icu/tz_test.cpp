#include "tz_test.h"
#include "date_time.h"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/gregorian/gregorian_types.hpp"
using namespace boost::posix_time;

#include "astra_utils.h"

#include <signal.h>
#include <unistd.h>
#include <tr1/array>

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace BASIC::date_time;

TDateTime EncodeDateTime(int y, int m, int d, int hh, int mm, int ss) {
    TDateTime date, time;
    EncodeDate(y, m, d, date);
    EncodeTime(hh, mm, ss, time);
    return date + time;
}

struct pass_data_conv_time {
    enum _exType {INVALID, AMBIGUOUS};
    union{
        TDateTime result_;
        _exType exception_;
    };
    int bResult;
    
    pass_data_conv_time(): bResult(-1) {}

    pass_data_conv_time& operator=(TDateTime dt) {
        result_ = dt;
        bResult = 1;
        return *this;
    }

    void invalid(){ bResult = 0; exception_ = INVALID; }
    void ambiguous(){ bResult = 0; exception_ = AMBIGUOUS; }

    bool operator== (const pass_data_conv_time& oth) const {
        if (bResult != oth.bResult)
            return false;

        if (bResult)
            return result_ == oth.result_;

        return exception_ == oth.exception_;
    }

    bool operator !=(const pass_data_conv_time& oth) const {
        return !(*this == oth);
    }
};

std::ostream& operator<<(std::ostream& os, const pass_data_conv_time& p) {
    if(p.bResult == -1) {
        os << "Invalid state";
    }
    else
        if (p.bResult == 0)
            os << (p.exception_ == pass_data_conv_time::INVALID ? "skipped time" : "ambiguous time");
        else
            os << DateTimeToStr(p.result_);
    return os;
}

struct BSTOper {
    inline static TDateTime call(TDateTime dt, const std::string& region, int isDst) {
        return old::LocalToUTC(dt, region, isDst);
    }
};

struct ICUOper {
    inline static TDateTime call(TDateTime dt, const std::string& region, int isDst) {
        return BASIC::date_time::LocalToUTC(dt, region, isDst);
    }
};

template<class Oper, class Ret = pass_data_conv_time>
struct test {

    //template<class... Args>
    //static Ret run(Args&&...args) {
    static Ret run(TDateTime dt, const std::string& region, int isDst) {

        Ret p;
        try {
            p = Oper::call(dt, region, isDst /*std::forward<Args>(args)...*/);
        }
        catch(boost::local_time::time_label_invalid&) {
            p.invalid();
        }
        catch(boost::local_time::ambiguous_result) {
            p.ambiguous();
        }

        return p;
    }
};

struct dst_conv_data {
    pass_data_conv_time bst;
    pass_data_conv_time icu;

    dst_conv_data(){}

    dst_conv_data(const pass_data_conv_time& b, const pass_data_conv_time& i) : bst(b), icu(i) {
        result = (bst == icu);
    }

    dst_conv_data(const dst_conv_data& oth) {
        bst = oth.bst;
        icu = oth.icu;
        result = oth.result;
    }

    dst_conv_data operator= (const dst_conv_data& oth) {
        if (&oth == this)
            return *this;

        bst = oth.bst;
        icu = oth.icu;
        result = oth.result;
        
        return *this;
    }

    bool getTestResult() const { return result; }

private:
    bool result;
};

typedef char DstResultMask;
struct datetime_conv_data {
    dst_conv_data dst[3];
    TDateTime dt_;

    datetime_conv_data(TDateTime dt): dt_(dt) { }

    void setDstData(int isDst, const dst_conv_data& dst_data) {
        size_t index;
        switch (isDst) {
        case ASTRA::NoExists:
            index = 0;
            break;
        case 0:
            index = 1;
            break;
        case 1:
            index = 2;
            break;
        default:
            throw std::out_of_range("isDst");
        }
        dst[index] = dst_data;
    }

    TDateTime getDateTime() const { return dt_; }

    bool getResult() const {
        // все три теста успешны (7 = 0000 0111)
        return 7 == getMask();
    }

    DstResultMask getMask() const {
        DstResultMask res = 0;
        for(size_t i = 0; i < 3; ++i)
            res |= static_cast<char>( dst[i].getTestResult() ) << i;

        return res;
    }
};

std::ostream& operator<<(std::ostream& os, const dst_conv_data& p) {
    std::cout << "\tboost: " << p.bst << " icu: " << p.icu << " " << (p.getTestResult() ? "OK" : "FAILED");
    return os;
}
    
std::ostream& operator<<(std::ostream& os, const datetime_conv_data& p) {
    std::cout << DateTimeToStr(p.getDateTime()) << (p.getResult() ? " OK": " FAILED") <<std::endl
              << "isdst: NoExists " << p.dst[0] << std::endl
              << "isdst: 0 " << p.dst[1] << std::endl
              << "isdst: 1 " << p.dst[2] << std::endl;
    return os;
}

class ConversionTest {
public:
    typedef datetime_conv_data test_result;

    static test_result run(const std::string& region, TDateTime dt) {
        test_result result(dt);
        for(size_t i = 0; i < 3; ++i) {
            result.setDstData(dst_[i], run_test(region, dt, dst_[i]) );
        }
        return  result;
    }

private:
    static const int dst_[3];

    static dst_conv_data run_test(const std::string& region, TDateTime dt, int isDst)
    {
        pass_data_conv_time bst = test<BSTOper>::run(dt, region, isDst);
        pass_data_conv_time icu = test<ICUOper>::run(dt, region, isDst);

        return dst_conv_data(bst, icu);
    }
};

const int ConversionTest::dst_[3] = {ASTRA::NoExists, 0, 1};

class RegionTest {
public:
    typedef ConversionTest::test_result datetime_result;
    typedef std::vector<datetime_result> test_result;

    RegionTest(const ptime& from, const ptime& to) : start_(from), end_(to) {}

    test_result run(const std::string& region) {
        ptime begin = start_;

        while(begin < end_) {
            result_.push_back( ConversionTest::run(region, BoostToDateTime(begin)) );
            add30min(begin);
        }

        return result_;
    }

protected:
    void add30min(ptime& t) {
        t = t + minutes(30);
    }

private:
    test_result result_;
    const ptime start_;
    const ptime end_;
};

struct OutputPolicy {
    OutputPolicy(std::ostream& out = std::cout) : os(out) {}
    virtual void operator()(const std::string&, RegionTest::test_result&) = 0;
    virtual ~OutputPolicy(){}
protected:
    std::ostream& os;
};

struct PolicyRegion : OutputPolicy {
    PolicyRegion(std::ostream& os): OutputPolicy(os) {}
    void operator()(const std::string& region, RegionTest::test_result& res) {
        bool region_res = true;
        //for(RegionTest::datetime_result r : res) {
        for(RegionTest::test_result::const_iterator r = res.begin(); r != res.end(); ++r) {
            region_res = region_res && r->getResult();
        }
        os << region << (region_res ? " OK" : " FAILED") << std::endl;
    }
};

struct PolicyDateTime : OutputPolicy {
    PolicyDateTime(std::ostream& os): OutputPolicy(os) {}
    typedef RegionTest::test_result::const_iterator iterator;
    typedef std::vector<std::pair<iterator, iterator> > ranges;

    void operator()(const std::string& region, RegionTest::test_result& res) {
        ranges good, bad, *first, *second;
        splitByRanges(good, bad, res);

        if(!good.size() && bad.size()){
            first = &bad;
            second = &good;
        }
        else if (!bad.size() && good.size()){
            first = &good;
            second = &bad;
        }
         else if(good.front().first < bad.front().first) {
            first = &good;
            second = &bad;
        }
        else {
            first = &bad;
            second = &good;
        }

        ranges::const_iterator fi = first->begin();
        ranges::const_iterator si = second->begin();

        while(fi != first->end() || si != second->end()) {
            os << region << " " << DateTimeToStr(fi->first->getDateTime()) << " - " << DateTimeToStr(fi->second->getDateTime())
                      << " " << (first == &good ? "OK" : "FAILED") << std::endl;

            swap(first, second);
            swap(++fi, si);
        }
    }

    void splitByRanges(ranges& good, ranges& bad, const RegionTest::test_result& res) {
        ranges *currentRange;
        iterator i = res.begin(), first, last;

        bool prev = i->getResult();
        currentRange = prev ? &good : &bad;
        first = i;

        for(++i; i != res.end(); ++i) {
            bool cur = i->getResult();
            
            if(cur == prev) {
                last = i;
            }
            else {
                currentRange->push_back(std::make_pair(first, last));
                first = i;
            }

            prev = cur;
            currentRange = prev ? &good : &bad;
        }

        currentRange->push_back(std::make_pair(first, last));
    }
};

struct PolicyFull : OutputPolicy {
    PolicyFull(std::ostream& os): OutputPolicy(os) {

    }
    void operator()(const std::string& region, RegionTest::test_result& res) {
        for(RegionTest::test_result::const_iterator i = res.begin(); i != res.end(); ++i) {
            os << region << " " << *i << std::endl;
        }
    }
};

OutputPolicy* policyFactory(int pol, std::ostream& os) {
    switch(pol) {
    case 0:
        return new PolicyRegion(os);
    case 1:
        return new PolicyDateTime(os);
    case 2:
        return new PolicyFull(os);
    default:
        return 0;
    }
}

void printTest(const std::string& region, RegionTest::test_result res, int opt, std::ostream& os = std::cout)
{
    std::auto_ptr<OutputPolicy> pol( policyFactory(opt, os) );
    (*pol)(region, res);
    os << std::endl;
}

void Test(const std::string& region, const ptime& begin, const ptime& end, int opt) {
    RegionTest test(begin, end);
    RegionTest::test_result res = test.run(region);
    printTest(region, res, opt);
}

bool gAirp = true;

void initReqInfo() {
    TReqInfo *ri = TReqInfo::Instance();
    ri->user.access.set_total_permit();

    if(gAirp) {
        TAccessElems<std::string> airps;
        airps.set_elems_permit(true);

        TQuery Qry(&OraSession);
        Qry.Clear();
        Qry.SQLText =
            "SELECT b.* FROM ( "
            "   select max(num) AS num, tz_region "
            "   FROM ( "
            "      select count(*) AS num, routes.airp, cities.tz_region "
            "      from routes, airps, cities "
            "      where routes.airp=airps.code AND airps.city=cities.code "
            "      group by routes.airp, cities.tz_region) "
            "   group by tz_region ) a,"

            "   (select count(*) AS num, routes.airp, cities.tz_region "
            "   from routes, airps, cities "
            "   where routes.airp=airps.code AND airps.city=cities.code "
            "   group by routes.airp, cities.tz_region ) b "
            "where a.tz_region=b.tz_region AND a.num=b.num "
            "ORDER by b.tz_region";

        Qry.Execute();
        for(; !Qry.Eof; Qry.Next()) {
            airps.add_elem(Qry.FieldAsString("airp"));
        }
        ri->user.access.merge_airps(airps);
    }
    
    ri->user.user_type = gAirp ? utAirport : utSupport;
    ri->client_type = ASTRA::ctTerm;
    ri->desk.version = "201701_0000000";
}

void setTimeType(TUserSettingType ust) {
    TReqInfo *reqInfo = TReqInfo::Instance();

    reqInfo->user.sets.time = ust;
}

void setRegion(const std::string& city, const std::string& region) {
    TReqInfo *reqInfo = TReqInfo::Instance();

    reqInfo->desk.city = city;
    reqInfo->desk.tz_region = region;
}

struct IfaceTest {
    virtual void setRequest(xmlNodePtr& xml) = 0;
    virtual void setResult(const XMLDoc& query, const XMLDoc& answer) = 0;
    virtual ~IfaceTest(){}
};

struct seasonTestStg : IfaceTest {
    TDateTime date;
    std::string query;
    std::string answer;

    seasonTestStg(TDateTime dt) : date(dt) {}
    
    virtual void setRequest(xmlNodePtr& xml) {
        if(gAirp)
            NewTextChild(xml, "date", DateTimeToStr(date));
        else
            NewTextChild(xml, "flight_date", DateTimeToStr(date));
    }

    virtual void setResult(const XMLDoc& query, const XMLDoc& answer) {
        this->query = XMLTreeToText(query.docPtr());
        this->answer = XMLTreeToText(answer.docPtr());
    }

    bool operator == (const seasonTestStg& oth) const {
        return query == oth.query && answer == oth.answer;
    }

    bool operator != (const seasonTestStg& oth) const {
        return !(*this == oth);
    }
};

void testIfaceEvent(const std::string& Iface, const std::string& Event, IfaceTest& test
                    /*TDateTime dt, std::string& query, std::string& answer */) {
    XMLDoc doc1("query");
    XMLDoc doc2("answer");
    xmlNodePtr reqNode=NodeAsNode("/query", doc1.docPtr());
    xmlNodePtr resNode=NodeAsNode("/answer", doc2.docPtr());

    test.setRequest(reqNode);
//    if(gAirp)
//        NewTextChild(reqNode, "date", DateTimeToStr(dt));
//    else
//        NewTextChild(reqNode, "flight_date", DateTimeToStr(dt));

    try {
        JxtInterfaceMng::Instance()->
            GetInterface(Iface)->
            OnEvent(Event,  getXmlCtxt(),
                    reqNode,
                    resNode);
    }
    catch(std::exception& e){
        std::cout << "EXCEPTION: " << e.what() << std::endl;
    }
    
    test.setResult(doc1, doc2);

    //query = XMLTreeToText(doc1.docPtr());
    //answer = XMLTreeToText(doc2.docPtr());
}

std::string getTimeName(TUserSettingType t) {
    std::string res;
    switch(t) {
    case ustTimeUTC:
        res = "ustTimeUTC";
        break;
    case ustTimeLocalDesk:
        res = "ustTimeLocalDesk";
        break;
    case ustTimeLocalAirp:
        res = "ustTimeLocalAirp";
        break;
    default:
        res = "Unknown time type";
    }
    return res;

}

void TestRegionConvForSeasons(const std::string& region, const ptime& begin, const ptime& end, std::ostream& os) {
    RegionTest test(begin, end);
    RegionTest::test_result res = test.run(region);
    printTest(region, res, 1, os);
}

void testSPP(const std::string& iface, const std::string& event, TDateTime dt, testlog& log) {
    try{
        initReqInfo();

        TQuery Qry(&OraSession);
        Qry.Clear();
        Qry.SQLText =
            "select min(code) as city, cities.tz_region "
            "from cities, desk_grp " //, seasons "
            "where cities.code = desk_grp.city "// and cities.tz_region = 'Asia/Almaty' " //AND country='??' AND seasons.region=cities.tz_region and cities.tz_region = 'Asia/Sakhalin' "
            "group by cities.tz_region";

        Qry.Execute();
        typedef std::tr1::array<TUserSettingType, 1> time_type_t;
        time_type_t timeType;
        //timeType[0] = ustTimeUTC;
        //timeType[0] = ustTimeLocalDesk;
        timeType[0] = ustTimeLocalAirp;

        unsigned iter = 1;
        //bool diff = false;

        for(; !Qry.Eof; Qry.Next(), ++iter) {
            testlog::paramSet params;
            std::string city = Qry.FieldAsString("city"),
                    region = Qry.FieldAsString("tz_region"),
                    cityField = "city",
                    regionField = "tz_region";

            params[cityField] = city;
            params[regionField] = region;

            setRegion(city, region);

            //std::cout << iter << " Processing region: " << region << ", city: " << city << std::endl;
            std::cout << iter << " Processing date: " << DateTimeToStr(dt) << std::endl;

            seasonTestStg bstTest(dt), icuTest(dt);

            for(time_type_t::const_iterator i = timeType.begin(); i != timeType.end(); ++i) {
		//std::tr1::array<TUserSettingType,3>::const_iterator i = timeType.begin();

                setTimeType(*i);
                params["timeType"] = getTimeName(*i);
                params["date"] = DateTimeToStr(dt);

                //std::string query_bst, answer_bst;
                //std::string query_icu, answer_icu;

                //std::cout << params["timeType"] << std::endl << "BOOST" << std::endl;
/*                ProgTrace( TRACE5, "  ==== BOOST %s ====", getTimeName(*i).c_str());
                Realization::set(Rlz::BOOST);
                testIfaceEvent(iface, event, bstTest);

                //std::cout << "ICU" << std::endl;
                ProgTrace( TRACE5, "  ====  ICU  %s ====", getTimeName(*i).c_str());
                Realization::set(Rlz::ICU);
                testIfaceEvent(iface, event, icuTest);
*/
                 /*if(query_bst != query_icu || answer_bst != answer_icu) {
                    log.write(params, query_bst, answer_bst, query_icu, answer_icu);
                    diff = true;
                }*/

                if(bstTest == icuTest)
                    log.write(params, bstTest.query, bstTest.answer,
                                      icuTest.query, icuTest.answer);
            }
        }

    }
    catch(std::exception& e) {
        std::cout<<"EXCEPTION: " << e.what() << std::endl;
    }
    OraSession.Rollback();
}


inline bool equalDates(double d1, double d2) {
    // std::cout << "    boost: " << DateTimeToStr(d1).c_str() << ", icu: " << DateTimeToStr(d2).c_str() << ", diff: " << std::fabs(d1 - d2) << std::endl;
    
    return std::fabs(d1 - d2) < 0.000011574; // 1 sec. 1/(24*60*60)
}

bool testDateTimeEquality(TDateTime udt_orig, TDateTime offset, const std::string& region) {
//    std::cout << __FUNCTION__ << ", Orig: " << DateTimeToStr(udt_orig).c_str() << ", offset: " << DateTimeToStr(offset).c_str() << ", region: " << region << std::endl;

    TDateTime ldt_boost = old::UTCToLocal(udt_orig, region);
    TDateTime ldt_icu = BASIC::date_time::UTCToLocal(udt_orig + offset, region);
    
    return equalDates(ldt_boost, ldt_icu);
}

inline bool testTimeEquality(TDateTime udt_orig, int hours_offset, const std::string& region) {
    TDateTime dtOffset;
    EncodeTime(std::abs(hours_offset), 0, 0, dtOffset);
    
    return testDateTimeEquality(udt_orig, hours_offset < 0 ? -dtOffset: dtOffset, region);
}

inline bool testDateEquality(TDateTime udt_orig, int days_offset, const std::string& region) {
    return testDateTimeEquality(udt_orig, days_offset, region);
}

void testConversion() {
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText = "select sd.trip_id, sd.num, sd.move_id, sd.first_day, sd.last_day, sd.days, "
                  "rt.num as item_num, rt.scd_in, rt.delta_in, rt.scd_out, rt.delta_out, "
                  "cv.hours_in, cv.hours_out, cv.days_offset, c.tz_region as region "
                  "from sched_days sd "
                  "join routes rt on sd.move_id = rt.move_id "
                  "join airps a on rt.airp = a.code "
                  "join cities c on c.code = a.city "
                  "left outer join boost_icu_conv cv on cv.trip_id = sd.trip_id and cv.trip_num = sd.num and cv.move_id = rt.move_id and cv.move_num = rt.num "
//                  "where (EXTRACT(YEAR FROM first_day) > 2015 or EXTRACT(YEAR FROM last_day) > 2015) "//and sd.trip_id = 60827 and sd.num = 12 and rt.num = 0 "
                  "order by sd.trip_id, sd.num, sd.move_id";
    
    int count = 0, err_count = 0;

    for(Qry.Execute(); !Qry.Eof; Qry.Next(), ++count) {
        std::string region(Qry.FieldAsString("region"));

        TDateTime first_day = Qry.FieldAsDateTime("first_day");
        TDateTime last_day = Qry.FieldAsDateTime("last_day");
    
        int w = 0; 
        try{ 
            if(!Qry.FieldIsNULL("scd_in")) {
                TimeWithDelta in(Qry.FieldAsDateTime("scd_in"), Qry.FieldAsInteger("delta_in"));
                int hbegin = Qry.FieldIsNULL("hours_in") ? 0 : Qry.FieldAsInteger("hours_in");

                if(!testTimeEquality(in.applyTo(first_day), hbegin, region)) {
                    std::cout << "trip: " << Qry.FieldAsInteger("trip_id")
                              << " trip_num: " << Qry.FieldAsInteger("num")
                              << " move: " << Qry.FieldAsInteger("move_id")
                              << " move_num: " << Qry.FieldAsInteger("item_num")
                              << " first_day: " << DateTimeToStr(first_day).c_str()
                              << " scd_in: " << DateTimeToStr(Qry.FieldAsDateTime("scd_in")).c_str()
                              << " delta_in: " <<  Qry.FieldAsInteger("delta_in") 
                              << " hin: " << hbegin << std::endl;
                    ++err_count;
                    continue;
                }
            }

            w = 1;
            if(!Qry.FieldIsNULL("scd_out")) {
                TimeWithDelta out(Qry.FieldAsDateTime("scd_out"), Qry.FieldAsInteger("delta_out"));
                int hend = Qry.FieldIsNULL("hours_out") ? 0 : Qry.FieldAsInteger("hours_out");

                if(!testTimeEquality(out.applyTo(first_day), hend, region)) {
                    std::cout << "trip: " << Qry.FieldAsInteger("trip_id")
                              << " trip_num: " << Qry.FieldAsInteger("num")
                              << " move: " << Qry.FieldAsInteger("move_id")
                              << " move_num: " << Qry.FieldAsInteger("item_num")
                              << " first_day: " << DateTimeToStr(first_day).c_str()
                              << " scd_out: " << DateTimeToStr(Qry.FieldAsDateTime("scd_out")).c_str()
                              << " delta_out: " <<  Qry.FieldAsInteger("delta_out") 
                              << " hout: " << hend << std::endl;
                    ++err_count;
                    continue;
                }
     
            }
        }
        catch(std::exception& e) {
            std::cout << "Exception: " << e.what() << " in " << __FUNCTION__ << " trip: " <<  Qry.FieldAsInteger("trip_id") << " move num: " << Qry.FieldAsInteger("num") << " item: " << Qry.FieldAsInteger("item_num") << (w ? " scd_out: " : " scd_in: ") << (w?Qry.FieldAsDateTime("scd_out") : Qry.FieldAsDateTime("scd_in")) << std::endl;
        }

    }
    std::cout << "Total rows: " << count << ", Missed rows: " << err_count << std::endl;
}

#include "timer.h"

int getNextMoveId() {
    TQuery query(&OraSession);
    query.SQLText = "BEGIN "\
                    " select move_id.nextval into :id from dual;"\
                    "END;";
    query.DeclareVariable( "id", otInteger );
    query.Execute();
    return query.GetVariableAsInteger( "id" );
}

void createPoints() {
    TDateTime utcStart, utcEnd;
    EncodeDate(2016, 10, 24, utcStart);
    EncodeDate(2016, 11, 7, utcEnd);

    int id_start = getNextMoveId();
    for(;utcStart < utcEnd; utcStart += 1.)
        createSPP(utcStart);

    std::cout << "move_id BETWEEN " << id_start << " AND " << getNextMoveId() << std::endl;
}

struct sppTestStg : seasonTestStg {
    sppTestStg(TDateTime dt) : seasonTestStg(dt) {}

    virtual void setRequest(xmlNodePtr& xml) {
        NewTextChild(xml, "flight_date", DateTimeToStr(date));
    }
};

void createXMLFromPoints() {
    TDateTime utcStart, utcEnd;
    EncodeDate(2016, 10, 24, utcStart); // 21
    EncodeDate(2016, 11, 7, utcEnd); // 31

    initReqInfo();
    setTimeType(ustTimeLocalAirp);

    std::ofstream out;
    out.open("points.xml", std::ios_base::out|std::ios_base::ate);

    for(;utcStart < utcEnd; utcStart += 1.){
        sppTestStg sppTest(utcStart);
        testIfaceEvent("sopp", "ReadTrips", sppTest);

        out << DateTimeToStr(utcStart) << std::endl << sppTest.answer << std::endl << std::endl;
    }
}
