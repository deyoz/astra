#pragma once

#include <boost/date_time.hpp>
#include <boost/optional.hpp>
#include <string>


typedef struct _xmlDoc xmlDoc;
typedef xmlDoc *xmlDocPtr;

typedef struct _xmlNode xmlNode;
typedef xmlNode *xmlNodePtr;

typedef struct _xmlNs xmlNs;
typedef xmlNs *xmlNsPtr;


namespace Timatic {

template <class T>
using Optional = boost::optional<T>;

enum class DateFormat {
    Date,
    DateTime
};

//-----------------------------------------------

enum class Country {
    AA,
    AD,
    AE,
    AF,
    AG,
    AI,
    AL,
    AM,
    AO,
    AR,
    AS,
    AT,
    AU,
    AW,
    AZ,
    BA,
    BB,
    BD,
    BE,
    BF,
    BG,
    BH,
    BI,
    BJ,
    BL,
    BM,
    BN,
    BO,
    BQ,
    BR,
    BS,
    BT,
    BW,
    BY,
    BZ,
    CA,
    CC,
    CD,
    CF,
    CG,
    CH,
    CI,
    CK,
    CL,
    CM,
    CN,
    CO,
    CR,
    CU,
    CV,
    CW,
    CX,
    CY,
    CZ,
    DE,
    DJ,
    DK,
    DM,
    DO,
    DZ,
    EC,
    EE,
    EG,
    ER,
    ES,
    ET,
    FI,
    FJ,
    FK,
    FM,
    FR,
    GA,
    GB,
    GD,
    GE,
    GF,
    GH,
    GI,
    GM,
    GN,
    GP,
    GQ,
    GR,
    GT,
    GU,
    GW,
    GY,
    HK,
    HN,
    HR,
    HT,
    HU,
    ID,
    IE,
    IL,
    IN,
    IQ,
    IR,
    IS,
    IT,
    JM,
    JO,
    JP,
    KE,
    KG,
    KH,
    KI,
    KM,
    KN,
    KP,
    KR,
    KW,
    KY,
    KZ,
    LA,
    LB,
    LC,
    LI,
    LK,
    LR,
    LS,
    LT,
    LU,
    LV,
    LY,
    MA,
    MC,
    MD,
    ME,
    MF,
    MG,
    MH,
    MK,
    ML,
    MM,
    MN,
    MO,
    MP,
    MQ,
    MR,
    MS,
    MT,
    MU,
    MV,
    MW,
    MX,
    MY,
    MZ,
    NA,
    NC,
    NE,
    NF,
    NG,
    NI,
    NL,
    NO,
    NP,
    NR,
    NU,
    NZ,
    OM,
    PA,
    PE,
    PF,
    PG,
    PH,
    PK,
    PL,
    PM,
    PN,
    PR,
    PS,
    PT,
    PW,
    PY,
    QA,
    RE,
    RK,
    RO,
    RS,
    RU,
    RW,
    SA,
    SB,
    SC,
    SD,
    SE,
    SG,
    SH,
    SI,
    SK,
    SL,
    SM,
    SN,
    SO,
    SR,
    SS,
    ST,
    SV,
    SX,
    SY,
    SZ,
    TC,
    TD,
    TG,
    TH,
    TJ,
    TL,
    TM,
    TN,
    TO,
    TR,
    TT,
    TV,
    TW,
    TZ,
    UA,
    UG,
    US,
    UU,
    UY,
    UZ,
    VA,
    VC,
    VE,
    VG,
    VI,
    VN,
    VU,
    WF,
    WS,
    XA,
    XB,
    XC,
    XX,
    YE,
    YT,
    ZA,
    ZM,
    ZW,
    ZZ
};

//-----------------------------------------------

enum class DataSection {
    PassportVisaHealth,
    PassportVisa,
    Passport,
    Health,
    All
};

//-----------------------------------------------

enum class DocumentType {
    AliensPassport,
    BirthCertificate,
    BritishCitizen,
    BritishNationalOverseas,
    BritishOverseasTerritoriesCitizen,
    BritishProtectedPerson,
    BritishSubject,
    BritshOverseasCitizen,
    CDB,
    CNExitandEntryPermit,
    CNOneWayPermit,
    CTD1951,
    CTD1954,
    CertificateofCitizenship,
    CertificateofIdentity,
    CertificateofNaturalization,
    ConsularPassport,
    CrewGeneralDeclarationForm,
    CrewMemberCertificate,
    CrewMemberIDCard,
    CrewMemberLicence,
    Diplomaticpassport,
    DocumentofIdentity,
    EmergencyPassport,
    FormI327,
    FormI512,
    FormI571,
    GRPoliceID,
    HongKongSARChinapassport,
    HuiXiangZheng,
    ICRCTD,
    InterpolIDCard,
    InterpolPassport,
    Kinderausweiswithoutphotograph,
    Kinderausweiswithphotograph,
    LaissezPasser,
    LuBaoZheng,
    LuXingZheng,
    MacaoSARChinapassport,
    MilitaryIDCard,
    MilitaryIdentityDocument,
    NationalIDCard,
    NexusCard,
    None,
    NotarizedAffidavitofCitizenship,
    OASOfficialTravelDocument,
    OfficialPassport,
    OfficialPhotoID,
    PSTD,
    Passport,
    PassportCard,
    PublicAffairsHKMOTravelPermit,
    SJDR,
    SJTD,
    SeafarerID,
    SeamanBook,
    ServicePassport,
    SpecialPassport,
    TaiBaoZheng,
    TemporaryTravelDocument,
    Temporarypassport,
    TitredeVoyage,
    TransportationLetter,
    TravelCertificate,
    TravelPermit,
    TravelPermitHKMO,
    UNLaissezPasser,
    UNMIKTravelDocument,
    VisitPermitforResidentsofMacaotoHKSAR,
    VotersRegistrationCard,
};

//-----------------------------------------------

enum class SecondaryDocumentType {
    BirthCertificate,
    CNExitandEntryPermit,
    CNOneWayPermit,
    HuiXiangZheng,
    InterpolIDCard,
    InterpolPassport,
    LuBaoZheng,
    LuXingZheng,
    MilitaryIDCard,
    NationalIDCard,
    PublicAffairsHKMOTravelPermit,
    SeafarerID,
    SeamanBook,
    TaiBaoZheng,
    TravelPermitHKMO,
};

//-----------------------------------------------

enum class DocumentGroup {
    AlienResidents,
    AirlineCrew,
    GovernmentDutyPassports,
    MerchantSeamen,
    Military,
    Normal,
    Other,
    StatelessAndRefugees,
};

//-----------------------------------------------

enum class Gender {
    Male,
    Female
};

//-----------------------------------------------

enum class StayType {
    Business,
    Duty,
    Vacation,
};

//-----------------------------------------------

enum class Visa {
    NoVisa,
    ValidVisa
};

//-----------------------------------------------

enum class ResidencyDocument {
    AliensPassport,
    PermanentResidentResidentAlienCardFormI551,
    ReentryPermit,
    ResidencePermit,
};

//-----------------------------------------------

enum class DocumentFeature {
    Biometric,
    DigitalPhoto,
    MachineReadableDocument,
    MachineReadableDocumentWithDigitalPhoto,
    None,
};

//----------------------------------------------

enum class Ticket {
    NoTicket,
    Ticket,
};

//-----------------------------------------------

enum class SufficientDocumentation {
    Yes,
    No,
    Conditional,
};

//-----------------------------------------------

enum class ParagraphType {
    Information,
    Restriction,
    Exception,
    Requirement,
    Recommendation,
    Applicable
};

//-----------------------------------------------

enum class ParameterName {
    Country,
    DataSection,
    DocumentType,
    SecondaryDocumentType,
    DocumentGroup,
    Gender,
    StayType,
    Visa,
    ResidencyDocument,
    DocumentFeature,
    Ticket
};

//-----------------------------------------------

enum class GroupName {
    Organisation,
    Infected
};

//-----------------------------------------------

class ErrorType {
public:
    ErrorType(const xmlNodePtr node);
    std::string message;
    Optional<std::string> code;
};

//-----------------------------------------------

class MessageType {
public:
    MessageType(const xmlNodePtr node);
    std::string text;
    Optional<std::string> code;
};

//-----------------------------------------------

class ParameterType {
public:
    ParameterType(const xmlNodePtr node);
    std::string parameterName;
    bool mandatory;
};

//-----------------------------------------------

class ParamValue {
public:
    ParamValue(const char *pn, const char *pd);
    ParamValue(const char *pn, const char *pd, const char *pc);
    ParamValue(const xmlNodePtr node);
    std::string name;
    std::string displayName;
    Optional<std::string> code;
};

//-----------------------------------------------

class CityDetail {
public:
    CityDetail(const xmlNodePtr node);
    std::string cityName;
    std::string cityCode;
};

//-----------------------------------------------

class AlternateDetail {
public:
    AlternateDetail(const xmlNodePtr node);
    std::string countryName;
    std::string countryCode;
};

//-----------------------------------------------

class VisaType {
public:
    VisaType(const xmlNodePtr node);
    std::string visaType;
    std::string issuedBy;
    std::string applicableFor;
    Optional<boost::posix_time::ptime> expiryDateGMT;
};

//-----------------------------------------------

class StayDuration {
public:
    void validate() const;
    void fill(xmlNodePtr node) const;

