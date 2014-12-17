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

#include <basic.h>
#include <stl_utils.h>
#include <astra_consts.h>
#include "xml_unit.h"

namespace Paxlst
{
    
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

//-------------------------------------------------------------------------------------------------
    
class FlightInfo
{
    std::string m_carrier;

    // Carrier Code/Flight Number. For example: OK051
    /* maxlen = 17 */
    /* required = C */
    std::string m_flight;

    // Flight departure Airport. Three-character IATA Code
    /* maxlen = 25 */
    /* required = C */
    std::string m_depPort;

    // Departure Flight date and time
    /* required = C */
    BASIC::TDateTime m_depDateTime;

    // Flight arrival Airport. Three-character IATA Code
    /* maxlen = 25 */
    /* required = C */
    std::string m_arrPort;

    // Arrival Flight date and time
    /* required = C */
    BASIC::TDateTime m_arrDateTime;
    
public:
    FlightInfo()
        : m_depDateTime( ASTRA::NoExists ), m_arrDateTime( ASTRA::NoExists )
    {}
    
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
    
    // departure airport
    const std::string& depPort() const {
        return m_depPort;
    }
    void setDepPort( const std::string& dp ) {
        m_depPort = upperc( dp.substr( 0, 25 ) );
    }
    
    // departure date/time
    void setDepDateTime( const BASIC::TDateTime& ddt ) {
        m_depDateTime = ddt;
    }
    const BASIC::TDateTime& depDateTime() const {
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
    void setArrDateTime( const BASIC::TDateTime& adt ) {
        m_arrDateTime = adt;
    }
    const BASIC::TDateTime& arrDateTime() const {
        return m_arrDateTime;
    }
};

//-------------------------------------------------------------------------------------------------

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

//-------------------------------------------------------------------------------------------------

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
    BASIC::TDateTime m_birthDate;

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
    BASIC::TDateTime m_docExpirateDate;

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

public:
    PassengerInfo()
        : m_birthDate( ASTRA::NoExists ), m_docExpirateDate( ASTRA::NoExists )
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
    const BASIC::TDateTime& birthDate() const {
        return m_birthDate;
    }
    void setBirthDate( const BASIC::TDateTime& bd ) {
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
    const BASIC::TDateTime& docExpirateDate() const {
        return m_docExpirateDate;
    }
    void setDocExpirateDate( const BASIC::TDateTime& ded ) {
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
    const bool prBrd() const {
        return pr_brd;
    }
    void setPrBrd(bool boarded) {
      pr_brd = boarded;
    }
    const bool goShow() const {
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
    void setSeats(std::vector< std::pair<int, std::string> >& values) {
      pax_seats = values;
    }
};
typedef std::list< PassengerInfo > PassengersList_t;

//-------------------------------------------------------------------------------------------------

class PaxlstSettings
{
    std::string m_appRef;
    std::string m_mesRelNum;
    std::string m_mesAssCode;
    std::string m_respAgnCode;
    bool m_viewUNGandUNE;
    
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

    const bool viewUNGandUNE() const { return m_viewUNGandUNE; }
    void setViewUNGandUNE( const bool& viewUNGandUNE ) { m_viewUNGandUNE = viewUNGandUNE; }
};

//-------------------------------------------------------------------------------------------------

class FlightLeg {
private:
  int loc_qualifier;
  std::string airp;
  std::string country;
  BASIC::TDateTime sch_in;
  BASIC::TDateTime sch_out;
public:
  FlightLeg (std::string airp, std::string country, BASIC::TDateTime sch_in, BASIC::TDateTime sch_out):
    airp(airp), country(country), sch_in(sch_in), sch_out(sch_out) {}
  void setLocQualifier(const int value) {loc_qualifier = value; }
  const std::string Country() { return country; }
  void toXML(xmlNodePtr FlightLegsNode) const;
};

class FlightLegs : public std::vector<FlightLeg> {
  public:
  void FlightLegstoXML(xmlNodePtr FlightLegsNode) const;
  void MakeFlightLegs(int first_point);
};

//-------------------------------------------------------------------------------------------------

class PaxlstInfo: public GeneralInfo, public PartyInfo, public FlightInfo, public FlightLegs
{
public:
    enum PaxlstType
    {
        FlightPassengerManifest,
        FlightCrewManifest
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
    void toXMLFormat(xmlNodePtr emulApisNode, int psg_num, int crew_num, bool is_original) const;
    std::vector< std::string > toEdiStrings( unsigned maxPaxPerString ) const;
    
protected:
    void checkInvariant() const;
};

//-------------------------------------------------------------------------------------------------

std::string createEdiPaxlstFileName( const std::string& carrierCode,
                                     const int& flightNumber,
                                     const std::string& flightSuffix,
                                     const std::string& origin,
                                     const std::string& destination,
                                     const BASIC::TDateTime& departureDate,
                                     const std::string& ext,
                                     unsigned partNum = 0 );

std::string createIataCode( const std::string& flight,
                            const BASIC::TDateTime& destDateTime,
                            const std::string& destDateTimeFmt = "/yymmdd/hhnn" );

const std::string generate_envelope_id (const std::string& airl);
}//namespace Paxlst

#endif//_APIS_EDI_FILE_H_
