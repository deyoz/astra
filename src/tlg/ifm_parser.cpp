#include "ifm_parser.h"
#include "iatci_api.h"
#include "basetables.h"

#include <typeb/FlightTemplate.h>
#include <typeb/NameTemplate.h>
#include <typeb/EndTemplate.h>
#include <typeb/FlightElem.h>
#include <typeb/NameElem.h>
#include <typeb/typeb_msg.h>


#include <serverlib/isdigit.h>
#include <serverlib/helpcpp.h>
#include <serverlib/dates.h>

#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace typeb_parser {

// RcvFlightElem and RcvFlightTemplate
RcvFlightElem* RcvFlightElem::parse(const std::string &text)
{
    if (!boost::regex_match(text,
         boost::regex("^-[0-9A-Z€-Ÿð]{1,2}[A-Z€-Ÿð]{0,1}\\d{1,4}[A-Z]{0,1}/"
               "[0-9]{2}[A-Z€-Ÿð]{3}((?:)|\\d{2}) [A-Z€-Ÿð]{3}[A-Z€-Ÿð]{3}$"),
               boost::match_any))
    {
        throw typeb_lz_parse_except(STDLOG, TBMsg::INV_FORMAT_z1z) <<
                "Flight element";
    }

    std::string txt = text.substr(1);

    std::string::size_type pos = txt.find('/');
    std::string Flight = txt.substr(0, pos);
    if(ISALPHA(Flight.at(Flight.size()-1)))
    {
        // Žâá¥ª ¥¬ áãää¨ªá
        Flight.erase(Flight.size()-1,1);
    }

    if(Flight.length() < 3 || Flight.length() > 8)
    {
        throw typeb_lz_parse_except(STDLOG, TBMsg::INV_FORMAT_z1z) << "Flight element";
    }

    std::string Airline = Flight.substr(0, (ISDIGIT(Flight[2]) ? 2 : 3));

    unsigned FlNum;
    try
    {
        FlNum = boost::lexical_cast<unsigned>(Flight.substr(Airline.length()));
    }
    catch(boost::bad_lexical_cast &e)
    {
        throw typeb_lz_parse_except(STDLOG, TBMsg::INV_FORMAT_z1z_FIELD_z2z, e.what()) <<
                "Flight element" << "Flight number";
    }

    std::vector<std::string> other_args;
    std::string other_args_str(txt,pos+1);
    boost::split(other_args, other_args_str, boost::algorithm::is_any_of(" "));

    if(other_args.size() < 2)
    {
        throw typeb_lz_parse_except(STDLOG, TBMsg::INV_FORMAT_z1z_FIELD_z2z) <<
                "Flight element" << "date dep, dep point, part number";
    }

    if(other_args[0].length() != 5 && other_args[1].length() != 6)
    {
        throw typeb_lz_parse_except(STDLOG, TBMsg::INV_FORMAT_z1z_FIELD_z2z) <<
                "Flight element" << "date dep";
    }

    std::string Depp = other_args[1].substr(0, 3);
    std::string Arrp = other_args[1].substr(3, 3);

    std::string Host;
    if(other_args.size() == 3) {
        Host = other_args[2];
    }

    Dates::Date_t DepDate = boost::gregorian::day_clock::local_day();
    if(other_args[0].length() == 7)
        DepDate = Dates::DateFromDDMONYY(other_args[0],
                                         Dates::YY2YYYY_UseCurrentCentury,
                                         boost::gregorian::day_clock::local_day());
    else if(other_args[0].length() == 5)
        DepDate = Dates::DateFromDDMON(other_args[0],
                                       Dates::GuessYear_Itin(),
                                       boost::gregorian::day_clock::local_day());

    if(DepDate.is_not_a_date())
        throw typeb_lz_parse_except(STDLOG, TBMsg::INV_FORMAT_z1z_FIELD_z2z, "invalid date") <<
                "Flight element" << "date dep";

    if(other_args[1].length() != 6)
    {
        throw typeb_lz_parse_except(STDLOG, TBMsg::INV_FORMAT_z1z_FIELD_z2z) <<
                "Flight element" << "dep point and arr point";
    }

    return new RcvFlightElem(Airline,
                             FlNum,
                             DepDate,
                             Depp,
                             Arrp,
                             Host);
}

