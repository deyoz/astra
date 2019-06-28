#ifndef _EDI_ELEMENTS_H_
#define _EDI_ELEMENTS_H_

#include "date_time.h"
#include "ticket_types.h"
#include "astra_dates.h"
#include "astra_ticket.h"
#include "CheckinBaseTypes.h"

#include <etick/tick_doctype.h>
#include <etick/tick_data.h>

#include <string>
#include <boost/optional.hpp>
#include <stl_utils.h>


namespace edifact {

using BASIC::date_time::TDateTime;


///@class UnbElem - Interchange Header --UNB
struct UnbElem
{
    std::string m_senderCarrierCode;
    std::string m_recipientCarrierCode;

    UnbElem(const std::string& senderCC,
            const std::string& recipientCC)
        : m_senderCarrierCode(senderCC),
          m_recipientCarrierCode(recipientCC)
    {}
};

//---------------------------------------------------------------------------------------

///@class UngElem - Group Header --UNG
struct UngElem
{
    std::string m_msgGroupName;
    std::string m_senderName;
    std::string m_senderCarrierCode;
    std::string m_recipientName;
    std::string m_recipientCarrierCode;
    TDateTime m_prepareDateTime;
    std::string m_groupRefNum;
    std::string m_cntrlAgnCode;
    std::string m_msgVerNum;
    std::string m_msgRelNum;

    UngElem(const std::string& msgGroupName,
            const std::string& senderName,
            const std::string& senderCC,
            const std::string& recipientName,
            const std::string& recipientCC,
            const TDateTime& prepareDt,
            const std::string& groupRefNum,
            const std::string& cntrlAgnCode,
            const std::string& msgVerNum,
            const std::string& msgRelNum)
        : m_msgGroupName(msgGroupName),
          m_senderName(senderName),
          m_senderCarrierCode(senderCC),
          m_recipientName(recipientName),
          m_recipientCarrierCode(recipientCC),
          m_prepareDateTime(prepareDt),
          m_groupRefNum(groupRefNum),
          m_cntrlAgnCode(cntrlAgnCode),
          m_msgVerNum(msgVerNum),
          m_msgRelNum(msgRelNum)
    {}
};

//---------------------------------------------------------------------------------------

///@class UnhElem - Message Header --UNH
struct UnhElem
{
    enum SeqFlag
    {
        First,
        Middle,
        Last
    };

    std::string m_msgTypeId;
    std::string m_msgVerNum;
    std::string m_msgRelNum;
    std::string m_cntrlAgnCode;
    std::string m_assAccCode;
    unsigned m_seqNumber;
    SeqFlag m_seqFlag;

    UnhElem(const std::string& msgTypeId,
            const std::string& msgVerNum,
            const std::string& msgRelNum,
            const std::string& cntrlAgnCode,
            const std::string& assAccCode,
            unsigned seqNum,
            SeqFlag seqFlag)
        : m_msgTypeId(msgTypeId),
          m_msgVerNum(msgVerNum),
          m_msgRelNum(msgRelNum),
          m_cntrlAgnCode(cntrlAgnCode),
          m_assAccCode(assAccCode),
          m_seqNumber(seqNum),
          m_seqFlag(seqFlag)
    {}

    static std::string seqFlagToStr(SeqFlag flag)
    {
        if(flag == First)
            return "C";
        else if(flag == Last)
            return "F";
        return "";
    }
};

//---------------------------------------------------------------------------------------

///@class BgmElem - Beginning of Message --BGM
struct BgmElem
{
    std::string m_docCode;
    std::string m_docId;

    BgmElem(const std::string& docCode,
            const std::string& docId)
        : m_docCode(docCode),
          m_docId(docId)
    {}
};

//---------------------------------------------------------------------------------------

///@class NadElem - Name and Address
struct NadElem
{
    std::string m_funcCode;
    std::string m_partyName;
    std::string m_partyName2;
    std::string m_partyName3;
    std::string m_street;
    std::string m_city;
    std::string m_countrySubEntityCode;
    std::string m_postalCode;
    std::string m_country;

