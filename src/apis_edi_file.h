//
// C++ Interface: apis_edi_file
//
// Description:
//
//
// Author: anton <anton@whale>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef _APIS_EDI_FILE_H_
#define _APIS_EDI_FILE_H_

#include <string>
#include <list>
#include <vector>

#include "date_time.h"
#include <stl_utils.h>
#include <astra_consts.h>
#include "xml_unit.h"
#include "remarks.h"
#include "apis_tools.h"
#include "astra_misc.h"
#include "tlg/edi_elements.h"

#include <edilib/edi_types.h>
#include <edilib/edi_request_handler.h>

struct _EDI_REAL_MES_STRUCT_;

namespace Paxlst {

using BASIC::date_time::TDateTime;

class GeneralInfo
{
    // Name of the company responsible for sending the information
    /* maxlen = 35 */
    /* required = M */
    std::string m_senderName;

    // Carrier Code of the sender company
    /* maxlen = 4 */
    /* required = C */
    std::string m_senderCarrierCode;

    // Name of the company responsible for receiving the information
    /* maxlen = 35 */
    /* required = M */
    std::string m_recipientName;

    // Carrier Code of the recipient company
    /* maxlen = 4 */
    /* required = C */
    std::string m_recipientCarrierCode;

    // IATA flight code. Example: OK0012/070915/1210
    /* maxlen = 35 */
    /* required = C */
    std::string m_iataCode;

public:
    GeneralInfo()
    {}

    // sender name
    const std::string& senderName() const {
        return m_senderName;
    }
    void setSenderName( const std::string& sn ) {
        m_senderName = upperc( sn.substr( 0, 35 ) );
    }

    // sender carrier code
    const std::string& senderCarrierCode() const {
        return m_senderCarrierCode;
    }
    void setSenderCarrierCode( const std::string& scc ) {
        m_senderCarrierCode = upperc( scc.substr( 0, 4 ) );
    }

    // recipient name
    const std::string& recipientName() const {
        return m_recipientName;
    }
    void setRecipientName( const std::string& rn ) {
        m_recipientName = upperc( rn.substr( 0, 35 ) );
    }

    // recipient carrier code
    const std::string& recipientCarrierCode() const {
        return m_recipientCarrierCode;
    }
    void setRecipientCarrierCode( const std::string& rcc ) {
        m_recipientCarrierCode = upperc( rcc.substr( 0, 4 ) );
    }

    // iata code
    const std::string& iataCode() const {
        return m_iataCode;
    }
    void setIataCode( const std::string& ic ) {
        m_iataCode = upperc( ic.substr( 0, 35 ) );
    }
};

//---------------------------------------------------------------------------------------

class FlightStop
{
    // Flight departure Airport. Three-character IATA Code
    /* maxlen = 25 */
    /* required = C */
    std::string m_depPort;

    // Departure Flight date and time
    /* required = C */
    TDateTime m_depDateTime;

    // Flight arrival Airport. Three-character IATA Code
    /* maxlen = 25 */
    /* required = C */
    std::string m_arrPort;

    // Arrival Flight date and time
    /* required = C */
    TDateTime m_arrDateTime;

public:
    FlightStop(const std::string& port,
               const TDateTime arrDateTime,    //??? ????? ??????? ???????????? ??????
               const TDateTime depDateTime)    //??? ????? ?????? ???????????? ??????
    {
        setDepPort(port);
        setArrPort(port); // ?????? ??? ?? ???? ?? ???????? ? ????????? ??? ???????? ??????????? ??????? ?? ????????? ? ?????????? ?????????? ??????
        setDepDateTime(depDateTime);
        setArrDateTime(arrDateTime);
    }
    // departure airport
    const std::string& depPort() const {
        return m_depPort;
    }
    void setDepPort( const std::string& dp ) {
        m_depPort = upperc( dp.substr( 0, 25 ) );
    }

    // departure date/time
    void setDepDateTime( const TDateTime& ddt ) {
        m_depDateTime = ddt;
    }
    const TDateTime& depDateTime() const {
        return m_depDateTime;
    }

    // arrival airport
    const std::string& arrPort() const {
        return m_arrPort;
    }
    void setArrPort( const std::string& ap ) {
        m_arrPort = upperc( ap.substr( 0, 25 ) );
    }

