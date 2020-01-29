/*
*  C++ Implementation: FlightElem
*
* Description: FlightElem в ETL телеграмме
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
*
*/
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/regex.hpp>
#include <vector>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include "FlightElem.h"
#include "typeb_msg.h"
#include "typeb_parse_exceptions.h"
#include <serverlib/isdigit.h>
#include <serverlib/dates.h>

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace typeb_parser
{
using std::string;
using std::vector;
using boost::bad_lexical_cast;
using namespace boost::gregorian;
using namespace boost;
using namespace boost::posix_time;


namespace
{

boost::gregorian::date MakeGregorianDate_NoThrow(unsigned year, unsigned month, unsigned day)
{
    try {
        return boost::gregorian::date(year, month, day);
    } catch (const boost::gregorian::bad_year&) {
        return boost::gregorian::date();
    } catch (const boost::gregorian::bad_month&) {
        return boost::gregorian::date();
    } catch (const boost::gregorian::bad_day_of_month&) {
        return boost::gregorian::date();
    }
}

boost::gregorian::date guessYear(unsigned month, unsigned day,
                                 boost::gregorian::date currentDate,
                                 unsigned limitFuture, unsigned limitPast)
{
    boost::gregorian::date date = MakeGregorianDate_NoThrow(currentDate.year(), month, day);
    if (date.is_not_a_date()) return date;

    if (date > currentDate + boost::gregorian::days(limitFuture))
        return MakeGregorianDate_NoThrow(date.year() - 1, month, day);
    else if (date < currentDate - boost::gregorian::days(limitPast))
        return MakeGregorianDate_NoThrow(date.year() + 1, month, day);
    else
        return date;
}

boost::gregorian::date DateFromDDMON_NoThrow(const string& dateStr,
        boost::gregorian::date currentDate = boost::gregorian::day_clock::local_day(),
        unsigned limitFuture = 180, unsigned limitPast = 180)
{
    unsigned day = 0;
    char monthStr[8] = { 0 };
    int n = 0;

    if (dateStr.size() != 5
        || sscanf(dateStr.c_str(), "%2u%3s%n", &day, monthStr, &n) != 2
        || n != 5)
    {
        return boost::gregorian::date();
    }

    unsigned month = Dates::getMonthNum(monthStr);
    if (month == 0) return boost::gregorian::date();

    return guessYear(month, day, currentDate, limitFuture, limitPast);
}

boost::gregorian::date DateFromDDMONYY_NoThrow(const string& dateStr)
{
    unsigned day = 0, year = 0;
    char monthStr[8] = { 0 };
    int n = 0;

    if (dateStr.size() != 7
        || sscanf(dateStr.c_str(), "%2u%3s%2u%n", &day, monthStr, &year, &n) != 3
        || n != 7)
    {
        return boost::gregorian::date();
    }

    unsigned month = Dates::getMonthNum(dateStr.substr(2, 3));
    if (month == 0) return boost::gregorian::date();

    year = Dates::NormalizeYear(year);

    return MakeGregorianDate_NoThrow(year, month, day);
}

} // namespace

///////////////////////////////////////////////////////////////////////////////

// P2452/24OCT TJM PART1
FlightElem * FlightElem::parse (const std::string &txt)
{
    // D{0,1} вставлено только ради кривой DCS в домодедово
    // Типа дополнительный рейс
    if (!regex_match(txt,
         regex("^[0-9A-ZА-ЯЁ]{1,2}[A-ZА-ЯЁ]{0,1}\\d{1,4}[A-Z]{0,1}/"
               "[0-9]{2}[A-ZА-ЯЁ]{3}((?:)|\\d{2}) [A-ZА-ЯЁ]{3}"
               " PART\\d{1,2}$"),
               match_any))
    {
        throw typeb_lz_parse_except(STDLOG, TBMsg::INV_FORMAT_z1z) <<
                "Flight element";
    }

    string::size_type pos = txt.find('/');
    std::string Flight = txt.substr(0, pos);
    if(ISALPHA(Flight.at(Flight.size()-1)))
    {
        // Отсекаем долбаный суфикс
        Flight.erase(Flight.size()-1,1);
    }

    if(Flight.length() < 3 || Flight.length() > 8)
    {
        throw typeb_lz_parse_except(STDLOG, TBMsg::INV_FORMAT_z1z) << "Flight element";
    }

    std::string Airline;
    if(ISDIGIT(Flight[2]))
        Airline = Flight.substr(0,2);
    else
        Airline = Flight.substr(0,3);

    unsigned FlNum;
    try{
        FlNum = lexical_cast<unsigned>(Flight.substr(Airline.length()));
    }
    catch(bad_lexical_cast &e)
    {
        throw typeb_lz_parse_except(STDLOG, TBMsg::INV_FORMAT_z1z_FIELD_z2z, e.what()) <<
                "Flight element" << "Flight number";
    }

    vector<string> date_depp_part;
    std::string date_depp_part_str(txt,pos+1);
    boost::split(date_depp_part, date_depp_part_str, boost::algorithm::is_any_of(" "));

    if(date_depp_part.size()!=3)
    {
        throw typeb_lz_parse_except(STDLOG, TBMsg::INV_FORMAT_z1z_FIELD_z2z) <<
                "Flight element" << "date dep, dep point, part number";
    }

    if(date_depp_part[0].length() != 5 && date_depp_part[0].length() != 7)
    {
        throw typeb_lz_parse_except(STDLOG, TBMsg::INV_FORMAT_z1z_FIELD_z2z) <<
                "Flight element" << "date dep";
    }

    date DepDate;
    if(date_depp_part[0].length() == 7)
        DepDate = DateFromDDMONYY_NoThrow(date_depp_part[0]);
    else if(date_depp_part[0].length() == 5)
        DepDate = DateFromDDMON_NoThrow(date_depp_part[0]);

    if(DepDate.is_not_a_date())
        throw typeb_lz_parse_except(STDLOG, TBMsg::INV_FORMAT_z1z_FIELD_z2z, "invalid date") <<
                "Flight element" << "date dep";

    if(date_depp_part[1].length() != 3)
    {
        throw typeb_lz_parse_except(STDLOG, TBMsg::INV_FORMAT_z1z_FIELD_z2z) <<
                "Flight element" << "dep point";
    }

    static regex re("^PART([[:digit:]]{1,2})$");
    smatch what;
    if (!regex_match(date_depp_part[2], what, re, match_any))
    {
        throw typeb_lz_parse_except(STDLOG, TBMsg::INV_FORMAT_z1z_FIELD_z2z) <<
                "Flight element" << "part number";
    }

    return new FlightElem(Airline, FlNum, DepDate, date_depp_part[1], boost::lexical_cast<size_t>(what[1]));
}


} // typeb_parser