    NadElem(const std::string& funcCode,
            const std::string& partyName)
        : m_funcCode(funcCode),
          m_partyName(partyName)
    {}

    NadElem(const std::string& funcCode,
            const std::string& partyName,
            const std::string& partyName2,
            const std::string& partyName3,
            const std::string& street,
            const std::string& city,
            const std::string& countrySubEntityCode,
            const std::string& postalCode,
            const std::string& country)
        : m_funcCode(funcCode),
          m_partyName(partyName),
          m_partyName2(partyName2),
          m_partyName3(partyName3),
          m_street(street),
          m_city(city),
          m_countrySubEntityCode(countrySubEntityCode),
          m_postalCode(postalCode),
          m_country(country)
    {}

};

//---------------------------------------------------------------------------------------

///@class ComElem - Comminication Contact --COM
struct ComElem
{
    std::string m_phone;
    std::string m_fax;
    std::string m_email;

    ComElem(const std::string& phone,
            const std::string& fax,
            const std::string& email = "")
        : m_phone(phone),
          m_fax(fax),
          m_email(email)
    {}
};

//---------------------------------------------------------------------------------------

///@class TdtElem - Details of Transport --TDT
struct TdtElem
{
    std::string m_stageQuailifier;
    std::string m_journeyId;
    std::string m_carrierId;

    TdtElem(const std::string& stageQualifier,
            const std::string& journeyId,
            const std::string& carrierId)
        : m_stageQuailifier(stageQualifier),
          m_journeyId(journeyId),
          m_carrierId(carrierId)
    {}
};

//---------------------------------------------------------------------------------------

///@class LocElem - Place/Location Identification --LOC
struct LocElem
{
    enum LocQualifier
    {
        Departure = 125,
        Arrival = 87,
        CustomsAndBorderProtection = 22,
        StartJourney = 178,
        FinishJourney = 179,
        CountryOfResidence = 174,
        CountryOfBirth = 180,
        DocCountry = 91,
        Next = 92,
    };

    LocQualifier m_qualifier;
    std::string  m_location;
    std::string  m_relatedLocation1;
    std::string  m_relatedLocation2;

    LocElem(LocQualifier qualifier,
            const std::string& location,
            const std::string& relatedLocation1 = "",
            const std::string& relatedLocation2 = "");

    LocElem(const std::string& qualifier,
            const std::string& location,
            const std::string& relatedLocation1 = "",
            const std::string& relatedLocation2 = "");

    static LocQualifier qualifierFromStr(const std::string& str);
};

//---------------------------------------------------------------------------------------

///@class DtmElem - Date/Time/Period --DTM
struct DtmElem
{
    enum DtmQualifier
    {
        Departure = 189,
        Arrival = 232,
        DocExpireDate = 36,
        DocIssueDate = 182,
        DateOfBirth = 329
    };

    DtmQualifier m_qualifier;
    TDateTime    m_dateTime;
    std::string  m_formatCode;

    DtmElem(DtmQualifier qualifier,
            const TDateTime& dateTime,
            const std::string& formatCode = "");

    DtmElem(const std::string& qualifier,
            const std::string& dateTime,
            const std::string& formatCode);

    static DtmQualifier qualifierFromStr(const std::string& str);
};

//---------------------------------------------------------------------------------------

///@class AttElem - Attribute --ATT
struct AttElem
{
    std::string m_funcCode;
    std::string m_value;

    AttElem(const std::string& funcCode,
            const std::string& value)
        : m_funcCode(funcCode),
          m_value(value)
    {}
};

//---------------------------------------------------------------------------------------

///@class MeaElem - Attribute --MEA
struct MeaElem
{
    enum MeaQualifier
    {
        BagCount,
        BagWeight
    };

    MeaQualifier m_meaQualifier;
    int m_mea;

