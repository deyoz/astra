#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

#include <set>

#include <serverlib/exception.h>
#include <serverlib/str_utils.h>
#include <serverlib/a_ya_a_z_0_9.h>
#include <serverlib/logopt.h>
#include <serverlib/dates.h>
#include "pnr.h"
#include "checkin.h"

#define NICKNAME "NONSTOP"
#include <serverlib/slogger.h>

#define RUS_RECLOC_CHARS "БВГДКЛМНПРСТФХЦЖШ"
#define LAT_RECLOC_CHARS "BVGDKLMNPRSTFXCZW"

namespace
{

static std::set<char> MakeCharSet(const char* charSet)
{
    std::set<char> result;
    while (*charSet != '\0') result.insert(*charSet++);
    return result;
}

static const char* RusCharSet = ZERO_NINE RUS_RECLOC_CHARS;
static const char* LatCharSet = ZERO_NINE LAT_RECLOC_CHARS;
static const char* FullLatCharSet = ZERO_NINE A_Z;

static std::map<char, char> MakeCharMap(const char* from, const char* to)
{
    std::map<char, char> result;
    while (*from != '\0' && *to != '\0') result[*from++] = *to++;
    return result;
}

// В случае успешного завершения возвращает TRUE
// иначе, если do_throw==true, то кидает исключение
//        если do_throw==false, то возвращает FALSE
static bool ValidateRecloc(const std::string& recloc, const std::set<char>& charSet)
{
    for (std::string::const_iterator c = recloc.begin(); c != recloc.end(); ++c) {
        if (charSet.find(*c) == charSet.end()) {
            LogTrace(TRACE5) << "[" << *c << "] not found in " << LogCont(" ", charSet);
            return false;
        }
    }
    return true;
}

static std::string TranslateRecloc(const std::string& recloc,
                                   const std::map<char, char>& mapping,
                                   const std::set<char> oppositeCharSet)
{
    std::string result;
    for (std::string::const_iterator c = recloc.begin();
         c != recloc.end();
         ++c)
    {
        std::map<char, char>::const_iterator it = mapping.find(*c);
        if (it == mapping.end()) {
            return recloc;
        } else
            result += it->second;
    }
    return result;
}

/* Описание см. в recloc.h */
std::string Rus2LatRecloc(const std::string& recloc)
{
    static std::map<char, char> rus2LatMap = MakeCharMap(RusCharSet, LatCharSet);
    static std::set<char> fullLatCharSet = MakeCharSet(FullLatCharSet);
    return TranslateRecloc(recloc, rus2LatMap, fullLatCharSet);
}

/* Описание см. в recloc.h */
std::string Lat2RusRecloc(const std::string& recloc)
{
    static std::map<char, char> lat2RusMap = MakeCharMap(LatCharSet, RusCharSet);
    static std::set<char> rusCharSet = MakeCharSet(RusCharSet);
    return TranslateRecloc(recloc, lat2RusMap, rusCharSet);
}

} // namespaec

