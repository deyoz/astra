#ifndef CORETYPES_RFISC_H
#define CORETYPES_RFISC_H

#include <iosfwd>
#include <serverlib/rip.h>
#include <serverlib/rip_validators.h>
#include <serverlib/enum.h>

namespace ct
{

DECL_RIP_REGEX(RfiscSubCode, std::string, "([A-Z0-9]{3})");
DECL_RIP_REGEX(RfiscCommercialName, std::string, "([A-Z 0-9]{1,30})");
DECL_RIP_REGEX(RfiscGroupCode, std::string, "([A-Z]{2})");
DECL_RIP_REGEX(RfiscSubGroupCode, std::string, "([A-Z0-9]{2})");

enum class Rfic { A = 1, B, C, D, E, F, G, I };
ENUM_NAMES_DECL(Rfic);

struct RfiscGroup
{
    RfiscGroupCode code;
    std::string name;

    RfiscGroup(const RfiscGroupCode&, const std::string&);
};
bool operator==(const RfiscGroup&, const RfiscGroup&);
std::ostream& operator<<(std::ostream&, const RfiscGroup&);

const std::vector<RfiscGroup>& rfiscGroups();

struct RfiscSubGroup
{
    RfiscSubGroupCode code;
    std::string name;
    RfiscGroupCode groupCode;

    RfiscSubGroup(const RfiscSubGroupCode&, const std::string&, const RfiscGroupCode&);
};
bool operator==(const RfiscSubGroup&, const RfiscSubGroup&);
std::ostream& operator<<(std::ostream&, const RfiscSubGroup&);

const std::vector<RfiscSubGroup>& rfiscSubGroups();

} // ct

#endif /* CORETYPES_RFISC_H */
