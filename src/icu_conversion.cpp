
#include <string>
#include "astra_utils.h"

#include <cstdlib>
#include "date_time.h"
#include "astra_consts.h"

#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/gregorian/gregorian_types.hpp"
#include <boost/date_time/local_time/local_time.hpp>
using namespace boost::local_time;
using namespace boost::posix_time;
using namespace BASIC::date_time;

std::string IntToString(int val);

const std::string conversion_table = "ICU_BOOST_CONV";
const std::string icu_routes_table = "ICU_ROUTES";
const std::string icu_schedule_table = "ICU_SCHED_DAYS";

const std::string boost_routes_table = "BOOST_ROUTES";
const std::string boost_schedule_table = "BOOST_SCHED_DAYS";


void drop_table(const std::string& name){
    TQuery qry(&OraSession);

    qry.SQLText = "DROP table " + name + " purge";
    try{
        qry.Execute();
        OraSession.Commit();
    } catch(std::exception& e) {
        std::cout << "Query: 'DROP TABLE " << name << " purge' failed. " << std::endl << e.what() << std::endl;
    }
}

void create_conversion_table() {
    TQuery qry(&OraSession);

    qry.SQLText =
        "CREATE TABLE \"" + conversion_table + "\" ("
            "\"TRIP_ID\" NUMBER(9,0), "
            "\"TRIP_NUM\" NUMBER(3,0), "
            "\"MOVE_ID\" NUMBER(9,0), "
            "\"MOVE_NUM\" NUMBER(3,0), "
            "\"HOURS_IN\" FLOAT(126), "
            "\"HOURS_OUT\" FLOAT(126), "
            "\"DAYS_OFFSET\" NUMBER(2,0), "
            "\"DAYS\" VARCHAR2(7 BYTE) )";
    qry.Execute();
    OraSession.Commit();
}

void create_routes_table(){
    TQuery qry(&OraSession);

    qry.SQLText =
        "CREATE TABLE \"" + icu_routes_table + "\" ( "
            "\"MOVE_ID\" NUMBER(9,0), "
            "\"NUM\" NUMBER(3,0), "
            "\"AIRP\" VARCHAR2(3 BYTE), "
            "\"AIRP_FMT\" NUMBER(1,0), "
            "\"AIRLINE\" VARCHAR2(3 BYTE), "
            "\"AIRLINE_FMT\" NUMBER(1,0), "
            "\"FLT_NO\" NUMBER(5,0), "
            "\"SUFFIX\" VARCHAR2(1 BYTE), "
            "\"SUFFIX_FMT\" NUMBER(1,0), "
            "\"CRAFT\" VARCHAR2(3 BYTE), "
            "\"CRAFT_FMT\" NUMBER(1,0), "
            "\"SCD_IN\" DATE, "
            "\"DELTA_IN\" NUMBER(2,0), "
            "\"SCD_OUT\" DATE, "
            "\"DELTA_OUT\" NUMBER(2,0), "
            "\"TRIP_TYPE\" VARCHAR2(1 BYTE), "
            "\"LITERA\" VARCHAR2(3 BYTE), "
            "\"PR_DEL\" NUMBER(1,0) DEFAULT 0, "
            "\"F\" NUMBER(3,0), "
            "\"C\" NUMBER(3,0), "
            "\"Y\" NUMBER(3,0), "
            "\"UNITRIP\" VARCHAR2(255 BYTE) )";
    qry.Execute();

    qry.Clear();
    qry.SQLText = "insert into " + icu_routes_table + " select * from routes";
    qry.Execute();
    OraSession.Commit();
}

