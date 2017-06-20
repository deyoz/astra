
#include <fstream>
#include "date_time.h"
#include "astra_utils.h"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/gregorian/gregorian_types.hpp"
#include <boost/date_time/local_time/local_time.hpp>
using namespace boost::local_time;
using namespace boost::posix_time;

class testlog {
    const std::string path;
    const std::string bstlog;
    const std::string iculog/* = "icu.log"*/;
    const std::string ext;
        
    std::string pfx;
    std::ofstream bst_file;
    std::ofstream icu_file;
    bool is_open;
    
public:

    typedef std::map<std::string, std::string> paramSet;

    testlog(const std::string& p, const std::string& prefix = std::string()) : path(p), bstlog("boost"), iculog("icu"), ext(".log"), is_open(false) {
        //open();
    }
    
    void prefix(const std::string& pref) {
        if(pfx != pref){
            pfx = pref;
            close();
            //open();
        }
    }

    virtual ~testlog() {
        close();
    }

    void write(const paramSet& params, const std::string& query_bst, const std::string& answer_bst,
                           const std::string& query_icu, const std::string& answer_icu) {
        if(!is_open) open();
            
        writeParams(bst_file, params);
        bst_file << "Query:\n" << query_bst << std::endl << std::endl
                 << "Answer:\n" << answer_bst << std::endl << std::endl;

        writeParams(icu_file, params);
        icu_file << "Query:\n" << query_icu << std::endl << std::endl
                 << "Answer:\n" << answer_icu << std::endl << std::endl;
    }
protected:
    void writeParams(std::ostream& os, const paramSet& params) {
        os << "\n==================================================\n" << "Parameters: ";
        for(std::map<std::string, std::string>::const_iterator  i = params.begin(); i != params.end(); ++i) {
        //for(auto i = params.cbegin(); i != params.cend(); ++i) {
            os << i->first << ": " << i->second << ", ";
        }
        os << std::endl;
    }
private:
    std::string getLogPath(const std::string& target) {
        return path + '/' + (pfx.empty() ?  pfx : pfx +  '_') + target + ext;
    }
    
    void open() {
        is_open = true;

        bst_file.open(getLogPath(bstlog).c_str(), std::ios_base::app);
        icu_file.open(getLogPath(iculog).c_str(), std::ios_base::app);
    }
    
    void close() {
        if(bst_file.is_open())
            bst_file.close();
        if(icu_file.is_open())
            icu_file.close();

        is_open = false;
    }
};

using BASIC::date_time::TDateTime;
using namespace ASTRA;
TDateTime formDate(TDateTime date, TDateTime time);

struct TimeWithDelta {
    TDateTime time;
    int base_shift;

    TimeWithDelta() : time(0.), base_shift(0) {}

    TimeWithDelta(TDateTime t, int d) : time(t), base_shift(d) { }

    TDateTime applyTo(TDateTime base) {
        if(time == NoExists)
            return time;

        return formDate(base + base_shift, time);
    }

    operator TDateTime() {
        return applyTo(0.0);
    }
};

namespace old {
    tz_database &get_tz_database();
    TDateTime UTCToLocal(TDateTime d, const std::string& region);
    TDateTime LocalToUTC(TDateTime d, const std::string& region, int is_dst);
}

void Test(const std::string& region, const ptime& begin, const ptime& end, int opt = 0);
void testSPP(const std::string& Iface, const std::string& Event, TDateTime, testlog&);

void generateConversionData();
void testConversion();
void applyConversion();

void createPoints();
void createXMLFromPoints();

int getNextMoveId();
