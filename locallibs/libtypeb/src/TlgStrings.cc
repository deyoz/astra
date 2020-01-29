#ifndef _TLGSTRINGS_H_
#define _TLGSTRINGS_H_
#define NICKNAME "ASH"
#define NICKTRACE ASH_TRACE
#include <string>
#include <serverlib/a_ya_a_z_0_9.h>

namespace typeb_parser {

extern const std::string SsmReferenceString = 
        //date
        "^([\\d]{2}[" A_Z A_YA "]{3})"
        //message group serial number and continuation/end code and msg serial number
        "([\\d]{5})([E…‘C])([\\d]{3})\\s*"
        //creator reference (opt)
       "(/[\\S ]{1,35})?$";

extern const std::string SsmReidRx = "([" ZERO_NINE A_Z A_YA "]{2}[" A_Z A_YA "]?"
                           "[" ZERO_NINE "]{3,4}[" A_Z A_YA "]?)";

extern const std::string SsmDateRx = "(\\d{2}[" A_Z A_YA "]{3}(\\d{2})?)";

extern const std::string SsmPeriodRx = SsmDateRx + "\\s" + SsmDateRx + "\\s([1234567]{1,7})"
                             + "(\\s*/[" A_Z A_YA "]\\d)?"; //freq and optional freq rate

extern const std::string SsmOwnerRx = "[\\d" A_Z A_YA "]{2}[" A_Z A_YA "]?";

extern const std::string SsmDEI1Rx = "1(/" + SsmOwnerRx + "){2,3}";

extern const std::string SsmDEI6Rx = "6/([\\d" A_Z A_YA "]{2}[" A_Z A_YA "]?"
                           "\\d{3,4}[" A_Z A_YA "]?(/\\d)?)";

extern const std::string SsmDEI7Rx = "7("
                           "(/([" A_Z A_YA "]{2,3})){1,5}|"
                           "//[" A_Z A_YA "]{1,2}|"
                           "(/[" A_Z A_YA "]{2,3}){1,5}(//[" A_Z A_YA "]{1,2})?)";

extern const std::string SsmDEINRx = "[234589]/(" + SsmOwnerRx + "|X)";

extern const std::string SsmPrbdRx = "[" A_Z A_YA "][\\d" A_Z A_YA "]*"; //passenger reservation booking designator

extern const std::string SsmPrbmRx = "([" A_Z A_YA "]{2})+"; //... modifier

extern const std::string SsmCraftRx = "\\S+"; //ACV

extern const std::string SsmLegsChangeRx = "[" A_Z A_YA "]{3}(/[" A_Z A_YA "]{3}){1,11}";

extern const std::string SsmFlightString = "^" + SsmReidRx
                         + "(\\s" + SsmDEI1Rx + ")?"
                         + "((\\s" + SsmDEINRx + "){0,5}$)";

extern const std::string SsmRevFlightString = "^" + SsmReidRx + "\\s" + SsmPeriodRx + "$";

extern const std::string SsmLongPeriodString = SsmPeriodRx +  
                         + "(\\s" + SsmDEI1Rx + ")?"
                         + "(\\s" + SsmDEI6Rx + ")?"
                         + "((\\s" + SsmDEINRx + "){0,5}$)";

extern const std::string SsmSkdPeriodString = "^" + SsmDateRx + "(\\s" + SsmDateRx + ")?"; //sch validity effective and dicont dates

extern const std::string SsmShortFlightString = "^" + SsmReidRx + "$";

extern const std::string SsmEquipmentString = "^([" A_Z A_YA "])\\s" //service type
                        "([\\d" A_Z A_YA "]{3})\\s" //aircraft type
                        "((" + SsmPrbdRx + ")?(/" + SsmPrbmRx + ")?(\\." + SsmCraftRx + ")?)" //config string
                        "(\\s[[:alnum:]]{2,10})?" //registration
                        "((\\s" + SsmDEINRx + "){0,4}+)"//dei 2,3,4,5
                        "(\\s" + SsmDEI6Rx + ")?"
                        "(\\s" + SsmDEINRx +")?$";

extern const std::string SsmRoutingString = "^([" A_Z A_YA "]{3})" //dep station
                         "(\\d{4})""(/(M?\\d))?"//scheduled time of aircraft departure and its date variation
                         "(/(\\d{4}))?\\s"//passenger STD
                         "([" A_Z A_YA "]{3})" //arr station
                         "(\\d{4})""(/(M?\\d))?"//scheduled time of aircraft arrival and its date variation
                         "(/(\\d{4}))?"//passenger STA
                         "(\\s" + SsmDEI1Rx + ")?"
                         + "((\\s" + SsmDEINRx + "){0,4}+)" //dei 2-5
                         + "(\\s" + SsmDEI6Rx + ")?"
                         + "(\\s" + SsmDEI7Rx +")?"
                         + "(\\s" + SsmDEINRx + ")?$"; //dei 9

extern const std::string SsmLegChangeString = "^(" + SsmLegsChangeRx + ")\\s*"
                                        + "(\\s" + SsmDEI1Rx + ")?"
                                        + "((\\s" + SsmDEINRx + "){0,4}+)" //dei 2-5
                                        + "(\\s" + SsmDEI6Rx + ")?"
                                        + "(\\s" + SsmDEI7Rx +")?"
                                        + "(\\s" + SsmDEINRx + ")?$"; //dei 9

extern const std::string SsmSegmentString = "^([" A_Z A_YA "]{6})\\s" //segment
                         "(\\d{1,3}(/.*)?)$";

extern const std::string SsmSupplInfoString = "^SI\\s(.+)$";

extern const std::string SsmRejectInfoString = "^(\\d{3})\\s([" A_Z A_YA ZERO_NINE "\\s\\-\\.\\/]*)$"; // for NAC

extern const std::string SsmDontCareString = "^.*$"; //for NAC, ACK etc
extern const std::string SsmEmptyString = "^\\s*$"; // empty or whitespace string

extern const std::string AsmFlightIdentifier = "(" + SsmReidRx + "/(\\d{2}([" A_Z A_YA "]{3})?(\\d{2})?))";

extern const std::string AsmFlightString = "^" + AsmFlightIdentifier + "(\\s" + SsmLegsChangeRx + ")?" +
                        "(\\s" + AsmFlightIdentifier + ")?"
                        "(\\s" + SsmDEI1Rx + ")?" +
                        "((\\s" + SsmDEINRx + "){0,4}+)"//dei 2,3,4,5
                        "(\\s" + SsmDEI6Rx + ")?"
                        "(\\s" + SsmDEI7Rx + ")?"
                        "(\\s" + SsmDEINRx +")?$"; //9

extern const std::string AsmRoutingString = "^([" A_Z A_YA "]{3})" //dep station
                         "(\\d{2})?(\\d{4})"//scheduled date and time of aircraft departure
                         "(/(\\d{4}))?\\s"//passenger STD
                         "([" A_Z A_YA "]{3})" //arr station
                         "(\\d{2})?(\\d{4})"//scheduled date and time of aircraft arrival
                         "(/(\\d{4}))?"//passenger STA
                         "(\\s" + SsmDEI1Rx + ")?"
                         + "((\\s" + SsmDEINRx + "){0,4}+)" //dei 2-5
                         + "(\\s" + SsmDEI6Rx + ")?"
                         + "(\\s" + SsmDEI7Rx +")?"
                         + "(\\s" + SsmDEINRx + ")?$"; //dei 9

} //namespace typeb_parser

#endif //_TLGSTRINGS_H_