void create_schedule_table() {
    TQuery qry(&OraSession);

    qry.SQLText =
        "CREATE TABLE \"" + icu_schedule_table + "\" ( "
            "\"TRIP_ID\" NUMBER(9,0), "
            "\"NUM\" NUMBER(3,0), "
            "\"MOVE_ID\" NUMBER(9,0), "
            "\"FIRST_DAY\" DATE, "
            "\"LAST_DAY\" DATE, "
            "\"DAYS\" VARCHAR2(7 BYTE), "
            "\"PR_DEL\" NUMBER(1,0) DEFAULT 0, "
            "\"TLG\" VARCHAR2(10 BYTE), "
            "\"REFERENCE\" VARCHAR2(255 BYTE), "
            "\"REGION\" VARCHAR2(50 BYTE) )";
    qry.Execute();

    qry.Clear();
    qry.SQLText = "insert into " + icu_schedule_table + " select * from sched_days";
    qry.Execute();
    OraSession.Commit();
}

// Preparation to DB conversion

using ASTRA::NoExists;

/*inline*/ TDateTime formDate(TDateTime date, TDateTime time)
{
    if(date == NoExists || time == NoExists)
        return NoExists;

    TDateTime dt;
    modf(date, &dt);
    return dt + time;
}

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
    /* korotaev Удалить после конвертации */
    tz_database &get_tz_database()
    {
        static bool init=false;
        static tz_database tz_db;
        if (!init) {
            try
            {
                tz_db.load_from_file("date_time_zonespec.csv");
                init=true;
            }
            catch (boost::local_time::data_not_accessible)
            {
                throw EXCEPTIONS::Exception("File 'date_time_zonespec.csv' not found");
            }
            catch (boost::local_time::bad_field_count)
            {
                throw EXCEPTIONS::Exception("File 'date_time_zonespec.csv' wrong format");
            };
        }

        return tz_db;
    }

    TDateTime UTCToLocal(TDateTime d, const std::string& region)
    {
        if (region.empty()) throw EXCEPTIONS::Exception("Region not specified");
        tz_database &tz_db = get_tz_database();
        time_zone_ptr tz = tz_db.time_zone_from_region(region);
        if (tz==NULL) throw EXCEPTIONS::Exception("Region '%s' not found",region.c_str());
        local_date_time ld(DateTimeToBoost(d),tz);
        return BoostToDateTime(ld.local_time());
    }

    TDateTime LocalToUTC(TDateTime d, const std::string& region, int is_dst)
    {
        if (region.empty()) throw EXCEPTIONS::Exception("Region not specified");
        tz_database &tz_db = get_tz_database();
        time_zone_ptr tz = tz_db.time_zone_from_region(region);
        if (tz==NULL) throw EXCEPTIONS::Exception("Region '%s' not found",region.c_str());
        ptime pt=DateTimeToBoost(d);
        try {
            local_date_time ld(pt.date(),pt.time_of_day(),tz,local_date_time::EXCEPTION_ON_ERROR);
            return BoostToDateTime(ld.utc_time());
        }
        catch( boost::local_time::ambiguous_result ) {
            if (is_dst == ASTRA::NoExists) throw;
            local_date_time ld(pt.date(),pt.time_of_day(),tz,(bool)is_dst);
            return BoostToDateTime(ld.utc_time());
        }
    }
}

float getIcuBoostHoursDiff(TDateTime udt_orig, const std::string& region) {
    if(udt_orig == NoExists)
        return 0;

    TDateTime udt_icu = 0.0;
    TDateTime ldt_boost = 0.0;

    ldt_boost = old::UTCToLocal(udt_orig, region);

    try{
        udt_icu = BASIC::date_time::LocalToUTC(ldt_boost, region);
    }
    catch(boost::local_time::ambiguous_result& e) {
    // Добавляем день, переводим, вычитаем день
        udt_icu = BASIC::date_time::LocalToUTC(ldt_boost + 1, region) - 1;
    }

    int h, m, s;
    TDateTime diff = udt_icu - udt_orig;

    DecodeTime(diff, h, m, s);
    float res = float(h) + float(m)/60;
    return diff < 0 ? -res : res;
}