    MeaElem(const MeaQualifier meaQualifier,
            const int mea)
        : m_meaQualifier(meaQualifier),
          m_mea(mea)
    {}

    static std::string meaQualifierToStr(MeaQualifier qualifier)
    {
        if(qualifier == BagCount)
            return "CT";
        return "WT";
    }
};

//---------------------------------------------------------------------------------------

///@class FtxElem - Free Text --FTX
struct FtxElem
{
    std::string m_subjectCode;
    std::string m_freeText;

    FtxElem(const std::string& subjectCode,
            const std::string& freeText)
        : m_subjectCode(subjectCode),
          m_freeText(freeText)
    {}
};

//---------------------------------------------------------------------------------------

///@class ErpElem - Error point detailt --ERP
struct ErpElem
{
    std::string m_msgSectionCode;

    ErpElem(const std::string& msgSectionCode)
        : m_msgSectionCode(msgSectionCode)
    {}
};

//---------------------------------------------------------------------------------------

///@class ErcElem - Application error information --ERC
struct ErcElem
{
    std::string m_errorCode;

    ErcElem(const std::string& errorCode)
        : m_errorCode(errorCode)
    {}
};

//---------------------------------------------------------------------------------------

///@class Ftx2Elem - Free Text --FTX
struct Ftx2Elem
{
  std::string m_qualifier;
  std::string m_str1;
  std::string m_str2;

  Ftx2Elem(const std::string &qualifier,
           const std::string &str1,
           const std::string &str2)
    : m_qualifier(upperc(qualifier.substr(0, 3))),
      m_str1(upperc(str1.substr(0, 512))),
      m_str2(upperc(str2.substr(0, 512)))
  {}
};

//---------------------------------------------------------------------------------------

///@class NatElem - Nationality --NAT
struct NatElem
{
    std::string m_natQualifier;
    std::string m_nat;

    NatElem(const std::string& natQualifier,
             const std::string& nat)
        : m_natQualifier(natQualifier),
          m_nat(nat)
    {}
};

//---------------------------------------------------------------------------------------

///@class RffElem - Reference --RFF
struct RffElem
{
    std::string m_qualifier;
    std::string m_ref;

    RffElem(const std::string& rffQualifier,
            const std::string& ref)
        : m_qualifier(rffQualifier),
          m_ref(ref)
    {}
};

//---------------------------------------------------------------------------------------

///@class DocElem - Document/Message Details --DOC
struct DocElem
{
    std::string m_docCode;
    std::string m_docNum;
    std::string m_respAgnCode;
    std::string m_idCode;

    DocElem(const std::string& docCode,
            const std::string& docNum,
            const std::string& respAgnCode,
            const std::string& idCode = "110")
        : m_docCode(docCode),
          m_docNum(docNum),
          m_respAgnCode(respAgnCode),
          m_idCode(idCode)
    {}
};

//---------------------------------------------------------------------------------------

///@class CntElem - Control Total --CNT
struct CntElem
{
    enum CntType
    {
        CrewTotal = 41,
        PassengersTotal = 42
    };

    CntType m_cntType;
    unsigned m_cnt;

    CntElem(CntType cntType, unsigned cnt)
        : m_cntType(cntType),
          m_cnt(cnt)
    {}
};

//---------------------------------------------------------------------------------------

///@class UneElem - Group Trailer --UNE
struct UneElem
{
    std::string m_refNum;
    unsigned m_cntrlCnt;

    UneElem(const std::string& refNum,
            unsigned cntrlCnt = 1)
        : m_refNum(refNum),
          m_cntrlCnt(cntrlCnt)
    {}
};

//---------------------------------------------------------------------------------------

///@class PtkElem - Ppricing/Ticketing Details --PTK
struct PtkElem
{
    Dates::date m_issueDate;
};

//---------------------------------------------------------------------------------------

///@class PtsElem - Pricing/Ticketing Subsequent --PTS
struct PtsElem
{
    std::string m_fareBasis;
    std::string m_rfic;
    std::string m_rfisc;
    int         m_itemNumber;

