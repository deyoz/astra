#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

#include <serverlib/string_cast.h>
#include <serverlib/str_utils.h>
#include <serverlib/posthooks.h>
#include <serverlib/tcl_utils.h>

#include "nsi.h"
#include "exception.h"
#include "callbacks.h"

#define NICKNAME "NONSTOP"
#include <serverlib/slogger.h>

namespace nsi {
void clearCachedGeozoneRelations();
} //nsi

static std::shared_ptr<nsi::Callbacks> cb;

static bool isExpired(time_t timestamp, time_t now)
{
    static const int timeout = readIntFromTcl("NSI_TIMEOUT", 300);
    return (now - timestamp) > timeout;
}

static std::string makeNsiCacheErrText(int id)
{
    return "Nsi cache: insert by id " + HelpCpp::string_cast(id) + " failed.";
}

static std::string makeNsiCacheErrText(const EncString& code)
{
    std::stringstream os;
    os << "Nsi cache: insert by code (" << code << ") failed.";
    return os.str();
}

namespace
{

template <typename T> void clearExternals() {}

#define TYPE_EXT_CLEAR_IMPL(TypeName) \
template <> void clearExternals<nsi::TypeName::data_type>() \
{\
    nsi::clearCachedGeozoneRelations();\
}

TYPE_EXT_CLEAR_IMPL(Geozone)
TYPE_EXT_CLEAR_IMPL(Country)
TYPE_EXT_CLEAR_IMPL(Region)
TYPE_EXT_CLEAR_IMPL(City)

template<typename T> class ClearCacheHook;

template<typename T>
class Cache
{
public:
    typedef Cache<T> this_type;
    typedef std::shared_ptr<T> data_ptr_type;
    typedef T data_type;
    typedef typename T::id_type id_type;
    typedef std::map<id_type, data_ptr_type> map_by_id_type;
    typedef std::map<EncString, data_ptr_type> map_by_code_type;
    // FIXME: интересная ситуация когда у разных объектов код на одном языке совпадает с кодом на другом языке

    static this_type& instance()
    {
        static this_type cache;
        return cache;
    }

    const data_ptr_type get(const id_type& id)
    {
        setupClearHook();
        typename map_by_id_type::const_iterator it(indexId_.find(id));
        if (it == indexId_.end()) {
            return get__(find(id));
        }
        return it->second;
    }

    const data_ptr_type get(const EncString& code_)
    {
        setupClearHook();
        const EncString code(code_.trim());
        typename map_by_code_type::const_iterator it(indexCode_.find(code));
        if (it == indexCode_.end()) {
            return get__(find(code));
        }
        return it->second;
    }

    void clear()
    {
        indexId_.clear();
        indexCode_.clear();
        clearExternals<T>();
        updateCheckTime(time(nullptr));
    }

    void clearByTime()
    {
        const time_t now = time(nullptr);

        for (auto i = indexId_.begin(); i != indexId_.end();) {
            if (isExpired(i->second->timestamp, now)) {
                for (const auto& ci : i->second->codes) {
                    indexCode_.erase(ci.second);
                }
                i = indexId_.erase(i);
            } else {
                ++i;
            }
        }
    }

    void clearByVersion(bool checkTime)
    {
        const time_t now = time(nullptr);
        if (checkTime && !isExpired(lastCheckTime(), now)) {
            return;
        }
        const unsigned maxVersion(getMaxVersion());
        if (version_ != maxVersion) {
            clear();
            setVersion(maxVersion);
        }
        updateCheckTime(now);
    }

    void updateCheckTime(time_t t)
    {
        checkTime_ = t;
    }

    void setVersion(unsigned version)
    {
        version_ = version;
    }

    unsigned version() const
    {
        return version_;
    }

    size_t getMaxVersion() const;

    void setupClearHook() const
    {
        Posthooks::sethCleanCache(ClearCacheHook<data_type>());
    }

