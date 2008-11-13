//
// C++ Interface: czech_police_edi_file
//
// Description:
//
//
// Author: anton <anton@whale>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef _CZECH_POLICE_EDI_FILE_H_
#define _CZECH_POLICE_EDI_FILE_H_

#include <string>
#include <list>
#include <exception>

#include <basic.h>
#include <stl_utils.h>
#include <astra_consts.h>


namespace Paxlst
{
    using std::string;
    using std::list;
    using std::exception;


    class PaxlstInfo;

    string CreateEdiPaxlstString( const PaxlstInfo& paxlstInfo );

    string CreateEdiPaxlstFileName( const string& flight,
                                    const string& origin,
                                    const string& destination,
                                    const string& departureDate,
                                    const string& ext );

    bool CreateEdiPaxlstFileName(   string& result,
                                    const string& flight, const string& origin,
                                    const string& destination,
                                    const BASIC::TDateTime& departureDate,
                                    const string& ext );

    bool CreateEdiInterchangeReference( string& res );

    bool CreateDateTimeStr( string& res,
                            const BASIC::TDateTime& dt, const string& format );

    string CreateIATACode( const string& flight,
                           const string& destDate, const string& destTime );

    bool CreateIATACode( string& result, const string& flight,
                         const BASIC::TDateTime& destDateTime );


    class PaxlstException: public exception
    {
    public:
        PaxlstException() : _errMsg( "" ) {}
        PaxlstException( const string& theErrMsg ) { _errMsg = theErrMsg; }

        virtual ~PaxlstException() throw() {}

        const string& errMsg() const { return _errMsg; }

    private:
        string _errMsg;
    };


    class GeneralInfo
    {
    public:
        // Constructor
        GeneralInfo()
        {
            senderName = "";
            senderCarrierCode = "";
            recipientCarrierCode = "";
            iataCode = "";
        }

        // Getters/Setters
        void setSenderName( const string& theSenderName ) {
            senderName = upperc( theSenderName );
            if ( senderName.size() > 35 ) senderName.resize( 35 );
        }
        const string& getSenderName() const {
            return senderName;
        }

        void setSenderCarrierCode( const string& theSenderCarrierCode ) {
            senderCarrierCode = upperc( theSenderCarrierCode );
            if ( senderCarrierCode.size() > 4 ) senderCarrierCode.resize( 4 );
        }
        const string& getSenderCarrierCode() const {
            return senderCarrierCode;
        }

        void setRecipientCarrierCode( const string& theRecipientCarrierCode ) {
            recipientCarrierCode = upperc( theRecipientCarrierCode );
            if ( recipientCarrierCode.size() > 4 ) recipientCarrierCode.resize( 4 );
        }
        const string& getRecipientCarrierCode() const {
            return recipientCarrierCode;
        }

        void setIATAcode( const string& theIataCode ) {
            iataCode = upperc( theIataCode );
            if ( iataCode.size() > 35 ) iataCode.resize( 35 );
        }
        const string& getIATAcode() const {
            return iataCode;
        }


        bool isSenderNameSet() const {
            return !senderName.empty();
        }


    private:
        // Name of the company responsible for sending the information
        /* maxlen = 35 */
        /* required = M */
        string senderName;

        // Carrier Code of the sender company
        /* maxlen = 4 */
        /* required = C */
        string senderCarrierCode;

        // Carrier Code of the recipient company
        /* maxlen = 4 */
        /* required = C */
        string recipientCarrierCode;

        // IATA flight code. Example: OK0012/070915/1210
        /* maxlen = 35 */
        /* required = C */
        string iataCode;
    };


    class FlightInfo
    {
    public:
        // Constructor
        FlightInfo()
        {
            flight = "";
            departureAirport = "";
            departureDate = ASTRA::NoExists;
            departureDateSetFlag = false;
            arrivalAirport = "";
            arrivalDate = ASTRA::NoExists;
            arrivalDateSetFlag = false;
        }