    PtsElem()
        : m_itemNumber(0)
    {}
};

//---------------------------------------------------------------------------------------

///@class RciElem - Reservation Control Information --RCI
struct RciElem
{
    std::string m_airline;
    std::string m_recloc;
    std::string m_type;

    RciElem(const std::string& airline, const std::string& recloc, const std::string& type = "1")
        : m_airline(airline), m_recloc(recloc), m_type(type)
    {
        if(m_type.empty())
            m_type = "1";
    }
};

//---------------------------------------------------------------------------------------

///@class MonElem - Monetary Information --MON
struct MonElem
{
    Ticketing::AmountCode m_code;
    bool m_addCollect;
    std::string m_currency;
    std::string m_value;
    MonElem()
        : m_addCollect(false)
    {}
};

//---------------------------------------------------------------------------------------

///@class FtiElem - Frequent Traveller Information --FTI
struct FtiElem
{
    std::string m_airline;
    std::string m_fqtvIdCode;
    std::string m_passRefNum;
    std::string m_statusCode;

    FtiElem(const std::string& airline, const std::string& fqtvIdCode)
        : m_airline(airline), m_fqtvIdCode(fqtvIdCode)
    {}
};

//---------------------------------------------------------------------------------------

///@class EbdElem - Excess Baggage Details --EBD
struct EbdElem {
    int         m_quantity;
    std::string m_charge;
    std::string m_measure;

    EbdElem(int quantity, const std::string& charge, const std::string& measure)
        : m_quantity(quantity), m_charge(charge), m_measure(measure)
    {}
};

//---------------------------------------------------------------------------------------

///@class EqnElem - Number Of Units --EQN
struct EqnElem {
    unsigned    m_numberOfUnits;
    std::string m_qualifier;

    EqnElem(int nof, const std::string& qualifier)
        : m_numberOfUnits(nof), m_qualifier(qualifier)
    {}
};

//---------------------------------------------------------------------------------------

///@class TvlElem - Travel Product Information --TVL
struct TvlElem
{
    Dates::Date_t          m_depDate;
    Dates::time_duration   m_depTime;
    Dates::Date_t          m_arrDate;
    Dates::time_duration   m_arrTime;
    std::string            m_depPoint;
    std::string            m_arrPoint;
    std::string            m_airline;  // string ?
    std::string            m_operAirline;
    Ticketing::FlightNum_t m_flNum;
    Ticketing::FlightNum_t m_operFlNum;

    TvlElem()
        : m_depTime(Dates::not_a_date_time),
          m_arrTime(Dates::not_a_date_time)
    {}
};

//---------------------------------------------------------------------------------------

///@class TktElem - Ticket Number Details --TKT
struct TktElem
{
    Ticketing::DocType           m_docType;
    Ticketing::TicketNum_t       m_ticketNum;
    boost::optional<int>         m_nBooklets;
    boost::optional<int>         m_conjunctionNum;
    boost::optional<Ticketing::TicketNum_t> m_inConnectionTicketNum;
    boost::optional<Ticketing::TickStatAction::TickStatAction_t> m_tickStatAction;
};

//---------------------------------------------------------------------------------------

///@class CpnElem - Coupon Information --CPN
struct CpnElem
{
    Ticketing::CouponNum_t m_num;
    Ticketing::TicketMedia m_media;
    Ticketing::TaxAmount::Amount m_amount;
    Ticketing::CouponStatus m_status;
    std::string m_sac;
    Ticketing::CouponStatus m_prevStatus;
    std::string m_action;
    Ticketing::CouponNum_t m_connectedNum;
};

//---------------------------------------------------------------------------------------

///@class RciElements
struct RciElements
{
    std::list<RciElem>  m_lReclocs;
    std::list<Ticketing::FormOfId> m_lFoid;

