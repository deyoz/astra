#include "rfisc.h"

#define NICKNAME "NONSTOP"
#include <serverlib/slogger.h>

namespace ct
{

ENUM_NAMES_BEGIN(Rfic)
    (Rfic::A, "A")
    (Rfic::B, "B")
    (Rfic::C, "C")
    (Rfic::D, "D")
    (Rfic::E, "E")
    (Rfic::F, "F")
    (Rfic::G, "G")
    (Rfic::I, "I")
ENUM_NAMES_END(Rfic)

RfiscGroup::RfiscGroup(const RfiscGroupCode& i_code, const std::string& i_name)
    : code(i_code), name(i_name)
{}

bool operator==(const RfiscGroup& lhs, const RfiscGroup& rhs)
{
    return lhs.code == rhs.code
        && lhs.name == rhs.name;
}

std::ostream& operator<<(std::ostream& os, const RfiscGroup& r)
{
    return os << r.code << ' ' << r.name;
}

const std::vector<RfiscGroup>& rfiscGroups()
{
    static const std::vector<RfiscGroup> groups  = {
        RfiscGroup(ct::RfiscGroupCode("BD"), "Bundled Service"),
        RfiscGroup(ct::RfiscGroupCode("BG"), "Baggage"),
        RfiscGroup(ct::RfiscGroupCode("CO"), "Carbon Offset"),
        RfiscGroup(ct::RfiscGroupCode("FF"), "Frequent Flyer"),
        RfiscGroup(ct::RfiscGroupCode("GT"), "Ground Transportation and Non Air Services"),
        RfiscGroup(ct::RfiscGroupCode("IE"), "In-flight Entertainment"),
        RfiscGroup(ct::RfiscGroupCode("LG"), "Lounge"),
        RfiscGroup(ct::RfiscGroupCode("MD"), "Medical"),
        RfiscGroup(ct::RfiscGroupCode("ML"), "Meal/Beverage"),
        RfiscGroup(ct::RfiscGroupCode("PT"), "Pets"),
        RfiscGroup(ct::RfiscGroupCode("RO"), "Rule Override"),
        RfiscGroup(ct::RfiscGroupCode("SA"), "Pre-reserved Seat Assignment"),
        RfiscGroup(ct::RfiscGroupCode("SB"), "Standby"),
        RfiscGroup(ct::RfiscGroupCode("ST"), "Store"),
        RfiscGroup(ct::RfiscGroupCode("TS"), "Travel Services"),
        RfiscGroup(ct::RfiscGroupCode("UN"), "Unaccompanied Travel (Escort)"),
        RfiscGroup(ct::RfiscGroupCode("UU"), "Unaccompanied Travel (Unescorted)"),
        RfiscGroup(ct::RfiscGroupCode("UP"), "Upgrades")
    };
    return groups;
}

RfiscSubGroup::RfiscSubGroup(const RfiscSubGroupCode& i_code, const std::string& i_name, const RfiscGroupCode& i_grp)
    : code(i_code), name(i_name), groupCode(i_grp)
{}

bool operator==(const RfiscSubGroup& lhs, const RfiscSubGroup& rhs)
{
    return lhs.code == rhs.code
        && lhs.groupCode == rhs.groupCode
        && lhs.name == rhs.name;
}

std::ostream& operator<<(std::ostream& os, const RfiscSubGroup& r)
{
    return os << r.code << ' ' << r.groupCode << ' ' << r.name;
}

const std::vector<RfiscSubGroup>& rfiscSubGroups()
{
    static const std::vector<RfiscSubGroup> subGroups = {
        RfiscSubGroup(ct::RfiscSubGroupCode("XS"), "Baggage Excess",            ct::RfiscGroupCode("BG")),
        RfiscSubGroup(ct::RfiscSubGroupCode("AD"), "Assistive Devices",         ct::RfiscGroupCode("BG")),
        RfiscSubGroup(ct::RfiscSubGroupCode("CY"), "Carry On Hand Baggage",     ct::RfiscGroupCode("BG")),
        RfiscSubGroup(ct::RfiscSubGroupCode("IN"), "Infant Baggage",            ct::RfiscGroupCode("BG")),
        RfiscSubGroup(ct::RfiscSubGroupCode("MI"), "Musical Instruments",       ct::RfiscGroupCode("BG")),
        RfiscSubGroup(ct::RfiscSubGroupCode("PN"), "Pet in Hold",               ct::RfiscGroupCode("BG")),
        RfiscSubGroup(ct::RfiscSubGroupCode("SI"), "Specialty Item",            ct::RfiscGroupCode("BG")),
        RfiscSubGroup(ct::RfiscSubGroupCode("SP"), "Sporting Equipment",        ct::RfiscGroupCode("BG")),
        RfiscSubGroup(ct::RfiscSubGroupCode("PN"), "Pet in Hold",               ct::RfiscGroupCode("BG")),
        RfiscSubGroup(ct::RfiscSubGroupCode("MG"), "Mileage Accrual",           ct::RfiscGroupCode("FF")),
        RfiscSubGroup(ct::RfiscSubGroupCode("BU"), "Bus",                       ct::RfiscGroupCode("GT")),
        RfiscSubGroup(ct::RfiscSubGroupCode("EC"), "Electric Cart",             ct::RfiscGroupCode("GT")),
        RfiscSubGroup(ct::RfiscSubGroupCode("HT"), "Hotel",                     ct::RfiscGroupCode("GT")),
        RfiscSubGroup(ct::RfiscSubGroupCode("LI"), "Limo",                      ct::RfiscGroupCode("GT")),
        RfiscSubGroup(ct::RfiscSubGroupCode("PK"), "Parking",                   ct::RfiscGroupCode("GT")),
        RfiscSubGroup(ct::RfiscSubGroupCode("TF"), "Transfer",                  ct::RfiscGroupCode("GT")),
        RfiscSubGroup(ct::RfiscSubGroupCode("TN"), "Train",                     ct::RfiscGroupCode("GT")),
        RfiscSubGroup(ct::RfiscSubGroupCode("TU"), "Tour",                      ct::RfiscGroupCode("GT")),
        RfiscSubGroup(ct::RfiscSubGroupCode("VG"), "Video Games",               ct::RfiscGroupCode("IE")),
        RfiscSubGroup(ct::RfiscSubGroupCode("DP"), "Media Download",            ct::RfiscGroupCode("IE")),
        RfiscSubGroup(ct::RfiscSubGroupCode("FP"), "Fun Pack",                  ct::RfiscGroupCode("IE")),
        RfiscSubGroup(ct::RfiscSubGroupCode("HS"), "Headset",                   ct::RfiscGroupCode("IE")),
        RfiscSubGroup(ct::RfiscSubGroupCode("IT"), "Internet Access",           ct::RfiscGroupCode("IE")),
        RfiscSubGroup(ct::RfiscSubGroupCode("PB"), "Pillow/Blanket",            ct::RfiscGroupCode("IE")),
        RfiscSubGroup(ct::RfiscSubGroupCode("TL"), "Toiletries",                ct::RfiscGroupCode("IE")),
        RfiscSubGroup(ct::RfiscSubGroupCode("OE"), "Overhead Entertainment",    ct::RfiscGroupCode("IE")),
        RfiscSubGroup(ct::RfiscSubGroupCode("PE"), "Personal Entertainment",    ct::RfiscGroupCode("IE")),
        RfiscSubGroup(ct::RfiscSubGroupCode("AC"), "110V AC Power",             ct::RfiscGroupCode("IE")),
        RfiscSubGroup(ct::RfiscSubGroupCode("US"), "USB Power",                 ct::RfiscGroupCode("IE")),
        RfiscSubGroup(ct::RfiscSubGroupCode("LS"), "Live Sattelite Television", ct::RfiscGroupCode("IE")),
        RfiscSubGroup(ct::RfiscSubGroupCode("IR"), "Incubator",                 ct::RfiscGroupCode("MD")),
        RfiscSubGroup(ct::RfiscSubGroupCode("MA"), "Medical Assistance",        ct::RfiscGroupCode("MD")),
        RfiscSubGroup(ct::RfiscSubGroupCode("OX"), "Oxygen",                    ct::RfiscGroupCode("MD")),
        RfiscSubGroup(ct::RfiscSubGroupCode("SC"), "Stretcher",                 ct::RfiscGroupCode("MD")),
        RfiscSubGroup(ct::RfiscSubGroupCode("BR"), "Breakfast",                 ct::RfiscGroupCode("ML")),
        RfiscSubGroup(ct::RfiscSubGroupCode("DI"), "Dinner",                    ct::RfiscGroupCode("ML")),
        RfiscSubGroup(ct::RfiscSubGroupCode("DR"), "Drink",                     ct::RfiscGroupCode("ML")),
        RfiscSubGroup(ct::RfiscSubGroupCode("LU"), "Lunch",                     ct::RfiscGroupCode("ML")),
        RfiscSubGroup(ct::RfiscSubGroupCode("SN"), "Snack",                     ct::RfiscGroupCode("ML")),
        RfiscSubGroup(ct::RfiscSubGroupCode("PC"), "In Cabin",                  ct::RfiscGroupCode("PT")),
        RfiscSubGroup(ct::RfiscSubGroupCode("PH"), "In Hold",                   ct::RfiscGroupCode("PT")),
        RfiscSubGroup(ct::RfiscSubGroupCode("31"), "Category 31 Override",      ct::RfiscGroupCode("RO")),
        RfiscSubGroup(ct::RfiscSubGroupCode("3A"), "Category 31 and 33 Override", ct::RfiscGroupCode("RO")),
        RfiscSubGroup(ct::RfiscSubGroupCode("33"), "Category 33 Override",      ct::RfiscGroupCode("RO")),
        RfiscSubGroup(ct::RfiscSubGroupCode("CF"), "Confirmed",                 ct::RfiscGroupCode("SB")),
        RfiscSubGroup(ct::RfiscSubGroupCode("AP"), "Apparel",                   ct::RfiscGroupCode("ST")),
        RfiscSubGroup(ct::RfiscSubGroupCode("GC"), "Gift Card/Certificate",     ct::RfiscGroupCode("ST")),
        RfiscSubGroup(ct::RfiscSubGroupCode("TY"), "Toys",                      ct::RfiscGroupCode("ST")),
        RfiscSubGroup(ct::RfiscSubGroupCode("CH"), "Charter",                   ct::RfiscGroupCode("TS")),
        RfiscSubGroup(ct::RfiscSubGroupCode("CI"), "Check-in",                  ct::RfiscGroupCode("TS")),
        RfiscSubGroup(ct::RfiscSubGroupCode("DB"), "Denied Boarding Compensation", ct::RfiscGroupCode("TS")),
        RfiscSubGroup(ct::RfiscSubGroupCode("FT"), "Fast Track",                ct::RfiscGroupCode("TS")),
        RfiscSubGroup(ct::RfiscSubGroupCode("GR"), "Group",                     ct::RfiscGroupCode("TS")),
        RfiscSubGroup(ct::RfiscSubGroupCode("LT"), "Lost Ticket",               ct::RfiscGroupCode("TS")),
        RfiscSubGroup(ct::RfiscSubGroupCode("NS"), "No Show Fee",               ct::RfiscGroupCode("TS")),
        RfiscSubGroup(ct::RfiscSubGroupCode("PI"), "Premium Trip Insurance",    ct::RfiscGroupCode("TS")),
        RfiscSubGroup(ct::RfiscSubGroupCode("PD"), "Prepaid",                   ct::RfiscGroupCode("TS")),
        RfiscSubGroup(ct::RfiscSubGroupCode("PR"), "Priority Boarding",         ct::RfiscGroupCode("TS")),
        RfiscSubGroup(ct::RfiscSubGroupCode("PY"), "Priority Baggage",          ct::RfiscGroupCode("TS")),
        RfiscSubGroup(ct::RfiscSubGroupCode("SY"), "Security",                  ct::RfiscGroupCode("TS")),
        RfiscSubGroup(ct::RfiscSubGroupCode("TI"), "Trip Insurance",            ct::RfiscGroupCode("TS")),
        RfiscSubGroup(ct::RfiscSubGroupCode("VI"), "Visa Services",             ct::RfiscGroupCode("TS")),
        RfiscSubGroup(ct::RfiscSubGroupCode("AS"), "Assistance",                ct::RfiscGroupCode("UN")),
        RfiscSubGroup(ct::RfiscSubGroupCode("MR"), "Unaccompanied Minor",       ct::RfiscGroupCode("UN")),
        RfiscSubGroup(ct::RfiscSubGroupCode("SR"), "Unaccompanied Senior",      ct::RfiscGroupCode("UN")),
        RfiscSubGroup(ct::RfiscSubGroupCode("UM"), "Unaccompanied Minor",       ct::RfiscGroupCode("UU")),
        RfiscSubGroup(ct::RfiscSubGroupCode("ME"), "Mileage",                   ct::RfiscGroupCode("UP"))
    };
    return subGroups;
}

} // ct