        // Getters/Setters
        void setFlight( const string& theFlight ) {
            flight = upperc( theFlight );
            if ( flight.size() > 17 ) flight.resize( 17 );
        }
        const string& getFlight() const {
            return flight;
        }

        void setDepartureAirport( const string& theDepartureAirport ) {
            departureAirport = upperc( theDepartureAirport );
            if ( departureAirport.size() > 25 ) departureAirport.resize( 25 );
        }
        const string& getDepartureAirport() const {
            return departureAirport;
        }

        void setArrivalAirport( const string& theArrivalAirport ) {
            arrivalAirport = upperc( theArrivalAirport );
            if ( arrivalAirport.size() > 25 ) arrivalAirport.resize( 25 );
        }
        const string& getArrivalAirport() const {
            return arrivalAirport;
        }

        void setDepartureDate( const BASIC::TDateTime& theDepartureDate ) {
            departureDate = theDepartureDate; departureDateSetFlag = true;
        }
        const BASIC::TDateTime& getDepartureDate() const {
            return departureDate;
        }

        void setArrivalDate( const BASIC::TDateTime& theArrivalDate ) {
            arrivalDate = theArrivalDate; arrivalDateSetFlag = true;
        }
        const BASIC::TDateTime& getArrivalDate() const {
            return arrivalDate;
        }



        // Other methods
        bool isDepartureDateSet() const {
            return departureDateSetFlag;
        }

        bool isDepartureAirportSet() const {
            return !departureAirport.empty();
        }

        bool isArrivalDateSet() const {
            return arrivalDateSetFlag;
        }

        bool isArrivalAirportSet() const {
            return !arrivalAirport.empty();
        }

        bool isFlightSet() const {
            return !flight.empty();
        }

    private:
        // Carrier Code/Flight Number. For example: OK051
        /* maxlen = 17 */
        /* required = C */
        string flight;

        // Flight departure Airport. Three-character IATA Code
        /* maxlen = 25 */
        /* required = C */
        string departureAirport;

        // Departure Flight date and time
        /* required = C */
        BASIC::TDateTime departureDate;
        bool departureDateSetFlag;

        // Flight arrival Airport. Three-character IATA Code
        /* maxlen = 25 */
        /* required = C */
        string arrivalAirport;

        // Arrival Flight date and time
        /* required = C */
        BASIC::TDateTime arrivalDate;
        bool arrivalDateSetFlag;
    };


    class PartyInfo
    {
    public:
        // Constructor
        PartyInfo()
        {
            partyName = "";
            phone = "";
            fax = "";
        }

        // Gettes/Setters
        void setPartyName( const string& thePartyName ) {
            partyName = upperc( thePartyName );
            if ( partyName.size() > 35 ) partyName.resize( 35 );
        }
        const string& getPartyName() const {
            return partyName;
        }

        void setPhone( const string& thePhone ) {
            phone = upperc( thePhone );
            if ( phone.size() > 25 ) phone.resize( 25 );
        }
        const string& getPhone() const {
            return phone;
        }

        void setFax( const string& theFax ) {
            fax = upperc( theFax );
            if ( fax.size() > 25 ) fax.resize( 25 );
        }
        const string& getFax() const {
            return fax;
        }


        // Other methods
        bool isPhoneAndFaxSet() const {
            return ( !phone.empty() && !fax.empty() );
        }

        bool isPartyNameSet() const {
            return !partyName.empty();
        }

    private:
        // Full name of the company
        /* maxlen = 35 */
        string partyName;

        // Next pair of values (phone and fax) may be empty string

        // Telephone number of the company
        /* maxlen = 25 */
        string phone;

        // Fax number of the company
        /* maxlen = 25 */
        string fax;

    };


    class PassengerInfo
    {
        friend class PaxlstInfo;

    public:
        // Constructor
        PassengerInfo()
        {
            passengerName = "";
            passengerSurname = "";
            passengerSex = "";
            passengerCity = "";
            passengerStreet = "";
            birthDate = ASTRA::NoExists;
            birthDateSetFlag = false;
            departurePassenger = "";
            arrivalPassenger = "";
            passengerCountry = "";
            passengerNumber = "";
            passengerType = "";
            idNumber = "";
            expirateDate = ASTRA::NoExists;
            expirateDateSetFlag = false;
            docCountry = "";
        }