    RciElements operator+(const RciElements &other) const;
};

//---------------------------------------------------------------------------------------

///@class MonElements
struct MonElements
{
    std::list<MonElem> m_lMon;

    MonElements operator+(const MonElements &other) const;
};

//---------------------------------------------------------------------------------------

///@class IftElements
struct IftElements
{
    std::list<Ticketing::FreeTextInfo> m_lIft;

    IftElements operator+(const IftElements &other) const;
};

//---------------------------------------------------------------------------------------

///@class TxdElements
struct TxdElements
{
    std::list<Ticketing::TaxDetails> m_lTax;

    TxdElements operator+(const TxdElements &other) const;
};

//---------------------------------------------------------------------------------------

///@class FopElements
struct FopElements
{
    std::list<Ticketing::FormOfPayment> m_lFop;

    FopElements operator+(const FopElements &other) const;
};

//---------------------------------------------------------------------------------------

std::ostream& operator<<(std::ostream &os, const TktElem &tkt);
std::ostream& operator<<(std::ostream &os, const CpnElem &cpn);

//---------------------------------------------------------------------------------------

///@class LorElem - Location/Originator details --LOR
struct LorElem
{
    std::string m_airline;
    std::string m_port;
};

//---------------------------------------------------------------------------------------

///@class FdqElem - Flight details query --FDQ
struct FdqElem
{
    std::string m_outbAirl;
    Ticketing::FlightNum_t m_outbFlNum;
    Dates::Date_t m_outbDepDate;
    Dates::time_duration m_outbDepTime;
    std::string m_outbDepPoint;
    std::string m_outbArrPoint;

    std::string m_inbAirl;
    Ticketing::FlightNum_t m_inbFlNum;
    Dates::Date_t m_inbDepDate;
    Dates::time_duration m_inbDepTime;
    Dates::Date_t m_inbArrDate;
    Dates::time_duration m_inbArrTime;
    std::string m_inbDepPoint;
    std::string m_inbArrPoint;
    std::string m_flIndicator;

    FdqElem()
        : m_outbDepDate(Dates::not_a_date_time),
          m_outbDepTime(Dates::not_a_date_time),
          m_inbDepDate(Dates::not_a_date_time),
          m_inbDepTime(Dates::not_a_date_time),
          m_inbArrDate(Dates::not_a_date_time),
          m_inbArrTime(Dates::not_a_date_time)
    {}
};

//---------------------------------------------------------------------------------------

///@class PpdElem - Passenger personal details --PPD
struct PpdElem
{
    std::string m_passSurname;
    std::string m_passName;
    std::string m_passType;
    std::string m_withInftIndicator;
    std::string m_passRespRef;
    std::string m_passQryRef;
    std::string m_inftSurname;
    std::string m_inftName;
    std::string m_inftRespRef;
    std::string m_inftQryRef;
};

//---------------------------------------------------------------------------------------

///@class PrdElem - Passenger reservation details --PRD
struct PrdElem
{
    std::string m_rbd;
};

//---------------------------------------------------------------------------------------

///@class PsdElem - Passenger seat request details --PSD
struct PsdElem
{
    std::string m_seat;
    std::string m_noSmokingInd;
    std::string m_characteristic; // TODO
};

//---------------------------------------------------------------------------------------

///@class PbdElem - Passenger baggage information --PBD
struct PbdElem
{
    struct Bag
    {
        unsigned m_numOfPieces;
        unsigned m_weight;

        Bag()
            : m_numOfPieces(0), m_weight(0)
        {}
    };

    struct Tag
    {
        std::string m_carrierCode;
        unsigned    m_tagNum;
        unsigned    m_qtty;
        std::string m_dest;
        unsigned    m_accode;

        Tag()
            : m_tagNum(0), m_qtty(0), m_accode(0)
        {}
    };

