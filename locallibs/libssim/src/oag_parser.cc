#include "oag_parser.h"
#include "oag_records_handler.h"

#include <iostream>

#include <boost/lexical_cast.hpp>

#include <serverlib/str_utils.h>

#define NICKNAME "D.ZERNOV"
#include <serverlib/slogger.h>

const std::string HEADER_TITLE = "AIRLINE STANDARD SCHEDULE DATA SET";
const std::string RECORD_SERIAL_NUMBER = "000001";

namespace {

typedef std::map<std::string, std::size_t> StringMap;

void createTerminalsFile(const Oag::StringListMap& airportTerminals)
{
    if (airportTerminals.empty()) {
        std::clog << "Terminals info isn't existed" << std::endl;
        return;
    }
#ifndef XP_TESTING
    std::ifstream cities("CITIES");
    StringMap cityMap;
    if (cities) {
        std::clog << "Opened city-file..." << std::endl;
        while(!cities.eof()) {
            std::string city;
            std::size_t cityID;
            cities >> city >> cityID;
            if (!city.empty()) {
                cityMap[city] = cityID;
            }
        }
    } else {
        std::clog << "CITIES hasn't opened" << std::endl;
        return;
    }

    std::clog << "Open terminals-file..." << std::endl;
    std::ofstream out("terminals.map");
    std::size_t id = 1;
    std::clog << "Fill terminals..." << std::endl;
    for (Oag::StringListMap::const_iterator it = airportTerminals.begin(), end = airportTerminals.end(); it != end; ++it) {
        for (Oag::StringSet::const_iterator term = it->second.begin(), termE = it->second.end(); term != termE; ++term) {
            StringMap::const_iterator cityIt = cityMap.find(it->first);
            if (cityIt != cityMap.end()) {
                out << "0|" << id++ << "|" << cityIt->second << "|" << *term << std::endl;
            }
        }
    }
    out.close();
    std::clog << "Terminal-file has filled..." << std::endl;
#endif
}

void createAircraftFile(const Oag::StringSet& aircrafts)
{
#ifdef XP_TESTING
    std::stringstream out;
#else
    std::ofstream out("aircrafts.map");
#endif
    std::size_t id = 1;
    for (const Oag::StringSet::value_type& aircraft :aircrafts) {
        out << "0|" << aircraft << "||" << id++ << "||" << std::endl;
    }
#ifdef XP_TESTING
    LogTrace(TRACE5) << "AIRCRAFT TYPES:\n" << out.str();
#else
    out.close();
#endif
}

}

