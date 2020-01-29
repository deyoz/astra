#pragma once

#include <array>

#include "oag_records.h"
#include "oag_records_handler.h"
#include "oag_filter.h"

#include <array>
#include <list>
#include <set>
#include <map>
#include <string>
#include <fstream>

namespace Oag {

typedef std::set<std::string> StringSet;
typedef std::map<std::string, StringSet> StringListMap;

class OagRecordsHandler;

class OagParser
{
public:

    typedef enum {
        NO_ERROR = 0,
        FILE_OPENED_ERROR,
        RECORD_TYPE_ERROR,
        TITLE_CONTENT_ERROR,
        DATA_SERIAL_NUMBER_ERROR,
        TIME_MODE_ERROR,
        SCHEDULE_STATUS_ERROR,
        SERIAL_NUMBER_ERROR,
        AIRLINE_RECORDS_EQUAL_ERROR,
        FLIGHT_NUMBER_ERROR,
        ITINERARY_ID_ERROR,
        LEG_SEQUENCE_NUMBER_ERROR,
        DATA_ELEMENT_ID_ERROR,
        CHECH_SERIAL_NUMBER_ERROR,
        CONTINUE_END_CODE_ERROR,
    } ParserError;

public:
    OagParser(const std::string& oagFile);
    ~OagParser();

    bool ParseFile();

    std::size_t error() const { return err; }
    std::string getStringError() const;

    void setRecordsHandler(OagRecordsHandler::OagRecordsHandlerPtr);

    void addRecordsFilter(OagFilter::OagFilterPtr);

private:

    typedef enum {NULL_RECORD = 0,
                  HEADER_RECORD = 1,
                  CARRIER_RECORD,
                  FLIGHT_LEG_RECORD,
                  SEGMENT_DATA_RECORD,
                  TRAILER_RECORD} RecordType;

    bool ParseHeaderRecord();
    bool ParseCarrierRecord();
    bool ParseFlightLegRecord();
    bool ParseSegmentDataRecord();
    bool ParseTrailerRecord();

    bool ParseSerialNumber(size_t&);

    void applyHandleFilter();
    void applyHandleSuffix();

    bool readLine();

    bool needContinue() const;

    std::ifstream inputFile;
    std::size_t err;

    typedef std::array<char, 200> Record;
    Record record;

    RecordType type;
//    std::size_t currentSeriallNum;
    std::size_t handledLine;
    std::unique_ptr<CarrierRecord> currentRecord;

    std::vector<CarrierRecord::FlightLegs> suffixVariations;
    TrailerRecord trailRecord;

    OagRecordsHandler::OagRecordsHandlerPtr handler;
    std::list<OagFilter::OagFilterPtr> filters;

    StringListMap airportTerminals;
    StringSet aircrafts;
    bool parseCompany;

    friend std::ostream& operator<<(std::ostream& os, const Record& record);
    friend std::istream& operator>>(std::istream& is, Record& record);
};

}