//---------------------------------------------------------------------------------------

TbElement* RcvFlightTemplate::parse(const std::string &txt) const
{
    return RcvFlightElem::parse(txt);
}

//---------------------------------------------------------------------------------------

ActionIndicatorElem* ActionIndicatorElem::parse(const std::string &text)
{
    if(text != "DEL" && text != "CHG")
    {
        throw typeb_lz_parse_except(STDLOG, TBMsg::INV_FORMAT_z1z) <<
                "Action indicator element";
    }

    return new ActionIndicatorElem(text);
}

//---------------------------------------------------------------------------------------

TbElement* ActionIndicatorTemplate::parse(const std::string &txt) const
{
    return ActionIndicatorElem::parse(txt);
}

//---------------------------------------------------------------------------------------

IFM_template::IFM_template()
    : typeb_template("IFM", "Inter airline fallback message")
{
    addElement(new FlightTemplate);
    addElement(new RcvFlightTemplate);
    addElement(new ActionIndicatorTemplate);
    addElement(new NameTemplate);
    addElement(new EndTemplate);
}

}//namespace typeb_parser

/////////////////////////////////////////////////////////////////////////////////////////

namespace TypeB {

void HandleTypebIfm::handle(const typeb_parser::TypeBMessage& tbmsg)
{
    using namespace typeb_parser;
    LogTrace(TRACE1) << "Enter to HandleTypebIfm::handle";

    const ActionIndicatorElem* act = TbElem_cast<ActionIndicatorElem>(tbmsg.findp("ActIndicator"));
    ASSERT(act != NULL);
    LogTrace(TRACE1) << "Handle " << act->act() << " action";

    const FlightElem* fl = TbElem_cast<FlightElem>(tbmsg.findp("Flight"));
    if(fl) {
        LogTrace(TRACE1) << "Delivering flight: "
                         << fl->airline() << "-" << fl->flightNum()
                         << "/" << fl->depDate() << " "
                         << "(" << fl->depPoint() << ") ";
    }

    const RcvFlightElem* rfl = TbElem_cast<RcvFlightElem>(tbmsg.findp("RcvFlight"));
    ASSERT(rfl != NULL);
    LogTrace(TRACE1) << "Receiving flight: "
                     << rfl->airline() << "-" << rfl->flightNum()
                     << "/" << rfl->depDate() << " "
                     << "(" << rfl->depPoint() << " " << rfl->arrPoint() << ") "
                     << " " << rfl->hostIndication();

    const NameElem* name = TbElem_cast<NameElem>(tbmsg.findp("Name"));
    ASSERT(name != NULL);
    LogTrace(TRACE1) << "Name: "
                     << name->surName() << " " << name->passName()
                     << " : " << name->nseats();


    // TODO
    std::list<iatci::dcqckx::PaxGroup> lPxg;
    lPxg.push_back(
        iatci::dcqckx::PaxGroup(iatci::PaxDetails(name->surName(),
                                                  name->passName(),
                                                  iatci::PaxDetails::Adult)));

    iatci::dcqckx::FlightGroup flg(iatci::FlightDetails(BaseTables::Company(rfl->airline())->rcode(),
                                                        Ticketing::FlightNum_t(rfl->flightNum()),
                                                        BaseTables::Port(rfl->depPoint())->rcode(),
                                                        BaseTables::Port(rfl->arrPoint())->rcode(),
                                                        rfl->depDate()),
                                   boost::none,
                                   lPxg);

    iatci::CkxParams cancelParams(iatci::OriginatorDetails(""),
                                  boost::none,
                                  flg);

    if(iatci::cancelCheckin(cancelParams).status() != iatci::dcrcka::Result::OkWithNoData) {
        LogError(STDLOG) << "Unable to handle IFM with DEL indicator!";
    }
}

}//namespace TypeB