int getIcuBoostDaysDiff(TDateTime udt_orig, const std::string& region) {
    if(udt_orig == NoExists)
        return 0;

    TDateTime ldt_boost = 0.0;
    TDateTime udt_icu = 0.0;

    ldt_boost = old::UTCToLocal(udt_orig, region);

    try {
        udt_icu = BASIC::date_time::LocalToUTC(ldt_boost, region);
    }
    catch(boost::local_time::ambiguous_result& e) {
        // Добавляем день, переводим, вычитаем день
        udt_icu = BASIC::date_time::LocalToUTC(ldt_boost + 1, region) - 1;
    }

    TDateTime dorig, dicu;
    modf(udt_orig, &dorig);
    modf(udt_icu, &dicu);

    return dicu - dorig;
}

struct MoveItem {

    TimeWithDelta in; // routes: scd_in + delta_in
    TimeWithDelta out; // routes: scd_out + delta_out
    const std::string* pRegion;

    float icu_in_hours;
    float icu_out_hours;

    MoveItem() : icu_in_hours(0), icu_out_hours(0), pRegion(NULL) {}

    MoveItem(TDateTime scdIn, int deltaIn, TDateTime scdOut, int deltaOut, const std::string* reg) :
        in(scdIn, deltaIn), out(scdOut, deltaOut), icu_in_hours(0), icu_out_hours(0), pRegion(reg)  { }

    void convert(TDateTime base) {
        //std::cout << "\tmove_item base " << DateTimeToStr(base).c_str() << std::endl;

        icu_in_hours  = getIcuBoostHoursDiff(in.applyTo(base), *pRegion);
        icu_out_hours = getIcuBoostHoursDiff(out.applyTo(base), *pRegion);

        //std::cout << "\tDiffs in " << icu_in_hours << " out " << icu_out_hours << std::endl;
    }
};

std::string ShiftDays( std::string days, int offset )
{
  std::string res = ASTRA::NoDays;
  for ( int i = 0; i < 7; i++ ) {
    if ( days[ i ] == '.' )
      continue;

    int day = static_cast<int>(std::strtol( days.substr(i,1).c_str(), NULL, 10 )) + offset;
    if ( day > 7 )
      day -= 7;
    else
      if ( day <= 0 )
        day += 7;

    res[ day - 1 ] = *IntToString( day ).c_str();
  }
  return res;
}

typedef std::map<std::string, TDateTime> vars_t;

void fillConvTable(int trip_id, int trip_order,
                   int move_id,   int move_order,
                   float hours_in, float hours_out,
                   int days_offset, const std::string& days) {
 // ==========================================================

    if(hours_in || hours_out || days_offset) {
        TQuery qry(&OraSession);
        std::ostringstream out;

        out << "insert into " << conversion_table << " values(" << trip_id << ',' << trip_order << ',' << move_id << ',' << move_order << ','
                 << ":hours_in, :hours_out," << days_offset << ',' << (days.empty() ? "null": "'"+days+"'") <<  ")";
        qry.SQLText = out.str();

        qry.CreateVariable("hours_in", otFloat, hours_in);
        qry.CreateVariable("hours_out", otFloat, hours_out);

        qry.Execute();
    }
}

struct Move {
    typedef int order_type;

    int id;
    TDateTime begin; // sched_days first_day
    TDateTime end; // sched_days last_day

    int icu_days_offset;

    typedef std::map<order_type, MoveItem> item_list_t;
    item_list_t items;
    std::string bstDays;

    bool grouped;

    Move() : id(-1), begin(0.), end(0.), icu_days_offset(0), grouped(false) {}

    Move(int Id, TDateTime b, TDateTime e, const std::string& boostDays) :
         id(Id), begin(b), end(e), icu_days_offset(0), bstDays(boostDays), grouped(false) {}

    void add(order_type o, MoveItem item) { items[o] = item; }

    void convert() {
        item_list_t::iterator f = items.begin();

        icu_days_offset = getIcuBoostDaysDiff(formDate(begin, f->second.out), *f->second.pRegion);

        for(; f != items.end(); ++f) {
            f->second.convert(begin);
        }
    }