        // Getters/Setters
        void setPassengerName( const string& thePassengerName ) {
            passengerName = upperc( thePassengerName );
            if ( passengerName.size() > 35 ) passengerName.resize( 35 );
        }
        const string& getPassengerName() const {
            return passengerName;
        }

        void setPassengerSurname( const string& thePassengerSurname ) {
            passengerSurname = upperc( thePassengerSurname );
            if ( passengerSurname.size() > 35 ) passengerSurname.resize( 35 );
        }
        const string& getPassengerSurname() const {
            return passengerSurname;
        }

        void setPassengerSex( const string& thePassengerSex ) {
            passengerSex = upperc( thePassengerSex );
            if ( passengerSex.size() > 17 ) passengerSex.resize( 17 );
        }
        const string& getPassengerSex() const {
            return passengerSex;
        }

        void setPassengerCity( const string& thePassengerCity ) {
            passengerCity = upperc( thePassengerCity );
            if ( passengerCity.size() > 35 ) passengerCity.resize( 35 );
        }
        const string& getPassengerCity() const {
            return passengerCity;
        }

        void setPassengerStreet( const string& thePassengerStreet ) {
            passengerStreet = upperc( thePassengerStreet );
            if ( passengerStreet.size() > 35 ) passengerStreet.resize( 35 );
        }
        const string& getPassengerStreet() const {
            return passengerStreet;
        }

        void setBirthDate( const BASIC::TDateTime& theBirthDate ) {
            birthDate = theBirthDate; birthDateSetFlag = true;
        }
        const BASIC::TDateTime& getBirthDate() const {
            return birthDate;
        }

        void setDeparturePassenger( const string& theDeparturePassenger ) {
            departurePassenger = upperc( theDeparturePassenger );
            if ( departurePassenger.size() > 25 ) departurePassenger.resize( 25 );
        }
        const string& getDeparturePassenger() const {
            return departurePassenger;
        }

        void setArrivalPassenger( const string& theArrivalPassenger ) {
            arrivalPassenger = upperc( theArrivalPassenger );
            if ( arrivalPassenger.size() > 25 ) arrivalPassenger.resize( 25 );
        }
        const string& getArrivalPassenger() const {
            return arrivalPassenger;
        }

        void setPassengerCountry( const string& thePassengerCountry ) {
            passengerCountry = upperc( thePassengerCountry );
            if ( passengerCountry.size() > 3 ) passengerCountry.resize( 3 );
        }
        const string& getPassengerCountry() const {
            return passengerCountry;
        }

        void setPassengerNumber( const string& thePassengerNumber ) {
            passengerNumber = upperc( thePassengerNumber );
            if ( passengerNumber.size() > 35 ) passengerNumber.resize( 35 );
        }
        const string& getPassengerNumber() const {
            return passengerNumber;
        }

        void setPassengerType( const string& thePassengerType ) {
            passengerType = upperc( thePassengerType );
            if ( passengerType.size() > 3 ) passengerType.resize( 3 );
        }
        const string& getPassengerType() const {
            return passengerType;
        }

        void setIdNumber( const string& theIdNumber ) {
            idNumber = upperc( theIdNumber );
            if ( idNumber.size() > 35 ) idNumber.resize( 35 );
        }
        const string& getIdNumber() const {
            return idNumber;
        }

        void setExpirateDate( const BASIC::TDateTime& theExpirateDate ) {
            expirateDate = theExpirateDate; expirateDateSetFlag = true;
        }
        const BASIC::TDateTime& getExpirateDate() const {
            return expirateDate;
        }

        void setDocCountry( const string& theDocCountry ) {
            docCountry = upperc( theDocCountry );
            if ( docCountry.size() > 25 )docCountry.resize( 25 );
        }
        const string& getDocCountry() const {
            return docCountry;
        }


