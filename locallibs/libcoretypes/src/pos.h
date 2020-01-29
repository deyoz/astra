#ifndef CORETYPES_POS_H
#define CORETYPES_POS_H

#include <serverlib/encstring.h>
#include <serverlib/rip_validators.h>
#include <libnsi/nsi.h>

namespace ct
{

DECL_RIP_LENGTH(Agency, EncString, 1, 9);
DECL_RIP_LENGTH(Ppr, EncString, 7, 8);
DECL_RIP_LENGTH(Pult, EncString, 6, 6);
DECL_RIP_LENGTH(Operator, EncString, 1, 12);

struct PointOfSale
{
    ct::Agency agency;
    boost::optional<ct::Ppr> ppr;
    boost::optional<nsi::CompanyId> airline;
    EncString closestCityPort;
    EncString userType;
    EncString country;
    EncString currency;
    EncString dutyCode;
    EncString erspId;
    EncString firstDep;
    EncString pcc;
    boost::optional<std::pair<Pult, Operator>> pultOpr;
    PointOfSale(const ct::Agency&);
};
bool operator==(const PointOfSale&, const PointOfSale&);
bool operator!=(const PointOfSale&, const PointOfSale&);
bool operator<(const PointOfSale&, const PointOfSale&);
std::ostream& operator<<(std::ostream&, const PointOfSale&);

} // ct

#endif /* CORETYPES_POS_H */