namespace Oag {

std::ostream& operator<<(std::ostream& os, const OagParser::Record& record)
{
    std::copy(record.begin(), record.end(), std::ostream_iterator<char>(os));
    return os;
}

std::istream& operator>>(std::istream& is, OagParser::Record& record)
{
    char tmp[2];
    return is.read(record.data(), record.size()).read(tmp, 2);
}

OagParser::OagParser(const std::string& oagFile)
    : inputFile(oagFile.c_str()), err(NO_ERROR), /*currentSeriallNum(1),*/ handledLine(0), parseCompany(true)
{
    if (!inputFile.good()) {
        err = FILE_OPENED_ERROR;
    }
}

OagParser::~OagParser()
{
    createTerminalsFile(airportTerminals);
    createAircraftFile(aircrafts);
}

bool OagParser::ParseFile()
{
    if (!readLine()) {
        return false;
    }

    type = static_cast<RecordType>(record[0] -  '0');
    switch (type) {
    case NULL_RECORD:
        break;
    case HEADER_RECORD:
        if (!ParseHeaderRecord()) {
            return false;
        }
        break;
    case CARRIER_RECORD:
        if (!ParseCarrierRecord()) {
            return false;
        }
        break;
    case FLIGHT_LEG_RECORD:
        if (parseCompany && !ParseFlightLegRecord()) {
            return false;
        }
        break;
    case SEGMENT_DATA_RECORD:
        if (parseCompany && !ParseSegmentDataRecord()) {
            return false;
        }
        break;
    case TRAILER_RECORD:
        if (!ParseTrailerRecord()) {
            return false;
        }
        break;
    default:
        err = RECORD_TYPE_ERROR;
        return false;
    }

    return true;
}

static std::string err_to_string(size_t err)
{
    switch (err) {
    case OagParser::FILE_OPENED_ERROR:
        return std::string("OagParser Error: can not open oag-file");
    case OagParser::RECORD_TYPE_ERROR:
        return "Record type 1: incorrect record type";
    case OagParser::TITLE_CONTENT_ERROR:
        return "Record type 1: incorrect title content";
    case OagParser::DATA_SERIAL_NUMBER_ERROR:
        return "Record type 1: incorrect data serial number";
    case OagParser::TIME_MODE_ERROR:
        return "Record type 2: time mode error";
    case OagParser::SCHEDULE_STATUS_ERROR:
        return "Record type 2: incorrect schedule status";
    case OagParser::SERIAL_NUMBER_ERROR:
        return "Record type all: incorrect record serial number";
    case OagParser::AIRLINE_RECORDS_EQUAL_ERROR:
        return "Record type 3, 4: incorrect airline in record. It must be equal the record 2";
    case OagParser::FLIGHT_NUMBER_ERROR:
        return "Record type 3, 4: incorrect flight number";
    case OagParser::ITINERARY_ID_ERROR:
        return "Record type 3, 4: incorrect itinerary identificator";
    case OagParser::LEG_SEQUENCE_NUMBER_ERROR:
        return "Record type 3, 4: incorrect leg sequence number";
    case OagParser::DATA_ELEMENT_ID_ERROR:
        return "Record type 4: incorrect data element id";
    case OagParser::CHECH_SERIAL_NUMBER_ERROR:
        return "Record type 4: incorrect data element id";
    case OagParser::CONTINUE_END_CODE_ERROR:
        return "Record type 5: incorrect continue end code";
    case OagParser::NO_ERROR:
    default:
        break;
    }
    return std::string();
}

std::string OagParser::getStringError() const
{
    const std::string m = err_to_string(err);
    if (m.empty() || err == FILE_OPENED_ERROR) {
        return m;
    }

    return m + " at line " + std::to_string(handledLine);
}

void OagParser::setRecordsHandler(OagRecordsHandler::OagRecordsHandlerPtr h)
{
    handler = h;
}

void OagParser::addRecordsFilter(OagFilter::OagFilterPtr f)
{
    filters.push_back(f);
}

// Recort Type 1
bool OagParser::ParseHeaderRecord()
{
    std::string title(record.data() + 1, record.data() + 35);
    if (title != HEADER_TITLE) {
        err = TITLE_CONTENT_ERROR;
        return false;
    }

    try {
        std::string dataSetSerialNum(record.data() + 191, record.data() + 194);
        boost::lexical_cast<std::size_t>(StrUtils::trim(dataSetSerialNum));
    } catch (boost::bad_lexical_cast&) {
        err = DATA_SERIAL_NUMBER_ERROR;
        return 0;
    }

    std::string serialNum(record.data() + 194, record.data() + 200);
    if (serialNum != RECORD_SERIAL_NUMBER) {
        err = SERIAL_NUMBER_ERROR;
        return false;
    }
    return true;
}

// Recort Type 2
bool OagParser::ParseCarrierRecord()
{
    applyHandleFilter();
    applyHandleSuffix();

    char tm = record[1];
    //std::cout << "Time mode '" << timeMode << "'" << std::endl;
    if (tm != 'U' && tm != 'L') {
        err = TIME_MODE_ERROR;
        return false;
    }

    currentRecord.reset(new CarrierRecord);
    currentRecord->timeMode = ((tm == 'U') ? CarrierRecord::UTC_TIME_MODE : CarrierRecord::LOCAL_TIME_MODE);

    currentRecord->airline.assign(record.data() + 2, record.data() + 5);
    //std::clog << "Airline Designator '" << carrierRecord->airline << "'" << std::endl;

    handler->handleCarrierProcBegin(currentRecord->airline);

    if (!needContinue()) {
        parseCompany = false;
        return true;
    }
    parseCompany = true;

    currentRecord->schedulePeriodFrom.assign(record.data() + 14, record.data() + 21);
    //std::cout << "Period of Schedule Validity(from) '" << carrierRec.schedulePeriodFrom << "'" << std::endl;

    currentRecord->schedulePeriodTo.assign(record.data() + 21, record.data() + 28);
    //std::cout << "Period of Schedule Validity(to) '" << carrierRec.schedulePeriodTo << "'" << std::endl;

    currentRecord->creationDate.assign(record.data() + 28, record.data() + 35);
    //std::cout << "Creation date '" << carrierRec.creationDate << "'" << std::endl;

    currentRecord->dataTitle = std::string(record.data() + 35, record.data() + 64);
    //std::cout << "Title of data '" << carrierRec.dataTitle.get() << "'" << std::endl;

    currentRecord->releaseDate = StrUtils::trim(std::string(record.data() + 64, record.data() + 71));

    char scheduleStatus = record[71];
    //std::cout << "Schedule status '" << scheduleStatus << "'" << std::endl;
    if (scheduleStatus != 'P' && scheduleStatus != 'C') {
        err = SCHEDULE_STATUS_ERROR;
        return false;
    }
    currentRecord->scheduleStatus = scheduleStatus;

    currentRecord->secureFlight = record[168];

    const std::string inflightService = StrUtils::ltrim(std::string(record.data() + 169, record.data() + 188));
    if (!inflightService.empty()) {
        currentRecord->flightServiceInfo = inflightService;
        //std::cout << "Flying Service Information '" << carrierRec.flightServiceInfo.get() << "'" << std::endl;
    }

    std::string eTicketInfo(record.data() + 188, record.data() + 190);
    if (!eTicketInfo.empty()) {
        currentRecord->eTicketInfo = eTicketInfo;
        //std::cout << "Electronic Ticket Information '" << carrierRec.eTicketInfo.get() << "'" << std::endl;
    }

    currentRecord->creationTime.assign(record.data() + 190, record.data() + 194);
    //std::cout << "Data set creation time(hours/minutes) '" << carrierRec.creationTime << "'" << std::endl;


    if (!ParseSerialNumber(currentRecord->serialNum)) {
        err = SERIAL_NUMBER_ERROR;
        return false;
    }

    return true;
}

// Recort Type 3
bool OagParser::ParseFlightLegRecord()
{
    if (!currentRecord) {
        std::cerr << "The record type 2 hasn't been read yet" << std::endl;
    }

    if (!needContinue()) {
        return true;
    }

    FlightLegRecord flg;

    std::string value = StrUtils::trim(std::string(record.data() + 1, record.data() + 2));
    if (!value.empty()) {
        flg.opSuffix = value;
    }

    flg.airline.assign(record.data() + 2, record.data() + 5);
    if (flg.airline != currentRecord->airline) {
        LogTrace(TRACE5) << "type2.airline=" << currentRecord->airline << " type3.airline=" << flg.airline;
        err = AIRLINE_RECORDS_EQUAL_ERROR;
        return false;
    }
    try {
        std::string flightNumStr(record.data() + 5, record.data() + 9);
        flg.flightNum = boost::lexical_cast<std::size_t>(StrUtils::ltrim(flightNumStr));
        //std::cout << "Flight Number '" << flightNumStr << "'" << std::endl;
    } catch (boost::bad_lexical_cast&) {
        err = FLIGHT_NUMBER_ERROR;
        return false;
    }

    try {
        std::string itinStr(record.data() + 9, record.data() + 11);
        flg.itinVarId = boost::lexical_cast<std::size_t>(itinStr);
        //std::cout << "Itinerary Variation Identifier '" << itinStr << "'" << std::endl;
    } catch (boost::bad_lexical_cast&) {
        err = ITINERARY_ID_ERROR;
        return false;
    }

    try {
        std::string legSeqStr(record.data() + 11, record.data() + 13);
        flg.legSeqNum = boost::lexical_cast<std::size_t>(legSeqStr);
        //std::cout << "Leg Sequence Number '" << legSeqStr << "'" << std::endl;
    } catch (boost::bad_lexical_cast&) {
        err = LEG_SEQUENCE_NUMBER_ERROR;
        return false;
    }

    flg.serviceType = record[13];
    //std::cout << "Service type '" << flightLegRec.serviceType << "'" << std::endl;

    flg.opPeriodFrom.assign(record.data() + 14, record.data() + 21);
    //std::cout << "Operation Period From '" << flightLegRec.opPeriodFrom << "'" << std::endl;

    flg.opPeriodTo.assign(record.data() + 21, record.data() + 28);
    //std::cout << "Operation Period To '" << flightLegRec.opPeriodTo << "'" << std::endl;

    flg.opDays = StrUtils::delSpaces(std::string(record.data() + 28, record.data() + 35));
    //std::cout << "Day(s) of Operation '" << flightLeg->opDays << "'" << std::endl;

    flg.biweekly = (record[35] == '2');

    flg.departureStation.assign(record.data() + 36, record.data() + 39);
    //std::cout << "Departure Station '" << flightLegRec.departureStation << "'" << std::endl;

    flg.schPassDepTime.assign(record.data() + 39, record.data() + 43);
    //std::cout << "Scheduled Time of Passenger Departure '" << flightLegRec.schPassDepTime << "'" << std::endl;

    flg.schAirDepTime.assign(record.data() + 43, record.data() + 47);
    //std::cout << "Scheduled Time of Aircraft Departure '" << flightLegRec.schAirDepTime << "'" << std::endl;

    flg.timeVariationDep.assign(record.data() + 47, record.data() + 52);
    //std::cout << "UTC/Local Time Variation Departure '" << flightLeg->timeVariationDep << "'" << std::endl;

    value = StrUtils::rtrim(std::string(record.data() + 52, record.data() + 54));
    if (!value.empty()) {
        airportTerminals[flg.departureStation].insert(value);
        flg.depPassTerminal = value;
        //std::cout << "Passenger Terminal Departure '" << value << "'" << std::endl;
    }

    flg.arrivalStation.assign(record.data() + 54, record.data() + 57);
    //std::cout << "Arrival Station '" << flightLegRec.arrivalStation << "'" << std::endl;

    flg.schAirArrTime.assign(record.data() + 57, record.data() + 61);
    //std::cout << "Scheduled Time of Aircraft Arrival '" << flightLegRec.schAirArrTime << "'" << std::endl;

    flg.schPassArrTime.assign(record.data() + 61, record.data() + 65);
    //std::cout << "Scheduled Time of Passenger Arrival '" << flightLegRec.schAirDepTime << "'" << std::endl;

    flg.timeVariationArr.assign(record.data() + 65, record.data() + 70);
    //std::cout << "UTC/Local Time Veriation Arrival '" << flightLegRec.timeVariationArr << "'" << std::endl;

    value = StrUtils::rtrim(std::string(record.data() + 70, record.data() + 72));
    if (!value.empty()) {
        flg.arrPassTerminal = value;
        airportTerminals[flg.arrivalStation].insert(value);
        //std::cout << "Passenger Terminal Arrival '" << value << "'" << std::endl;
    }

    flg.airType.assign(record.data() + 72, record.data() + 75);
    if (!flg.airType.empty()) {
        aircrafts.insert(flg.airType);
    }
    //std::cout << "Aircraft Type '" << flightLegRec.airType << "'" << std::endl;

    value = StrUtils::trim(std::string(record.data() + 75, record.data() + 95));
    if (!value.empty()) {
        flg.passReservBookDes = StrUtils::trim(value);
        //std::cout << "Passenger Reservations Booking Designator '" << value << "'" << std::endl;
    }

    value = StrUtils::trim(std::string(record.data() + 95, record.data() + 100));
    if (!value.empty()) {
        flg.passReservBookModif = value;
        //std::cout << "Passenger Reservations Booking Modifier '" << value << "'" << std::endl;
    }

    flg.mealServNote = StrUtils::rtrim(
        std::string(record.data() + 100, record.data() + 110)
    );
    //std::cout << "Meal Service Note '" << flightLegRec.mealServNote << "'" << std::endl;

    value = StrUtils::trim(std::string(record.data() + 110, record.data() + 119));
    if (!value.empty()) {
        flg.opAirDesignators = value;
        //std::cout << "Join Operation Airline Designators '" << value << "'" << std::endl;
    }

    //Secure Flight Indicator
    flg.secureFlight = record[121];

    flg.aircraftOwner = StrUtils::trim(std::string(record.data() + 128, record.data() + 131));

    //Cockpit Crew Employer [132 - 134]
    //Cabin Crew Employer [135 - 137]
    //Onward Flight [138 - 146]

    if (record[148] != ' ') {
        flg.airDiscloseCsh = record[148];
    }

    //Traffic Restriction Code [150 - 160]
    flg.traffRestrCode.assign(record.data() + 149, record.data() + 160);
    //Traffic Restriction Code Leg Overflow Indicator (when more than 11 legs)
    flg.traffRestrOverflow = (record[160] == 'Z');

    value = StrUtils::trim(std::string(record.data() + 172, record.data() + 192));
    if (!value.empty()) {
        flg.airConfig = value;
        //std::cout << "Aircraft Configuration/Version '" << value << "'" << std::endl;
    }

    //Date Variation [193 - 194]
    if (record[192] != ' ' && record[192] != '0') {
        flg.depDateVar = record[192];
    }
    if (record[193] != ' ' && record[193] != '0') {
        flg.arrDateVar = record[193];
    }

    if (!ParseSerialNumber(flg.serialNum)) {
        err = SERIAL_NUMBER_ERROR;
        return false;
    }

    if (!currentRecord->flightLegs.empty()) {
        if (flg.legSeqNum - currentRecord->flightLegs.back().legSeqNum != 1) {
            //another variation started - run processing of previous variation
            applyHandleFilter();
            if (!suffixVariations.empty() && suffixVariations.front().front().flightNum != flg.flightNum) {
                applyHandleSuffix();
            }
        }
    }

    if (!flg.passReservBookDes || *flg.passReservBookDes != "XX") {
        LogTrace(TRACE5) << "SALECONF: " << StrUtils::trim(flg.airline) << "-" << flg.flightNum << " "
                         << flg.airType << "-"
                         << (flg.airConfig ? *flg.airConfig : "noAcv")
                         << "-"
                         << (flg.passReservBookDes ? *flg.passReservBookDes : "noPrbd");
    }

    currentRecord->flightLegs.emplace_back(std::move(flg));
    return true;
}

// Recort Type 4
bool OagParser::ParseSegmentDataRecord()
{
    if (!currentRecord) {
        std::cerr << "The record type 2 hasn't been read yet" << std::endl;
        return false;
    }
    if (!needContinue()) {
        return true;
    }
    if (currentRecord->flightLegs.empty()) {
        std::cerr << "The record type 3 hasn't been read yet: line " << handledLine << "'" << std::endl;
        return false;
    }

    FlightLegRecord& flightLeg = *currentRecord->flightLegs.rbegin();

    std::string airline(record.data() + 2, record.data() + 5);
    if (airline != currentRecord->airline) {
        LogTrace(TRACE5) << "type2.airline=" << currentRecord->airline << " type4.airline=" << airline;
        err = AIRLINE_RECORDS_EQUAL_ERROR;
        return false;
    }

    std::size_t flightNum = 0;
    try {
        std::string flightNumStr(record.data() + 5, record.data() + 9);
        flightNum = boost::lexical_cast<std::size_t>(StrUtils::ltrim(flightNumStr));
    } catch (boost::bad_lexical_cast&) {
        err = FLIGHT_NUMBER_ERROR;
        return false;
    }
    if (flightNum != flightLeg.flightNum) {
        err = FLIGHT_NUMBER_ERROR;
        return false;
    }

    std::size_t itinId = 0;
    try {
        std::string itinStr(record.data() + 9, record.data() + 11);
        itinId = boost::lexical_cast<std::size_t>(itinStr);
    } catch (boost::bad_lexical_cast&) {
        err = ITINERARY_ID_ERROR;
        return false;
    }
    if (itinId != flightLeg.itinVarId) {
        err = ITINERARY_ID_ERROR;
        return false;
    }

    std::size_t legNum = 0;
    try {
        std::string legSeqStr(record.data() + 11, record.data() + 13);
        legNum = boost::lexical_cast<std::size_t>(legSeqStr);
    } catch (boost::bad_lexical_cast&) {
        err = LEG_SEQUENCE_NUMBER_ERROR;
        return false;
    }
    if (legNum != flightLeg.legSeqNum) {
        err = LEG_SEQUENCE_NUMBER_ERROR;
        return false;
    }

    const char serviceType = record[13];

    std::size_t deiCode = 0;
    try {
        std::string dateElIdent(record.data() + 30, record.data() + 33);
        deiCode = boost::lexical_cast<std::size_t>(dateElIdent);
    } catch (boost::bad_lexical_cast&) {
        err = DATA_ELEMENT_ID_ERROR;
        return false;
    }

    const DepArrPoints dap(std::string(record.data() + 33, record.data() + 36), std::string(record.data() + 36, record.data() + 39));
    const SegmentData sd(serviceType, dap, std::string(record.data() + 39, record.data() + 194));

    flightLeg.segData[deiCode].push_back(sd);

    if (deiCode == 106 && flightLeg.passReservBookDes && *flightLeg.passReservBookDes == "XX") {
        LogTrace(TRACE5) << "SALECONF: " << StrUtils::trim(flightLeg.airline) << "-" << flightLeg.flightNum << " "
                         << flightLeg.airType << "-"
                         << (flightLeg.airConfig ? *flightLeg.airConfig : "noAcv")
                         << "-"
                         << sd.data;
    }
    return true;
}

// Recort Type 5
bool OagParser::ParseTrailerRecord()
{
    applyHandleFilter();
    applyHandleSuffix();

    trailRecord.airline = StrUtils::rtrim(std::string(record.data() + 2, record.data() + 5));
    //std::cout << "Airline Designator '" << trailRecord.airline << "'" << std::endl;

    trailRecord.releaseDate = StrUtils::trim(std::string(record.data() + 5, record.data() + 12));

    try {
        std::string checkSerialNum(record.data() + 187, record.data() + 193);
        trailRecord.serialNumCheckRef = boost::lexical_cast<std::size_t>(checkSerialNum);
        //std::cout << "Serial Number Check Reference '" << checkSerialNum << "'" << std::endl;
    } catch (boost::bad_lexical_cast&) {
        err = CHECH_SERIAL_NUMBER_ERROR;
        return 0;
    }

    char code = record[193];
    if (code != 'C' && code != 'E') {
        err = CONTINUE_END_CODE_ERROR;
        return false;
    }
    trailRecord.code = code;
    //std::cout << "Continuation/End code '" << code << "'" << std::endl;

    if (!ParseSerialNumber(trailRecord.serialNum)) {
        err = SERIAL_NUMBER_ERROR;
        return false;
    }

    handler->handleCarrierProcEnd(trailRecord.airline, *currentRecord);

    return true;
}

bool OagParser::ParseSerialNumber(size_t& /*sNum*/)
{
//    std::string recordSerialNum(record.data() + 194, record.data() + 200);
//    try {
//        sNum = boost::lexical_cast<std::size_t>(recordSerialNum);
//        bool errorFlag = false;
//        if (currentSeriallNum == 999999) {
//            if (sNum != 2) {
//                errorFlag = true;
//            }
//        } else {
//            if (sNum - currentSeriallNum != 1) {
//                errorFlag = true;
//            }
//        }
//        if (errorFlag) {
//            std::cout << "New serial num '" << recordSerialNum << "' prev serial num '" << currentSeriallNum << std::endl;
//            return false;
//        }
//        currentSeriallNum = sNum;
//    } catch (boost::bad_lexical_cast&) {
//        std::cout << "New serial num '" << recordSerialNum << "' prev serial num '" << currentSeriallNum << std::endl;
//        return false;
//    }
    return true;
}

void OagParser::applyHandleFilter()
{
    if (handler && currentRecord && !currentRecord->flightLegs.empty()) {
        const bool shouldBeHandled = std::all_of(filters.begin(), filters.end(),
            std::bind(&OagFilter::applyFilter, std::placeholders::_1, std::cref(*currentRecord))
        );

        if (shouldBeHandled) {
            if (currentRecord->flightLegs.front().opSuffix) {
                //flight variation with suffix - should stash it for future processing
                //to avoid creation of partial SSM-SKD-NEW of non-suffix flight variations
                suffixVariations.push_back({});
                currentRecord->flightLegs.swap(suffixVariations.back());
            } else {
                handler->handleRecords(*currentRecord);
            }
        }
        currentRecord->flightLegs.clear();
    }
}

void OagParser::applyHandleSuffix()
{
    for (CarrierRecord::FlightLegs& legs : suffixVariations) {
        currentRecord->flightLegs.swap(legs);
        handler->handleRecords(*currentRecord);
        currentRecord->flightLegs.clear();
    }
    suffixVariations.clear();
}

bool OagParser::readLine()
{
    std::string line;
    if (std::getline(inputFile, line).eof()) {
        return false;
    }
    if (line.size() < 200) {
        throw std::logic_error("bad file format at line " + std::to_string(handledLine + 1));
    }
    std::copy(line.begin(), line.begin() + 200, record.begin());
    ++handledLine;
    return true;
}

bool OagParser::needContinue() const
{
    return std::all_of(filters.begin(), filters.end(),
        std::bind(&OagFilter::checkAirline, std::placeholders::_1, std::cref(*currentRecord))
    );
}

}