    // arrival date/time
    void setArrDateTime( const TDateTime& adt ) {
        m_arrDateTime = adt;
    }
    const TDateTime& arrDateTime() const {
        return m_arrDateTime;
    }
};

class FlightStops : public std::vector<FlightStop> {};

class FlightInfo
{
    std::string m_carrier;

    // Carrier Code/Flight Number. For example: OK051
    /* maxlen = 17 */
    /* required = C */
    std::string m_flight;

    FlightStops m_stopsBeforeBorder;
    FlightStops m_stopsAfterBorder;

    // Marketing flights
    std::map<std::string, std::string> mktFlts;
    // Flight legs
    FlightLegs legs;

public:
    FlightInfo() {}

    // carrier
    const std::string& carrier() const {
        return m_carrier;
    }
    void setCarrier( const std::string& f ) {
        m_carrier = upperc( f.substr( 0, 17 ) );
    }

    // flight
    const std::string& flight() const {
        return m_flight;
    }
    void setFlight( const std::string& f ) {
        m_flight = upperc( f.substr( 0, 17 ) );
    }

    // marketing flights
    void addMarkFlt( const std::string& airline, const std::string& flight ) {
        mktFlts.insert(std::pair<std::string, std::string>(airline, flight));
    }
    const std::map<std::string, std::string>& markFlts() const {
        return mktFlts;
    }
    // flight legs
    void setFltLegs( const FlightLegs& flt_legs ) {
        legs = flt_legs;
    }
    const FlightLegs& fltLegs() const {
        return legs;
    }

    const FlightStops& stopsBeforeBorder() const {
        return m_stopsBeforeBorder;
    }
    FlightStops& stopsBeforeBorder() {
        return m_stopsBeforeBorder;
    }
    const FlightStops& stopsAfterBorder() const {
        return m_stopsAfterBorder;
    }
    FlightStops& stopsAfterBorder() {
        return m_stopsAfterBorder;
    }

    void setCrossBorderFlightStops(const std::string& dp,
                                   const TDateTime& ddt,
                                   const std::string& ap,
                                   const TDateTime& adt)
    {
      m_stopsBeforeBorder.clear();
      m_stopsBeforeBorder.emplace_back(dp, ASTRA::NoExists, ddt);
      m_stopsAfterBorder.clear();
      m_stopsAfterBorder.emplace_back(ap, adt, ASTRA::NoExists);
    }

    void getCrossBorderFlightStops(std::string& dp,
                                   TDateTime& ddt,
                                   std::string& ap,
                                   TDateTime& adt) const
    {
      dp.clear();
      ddt=ASTRA::NoExists;
      ap.clear();
      adt=ASTRA::NoExists;
      if (m_stopsBeforeBorder.size()==1 &&
          m_stopsAfterBorder.size()==1)
      {
        dp=m_stopsBeforeBorder.front().depPort();
        ddt=m_stopsBeforeBorder.front().depDateTime();
        ap=m_stopsAfterBorder.back().arrPort();
        adt=m_stopsAfterBorder.back().arrDateTime();
      }
    }
};

//---------------------------------------------------------------------------------------

class PartyInfo
{
    // Full name of the company
    /* maxlen = 35 */
    std::string m_partyName;

    // Telephone number of the company
    /* maxlen = 25 */
    std::string m_phone;

    // Fax number of the company
    /* maxlen = 25 */
    std::string m_fax;

    // E-Mail address of the company
    /* maxlen = 25 */
    std::string m_email;

public:
    PartyInfo()
    {}

    // Company name
    const std::string& partyName() const {
        return m_partyName;
    }
    void setPartyName( const std::string& pn ) {
        m_partyName = upperc( pn.substr( 0, 35 ) );
    }

    // Company phone
    const std::string& phone() const {
        return m_phone;
    }
    void setPhone( const std::string& p ) {
        m_phone = upperc( p.substr( 0, 25 ) );
    }

    // Company fax
    const std::string& fax() const {
        return m_fax;
    }
    void setFax( const std::string& f ) {
        m_fax = upperc( f.substr( 0, 25 ) );
    }

    // Company email
    const std::string& email() const {
        return m_email;
    }
    void setEMail( const std::string& em ) {
        m_email = upperc( em.substr( 0, 25 ) );
    }
};

//---------------------------------------------------------------------------------------

class PassengerInfo
{
    friend class PaxlstInfo;

