#ifndef LIBNSI_NSI_H
#define LIBNSI_NSI_H

#include <ostream>
#include <string>
#include <vector>

#include <serverlib/lngv.h>
#include <serverlib/rip.h>
#include <serverlib/rip_validators.h>
#include <serverlib/bool_with_not.h>
#include <serverlib/enum.h>

namespace nsi
{

namespace details
{

template<typename T>
struct DepArr
{
    T dep, arr;
    DepArr(const T& d, const T& a): dep(d), arr(a) {}
};

template<typename T>
std::ostream& operator<<(std::ostream& s, const DepArr<T>& ft) {
    return s << ft.dep << "-" << ft.arr;
}

template<typename T>
bool operator<(const DepArr<T>& x, const DepArr<T>& y) {
    return (x.dep != y.dep) ? (x.dep < y.dep) : (x.arr < y.arr);
}

template<typename T>
bool operator>(const DepArr<T>& x, const DepArr<T>& y) {
    return (x.dep != y.dep) ? (x.dep > y.dep) : (x.arr > y.arr);
}

template<typename T>
bool operator==(const DepArr<T>& x, const DepArr<T>& y) {
    return x.dep == y.dep && x.arr == y.arr;
}

template<typename T>
bool operator!=(const DepArr<T>& x, const DepArr<T>& y) {
    return !(x == y);
}

} // details

enum PassCategory_t {
    Adult = 0,
    Child = 1,
    Infant = 2
};

enum FltServiceType {
    REGULAR = 0,
    ADDITIONAL,
    CHARTER,
    OTHER
};
ENUM_NAMES_DECL(FltServiceType);

DECL_RIP(DocTypeId, int);
DECL_RIP(SsrTypeId, int);
DECL_RIP(GeozoneId, int);
DECL_RIP(CountryId, int);
DECL_RIP(RegionId, int);
DECL_RIP(CityId, int);
DECL_RIP(PointId, int);
DECL_RIP_LENGTH(TermId, std::string, 1, 2);
DECL_RIP(RouterId, int);
typedef details::DepArr<CityId> DepArrCities;
typedef details::DepArr<PointId> DepArrPoints;
typedef std::vector<PointId> Points;
DECL_RIP(CompanyId, int);
DECL_RIP(AircraftTypeId, int);
DECL_RIP(CurrencyId, int);
DECL_RIP(RestrictionId, int);
DECL_RIP(MealServiceId, int);
DECL_RIP(InflServiceId, int);
DECL_RIP(ServiceTypeId, int);

enum StringChoiceMode { SCM_STRICT, SCM_NOSTRICT };

class Callbacks;
void setCallbacks(Callbacks*);
Callbacks& callbacks();
void clearCache();

template<typename T> std::string makeNsiErrId(int);
template<typename T> std::string makeNsiErrCode(const std::string&);

} // nsi

#include "details.h"

namespace nsi
{

struct CompanyData : public details::BasicNsiData<CompanyId>
{
    explicit CompanyData(const CompanyId&);

    std::string accountCode;
};

class Company : public details::BasicNsiObject<CompanyData>
{
public:
    explicit Company(const CompanyId&);
    explicit Company(const EncString&);
    static BoolWithNot find(const EncString&);

    static boost::optional<CompanyId> findCompanyIdByAccountCode(const EncString&);
    std::string accountCode() const;
};

struct DocTypeData : public details::BasicNsiData<DocTypeId>
{
    explicit DocTypeData(const DocTypeId& id);

    bool international;
    bool requireAge;
    bool requireCitizenship;
};

class DocType : public details::BasicNsiObject<DocTypeData>
{
public:
    explicit DocType(const DocTypeId&);
    explicit DocType(const EncString&);
    static BoolWithNot find(const EncString&);

    bool international() const;
    bool requireAge() const;
    bool requireCitizenship() const;
};

enum SsrRequiredType {
    RT_DENIED = 0,      // запрещено
    RT_ALLOWED = 1,     // разрешено
    RT_REQUIRED = 2     // обязательно к заполнению
};

struct SsrTypeData : public details::BasicNsiData<SsrTypeId>
{
    explicit SsrTypeData(const SsrTypeId&);

    bool needConfirm;                  // обязательность подтверждения
    SsrRequiredType actionCode;
    SsrRequiredType freeText;          // свободный текст в запросе
    SsrRequiredType freeTextAnswer;    // свободный текст в ответе
    SsrRequiredType pass;              // привязка к пассажиру
    SsrRequiredType seg;               // привязка к сегменту
};

class SsrType : public details::BasicNsiObject<SsrTypeData>
{
public:
    explicit SsrType(const SsrTypeId&);
    explicit SsrType(const EncString&);
    static BoolWithNot find(const EncString&);

    bool needConfirm() const;
    SsrRequiredType actionCode() const;
    SsrRequiredType freeText() const;
    SsrRequiredType freeTextAnswer() const;
    SsrRequiredType pass() const;
    SsrRequiredType seg() const;