        // Other methods
        bool isDocCountrySet() const {
            return !docCountry.empty();
        }

        bool isPassengerSexSet() const {
            return !passengerSex.empty();
        }

        bool isDeparturePassengerSet() const {
            return !departurePassenger.empty();
        }

        bool isArrivalPassengerSet() const {
            return !arrivalPassenger.empty();
        }

        bool isBirthDateSet() const {
            return birthDateSetFlag;
        }

        bool isPassengerCountrySet() const {
            return !passengerCountry.empty();
        }

        bool isPassengerNumberSet() const {
            return !passengerNumber.empty();
        }

        bool isPassengerTypeOrIdNumberSet() const {
            return ( !passengerType.empty() || !idNumber.empty() );
        }

        bool isPassengerTypeSet() const {
            return !passengerType.empty();
        }

        bool isIdNumberSet() const {
            return !idNumber.empty();
        }

        bool isExpirateDateSet() const {
            return expirateDateSetFlag;
        }

        bool isPassengerSurnameSet() const {
            return !passengerSurname.empty();
        }

        bool isPassengerNameSet() const {
            return !passengerName.empty();
        }

        bool isPassengerCitySet() const {
            return !passengerCity.empty();
        }

        bool isPassengerStreetSet() const {
            return !passengerStreet.empty();
        }

    private:
        // Contains the passenger's personal data. As a minimum,
        // the name and surname should appear. These data can include
        // all passenger's personal data, or omit some of them
        /* maxlen = 35 */
        /* required = C */
        string passengerName;
        string passengerSurname;

        // Passenger's Gender. One character. Validity includes:
        // M = Male
        // F = Female
        /* maxlen = 17 */
        /* required = C */
        string passengerSex;

        // Passenger's City
        /* maxlen = 35 */
        /* required = C */
        string passengerCity;

        // Passenger's Street
        /* maxlen = 35 */
        /* required = C */
        string passengerStreet;

        // Passenger's date of birth
        /* required = C */
        BASIC::TDateTime birthDate;
        bool birthDateSetFlag;

        // Passenger's departure airport. Three-character IATA code
        /* maxlen = 25 */
        /* required = C */
        string departurePassenger;

        // Passenger's arrival airport. Three-character IATA code
        /* maxlen = 25 */
        /* required = C */
        string arrivalPassenger;

        // Passenger Nationality. Three-character country code for
        // passenger's country, as per ISO 3166
        /* maxlen = 3 */
        /* required = C */
        string passengerCountry;

        // Flight passenger reservation number. 35 characters maximum
        /* maxlen = 35 */
        /* required = C */
        string passengerNumber;

        // Passenger type
        /* maxlen = 3 */
        /* required = C */
        string passengerType;

        // Unique number assigned to the identification document
        // produced by the passenger
        /* maxlen = 35 */
        /* required = C */
        string idNumber;

        // Expiration date of the identification document produced
        // by the passenger
        /* required = C */
        BASIC::TDateTime expirateDate;
        bool expirateDateSetFlag;

        // Country code where the produced document is used, as per ISO 3166
        /* maxlen = 25 */
        /* required = C */
        string docCountry;
    };


    class PaxlstInfo: public GeneralInfo, public PartyInfo, public FlightInfo
    {
    public:
        // Constructor
        PaxlstInfo()
        {
        }

        // Getters/Setters
        const list< PassengerInfo >& getPassengersList() const {
            return passengersList;
        }

        void addPassenger( const PassengerInfo& passInfo ) {
            passengersList.push_back( passInfo );
        }

        // Other methods
        bool toEdiString( string& out, string& err ) const
        {
            err = "";
            try
            {
                out = CreateEdiPaxlstString( *this );
            }
            catch( PaxlstException& e )
            {
                err = e.errMsg();
                return false;
            }

            return true;
        }

    private:
        // list of passengers
        list< PassengerInfo > passengersList;
    };

} // namespace Paxlst




#endif//_CZECH_POLICE_EDI_FILE_H_