    void applyUpdates(int trip_id, int move_ord) const {

        std::string days = icu_days_offset ? ShiftDays(bstDays, icu_days_offset) : "";
        for( item_list_t::const_iterator i = items.begin(); i != items.end(); ++i) {
            fillConvTable(trip_id, move_ord, id, i->first,
                         i->second.icu_in_hours, i->second.icu_out_hours,
                         icu_days_offset, days);
        }
    }

    bool equal(const Move& oth) {
        if(grouped) return false;

        if(id != oth.id || icu_days_offset != oth.icu_days_offset) return false;

        item_list_t::const_iterator i = items.begin();
        item_list_t::const_iterator j = oth.items.begin();

        for(; i != items.end(); ++i, ++j) {
            if(i->second.icu_in_hours != j->second.icu_in_hours ||
               i->second.icu_out_hours != j->second.icu_out_hours)
                return false;
        }

        return true;
    }

    bool hasChanges() {
        if(icu_days_offset) return true;

        for(item_list_t::const_iterator i = items.begin(); i != items.end(); ++i) {
            if(i->second.icu_in_hours || i->second.icu_out_hours )
                return true;
        }

        return false;
    }

    void setGrouped() { grouped = true; }
    bool isGrouped() { return grouped; }

    static int getNewId() {
        TQuery qry( &OraSession );
        qry.SQLText = "SELECT routes_move_id.nextval AS move_id FROM dual";
        qry.Execute();
        return qry.FieldAsInteger(0);
    }

    int duplicate() {
        int new_id = getNewId();

        TQuery qry( &OraSession );
        qry.SQLText = "insert into " + icu_routes_table +
            " select :new_id, NUM, AIRP, AIRP_FMT, AIRLINE, AIRLINE_FMT, FLT_NO, SUFFIX, SUFFIX_FMT, "
            "CRAFT, CRAFT_FMT, SCD_IN, DELTA_IN, SCD_OUT, DELTA_OUT, TRIP_TYPE, LITERA, PR_DEL, F, C, Y, UNITRIP "
            "from " + icu_routes_table + " WHERE move_id = :old_id";
        qry.CreateVariable("new_id", otInteger, new_id);
        qry.CreateVariable("old_id", otInteger, id);
        qry.Execute();

        return new_id;
    }

    void replaceId(int new_id) {
        id = new_id;
    }
};

struct Trip {
    typedef int move_order_type;
    typedef int move_id_type;

    typedef std::pair<move_id_type, move_order_type> move_key_t;
    typedef std::map<move_key_t, Move> move_list_t;

    int id_;
    move_list_t moves;

    typedef std::vector<move_list_t::iterator> grp_moves_t;

    typedef std::pair<size_t, move_id_type> grp_key_t;
    typedef std::map<grp_key_t, grp_moves_t> grp_list_t;
    grp_list_t newMovesList;

    Trip() : id_(-1) {}
    Trip(int id) : id_(id) {}

    Move& add(move_id_type id, move_order_type o, Move move) {
        return moves[ move_key_t(id, o) ] = move;
    }

    bool haveOldMoves(int id) {
        for(move_list_t::iterator i = moves.begin(); i != moves.end(); ++i) {
            if(i->first.first == id && !i->second.isGrouped())
                return true;
        }

        return false;
    }

    void replaceMoveId(const grp_moves_t& moves, int new_id) {
        TQuery qry(&OraSession);
        qry.SQLText = "update " + icu_schedule_table + " set move_id = :new_id where trip_id = :trip_id and num = :trip_num";
        qry.CreateVariable("new_id", otInteger, new_id);
        qry.CreateVariable("trip_id", otInteger, id_);

        for(grp_moves_t::const_iterator i = moves.begin(); i != moves.end(); ++i) {
            int trip_num = (*i)->first.second;
            qry.CreateVariable("trip_num", otInteger, trip_num);

            qry.Execute();
        }
    }

    void applyUpdates() {
        flushNewMoves();
        for(move_list_t::const_iterator i = moves.begin(); i != moves.end(); ++i) {
            i->second.applyUpdates(id_, i->first.second);
        }
    }