    static SsrTypeId inftId();
    static SsrTypeId chldId();
    static SsrTypeId seatId();
    static SsrTypeId othsId();
    static SsrTypeId docsId();
    static SsrTypeId docaId();
    static SsrTypeId docoId();
    static SsrTypeId cbbgId();
    static SsrTypeId exstId();
    static SsrTypeId stcrId();
    static SsrTypeId pctcId();
    static SsrTypeId foidId();
};
bool isSeatSsr(const nsi::SsrTypeId& ssr);
bool isFQTVSsr(const nsi::SsrTypeId& ssr);

struct GeozoneData : public details::BasicNsiData<GeozoneId>
{
    explicit GeozoneData(const GeozoneId&);
};

class Geozone : public details::BasicNsiObject<GeozoneData>
{
public:
    explicit Geozone(const GeozoneId&);
    explicit Geozone(const EncString&);
    static BoolWithNot find(const EncString&);
};

struct CountryData : public details::BasicNsiData<CountryId>
{
    explicit CountryData(const CountryId&, const CurrencyId&);

    std::string isoCode;
    CurrencyId currency;
};

class Country : public details::BasicNsiObject<CountryData>
{
public:
    explicit Country(const CountryId&);
    explicit Country(const EncString&);
    static BoolWithNot find(const EncString&);
    static boost::optional<CountryId> findCountryIdByIso(const EncString&);
    static boost::optional<CountryId> findCountryId(const EncString&);

    const std::string& isoCode() const;
    const CurrencyId currency() const;
};

struct RegionData : public details::BasicNsiData<RegionId>
{
    explicit RegionData(const RegionId&, const CountryId&);

    CountryId countryId;
};

class Region : public details::BasicNsiObject<RegionData>
{
public:
    explicit Region(const RegionId&);
    explicit Region(const EncString&);
    static BoolWithNot find(const EncString&);

    const CountryId& countryId() const;
};

struct CityData : public details::BasicNsiData<CityId>
{
    CityData(const CityId&, const CountryId&, const boost::optional<RegionId>&);

    CountryId countryId;
    boost::optional<RegionId> regionId;
    int longitude;
    int latitude;
    std::string timezone;
};

class City : public details::BasicNsiObject<CityData>
{
public:
    explicit City(const CityId&);
    explicit City(const EncString&);
    static BoolWithNot find(const EncString&);

    const CountryId& countryId() const;
    const boost::optional<RegionId>& regionId() const;
    int longitude() const;
    int latitude() const;
    const std::string& timezone() const;
};

struct PointData : public details::BasicNsiData<PointId>
{
    explicit PointData(const PointId&, const CityId&);

    CityId cityId;
    int longitude;
    int latitude;
    std::vector<TermId> terms;
};

class Point : public details::BasicNsiObject<PointData>
{
public:
    explicit Point(const PointId&);
    explicit Point(const EncString&);
    static BoolWithNot find(const EncString&);

    const CityId& cityId() const;
    int longitude() const;
    int latitude() const;
    const std::vector<TermId>& terms() const;
};

struct PointOrCode
{
    explicit PointOrCode(PointId);
    explicit PointOrCode(const EncString&);

    const boost::optional<nsi::PointId>& id() const;
    const EncString& code(Language lang, StringChoiceMode m = SCM_NOSTRICT) const;
private:
    EncString code_;
    boost::optional<nsi::PointId> id_;
};
bool operator==(const PointOrCode&, const PointOrCode&);
bool operator!=(const PointOrCode&, const PointOrCode&);
std::ostream& operator<<(std::ostream&, const PointOrCode&);

class CityPoint
{
public:
    explicit CityPoint(CityId);
    explicit CityPoint(PointId);
    explicit CityPoint(const EncString&);

    static BoolWithNot find(const EncString&);

    EncString lcode() const;
    EncString code(Language lang, StringChoiceMode m = SCM_NOSTRICT) const;
    EncString name(Language lang, StringChoiceMode m = SCM_NOSTRICT) const;

    CityId cityId() const;
    boost::optional<nsi::PointId> pointId() const;
private:
    CityId cityId_;
    boost::optional<PointId> pointId_;

    friend bool operator==(const CityPoint&, const CityPoint&);
    friend std::ostream& operator<<(std::ostream&, const CityPoint&);
};
bool operator==(const CityPoint&, const CityPoint&);
std::ostream& operator<<(std::ostream&, const CityPoint&);

struct AircraftTypeData : public details::BasicNsiData<AircraftTypeId>
{
    explicit AircraftTypeData(const AircraftTypeId&);
};

class AircraftType : public details::BasicNsiObject<AircraftTypeData>
{
public:
    explicit AircraftType(const AircraftTypeId&);
    explicit AircraftType(const EncString&);
    static BoolWithNot find(const EncString&);
};

struct CurrencyData : public details::BasicNsiData<CurrencyId>
{
    explicit CurrencyData(const CurrencyId&);
    boost::optional<int> isoNum;
};

class Currency : public details::BasicNsiObject<CurrencyData>
{
public:
    explicit Currency(const CurrencyId&);
    explicit Currency(const EncString&);
    boost::optional<int> isoNum() const;