    Optional<int> days() const { return days_; }
    Optional<int> hours() const { return hours_; }

    void days(const int val) { days_ = val; }
    void hours(const int val) { hours_ = val; }

private:
    Optional<int> days_;
    Optional<int> hours_;
};

//-----------------------------------------------

class VisaData {
public:
    void validate() const;
    void fill(xmlNodePtr node) const;

    const std::string &visaType() const { return visaType_; }
    const std::string &issuedBy() const { return issuedBy_; }
    const boost::posix_time::ptime &expiryDateGMT() const { return expiryDateGMT_; }

    void visaType(const std::string &val) { visaType_ = val; }
    void issuedBy(const std::string &val) { issuedBy_ = val; }
    void expiryDateGMT(const boost::posix_time::ptime &val) { expiryDateGMT_ = val; }

private:
    std::string visaType_;
    std::string issuedBy_;
    boost::posix_time::ptime expiryDateGMT_;
};

//-----------------------------------------------

class TransitCountry {
public:
    void validate() const;
    void fill(xmlNodePtr node) const;

    const std::string &airportCode() const { return airportCode_; }
    const Optional<boost::posix_time::ptime> &arrivalTimestamp() const { return arrivalTimestamp_; }
    const Optional<boost::posix_time::ptime> &departTimestamp() const { return departTimestamp_; }
    const Optional<Visa> &visa() const { return visa_; }
    const Optional<VisaData> &visaData() const { return visaData_; }
    const Optional<Ticket> &ticket() const { return ticket_; }