    time_t lastCheckTime() const
    {
        return checkTime_;
    }
private:
    Cache(): version_(0), checkTime_(time(nullptr)) {}
    Cache(const Cache&) = delete;
    Cache& operator==(const Cache&) = delete;

    const data_ptr_type get__(const boost::optional<data_type>& v)
    {
        if (v) {
            const auto p = std::make_shared<T>(*v);
            if (!indexId_.emplace(v->id, p).second) {
                throw nsi::Exception(__FILE__, __LINE__, makeNsiCacheErrText(v->id.get()));
            }
            for (const auto& vc : v->codes) {
                if (!vc.second.empty()) {
                    if (indexCode_.emplace(vc.second, p).first->second != p) {
                        throw nsi::Exception(__FILE__, __LINE__, makeNsiCacheErrText(vc.second));
                    }
                }
            }
            return p;
        }
        return nullptr;
    }
    boost::optional<T> find(const id_type&);
    boost::optional<T> find(const EncString&);

    map_by_id_type indexId_;
    map_by_code_type indexCode_;
    unsigned version_;
    time_t checkTime_;
};


template<typename T>
class ClearCacheHook: public Posthooks::BaseHook
{
public:
    ClearCacheHook()
    {}
    virtual ClearCacheHook<T>* clone() const override
    {
        return new ClearCacheHook<T>(*this);
    }
    virtual void run() override
    {
        if (cb->needCheckVersion()) {
            Cache<T>::instance().clearByVersion(true);
        } else {
            Cache<T>::instance().clearByTime();
        }
    }
private:
    virtual bool less2(const Posthooks::BaseHook* ptr) const noexcept override { return false; }
private:
    unsigned version_;
};

#define TYPE_CACHE_IMPL(TypeName) \
template<> boost::optional<nsi::TypeName::data_type> Cache<nsi::TypeName::data_type>::find(const nsi::TypeName::id_type& id) { \
    return cb->MakeName3(find, TypeName, Data)(id); \
} \
template<> boost::optional<nsi::TypeName::data_type> Cache<nsi::TypeName::data_type>::find(const EncString& code) { \
    const boost::optional<nsi::TypeName::id_type> id(cb->MakeName3(find, TypeName, Id)(code)); \
    if (!id) { \
        return boost::optional<nsi::TypeName::data_type>(); \
    } \
    return cb->MakeName3(find, TypeName, Data)(*id); \
} \
template<> size_t Cache<nsi::TypeName::data_type>::getMaxVersion() const { \
    return cb->MakeName3(get, TypeName, MaxVersion)(); \
}

TYPE_CACHE_IMPL(Company);
TYPE_CACHE_IMPL(DocType);
TYPE_CACHE_IMPL(SsrType);
TYPE_CACHE_IMPL(Geozone);
TYPE_CACHE_IMPL(Country);
TYPE_CACHE_IMPL(Region);
TYPE_CACHE_IMPL(City);
//TYPE_CACHE_IMPL(Point);
/* Кэш для Point выключен в Сирене. В Лео это не должно быть перенесено! См. #26732.
 * Реализация Point в Сирене допускает дублирование кодов для разных точек - может быть город ПЕК и порт ПЕК,
 * при этом и город, и порт могут использоваться как код точки. Реализация кэша здесь для этого не подходит,
 * но при этом будет использован Сиреновский кеш Basetables отдельно для точек и городов, поэтому
 * для Сирены отключение данного кэша допустимо.*/
TYPE_CACHE_IMPL(AircraftType);
TYPE_CACHE_IMPL(Router);
TYPE_CACHE_IMPL(Currency);
TYPE_CACHE_IMPL(Restriction);
TYPE_CACHE_IMPL(MealService);
TYPE_CACHE_IMPL(InflService);
TYPE_CACHE_IMPL(ServiceType);
} // namespace