    // Contains the passenger's personal data. As a minimum,
    // the name and surname should appear. These data can include
    // all passenger's personal data, or omit some of them
    /* maxlen = 35 */
    /* required = C */
    std::string m_surname;
    std::string m_first_name;
    std::string m_second_name;

    // Passenger's Gender. One character. Validity includes:
    // M = Male
    // F = Female
    /* maxlen = 17 */
    /* required = C */
    std::string m_sex;

    // Passenger's City
    /* maxlen = 35 */
    /* required = C */
    std::string m_city;

    // Passenger's Street
    /* maxlen = 35 */
    /* required = C */
    std::string m_street;

    std::string m_countrySubEntityCode;
    std::string m_postalCode;
    std::string m_destCountry;
    std::string m_residCountry;

    // Passenger's date of birth
    /* required = C */
    TDateTime m_birthDate;

    std::string m_CBPPort;

    // Passenger's departure airport. Three-character IATA code
    /* maxlen = 25 */
    /* required = C */
    std::string m_depPort;

    // Passenger's arrival airport. Three-character IATA code
    /* maxlen = 25 */
    /* required = C */
    std::string m_arrPort;

    // Passenger Nationality. Three-character country code for
    // passenger's country, as per ISO 3166
    /* maxlen = 3 */
    /* required = C */
    std::string m_nationality;

    // Flight passenger reservation number. 35 characters maximum
    /* maxlen = 35 */
    /* required = C */
    std::string m_reservNum;

    // Passenger type
    /* maxlen = 3 */
    /* required = C */
    std::string m_docType;

    // Unique number assigned to the identification document
    // produced by the passenger
    /* maxlen = 35 */
    /* required = C */
    std::string m_docNumber;

    // Expiration date of the identification document produced
    // by the passenger
    /* required = C */
    TDateTime m_docExpirateDate;

    // Country code where the produced document is used, as per ISO 3166
    /* maxlen = 25 */
    /* required = C */
    std::string m_docCountry;

    std::string m_birthCountry;
    std::string m_birthCity;
    std::string m_birthRegion;
    bool pr_brd;
    bool go_show;
    std::string pers_type;
    std::string ticket_num;
    std::vector< std::pair<int, std::string> > pax_seats;
    int m_bagCount;
    int m_bagWeight;
    std::set<std::string> m_bagTags;
    std::set<CheckIn::TPaxFQTItem> pax_fqts;
    std::string m_pax_ref;
    std::string m_proc_info; // Processing Information

    std::string m_doco_type;
    std::string m_doco_no;
    std::string m_doco_country;
    TDateTime   m_docoExpirateDate = ASTRA::NoExists;

public:
    PassengerInfo()
        : m_birthDate( ASTRA::NoExists ), m_docExpirateDate( ASTRA::NoExists ),
          m_bagCount( ASTRA::NoExists ), m_bagWeight( ASTRA::NoExists ),
          m_docoExpirateDate( ASTRA::NoExists )
    {}

    // passenger's surname
    const std::string& surname() const {
        return m_surname;
    }
    void setSurname( const std::string& s ) {
        m_surname = upperc( s.substr( 0, 35 ) );
    }

    // passenger's first name
    const std::string& first_name() const {
        return m_first_name;
    }
    void setFirstName( const std::string& n ) {
        m_first_name = upperc( n.substr( 0, 35 ) );
    }

    // passenger's second name
    const std::string& second_name() const {
        return m_second_name;
    }
    void setSecondName( const std::string& n ) {
        m_second_name = upperc( n.substr( 0, 35 ) );
    }

    // passenger's sex
    const std::string& sex() const {
        return m_sex;
    }
    void setSex( const std::string& s ) {
        m_sex = upperc( s.substr( 0, 17 ) );
    }

    // passenger's city
    const std::string& city() const {
        return m_city;
    }
    void setCity( const std::string& c ) {
        m_city = upperc( c.substr( 0, 35 ) );
    }

    // passenger's street
    const std::string& street() const {
        return m_street;
    }
    void setStreet( const std::string& s ) {
        m_street = upperc( s.substr( 0, 35 ) );
    }

