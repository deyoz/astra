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


namespace Paxlst
{
    using std::string;
    using std::list;
    using std::exception;


    struct PaxlstInfo;

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


    struct GeneralInfo
    {
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

        bool isSenderNameSet() const
        {
            return !senderName.empty();
        }
    };


    struct FlightInfo
    {
        FlightInfo()
        {
            // "01.01.86" - the departureDate that is 99.99% not set
            BASIC::StrToDateTime( "01.01.86 00:00:00", departureDate );

            // "01.01.86" - the arrivalDate that is 99.99% not set
            BASIC::StrToDateTime( "01.01.86 00:00:00", arrivalDate );
        }

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

        // Flight arrival Airport. Three-character IATA Code
        /* maxlen = 25 */
        /* required = C */
        string arrivalAirport;

        // Arrival Flight date and time
        /* required = C */
        BASIC::TDateTime arrivalDate;


        bool isDepartureDateSet() const
        {
            return BASIC::DateTimeToStr( departureDate, "yymmdd" ) != "860101";
        }

        bool isDepartureAirportSet() const
        {
            return !departureAirport.empty();
        }

        bool isArrivalDateSet() const
        {
            return BASIC::DateTimeToStr( arrivalDate, "yymmdd" ) != "860101";
        }

        bool isArrivalAirportSet() const
        {
            return !arrivalAirport.empty();
        }

        bool isFlightSet() const
        {
            return !flight.empty();
        }
    };


    struct PartyInfo
    {
        // Full name of the company
        /* maxlen = 35 */
        /* required = M */
        string partyName;


        // Next pair of values (phone and fax) may be empty string

        // Telephone number of the company
        /* maxlen = 25 */
        string phone;

        // Fax number of the company
        /* maxlen = 25 */
        string fax;


        bool isPhoneAndFaxSet() const
        {
            return ( !phone.empty() && !fax.empty() );
        }

        bool isPartyNameSet() const
        {
            return !partyName.empty();
        }
    };


    struct PassengerInfo
    {
        PassengerInfo()
        {
            // "01.01.86" - the expirateDate that is 99.99% not set
            BASIC::StrToDateTime( "01.01.86 00:00:00", expirateDate );

            // "01.01.86" - the birthDate that is 99.99% not set
            BASIC::StrToDateTime( "01.01.86 00:00:00", birthDate );
        }


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
        /* maxlen = 1 */
        /* required = C */
        string passengerSex;

        // Passenger's date of birth
        /* required = C */
        BASIC::TDateTime birthDate;

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

        // Country code where the produced document is used, as per ISO 3166
        /* maxlen = 25 */
        /* required = C */
        string docCountry;


        bool isDocCountrySet() const
        {
            return !docCountry.empty();
        }

        bool isPassengerSexSet() const
        {
            return !passengerSex.empty();
        }

        bool isDeparturePassengerSet() const
        {
            return !departurePassenger.empty();
        }

        bool isArrivalPassengerSet() const
        {
            return !arrivalPassenger.empty();
        }

        bool isBirthDateSet() const
        {
            return BASIC::DateTimeToStr( birthDate, "yymmdd" ) != "860101";
        }

        bool isPassengerCountrySet() const
        {
            return !passengerCountry.empty();
        }

        bool isPassengerNumberSet() const
        {
            return !passengerNumber.empty();
        }

        bool isPassengerTypeOrIdNumberSet() const
        {
            return ( !passengerType.empty() || !idNumber.empty() );
        }

        bool isPassengerTypeSet() const
        {
            return !passengerType.empty();
        }

        bool isIdNumberSet() const
        {
            return !idNumber.empty();
        }

        bool isExpirateDateSet() const
        {
            return BASIC::DateTimeToStr( expirateDate, "yymmdd" ) != "860101";
        }

        bool isPassengerSurnameSet() const
        {
            return !passengerSurname.empty();
        }

        bool isPassengerNameSet() const
        {
            return !passengerName.empty();
        }

    };


    struct PaxlstInfo: public GeneralInfo, PartyInfo, FlightInfo
    {
        // list of passengers
        list< PassengerInfo > passangersList;


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

    };

} // namespace Paxlst


#endif//_CZECH_POLICE_EDI_FILE_H_