    void genNewMoves() {
        size_t count = 0;
        for(move_list_t::iterator i = moves.begin(); i != moves.end(); ++i) {
            if(i->second.isGrouped())
                continue;

            bool hasChanges = i->second.hasChanges();

            grp_moves_t grp;
            grp.push_back(i);
            move_list_t::iterator j = i;

            for(++j; j != moves.end(); ++j) {
                if( i->second.equal(j->second) ) {
                    grp.push_back(j);
                    j->second.setGrouped();
                }
            }

            if(grp.size() > 1 || hasChanges) {
                i->second.setGrouped();
                newMovesList[std::make_pair(count++, i->first.first /* move_id */) ] = grp;
            }
        }

       /* for(grp_list_t::iterator i = newMovesList.begin(); i != newMovesList.end(); ++i) {
            std::cout << i->first.first << " move: " << i->first.second << std::endl;
            grp_moves_t& vec = i->second;
            for(grp_moves_t::iterator j = vec.begin(); j != vec.end(); ++j) {
                std::cout << "  " << "num: " << (*j)->first.second << std::endl;
            }
        }*/
    }

    void flushNewMoves() {
         std::set<move_id_type> processed;

        for(grp_list_t::const_iterator m = newMovesList.begin(); m != newMovesList.end(); ++m) {
            grp_moves_t list = m->second;
            int move_id = m->first.second;

            bool bOld = haveOldMoves(move_id);
            bool updated = processed.find(move_id) != processed.end();

            if(updated || bOld) {
                int new_id = list.front()->second.duplicate();
                replaceMoveId(m->second, new_id);

                for(grp_moves_t::iterator i = list.begin(); i != list.end(); ++i) {
                    (*i)->second.replaceId(new_id);
                }
            }

            processed.insert(move_id);
        }
    }
};

typedef std::map<int, Trip> TripList;

void updateTrips(TripList& trips, std::vector<std::string>& regions) {

    TQuery Qry(&OraSession);
    Qry.Clear();

    Qry.SQLText = "select sd.trip_id, sd.num, sd.move_id, rt.num as item_num, sd.first_day, sd.last_day, sd.days, "
                  "rt.scd_in - trunc(rt.scd_in) scd_in, rt.delta_in, rt.scd_out - trunc(rt.scd_out) scd_out, rt.delta_out, c.tz_region as airp_reg "
                  "from " + icu_schedule_table + " sd "
                  "join " + icu_routes_table + " rt on sd.move_id = rt.move_id "
                  "join airps a on rt.airp = a.code "
                  "join cities c on c.code = a.city "
                  "order by sd.trip_id, sd.num, sd.move_id, rt.num";

    Qry.Execute();
    while(!Qry.Eof) {
        int tripId = Qry.FieldAsInteger("trip_id");
        TripList::iterator ExistTrip =  trips.find(tripId);

        Trip& trip = (ExistTrip == trips.end() ? trips[tripId] = Trip(tripId) : trips[tripId]);

        while(!Qry.Eof) {
            if(tripId != Qry.FieldAsInteger("trip_id"))
                break;

            int moveId = Qry.FieldAsInteger("move_id");
            int move_order = Qry.FieldAsInteger("num");

            Move& move = trip.add(moveId, move_order,
                                  Move(moveId, Qry.FieldAsDateTime("first_day"),
                                               Qry.FieldAsDateTime("last_day"),
                                               Qry.FieldAsString("days")) );
            while(!Qry.Eof) {
                if(moveId != Qry.FieldAsInteger("move_id") || move_order != Qry.FieldAsInteger("num"))
                    break;

                TDateTime scd_in = Qry.FieldIsNULL("scd_in") ? NoExists : Qry.FieldAsDateTime("scd_in");
                int delta_in = Qry.FieldIsNULL("delta_in") ? 0 : Qry.FieldAsInteger("delta_in");
                TDateTime scd_out = Qry.FieldIsNULL("scd_out") ? NoExists : Qry.FieldAsDateTime("scd_out");
                int delta_out = Qry.FieldIsNULL("delta_out") ? 0 : Qry.FieldAsInteger("delta_out");

                std::vector<std::string>::iterator preg =
                     std::find(regions.begin(), regions.end(),
                               std::string(Qry.FieldAsString("airp_reg")));

                MoveItem moveItem(scd_in, delta_in, scd_out, delta_out, &(*preg));

                move.add(Qry.FieldAsInteger("item_num"), moveItem);
                Qry.Next();
            }

            try {
                move.convert();
            }
            catch(std::exception& e){
                std::cout << "Exception " << e.what() << " in func: " << __FUNCTION__ << " trip: " << tripId << std::endl;
            }
        }
    }

    for(TripList::iterator i = trips.begin(); i != trips.end(); ++i)
        i->second.genNewMoves();
}

