#ifndef _EDI_ELEMENTS_H_
#define _EDI_ELEMENTS_H_

#include "basic.h"
#include "ticket_types.h"
#include "astra_dates.h"
#include "astra_ticket.h"
#include "CheckinBaseTypes.h"

#include <etick/tick_doctype.h>
#include <etick/tick_data.h>

#include <string>
#include <boost/optional.hpp>


namespace edifact
{

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

//-----------------------------------------------------------------------------

///@class UngElem - Group Header --UNG
struct UngElem
{
    std::string m_msgGroupName;
    std::string m_senderName;
    std::string m_senderCarrierCode;
    std::string m_recipientName;
    std::string m_recipientCarrierCode;
    BASIC::TDateTime m_prepareDateTime;
    std::string m_groupRefNum;
    std::string m_cntrlAgnCode;
    std::string m_msgVerNum;
    std::string m_msgRelNum;

    UngElem(const std::string& msgGroupName,
             const std::string& senderName,
             const std::string& senderCC,
             const std::string& recipientName,
             const std::string& recipientCC,
             const BASIC::TDateTime& prepareDt,
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

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

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
             const std::string& partyName
          )
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
             const std::string& country
          )
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

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

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
        DocCountry = 91
    };

    LocQualifier m_locQualifier;
    std::string m_locName;
    std::string m_relatedLocName1;
    std::string m_relatedLocName2;

    LocElem(LocQualifier locQualifier,
             const std::string& locName,
             const std::string& relatedLocName1 = "",
             const std::string& relatedLocName2 = ""
          )
        : m_locQualifier(locQualifier),
          m_locName(locName),
          m_relatedLocName1(relatedLocName1),
          m_relatedLocName2(relatedLocName2)
    {}
};

//-----------------------------------------------------------------------------

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

    DtmQualifier m_dtmQualifier;
    BASIC::TDateTime m_dateTime;
    std::string m_formatCode;

    DtmElem(DtmQualifier dtmQualifier,
             const BASIC::TDateTime& dateTime,
             const std::string& formatCode = "")
        : m_dtmQualifier(dtmQualifier),
          m_dateTime(dateTime),
          m_formatCode(formatCode)
    {}
};

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

///@class RffElem - Reference --RFF
struct RffElem
{
    std::string m_rffQualifier;
    std::string m_ref;

    RffElem(const std::string& rffQualifier,
             const std::string& ref)
        : m_rffQualifier(rffQualifier),
          m_ref(ref)
    {}
};

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

///@class PtkElem - Ppricing/Ticketing Details --PTK
struct PtkElem
{
    Dates::date m_issueDate;
};

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

///@class EbdElem - Excess Baggage Details --EBD
struct EbdElem {
    int         m_quantity;
    std::string m_charge;
    std::string m_measure;

    EbdElem(int quantity, const std::string& charge, const std::string& measure)
        : m_quantity(quantity), m_charge(charge), m_measure(measure)
    {}
};

//-----------------------------------------------------------------------------

///@class EqnElem - Number Of Units --EQN
struct EqnElem {
    unsigned    m_numberOfUnits;
    std::string m_qualifier;

    EqnElem(int nof, const std::string& qualifier)
        : m_numberOfUnits(nof), m_qualifier(qualifier)
    {}
};

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

///@class CpnElem - Coupon Information --CPN
struct CpnElem
{
    Ticketing::CouponNum_t m_num;
    Ticketing::TicketMedia m_media;
    Ticketing::TaxAmount::Amount m_amount;
    Ticketing::CouponStatus m_status;
    std::string m_sac;
    std::string m_action;
    Ticketing::CouponNum_t m_connectedNum;
};

//-----------------------------------------------------------------------------

///@class RciElements
struct RciElements
{
    std::list<RciElem>  m_lReclocs;
    std::list<Ticketing::FormOfId> m_lFoid;

    RciElements operator+(const RciElements &other) const;
};

//-----------------------------------------------------------------------------

///@class MonElements
struct MonElements
{
    std::list<MonElem> m_lMon;

    MonElements operator+(const MonElements &other) const;
};

//-----------------------------------------------------------------------------

///@class IftElements
struct IftElements
{
    std::list<Ticketing::FreeTextInfo> m_lIft;

    IftElements operator+(const IftElements &other) const;
};

//-----------------------------------------------------------------------------

///@class TxdElements
struct TxdElements
{
    std::list<Ticketing::TaxDetails> m_lTax;

    TxdElements operator+(const TxdElements &other) const;
};

//-----------------------------------------------------------------------------

///@class FopElements
struct FopElements
{
    std::list<Ticketing::FormOfPayment> m_lFop;

    FopElements operator+(const FopElements &other) const;
};

//-----------------------------------------------------------------------------

std::ostream& operator<<(std::ostream &os, const TktElem &tkt);
std::ostream& operator<<(std::ostream &os, const CpnElem &cpn);

