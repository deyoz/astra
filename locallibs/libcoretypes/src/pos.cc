#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

#include <serverlib/logopt.h>
#include <serverlib/helpcpp.h>
#include <serverlib/enum.h>

#include "pos.h"

#define NICKNAME "NONSTOP"
#include <serverlib/slogger.h>

namespace ct
{

PointOfSale::PointOfSale(const ct::Agency& a)
    : agency(a)
{
}

bool operator==(const PointOfSale& lhs, const PointOfSale& rhs)
{
    return lhs.agency == rhs.agency
        && lhs.ppr == rhs.ppr
        && lhs.airline == rhs.airline
        && lhs.closestCityPort == rhs.closestCityPort
        && lhs.userType == rhs.userType
        && lhs.country == rhs.country
        && lhs.currency == rhs.currency
        && lhs.dutyCode == rhs.dutyCode
        && lhs.erspId == rhs.erspId
        && lhs.firstDep == rhs.firstDep
        && lhs.pcc == rhs.pcc
        && lhs.pultOpr == rhs.pultOpr;
}

bool operator!=(const PointOfSale& lhs, const PointOfSale& rhs)
{
    return !(lhs == rhs);
}

bool operator<(const PointOfSale& lhs, const PointOfSale& rhs)
{
    FIELD_LESS(lhs.agency, rhs.agency);
    FIELD_LESS(lhs.ppr, rhs.ppr);
    FIELD_LESS(lhs.airline, rhs.airline);
    FIELD_LESS(lhs.closestCityPort, rhs.closestCityPort);
    FIELD_LESS(lhs.userType, rhs.userType);
    FIELD_LESS(lhs.country, rhs.country);
    FIELD_LESS(lhs.currency, rhs.currency);
    FIELD_LESS(lhs.dutyCode, rhs.dutyCode);
    FIELD_LESS(lhs.erspId, rhs.erspId);
    FIELD_LESS(lhs.firstDep, rhs.firstDep);
    FIELD_LESS(lhs.pcc, rhs.pcc);
    return lhs.pultOpr < rhs.pultOpr;
}

std::ostream& operator<<(std::ostream& os, const PointOfSale& pos)
{
    os << pos.agency
        << LogOpt(" ppr=", pos.ppr)
        << LogOpt(" air=", pos.airline)
        << '/' << pos.closestCityPort
        << '/' << pos.userType
        << '/' << pos.country
        << '/' << pos.currency
        << '/' << pos.dutyCode
        << '/' << pos.erspId
        << '/' << pos.firstDep
        << '/' << pos.pcc;
    if (pos.pultOpr) {
        os << " " << pos.pultOpr->first << '/' << pos.pultOpr->second;
    }
    return os;
}

} // ct