void getRegions(std::vector<std::string>& regs) {

    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText = "select distinct tz_region from cities where tz_region is not null order by tz_region";

    for(Qry.Execute(); !Qry.Eof; Qry.Next())
        regs.push_back(Qry.FieldAsString("tz_region"));
}

void generateConversionData() {
    std::cout << "Generating updates..." << std::endl;

    std::vector<std::string> regions;
    getRegions(regions);

    TripList trips;
    std::stringstream query;

    updateTrips(trips, regions);

    try {
        for(TripList::iterator tripIt = trips.begin(); tripIt != trips.end(); ++tripIt)
            tripIt->second.applyUpdates();

        OraSession.Commit();
    }
    catch(std::exception& e) {
        std::cout << "In Function: " << __FUNCTION__ << " Exception: " << e.what() << std::endl;
        OraSession.Rollback();
    }

}

/* Применяем обновления */

void genShedUpdateSQL(TQuery& in, TQuery& out) {
    int offset = in.FieldAsInteger("days_offset");

    TDateTime fd = in.FieldAsDateTime("first_day") + offset;
    TDateTime ld = in.FieldAsDateTime("last_day") + offset;

    std::string new_days = in.FieldIsNULL("new_days") ? in.FieldAsString("days"): in.FieldAsString("new_days");

    out.SQLText = "update " + icu_schedule_table + " set first_day = :new_fd, last_day = :new_ld, days = :new_days where trip_id = :trip_id and num = :num and move_id = :move_id";

    out.CreateVariable("trip_id", otInteger, in.FieldAsInteger("trip_id"));
    out.CreateVariable("num", otInteger, in.FieldAsInteger("num"));
    out.CreateVariable("move_id", otInteger, in.FieldAsInteger("move_id"));

    out.CreateVariable("new_fd", otDate, fd);
    out.CreateVariable("new_ld", otDate, ld);
    out.CreateVariable("new_days", otString, new_days);
}

void genRouteUpdateSQL(TQuery& in, TQuery& out) {
    TDateTime scd_in = in.FieldAsDateTime("scd_in") + double(in.FieldAsFloat("hours_in"))/24;
    TDateTime scd_out = in.FieldAsDateTime("scd_out") + double(in.FieldAsFloat("hours_out"))/24;

    out.SQLText = "update " + icu_routes_table + " set scd_in = :scd_in, scd_out = :scd_out where move_id = :move_id and num = :num";

    out.CreateVariable("num", otInteger, in.FieldAsInteger("item_num"));
    out.CreateVariable("move_id", otInteger, in.FieldAsInteger("move_id"));
    out.CreateVariable("scd_in", otDate, scd_in);
    out.CreateVariable("scd_out", otDate, scd_out);
}