namespace ct
{

ENUM_NAMES_BEGIN(PassType)
    (PASS_ADULT, "AAA")
    (PASS_CHILD, "CHD")
    (PASS_INFANT, "IFT")
ENUM_NAMES_END(PassType)
ENUM_NAMES_END_AUX(PassType)

ENUM_NAMES_BEGIN(Sex)
    (PASS_MALE, "M")
    (PASS_FEMALE, "F")
ENUM_NAMES_END(Sex)
ENUM_NAMES_END_AUX(Sex)

ENUM_NAMES_BEGIN(EmdType)
    (EmdType::EMD_A, "A")
    (EmdType::EMD_S, "S")
ENUM_NAMES_END(EmdType)
ENUM_NAMES_END_AUX(EmdType)

ENUM_NAMES_BEGIN2(SegStatus, Code)
    (SegStatus::CONFIRMED, "HK")
    (SegStatus::REQUESTED, "HN")
    (SegStatus::WAITLIST_REQUESTED, "LL")
    (SegStatus::WAITLISTED, "HL")
    (SegStatus::STANDBY, "SA")
    (SegStatus::NON_CONTROLLED, "NC")
    (SegStatus::CANCELLED, "XX")
    (SegStatus::UNABLE, "UN")
    (SegStatus::REJECTED, "UC")
ENUM_NAMES_END2(SegStatus, Code)

ENUM_NAMES_BEGIN2(ArrStatus, Code)
    (ArrStatus::CONFIRMED, "HK")
    (ArrStatus::REQUESTED, "HN")
    (ArrStatus::WAITLISTED, "HL")
ENUM_NAMES_END2(ArrStatus, Code)

ENUM_NAMES_BEGIN2(SsrStatus, Code)
    (SsrStatus::CONFIRMED, "HK")
    (SsrStatus::REQUESTED, "HN")
    (SsrStatus::CANCELLED, "XX")
    (SsrStatus::UNABLE, "UN")
    (SsrStatus::REJECTED, "UC")
    (SsrStatus::IGNORED, "NO")
ENUM_NAMES_END2(SsrStatus, Code)

ENUM_NAMES_BEGIN2(SvcStatus, Code)
    (SvcStatus::CONFIRMED_AND_EMD_REQUIRED, "HD")
    (SvcStatus::CONFIRMED_AND_EMD_ISSUED, "HI")
    (SvcStatus::CONFIRMED, "HK")
    (SvcStatus::REQUESTED, "HN")
    (SvcStatus::IGNORED, "NO")
    (SvcStatus::REJECTED, "UC")
    (SvcStatus::UNABLE, "UN")
    (SvcStatus::CANCELLED, "XX")
ENUM_NAMES_END2(SvcStatus, Code)

bool isValidRecloc(const std::string& s)
{
    if (s.size() != 6) {
        return false;
    }
    const bool hasRus(StrUtils::containsRus(s));
    const bool hasLat(StrUtils::containsLat(s));
    if (hasRus && hasLat) {
        return false;
    }
    static std::set<char> latCharSet = MakeCharSet(FullLatCharSet);
    static std::set<char> rusCharSet = MakeCharSet(RusCharSet);
    if (hasRus && !ValidateRecloc(s, rusCharSet)) {
        return false;
    }
    if (hasLat && !ValidateRecloc(s, latCharSet)) {
        return false;
    }
    return true;
}

bool isValidSpecRes(const EncString& str)
{
    const EncString::base_type::size_type size = str.to866().size();
    return (0 < size) && (size < 4);
}

bool isValidGroupName(const EncString& str)
{
    const EncString::base_type::size_type size = str.to866().size();
    return (0 < size) && (size < 20);
}

bool isValidLangCode(const std::string& langCode)
{
    return langCode.size() == 2 && StrUtils::isLatStr(langCode);
}

bool isValidServiceType(const std::string& t)
{
    return t.size() == 1 && StrUtils::isLatStr(t);
}

bool operator==(const NameParts& lhs, const NameParts& rhs)
{
    return lhs.firstName == rhs.firstName
        && lhs.secondName == rhs.secondName
        && lhs.title == rhs.title;
}

std::ostream& operator<<(std::ostream& os, const NameParts& np)
{
    return os << '[' << np.firstName
        << '/' << np.secondName << '/' << np.title << ']';
}

static bool isTwoLetterTitle(const std::string& str)
{
    return str == "MR" || str == "MS";
}

static bool isThreeLetterTitle(const std::string& str)
{
    return str == "MRS" || str == "CHD";
}

bool isTitle(const std::string& str)
{
    return isTwoLetterTitle(str) || isThreeLetterTitle(str);
}

using NameAndTitle = boost::optional<std::pair<std::string, std::string>>;

static NameAndTitle splitNameAndTitle(const std::string& name)
{
    // ignore names consisting of 1 letter and a title or just a title such as: AMR, CHD, etc
    if (name.size() <= 3) {
        return boost::none;
    }

    std::string possibleTitle = name.substr(name.size() - 2);
    if (isTwoLetterTitle(possibleTitle)) {
        return std::make_pair(name.substr(0, name.size() - 2), possibleTitle);
    }

    if (name.size() > 3) {
        possibleTitle = name.substr(name.size() - 3);
        if (isThreeLetterTitle(possibleTitle)) {
            return std::make_pair(name.substr(0, name.size() - 3), possibleTitle);
        }
    }

    return boost::none;
}

NameParts parseNameParts(const std::string& nameStr)
{
    NameParts np;
    const auto pos1 = nameStr.find_first_of(' ');
    const auto pos2 = nameStr.find_last_of(' ');

    if (pos1 == std::string::npos) {
        if (const auto nameAndTitle = splitNameAndTitle(nameStr)) {
            np.firstName = nameAndTitle->first;
            np.title = nameAndTitle->second;
        } else {
            np.firstName = nameStr;
        }

        return np;
    }

    np.firstName = nameStr.substr(0, pos1);
    if (pos1 == pos2) {
        np.secondName = nameStr.substr(pos1 + 1);
        if (isTitle(np.secondName)) {
            std::swap(np.title, np.secondName);
        } else if (const auto nameAndTitle = splitNameAndTitle(np.secondName)) {
            np.secondName = nameAndTitle->first;
            np.title = nameAndTitle->second;
        }
        return np;
    }

    np.title = nameStr.substr(pos2 + 1);
    if (isTitle(np.title)) {
        np.secondName = nameStr.substr(pos1 + 1, pos2 - pos1 - 1);
    } else {
        np.secondName = nameStr.substr(pos1 + 1);
        np.title.clear();
    }

    return np;
}

std::string trRecloc(const Recloc& recloc, Language l)
{
    if (l == ENGLISH) {
        if (StrUtils::containsRus(recloc.get())) {
            return Rus2LatRecloc(recloc.get());
        }
    } else {
        if (StrUtils::containsLat(recloc.get())) {
            return Lat2RusRecloc(recloc.get());
        }
    }
    return recloc.get();
}

std::string trRemoteRecloc(const RemoteRecloc& recloc, Language l)
{
    if (l == ENGLISH) {
        if (StrUtils::containsRus(recloc.get())) {
            return Rus2LatRecloc(recloc.get());
        }
    } else {
        if (StrUtils::containsLat(recloc.get())) {
            return Lat2RusRecloc(recloc.get());
        }
    }
    return recloc.get();
}

std::ostream& operator<<(std::ostream& os, const EmdType& emdType)
{
    return os << enumToStr(emdType);
}

DocTypeEx::DocTypeEx(const EncString& i_str)
    : str(i_str)
{}

DocTypeEx::DocTypeEx(const nsi::DocTypeId& i_id)
    : str(nsi::DocType(i_id).code(ENGLISH)), id(i_id)
{}

bool operator==(const DocTypeEx& lhs, const DocTypeEx& rhs)
{
    return lhs.str == rhs.str
        && lhs.id == rhs.id;
}

bool operator!=(const DocTypeEx& lhs, const DocTypeEx& rhs)
{
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const DocTypeEx& docType)
{
    if (docType.id) {
        os << *docType.id;
    } else {
        os << docType.str;
    }
    return os;
}

bool isPassport(const ct::DocTypeEx& docType)
{
    static const EncString passport(EncString::fromUtf("PS"));
    return docType.str == passport;
}

bool isInternationalPassport(const ct::DocTypeEx& docType)
{
    static const EncString internationalPassport(EncString::fromUtf("PSP"));
    return docType.str == internationalPassport;
}

std::string makeDocTypeText(const ct::DocTypeEx& docType)
{
    // API codes
    static const EncString fCode(EncString::fromUtf("F"));
    static const EncString aCode(EncString::fromUtf("A"));
    static const EncString cCode(EncString::fromUtf("C"));
    static const EncString iCode(EncString::fromUtf("I"));
    static const EncString mCode(EncString::fromUtf("M"));
    static const EncString pCode(EncString::fromUtf("P"));
    static const EncString ipCode(EncString::fromUtf("IP"));

    if (docType.str == fCode
        || docType.str == aCode
        || docType.str == cCode
        || docType.str == iCode
        || docType.str == mCode
        || docType.str == pCode
        || docType.str == ipCode) {
        return docType.str.to866();
    }

    static const EncString dpCode(EncString::fromUtf("DP"));
    static const EncString spCode(EncString::fromUtf("SP"));
    static const EncString pmCode(EncString::fromUtf("PM"));
    static const EncString zaCode(EncString::fromUtf("ZA"));
    static const EncString zbCode(EncString::fromUtf("ZB"));
    static const EncString zcCode(EncString::fromUtf("ZC"));
    static const EncString npCode(EncString::fromUtf("NP"));
    static const EncString upCode(EncString::fromUtf("UP"));
    static const EncString vbCode(EncString::fromUtf("VB"));
    static const EncString udlCode(EncString::fromUtf("UDL"));
    static const EncString niCode(EncString::fromUtf("NI"));
    static const EncString idCode(EncString::fromUtf("ID"));

    if (isPassport(docType)
        || isInternationalPassport(docType)
        || docType.str == dpCode
        || docType.str == spCode
        || docType.str == pmCode
        || docType.str == zaCode
        || docType.str == zbCode
        || docType.str == zcCode
        || docType.str == npCode
        || docType.str == upCode) {
        return "P";
    }
    if (docType.str == vbCode || docType.str == udlCode) {
        return "M";
    };
    if (docType.str == niCode || docType.str == idCode) {
        return  "I";
    };
    return "F";
}

/*******************************************************************************
 * Статус полётного сегмента в PNR
 ******************************************************************************/

bool SegStatus::isActive() const
{
    switch (code_) {
        case CONFIRMED:          return true;
        case REQUESTED:          return true;
        case WAITLIST_REQUESTED: return true;
        case WAITLISTED:         return true;
        case STANDBY:            return true;
        case NON_CONTROLLED:     return true;
        case CANCELLED:          return false;
        case UNABLE:             return false;
        case REJECTED:           return false;
    };
    LogError(STDLOG) << __FUNCTION__ << ": unexpected status = " << code_;
    throw comtech::Exception(STDLOG, __FUNCTION__, "unexpected status");
}

static std::string SegStatusCode_toString(SegStatus::Code code)
{
    switch (code) {
        case SegStatus::CONFIRMED:          return "CONF";
        case SegStatus::REQUESTED:          return "REQ";
        case SegStatus::WAITLIST_REQUESTED: return "WLREQ";
        case SegStatus::WAITLISTED:         return "WL";
        case SegStatus::STANDBY:            return "STANDBY";
        case SegStatus::NON_CONTROLLED:     return "NC";
        case SegStatus::CANCELLED:          return "CNL";
        case SegStatus::UNABLE:             return "UN";
        case SegStatus::REJECTED:           return "REJ";
    };
    LogError(STDLOG) << __FUNCTION__ << ": unexpected status = " << code;
    throw comtech::Exception(STDLOG, __FUNCTION__, "unexpected status");
}

std::ostream& operator<<(std::ostream& out, const SegStatus& status)
{
    out << SegStatusCode_toString(status.code()) << "(" << status.code() << ")";
    return out;
}

std::set<ct::SegStatus> getActiveSegStatuses()
{
    std::set<ct::SegStatus> activeSegSts;
    for (ct::SegStatus::Code code : enumSorted<ct::SegStatus::Code>()) {
        ct::SegStatus segStat(code);
        if (segStat.isActive()) {
            activeSegSts.insert(segStat);
        }
    }
    return activeSegSts;
}

/*******************************************************************************
 * Статус сегмента прибытия в PNR
 ******************************************************************************/

static std::string ArrStatusCode_toString(ArrStatus::Code code)
{
    switch (code) {
        case ArrStatus::CONFIRMED:  return "CONFIRMED";
        case ArrStatus::REQUESTED:  return "REQUESTED";
        case ArrStatus::WAITLISTED: return "WAITLISTED";
    };
    LogError(STDLOG) << __FUNCTION__ << ": unexpected status = " << code;
    throw comtech::Exception(STDLOG, __FUNCTION__, "unexpected status");
}

std::ostream& operator<<(std::ostream& out, const ArrStatus& status)
{
    out << ArrStatusCode_toString(status.code()) << "(" << status.code() << ")";
    return out;
}

/*******************************************************************************
 * Статус SSR в PNR
 ******************************************************************************/

bool SsrStatus::isActive() const
{
    switch (code_) {
        case CONFIRMED: return true;
        case REQUESTED: return true;
        case CANCELLED: return false;
        case UNABLE:    return false;
        case REJECTED:  return false;
        case IGNORED:   return false;
    };
    LogError(STDLOG) << __FUNCTION__ << ": unexpected status = " << code_;
    throw comtech::Exception(STDLOG, __FUNCTION__, "unexpected status");
}

static std::string SsrStatusCode_toString(SsrStatus::Code code)
{
    switch (code) {
        case SsrStatus::CONFIRMED: return "CONFIRMED";
        case SsrStatus::REQUESTED: return "REQUESTED";
        case SsrStatus::CANCELLED: return "CANCELLED";
        case SsrStatus::UNABLE:    return "UNABLE";
        case SsrStatus::REJECTED:  return "REJECTED";
        case SsrStatus::IGNORED:   return "IGNORED";
    };
    LogError(STDLOG) << __FUNCTION__ << ": unexpected status = " << code;
    throw comtech::Exception(STDLOG, __FUNCTION__, "unexpected status");
}

std::ostream& operator<<(std::ostream& out, const SsrStatus& status)
{
    out << SsrStatusCode_toString(status.code()) << "(" << status.code() << ")";
    return out;
}

/*******************************************************************************
 * Статус SVC в PNR
 ******************************************************************************/

bool SvcStatus::isActive() const
{
    switch (code_) {
        case CONFIRMED_AND_EMD_REQUIRED: return true;
        case CONFIRMED_AND_EMD_ISSUED: return true;
        case CONFIRMED: return true;
        case REQUESTED: return true;
        case IGNORED: return false;
        case REJECTED: return false;
        case UNABLE: return false;
        case CANCELLED: return false;
    };
    LogError(STDLOG) << __FUNCTION__ << ": unexpected status = " << code_;
    throw comtech::Exception(STDLOG, __FUNCTION__, "unexpected SVC status code");
}

static std::string SvcStatusCode_toString(SvcStatus::Code code)
{
    switch (code) {
        case SvcStatus::CONFIRMED_AND_EMD_REQUIRED: return "CONFIRMED_AND_EMD_REQUIRED";
        case SvcStatus::CONFIRMED_AND_EMD_ISSUED: return "CONFIRMED_AND_EMD_ISSUED";
        case SvcStatus::CONFIRMED: return "CONFIRMED";
        case SvcStatus::REQUESTED: return "REQUESTED";
        case SvcStatus::IGNORED: return "IGNORED";
        case SvcStatus::REJECTED: return "REJECTED";
        case SvcStatus::UNABLE: return "UNABLE";
        case SvcStatus::CANCELLED: return "CANCELLED";
    };
    LogError(STDLOG) << __FUNCTION__ << ": unexpected status = " << code;
    throw comtech::Exception(STDLOG, __FUNCTION__, "unexpected SVC status code");
}

std::ostream& operator<<(std::ostream& out, const SvcStatus& status)
{
    out << SvcStatusCode_toString(status.code()) << "(" << status.code() << ")";
    return out;
}

/*********************************************************************
 * Checkin
 ********************************************************************/

bool operator==(const Checkin& lhs, const Checkin& rhs)
{
    return lhs.time == rhs.time
        && lhs.luggage == rhs.luggage
        && lhs.dt == rhs.dt
        && lhs.at == rhs.at;
}

std::ostream& operator<<(std::ostream& os, const Checkin& c)
{
    return os << "Checkin:"
        << LogOpt(" time=", c.time)
        << LogOpt(" luggage=", c.luggage)
        << LogOpt(" dt=", c.dt)
        << LogOpt(" at=", c.at);
}

/*********************************************************************
 * SsrCode
 ********************************************************************/

static bool isValidSsrCode(const std::string& s)
{
    const int sz = s.size();
    if (sz != 4) {
        return false;
    }
    for (char c : s) {
        if (c < 'A' || c > 'Z') {
            return false;
        }
    }
    return true;
}

SsrCode::SsrCode()
{
}

SsrCode::SsrCode(const std::string& s)
{
    if (const auto ssr = SsrCode::create(s)) {
        code_ = ssr->code_;
    } else {
        throw comtech::Exception(STDLOG, __FUNCTION__, "invalid string for SsrCode: [" + s + "]");
    }
}

SsrCode::SsrCode(const EncString& s)
{
    if (const auto ssr = SsrCode::create(s.toUtf())) {
        code_ = ssr->code_;
    } else {
        throw comtech::Exception(STDLOG, __FUNCTION__, "invalid string for SsrCode: [" + s.toUtf() + "]");
    }
}

SsrCode SsrCode::fromNsi(nsi::SsrTypeId id)
{
    return SsrCode(nsi::SsrType(id).lcode().toUtf());
}

nsi::SsrTypeId SsrCode::toNsi() const
{
    return nsi::SsrType(EncString::fromUtf(code_)).id();
}

boost::optional<SsrCode> SsrCode::create(const std::string& s)
{
    if (isValidSsrCode(s)) {
        SsrCode ssr;
        ssr.code_ = s;
        return ssr;
    }
    return boost::none;
}

boost::optional<SsrCode> SsrCode::create(const EncString& s)
{
    if (isValidSsrCode(s.toUtf())) {
        SsrCode ssr;
        ssr.code_ = s.toUtf();
        return ssr;
    }
    if (!nsi::SsrType::find(s)) {
        return boost::none;
    }
    LogError(STDLOG) << "SsrCode from rus letters: " << s;
    SsrCode ssr;
    ssr.code_ = nsi::SsrType(s).code(ENGLISH).toUtf();
    return ssr;
}

const std::string& SsrCode::get() const
{
    return code_;
}

const SsrCode& SsrCode::ADMD() { static const ct::SsrCode code("ADMD"); return code; }
const SsrCode& SsrCode::ADTK() { static const ct::SsrCode code("ADTK"); return code; }
const SsrCode& SsrCode::CBBG() { static const ct::SsrCode code("CBBG"); return code; }
const SsrCode& SsrCode::CHLD() { static const ct::SsrCode code("CHLD"); return code; }
const SsrCode& SsrCode::CHML() { static const ct::SsrCode code("CHML"); return code; }
const SsrCode& SsrCode::CKIN() { static const ct::SsrCode code("CKIN"); return code; }
const SsrCode& SsrCode::CLID() { static const ct::SsrCode code("CLID"); return code; }
const SsrCode& SsrCode::CTCE() { static const ct::SsrCode code("CTCE"); return code; }
const SsrCode& SsrCode::CTCM() { static const ct::SsrCode code("CTCM"); return code; }
const SsrCode& SsrCode::CTCR() { static const ct::SsrCode code("CTCR"); return code; }
const SsrCode& SsrCode::DBML() { static const ct::SsrCode code("DBML"); return code; }
const SsrCode& SsrCode::DOCA() { static const ct::SsrCode code("DOCA"); return code; }
const SsrCode& SsrCode::DOCO() { static const ct::SsrCode code("DOCO"); return code; }
const SsrCode& SsrCode::DOCS() { static const ct::SsrCode code("DOCS"); return code; }
const SsrCode& SsrCode::EXST() { static const ct::SsrCode code("EXST"); return code; }
const SsrCode& SsrCode::FOID() { static const ct::SsrCode code("FOID"); return code; }
const SsrCode& SsrCode::FQTR() { static const ct::SsrCode code("FQTR"); return code; }
const SsrCode& SsrCode::FQTS() { static const ct::SsrCode code("FQTS"); return code; }
const SsrCode& SsrCode::FQTU() { static const ct::SsrCode code("FQTU"); return code; }
const SsrCode& SsrCode::FQTV() { static const ct::SsrCode code("FQTV"); return code; }
const SsrCode& SsrCode::GPST() { static const ct::SsrCode code("GPST"); return code; }
const SsrCode& SsrCode::GRPS() { static const ct::SsrCode code("GRPS"); return code; }
const SsrCode& SsrCode::INFT() { static const ct::SsrCode code("INFT"); return code; }
const SsrCode& SsrCode::MEDA() { static const ct::SsrCode code("MEDA"); return code; }
const SsrCode& SsrCode::NRSB() { static const ct::SsrCode code("NRSB"); return code; }
const SsrCode& SsrCode::OTHS() { static const ct::SsrCode code("OTHS"); return code; }
const SsrCode& SsrCode::PCTC() { static const ct::SsrCode code("PCTC"); return code; }
const SsrCode& SsrCode::RLOC() { static const ct::SsrCode code("RLOC"); return code; }
const SsrCode& SsrCode::RQST() { static const ct::SsrCode code("RQST"); return code; }
const SsrCode& SsrCode::SEAT() { static const ct::SsrCode code("SEAT"); return code; }
const SsrCode& SsrCode::STCR() { static const ct::SsrCode code("STCR"); return code; }
const SsrCode& SsrCode::TKNA() { static const ct::SsrCode code("TKNA"); return code; }
const SsrCode& SsrCode::TKNE() { static const ct::SsrCode code("TKNE"); return code; }
const SsrCode& SsrCode::TKNM() { static const ct::SsrCode code("TKNM"); return code; }
const SsrCode& SsrCode::TKNR() { static const ct::SsrCode code("TKNR"); return code; }
const SsrCode& SsrCode::TKTL() { static const ct::SsrCode code("TKTL"); return code; }
const SsrCode& SsrCode::TLAC() { static const ct::SsrCode code("TLAC"); return code; }
const SsrCode& SsrCode::VGML() { static const ct::SsrCode code("VGML"); return code; }

std::ostream& operator<<(std::ostream& os, const SsrCode& ssr)
{
    return os << ssr.get();
}

bool isSeatSsr(const SsrCode& ssr)
{
    static const std::set<std::string> seatSsrs = {
        "NSSA",
        "NSSB",
        "NSSR",
        "NSST",
        "NSSW",
        "RQST",
        "SEAT",
        "SMSA",
        "SMSB",
        "SMSR",
        "SMST",
        "SMSW",
    };
    return end(seatSsrs) != seatSsrs.find(ssr.get());
}

bool isFQTVSsr(const SsrCode& ssr)
{
    return "FQTV" == ssr.get();
}

} // namespace ct

