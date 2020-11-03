//
// C++ Interface: emd_data
//
// Description:
//
//
// Author: Anton Bokov <a.bokov@sirena2000.ru>, (C) 2011
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef _ETICK_EMD_DATA_H_
#define _ETICK_EMD_DATA_H_

#include <string>
#include <serverlib/base_code_set.h>

namespace Ticketing
{

class RficTypeList: public BaseTypeElem< int >
{
    typedef BaseTypeElem<int> CodeListData_t;
public:
    static const char *ElemName;
    RficTypeList( int codeI, const char *actCd,
                  const char *ldesc, const char *rdesc ) throw()
        : CodeListData_t( codeI, actCd, actCd, rdesc, ldesc )
    {}
    virtual ~RficTypeList() {}
};


class RficType: public BaseTypeElemHolder< RficTypeList >
{
public:
    typedef BaseTypeElemHolder< RficTypeList > TypeElemHolder;

    enum RficType_t
    {
        AirTransportation = 1,
        SurfaceTransportation = 2,
        Baggage = 3,
        FinancialImpact = 4,
        AirportServices = 5,
        Merchandise = 6,
        InFlightService = 7,
        IndividualAirlineUse = 8,
        TchService = 9,
        TchMcoBaggage = 10
    };

    RficType()
        : TypeElemHolder()
    {}
    RficType( const TypeElemHolder& base )
        : TypeElemHolder( base )
    {}
    RficType( RficType_t t )
        : TypeElemHolder( int(t) )
    {}
    RficType( int t )
        : TypeElemHolder( t )
    {}
    RficType( const std::string& status )
        : TypeElemHolder( status )
    {}
    RficType( const char* status )
        : TypeElemHolder( status )
    {}
};


class ServiceTypeList: public BaseTypeElem< int >
{
    typedef BaseTypeElem<int> CodeListData_t;
public:
    static const char *ElemName;
    ServiceTypeList( int codeI, const char *actCd,
                  const char *ldesc, const char *rdesc ) throw()
        : CodeListData_t( codeI, actCd, actCd, rdesc, ldesc )
    {}
    virtual ~ServiceTypeList() {}
};


class ServiceType: public BaseTypeElemHolder< ServiceTypeList >
{
public:
    typedef BaseTypeElemHolder< ServiceTypeList > TypeElemHolder;

    enum ServiceType_t: int {
        BaggageAllowance = 1,
        CarryOnBaggageAllowance = 2,
        BaggageCharge = 3,
        Embargoes = 4,
        FlightRelated = 5,
        Merchandise = 6,
        PrepaidBaggage = 7,
        RuleBuster = 8,
        TicketRelated = 9
    };

    ServiceType()
        : TypeElemHolder()
    {}
    ServiceType( const TypeElemHolder& base )
        : TypeElemHolder( base )
    {}
    ServiceType( int t )
        : TypeElemHolder( t )
    {}
    ServiceType( ServiceType_t t )
        : TypeElemHolder( int(t) )
    {}
    ServiceType( const std::string& status )
        : TypeElemHolder( status )
    {}
    ServiceType( const char* status )
        : TypeElemHolder( status )
    {}
};


class ServiceQualifierList: public BaseTypeElem< int >
{
    typedef BaseTypeElem<int> CodeListData_t;
public:
    static const char *ElemName;
    ServiceQualifierList( int codeI, const char *actCd,
                  const char *ldesc, const char *rdesc ) throw()
        : CodeListData_t( codeI, actCd, actCd, rdesc, ldesc )
    {}
    virtual ~ServiceQualifierList() {}
};

class ServiceQualifier: public BaseTypeElemHolder< ServiceQualifierList >
{
public:
    typedef BaseTypeElemHolder< ServiceQualifierList > TypeElemHolder;

    enum ServiceQualifier_t: int {
        IndustryDefined = 1,
        CarrierDefined = 2
    };

    ServiceQualifier()
        : TypeElemHolder()
    {}
    ServiceQualifier( const TypeElemHolder& base )
        : TypeElemHolder( base )
    {}
    ServiceQualifier( int t )
        : TypeElemHolder( t )
    {}
    ServiceQualifier( ServiceQualifier_t t )
        : TypeElemHolder( int(t) )
    {}
    ServiceQualifier( const std::string& status )
        : TypeElemHolder( status )
    {}
    ServiceQualifier( const char* status )
        : TypeElemHolder( status )
    {}
};

}//namespace Ticketing

#endif /*_ETICK_EMD_DATA_H_*/