    void airportCode(const std::string &val) { airportCode_ = val; }
    void arrivalTimestamp(const boost::posix_time::ptime &val) { arrivalTimestamp_ = val; }
    void departTimestamp(const boost::posix_time::ptime &val) { departTimestamp_ = val; }
    void visa(const Visa &val) { visa_ = val; }
    void visaData(const VisaData &val) { visaData_ = val; }
    void ticket(const Ticket &val) { ticket_ = val; }

private:
    std::string airportCode_;
    Optional<boost::posix_time::ptime> arrivalTimestamp_;
    Optional<boost::posix_time::ptime> departTimestamp_;
    Optional<Visa> visa_;
    Optional<VisaData> visaData_;
    Optional<Ticket> ticket_;
};

//-----------------------------------------------

struct Config {
    Config(const std::string &username = "",
           const std::string &subUsername = "",
           const std::string &password = "",
           const std::string &host = "",
           const int port = 443);
    std::string username;
    std::string subUsername;
    std::string password;
    std::string host;
    int port;
};

struct Session {
    std::string sessionID;
    std::string jsessionID;
    int expires = 0;
};

//-----------------------------------------------

const ParamValue &getParams(const ParameterName &val);
const ParamValue &getParams(const Country &val);
const ParamValue &getParams(const DataSection &val);
const ParamValue &getParams(const DocumentType &val);
const ParamValue &getParams(const SecondaryDocumentType &val);
const ParamValue &getParams(const DocumentGroup &val);
const ParamValue &getParams(const Gender &val);
const ParamValue &getParams(const StayType &val);
const ParamValue &getParams(const Visa &val);
const ParamValue &getParams(const ResidencyDocument &val);
const ParamValue &getParams(const DocumentFeature &val);
const ParamValue &getParams(const Ticket &val);

const std::map<Country, ParamValue> &getCountryMap();
const std::map<DataSection, ParamValue> &getDataSectionMap();
const std::map<DocumentType, ParamValue> &getDocumentTypeMap();
const std::map<SecondaryDocumentType, ParamValue> &getSecondaryDocumentTypeMap();
const std::map<ParameterName, ParamValue> &getParameterNameMap();
const std::map<DocumentGroup, ParamValue> &getDocumentGroupMap();
const std::map<Gender, ParamValue> &getGenderMap();
const std::map<StayType, ParamValue> &getStayTypeMap();
const std::map<Visa, ParamValue> &getVisaMap();
const std::map<ResidencyDocument, ParamValue> &getResidencyDocumentMap();
const std::map<DocumentFeature, ParamValue> &getDocumentFeatureMap();
const std::map<Ticket, ParamValue> &getTicketMap();

std::string toString(const Country &val);
std::string toString(const DataSection &val);
std::string toString(const DocumentType &val);
std::string toString(const SecondaryDocumentType &val);
std::string toString(const DocumentGroup &val);
std::string toString(const Gender &val);
std::string toString(const StayType &val);
std::string toString(const Visa &val);
std::string toString(const ResidencyDocument &val);
std::string toString(const DocumentFeature &val);
std::string toString(const Ticket &val);
std::string toString(const ParameterName &val);
std::string toString(const GroupName &val);
std::string toString(const boost::posix_time::ptime &val, const DateFormat &df);

//-----------------------------------------------

DataSection getDataSection(const std::string &val);
SufficientDocumentation getSufficientDocumentation(const std::string &val);
ParagraphType getParagraphType(const std::string &val);
boost::posix_time::ptime getPTime(const std::string &val);

} // Timatic
