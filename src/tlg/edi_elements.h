#ifndef _EDI_ELEMENTS_H_
#define _EDI_ELEMENTS_H_

#include "basic.h"

#include <string>
#include <boost/optional.hpp>
#include <etick/tick_doctype.h>
#include <etick/tick_data.h>
#include "ticket_types.h"

namespace ASTRA
{
namespace edifact
{

///@class UnbElem - Interchange Header --UNB
struct UnbElem
{
    std::string m_senderCarrierCode;
    std::string m_recipientCarrierCode;

    UnbElem( const std::string& senderCC,
             const std::string& recipientCC )
        : m_senderCarrierCode( senderCC ),
          m_recipientCarrierCode( recipientCC )
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

    UngElem( const std::string& msgGroupName,
             const std::string& senderName,
             const std::string& senderCC,
             const std::string& recipientName,
             const std::string& recipientCC,
             const BASIC::TDateTime& prepareDt,
             const std::string& groupRefNum,
             const std::string& cntrlAgnCode,
             const std::string& msgVerNum,
             const std::string& msgRelNum )
        : m_msgGroupName( msgGroupName ),
          m_senderName( senderName ),
          m_senderCarrierCode( senderCC ),
          m_recipientName( recipientName ),
          m_recipientCarrierCode( recipientCC ),
          m_prepareDateTime( prepareDt ),
          m_groupRefNum( groupRefNum ),
          m_cntrlAgnCode( cntrlAgnCode ),
          m_msgVerNum( msgVerNum ),
          m_msgRelNum( msgRelNum )
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

    UnhElem( const std::string& msgTypeId,
             const std::string& msgVerNum,
             const std::string& msgRelNum,
             const std::string& cntrlAgnCode,
             const std::string& assAccCode,
             unsigned seqNum,
             SeqFlag seqFlag )
        : m_msgTypeId( msgTypeId ),
          m_msgVerNum( msgVerNum ),
          m_msgRelNum( msgRelNum ),
          m_cntrlAgnCode( cntrlAgnCode ),
          m_assAccCode( assAccCode ),
          m_seqNumber( seqNum ),
          m_seqFlag( seqFlag )
    {}

    static std::string seqFlagToStr( SeqFlag flag )
    {
        if( flag == First )
            return "C";
        else if( flag == Last )
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

    BgmElem( const std::string& docCode,
             const std::string& docId )
        : m_docCode( docCode ),
          m_docId( docId )
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

    NadElem( const std::string& funcCode,
             const std::string& partyName
           )
        : m_funcCode( funcCode ),
          m_partyName( partyName )
    {}

    NadElem( const std::string& funcCode,
             const std::string& partyName,
             const std::string& partyName2,
             const std::string& partyName3,
             const std::string& street,
             const std::string& city,
             const std::string& countrySubEntityCode,
             const std::string& postalCode,
             const std::string& country
           )
        : m_funcCode( funcCode ),
          m_partyName( partyName ),
          m_partyName2( partyName2 ),
          m_partyName3( partyName3 ),
          m_street( street ),
          m_city( city ),
          m_countrySubEntityCode( countrySubEntityCode ),
          m_postalCode( postalCode ),
          m_country( country )
    {}

};

//-----------------------------------------------------------------------------

///@class ComElem - Comminication Contact --COM
struct ComElem
{
    std::string m_phone;
    std::string m_fax;
    std::string m_email;

    ComElem( const std::string& phone,
             const std::string& fax,
             const std::string& email = "" )
        : m_phone( phone ),
          m_fax( fax ),
          m_email( email )
    {}
};

//-----------------------------------------------------------------------------

///@class TdtElem - Details of Transport --TDT
struct TdtElem
{
    std::string m_stageQuailifier;
    std::string m_journeyId;
    std::string m_carrierId;

    TdtElem( const std::string& stageQualifier,
             const std::string& journeyId,
             const std::string& carrierId )
        : m_stageQuailifier( stageQualifier ),
          m_journeyId( journeyId ),
          m_carrierId( carrierId )
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

    LocElem( LocQualifier locQualifier,
             const std::string& locName,
             const std::string& relatedLocName1 = "",
             const std::string& relatedLocName2 = ""
           )
        : m_locQualifier( locQualifier ),
          m_locName( locName ),
          m_relatedLocName1( relatedLocName1 ),
          m_relatedLocName2( relatedLocName2 )
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

    DtmElem( DtmQualifier dtmQualifier,
             const BASIC::TDateTime& dateTime,
             const std::string& formatCode = "" )
        : m_dtmQualifier( dtmQualifier ),
          m_dateTime( dateTime ),
          m_formatCode( formatCode )
    {}
};

//-----------------------------------------------------------------------------

///@class AttElem - Attribute --ATT
struct AttElem
{
    std::string m_funcCode;
    std::string m_value;

    AttElem( const std::string& funcCode,
             const std::string& value )
        : m_funcCode( funcCode ),
          m_value( value )
    {}
};

//-----------------------------------------------------------------------------

///@class NatElem - Nationality --NAT
struct NatElem
{
    std::string m_natQualifier;
    std::string m_nat;

    NatElem( const std::string& natQualifier,
             const std::string& nat )
        : m_natQualifier( natQualifier ),
          m_nat( nat )
    {}
};

//-----------------------------------------------------------------------------

///@class RffElem - Reference --RFF
struct RffElem
{
    std::string m_rffQualifier;
    std::string m_ref;

    RffElem( const std::string& rffQualifier,
             const std::string& ref )
        : m_rffQualifier( rffQualifier ),
          m_ref( ref )
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

    DocElem( const std::string& docCode,
             const std::string& docNum,
             const std::string& respAgnCode,
             const std::string& idCode = "110" )
        : m_docCode( docCode ),
          m_docNum( docNum ),
          m_respAgnCode( respAgnCode ),
          m_idCode( idCode )
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

    CntElem( CntType cntType, unsigned cnt )
        : m_cntType( cntType ),
          m_cnt( cnt )
    {}
};

//-----------------------------------------------------------------------------

///@class UneElem - Group Trailer --UNE
struct UneElem
{
    std::string m_refNum;
    unsigned m_cntrlCnt;

    UneElem( const std::string& refNum,
             unsigned cntrlCnt = 1 )
        : m_refNum( refNum ),
          m_cntrlCnt( cntrlCnt )
    {}
};

struct TktElement {
    Ticketing::DocType           docType;
    Ticketing::TicketNum_t       ticketNum;
    boost::optional<int>         nBooklets;
    boost::optional<int>         conjunctionNum;
    boost::optional<Ticketing::TicketNum_t> inConnectionTicketNum;
    boost::optional<Ticketing::TickStatAction::TickStatAction_t> tickStatAction;
};

std::ostream & operator << (std::ostream &os, const TktElement &tkt);

struct CpnElement
{
    Ticketing::CouponNum_t num;
    Ticketing::TicketMedia media;
    Ticketing::TaxAmount::Amount amount;
    Ticketing::CouponStatus status;
    std::string sac;
    std::string action;
    Ticketing::CouponNum_t connectedNum;
};
typedef boost::optional<CpnElement> CpnElement_o;
std::ostream &operator << (std::ostream &s, const CpnElement&);

}//namespace edifact
}//namespace ASTRA

#endif/*_EDI_ELEMENTS_H_*/
