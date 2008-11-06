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

    string CreateEdiPaxlstFileName( const string& flightNumber,
                                    const string& origin,
                                    const string& destination,
                                    const string& departureDate,
                                    const string& ext );

    string CreateEdiInterchangeReference();


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
        string senderName;

        // Carrier Code of the sender company
        /* maxlen = 4 */
        string senderCarrierCode;

        // Carrier Code of the recipient company
        /* maxlen = 4 */
        string recipientCarrierCode;

        // IATA flight code. Example: OK0012/070915/1210
        /* maxlen = 35 */
        string iataCode;
    };


    struct FlightInfo
    {
        // Carrier Code/Flight Number. For example: OK051
        /* maxlen = 17 */
        string flight;

        // Flight departure Airport. Three-character IATA Code
        /* maxlen = 25 */
        string departureAirport;

        // Departure Flight date and time
        BASIC::TDateTime departureDate;

        // Flight arrival Airport. Three-character IATA Code
        /* maxlen = 25 */
        string arrivalAirport;

        // Arrival Flight date and time
        BASIC::TDateTime arrivalDate;
    };


    struct PartyInfo
    {
        // Full name of the company
        /* maxlen = 35 */
        string partyName;

        // Telephone number of the company
        /* maxlen = 25 */
        string phone;

        // Fax number of the company
        /* maxlen = 25 */
        string fax;
    };


    struct PassengerInfo
    {
        PassengerInfo()
        {
            // "01.01.86" - the expiratedate that is not set
            BASIC::StrToDateTime( "01.01.86 00:00:00", expirateDate );
        }


        // Contains the passenger's personal data. As a minimum,
        // the name and surname should appear. These data can include
        // all passenger's personal data, or omit some of them
        /* maxlen = 35 */
        string passengerName;
        string passengerSurname;

        // Passenger's Gender. One character. Validity includes:
        // M = Male
        // F = Female
        string passengerSex;

        // Passenger's date of birth
        BASIC::TDateTime birthDate;

        // Passenger's departure airport. Three-character IATA code
        /* maxlen = 25 */
        string departurePassenger;

        // Passenger's arrival airport. Three-character IATA code
        /* maxlen = 25 */
        string arrivalPassenger;

        // Passenger Nationality. Three-character country code for
        // passenger's country, as per ISO 3166
        /* maxlen = 3 */
        string passengerCountry;

        // Flight passenger reservation number. 35 characters maximum
        /* maxlen = 35 */
        string passengerNumber;

        // Passenger type
        /* maxlen = 3 */
        string passengerType;

        // Unique number assigned to the identification document
        // produced by the passenger
        /* maxlen = 35 */
        string idNumber;

        // Expiration date of the identification document produced
        // by the passenger
        BASIC::TDateTime expirateDate;

        // Country code where the produced document is used, as per ISO 3166
        /* maxlen = 25 */
        string docCountry;


        bool isExpirateDateSet() const
        {
            return BASIC::DateTimeToStr( expirateDate, "yymmdd" ) != "860101";
        }

        bool isDocCountrySet() const
        {
            return !docCountry.empty();
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