    // passenger's country sub-entity name code
    const std::string& countrySubEntityCode() const {
        return m_countrySubEntityCode;
    }
    void setCountrySubEntityCode( const std::string& s ) {
        m_countrySubEntityCode = upperc( s.substr( 0, 9 ) );
    }

    // passenger's postal identification code
    const std::string& postalCode() const {
        return m_postalCode;
    }
    void setPostalCode( const std::string& s ) {
        m_postalCode = upperc( s.substr( 0, 17 ) );
    }

    // passenger's country name code
    // Destination
    const std::string& destCountry() const {
        return m_destCountry;
    }
    void setDestCountry( const std::string& s ) {
        m_destCountry = upperc( s.substr( 0, 3 ) );
    }
    // Residence
    const std::string& residCountry() const {
        return m_residCountry;
    }
    void setResidCountry( const std::string& s ) {
        m_residCountry = upperc( s.substr( 0, 3 ) );
    }

    // passenger's birth date
    const TDateTime& birthDate() const {
        return m_birthDate;
    }
    void setBirthDate( const TDateTime& bd ) {
        m_birthDate = bd;
    }

    // passenger's Customs and Border Protection (CBP) airport
    const std::string& CBPPort() const {
        return m_CBPPort;
    }
    void setCBPPort( const std::string& cbpp ) {
        m_CBPPort = upperc( cbpp.substr( 0, 25 ) );
    }

    // passenger's departure airport
    const std::string& depPort() const {
        return m_depPort;
    }
    void setDepPort( const std::string& dp ) {
        m_depPort = upperc( dp.substr( 0, 25 ) );
    }

    // passenger's arrival airport
    const std::string& arrPort() const {
        return m_arrPort;
    }
    void setArrPort( const std::string& ap ) {
        m_arrPort = upperc( ap.substr( 0, 25 ) );
    }

    // passenger's nationality
    const std::string& nationality() const {
        return m_nationality;
    }
    void setNationality( const std::string& n ) {
        m_nationality = upperc( n.substr( 0, 3 ) );
    }

    // passenger's reservation number
    const std::string& reservNum() const {
        return m_reservNum;
    }
    void setReservNum( const std::string& rn ) {
        m_reservNum = upperc( rn.substr( 0, 35 ) );
    }

    // passenger's document type
    const std::string& docType() const {
        return m_docType;
    }
    void setDocType( const std::string& t ) {
        m_docType = upperc( t.substr( 0, 3 ) );
    }

    // passenger's document number
    const std::string& docNumber() const {
        return m_docNumber;
    }
    void setDocNumber( const std::string& dn ) {
        m_docNumber = upperc( dn.substr( 0, 35 ) );
    }

    // passenger's document expiration date
    const TDateTime& docExpirateDate() const {
        return m_docExpirateDate;
    }
    void setDocExpirateDate( const TDateTime& ded ) {
        m_docExpirateDate = ded;
    }

    // passenger's document country
    const std::string& docCountry() const {
        return m_docCountry;
    }
    void setDocCountry( const std::string& dc ) {
        m_docCountry = upperc( dc.substr( 0, 25 ) );
    }

    // Birth
    const std::string& birthCountry() const {
        return m_birthCountry;
    }
    void setBirthCountry( const std::string& s ) {
        m_birthCountry = upperc( s.substr( 0, 3 ) );
    }

    const std::string& birthCity() const {
        return m_birthCity;
    }
    void setBirthCity( const std::string& s ) {
        m_birthCity = upperc( s.substr( 0, 70 ) );
    }

