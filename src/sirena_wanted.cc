#include "basel_aero.h"

#include <string>
#include <vector>
#include <tcl.h>
#include "base_tables.h"

#include "basic.h"
#include "stl_utils.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "xml_unit.h"
#include "cache.h"
#include "passenger.h"
#include "events.h"
#include "jxtlib/xml_stuff.h"
#include "serverlib/logger.h"
#include "http_io.h"

#define NICKNAME "DJEK"
#define NICKTRACE DJEK_TRACE
#include "serverlib/test.h"

using namespace BASIC;
using namespace EXCEPTIONS;



using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;

struct TPaxWanted {
    TDateTime transactionDate;
    string airline;
    string flightNumber; // суффикс???
    TDateTime departureDate;
    string rackNumber;
    string seatNumber;
    string firstName;
    string lastName;
    string patronymic;
    string documentNumber;
    string operationType;
    string baggageReceiptsNumber;
    string departureAirport;
    string arrivalAirport;
    string baggageWeight;
    TDateTime departureTime;
    string PNR;
    string visaNumber;
    TDateTime visaDate;
    string visaPlace;
    string visaCountryCode;
    string nationality;
    string gender;
};


void get_pax_wanted( vector<TPaxWanted> &paxs )
{
    TDateTime time_now = NowUTC();
    paxs.clear();
    TPaxWanted pax;

    // Гражданин США, летящий из Душанбе в Москву
    pax.transactionDate = time_now;
    pax.airline = "ЮТ";
    pax.flightNumber = "001A";
    pax.departureDate = time_now;
    pax.rackNumber = "GATE01";
    pax.seatNumber = "01A";
    pax.firstName = "Иванов";
    pax.lastName = "Иван";
    pax.patronymic = "Иванович";
    pax.documentNumber = "7789365364";
    pax.operationType = "K1";
    pax.baggageReceiptsNumber = "123";
    pax.departureAirport =  "ДШБ";
    pax.arrivalAirport = "ДМД";
    pax.baggageWeight = "300";
    pax.PNR = "494GZL";
    pax.visaNumber = "775";
    pax.visaDate = time_now;
    pax.visaPlace = "Москва";
    pax.visaCountryCode = "РФ";
    pax.nationality = "ЮС";
    pax.gender = "M";
    paxs.push_back(pax);

    // Гражданин США, летящий из Москвы в Душанбе
    pax.firstName = "Петров";
    pax.departureAirport =  "ДМД";
    pax.arrivalAirport = "ДШБ";
    paxs.push_back(pax);

    // Гражданин Таджикистана, летящий неизвестно куда
    pax.firstName = "Сидоров";
    pax.departureAirport =  "ЦДГ";
    pax.arrivalAirport = "ЛБГ";
    pax.nationality = "ТД";
    paxs.push_back(pax);
}

string make_soap_content(const vector<TPaxWanted> &paxs)
{
    ostringstream result;
    result <<
        "<soapenv:Envelope xmlns:soapenv='http://schemas.xmlsoap.org/soap/envelope/' xmlns:sir='http://vtsft.ru/sirenaSearchService/'>\n"
        "   <soapenv:Header/>\n"
        "   <soapenv:Body>\n"
        "      <sir:importASTDateRequest>\n"
        "         <sir:login>test</sir:login>\n";
    for(vector<TPaxWanted>::const_iterator iv = paxs.begin(); iv != paxs.end(); iv++)
        result <<
            "         <sir:policyParameters>\n"
            "            <sir:transactionDate>" << DateTimeToStr(iv->transactionDate, "yyyy-mm-dd") << "</sir:transactionDate>\n"
            "            <sir:transactionTime>" << DateTimeToStr(iv->transactionDate, "yyyymmddhhnnss") << "</sir:transactionTime>\n"
            "            <sir:flightNumber>" << iv->flightNumber << "</sir:flightNumber>\n"
            "            <sir:departureDate>" << DateTimeToStr(iv->departureDate, "yyyy-mm-dd") << "</sir:departureDate>\n"
            "            <sir:rackNumber>" << iv->rackNumber << "</sir:rackNumber>\n"
            "            <sir:seatNumber>" << iv->seatNumber << "</sir:seatNumber>\n"
            "            <sir:firstName>" << iv->firstName << "</sir:firstName>\n"
            "            <sir:lastName>" << iv->lastName << "</sir:lastName>\n"
            "            <sir:patronymic>" << iv->patronymic << "</sir:patronymic>\n"
            "            <sir:documentNumber>" << iv->documentNumber << "</sir:documentNumber>\n"
            "            <sir:operationType>" << iv->operationType << "</sir:operationType>\n"
            "            <sir:baggageReceiptsNumber>" << iv->baggageReceiptsNumber << "</sir:baggageReceiptsNumber>\n"
            "            <sir:airline>" << iv->airline << "</sir:airline>\n"
            "            <sir:departureAirport>" << iv->departureAirport << "</sir:departureAirport>\n"
            "            <sir:arrivalAirport>" << iv->arrivalAirport << "</sir:arrivalAirport>\n"
            "            <sir:baggageWeight>" << iv->baggageWeight << "</sir:baggageWeight>\n"
            "            <sir:departureTime>" << DateTimeToStr(iv->departureDate, "yyyymmddhhnnss") << "</sir:departureTime>\n"
            "            <sir:PNR>" << iv->PNR << "</sir:PNR>\n"
            "            <sir:visaNumber>" << iv->visaNumber << "</sir:visaNumber>\n"
            "            <sir:visaDate>" << DateTimeToStr(iv->visaDate, "yyyy-mm-dd") << "</sir:visaDate>\n"
            "            <sir:visaPlace>" << iv->visaPlace << "</sir:visaPlace>\n"
            "            <sir:visaCountryCode>" << iv->visaCountryCode << "</sir:visaCountryCode>\n"
//            "            <sir:nationality>" << iv->nationality << "</sir:nationality>\n"
            "            <sir:sex>" << iv->gender << "</sir:sex>\n"
            "         </sir:policyParameters>\n";
    result <<
        "      </sir:importASTDateRequest>\n"
        "   </soapenv:Body>\n"
        "</soapenv:Envelope>";
    return ConvertCodepage(result.str(), "CP866", "UTF-8");
}

void send_pax_wanted( const vector<TPaxWanted> &paxs )
{
    try {
        sirena_wanted_send(make_soap_content(paxs));
        ProgTrace(TRACE5, "sirena_wanted_send completed");
    } catch(Exception &E) {
        ProgError(STDLOG, "sirena_wanted_send failed: %s", E.what());
    }
}

void sync_sirena_wanted( TDateTime utcdate )
{
    ProgTrace(TRACE5,"sync_sirena_codes started");
    vector<TPaxWanted> paxs;
    get_pax_wanted( paxs );
    send_pax_wanted( paxs );
}