    boost::optional<Bag> m_bag;
    boost::optional<Bag> m_handBag;
    std::list<Tag>       m_tags;
};

//---------------------------------------------------------------------------------------

///@class PsiElem - Passenger service information --PSI
struct PsiElem
{
    struct SsrDetails
    {
        std::string m_ssrCode;
        std::string m_airline;
        std::string m_ssrText;
        unsigned    m_age;
        unsigned    m_numOfPieces;
        unsigned    m_weight;
        std::string m_freeText;
        std::string m_qualifier;

        SsrDetails()
            : m_age(0), m_numOfPieces(0), m_weight(0)
        {}
    };

    std::string           m_osi;
    std::list<SsrDetails> m_lSsr;
};


//---------------------------------------------------------------------------------------

///@class FdrElem - Flight details response --FDR
struct FdrElem
{
    std::string m_airl;
    Ticketing::FlightNum_t m_flNum;
    Dates::Date_t m_depDate;
    Dates::time_duration m_depTime;
    Dates::Date_t m_arrDate;
    Dates::time_duration m_arrTime;
    std::string m_depPoint;
    std::string m_arrPoint;

    FdrElem()
        : m_depDate(Dates::not_a_date_time),
          m_depTime(Dates::not_a_date_time),
          m_arrDate(Dates::not_a_date_time),
          m_arrTime(Dates::not_a_date_time)
    {}
};

//---------------------------------------------------------------------------------------

///@class RadElem - Response analysis details --RAD
struct RadElem
{
    std::string m_respType;
    std::string m_status;
};

//---------------------------------------------------------------------------------------

///@class PfdElem - Passenger flight details --PFD
struct PfdElem
{
    std::string m_seat;
    std::string m_noSmokingInd;
    std::string m_cabinClass;
    std::string m_regNo;
    std::string m_infantRegNo;
};

//---------------------------------------------------------------------------------------

///@class ChdElem - Cascading host details --CHD
struct ChdElem
{
    std::string m_origAirline;
    std::string m_origPoint;
    std::string m_outbAirline;
    Ticketing::FlightNum_t m_outbFlNum;
    boost::gregorian::date m_depDate;
    std::string m_depPoint;
    std::string m_arrPoint;
    std::string m_outbFlContinIndic;

    std::list<std::string> m_hostAirlines;
};

//---------------------------------------------------------------------------------------

///@class DmcElem - Default message characteristics --DMC
struct DmcElem
{
    std::string m_maxNumRespFlights;
};

//---------------------------------------------------------------------------------------

///@class FsdElem - Flight segment details --FSD
struct FsdElem
{
    boost::posix_time::time_duration m_boardingTime;
    std::string                      m_gate;

    FsdElem()
        : m_boardingTime(boost::posix_time::not_a_date_time)
    {}
};

//---------------------------------------------------------------------------------------

///@class ErdElem - Error information --ERD
struct ErdElem
{
    std::string m_level;
    std::string m_messageNumber;
    std::string m_messageText;

    ErdElem()
    {}
    ErdElem(const std::string& level, const std::string& msgNum,
            const std::string& msgText = "")
        : m_level(level), m_messageNumber(msgNum),
          m_messageText(msgText)
    {}
};

//---------------------------------------------------------------------------------------

///@class SpdElem - Select personal details --SPD
struct SpdElem
{
    std::string m_passSurname;
    std::string m_passName;
    std::string m_rbd;
    std::string m_passSeat;
    std::string m_passRespRef;
    std::string m_passQryRef;
    std::string m_securityId;
    std::string m_recloc;
    std::string m_tickNum;
};

//---------------------------------------------------------------------------------------

///@class UpdElem - Update personal details --UPD
struct UpdElem
{
    std::string m_actionCode;
    std::string m_surname;
    std::string m_name;
    std::string m_withInftIndicator;
    std::string m_passQryRef;
    std::string m_inftSurname;
    std::string m_inftName;
    std::string m_inftQryRef;
};

//---------------------------------------------------------------------------------------

///@class UsdElem - Update seat request details --USD
struct UsdElem
{
    std::string m_actionCode;
    std::string m_seat;
    std::string m_noSmokingInd;
};

//---------------------------------------------------------------------------------------

///@class UbdElem - Update baggage details --UBD
struct UbdElem
{
    struct Bag
    {
        std::string m_actionCode;
        unsigned    m_numOfPieces;
        unsigned    m_weight;