void applyConversion() {

    std::cout << "Apply updates..." << std::endl;

    TQuery Qry(&OraSession);
    TQuery updSchedQry(&OraSession);
    TQuery updRouteQry(&OraSession);

    Qry.SQLText =
            "select sd.trip_id, sd.num, sd.move_id, rt.num as item_num, "
            "sd.first_day, sd.last_day, sd.days, rt.scd_in, rt.scd_out, "
            "cv.hours_in, cv.hours_out, cv.days_offset, cv.days as new_days "

            "from " + icu_schedule_table + " sd "
            "join " + icu_routes_table + " rt on sd.move_id = rt.move_id "
            "join " + conversion_table + " cv on rt.move_id = cv.move_id and "
                                      "rt.num = cv.move_num and "
                                      "cv.trip_id = sd.trip_id and "
                                      "cv.trip_num = sd.num "
            "order by sd.trip_id, sd.num, sd.move_id, rt.num";

    unsigned cnt = 0;

    try{
        for(Qry.Execute(); !Qry.Eof; Qry.Next()) {
            genShedUpdateSQL(Qry, updSchedQry);
            genRouteUpdateSQL(Qry, updRouteQry);

            updSchedQry.Execute();
            updRouteQry.Execute();

            if(++cnt % 1000 == 0)
                std::cout << '\r' << cnt << std::flush;
        }
        std::cout << '\r' << cnt << std::endl;
        OraSession.Commit();
    }
    catch(std::exception& e){
        OraSession.Rollback();
        std::cout << e.what() << std::endl;
    }
}

void replace_tables() {
    TQuery qry(&OraSession);
    try {
        qry.SQLText = "RENAME SCHED_DAYS TO " + boost_schedule_table;
        qry.Execute();
        qry.Clear();

        qry.SQLText = "RENAME " + icu_schedule_table + " TO SCHED_DAYS";
        qry.Execute();
        qry.Clear();

        qry.SQLText = "RENAME ROUTES TO " + boost_routes_table;
        qry.Execute();
        qry.Clear();

        qry.SQLText = "RENAME " + icu_routes_table + " TO ROUTES";
        qry.Execute();

        drop_table(conversion_table);
        OraSession.Commit();
    }
    catch(std::exception& e){
        std::cout << "Exception in " << __FUNCTION__ << ": " << e.what() << std::endl;
    }
}

void rollback_tables(){
    TQuery qry(&OraSession);
    try {
        qry.SQLText = "RENAME SCHED_DAYS TO " + icu_schedule_table;
        qry.Execute();
        qry.Clear();

        qry.SQLText = "RENAME " + boost_schedule_table + " TO SCHED_DAYS";
        qry.Execute();
        qry.Clear();

        qry.SQLText = "RENAME ROUTES TO " + icu_routes_table;
        qry.Execute();

        qry.SQLText = "RENAME " + boost_routes_table + " TO ROUTES";
        qry.Execute();
        qry.Clear();

        OraSession.Commit();
    }
    catch(std::exception& e){
        std::cout << "Exception in " << __FUNCTION__ << ": " << e.what() << std::endl;
    }
}

bool isTableExists(const std::string& table){
    if(table.empty())
        return false;

    TQuery q(&OraSession);
    q.SQLText = "select * from " + table;
    
    bool res = false;
    try {
        q.Execute();
        res = true;
    }
    catch(...) {}

    return res;
}

void TZUpdate() {

    if(isTableExists(boost_schedule_table) || isTableExists(boost_routes_table) ) {
        
        std::cout <<
             "В базе данных найдены backup-таблицы (" << boost_schedule_table << ", " << boost_routes_table << "). " << std::endl <<
             "Возможно в таблицах SCHED_DAYS и ROUTES находятся уже сконвертированные данные."                       << std::endl <<
             "Убедитесь что в таблицах SCHED_DAYS и ROUTES находятся данные подлежащие конвертации, переименуйте или удалите backup-таблицы, и перезапустите процедуру конвертации."
        << std::endl;
        return;
    }
    

    std::cout << "Creating tables..." << std::endl;
try{
    drop_table(conversion_table);
    drop_table(icu_schedule_table);
    drop_table(icu_routes_table);

    create_conversion_table();
    create_routes_table();
    create_schedule_table();

    std::cout << "Generating conversion data." << std::endl;

    generateConversionData();

    std::cout << "Applying conversion.." << std::endl;

    applyConversion();

//    replace_tables();

    OraSession.Commit();
    std::cout << "Conversion finished successfully." << std::endl;
}
catch(std::exception &e) {
    std::cout << "Exception: " << e.what() << std::endl;
    OraSession.Rollback();
}

}