    const std::string& birthRegion() const {
        return m_birthRegion;
    }
    void setBirthRegion( const std::string& s ) {
        m_birthRegion = upperc( s.substr( 0, 70 ) );
    }
    bool prBrd() const {
        return pr_brd;
    }
    void setPrBrd(bool boarded) {
      pr_brd = boarded;
    }
    bool goShow() const {
        return go_show;
    }
    void setGoShow(bool goshow) {
      go_show = goshow;
    }
    const std::string& persType() const {
        return pers_type;
    }
    void setPersType(const std::string& type) {
      pers_type = type;
    }
    const std::string& ticketNumber() const {
        return ticket_num;
    }
    void setTicketNumber(const std::string& num) {
      ticket_num = num;
    }
    const std::vector< std::pair<int, std::string> >& seats() const {
        return pax_seats;
    }
    void setSeats(const std::vector< std::pair<int, std::string> >& values) {
      pax_seats = values;
    }
    int bagCount() const {
        return m_bagCount;
    }
    void setBagCount( const int value ) {
      m_bagCount = value;
    }
    int bagWeight() const {
        return m_bagWeight;
    }
    void setBagWeight( const int value ) {
      m_bagWeight = value;
    }
    const std::set<std::string>& bagTags() const
    {
      return m_bagTags;
    }
    void setBagTags(const std::set<std::string>& tags)
    {
      m_bagTags = tags;
    }
    const std::set<CheckIn::TPaxFQTItem>& fqts() const {
        return pax_fqts;
    }
    void setFqts(std::set<CheckIn::TPaxFQTItem>& values) {
      pax_fqts = values;
    }
    const std::string& paxRef() const
    {
      return m_pax_ref;
    }
    void setPaxRef( const std::string& s )
    {
      m_pax_ref = upperc( s.substr( 0, 35 ) );
    }
    const std::string& procInfo() const
    {
      return m_proc_info;
    }
    void setProcInfo( const std::string& s )
    {
      m_proc_info = upperc( s.substr( 0, 3 ) );
    }

    // passenger's visa type
    const std::string& docoType() const
    {
      return m_doco_type;
    }
    void setDocoType( const std::string& t )
    {
      m_doco_type = upperc( t.substr( 0, 3 ) );
    }

    // passenger's visa number
    const std::string& docoNumber() const
    {
      return m_doco_no;
    }
    void setDocoNumber( const std::string& dn )
    {
      m_doco_no = upperc( dn.substr( 0, 35 ) );
    }

    // passenger's visa country
    const std::string& docoCountry() const
    {
      return m_doco_country;
    }
    void setDocoCountry( const std::string& dc )
    {
      m_doco_country = upperc( dc.substr( 0, 25 ) );
    }

    // passenger's visa expirate date
    const TDateTime& docoExpirateDate() const
    {
        return m_docoExpirateDate;
    }
    void setDocoExpirateDate( const TDateTime& ded )
    {
        m_docoExpirateDate = ded;
    }


};
typedef std::list< PassengerInfo > PassengersList_t;

//---------------------------------------------------------------------------------------

class PaxlstSettings
{
    std::string m_appRef;
    std::string m_mesRelNum;
    std::string m_mesAssCode;
    std::string m_respAgnCode;
    bool m_viewUNGandUNE;
    bool m_view_RFF_TN = false;
    std::string m_RFF_TN;
    std::string m_unh_number;

public:
    PaxlstSettings()
        : m_appRef( "APIS" ),
          m_mesRelNum( "02B" ),
          m_mesAssCode( "IATA" ),
          m_respAgnCode( "111" ),
          m_viewUNGandUNE(false)
    {}

    const std::string& appRef() const { return m_appRef; }
    void setAppRef( const std::string& appRef ) { m_appRef = appRef; }

    const std::string& mesRelNum() const { return m_mesRelNum; }
    void setMesRelNum( const std::string& mesRelNum ) { m_mesRelNum = mesRelNum; }

    const std::string& mesAssCode() const { return m_mesAssCode; }
    void setMesAssCode( const std::string& mesAssCode ) { m_mesAssCode = mesAssCode; }

    const std::string& respAgnCode() const { return m_respAgnCode; }
    void setRespAgnCode( const std::string& respAgnCode ) { m_respAgnCode = respAgnCode; }

    bool viewUNGandUNE() const { return m_viewUNGandUNE; }
    void setViewUNGandUNE( const bool& viewUNGandUNE ) { m_viewUNGandUNE = viewUNGandUNE; }

    bool view_RFF_TN() const { return m_view_RFF_TN; }
    void set_view_RFF_TN( bool view_RFF_TN ) { m_view_RFF_TN = view_RFF_TN; }

    const std::string& RFF_TN() const { return m_RFF_TN; }
    void set_RFF_TN( const std::string& rff_tn ) { m_RFF_TN = rff_tn; }