        Bag()
            : m_numOfPieces(0), m_weight(0)
        {}
    };

    struct Tag
    {
        std::string m_actionCode;
        std::string m_carrierCode;
        unsigned    m_tagNum;
        unsigned    m_qtty;
        std::string m_dest;
        unsigned    m_accode;

        Tag()
            : m_tagNum(0), m_qtty(0), m_accode(0)
        {}
    };

    boost::optional<Bag> m_bag;
    boost::optional<Bag> m_handBag;
    std::list<Tag>       m_tags;
};

//---------------------------------------------------------------------------------------

///@class WadElem - Warning information --WAD
struct WadElem
{
    std::string m_level;
    std::string m_messageNumber;
    std::string m_messageText;
};

//---------------------------------------------------------------------------------------

///@class SrpElem - Seat request parameters --SRP
struct SrpElem
{
    std::string m_cabinClass;
    std::string m_noSmokingInd;
};

//---------------------------------------------------------------------------------------

///@class EqdElem - Equipment information --EQD
struct EqdElem
{
    std::string m_equipment;
};

//---------------------------------------------------------------------------------------

///@class CbdElem - Cabin details --CBD
struct CbdElem
{
    struct SeatColumn
    {
        std::string m_col;
        std::string m_desc1;
        std::string m_desc2;

        SeatColumn()
        {}
        SeatColumn(const std::string& column,
                   const std::string& desc1,
                   const std::string& desc2 = "")
            : m_col(column), m_desc1(desc1), m_desc2(desc2)
        {}
    };

    std::string m_cabinClass;
    unsigned m_firstClassRow;
    unsigned m_lastClassRow;
    std::string m_deck;
    unsigned m_firstSmokingRow;
    unsigned m_lastSmokingRow;
    std::string m_seatOccupDefIndic;
    unsigned m_firstOverwingRow;
    unsigned m_lastOverwingRow;
    std::list<SeatColumn> m_lSeatColumns;

    CbdElem()
        : m_firstClassRow(0), m_lastClassRow(0),
          m_firstSmokingRow(0), m_lastSmokingRow(0),
          m_firstOverwingRow(0), m_lastOverwingRow(0)
    {}
};

//---------------------------------------------------------------------------------------

///@class RodElem - Row details --ROD
struct RodElem
{
    struct SeatOccupation
    {
        std::string m_col;
        std::string m_occup;
        std::list<std::string> m_lCharacteristics;

        SeatOccupation()
        {}
        SeatOccupation(const std::string& col,
                       const std::string& occup,
                       const std::list<std::string>& lCharacteristics = std::list<std::string>())
            : m_col(col), m_occup(occup),
              m_lCharacteristics(lCharacteristics)
        {}
    };

    std::string m_row;
    std::string m_characteristic;
    std::list<SeatOccupation> m_lSeatOccupations;
};

//---------------------------------------------------------------------------------------


///@class PapElem - Passenger API/DOT information --PAP
struct PapElem
{
    struct PapDoc
    {
        std::string   m_docQualifier;
        std::string   m_docNumber;
        std::string   m_placeOfIssue;
        std::string   m_freeText;
        Dates::Date_t m_expiryDate;
        std::string   m_gender;
        std::string   m_cityOfIssue;
        Dates::Date_t m_issueDate;
        std::string   m_surname;
        std::string   m_name;
        std::string   m_otherName;
    };