#ifdef XP_TESTING
void init_pnr_tests() {}

#include <serverlib/xp_test_utils.h>
#include <serverlib/checkunit.h>

namespace
{

START_TEST(check_StatusCodeValues)
{
    // Коды статусов могут быть использованы в БД,
    // менять их стоит с отсторожностью.
    // Есть надежда, что данный тест напомнит об этом при случае.
    fail_unless(ct::SegStatus::CONFIRMED == 0);
    fail_unless(ct::SegStatus::REQUESTED == 1);
    fail_unless(ct::SegStatus::WAITLIST_REQUESTED == 2);
    fail_unless(ct::SegStatus::WAITLISTED == 3);
    fail_unless(ct::SegStatus::STANDBY == 4);
    fail_unless(ct::SegStatus::NON_CONTROLLED == 5);
    fail_unless(ct::SegStatus::CANCELLED == 6);
    fail_unless(ct::SegStatus::UNABLE == 7);
    fail_unless(ct::SegStatus::REJECTED == 8);

    fail_unless(ct::ArrStatus::CONFIRMED == 0);
    fail_unless(ct::ArrStatus::REQUESTED == 1);
    fail_unless(ct::ArrStatus::WAITLISTED == 2);

    fail_unless(ct::SsrStatus::CONFIRMED == 0);
    fail_unless(ct::SsrStatus::REQUESTED == 1);
    fail_unless(ct::SsrStatus::CANCELLED == 2);
    fail_unless(ct::SsrStatus::UNABLE == 3);
    fail_unless(ct::SsrStatus::REJECTED == 4);
}
END_TEST;

START_TEST(check_trRecloc)
{
#define CHECK_TR_RECLOC(from, lang, to) { \
    LogTrace(TRACE5) << "check " << from << " " << to; \
    const std::string s(ct::trRecloc(ct::Recloc(from), lang)); \
    fail_unless(s == to, "trRecloc(%s" #lang") failed: [%s] != [%s]", from, s.c_str(), to); \
}
    CHECK_TR_RECLOC("123БВГ", ENGLISH, "123BVG");
    CHECK_TR_RECLOC("123BVG", ENGLISH, "123BVG");

    CHECK_TR_RECLOC("123БВГ", RUSSIAN, "123БВГ");
    CHECK_TR_RECLOC("123BVG", RUSSIAN, "123БВГ");

    try {
        const ct::Recloc r("123БVG");
        fail_if(1, "construct invalid recloc: %s", r.get().c_str());
    } catch (const comtech::Exception& e) {
        ProgTrace(TRACE5, "%s", e.what());
    }

    // буквы E нет в списке допустимых, поэтому перевод на русский невозможен
    CHECK_TR_RECLOC("MLGW8E", RUSSIAN, "MLGW8E");
#undef CHECK_TR_RECLOC
} END_TEST

START_TEST(check_trRemoteRecloc)
{
#define CHECK_TR_RECLOC(from, lang, to) { \
    LogTrace(TRACE5) << "check " << from << " " << to; \
    const std::string s(ct::trRemoteRecloc(ct::RemoteRecloc(from), lang)); \
    fail_unless(s == to, "trRemoteRecloc(%s" #lang") failed: [%s] != [%s]", from, s.c_str(), to); \
}
    CHECK_TR_RECLOC("123БВГ", ENGLISH, "123BVG");
    CHECK_TR_RECLOC("123BVG", ENGLISH, "123BVG");

    CHECK_TR_RECLOC("123БВГ", RUSSIAN, "123БВГ");
    CHECK_TR_RECLOC("123BVG", RUSSIAN, "123БВГ");

    try {
        ct::trRecloc(ct::Recloc("123БVG"), ENGLISH);
        fail_if(1);
    } catch (const comtech::Exception& e) {
        ProgTrace(TRACE5, "%s", e.what());
    }

    // буквы E нет в списке допустимых, но для RemoteRecloc такие проверки игнорируются
    CHECK_TR_RECLOC("MLGW8E", ENGLISH, "MLGW8E");
    CHECK_TR_RECLOC("MLGW8E", RUSSIAN, "MLGW8E");
} END_TEST

START_TEST(check_parseNameParts)
{
    struct {
        const char* str;
        ct::NameParts np;
    } checks[] = {
        {"IVAN", {"IVAN"}},
        {"IVAN MR", {"IVAN", "", "MR"}},
        {"MARIA MRS", {"MARIA", "", "MRS"}},
        {"MARIA IVANOVNA", {"MARIA", "IVANOVNA", ""}},
        {"MARIA IVANOVNA MRS", {"MARIA", "IVANOVNA", "MRS"}},
        {"ANNA MARIA ANTOINETTE MRS", {"ANNA", "MARIA ANTOINETTE", "MRS"}},
        {"ANNA MARIA ANTOINETTE", {"ANNA", "MARIA ANTOINETTE", ""}},
        {"MARIA IVANOVNAMRS", {"MARIA", "IVANOVNA", "MRS"}},
        {"MARIAIVANOVNAMRS", {"MARIAIVANOVNA", "", "MRS"}},
        {"MR", {"MR", "", ""}},
        {"CHD", {"CHD", "", ""}},
    };
    for (const auto& c : checks) {
        const ct::NameParts np = ct::parseNameParts(c.str);
        LogTrace(TRACE5) << "parse: " << c.str;
        LogTrace(TRACE5) << "expected [" << c.np.firstName << '/' << c.np.secondName << '/' << c.np.title << ']';
        LogTrace(TRACE5) << "got      [" << np.firstName << '/' << np.secondName << '/' << np.title << ']';
        fail_unless(np == c.np, "[%s] failed", c.str);
    }
} END_TEST

#define SUITENAME "coretypes"
TCASEREGISTER(0, 0)
{
    ADD_TEST(check_StatusCodeValues);
    ADD_TEST(check_trRecloc);
    ADD_TEST(check_trRemoteRecloc);
    ADD_TEST(check_parseNameParts);
}
TCASEFINISH
} // namespace
#endif /* #ifdef XP_TESTING */