    const std::string& unh_number() const { return m_unh_number; }
    void set_unh_number( const std::string& num ) { m_unh_number = num.substr(0, EDI_MESNUM_LEN); }
};

//---------------------------------------------------------------------------------------

class PaxlstInfo: public GeneralInfo, public PartyInfo, public FlightInfo
{
public:
    enum PaxlstType
    {
        FlightPassengerManifest,
        FlightCrewManifest,
        IAPIClearPassengerRequest,
        IAPIChangePassengerData,
        IAPIFlightCloseOnBoard,
        IAPICancelFlight
    };
private:
    PaxlstType m_type;
    std::string m_docId;
    PassengersList_t m_passList;
    PaxlstSettings m_settings;

public:
    PaxlstInfo(const PaxlstType &paxlstType,
               const std::string &docId)
      : m_type( paxlstType ),
        m_docId( docId )
    {}

    const PassengersList_t& passengersList() const { return m_passList; }
    void setPassengersList( const PassengersList_t& passList ) { m_passList = passList; }
    void addPassenger( const PassengerInfo& passInfo );

    PaxlstSettings& settings() { return m_settings; }
    const PaxlstSettings& settings() const { return m_settings; }
    PaxlstType type() const { return m_type; }
    std::string docId() const { return m_docId; }

    std::string toEdiString() const;
    void toXMLFormat(xmlNodePtr emulApisNode, const int psg_num, const int crew_num, const int version) const;
    std::vector< std::string > toEdiStrings( unsigned maxPaxPerString ) const;
    edifact::BgmElem getBgmElem() const;
    edifact::CntElem getCntElem(const int totalCnt) const;
    edifact::NadElem getNadElem(const Paxlst::PassengerInfo& pax) const;

    bool passengersListMayBeEmpty() const { return m_type==IAPIFlightCloseOnBoard; }
    bool passengersListAlwaysEmpty() const { return m_type==IAPICancelFlight; }

protected:
    void checkInvariant() const;
};

//---------------------------------------------------------------------------------------

std::string createEdiPaxlstFileName( const std::string& carrierCode,
                                     const int& flightNumber,
                                     const std::string& flightSuffix,
                                     const std::string& origin,
                                     const std::string& destination,
                                     const TDateTime& departureDate,
                                     const std::string& ext,
                                     unsigned partNum = 0,
                                     const std::string& lst_type = "" );

std::string createIataCode( const std::string& flight,
                            const TDateTime& destDateTime,
                            const std::string& destDateTimeFmt = "/yymmdd/hhnn" );


}//namespace Paxlst

/////////////////////////////////////////////////////////////////////////////////////////

namespace edifact {

void collectPAXLST(_EDI_REAL_MES_STRUCT_ *pMes, const Paxlst::PaxlstInfo& paxlst);

//

struct Cusres
{
    struct SegGr3
    {
        RffElem m_rff;
        DtmElem m_dtm1;
        DtmElem m_dtm2;
        LocElem m_loc1;
        LocElem m_loc2;

        SegGr3(const RffElem& rff,
               const DtmElem& dtm1,
               const DtmElem& dtm2,
               const LocElem& loc1,
               const LocElem& loc2);
    };

    //---------------------------------

    struct SegGr4
    {
        ErpElem                  m_erp;
        ErcElem                  m_erc;
        std::vector<RffElem>     m_vRff;
        boost::optional<FtxElem> m_ftx;

        SegGr4(const ErpElem& erp,
               const ErcElem& erc);
    };

    //---------------------------------

    BgmElem                  m_bgm;
    boost::optional<UngElem> m_ung;
    boost::optional<RffElem> m_rff;
    boost::optional<UneElem> m_une;
    std::vector<SegGr3>      m_vSegGr3;
    std::vector<SegGr4>      m_vSegGr4;

    Cusres(const BgmElem& bgm);
};

//---------------------------------------------------------------------------------------

Cusres readCUSRES(_EDI_REAL_MES_STRUCT_ *pMes);
Cusres readCUSRES(const std::string& ediText);

//---------------------------------------------------------------------------------------

std::ostream& operator<<(std::ostream& os, const Cusres& cusres);

class CusresCallbacks
{
    public:
        virtual ~CusresCallbacks() {}
        virtual void onCusResponseHandle(TRACE_SIGNATURE, const Cusres& cusres) = 0;
        virtual void onCusRequestHandle(TRACE_SIGNATURE, const Cusres& cusres) = 0;
};

}//namespace edifact

#endif//_APIS_EDI_FILE_H_
