#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

#include "etick/emd_data.h"


const char* Ticketing::RficTypeList::ElemName = "RFIC type";

#define ADD_ELEMENT( El, Cl, Dl, Dr ) \
    addElem( VTypes,  Ticketing::RficTypeList( Ticketing::RficType::El, Cl, Dl, Dr ) );

DESCRIBE_CODE_SET( Ticketing::RficTypeList )
{
    ADD_ELEMENT( AirTransportation,     "A", "Air transportation",                      "Авиаперевозки" );
    ADD_ELEMENT( SurfaceTransportation, "B", "Surface transportation/Non air services", "Наземный транспорт" );
    ADD_ELEMENT( Baggage,               "C", "Baggage",                                 "Багаж" );
    ADD_ELEMENT( FinancialImpact,       "D", "Financial impact",                        "Финансовые последствия" );
    ADD_ELEMENT( AirportServices,       "E", "Airport service",                         "Услуги в аэропорту" );
    ADD_ELEMENT( Merchandise,           "F", "Merchandise",                             "Торговля" );
    ADD_ELEMENT( InFlightService,       "G", "In-flight service",                       "Услуги во время полета" );
    ADD_ELEMENT( IndividualAirlineUse,  "I", "Individual airline use",                  "Для авиакомпании" );
    ADD_ELEMENT( TchService,            "Ц", "Tch service",                             "Услуга от ТКП" );
}

#undef ADD_ELEMENT


const char* Ticketing::ServiceTypeList::ElemName = "Service type";

#define ADD_ELEMENT( El, Cl, Dl, Dr ) \
    addElem( VTypes,  Ticketing::ServiceTypeList( Ticketing::ServiceType::El, Cl, Dl, Dr ) );

DESCRIBE_CODE_SET( Ticketing::ServiceTypeList )
{
    ADD_ELEMENT( BaggageAllowance,        "A", "Baggage allowance",          "Норма багажа" );
    ADD_ELEMENT( CarryOnBaggageAllowance, "B", "Carry on baggage allowance", "Ручная кладь" );
    ADD_ELEMENT( BaggageCharge,           "C", "Baggage charges",            "Сверхнормативный багаж" );
    ADD_ELEMENT( Embargoes,               "E", "Embargoes",                  "Запреты" );
    ADD_ELEMENT( FlightRelated,           "F", "Flight related",             "Услуги во время полета" );
    ADD_ELEMENT( Merchandise,             "M", "Merchandise",                "Независимая продажа" );
    ADD_ELEMENT( PrepaidBaggage,          "P", "Prepaid baggage",            "Предоплаченный багаж" );
    ADD_ELEMENT( RuleBuster,              "R", "Rule buster",                "Изменения правил возврата/обмена" );
    ADD_ELEMENT( TicketRelated,           "T", "Ticket related",             "Услуга, связанная с билетом" );
}

#undef ADD_ELEMENT


const char* Ticketing::ServiceQualifierList::ElemName = "Service qualifier";

#define ADD_ELEMENT( El, Cl, Dl, Dr ) \
    addElem( VTypes,  Ticketing::ServiceQualifierList( Ticketing::ServiceQualifier::El, Cl, Dl, Dr ) );

DESCRIBE_CODE_SET( Ticketing::ServiceQualifierList )
{
    ADD_ELEMENT( IndustryDefined,          "1", "Industry defined",    "Предусмотрено в индустрии" );
    ADD_ELEMENT( CarrierDefined,           "2", "Defined by carrier",  "Предусмотрено перевозчиком" );
}

#undef ADD_ELEMENT