    std::string       m_type;
    Dates::Date_t     m_birthDate;
    std::string       m_nationality;
    std::string       m_surname;
    std::string       m_name;
    std::string       m_otherName;
    std::list<PapDoc> m_docs;

    boost::optional<PapDoc> findVisa() const;
    boost::optional<PapDoc> findDoc() const;
};

//---------------------------------------------------------------------------------------

///@class AddElem - Address Details --ADD
struct AddElem
{
    std::string m_actionCode;

    struct Address
    {
        std::string m_purposeCode;
        std::string m_address;
        std::string m_city;
        std::string m_region;
        std::string m_country;
        std::string m_postalCode;
    };

    std::list<Address> m_lAddr;
};

//---------------------------------------------------------------------------------------

///@class UapElem - Update API information --UAP
struct UapElem: public PapElem
{
    std::string m_actionCode;

    bool isGroupHeader() const { return m_actionCode.empty(); }
};

//---------------------------------------------------------------------------------------

///@class UsiElem - Update service information --USI
struct UsiElem
{
    struct UpdSsrDetails: public PsiElem::SsrDetails
    {
        std::string m_actionCode;

        UpdSsrDetails()
            : PsiElem::SsrDetails()
        {}
    };

    std::list<UpdSsrDetails> m_lSsr;
};

//---------------------------------------------------------------------------------------

std::ostream& operator<<(std::ostream &os, const LorElem &lor);
std::ostream& operator<<(std::ostream &os, const FdqElem &fdq);
std::ostream& operator<<(std::ostream &os, const PpdElem &ppd);
std::ostream& operator<<(std::ostream &os, const PrdElem &prd);
std::ostream& operator<<(std::ostream &os, const PsdElem &psd);
std::ostream& operator<<(std::ostream &os, const PbdElem &pbd);
std::ostream& operator<<(std::ostream &os, const PsiElem &psi);
std::ostream& operator<<(std::ostream &os, const FdrElem &fdr);
std::ostream& operator<<(std::ostream &os, const RadElem &rad);
std::ostream& operator<<(std::ostream &os, const PfdElem &pfd);
std::ostream& operator<<(std::ostream &os, const ChdElem &chd);
std::ostream& operator<<(std::ostream &os, const DmcElem &dmc);
std::ostream& operator<<(std::ostream &os, const FsdElem &fsd);
std::ostream& operator<<(std::ostream &os, const ErdElem &erd);
std::ostream& operator<<(std::ostream &os, const SpdElem &spd);
std::ostream& operator<<(std::ostream &os, const UpdElem &upd);
std::ostream& operator<<(std::ostream &os, const UsdElem &usd);
std::ostream& operator<<(std::ostream &os, const UbdElem &ubd);
std::ostream& operator<<(std::ostream &os, const WadElem &wad);
std::ostream& operator<<(std::ostream &os, const SrpElem &srp);
std::ostream& operator<<(std::ostream &os, const EqdElem &eqd);
std::ostream& operator<<(std::ostream &os, const CbdElem &cbd);
std::ostream& operator<<(std::ostream &os, const RodElem &rod);
std::ostream& operator<<(std::ostream &os, const PapElem &pap);
std::ostream& operator<<(std::ostream &os, const AddElem &add);
std::ostream& operator<<(std::ostream &os, const UapElem &uap);
std::ostream& operator<<(std::ostream &os, const UsiElem &usi);
std::ostream& operator<<(std::ostream &os, const BgmElem &bgm);
std::ostream& operator<<(std::ostream &os, const RffElem &rff);
std::ostream& operator<<(std::ostream &os, const DtmElem &dtm);
std::ostream& operator<<(std::ostream &os, const LocElem &loc);
std::ostream& operator<<(std::ostream &os, const ErcElem &erc);
std::ostream& operator<<(std::ostream &os, const FtxElem &ftx);
std::ostream& operator<<(std::ostream &os, const ErpElem &erp);

}//namespace edifact

#endif/*_EDI_ELEMENTS_H_*/