//-----------------------------------------------------------------------------

///@class LorElem - Location/Originator details --LOR
struct LorElem
{
    std::string m_airline;
    std::string m_port;
};

//-----------------------------------------------------------------------------

///@class FdqElem - Flight details query --FDQ
struct FdqElem
{
    std::string m_outbAirl;
    Ticketing::FlightNum_t m_outbFlNum;
    Dates::DateTime_t m_outbDepDateTime;
    std::string m_outbDepPoint;
    std::string m_outbArrPoint;

    std::string m_inbAirl;
    Ticketing::FlightNum_t m_inbFlNum;
    Dates::DateTime_t m_inbDepDateTime;
    Dates::DateTime_t m_inbArrDateTime;
    std::string m_inbDepPoint;
    std::string m_inbArrPoint;

    FdqElem()
        : m_outbDepDateTime(Dates::not_a_date_time),
          m_inbDepDateTime(Dates::not_a_date_time),
          m_inbArrDateTime(Dates::not_a_date_time)
    {}
};

//-----------------------------------------------------------------------------

///@class PpdElem - Passenger personal details --PPD
struct PpdElem
{
    std::string m_passSurname;
    std::string m_passName;
    std::string m_passType;
    std::string m_passRespRef;
    std::string m_passQryRef;
};

//-----------------------------------------------------------------------------

///@class PrdElem - Passenger reservation details --PRD
struct PrdElem
{
    std::string m_rbd;
};

//-----------------------------------------------------------------------------

///@class PsdElem - Passenger seat request details --PSD
struct PsdElem
{
    std::string m_noSmokingInd;
    std::string m_characteristic; // TODO
};

//-----------------------------------------------------------------------------

///@class PbdElem - Passenger baggage information
struct PbdElem
{
    unsigned m_numOfPieces;
    unsigned m_weight;

    PbdElem()
        : m_numOfPieces(0), m_weight(0)
    {}
};

//-----------------------------------------------------------------------------

///@class FdrElem - Flight details response
struct FdrElem
{
    std::string m_airl;
    Ticketing::FlightNum_t m_flNum;
    Dates::DateTime_t m_depDateTime;
    Dates::DateTime_t m_arrDateTime;
    std::string m_depPoint;
    std::string m_arrPoint;

    FdrElem()
        : m_depDateTime(Dates::not_a_date_time),
          m_arrDateTime(Dates::not_a_date_time)
    {}
};

//-----------------------------------------------------------------------------

///@class RadElem - Response analysis details
struct RadElem
{
    std::string m_respType;
    std::string m_status;
};

//-----------------------------------------------------------------------------

///@class PfdElem - Passenger flight details
struct PfdElem
{
    std::string m_seat;
    std::string m_noSmokingInd;
    std::string m_cabinClass;
    std::string m_securityId;
};

//-----------------------------------------------------------------------------

///@class ChdElem - Cascading host details
struct ChdElem
{
    std::string m_origAirline;
    std::string m_origPoint;
    std::list<std::string> m_hostAirlines;
};

//-----------------------------------------------------------------------------

///@class FsdElem - Flight segment details
struct FsdElem
{
    boost::posix_time::time_duration m_boardingTime;
    FsdElem()
        : m_boardingTime(boost::posix_time::not_a_date_time)
    {}
    FsdElem(const boost::posix_time::time_duration& brdngTime)
        : m_boardingTime(brdngTime)
    {}
};

//-----------------------------------------------------------------------------

///@class ErdElem - Error information
struct ErdElem
{
    unsigned m_level;
    unsigned m_messageNumber;
    std::string m_messageText;

    ErdElem()
        : m_level(0), m_messageNumber(0)
    {}
    ErdElem(unsigned level, unsigned msgNum,
            const std::string& msgText = "")
        : m_level(level), m_messageNumber(msgNum),
          m_messageText(msgText)
    {}
};

//-----------------------------------------------------------------------------

std::ostream& operator<<(std::ostream &os, const LorElem &lor);
std::ostream& operator<<(std::ostream &os, const FdqElem &fdq);
std::ostream& operator<<(std::ostream &os, const PpdElem &ppd);
std::ostream& operator<<(std::ostream &os, const PrdElem &prd);
std::ostream& operator<<(std::ostream &os, const PsdElem &psd);
std::ostream& operator<<(std::ostream &os, const PbdElem &pbd);
std::ostream& operator<<(std::ostream &os, const FdrElem &fdr);
std::ostream& operator<<(std::ostream &os, const RadElem &rad);
std::ostream& operator<<(std::ostream &os, const PfdElem &pfd);
std::ostream& operator<<(std::ostream &os, const ChdElem &chd);
std::ostream& operator<<(std::ostream &os, const FsdElem &fsd);
std::ostream& operator<<(std::ostream &os, const ErdElem &erd);

}//namespace edifact

#endif/*_EDI_ELEMENTS_H_*/