namespace nsi
{

Exception::Exception(const char* file, int line, const std::string& s)
    : ServerFramework::Exception("NSI", file, line, "", s)
{
}

void setCallbacks(Callbacks* p)
{
    cb.reset(p);
}

Callbacks& callbacks()
{
    return *cb;
}

void clearCache()
{
    Cache<Company::data_type>::instance().clear();
    Cache<DocType::data_type>::instance().clear();
    Cache<SsrType::data_type>::instance().clear();
    Cache<Geozone::data_type>::instance().clear();
    Cache<Country::data_type>::instance().clear();
    Cache<Region::data_type>::instance().clear();
    Cache<City::data_type>::instance().clear();
    //Cache<Point::data_type>::instance().clear();
    Cache<AircraftType::data_type>::instance().clear();
    Cache<Router::data_type>::instance().clear();
    Cache<Currency::data_type>::instance().clear();
    Cache<Restriction::data_type>::instance().clear();
    Cache<MealService::data_type>::instance().clear();
    Cache<InflService::data_type>::instance().clear();
    Cache<ServiceType::data_type>::instance().clear();
    nsi::clearCachedGeozoneRelations();
}

#define TYPE_FUNCS_IMPL(TypeName) \
namespace details \
{ \
template<> BasicNsiObject<TypeName::data_type>::BasicNsiObject(const TypeName::id_type& id) \
    : data_(Cache<data_type>::instance().get(id)) { \
    if (!data_) { \
        const std::string err(#TypeName " not found by id: " + HelpCpp::string_cast(id)); \
        throw Exception(__FILE__, __LINE__, err); \
    } \
} \
template<> BasicNsiObject<TypeName::data_type>::BasicNsiObject(const EncString& code) { \
    data_ = Cache<data_type>::instance().get(code); \
    if (!data_) { \
        const std::string err(#TypeName " not found by code (" + code.toDb() + ")"); \
        throw Exception(__FILE__, __LINE__, err); \
    } \
} \
} \
TypeName::TypeName(const TypeName::id_type& id) : details::BasicNsiObject<TypeName::data_type>(id) {} \
TypeName::TypeName(const EncString& code) : details::BasicNsiObject<TypeName::data_type>(code) {} \
BoolWithNot TypeName::find(const EncString& code) { \
    return bool(Cache<TypeName::data_type>::instance().get(code)); \
} \
template<> void clearCacheByVersion<TypeName>() { \
    return Cache<TypeName::data_type>::instance().clearByVersion(false); \
} \
template<> TypeName::id_type replaceObsoleteId<TypeName::id_type>(const TypeName::id_type& id) { \
    return cb->replaceObsoleteId(id); \
} \

TYPE_FUNCS_IMPL(Company);

CompanyData::CompanyData(const CompanyId& id) : details::BasicNsiData<CompanyId>(id)
{
}

std::string Company::accountCode() const
{
    return data().accountCode;
}

boost::optional<CompanyId> Company::findCompanyIdByAccountCode(const EncString& accCode)
{
    assert(cb);
    return cb->findCompanyIdByAccountCode(accCode);
}

TYPE_FUNCS_IMPL(DocType);

DocTypeData::DocTypeData(const DocTypeId& id) : details::BasicNsiData<DocTypeId>(id)
{
}

bool DocType::international() const
{
    return data().international;
}

bool DocType::requireAge() const
{
    return data().requireAge;
}

bool DocType::requireCitizenship() const
{
    return data().requireCitizenship;
}

TYPE_FUNCS_IMPL(SsrType);

SsrTypeData::SsrTypeData(const SsrTypeId& id) : details::BasicNsiData<SsrTypeId>(id),
    needConfirm(false), actionCode(RT_DENIED), freeText(RT_DENIED), freeTextAnswer(RT_DENIED), pass(RT_DENIED), seg(RT_DENIED)
{
}

bool SsrType::needConfirm() const
{
    return data().needConfirm;
}

#define SSR_TYPE_ID(Func, SsrCode) SsrTypeId SsrType::Func() {\
    static const SsrTypeId id = SsrType(EncString::fromDb(SsrCode)).id(); \
    return id; \
}

SSR_TYPE_ID(inftId, "INFT");
SSR_TYPE_ID(chldId, "CHLD");
SSR_TYPE_ID(seatId, "SEAT");
SSR_TYPE_ID(othsId, "OTHS");
SSR_TYPE_ID(docsId, "DOCS");
SSR_TYPE_ID(docaId, "DOCA");
SSR_TYPE_ID(docoId, "DOCO");
SSR_TYPE_ID(cbbgId, "CBBG");
SSR_TYPE_ID(exstId, "EXST");
SSR_TYPE_ID(stcrId, "STCR");
SSR_TYPE_ID(pctcId, "PCTC");
SSR_TYPE_ID(foidId, "FOID");

bool isSeatSsr(const nsi::SsrTypeId& ssr)
{
    static const std::set<nsi::SsrTypeId> seatSsrs{
        nsi::SsrType(EncString::fromUtf("SEAT")).id(),
        nsi::SsrType(EncString::fromUtf("RQST")).id(),
        nsi::SsrType(EncString::fromUtf("NSSA")).id(),
        nsi::SsrType(EncString::fromUtf("NSSB")).id(),
        nsi::SsrType(EncString::fromUtf("NSSR")).id(),
        nsi::SsrType(EncString::fromUtf("NSST")).id(),
        nsi::SsrType(EncString::fromUtf("NSSW")).id(),
        nsi::SsrType(EncString::fromUtf("SMSA")).id(),
        nsi::SsrType(EncString::fromUtf("SMSB")).id(),
        nsi::SsrType(EncString::fromUtf("SMSR")).id(),
        nsi::SsrType(EncString::fromUtf("SMST")).id(),
        nsi::SsrType(EncString::fromUtf("SMSW")).id(),
    };
    return seatSsrs.find(ssr) != seatSsrs.end();
}

bool isFQTVSsr(const nsi::SsrTypeId& ssr)
{
    return ssr == nsi::SsrType(EncString::fromUtf("FQTV")).id();
}

SsrRequiredType SsrType::actionCode() const
{
    return data().actionCode;
}

SsrRequiredType SsrType::freeText() const
{
    return data().freeText;
}

SsrRequiredType SsrType::freeTextAnswer() const
{
    return data().freeTextAnswer;
}

SsrRequiredType SsrType::pass() const
{
    return data().pass;
}

SsrRequiredType SsrType::seg() const
{
    return data().seg;
}

TYPE_FUNCS_IMPL(Geozone);

GeozoneData::GeozoneData(const GeozoneId& id) : details::BasicNsiData<GeozoneId>(id)
{
}

TYPE_FUNCS_IMPL(Country);

CountryData::CountryData(const CountryId& id, const CurrencyId& cu)
    : details::BasicNsiData<CountryId>(id), currency(cu)
{
}

boost::optional<CountryId> Country::findCountryIdByIso(const EncString& isoCode)
{
    assert(cb);
    return cb->findCountryIdByIso(isoCode);
}

boost::optional<CountryId> Country::findCountryId(const EncString& codeOrIso)
{
    if (3 == codeOrIso.to866().size()) {
        return Country::findCountryIdByIso(codeOrIso);
    } else if (2 == codeOrIso.to866().size()) {
        if (!Country::find(codeOrIso))
            return boost::none;
        return Country(codeOrIso).id();
    } else {
        return boost::none;
    }
}

const std::string& Country::isoCode() const
{
    return data().isoCode;
}

const CurrencyId Country::currency() const
{
    return data().currency;
}

TYPE_FUNCS_IMPL(Region);

RegionData::RegionData(const RegionId& id, const CountryId& c)
    : details::BasicNsiData<RegionId>(id), countryId(c)
{
}

const CountryId& Region::countryId() const
{
    return data().countryId;
}

TYPE_FUNCS_IMPL(City);

CityData::CityData(const CityId& id, const CountryId& c, const boost::optional<RegionId>& r)
    : details::BasicNsiData<CityId>(id), countryId(c), regionId(r), longitude(0), latitude(0)
{
}

const CountryId& City::countryId() const
{
    return data().countryId;
}

const boost::optional<RegionId>& City::regionId() const
{
    return data().regionId;
}

int City::longitude() const
{
    return data().longitude;
}


int City::latitude() const
{
    return data().latitude;
}

const std::string& City::timezone() const
{
    return data().timezone;
}

//TYPE_FUNCS_IMPL(Point);
namespace details
{
template<> BasicNsiObject<PointData>::BasicNsiObject(const PointId& id)
{
    const boost::optional<PointData> data = cb->findPointData(id);
    if (data)
        data_ = std::make_shared<PointData>(*data);
    else
    {
        const std::string err("Point not found by id: " + HelpCpp::string_cast(id));
        throw Exception(__FILE__, __LINE__, err);
    }
}
template<> BasicNsiObject<PointData>::BasicNsiObject(const EncString& code) {
    const boost::optional<PointId> id = cb->findPointId(code);
    boost::optional<PointData> data;
    if (id)
        data = cb->findPointData(*id);
    if (data)
        data_ = std::make_shared<PointData>(*data);
    else
    {
        const std::string err("Point not found by code (" + code.toDb() + ")");
        throw Exception(__FILE__, __LINE__, err);
    }
}
}
Point::Point(const PointId& id) : details::BasicNsiObject<PointData>(id) {}
Point::Point(const EncString& code) : details::BasicNsiObject<PointData>(code) {}
BoolWithNot Point::find(const EncString& code) {
    const boost::optional<PointId> id = cb->findPointId(code);
    return id && cb->findPointData(*id).is_initialized();
}

PointData::PointData(const PointId& id, const CityId& c)
    : details::BasicNsiData<PointId>(id), cityId(c), longitude(0), latitude(0)
{
}

const CityId& Point::cityId() const
{
    return data().cityId;
}

PointOrCode::PointOrCode(PointId p)
    : id_(nsi::Point(p).id()) // store only valid ids
{
}

PointOrCode::PointOrCode(const EncString& code)
{
    if (!nsi::Point::find(code)) {
        if (code.to866().length() == 3) {
            code_ = code;
        } else {
            id_ = nsi::Point(code).id(); // we want to throw 'not found by code' exception
        }
    } else {
        id_ = nsi::Point(code).id();
    }
}

const boost::optional<nsi::PointId>& PointOrCode::id() const
{
    return id_;
}

const EncString& PointOrCode::code(Language lang, StringChoiceMode m) const
{
    if (id_) {
        return nsi::Point(*id_).code(lang, m);
    } else {
        return code_;
    }
}

bool operator==(const PointOrCode& lhs, const PointOrCode& rhs)
{
    if (lhs.id() && rhs.id()) {
        return *lhs.id() == *rhs.id();
    } else if (!lhs.id() && !rhs.id()) {
        return lhs.code(ENGLISH) == rhs.code(ENGLISH);
    } else {
        return false;
    }
}

bool operator!=(const PointOrCode& lhs, const PointOrCode& rhs)
{
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const PointOrCode& p)
{
    if (p.id()) {
        return os << *p.id();
    } else {
        return os << '[' << p.code(ENGLISH) << ']';
    }
}

CityPoint::CityPoint(CityId c)
    : cityId_(c)
{
}

CityPoint::CityPoint(PointId p)
    : cityId_(nsi::Point(p).cityId()), pointId_(p)
{
}

CityPoint::CityPoint(const EncString& code)
    : cityId_(0)
{
    if (!Point::find(code)) {
        cityId_ = nsi::City(code).id();
    } else {
        pointId_ = nsi::Point(code).id();
        cityId_ = nsi::Point(*pointId_).cityId();
    }
}

BoolWithNot CityPoint::find(const EncString& code)
{
    if (!Point::find(code)) {
        if (!City::find(code)) {
            return false;
        }
    }
    return true;
}

EncString CityPoint::lcode() const
{
    if (pointId_) {
        return nsi::Point(*pointId_).lcode();
    }
    return nsi::City(cityId_).lcode();
}

EncString CityPoint::code(Language lang, StringChoiceMode m) const
{
    if (pointId_) {
        return nsi::Point(*pointId_).code(lang, m);
    }
    return nsi::City(cityId_).code(lang, m);
}

EncString CityPoint::name(Language lang, StringChoiceMode m) const
{
    if (pointId_) {
        return nsi::Point(*pointId_).name(lang, m);
    }
    return nsi::City(cityId_).name(lang, m);
}

CityId CityPoint::cityId() const
{
    return cityId_;
}

boost::optional<nsi::PointId> CityPoint::pointId() const
{
    return pointId_;
}

bool operator==(const CityPoint& lhs, const CityPoint& rhs)
{
    return lhs.cityId_ == rhs.cityId_
        && lhs.pointId_ == rhs.pointId_;
}

std::ostream& operator<<(std::ostream& os, const CityPoint& cp)
{
    if (cp.pointId()) {
        return os << nsi::Point(*cp.pointId()).code(ENGLISH);
    }
    return os << nsi::City(cp.cityId_).code(ENGLISH);
}

int Point::longitude() const
{
    return data().longitude;
}


int Point::latitude() const
{
    return data().latitude;
}

const std::vector<TermId>& Point::terms() const
{
    return data().terms;
}

TYPE_FUNCS_IMPL(AircraftType);

AircraftTypeData::AircraftTypeData(const AircraftTypeId& id) : details::BasicNsiData<AircraftTypeId>(id)
{
}

TYPE_FUNCS_IMPL(Router);

bool Router::blockAnswers() const
{
    return data().blockAnswers;
}

bool Router::blockRequests() const
{
    return data().blockRequests;
}
std::string Router::canonName() const
{
    return data().canonName;
}

bool Router::ediTOnO() const
{
    return data().ediTOnO;
}

bool Router::hth() const
{
    return data().hth;
}

std::string Router::hthAddress() const
{
    return data().hthAddress;
}

std::string Router::ipAddress() const
{
    return data().ipAddress;
}

short Router::ipPort() const
{
    return data().ipPort;
}

bool Router::loopback() const
{
    return data().loopback;
}

unsigned Router::maxHthPartSize() const
{
    return data().maxHthPartSize;
}

unsigned Router::maxPartSize() const
{
    return data().maxPartSize;
}

unsigned Router::maxTypebPartSize() const
{
    return data().maxTypebPartSize;
}

std::string Router::ourHthAddres() const
{
    return data().ourHthAddres;
}

unsigned Router::remAddrNum() const
{
    return data().remAddrNum;
}

unsigned Router::responseTimeout() const
{
    return data().responseTimeout;
}

bool Router::censor() const
{
    return data().censor;
}

bool Router::translit() const
{
    return data().translit;
}

bool Router::sendContrl() const
{
    return data().sendContrl;
}

std::string Router::senderName() const
{
    return data().senderName;
}

unsigned Router::tprLen() const
{
    return data().tprLen;
}

bool Router::trueTypeB() const
{
    return data().trueTypeB;
}

bool Router::tpb() const
{
    return data().tpb;
}

bool Router::email() const
{
    return data().email;
}

RouterData::RouterData(const RouterId& id)
    : details::BasicNsiData<RouterId>(id)
    , blockAnswers(false), blockRequests(false), ediTOnO(false), hth(false)
    , ipPort(0), loopback(false), maxHthPartSize(0), maxPartSize(0), maxTypebPartSize(0)
    , remAddrNum(0), responseTimeout(0), translit(false), sendContrl(false), tprLen(0),
    trueTypeB(false), tpb(false), email(false)
{
}

boost::optional<int> Currency::isoNum() const
{
    return data().isoNum;
}

TYPE_FUNCS_IMPL(Currency);

CurrencyData::CurrencyData(const CurrencyId& id)
    : details::BasicNsiData<CurrencyId>(id)
{
}

int restrictionsCount();
std::vector<RestrictionId> Restriction::restrictions()
{
    static std::vector<RestrictionId> res;
    if (res.empty()) {
        int amount = restrictionsCount();
        for (int i = 1; i <= amount; ++i)
            res.push_back(RestrictionId(i));
    }
    return res;
}

TYPE_FUNCS_IMPL(Restriction);

RestrictionData::RestrictionData(const RestrictionId& id)
    : details::BasicNsiData<RestrictionId>(id)
{
}

RestrictionData::RestrictionData(int id, const EncString& code_en, const EncString& name_en, const EncString& name_ru)
    : details::BasicNsiData<RestrictionId>(RestrictionId(id))
{
    codes[ENGLISH] = code_en;
    names[ENGLISH] = name_en;
    names[RUSSIAN] = name_ru;
}

int mealServicesCount();
std::vector<MealServiceId> MealService::mealServices()
{
    static std::vector<MealServiceId> res;
    if (res.empty()) {
        int amount = mealServicesCount();
        for (int i = 1; i <= amount; ++i)
            res.push_back(MealServiceId(i));
    }
    return res;
}

TYPE_FUNCS_IMPL(MealService);

MealServiceData::MealServiceData(const MealServiceId& id)
    : details::BasicNsiData<MealServiceId>(id)
{
}

MealServiceData::MealServiceData(int id, const EncString& code_en, const EncString& name_en, const EncString& name_ru)
    : details::BasicNsiData<MealServiceId>(MealServiceId(id))
{
    codes[ENGLISH] = code_en;
    names[ENGLISH] = name_en;
    names[RUSSIAN] = name_ru;
}

int inflServicesCount();
std::vector<InflServiceId> InflService::inflServices()
{
    static std::vector<InflServiceId> res;
    if (res.empty()) {
        int amount = inflServicesCount();
        for (int i = 1; i <= amount; ++i)
            res.push_back(InflServiceId(i));
    }
    return res;
}

TYPE_FUNCS_IMPL(InflService);

InflServiceData::InflServiceData(const InflServiceId& id)
    : details::BasicNsiData<InflServiceId>(id)
{
}

InflServiceData::InflServiceData(int id, const EncString& code_en, const EncString& name_en, const EncString& name_ru)
    : details::BasicNsiData<InflServiceId>(InflServiceId(id))
{
    codes[ENGLISH] = code_en;
    names[ENGLISH] = name_en;
    names[RUSSIAN] = name_ru;
}

ENUM_NAMES_BEGIN(FltServiceType)
    (REGULAR,    "REGULAR")
    (ADDITIONAL, "ADDITIONAL")
    (CHARTER,    "CHARTER")
    (OTHER,      "OTHER")
ENUM_NAMES_END(FltServiceType)
ENUM_NAMES_END_AUX(FltServiceType)

int serviceTypesCount();
std::vector<ServiceTypeId> ServiceType::serviceTypes()
{
    static std::vector<ServiceTypeId> res;
    if (res.empty()) {
        int amount = serviceTypesCount();
        for (int i = 1; i <= amount; ++i)
            res.push_back(ServiceTypeId(i));
    }
    return res;
}

TYPE_FUNCS_IMPL(ServiceType);

ServiceTypeData::ServiceTypeData(const ServiceTypeId& id)
    : details::BasicNsiData<ServiceTypeId>(id)
{
}

ServiceTypeData::ServiceTypeData(int id, const EncString& code_en, FltServiceType i_fltType,
                                 const EncString& name_en, const EncString& name_ru)
    : details::BasicNsiData<ServiceTypeId>(ServiceTypeId(id)), fltType(i_fltType)
{
    codes[ENGLISH] = code_en;
    names[ENGLISH] = name_en;
    names[RUSSIAN] = name_ru;
}

} // nsi
