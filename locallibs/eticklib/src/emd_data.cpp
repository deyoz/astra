#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

#include "etick/emd_data.h"


const char* Ticketing::RficTypeList::ElemName = "RFIC type";

#define ADD_ELEMENT( El, Cl, Dl, Dr ) \
    addElem( VTypes,  Ticketing::RficTypeList( Ticketing::RficType::El, Cl, Dl, Dr ) );

DESCRIBE_CODE_SET( Ticketing::RficTypeList )
{
    ADD_ELEMENT( AirTransportation,     "A", "Air transportation",                      "������ॢ����" );
    ADD_ELEMENT( SurfaceTransportation, "B", "Surface transportation/Non air services", "������� �࠭ᯮ��" );
    ADD_ELEMENT( Baggage,               "C", "Baggage",                                 "�����" );
    ADD_ELEMENT( FinancialImpact,       "D", "Financial impact",                        "�����ᮢ� ��᫥��⢨�" );
    ADD_ELEMENT( AirportServices,       "E", "Airport service",                         "��㣨 � ��ய����" );
    ADD_ELEMENT( Merchandise,           "F", "Merchandise",                             "��࣮���" );
    ADD_ELEMENT( InFlightService,       "G", "In-flight service",                       "��㣨 �� �६� �����" );
    ADD_ELEMENT( IndividualAirlineUse,  "I", "Individual airline use",                  "��� ������������" );
    ADD_ELEMENT( TchService,            "�", "Tch service",                             "��㣠 �� ���" );
}

#undef ADD_ELEMENT


const char* Ticketing::ServiceTypeList::ElemName = "Service type";

#define ADD_ELEMENT( El, Cl, Dl, Dr ) \
    addElem( VTypes,  Ticketing::ServiceTypeList( Ticketing::ServiceType::El, Cl, Dl, Dr ) );

DESCRIBE_CODE_SET( Ticketing::ServiceTypeList )
{
    ADD_ELEMENT( BaggageAllowance,        "A", "Baggage allowance",          "��ଠ ������" );
    ADD_ELEMENT( CarryOnBaggageAllowance, "B", "Carry on baggage allowance", "��筠� �����" );
    ADD_ELEMENT( BaggageCharge,           "C", "Baggage charges",            "����孮ଠ⨢�� �����" );
    ADD_ELEMENT( Embargoes,               "E", "Embargoes",                  "������" );
    ADD_ELEMENT( FlightRelated,           "F", "Flight related",             "��㣨 �� �६� �����" );
    ADD_ELEMENT( Merchandise,             "M", "Merchandise",                "������ᨬ�� �த���" );
    ADD_ELEMENT( PrepaidBaggage,          "P", "Prepaid baggage",            "�।����祭�� �����" );
    ADD_ELEMENT( RuleBuster,              "R", "Rule buster",                "��������� �ࠢ�� ������/������" );
    ADD_ELEMENT( TicketRelated,           "T", "Ticket related",             "��㣠, �易���� � ����⮬" );
}

#undef ADD_ELEMENT


const char* Ticketing::ServiceQualifierList::ElemName = "Service qualifier";

#define ADD_ELEMENT( El, Cl, Dl, Dr ) \
    addElem( VTypes,  Ticketing::ServiceQualifierList( Ticketing::ServiceQualifier::El, Cl, Dl, Dr ) );

DESCRIBE_CODE_SET( Ticketing::ServiceQualifierList )
{
    ADD_ELEMENT( IndustryDefined,          "1", "Industry defined",    "�।�ᬮ�७� � ������ਨ" );
    ADD_ELEMENT( CarrierDefined,           "2", "Defined by carrier",  "�।�ᬮ�७� ��ॢ��稪��" );
}

#undef ADD_ELEMENT