    static BoolWithNot find(const EncString&);
};


struct RestrictionData : public details::BasicNsiData<RestrictionId>
{
    explicit RestrictionData(const RestrictionId&);
    RestrictionData(int, const EncString&, const EncString&, const EncString&);
};


class Restriction : public details::BasicNsiObject<RestrictionData>
{
public:
    explicit Restriction(const RestrictionId&);
    explicit Restriction(const EncString&);
    static std::vector<RestrictionId> restrictions();

    static BoolWithNot find(const EncString&);
};

struct MealServiceData : public details::BasicNsiData<MealServiceId>
{
    explicit MealServiceData(const MealServiceId&);
    MealServiceData(int, const EncString&, const EncString&, const EncString&);
};


class MealService : public details::BasicNsiObject<MealServiceData>
{
public:
    explicit MealService(const MealServiceId&);
    explicit MealService(const EncString&);
    static std::vector<MealServiceId> mealServices();

    static BoolWithNot find(const EncString&);
};

struct InflServiceData : public details::BasicNsiData<InflServiceId>
{
    explicit InflServiceData(const InflServiceId&);
    InflServiceData(int, const EncString&, const EncString&, const EncString&);
};


class InflService : public details::BasicNsiObject<InflServiceData>
{
public:
    explicit InflService(const InflServiceId&);
    explicit InflService(const EncString&);
    static std::vector<InflServiceId> inflServices();

    static BoolWithNot find(const EncString&);
};

struct ServiceTypeData : public details::BasicNsiData<ServiceTypeId>
{
    explicit ServiceTypeData(const ServiceTypeId&);
    ServiceTypeData(int, const EncString&, FltServiceType, const EncString&, const EncString&);
    FltServiceType fltType;
};


class ServiceType : public details::BasicNsiObject<ServiceTypeData>
{
public:
    explicit ServiceType(const ServiceTypeId&);
    explicit ServiceType(const EncString&);
    static std::vector<ServiceTypeId> serviceTypes();

    static BoolWithNot find(const EncString&);
};

struct RouterData : public details::BasicNsiData<RouterId>
{
    RouterData(const RouterId& );

    bool blockAnswers;            // блокировать ответы из роутера
    bool blockRequests;           // блокировать запросы в роутер
    std::string canonName;        // каноническое имя
    bool ediTOnO;                 // edifact comm level: answer with LAST in series on Only on series
    bool hth;                     // использовать Host-To-Host контейнер
    std::string hthAddress;       // HTH-адрес роутера
    std::string ipAddress;        // IP адрес роутера
    short ipPort;                 // IP порт роутера
    bool loopback;                // адресат в этом же центре
    unsigned maxHthPartSize;      // максимальный размер части HTH сообщения
    unsigned maxPartSize;         // максимальный размер части сообщения
    unsigned maxTypebPartSize;    // максимальный размер части TypeB сообщения
    std::string ourHthAddres;     // наш HTH-адрес для данного роутера
    unsigned remAddrNum;          // HTH Layer 5 : remote addr number
    unsigned responseTimeout;     // время ожидания ответа - максимальное время жизни EDIFACT-телеграммы
    bool censor;                  // исключение недопустимых в Airimp символов перед отправкой (кроме русских букв)
    bool translit;                // транслитерация сообщения перед отправкой
    bool sendContrl;              // отправлять CONTRL
    std::string senderName;       // имя отправителя
    unsigned tprLen;              // длина TPR
    bool trueTypeB;               // делить TypeB - сообщения в соответствии со стандартами (не все системы это поддерживают)
    bool tpb;                     // поддержка TypeB
    bool email;                   // поддержка отправки по email
};

class Router : public details::BasicNsiObject<RouterData>
{
public:
    explicit Router(const RouterId&);
    explicit Router(const EncString&);
    static BoolWithNot find(const EncString&);

    bool blockAnswers() const;
    bool blockRequests() const;
    std::string canonName() const;
    bool ediTOnO() const;
    bool hth() const;
    std::string hthAddress() const;
    std::string ipAddress() const;
    short ipPort() const;
    bool loopback() const;
    unsigned maxHthPartSize() const;
    unsigned maxPartSize() const;
    unsigned maxTypebPartSize() const;
    std::string ourHthAddres() const;
    unsigned remAddrNum() const;
    unsigned responseTimeout() const;
    bool censor() const;
    bool translit() const;
    bool sendContrl() const;
    std::string senderName() const;
    unsigned tprLen() const;
    bool trueTypeB() const;
    bool tpb() const;
    bool email() const;
};

template<typename NsiObjType>
void clearCacheByVersion();

template<typename NsiObjTypeId>
NsiObjTypeId replaceObsoleteId(const NsiObjTypeId&);

} //nsi

#endif /* LIBNSI_NSI_H */
