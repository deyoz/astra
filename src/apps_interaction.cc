#include "apps_interaction.h"
#include "alarms.h"
#include "astra_misc.h"
#include "exceptions.h"
#include "points.h"
#include "qrys.h"
#include "trip_tasks.h"
#include "tlg/tlg.h"
#include "apis_utils.h"
#include "checkin.h"
#include "ckin_search.h"
#include "passenger.h"

#include "pg_session.h"
#include <serverlib/pg_cursctl.h>
#include <serverlib/pg_rip.h>

#include <serverlib/str_utils.h>
#include <serverlib/tcl_utils.h>
#include <serverlib/cursctl.h>
#include <serverlib/dump_table.h>
#include <serverlib/algo.h>
#include <serverlib/rip_oci.h>
#include <serverlib/dates_oci.h>
#include <serverlib/dates_io.h>

#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>
#include <functional>
#include <ctime>
#include <deque>
#include <boost/regex.hpp>
#include <sstream>

#define NICKNAME "FELIX"
#define NICKTRACE FELIX_TRACE
#include <serverlib/slogger.h>




using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC::date_time;
using namespace ASTRA;

// реализованы версии сообщений 21 и 26

// static const std::string ReqTypeCirq = "CIRQ";
// static const std::string ReqTypeCicx = "CICX";
// static const std::string ReqTypeCimr = "CIMR";

// static const std::string AnsTypeCirs = "CIRS";
// static const std::string AnsTypeCicc = "CICC";
// static const std::string AnsTypeCima = "CIMA";

// static const std::pair<std::string, int> FltTypeChk("CHK", 2);
// static const std::pair<std::string, int> FltTypeInt("INT", 8);
// static const std::pair<std::string, int> FltTypeExo("EXO", 4);
// static const std::pair<std::string, int> FltTypeExd("EXD", 4);
// static const std::pair<std::string, int> FltTypeInm("INM", 3);

// static const std::pair<std::string, int> PaxReqPrq("PRQ", 22);
// static const std::pair<std::string, int> PaxReqPcx("PCX", 20);

// static const std::pair<std::string, int> PaxAnsPrs("PRS", 27);
// static const std::pair<std::string, int> PaxAnsPcc("PCC", 26);

// static const std::pair<std::string, int> MftReqMrq("MRQ", 3);
// static const std::pair<std::string, int> MftAnsMak("MAK", 4);

// static const std::string AnsErrCode = "ERR";

// static const std::string APPSFormat = "APPS_FMT";

namespace APPS {
class AppsPaxNotFoundException : public std::exception {} ;

// версия спецификации документа
enum ESpecVer {SPEC_6_76, SPEC_6_83};
enum HolderType{Crs, Pax};
enum MessageType{CIRQ, CICX, CIMR};

const int pax_seq_num = 1;

static const int MaxCirqPaxNum = 5;
static const int MaxCicxPaxNum = 10;

const std::string APPS_FORMAT_21 = "APPS_21";
const std::string APPS_FORMAT_26 = "APPS_26";
const int APPS_VERSION_21 = 21;
const int APPS_VERSION_26 = 26;
const int APPS_VERSION_27 = 27;

class AppsSettings;
class TransactionData;
class FlightData;
class PaxData;
class PaxAddData;
class AppsPaxDTO;
class CheckInInfo;
class APPSMessage;
class IRequest;
class PaxRequest;
class ManifestRequest;
class APPSAnswer;
class PaxReqAnswer;
class ManifestAnswer;
class PaxRemCallbacks;
class PaxChangesCallbacks;
class AppsTasks;
class AppsSetsCallbacksCache;
class AppsPaxDTO;

template <typename Container>
void traceContainer(const Container& cont, const std::string& hint)
{
    LogTrace(TRACE5) << __FUNCTION__ << " " << hint << " size: " << cont.size();
    for(const auto & item : cont) {
        LogTrace(TRACE5) << item << " ";
    }
}



// Работа с версионностью
int fieldCount(const std::string& data_group, int version);
ESpecVer getSpecVer(int msg_ver);
static int versionByFormat(const std::string& fmt);
ASTRA::TGender::Enum femaleToGender(int is_female);

//Получение APPS настроек
bool checkAPPSSegment(const TPaxSegmentPair & seg);
std::set<AppsSettings> appsSetsForSegment(const TPaxSegmentPair &flt);
std::set<AppsSettings> appsSetsForSegment(const TAdvTripRouteItem& dep, const TAdvTripRouteItem& arv);
Opt<AppsSettings> appsSets(const AirlineCode_t& airline, const CountryCode_t& country);
Opt<AppsSettings> appsSetsForTripItem(const TAdvTripRouteItem& item);
std::set<AppsSettings> appsSetsForRoute(const TAdvTripRoute& route);
std::set<AppsSettings> getAppsSetsByPaxId(const PaxId_t &pax_id);
std::set<AppsSettings> getAppsSetsByCrsPaxId(const PaxId_t &pax_id);
std::set<AppsSettings> filterByPreCheckin(const std::set<AppsSettings> & sets);
//std::set<AppsSettings> filterByInboundOutbound(const std::set<AppsSettings> &sets);
std::set<std::string> needFltCloseout(const std::set<std::string>& countries, const AirlineCode_t &airline);

// Обработка трансферных пассажиров
TransferFlag getTransferFlagFromTransit(const PointId_t &point_id, const AirportCode_t& airp_arv);
TransferFlag getPaxTransferFlag(const PointId_t& point_id, const GrpId_t& pax_grp_id,
                                const AirportCode_t& airp_dep, const AirportCode_t& airp_arv);
TransferFlag getCrsTransferFlag(const map<int, CheckIn::TTransferItem> & trfer, const PointId_t &point_id,
                                const AirportCode_t& airp_arv);
bool isPointTransferDestination(const GrpId_t &pax_grp_id, const AirportCode_t& airp_arv);
bool isPointTransferOrigin(const GrpId_t &pax_grp_id, const AirportCode_t& airp_dep);

// Работа с тревогами
static void syncAlarms(const PaxId_t &pax_id, const PointId_t &point_id_spp);
static void addAlarm(const PaxId_t &pax_id, const std::initializer_list<Alarm::Enum>& alarms,
                         const PointId_t &point_id_spp = PointId_t(ASTRA::NoExists));
static void deleteAlarm(const PaxId_t& pax_id, const std::initializer_list<Alarm::Enum>& alarms,
                            const PointId_t& point_id_spp = PointId_t(ASTRA::NoExists));

void deleteAlarms(const PaxId_t &pax_id, const PointId_t &point_id_spp);
/* Утилиты для заполнения данных PaxRequest и др.*/
// TransactionData
static int getIdent();
static std::string getUserId(const AirlineCode_t& airl);
static std::string makeHeader(const AirlineCode_t& airl, int msgId);
static std::string getH2HReceiver();
static std::string getH2HTpr(int msgId);

// FlightData
void checkRouteSuffix(const TAdvTripRoute &route);

// PaxData
std::string outDocType(int version, const std::string &doc_type);
ASTRA::TTrickyGender::Enum getTrickyGender(const std::string &pers_type, ASTRA::TGender::Enum gender);
std::string givenNames(const CheckIn::TPaxDocItem& doc, const Name_t& name);
std::string familyName(const CheckIn::TPaxDocItem& doc, const Surname_t& surname);
std::string passReference(const ASTRA::TTrickyGender::Enum tricky_gender, bool isCrew, const Opt<RegNo_t> &reg_no);
// PaxAddData
std::string issuePlaceToCountry(const std::string& issue_place);
// Общие
bool isInternationalFlight(const AirportCode_t& airp_dep, const AirportCode_t& airp_arv);
bool isInternationalFlight(const TPaxSegmentPair& seg);
/***********************************************************/
static TransactionData createTransactionData(MessageType type, bool pre_checkin,
                                             const AirlineCode_t& airl);
static FlightData createFlightData(const TPaxSegmentPair &flt, const string &type);
static Opt<FlightData> createCkinFlightData(const GrpId_t &grp_id);
static PaxData createPaxData(const PaxId_t &paxId, const Surname_t& surname, const Name_t & name,
                         bool is_crew, const std::string& override_type,
                         const Opt<RegNo_t> &reg_no, ASTRA::TTrickyGender::Enum tricky_gender,
                         TransferFlag trfer);
static PaxAddData createPaxAddData(const PaxId_t &pax_id, const CheckIn::TPaxDocoItem &doco, PaxOrigin origin);
static Opt<ManifestRequest> createManifestReq(const TPaxSegmentPair &flt, const string &country_lat);

// Отправка APPS сообщений
void processPax(const PaxId_t &pax_id, const std::string& override_type = "", bool is_forced = false);
void processCrsPax(const PaxId_t& pax_id, const std::string& override_type = "");
//void updateCrsNeedApps(const PaxId_t &pax_id);
void sendAppsMessages(const PaxId_t &pax_id, const std::set<AppsSettings> &apps_settings,
                      HolderType reqType, const std::string& override_type,
                      bool is_forced=false);
bool isAlreadyCheckedIn(const PaxId_t &pax_id);
static void sendInfo(const PointId_t &point_id, const PointIdTlg_t &point_id_tlg);
//bool checkTime(const PointId_t& point_id);
// Обработка ответов
std::vector<Error> getAnswerErrors(const std::string& source);
std::string getAnsText(const std::string& tlg);
std::unique_ptr<APPSAnswer> createAnswer(const std::string& code, const std::string& answer);
std::unique_ptr<PaxReqAnswer> createPaxReqAnswer(const std::string& code, const std::string& source);
std::unique_ptr<ManifestAnswer> createManifestAnswer(const std::string& code, const std::string& source);


class AppsSettings
{
public:
    static Opt<AppsSettings> readSettings(const AirlineCode_t& airline, const CountryCode_t& country);
    static Opt<AppsSettings> readSettings(const AppsSettingsId_t & id);
    int version()               const { return versionByFormat(format);}
    AirlineCode_t getAirline()  const { return airline;      }
    CountryCode_t getCountry()  const { return country;      }
    std::string getFormat()     const { return format;       }
    std::string getRouter()     const { return router;       }
    AppsSettingsId_t  getId()   const { return id;           }
    bool getFltCloseout()       const { return flt_closeout; }
    bool getInbound()           const { return inbound;      }
    bool getOutbound()          const { return outbound;     }
    bool getDenial()            const { return denial;       }
    bool getPreCheckin()        const { return pre_checkin;  }
    friend bool operator < (const AppsSettings& lhs, const AppsSettings& rhs)
    {
        return lhs.id.get() < rhs.id.get();
    }
    void setRouter(const std::string& router)
    {
        this->router = router;
    }
    AppsSettings(AirlineCode_t airl, CountryCode_t country, const std::string& format,
                 const std::string& router, const AppsSettingsId_t& id, bool flt_closeout,
                 bool inbound, bool outbound, bool denial, bool pre_checkin):
        airline(airl),
        country(country),
        format(format),
        router(router),
        id(id),
        flt_closeout(flt_closeout),
        inbound(inbound),
        outbound(outbound),
        denial(denial),
        pre_checkin(pre_checkin)
    {}
private:
    AirlineCode_t airline;
    CountryCode_t country;
    std::string format;
    std::string router;  //for future
    AppsSettingsId_t id;
    bool flt_closeout;
    bool inbound;
    bool outbound;
    bool denial;
    bool pre_checkin;
};

class TransactionData
{
public:
    std::string code;
    std::string user_id;
    bool pre_checkin; // '' = Check-in Transaction. "P" = Pre-Check-in Vetting
    void validateData() const;
    std::string msg(int version, MessageType type) const;
};

class FlightData
{
public:
    std::string type;
    PointId_t point_id;
    std::string flt_num; // Airline and flight number.
    AirportCode_t dep_port; // IATA Airport Code
    Dates::DateTime_t dep_date;
    AirportCode_t arv_port;  // IATA Airport Code
    Dates::DateTime_t arv_date;

    void validateData() const;
    std::string msg(int version) const;
    FlightData& setPointId(const PointId_t& pointId)  {this->point_id = pointId;  return *this;}
    FlightData& setType(const std::string& type)      {this->type     = type;     return *this;}
    FlightData& setFltNum(const std::string &flt_num) {this->flt_num  = flt_num;  return *this;}
};


class PaxData
{
public:
    PaxId_t pax_id;
    std::string apps_pax_id; // только для "PCX". Уникальный ID пассажира. Получаем в ответ на CIRQ запрос
    std::string status;
    std::string pax_crew; // 'C' или 'P'
    std::string nationality;
    std::string issuing_state; // optional для "PCX"
    std::string passport;
    std::string check_char; // optional
    std::string doc_type; // optional, 'P' - пасспорт, 'O' - другой документ, 'N' - нет документа
    std::string doc_subtype;
    std::string expiry_date; //CCYYMMDD
    std::string sup_doc_type; // в настоящее время не используется
    std::string sup_passport; // в настоящее время не используется
    std::string sup_check_char; // в настоящее время не используется
    std::string family_name; // от 2 до 24 символов
    std::string given_names; // до 24 символов, если нет - '-'.
    std::string birth_date; // CCYYMMDD. If the day is not known, use CCYYMM00
    std::string sex; // "M" = Male, "F" = Female, "U" = Unspecified, "X" = Unspecified
    std::string birth_country; // optional
    std::string endorsee; // Blank for passport holder, "S" = Endorsee. Optional
    std::string trfer_at_origin; // optional
    std::string trfer_at_dest; // optional
    std::string override_codes; /* Optional. Format is: CxxCxxCxxCxxCxx,
    where "C" = Override code ('A' = airline override, 'G' = government override),
    "xx" = IATA country code */
    std::string pnr_source; // conditional
    std::string pnr_locator; // conditional
    std::string reference; // conditional, version 24
    void validateData() const;
    std::string msg(int version, MessageType type) const;
};

class PaxAddData // Passenger Additional Data
{
public:
    //int version = APPS_VERSION_26;
    std::string country_for_data; // Country for Additional Data // 4
    std::string doco_type; // Document Type // 5
    std::string doco_no; // Document Number X(20) // 6
    std::string country_issuance; // Country of Issuance // 7
    std::string doco_expiry_date; // Expiration Date CCYYMMDD // 8
    std::string num_street; // Address: Number and Street // 9
    std::string city; // Address: City // 10
    std::string state; // Address: State // 11
    std::string postal_code; // Address: Postal Code // 12
    std::string redress_number; // Passenger Redress Number // 13 // omit
    std::string traveller_number; // Known Traveller Number // 14 // omit

    static Opt<PaxAddData> createByPaxId(const PaxId_t &pax_id);
    static Opt<PaxAddData> createByCrsPaxId(const PaxId_t &pax_id);

    void validateData() const;
    std::string msg(int version) const;
};

class ManifestData
{
public:
    std::string country; // 2-character IATA Country Code of the participating country.
    std::string mft_pax; // The only option supported for UAE will be "C".
    std::string mft_crew; // The only option supported for UAE will be "C".

    explicit ManifestData(const std::string& code) : country(code), mft_pax("C"), mft_crew("C") {}
    void validateData() const;
    std::string msg(int version) const;
};

class PaxRequest
{
public:
    PaxRequest(const TransactionData& trs, const FlightData& inflight, Opt<FlightData> ckinflight,
             const PaxData& pax, Opt<PaxAddData> paxadd, const AppsSettingsId_t& id = AppsSettingsId_t(ASTRA::NoExists))
             : trans(trs), int_flt(inflight), ckin_flt(ckinflight), pax(pax), pax_add(paxadd)
    {
    }

    static Opt<PaxRequest> createFromPaxDB(HolderType type, const PaxId_t &pax_id, const std::string& override_type="");

    bool isNeedCancel(const std::string& status) const;
    bool isNeedUpdate(const std::string& status, bool is_the_same) const;
    bool isNeedNew(bool is_exists, const std::string& status, bool is_the_same, bool is_forced) const;
    void ifNeedDeleteAlarms(bool is_exists, const std::string& status) const;
    void ifOutOfSyncSendAlarm(const std::string& status, bool is_the_same) const;
    void ifNeedDeleteApps(const std::string& status, bool needNew, bool needCancel, bool needUpdate) const;

    std::string msg(int version, MessageType type) const;
    PointId_t getPointId() const
    {
        return getIntFlt().point_id;
    }

    const FlightData& getIntFlt() const
    {
        return int_flt;
    }
    const TransactionData& getTrans() const
    {
        return trans;
    }
    const PaxData& getPax() const
    {
        return pax;
    }
    const std::string getStatus() const
    {
        return pax.status;
    }

private:
    TransactionData trans;
    FlightData int_flt;
    Opt<FlightData> ckin_flt;
    PaxData pax;
    Opt<PaxAddData> pax_add;
    static Opt<PaxRequest> createByPaxId(const PaxId_t &pax_id,
                                         const std::string& override_type);
    static Opt<PaxRequest> createByCrsPaxId(const PaxId_t& pax_id, const std::string& override_type);
    static Opt<PaxRequest> createByFields(const HolderType reqType, const PaxId_t& pax_id,
                                          const APPS::CheckInInfo &info, bool pre_checkin,
                                          ASTRA::TGender::Enum gender, const std::string& override_type,
                                          const AirlineCode_t &airline, bool need_del, Opt<RegNo_t> reg_no);
};


class ManifestRequest
{
private:
    TransactionData trans;
    FlightData int_flt;
    ManifestData mft_req;
public:
    PointId_t getPointId() const
    {
        return getIntFlt().point_id;
    }
    std::string msg(int version, MessageType type) const;
    const FlightData& getIntFlt() const
    {
        return int_flt;
    }
    const TransactionData& getTrans() const
    {
        return trans;
    }
    ManifestRequest(const TransactionData &trs, const FlightData & flt, const ManifestData & mft) :
        trans(trs), int_flt(flt), mft_req(mft) {}
    ManifestRequest(const TPaxSegmentPair& flt, const std::string& country_lat);
};

class AnsPaxData
{
public:
    std::string country; // 2-character IATA Country Code of the country generating the response.
    int code; /* Статусы для CIRS: 8501 OK to Board, 8502 Do Not Board, 8503 Board if Docs OK, 8517 Override Accepted, 8630 Contact UAE govt.
    Статусы для CIСС: 8505 Cancelled, 8506 No Record */
    std::string status; /* Статусы для CIRS: "B" = OK to Board, "D" = Do Not Board (override is possible), "X" = Do Not Board (override is impossible),
    "U" = Unable to determine status, "I" = Not enought data, "T" = Timeout, "E" = Error.
    Статусы для CIСС: "C" = Cancelled, "N" = Not found, "T" = Timeout, "E" = Error. */
    std::string apps_pax_id;
    int error_code1; // Conditional (on error condition)
    std::string error_text1; // Conditional (on error condition)
    int error_code2; // Conditional (on error condition)
    std::string error_text2; // Conditional (on error condition)
    int error_code3; // Conditional (on error condition)
    std::string error_text3; // Conditional (on error condition)
    int version = 0;

    AnsPaxData() : code(ASTRA::NoExists), error_code1(ASTRA::NoExists),
                  error_code2(ASTRA::NoExists), error_code3(ASTRA::NoExists) {}
    std::string toString() const;
    void init(const string &source, int ver);
};

class Error
{
public:
    std::string country; /* 2-character IATA Country Code of the country generating the response.
    The field may be null if the error is not specific to any country. */
    int error_code; // Conditional (on error condition)
    std::string error_text; // Conditional (on error condition)
    Error() : error_code(ASTRA::NoExists) {}
};


class APPSMessage
{
public:
    APPSMessage(const AppsSettings & settings, const std::string& msg_text, const PointId_t& point_id, int msg_id);
    APPSMessage(const AppsSettingsId_t& settings_id, const PointId_t& point_id, const std::string& msg_text,
                int msg_id, int send_attempts, const Dates::DateTime_t& send_time);
    static Opt<APPSMessage> readAppsMsg(int msg_id);

    ~APPSMessage() = default;
    APPSMessage(const APPSMessage & msg)           = delete;
    APPSMessage& operator=(const APPSMessage &msg) = delete;
    APPSMessage(APPSMessage && other)              = default;
    APPSMessage & operator = (APPSMessage &&msg)   = default;

    void send();
    void save() const;
    void incAttempts();

    AppsSettingsId_t getSettingsId() const
    {
        return settings_id;
    }
    std::string getMsg() const
    {
        return msg;
    }
    int getMsgId() const
    {
        return msg_id;
    }
    PointId_t getPointId() const
    {
        return point_id;
    }
    int getSendAttempts() const
    {
        return send_attempts;
    }
    Dates::DateTime_t getSendTime() const
    {
        return send_time;
    }
    void setMsgId(const int id)
    {
        msg_id = id;
    }
    int getVersion() const;

private:
    AppsSettingsId_t settings_id;
    PointId_t point_id;
    std::string msg;
    int msg_id;
    int send_attempts = 0;
    Dates::DateTime_t send_time;
};

Opt<PointId_t> pointIdByMsgId(int msg_id)
{
    int point_id = 0;
    auto cur = get_pg_curs("select POINT_ID from APPS_PAX_DATA where (CIRQ_MSG_ID = :msg_id or CICX_MSG_ID = :msg_id) ");
    cur
        .def(point_id)
        .bind(":msg_id", msg_id)
        .EXfet();
    if(cur.err() == PgCpp::NoDataFound) {
        LogTrace(TRACE5) << __FUNCTION__ << " Query error. Not found data by msg_id: " << msg_id;
        return boost::none;
    }
    return PointId_t(point_id);
}

class AppsPaxDTO
{
public:
    explicit AppsPaxDTO(PgCpp::CursCtl &cursor) : cur(cursor)
    {
    }

    bool execute()
    {
        define();
        cur.EXfet();
        return cur.err() != PgCpp::NoDataFound;
    }

    void exec()
    {
        define();
        cur.exec();
    }

    std::string  paxStatus() const
    {
        return status;
    }

    std::string requestMsg() const
    {
        return cirq_msg_text;
    }

    static void save(const PaxRequest & request, const std::string& cirq_msg_text, const std::string& cicx_msg_text,
                     int cirq_msg_id, const AppsSettingsId_t & settings_id);
    static Opt<AppsPaxDTO> load(const PaxId_t &pax_id, const AppsSettingsId_t & settings_id);
    static Opt<AppsPaxDTO> load(int msg_id);
    static void deleteAPPSData(const std::string & field,  int value);
    std::string createCicxText();
    void updateCicxMsgId(const int msg_id);
    void updateStatus(const std::string& new_status, const std::string& new_apps_pax_id, int msg_id);

    void define()
    {
        cur
            .def    (pax_id)
            .def(point_id)
            .defNull(apps_pax_id,"")
            .defNull(status,"")
            .def(cirq_msg_id)
            .defNull(cicx_msg_id, ASTRA::NoExists)
            .def(cirq_msg_text)
            .defNull(cicx_msg_text, "")
            .defNull(settings_id, ASTRA::NoExists);
    }
    static const std::string AppsPaxDataReadQuery;
private:
    PgCpp::CursCtl &cur;
    std::string apps_pax_id;     // только для "PCX". Уникальный ID пассажира. Получаем в ответ на CIRQ запрос
    std::string status;
    std::string cirq_msg_text;
    std::string cicx_msg_text;
    int pax_id;
    int point_id;
    int cirq_msg_id;
    int cicx_msg_id;
    int settings_id;
};


const std::string AppsPaxDTO::AppsPaxDataReadQuery = "select PAX_ID, POINT_ID, APPS_PAX_ID, STATUS, CIRQ_MSG_ID, CICX_MSG_ID, "
                                         " CIRQ_MSG_TEXT, CICX_MSG_TEXT, SETTINGS_ID "
                                         "from APPS_PAX_DATA ";

void AppsPaxDTO::save(const PaxRequest & request, const std::string& cirq_msg_text, const std::string& cicx_msg_text,
                      int cirq_msg_id, const AppsSettingsId_t & settings_id)
{
    LogTrace(TRACE5) << __FUNCTION__ << " " << request.getTrans().code << ":" << cirq_msg_id;

    Dates::DateTime_t nowUtc = Dates::second_clock::universal_time();
    auto cur = get_pg_curs(
               "INSERT INTO apps_pax_data "
               "(pax_id, point_id, cirq_msg_id, cirq_msg_text, cicx_msg_text, settings_id, send_time, family_name) "
               "VALUES (:pax_id, :point_id, :cirq_msg_id, :cirq_msg_text, :cicx_msg_text, :settings_id, :send_time, :family_name)");
    cur
        .bind(":cirq_msg_id",      cirq_msg_id)
        .bind(":pax_id",           request.getPax().pax_id)
        .bind(":point_id",         request.getPointId())
        .bind(":cirq_msg_text",    cirq_msg_text)
        .bind(":cicx_msg_text",    cicx_msg_text)
        .bind(":settings_id",      settings_id)
        .bind(":send_time",        nowUtc)
        .bind(":family_name",      request.getPax().family_name)
        .exec();
}

void AppsPaxDTO::deleteAPPSData(const std::string & field,  int value)
{
    LogTrace(TRACE5) << __FUNCTION__ << " field: " << field << " value: " << value;

    auto cur = get_pg_curs(
               "delete from APPS_PAX_DATA where "+field+ "= :val");
    cur.bind(":val", value).exec();
}

Opt<AppsPaxDTO> AppsPaxDTO::load(const PaxId_t& pax_id, const AppsSettingsId_t& settings_id)
{
    LogTrace(TRACE3) << __FUNCTION__ << " by pax_id:" << pax_id;
    auto cur = get_pg_curs(
               AppsPaxDataReadQuery + "where PAX_ID = :pax_id and SETTINGS_ID = :settings_id");
    cur.bind(":pax_id", pax_id)
       .bind(":settings_id", settings_id);
    AppsPaxDTO appsReader(cur);
    if(!appsReader.execute())
    {
        LogTrace(TRACE5) << __FUNCTION__ << " Apps mapper can't execute ";
        return boost::none;
    }
    return appsReader;
}

Opt<AppsPaxDTO> AppsPaxDTO::load(int msg_id)
{
    LogTrace(TRACE3) << __FUNCTION__ << " by msg_id:" << msg_id;
    auto cur = get_pg_curs(
               AppsPaxDataReadQuery + "where CIRQ_MSG_ID = :msg_id");
    cur.bind(":msg_id", msg_id);
    AppsPaxDTO appsReader(cur);
    if(!appsReader.execute())
    {
        LogTrace(TRACE5) << __FUNCTION__ << " Apps mapper can't execute ";
        return boost::none;
    }
    return appsReader;
}


std::string AppsPaxDTO::createCicxText()
{
    LogTrace(TRACE5) << __FUNCTION__ << " apps_pax_id: "<< apps_pax_id;
    StrUtils::replaceSubstr(cicx_msg_text, "%apps_pax_id%", apps_pax_id);
    return cicx_msg_text;
}

void AppsPaxDTO::updateCicxMsgId(const int msg_id)
{
    LogTrace(TRACE5) << __FUNCTION__ << " msg_id: "<< msg_id;
    auto cur = get_pg_curs(
               "update APPS_PAX_DATA set CICX_MSG_ID = :cicx_msg_id "
               " where CIRQ_MSG_ID = :cirq_msg_id ");
    cur
       .bind(":cicx_msg_id", msg_id)
       .bind(":cirq_msg_id", cirq_msg_id)
       .exec();
}

void AppsPaxDTO::updateStatus(const std::string& new_status, const std::string& new_apps_pax_id, int msg_id)
{
    LogTrace(TRACE5) << __FUNCTION__ << " msg_id: " << msg_id << " status: " << status
                     << " apps_pax_id: " << new_apps_pax_id;

    auto cur = get_pg_curs(
               "update APPS_PAX_DATA set STATUS = :status, APPS_PAX_ID = :apps_pax_id "
               " where CIRQ_MSG_ID = :cirq_msg_id ");
    cur.bind(":status", new_status)
       .bind(":apps_pax_id", new_apps_pax_id)
       .bind(":cirq_msg_id", msg_id)
       .exec();

}

void dumpAppsMsg()
{
    Dates::DateTime_t send_time;
    int msg_id;
    int send_attempts;

    auto cur = get_pg_curs("select SEND_TIME, MSG_ID, SEND_ATTEMPTS from APPS_MESSAGES");
    cur.def(send_time).def(msg_id).def(send_attempts).exec();
    while(!cur.fen()) {
        LogTrace(TRACE5) << "DUMP MSGS: " << " time: " << send_time << " msg_id: " << msg_id << " attempts: " << send_attempts;
    }
}

void dumpAppsPaxData()
{
    int pax_id, point_id, cirq_msg_id, cicx_msg_id, settings_id;
    std::string status, apps_pax_id, family;
    auto cur = get_pg_curs("select PAX_ID, FAMILY_NAME, POINT_ID, APPS_PAX_ID, STATUS, CIRQ_MSG_ID, CICX_MSG_ID, "
                           " SETTINGS_ID from APPS_PAX_DATA ");
    cur.def(pax_id).def(family).def(point_id).defNull(apps_pax_id, "NULL")
            .defNull(status, "NULL").def(cirq_msg_id).defNull(cicx_msg_id, 0).defNull(settings_id,0).exec();
    while(!cur.fen()) {
        LogTrace(TRACE5) << " DUMP APPS: " << " family: " << family << " cirq_msg_id: " << cirq_msg_id << " cicx_msg_id: " << cicx_msg_id
                         << " pax_id: " << pax_id << " point_id: " << point_id << " settings_id: " << settings_id
                         << " status : " << status << " apps_pax_id: " << apps_pax_id;
    }
}


class APPSAnswer
{
protected:
    std::string code;
    std::vector<Error> errors; // Error (repeating). Conditional
    APPSMessage msg;
    bool checkIfNeedResend() const;
    virtual LEvntPrms getLogParams(const string &country, int status_code,
                                   int error_code, const std::string& error_text) const;
    APPSAnswer(const std::string& code, const std::string& source, APPSMessage msg) :
        code(code), msg(move(msg))
    {
        readAnswerErrors(source);
    }
    void readAnswerErrors(const std::string& source);

public:
    virtual ~APPSAnswer() = default;
    virtual std::string toString() const;
    virtual void beforeProcessAnswer() const;
    virtual void processErrors() const = 0;
    virtual void processAnswer() const = 0;
    virtual void logAnswer(const std::string& country, int status_code,
                           int error_code, const std::string& error_text) const = 0;
};

class PaxReqAnswer : public APPSAnswer
{
private:
    PaxId_t pax_id;
    std::string family_name;
    // PassengerData. Applicable to "CIRS" and "CIСС". Conditional (required if ERR data group is not present)
    std::vector<AnsPaxData> passengers;
public:
    PaxReqAnswer(const std::string& code, const std::string& source, APPSMessage msg,
                 const PaxId_t& pax_id, const std::string& family_name, std::vector<AnsPaxData> passengers):
        APPSAnswer(code, source, move(msg)),
        pax_id(pax_id),
        family_name(family_name),
        passengers(move(passengers))
    {}
    std::string toString() const override;
    void processErrors() const override;
    void processAnswer() const override;
    void logAnswer(const std::string &country, int status_code,
                   int error_code, const std::string& error_text) const override;
};

class ManifestAnswer : public APPSAnswer
{
private:
    // Manifest response. Applicable to "CIMA". Conditional (required if ERR data group is not present)
    std::string country; // 2-character IATA Country Code of the participating country.
    int resp_code; // 8700 = Request processed, 8701 = Request rejected
    int error_code; // Conditional (on error condition)
    std::string error_text; // Conditional (on error condition)

public:
    ManifestAnswer(const std::string& code, const std::string& source, APPSMessage msg,
                 const std::string& country, int resp_code, int error_code, const std::string& error_text):
      APPSAnswer(code, source, move(msg)),
      country(country),
      resp_code(resp_code),
      error_code(error_code),
      error_text(error_text)
    {}

    std::string toString() const override;
    void processErrors() const override;
    void processAnswer() const override;
    void logAnswer(const std::string &country, int status_code,
                 int error_code, const std::string& error_text) const override;
};


APPSMessage::APPSMessage(const AppsSettings &settings, const string &msg_text, const PointId_t &point_id, int msg_id)
    : settings_id(settings.getId()),
      point_id(point_id),
      msg(msg_text),
      msg_id(msg_id),
      send_time(Dates::second_clock::universal_time())
{
    StrUtils::replaceSubstr(msg, "%msg_id%", std::to_string(msg_id));
}

APPSMessage::APPSMessage(const AppsSettingsId_t &settings_id, const PointId_t &point_id, const string &msg_text,
                         int msg_id, int send_attempts, const Dates::DateTime_t &send_time):
    settings_id(settings_id),
    point_id(point_id),
    msg(msg_text),
    msg_id(msg_id),
    send_attempts(send_attempts),
    send_time(send_time)
{
    StrUtils::replaceSubstr(msg, "%msg_id%", std::to_string(msg_id));
}

Opt<APPSMessage> APPSMessage::readAppsMsg(int msg_id)
{
    int settings_id;
    int point_id;
    std::string msg_text;
    int send_attempts;
    Dates::DateTime_t send_time;

    auto cur = get_pg_curs(
                "select SEND_ATTEMPTS, SEND_TIME, SETTINGS_ID "
                "from APPS_MESSAGES "
                "where MSG_ID=:msg_id");
    cur
       .def(send_attempts)
       .def(send_time)
       .defNull(settings_id, ASTRA::NoExists)
       .bind(":msg_id", msg_id)
       .EXfet();
    if(cur.err() == PgCpp::NoDataFound) {
        LogTrace(TRACE5) << __FUNCTION__ << " No data found in APPS_MESSAGES by msg_id: " << msg_id;
        return boost::none;
    }

    auto cur2 = get_pg_curs(
                "select POINT_ID, case when CICX_MSG_ID = :msg_id then CICX_MSG_TEXT "
                " else CIRQ_MSG_TEXT end "
                " from APPS_PAX_DATA "
                " where CIRQ_MSG_ID=:msg_id or CICX_MSG_ID=:msg_id");
    cur2
        .def(point_id)
        .def(msg_text)
        .bind(":msg_id", msg_id)
        .EXfet();
    if(cur2.err() == PgCpp::NoDataFound) {
        LogTrace(TRACE5) << __FUNCTION__ << " No data found in APPS_PAX_DATA by msg_id: " << msg_id;
        return boost::none;
    }

    return APPSMessage(AppsSettingsId_t(settings_id), PointId_t(point_id), msg_text, msg_id,
                       send_attempts, send_time);
}

void APPSMessage::send()
{
    // отправим телеграмму
    Opt<AppsSettings> settings = AppsSettings::readSettings(settings_id);
    if(!settings) {
        throw Exception("Cant read Apps Settings from this id: "+settings_id.get());
    }
    send_attempts++;
    //Установка текущего времени отправления сообщения
    send_time = Dates::second_clock::universal_time();
    sendTlg(settings->getRouter().c_str(), OWN_CANON_NAME(), qpOutApp, 20,
            std::string(makeHeader(settings->getAirline(), msg_id) + getMsg()),  ASTRA::NoExists, ASTRA::NoExists);
    sendCmd("CMD_APPS_HANDLER","H");
    ProgTrace(TRACE5, "New APPS request generated: %s", getMsg().c_str());
}


void APPSMessage::save() const
{
    LogTrace(TRACE5) << __FUNCTION__ << "settings_id: " << settings_id << " msg_id: " << msg_id
                     << "text: " << msg << " point_id: " << point_id;
    // сохраним информацию о сообщении
    Dates::DateTime_t nowUtc = Dates::second_clock::universal_time();
    auto cur = get_pg_curs(
               "insert into APPS_MESSAGES(MSG_ID, SEND_ATTEMPTS, SEND_TIME, SETTINGS_ID) "
               "values (:msg_id, :send_attempts, :send_time, :settings_id)");
    cur
            .bind(":msg_id", msg_id)
            .bind(":send_attempts", send_attempts)
            .bind(":send_time", nowUtc) // тоже самое DateTimeToBoost(NowUTC())
            .bind(":settings_id", settings_id.get())
            .exec();
}

void APPSMessage::incAttempts()
{
    this->send_time = Dates::second_clock::universal_time();
    this->send_attempts++;

    auto cur = get_pg_curs(
               "update APPS_MESSAGES "
               "set SEND_ATTEMPTS = :send_attempts, SEND_TIME = :send_time "
               "where MSG_ID = :msg_id ");
    cur
        .bind(":send_attempts", send_attempts)
        .bind(":send_time", send_time)
        .bind(":msg_id", msg_id)
        .exec();
}

int APPSMessage::getVersion() const
{
    Opt<AppsSettings> settings = AppsSettings::readSettings(settings_id);
    if(!settings){
        throw Exception("Cant read Apps Settings from this id: "+settings_id.get());
    }
    return settings->version();
}

void reSendMsg(int msg_id)
{
    Opt<APPSMessage> msg = APPSMessage::readAppsMsg(msg_id);
    if(!msg) {
        LogTrace(TRACE0) << __FUNCTION__ << " Not found msg by msg_id: " << msg_id;
        return;
    }
    Opt<AppsSettings> settings = AppsSettings::readSettings(msg->getSettingsId());
    if(!settings) {
        LogTrace(TRACE0) << __FUNCTION__ << " Not found apps settings by settings_id: " << msg->getSettingsId();
        return;
    }
    std::string msg_text = makeHeader(settings->getAirline(), getIdent()) + msg->getMsg();
    sendTlg(getAPPSRotName(), OWN_CANON_NAME(), qpOutApp, 20, msg_text, ASTRA::NoExists, ASTRA::NoExists);

    //  sendCmd("CMD_APPS_ANSWER_EMUL","H");
    sendCmd("CMD_APPS_HANDLER","H");
    msg->incAttempts();
    ProgTrace(TRACE5, "Message id=%d was re-sent. Send attempts: %d", msg_id, msg->getSendAttempts());
}

void processCrsPax(const PaxId_t& pax_id, const std::string& override_type)
{
    LogTrace(TRACE5) << __FUNCTION__ << " processCrsPax: " << pax_id;
    auto settings = getAppsSetsByCrsPaxId(pax_id);
    settings = filterByPreCheckin(settings);
    if(!settings.empty() && !isAlreadyCheckedIn(pax_id)) {
        sendAppsMessages(pax_id, settings, Crs, override_type);
    }
}

void processPax(const PaxId_t& pax_id, const std::string& override_type, bool is_forced)
{
    LogTrace(TRACE5) << __FUNCTION__ <<  " processPax: " << pax_id;

    auto settings = getAppsSetsByPaxId(pax_id);
    if(!getAppsSetsByPaxId(pax_id).empty()) {
        sendAppsMessages(pax_id, settings, Pax, override_type, is_forced);
    }
}

void sendAppsMessages(const PaxId_t& pax_id, const std::set<AppsSettings>& apps_settings,
                      HolderType reqType, const std::string& override_type,
                      bool is_forced)
{
    LogTrace(TRACE5) << __FUNCTION__ << " pax_id: " << pax_id;
    std::vector<APPSMessage> messages;

    Opt<PaxRequest> new_pax = PaxRequest::createFromPaxDB(reqType, pax_id, override_type);
    if(new_pax) {
        for(const auto & sets : apps_settings)
        {
            bool is_exists = false, is_the_same = false;
            bool needNew=false, needCancel=false, needUpdate=false;
            std::string status = "";

            Opt<AppsPaxDTO> paxData = AppsPaxDTO::load(pax_id, sets.getId());
            if(paxData) {
                std::string sendedMsg = paxData->requestMsg();
                is_exists = true;
                is_the_same = (sendedMsg == new_pax->msg(sets.version(), CIRQ));
                status = paxData->paxStatus();
            }

            needNew = new_pax->isNeedNew(is_exists, status, is_the_same, is_forced);
            new_pax->ifNeedDeleteAlarms(is_exists, status);
            if(is_exists) {
                needCancel = new_pax->isNeedCancel(status);
                needUpdate = new_pax->isNeedUpdate(status, is_the_same);
                new_pax->ifNeedDeleteApps(status, needNew, needCancel, needUpdate);
                new_pax->ifOutOfSyncSendAlarm(status, is_the_same);
            }
            LogTrace(TRACE5)<<__FUNCTION__
                            <<" is_exists: "    << is_exists  <<" is_the_same: "<< is_the_same
                            <<" is_the_cancel: "<< (is_exists && new_pax->getTrans().code=="CICX")
                            <<" status: "       << status    <<" needNew: "<< needNew
                            <<" needCancel: "   << needCancel <<" needUpdate: "<<needUpdate;

            PointId_t point_id = new_pax->getPointId();
            if(needCancel || needUpdate) {
                int cicx_msg_id = getIdent();
                paxData->updateCicxMsgId(cicx_msg_id);
                messages.emplace_back(sets, paxData->createCicxText(), point_id, cicx_msg_id);
            }
            if(needNew || needUpdate) {
                std::string cirq_msg = new_pax->msg(sets.version(), CIRQ);
                std::string cicx_msg = new_pax->msg(sets.version(), CICX);
                int cirq_msg_id = getIdent();
                AppsPaxDTO::save(*new_pax, cirq_msg, cicx_msg, cirq_msg_id, sets.getId());
                messages.emplace_back(sets, cirq_msg, point_id, cirq_msg_id); // посылаем только CIRQ
            }
        }
    }
    for(auto & msg : messages) {
        msg.send();
        msg.save();
    }
}

std::ostream& operator << (std::ostream& os, const TransferFlag& trfer)
{
    return os << static_cast<int>(trfer);
}

ESpecVer getSpecVer(int msg_ver)
{
    if (msg_ver == APPS_VERSION_21) {
        return SPEC_6_76;
    }
    return SPEC_6_83;
}

int fieldCount(const string &data_group, int version)
{
    // независимые от версии
    if (data_group == "CHK") return 2;
    if (data_group == "INT") return 8;
    if (data_group == "INM") return 3;
    if (data_group == "MRQ") return 3;
    if (data_group == "PCC") return 26;
    if (data_group == "EXO") return 4;
    if (data_group == "EXD") return 4;
    if (data_group == "MAK") return 4;
    // зависимые от версии
    if (version == APPS_VERSION_21)
    {
        if (data_group == "PRQ") return 22;
        if (data_group == "PCX") return 20;
        if (data_group == "PRS") return 27;
        if (data_group == "PAD") return 0;
    }
    if (version == APPS_VERSION_26)
    {
        if (data_group == "PRQ") return 34;
        if (data_group == "PCX") return 21; // NOTE в спецификации нет версии 26 для этой группы данных
        if (data_group == "PRS") return 29;
        if (data_group == "PAD") return 13;
    }
    throw Exception("unsupported field count, version: %d, data group: %s", version, data_group.c_str());
    return 0;
}

int versionByFormat(const std::string& fmt)
{
    if (fmt == APPS_FORMAT_21) {
        return APPS_VERSION_21;
    }
    if (fmt == APPS_FORMAT_26) {
        return APPS_VERSION_26;
    }
    throw Exception("cannot get version, format %s", fmt.c_str());
}

std::string getH2HReceiver()
{
    static std::string var = readStringFromTcl("APPS_H2H_ADDR", "");
    if(var.empty()) throw Exception("H2H receiever address empty");
    return var;
}

std::string getH2HTpr(int msgId)
{
    const size_t TprLen = 4;
    std::string res = StrUtils::ToBase36Lpad(msgId, TprLen);
    if(res.length() > TprLen) {
        res = res.substr(res.length() - TprLen, TprLen);
    }
    return res;
}

std::string makeHeader(const AirlineCode_t& airl, int msgId)
{
    return std::string("V.\rVHLG.WA/E5" + std::string(OWN_CANON_NAME()) + "/I5" +
                       getH2HReceiver() + "/P" + getH2HTpr(msgId) + "\rVGZ.\rV" +
                       BaseTables::Company(airl.get())->lcode() + "/MOW/////////RU\r");
}

std::string getUserId(const AirlineCode_t &airl)
{
    std::string user_id;
    BaseTables::Company comp(airl.get());
    if (!comp->lcode().empty() && !comp->lcodeIcao().empty()) {
        user_id =  comp->lcode() + comp->lcodeIcao() + "1";
    }
    else user_id = "AST1H";
    return user_id;
}

int getIdent()
{
    int vid = 0;
    auto cur = get_pg_curs("select nextval('APPS_MSG_ID__SEQ')");
    cur
       .def(vid)
       .EXfet();
    return vid;
}

const char* getAPPSRotName()
{
    static std::string var = readStringFromTcl("APPS_ROT_NAME","");
    if(var.empty()) throw Exception("APPS router name is empty");
    return var.c_str();
}

ASTRA::TGender::Enum femaleToGender(int is_female)
{
    return bool(is_female) ? ASTRA::TGender::Female : ASTRA::TGender::Male;
}

bool isTransit(const TAdvTripRoute & route)
{
    return route.size() > 2;
}

bool checkAPPSFormats(const PointId_t& point_dep, const AirportCode_t& airp_arv, std::set<std::string>* pFormats)
{
    LogTrace(TRACE5) << __FUNCTION__ << " point_id: " << point_dep << " airp_arv: " << airp_arv;
    TAdvTripRoute route = getTransitRoute(TPaxSegmentPair{point_dep.get(), airp_arv.get()});
    if(route.empty()) {
        return false;
    }
    if(!isInternationalFlight(AirportCode_t(route.front().airp), AirportCode_t(route.back().airp)))
    {
        return false;
    }
    if(route.size() < 2) {
        return false;
    }
    auto sets = appsSetsForRoute(route);
    if(sets.empty()) {
        return false;
    }
    for(const auto& set: sets) {
        pFormats->insert(set.getFormat());
    }
    return !sets.empty();
}

bool checkNeedAlarmScdIn(const PointId_t& point_id)
{
    LogTrace(TRACE5) << __FUNCTION__ << " point_id: " << point_id;
    auto route = getTransitRoute(TPaxSegmentPair{point_id.get(), ""});
    if(route.size() < 2) {
        LogTrace(TRACE5) << __FUNCTION__ << " rotue size < 2";
        return false;
    }
    //Проверка проводится только для пункта вылета прилета, то есть если
    // летим РОС-ЧЕХИЯ-РОС одним рейсом, то рейс будет не международный
    if(!isInternationalFlight(AirportCode_t(route.front().airp), AirportCode_t(route.back().airp)))
    {
        return false;
    }
    for(auto it = begin(route); it != prev(end(route)); it++) {
        //LogTrace(TRACE5) << __FUNCTION__ << " scd_in = NOExists " <<  bool(next(it)->scd_in==ASTRA::NoExists);
        if(!appsSetsForSegment(*it, *next(it)).empty() && next(it)->scd_in == ASTRA::NoExists) {
            return true;
        }
    }
    return false;
}

bool checkAPPSSegment(const TPaxSegmentPair & seg)
{
    LogTrace(TRACE5) << __FUNCTION__;
    TAdvTripRoute route = getTransitRoute(seg);
    if(route.size() < 2) {
        LogTrace(TRACE5) << __FUNCTION__ << " route size < 2";
        return false;
    }
    // Так как в seg airp может быть пустой строкой,
    // мы не можем передать в AirportCode_t пустую строку, то используем функцию
    // isInternationalFlight(airp,airp), а не (seg)
    if(!isInternationalFlight(AirportCode_t(route.front().airp), AirportCode_t(route.back().airp)))
    {
        LogTrace(TRACE5) << __FUNCTION__ << " not international flight";
        return false;
    }
    for(auto it = begin(route); it != prev(end(route)); it++) {
        if(!appsSetsForSegment(*it, *next(it)).empty()) {
            return true;
        }
    }
    return false;
}

bool checkAPPSSets(const PointId_t& point_dep)
{
    return checkAPPSSegment(TPaxSegmentPair{point_dep.get(), ""});
}

bool checkAPPSSets(const PointId_t& point_dep, const AirportCode_t& airp_arv)
{
    return checkAPPSSegment(TPaxSegmentPair{point_dep.get(), airp_arv.get()});
}

bool checkAPPSSets(const PointId_t& point_dep, const PointId_t& point_arv)
{
    auto point_info = getPointInfo(point_arv);
    if(!point_info) {
        return false;
    }
    return checkAPPSSegment(TPaxSegmentPair{point_dep.get(), point_info->airp});
}

bool checkTime(const PointId_t& point_id)
{
    LogTrace(TRACE5) << __FUNCTION__ << point_id;
    //start_time = ASTRA::NoExists;
    TDateTime now = NowUTC();
    auto point_info = getPointInfo(point_id);
    if(!point_info) {
        LogTrace(TRACE5) << __FUNCTION__ << " No point info " << point_id;
        return false;
    }
    // The APP System only allows transactions on [- 2 days] TODAY [+ 10 days].
    if ((now - point_info->scd_out) > 2 || (point_info->scd_out - now) > 10) {
        return false;
    }
    return true;
}

TDateTime getStartTime(const PointId_t& point_id)
{
    LogTrace(TRACE5) << __FUNCTION__;
    TDateTime start_time = ASTRA::NoExists;
    auto point_info = getPointInfo(point_id);
    if(point_info && (point_info->scd_out - NowUTC()) > 10) {
        start_time = point_info->scd_out - 10;
    }
    return start_time;
}

std::string issuePlaceToCountry(const std::string& issue_place)
{
    if (issue_place.empty()) {
        return issue_place;
    }
    TElemFmt elem_fmt;
    std::string country_id = issuePlaceToPaxDocCountryId(issue_place, elem_fmt);
    if (elem_fmt == efmtUnknown) {
        throw Exception("IssuePlaceToCountry failed: issue_place=\"%s\"", issue_place.c_str());
    }
    return country_id;
}



void syncAlarms(const PaxId_t& pax_id, const PointId_t& point_id_spp)
{
    if (point_id_spp.get() != ASTRA::NoExists) {
        TTripAlarmHook::set(Alarm::APPSProblem, point_id_spp.get());
    }
    else {
        if (pax_id.get() != ASTRA::NoExists) {
            TPaxAlarmHook::set(Alarm::APPSProblem, pax_id.get());
            TCrsPaxAlarmHook::set(Alarm::APPSProblem, pax_id.get());
        }
    }
}

void addAlarm(const PaxId_t& pax_id, const std::initializer_list<Alarm::Enum>& alarms,
              const PointId_t& point_id_spp)
{
    LogTrace(TRACE5) << __FUNCTION__ << " pax_id: " << pax_id;
    if (addAlarmByPaxId(pax_id.get(), alarms, {paxCheckIn, paxPnl})) {
        syncAlarms(pax_id, point_id_spp);
    }
}

void deleteAlarm(const PaxId_t &pax_id,
                 const std::initializer_list<Alarm::Enum>& alarms,
                 const PointId_t& point_id_spp)
{
    LogTrace(TRACE5) << __FUNCTION__ << " pax_id: " << pax_id;
    if (deleteAlarmByPaxId(pax_id.get(), alarms, {paxCheckIn, paxPnl})) {
        syncAlarms(pax_id, point_id_spp);
    }
}

void deleteAlarms(const PaxId_t& pax_id, const PointId_t& point_id_spp)
{
    LogTrace(TRACE5) << __FUNCTION__ << " pax_id: " << pax_id;
    if (deleteAlarmByPaxId(
                pax_id.get(),
                {Alarm::APPSNegativeDirective, Alarm::APPSError,Alarm::APPSConflict},
                {paxCheckIn, paxPnl}))
    {
        if (point_id_spp.get() != ASTRA::NoExists) {
            syncAlarms(pax_id, point_id_spp);
        }
        else {
            LogError(STDLOG) << __FUNCTION__ << ": point_id_spp==ASTRA::NoExists";
        }
    }
}

void AppsCollector::addPaxItem(const PaxId_t &pax_id, const std::string &override, bool is_forced)
{
    LogTrace(TRACE3) << __FUNCTION__ << " "
                     << pax_id << "; "
                     << override << "; "
                     << is_forced;
    m_paxItems.push_back({ pax_id, override, is_forced });
}

void AppsCollector::send()
{
    for(const PaxItem& paxItem: m_paxItems) {
        APPS::processPax(paxItem.pax_id, paxItem.override, paxItem.is_forced);
    }
}

Opt<AppsSettings> AppsSettings::readSettings(const AirlineCode_t& airline,
                                             const CountryCode_t& country)
{
    LogTrace(TRACE5) << __FUNCTION__ << " Airline: " << airline << " country: " << country;
    std::string format = "";
    int id;
    bool flt_closeout;
    bool inbound;
    bool outbound;
    bool denial;
    bool pre_checkin;

    auto cur = make_curs(
               "select FORMAT, ID, FLT_CLOSEOUT, INBOUND, OUTBOUND, PR_DENIAL, PRE_CHECKIN "
               "from APPS_SETS "
               "where AIRLINE=:airline and APPS_COUNTRY=:country and PR_DENIAL=0");
    cur
            .def(format)
            .def(id)
            .def(flt_closeout)
            .def(inbound)
            .def(outbound)
            .def(denial)
            .def(pre_checkin)
            .bind(":airline", airline)
            .bind(":country", country)
            .EXfet();
    if(cur.err() == NO_DATA_FOUND) {
        LogTrace(TRACE5) <<  __FUNCTION__ << " NO DATA FOUND ";
        return boost::none;
    }
    //Router получаем только , если настрйоки заведены
    std::string router(getAPPSRotName());
    //LogTrace(TRACE5) << __FUNCTION__ << " inbound: " << inbound << " outbound: "<< outbound;
    return AppsSettings{airline, country, format, router, AppsSettingsId_t(id),
            flt_closeout, inbound, outbound, denial, pre_checkin};
}

Opt<AppsSettings> AppsSettings::readSettings(const AppsSettingsId_t & id)
{
    LogTrace(TRACE5) << __FUNCTION__ << " Id: " << id;
    std::string airline;
    std::string country;
    std::string format = "";
    bool flt_closeout;
    bool inbound;
    bool outbound;
    bool denial;
    bool pre_checkin;

    auto cur = make_curs(
               "select FORMAT, FLT_CLOSEOUT, INBOUND, OUTBOUND, PR_DENIAL, AIRLINE, "
               "APPS_COUNTRY, PRE_CHECKIN "
               "from APPS_SETS "
               "where ID=:id and PR_DENIAL=0");
    cur
            .def(format)
            .def(flt_closeout)
            .def(inbound)
            .def(outbound)
            .def(denial)
            .def(airline)
            .def(country)
            .def(pre_checkin)
            .bind(":id", id.get())
            .EXfet();
    if(cur.err() == NO_DATA_FOUND) {
        LogTrace(TRACE5) <<  __FUNCTION__ << " NO DATA FOUND ";
        return boost::none;
    }
    //Router получаем только , если настрйоки заведены
    std::string router(getAPPSRotName());
    return AppsSettings{AirlineCode_t(airline), CountryCode_t(country), format, router, id,
                        flt_closeout, inbound, outbound, denial, pre_checkin};
}

void TransactionData::validateData() const
{
    if (code != "CIRQ" && code != "CICX" && code != "CIMR") {
        throw Exception("Incorrect transacion code");
    }
    if (user_id.empty() || user_id.size() > 6) {
        throw Exception("Incorrect User ID");
    }
}

std::string TransactionData::msg(int version, MessageType type) const
{
    validateData();
    std::ostringstream msg;

    std::string mcode;
    if(type == CIRQ) mcode = "CIRQ";
    else if(type == CICX) mcode = "CICX";
    else if(type == CIMR) mcode = "CIMR";

    /* 1 */ msg << mcode << ':';
    /* 2 */ msg << "%msg_id%" << '/';
//    if(type == CICX) {
//        /* 2 */ msg << "%msg_id%" << '/';
//    } else {
//        /* 2 */ msg << msg_id << '/';
//    }
    /* 3 */ msg << user_id << '/';
    if (code == "CIRQ" || code == "CICX")
    {
        /* 4 */ msg << "N" << '/';
        /* 5 */ msg << (pre_checkin ? "P" : "") << '/';
        /* 6 */ msg << version;
    }
    else if (code == "CIMR")
    {
        /* 4 */ msg << version;
    }
    return msg.str();
}

TransactionData createTransactionData(MessageType type, bool pre_checkin,
                                     const AirlineCode_t& airl)
{
    LogTrace(TRACE5) << __FUNCTION__;
    std::string userId = getUserId(airl);
    std::string code;
    if(type == CIRQ) code = "CIRQ";
    else if(type == CICX) code = "CICX";
    else if(type == CIMR) code = "CIMR";
    return {code, userId, pre_checkin};
}

FlightData createFlightData(const TPaxSegmentPair &flt, const string &type)
{
    LogTrace(TRACE5) << __FUNCTION__ << " point_id: " << flt.point_dep << " airp_arv: " << flt.airp_arv;
    auto route = getTransitRoute(flt);
    ASSERT(!route.empty());
    BaseTables::Port portDep(route.front().airp);
    AirportCode_t dep_port(portDep->lcode());
    Dates::DateTime_t dep_date;
    if (dep_port.get().empty()) {
        throw Exception("airp_dep.code_lat empty (code=%s)", dep_port.get().c_str());
    }
    if(type != "CHK" && route.front().scd_out != ASTRA::NoExists) {
        dep_date = DateTimeToBoost(UTCToLocal(route.front().scd_out, portDep->city()->tzRegion()));
    }
    BaseTables::Port portArv(route.back().airp);
    AirportCode_t arv_port(portArv->lcode());  // IATA Airport Code
    Dates::DateTime_t arv_date;
    if(type == "INT") {
        if (arv_port.get().empty()) {
            throw Exception("arv_port empty (code=%s)",arv_port.get().c_str());
        }
        if(route.back().scd_in != ASTRA::NoExists) {
            arv_date = DateTimeToBoost(UTCToLocal(route.back().scd_in, portArv->city()->tzRegion()));
        }
    }

    std::ostringstream trip;
    trip << BaseTables::Company(route.front().airline_out)->lcode()
         << setw(3) << setfill('0') << route.front().flt_num_out
         << route.front().suffix_out;

    string flt_num = trip.str();
    return {type, PointId_t(flt.point_dep), flt_num, dep_port, dep_date, arv_port, arv_date};
}

Opt<FlightData> createCkinFlightData(const GrpId_t& grp_id)
{
    LogTrace(TRACE5) << __FUNCTION__;
    Opt<TPaxSegmentPair> ckinSeg = CheckIn::ckinSegment(grp_id);
    if(ckinSeg){
        auto route = getTransitRoute(*ckinSeg);
        if(route.empty()) {
            return boost::none;
        }
        return createFlightData(*ckinSeg, "CHK");
    }
    return boost::none;
}

void FlightData::validateData() const
{
    if(type != "CHK" && type != "INM" && type != "INT")
        throw Exception("Incorrect type of flight: %s", type.c_str());
    if (flt_num.empty() || flt_num.size() > 8)
        throw Exception(" Incorrect flt_num: %s", flt_num.c_str());
    if (dep_port.get().empty() || dep_port.get().size() > 5)
        throw Exception("Incorrect airport: %s", dep_port.get().c_str());
    if (type != "CHK" && dep_date.is_not_a_date_time())
        throw Exception("Date empty");
    if (type == "INT") {
        if (arv_port.get().empty() || arv_port.get().size() > 5)
            throw Exception("Incorrect arrival airport: %s", arv_port.get().c_str());
        if (arv_date.is_not_a_date_time()) {
            TTripAlarmHook::setAlways(Alarm::APPSNotScdInTime, point_id.get());
            throw Exception("Arrival date empty");
        }
    }
}

std::string FlightData::msg(int version) const
{
    validateData();
    std::ostringstream msg;
    /* 1  */ msg << type << '/';
    /* 2  */ msg << fieldCount(type, version) << '/';
    if (type == "CHK")
    {
        //ToDo
        /* 3  */ msg << dep_port.get() << '/';
        /* 4  */ msg << flt_num;
    }
    else if (type == "INT")
    {
        /* 3  */ msg << "S" << '/';
        /* 4  */ msg << flt_num << '/';
        //ToDo
        /* 5  */ msg << dep_port.get() << '/';
        /* 6  */ msg << arv_port.get() << '/';
        /* 7  */ msg << HelpCpp::string_cast(dep_date, "%Y%m%d") << '/';
        /* 8  */ msg << HelpCpp::string_cast(dep_date, "%H%M%S") << '/';
        /* 9  */ msg << HelpCpp::string_cast(arv_date, "%Y%m%d") << '/';
        /* 10 */ msg << HelpCpp::string_cast(arv_date, "%H%M%S");
    }
    else if (type == "INM")
    {
        /* 3  */ msg << flt_num << '/';
        /* 4  */ msg << dep_port.get() << '/';
        /* 5  */ msg << HelpCpp::string_cast(dep_date, "%Y%m%d");
    }
    return msg.str();
}

std::string outDocType(int version, const std::string &doc_type)
{
    std::string type;
    switch (getSpecVer(version))
    {
    case SPEC_6_76:
        type = (doc_type == "P" || doc_type.empty())?"P":"O";
        break;
    case SPEC_6_83:
        if (doc_type == "P" || doc_type == "I")
            type = doc_type;
        else if (doc_type.empty())
            type = "P";
        else
            type = "O";
        break;
    default:
        throw Exception("Unknown specification version");
        break;
    }
    return type;
}

static CheckIn::TPaxDocItem loadPaxDoc(PaxId_t pax_id)
{
    CheckIn::TPaxDocItem doc;
    if (!CheckIn::LoadPaxDoc(pax_id.get(), doc))
        CheckIn::LoadCrsPaxDoc(pax_id.get(), doc);
    return doc;
}

std::string givenNames(const CheckIn::TPaxDocItem& doc, const Name_t & name)
{
    std::string given_names;
    if(!doc.surname.empty()) {
        if (!doc.first_name.empty()) {
            given_names = doc.first_name;
        }
        if (!given_names.empty() && !doc.second_name.empty()) {
            given_names = given_names + " " + doc.second_name;
        }
        given_names = given_names.substr(0, 40);
        given_names = transliter(given_names, 1, 1);
    }
    else {
        if(!name.get().empty()) {
            given_names = (transliter(name.get(), 1, 1));
        }
    }
    return given_names;
}

std::string familyName(const CheckIn::TPaxDocItem& doc, const Surname_t& surname)
{
    std::string family_name;
    if(!doc.surname.empty()) {
        family_name = transliter(doc.surname, 1, 1);
    }
    else {
        family_name = surname.get();
        family_name = transliter(family_name, 1, 1);
    }
    return family_name;
}


std::string passReference(const ASTRA::TTrickyGender::Enum tricky_gender, bool isCrew, const Opt<RegNo_t>& reg_no)
{
    std::string seq_num = std::to_string(abs(reg_no ? reg_no->get() : 0));
    if(isCrew) {
        seq_num = 'C' + seq_num;
    }

    int pass_desc;
    switch (tricky_gender)
    {
    case ASTRA::TTrickyGender::Male:    pass_desc = 1; break;
    case ASTRA::TTrickyGender::Female:  pass_desc = 2; break;
    case ASTRA::TTrickyGender::Child:   pass_desc = 3; break;
    case ASTRA::TTrickyGender::Infant:  pass_desc = 4; break;
    default:                            pass_desc = 8; break;
    }
    std::ostringstream ref;
    ref << setfill('0') << setw(4) << seq_num << pass_desc;
    return ref.str();
}

PaxData createPaxData(const PaxId_t& paxId, const Surname_t& surname, const Name_t& name,
                      bool is_crew, const std::string& override_type,
                      const Opt<RegNo_t>& reg_no, ASTRA::TTrickyGender::Enum tricky_gender,
                      TransferFlag trfer)
{
    LogTrace(TRACE5) << __FUNCTION__;
    const CheckIn::TPaxDocItem doc = loadPaxDoc(paxId);
    PaxId_t pax_id              = paxId;
    std::string apps_pax_id;    //
    std::string status;         //
    std::string pax_crew        = is_crew ? "C" : "P";
    std::string nationality     = doc.nationality;
    std::string issuing_state   = doc.issue_country;
    std::string passport        = doc.no.substr(0, 14);
    std::string check_char;     // optional
    std::string doc_type        = doc.type;
    std::string doc_subtype     = doc.subtype;
    std::string expiry_date     = (doc.expiry_date != ASTRA::NoExists) ? DateTimeToStr(doc.expiry_date, "yyyymmdd") : "";
    std::string sup_doc_type;   // в настоящее время не используется
    std::string sup_passport;   // в настоящее время не используется
    std::string sup_check_char; // в настоящее время не используется
    std::string family_name     = familyName(doc, surname);
    std::string given_names     = givenNames(doc, name);
    std::string birth_date      = (doc.birth_date != ASTRA::NoExists) ? DateTimeToStr(doc.birth_date, "yyyymmdd") : "";
    std::string sex             = (doc.gender == "M" || doc.gender == "F") ? doc.gender : "U";
    std::string birth_country;  // optional
    std::string endorsee;       // Blank for passport holder, "S" = Endorsee. Optional
    std::string trfer_at_origin = (trfer == TransferFlag::Origin || trfer == TransferFlag::Both) ? "Y" : "N";
    std::string trfer_at_dest   = (trfer == TransferFlag::Dest || trfer == TransferFlag::Both) ? "Y" : "N";
    std::string override_codes  = override_type;
    std::string pnr_source;     // conditional
    std::string pnr_locator;    // conditional
    std::string reference       = passReference(tricky_gender, is_crew, reg_no);

    return {pax_id, apps_pax_id, status, pax_crew, nationality, issuing_state, passport,check_char,
            doc_type, doc_subtype, expiry_date, sup_doc_type, sup_passport, sup_check_char,
            family_name, given_names, birth_date, sex, birth_country, endorsee, trfer_at_origin,
            trfer_at_dest, override_codes, pnr_source, pnr_locator, reference};
}


void PaxData::validateData() const
{
    if (pax_id.get() == ASTRA::NoExists)
        throw Exception("Empty pax_id");
    if (pax_crew != "C" && pax_crew != "P")
        throw Exception("Incorrect pax_crew %s", pax_crew.c_str());
    if(passport.size() > 14)
        throw Exception("Passport number too long: %s", passport.c_str());
    if(!doc_type.empty() && doc_type != "P" && doc_type != "O" && doc_type != "N" && doc_type != "I")
        //!!!!throw Exception("Incorrect doc_type: %s", doc_type.c_str());
        LogTrace(TRACE5) << __FUNCTION__ << " doc_type member = " << doc_type;
    if(family_name.empty() || family_name.size() > 40)
        throw Exception("Incorrect family_name: %s", family_name.c_str());
    if(given_names.size() > 40)
        throw Exception("given_names too long: %s", given_names.c_str());
    if(!sex.empty() && sex != "M" && sex != "F" && sex != "U" && sex != "X")
        throw Exception("Incorrect gender: %s", sex.c_str());
    if(!endorsee.empty() && endorsee != "S")
        throw Exception("Incorrect endorsee: %s", endorsee.c_str());
    if (reference.size() > 5)
        throw Exception("reference too long: %s", reference.c_str());
}

std::string PaxData::msg(int version, MessageType type) const
{
    validateData();
    std::ostringstream msg;
    std::string data_group;
    if(type == CIRQ)
    {
//    if (apps_pax_id.empty())
//    {
        /* PRQ */
        data_group = "PRQ";
        /* 1  */ msg << data_group << '/';
        /* 2  */ msg << fieldCount(data_group, version) << '/';
        /* 3  */ msg << pax_seq_num << '/';
        /* 4  */ msg << pax_crew << '/';
        /* 5  */ msg << nationality << '/';
        /* 6  */ msg << issuing_state << '/';
        /* 7  */ msg << passport << '/';
        /* 8  */ msg << check_char << '/';
        /* 9  */ msg << outDocType(version, doc_type) << '/';
        if (version >= APPS_VERSION_26)
        {
            /* 10 */ msg << doc_subtype << '/';
        }
        /* 11 */ msg << expiry_date << '/';
        /* 12 */ // reserved for version 27
        /* 13 */ msg << sup_doc_type << '/';
        /* 14 */ msg << sup_passport << '/';
        /* 15 */ msg << sup_check_char << '/';
        /* 16 */ msg << family_name << '/';
        /* 17 */ msg << (given_names.empty() ? "-" : given_names) << '/';
        /* 18 */ msg << birth_date << '/';
        /* 19 */ msg << sex << '/';
        /* 20 */ msg << birth_country << '/';
        /* 21 */ msg << endorsee << '/';
        /* 22 */ msg << trfer_at_origin << '/';
        /* 23 */ msg << trfer_at_dest << '/';
        /* 24 */ msg << override_codes << '/';
        /* 25 */ msg << pnr_source << '/';
        /* 26 */ msg << pnr_locator;
        if (version >= APPS_VERSION_26)
        {
            msg << '/';
            /* 27 */ msg << '/';
            /* 28 */ msg << '/';
            /* 29 */ msg << '/';
            /* 30 */ msg << reference;
        }
        if (version >= APPS_VERSION_26)
        {
            msg << '/';
            /* 31 */ msg << '/';
            /* 32 */ msg << '/';
            /* 33 */ msg << '/';
            /* 34 */ msg << '/';
            /* 35 */ msg << '/';
            /* 36 */ msg << '/';
            /* 37 */ msg << "";
        }
    }
    else if (type == CICX)
    {
        /* PCX */
        data_group = "PCX";
        /* 1  */ msg << data_group << '/';
        /* 2  */ msg << fieldCount(data_group, version) << '/';
        /* 3  */ msg << pax_seq_num << '/';
        /* 4  */ msg << "%apps_pax_id%" << '/';
        /* 5  */ msg << pax_crew << '/';
        /* 6  */ msg << nationality << '/';
        /* 7  */ msg << issuing_state << '/';
        /* 8  */ msg << passport << '/';
        /* 9  */ msg << check_char << '/';
        /* 10 */ msg << doc_type << '/';
        /* 11 */ msg << expiry_date << '/';
        /* 12 */ msg << sup_doc_type << '/';
        /* 13 */ msg << sup_passport << '/';
        /* 14 */ msg << sup_check_char << '/';
        /* 15 */ msg << family_name << '/';
        /* 16 */ msg << (given_names.empty() ? "-" : given_names) << '/';
        /* 17 */ msg << birth_date << '/';
        /* 18 */ msg << sex << '/';
        /* 19 */ msg << birth_country << '/';
        /* 20 */ msg << endorsee << '/';
        /* 21 */ msg << trfer_at_origin << '/';
        /* 22 */ msg << trfer_at_dest;
        if (version >= APPS_VERSION_26)
        {
            msg << '/';
            /* 23 */ msg << reference;
        }
    }
    return msg.str();
}

Opt<PaxAddData> PaxAddData::createByPaxId(const PaxId_t& pax_id)
{
    CheckIn::TPaxDocoItem doco;
    if (!CheckIn::LoadPaxDoco(pax_id.get(), doco))
    {
        return boost::none;
    }
    return createPaxAddData(pax_id, doco, paxCheckIn);
}

Opt<PaxAddData> PaxAddData::createByCrsPaxId(const PaxId_t& pax_id)
{
    LogTrace(TRACE5) << __FUNCTION__;
    CheckIn::TPaxDocoItem doco;
    if (!CheckIn::LoadCrsPaxVisa(pax_id.get(), doco))
    {
        return boost::none;
    }
    return createPaxAddData(pax_id, doco, paxPnl);
}

PaxAddData createPaxAddData(const PaxId_t& pax_id, const CheckIn::TPaxDocoItem& doco, PaxOrigin origin)
{
    LogTrace(TRACE5) << __FUNCTION__;
    CheckIn::TDocaMap docaMap;
    switch (origin) {
    case paxCheckIn:
        CheckIn::LoadPaxDoca(pax_id.get(), docaMap);
        break;
    case paxPnl:
        CheckIn::LoadCrsPaxDoca(pax_id.get(), docaMap);
        break;
    default:
        LogTrace(TRACE5) << __FUNCTION__;
        throw Exception("Unknow origin");
    }

    std::string country_data;
    if (!doco.applic_country.empty())
    {
        //ToDo
        //BaseTables::Country country_code
        std::string country_code = ((const TPaxDocCountriesRow&)base_tables.get("pax_doc_countries").
                                    get_row("code", doco.applic_country)).country;
        //ToDo
        country_data = (country_code.empty() ? "" : BaseTables::Country(country_code)->lcode());
    }
    std::string doco_type = doco.type;
    std::string doco_no = doco.no.substr(0, 20); // в БД doco.no VARCHAR2(25 BYTE)
    std::string country_issuance = issuePlaceToCountry(doco.issue_place);

    std::string doco_expiry_date;
    if (doco.expiry_date != ASTRA::NoExists) {
        doco_expiry_date = DateTimeToStr(doco.expiry_date, "yyyymmdd");
    }

    std::string num_street, city, state, postal_code, redress_number, traveller_number;
    Opt<CheckIn::TPaxDocaItem> docaOpt = docaMap.get(apiDocaD);
    if(docaOpt) {
        CheckIn::TPaxDocaItem docaD = docaOpt.get();
        num_street  = docaD.address;
        city        = docaD.city;
        state       = docaD.region.substr(0, 20); // уточнить
        postal_code = docaD.postal_code;
    }

    return {country_data, doco_type, doco_no, country_issuance, doco_expiry_date,
            num_street, city, state, postal_code, redress_number, traveller_number};
}

void PaxAddData::validateData() const
{
    if (country_for_data.size() > 2)
        throw Exception("country_for_data too long: %s", country_for_data.c_str());
    if (doco_type.size() > 2)
        throw Exception("doco_type too long: %s", doco_type.c_str());
    if (doco_no.size() > 20)
        throw Exception("doco_no too long: %s", doco_no.c_str());
    if (country_issuance.size() > 3)
        throw Exception("country_issuance too long: %s", country_issuance.c_str());
    if (doco_expiry_date.size() > 8)
        throw Exception("doco_expiry_date too long: %s", doco_expiry_date.c_str());
    if (num_street.size() > 60)
        throw Exception("num_street too long: %s", num_street.c_str());
    if (city.size() > 60)
        throw Exception("city too long: %s", city.c_str());
    if (state.size() > 20)
        throw Exception("state too long: %s", state.c_str());
    if (postal_code.size() > 20)
        throw Exception("postal_code too long: %s", postal_code.c_str());
    if (redress_number.size() > 13)
        throw Exception("redress_number too long: %s", redress_number.c_str());
    if (traveller_number.size() > 25)
        throw Exception("traveller_number too long: %s", traveller_number.c_str());
}

std::string PaxAddData::msg(int version) const
{
    validateData();
    std::ostringstream msg;
    /* 1  */ msg << "PAD" << '/';
    /* 2  */ msg << fieldCount("PAD", version) << '/';
    /* 3  */ msg << pax_seq_num << '/'; // Passenger Sequence Number
    /* 4  */ msg << country_for_data << '/';
    /* 5  */ msg << doco_type << '/';
    /* 6  */ msg << '/'; // subtype пока не используется
    /* 7  */ msg << doco_no << '/';
    /* 8  */ msg << country_issuance << '/';
    /* 9  */ msg << doco_expiry_date << '/';
    /* 10 */ // reserved for version 27
    /* 11 */ msg << num_street << '/';
    /* 12 */ msg << city << '/';
    /* 13 */ msg << state << '/';
    /* 14 */ msg << postal_code << '/';
    /* 15 */ msg << redress_number << '/';
    /* 16 */ msg << traveller_number;
    return msg.str();
}


bool PaxRequest::isNeedCancel(const std::string& status) const
{
    return status=="B" && (trans.code=="CICX");
}

bool PaxRequest::isNeedUpdate(const std::string& status, bool is_the_same) const
{
    return status=="B" && !is_the_same && (trans.code!="CICX");
}

bool PaxRequest::isNeedNew(bool is_exists, const std::string& status, bool is_the_same, bool is_forced) const
{
    return trans.code!="CICX" && (is_forced || !is_exists || (status=="P" && !is_the_same));
}

void PaxRequest::ifNeedDeleteAlarms(bool is_exists, const std::string& status) const
{
    if(trans.code=="CICX" && (!is_exists || !status.empty())) {
        deleteAlarms(pax.pax_id,  int_flt.point_id);
    }
}

void PaxRequest::ifOutOfSyncSendAlarm(const std::string& status, bool is_the_same) const
{
    if(status.empty() && !is_the_same) {
        addAlarm(pax.pax_id, {Alarm::APPSConflict}, int_flt.point_id);
    }
}

void PaxRequest::ifNeedDeleteApps(const std::string& status, bool needNew, bool needCancel, bool needUpdate) const
{
    //Подразумевается что is_exists = true
    if((trans.code=="CICX" && status!="B") || (needNew && !needCancel && !needUpdate)) {
        AppsPaxDTO::deleteAPPSData("PAX_ID", pax.pax_id.get());
    }
}

bool isAlreadyCheckedIn(const PaxId_t& pax_id)
{
    auto cur = make_curs(
               "select 1 from PAX where PAX_ID=:pax_id and REFUSE is null");
    cur
        .bind(":pax_id", pax_id.get())
        .exfet();
    return cur.err() != NO_DATA_FOUND;
}

Opt<AppsSettings> appsSetsForTripItem(const TAdvTripRouteItem& item)
{
    return appsSets(AirlineCode_t(item.airline_out), CountryCode_t(getCountryByAirp(item.airp).code));
}

Opt<AppsSettings> appsSets(const AirlineCode_t &airline, const CountryCode_t &country)
{
    if(auto set = AppsSettings::readSettings(airline, country))
    {
        if(set->getInbound() || set->getOutbound()) {
            return set;
        }
    }
    return boost::none;
}

std::set<AppsSettings> appsSetsForRoute(const TAdvTripRoute& route)
{
    LogTrace(TRACE5) << __FUNCTION__;
    std::set<AppsSettings> res;
    if(route.size() == 1) {
        auto sets = appsSetsForTripItem(route.front());
        if(sets) {
            res.insert(*sets);
        }
    } else {
        for(auto it = begin(route); it != prev(end(route)); it++) {
            auto seg_sets = appsSetsForSegment(*it, *next(it));
            res.insert(begin(seg_sets), end(seg_sets));
        }
    }
    return res;
}

std::set<AppsSettings> appsSetsForSegment(const TAdvTripRouteItem& dep, const TAdvTripRouteItem& arv)
{
    LogTrace(TRACE5) << __FUNCTION__;
    const std::string airline = dep.airline_out;
    std::set<AppsSettings> res;
    auto set_dep = AppsSettings::readSettings(AirlineCode_t(airline), CountryCode_t(getCountryByAirp(dep.airp).code));
    if(set_dep && set_dep->getOutbound()) {
        res.insert(*set_dep);
    }
    auto set_arv = AppsSettings::readSettings(AirlineCode_t(airline), CountryCode_t(getCountryByAirp(arv.airp).code));
    if(set_arv && set_arv->getInbound()) {
        res.insert(*set_arv);
    }
    return res;
}

std::set<AppsSettings> appsSetsForSegment(const TPaxSegmentPair & flt)
{
    auto info = getPointInfo(PointId_t(flt.point_dep));
    if(!info) {
        return {};
    }

    LogTrace(TRACE5) << __FUNCTION__ << " Airline: " << info->airline << " airp_dep: "
                     << info->airp << " airp_arv: " << flt.airp_arv;
    std::set<AppsSettings> res;
    if(auto set_dep = AppsSettings::readSettings(AirlineCode_t(info->airline),
                                                 CountryCode_t(getCountryByAirp(info->airp).code))) {
        if(set_dep->getOutbound()) {
            res.insert(*set_dep);
        }
    }
    if(auto set_arv = AppsSettings::readSettings(AirlineCode_t(info->airline),
                                                 CountryCode_t(getCountryByAirp(flt.airp_arv).code))) {
        if(set_arv->getInbound()) {
            res.insert(*set_arv);
        }
    }
    return res;
}

std::set<AppsSettings> filterByPreCheckin(const std::set<AppsSettings> & sets)
{
    return algo::filter(sets, [&](auto set)
    {
        if(set.getPreCheckin()) {
            return true;
        } else {
            LogTrace(TRACE3) << "Filtered by precheckin flag";
            return false;
        }
    });
}

std::set<AppsSettings> getAppsSetsByPaxId(const PaxId_t& pax_id)
{
    LogTrace(TRACE5) << __FUNCTION__;
    auto seg = CheckIn::paxSegment(pax_id);
    if(!seg) {
        return {};
    }
    return appsSetsForSegment(*seg);
}

std::set<AppsSettings> getAppsSetsByCrsPaxId(const PaxId_t& pax_id)
{
    auto seg = CheckIn::crsSegment(pax_id);
    if(!seg) {
        return {};
    }
    return appsSetsForSegment(*seg);
}

ASTRA::TTrickyGender::Enum getTrickyGender(const std::string &pers_type, ASTRA::TGender::Enum gender)
{
    return CheckIn::TSimplePaxItem::getTrickyGender(DecodePerson(pers_type.c_str()), gender);
}

bool isInternationalFlight(const AirportCode_t &airp_dep, const AirportCode_t &airp_arv)
{
    return getCountryByAirp(airp_dep.get()).code != getCountryByAirp(airp_arv.get()).code;
}

bool isInternationalFlight(const TPaxSegmentPair &seg)
{
    Opt<TTripInfo> point_info = getPointInfo(PointId_t(seg.point_dep));
    if(!point_info) {
        return false;
    }
    return isInternationalFlight(AirportCode_t(point_info->airp), AirportCode_t(seg.airp_arv));
}

class CheckInInfo
{
public:
    int point_dep;
    std::string status;
    std::string surname;
    std::string name;
    int grp_id = 0;
    std::string airp_dep;
    std::string airp_arv;
    std::string pers_type;
    int is_female;
    std::string subclass;
    int seats = 0;
};

Opt<PaxRequest> PaxRequest::createByPaxId(const PaxId_t& pax_id, const std::string& override_type)
{
    LogTrace(TRACE5) << __FUNCTION__<< " pax_id: " << pax_id;
    auto cur = make_curs(
                "select POINT_DEP, STATUS, SURNAME, NAME, PAX.GRP_ID,  AIRP_DEP, AIRP_ARV "
                ", PERS_TYPE, IS_FEMALE, REG_NO, REFUSE "
                "from PAX_GRP, PAX "
                "where PAX_ID = :pax_id and PAX_GRP.GRP_ID = PAX.GRP_ID");
    CheckInInfo info = {};
    std::string refuse;
    int reg_no = 0;
    const int NullFemale = -1;
    cur
        .def(info.point_dep)
        .def(info.status)
        .def(info.surname)
        .def(info.name)
        .def(info.grp_id)
        .def(info.airp_dep)
        .def(info.airp_arv)
        .def(info.pers_type)
        .defNull(info.is_female, NullFemale)
        .def(reg_no)
        .defNull(refuse,"")
        .bind(":pax_id", pax_id.get())
        .EXfet();
    if(cur.err() == NO_DATA_FOUND) {
        return boost::none;
    }
    Opt<TTripInfo> point_info = getPointInfo(PointId_t(info.point_dep));
    if(!point_info) {
        return boost::none;
    }
    if(!isInternationalFlight(AirportCode_t(info.airp_dep), AirportCode_t(info.airp_arv))) {
        return boost::none;
    }
    ASTRA::TGender::Enum gender = (info.is_female == NullFemale) ? ASTRA::TGender::Unknown
                                                                 : femaleToGender(info.is_female);
    return createByFields(Pax, pax_id, info, false, gender, override_type,
               AirlineCode_t(point_info->airline), !refuse.empty(), Opt<RegNo_t>(RegNo_t(reg_no)));
}

Opt<PaxRequest> PaxRequest::createByFields(const HolderType reqType, const PaxId_t &pax_id,
                                           const CheckInInfo& info, bool pre_checkin,
                                           ASTRA::TGender::Enum gender, const std::string& override_type,
                                           const AirlineCode_t& airline, bool need_del, Opt<RegNo_t> reg_no)
{
    TransferFlag trfer_flg;
    bool isCrew = false;
    auto tricky_gender = getTrickyGender(info.pers_type, gender);

    if(reqType == Pax) {
        trfer_flg = getPaxTransferFlag(PointId_t(info.point_dep), GrpId_t(info.grp_id),
                                       AirportCode_t(info.airp_dep), AirportCode_t(info.airp_arv));
        TPaxStatus pax_status = DecodePaxStatus(info.status.c_str());
        isCrew = pax_status==psCrew;
    } else {
        trfer_flg = getCrsTransferFlag(CheckInInterface::getCrsTransferMap(pax_id),
                                       PointId_t(info.point_dep), AirportCode_t(info.airp_arv));
    }
    TransactionData trs = createTransactionData(need_del ? CICX:CIRQ, pre_checkin,
                                                AirlineCode_t(airline));
    auto route = getTransitRoute(TPaxSegmentPair{info.point_dep, info.airp_arv});
    if(route.empty()) {
        return boost::none;
    }
    FlightData intFlight = createFlightData(TPaxSegmentPair{info.point_dep, info.airp_arv}, "INT");
    Opt<FlightData> ckinFlight = createCkinFlightData(GrpId_t(info.grp_id));
    PaxData pax = createPaxData(pax_id, Surname_t(info.surname), Name_t(info.name), isCrew, override_type,
             reg_no, tricky_gender, trfer_flg);
    return PaxRequest(trs, intFlight, ckinFlight, pax,
                      reqType==Pax ? PaxAddData::createByPaxId(pax_id) : PaxAddData::createByCrsPaxId(pax_id));
}

Opt<PaxRequest> PaxRequest::createByCrsPaxId(const PaxId_t &pax_id, const std::string& override_type)
{
    LogTrace(TRACE5) << __FUNCTION__<< " pax_id: " << pax_id;
    bool pr_del;
    CheckInInfo info;
    auto cur = make_curs(
               "select SURNAME, NAME, POINT_ID_SPP,  AIRP_ARV "
               ", PERS_TYPE, PR_DEL "
               "from CRS_PAX, CRS_PNR, TLG_BINDING "
               "where PAX_ID = :pax_id and CRS_PAX.PNR_ID = CRS_PNR.PNR_ID and "
               "      CRS_PNR.POINT_ID = TLG_BINDING.POINT_ID_TLG");
    cur
        .def(info.surname)
        .defNull(info.name,"")
        .def(info.point_dep)
        .def(info.airp_arv)
        .def(info.pers_type)
        .def(pr_del)
        .bind(":pax_id", pax_id.get())
        .EXfet();
    if(cur.err() == NO_DATA_FOUND) {
        return boost::none;
    }
    auto point_info = getPointInfo(PointId_t(info.point_dep));
    if(!point_info) {
        throw Exception("Not point info");
    }
    if(!isInternationalFlight(TPaxSegmentPair{info.point_dep, info.airp_arv}))
    {
        return boost::none;
    }
    return createByFields(Crs, pax_id, info, true, ASTRA::TGender::Unknown,
                          override_type, AirlineCode_t(point_info->airline), pr_del, boost::none);
}

Opt<PaxRequest> PaxRequest::createFromPaxDB(HolderType type, const PaxId_t& pax_id,
                                         const std::string& override_type)
{
    LogTrace(TRACE5) << __FUNCTION__;
    switch (type) {
    case HolderType::Pax:
        return PaxRequest::createByPaxId(pax_id, override_type);
    case HolderType::Crs:
        return PaxRequest::createByCrsPaxId(pax_id, override_type);
    default:
        throw Exception("Invalid RequestType: %d ", static_cast<int>(type));
    }
}

TransferFlag getTransferFlagFromTransit(const PointId_t& point_id, const AirportCode_t& airp_arv)
{
    /* Проверим транзит. В случае транзита через страну-участницу APPS, выставим
     флаг "transfer at destination". Это противоречит тому, что написано в
     спецификации, однако при проведении сертификации SITA потребовала именно
     так обрабатывать транзитные рейсы. */

    LogTrace(TRACE5) << __FUNCTION__ << " point_id: " << point_id << " airp_arv: " << airp_arv;
    TAdvTripRoute route = getTransitRoute(TPaxSegmentPair(point_id.get(), airp_arv.get()));
    bool transit = isTransit(route);
    return transit ? TransferFlag::Dest : TransferFlag::None;
}

TransferFlag getCrsTransferFlag(const map<int, CheckIn::TTransferItem> & trfer, const PointId_t& point_id,
                                const AirportCode_t &airp_arv)
{
    LogTrace(TRACE5) << __FUNCTION__;
    TransferFlag transfer = getTransferFlagFromTransit(point_id, airp_arv);
    if (!trfer.empty() && !trfer.at(1).airp_arv.empty()) {
        // сквозная регистрация
        transfer = TransferFlag::Dest; // исходящий трансфер
    }
    return transfer;
}

TransferFlag getPaxTransferFlag(const PointId_t& point_id, const GrpId_t& pax_grp_id,
                                const AirportCode_t &airp_dep, const AirportCode_t &airp_arv)
{
    LogTrace(TRACE5) << __FUNCTION__;
    TransferFlag transfer = getTransferFlagFromTransit(point_id, airp_arv);
    if(isPointTransferDestination(pax_grp_id, airp_arv)) {
        transfer = TransferFlag::Dest;
    }
    if(isPointTransferOrigin(pax_grp_id, airp_dep)) {
        transfer = ((transfer == TransferFlag::Dest) ? TransferFlag::Both : TransferFlag::Origin);
    }
    return transfer;
}

bool isPointTransferDestination(const GrpId_t& pax_grp_id, const AirportCode_t &airp_arv)
{
    auto next=TCkinRoute::getNextGrp(pax_grp_id,
                                     TCkinRoute::IgnoreDependence,
                                     TCkinRoute::WithoutTransit);
    if(!next) {
        LogTrace(TRACE5) << "next segment not found ";
        return false;
    }

    std::string country_arv = getCountryByAirp(airp_arv.get()).code;
    if (getCountryByAirp(next.get().airp_arv).code != country_arv) {
        return true;
    }

    return false;
}

bool isPointTransferOrigin(const GrpId_t& pax_grp_id, const AirportCode_t &airp_dep)
{
    auto prev=TCkinRoute::getPriorGrp(pax_grp_id,
                                      TCkinRoute::IgnoreDependence,
                                      TCkinRoute::WithoutTransit);
    if(!prev) {
        LogTrace(TRACE5) << "prev segment not found ";
        return false;
    }

    std::string country_dep = getCountryByAirp(airp_dep.get()).code;
    if (getCountryByAirp(prev.get().airp_dep).code != country_dep) {
        return true;
    }

    return false;
}

std::vector<std::string> statusesFromDb(const PaxId_t& pax_id)
{
    std::vector<std::string> res;
    auto cur = get_pg_curs(
               "select STATUS from APPS_PAX_DATA "
               "where PAX_ID = :pax_id order by SEND_TIME desc");
    std::string status;
    cur
        .defNull(status,"")
        .bind(":pax_id",pax_id.get())
        .exec();

    while(!cur.fen()) {
        res.emplace_back(status);
    }
    return res;
}

std::string PaxRequest::msg(int version, MessageType type) const
{
    std::ostringstream msg;
    msg << trans.msg(version, type) << "/";
    if (type == CIRQ && ckin_flt) {
        msg << ckin_flt->msg(version) << "/";
    }
    msg << int_flt.msg(version) << "/";
    msg << pax.msg(version, type) << "/";
    if(version >= APPS_VERSION_26 && type == CIRQ && pax_add) {
        msg << pax_add->msg(version) << "/";
    }
    return std::string("\x02" + msg.str() + "\x03");
}

void ManifestData::validateData() const
{
    if (country.empty()) {
        throw Exception("Empty country");
    }
    if (mft_pax != "C") {
        throw Exception("Incorrect mft_pax");
    }
    if (mft_crew != "C") {
        throw Exception("Incorrect mft_crew");
    }
}

std::string ManifestData::msg(int version) const
{
    validateData();
    std::ostringstream msg;
    /* 1 */ msg << "MRQ" << '/';
    /* 2 */ msg << fieldCount("MRQ", version) << '/';
    /* 3 */ msg << country << '/';
    /* 4 */ msg << mft_pax << '/';
    /* 5 */ msg << mft_crew;
    return msg.str();
}


std::string ManifestRequest::msg(int version, MessageType type) const
{
    std::string msg  = trans.msg(version, type)   + "/"
                     + int_flt.msg(version) + "/"
                     + mft_req.msg(version) + "/";
    return std::string("\x02" + msg + "\x03");
}

static int getInt(const std::string& val)
{
    if (val.empty()) {
        return ASTRA::NoExists;
    }
    return ToInt(val);
}

void AnsPaxData::init(const std::string& source, int ver)
{
    LogTrace(TRACE5) << __FUNCTION__;
    version = ver;
    /* PRS PCC */
    std::vector<std::string> tmp;
    boost::split(tmp, source, boost::is_any_of("/"));

    std::string grp_id = tmp[0]; /* 1 */
    if (grp_id != "PRS" && grp_id != "PCC") {
        throw Exception("Incorrect grp_id: %s", grp_id.c_str());
    }

    int field_count = getInt(tmp[1]); /* 2 */
    if ((grp_id == "PRS" && field_count != fieldCount("PRS", version)) ||
         (grp_id == "PCC" && field_count != fieldCount("PCC", version)))
    {
        throw Exception("Incorrect field_count: %d", field_count);
    }

    int seq_num = getInt(tmp[2]); /* 3 */
    if(seq_num != pax_seq_num) { // Passenger Sequence Number
        throw Exception("Incorrect seq_num: %d", seq_num);
    }

    country = tmp[3]; /* 4 */

    if (grp_id == "PRS")
    {
        /* PRS */
        int i = 22; // начальная позиция итератора (поле 23)
        if (version < APPS_VERSION_27) { --i;}
        if (version < APPS_VERSION_26) { --i;}
        code = getInt(tmp[i++]);         /* 23 */
        status = *(tmp[i++].begin());    /* 24 */
        apps_pax_id = tmp[i++];          /* 25 */
        error_code1 = getInt(tmp[i++]);  /* 26 */
        error_text1 = tmp[i++];          /* 27 */
        error_code2 = getInt(tmp[i++]);  /* 28 */
        error_text2 = tmp[i++];          /* 29 */
        error_code3 = getInt(tmp[i++]);  /* 30 */
        error_text3 = tmp[i];            /* 31 */
    }
    else
    {
        /* PCC */
        int i = 20; // начальная позиция итератора (поле 21)
        if (tmp.size() == 29) ++i; // workaround for incorrect data (excess field)
        code = getInt(tmp[i++]);         /* 21 */
        status = *(tmp[i++].begin());    /* 22 */
        error_code1 = getInt(tmp[i++]);  /* 23 */
        error_text1 = tmp[i++];          /* 24 */
        error_code2 = getInt(tmp[i++]);  /* 25 */
        error_text2 = tmp[i++];          /* 26 */
        error_code3 = getInt(tmp[i++]);  /* 27 */
        error_text3 = tmp[i];            /* 28 */
    }
}

std::string AnsPaxData::toString() const
{
    std::ostringstream res;
    res << "country: " << country << std::endl << "code: " << code
        << std::endl << "status: " << status << std::endl;
    if(!apps_pax_id.empty()) res << "apps_pax_id: " << apps_pax_id << std::endl;
    if(error_code1 != ASTRA::NoExists) res << "error_code1: " << error_code1 << std::endl;
    if(!error_text1.empty()) res << "error_text1: " << error_text1 << std::endl;
    if(error_code2 != ASTRA::NoExists) res << "error_code2: " << error_code2 << std::endl;
    if(!error_text2.empty()) res << "error_text2: " << error_text2 << std::endl;
    if(error_code3 != ASTRA::NoExists) res << "error_code3: " << error_code3 << std::endl;
    if(!error_text3.empty()) res << "error_text3: " << error_text3 << std::endl;
    return res.str();
}

LEvntPrms APPSAnswer::getLogParams(const std::string & country, int status_code,
                                   int error_code, const std::string& error_text) const
{
    LEvntPrms params;
    params << PrmLexema("req_type", "MSG.APPS_" + code);

    if (!country.empty()) {
        TElemFmt fmt;
        std::string apps_country = ElemToElemId(etCountry,country,fmt);
        if (fmt == efmtUnknown) {
            throw EConvertError("Unknown format");
        }
        PrmLexema lexema("country", "MSG.APPS_COUNTRY");
        lexema.prms << PrmElem<std::string>("country", etCountry, apps_country);
        params << lexema;
    } else {
        params << PrmSmpl<std::string>("country", "");
    }

    if (status_code != ASTRA::NoExists) {
        // нужно как-то различать Timeout и Error condition. Для обоих status_code 0000
        if (status_code == 0) {
            params << PrmLexema("result", (error_code != ASTRA::NoExists)?
                                     "MSG.APPS_STATUS_1111":"MSG.APPS_STATUS_2222");
        }
        else {
            params << PrmLexema("result", "MSG.APPS_STATUS_" + IntToString(status_code));
        }
    } else {
        params << PrmSmpl<string>("result", "");
    }
    if (error_code != ASTRA::NoExists) {
        std::string id = std::string("MSG.APPS_") + IntToString(error_code);
        if (AstraLocale::getLocaleText(id, AstraLocale::LANG_RU) != id &&
             AstraLocale::getLocaleText(id, AstraLocale::LANG_EN) != id) {
            params << PrmLexema("error", id);
        }
        else {
            params << PrmSmpl<string>("error", error_text);
        }
    }
    else {
        params << PrmSmpl<string>("error", "");
    }
    return params;
}

void APPSAnswer::readAnswerErrors(const std::string& source)
{
    vector<std::string> tmp;
    boost::split(tmp, source, boost::is_any_of("/"));
    auto it = next(tmp.begin());
    if(*(it++) == "ERR")
    {
        LogTrace(TRACE5)<<__FUNCTION__<< " Answer INIT ERR";
        size_t fld_count = getInt(*(it++));
        if (fld_count != (tmp.size() - 3)) {
            throw Exception("Incorrect fld_count: %d", fld_count);
        }
        while(it < tmp.end()) {
            Error error;
            error.country = *(it++);
            error.error_code = getInt(*(it++));
            error.error_text = *(it++);
            errors.push_back(error);
        }
    }
}

int readAnswerMsgId(const std::string & source)
{
    vector<std::string> tmp;
    boost::split(tmp, source, boost::is_any_of("/"));
    return getInt(tmp.front());
}

std::string APPSAnswer::toString() const
{
    std::ostringstream res;
    res << "code: " << code << std::endl << "msg_id: " << msg.getMsgId() << std::endl
        << "point_id: " << msg.getPointId() << std::endl << "msg_text: " << msg.getMsg() << std::endl
        << "send_attempts: " << msg.getSendAttempts() << std::endl;
    for (const auto & err: errors) {
        res << "country: " << err.country << std::endl << "error_code: " << err.error_code
            << std::endl << "error_text: " << err.error_text << std::endl;
    }
    return res.str();
}

void APPSAnswer::beforeProcessAnswer() const
{
    TFlights flightsForLock;
    flightsForLock.Get(msg.getPointId().get(), ftTranzit);
    flightsForLock.Lock(__FUNCTION__);

    // выключим тревогу "Нет связи с APPS"
    set_alarm(msg.getPointId().get(), Alarm::APPSOutage, false);
}

bool APPSAnswer::checkIfNeedResend() const
{
    bool need_resend = false;
    for(const auto & err: errors) {
        if (err.error_code == 6102 || err.error_code == 6900 || err.error_code == 6979
                || err.error_code == 5057) {
            // ... Try again later
            need_resend = true;
        }
    }
    // Если нужно отправить повторно, не удаляем сообщение из apps_messages, чтобы resend_tlg его нашел
    if (!need_resend) {
        deleteMsg(msg.getMsgId());
    }
    return need_resend;
}

std::unique_ptr<APPSAnswer> createAnswer(const std::string& code, const std::string& answer)
{
    LogTrace(TRACE5) << __FUNCTION__;
    if (code == "CIRS" || code == "CICC") {
        return createPaxReqAnswer(code, answer);
    }
    else if (code == "CIMA") {
        return createManifestAnswer(code, answer);
    }
    else {
        throw Exception(std::string("Unknown transaction code: " + code));
    }
    return nullptr;
}

bool processReply(const std::string& source_raw)
{
    try
    {
        std::string source = getAnsText(source_raw);
        if(source.empty()) {
            throw Exception("Answer is empty");
        }
        std::string code = source.substr(0, 4);
        std::string answer = source.substr(5, source.size() - 6); // отрезаем код транзакции и замыкающий '/' (XXXX:text_to_parse/)

        auto res = createAnswer(code, answer);
        if(res) {
            ProgTrace(TRACE5, "Result: %s", res->toString().c_str());
            res->beforeProcessAnswer();
            res->processErrors();
            res->processAnswer();
            return true;
        }
        return false;
    }
    catch(EOracleError &E)
    {
        ProgError(STDLOG,"EOracleError %d: %s", E.Code,E.what());
    }
    catch(std::exception &E)
    {
        ProgError(STDLOG,"std::exception: %s", E.what());
    }
    catch(...)
    {
        ProgError(STDLOG, "Unknown exception");
    }
    return false;
}

std::unique_ptr<PaxReqAnswer> createPaxReqAnswer(const std::string& code, const std::string& source)
{
    LogTrace(TRACE5) << __FUNCTION__;
    Opt<APPSMessage> msg = APPSMessage::readAppsMsg(readAnswerMsgId(source));
    if(!msg){
        return nullptr;
    }
    // PassengerData. Applicable to "CIRS" and "CIСС". Conditional (required if ERR data group is not present)
    std::vector<AnsPaxData> passengers;

    std::ostringstream sql;
    sql << "select PAX_ID, FAMILY_NAME "
           "from APPS_PAX_DATA ";
    if (code == "CIRS") {
        sql << "where CIRQ_MSG_ID = :msg_id";
    } else {
        sql << "where CICX_MSG_ID = :msg_id";
    }
    int pax_id = 0;
    std::string family_name = "";
    auto cur = get_pg_curs(sql.str());
    cur
       .def(pax_id)
       .def(family_name)
       .bind(":msg_id", msg->getMsgId())
       .EXfet();
    if(cur.err() == PgCpp::NoDataFound) {
        LogTrace(TRACE5) << " NO DATA FOUND by msg_id: " << msg->getMsgId() << " code: " << code;
        return nullptr;
    }
    //LogTrace(TRACE5) << __FUNCTION__ << " version: " << msg->getVersion() << " msg_id: " << msg->getMsgId();
    std::string delim = (code == "CIRS") ? "PRS" : "PCC";
    delim = std::string("/") + delim + std::string("/");
    std::size_t pos1 = source.find(delim);
    while (pos1 != std::string::npos) {
        std::size_t pos2 = source.find(delim, pos1 + delim.size());
        AnsPaxData data;
        data.init(source.substr(pos1 + 1, pos2 - pos1),  msg->getVersion());
        passengers.push_back(data);
        pos1 = pos2;
    }
    return make_unique<PaxReqAnswer>(code, source, move(*msg), PaxId_t(pax_id), family_name, passengers);
}

std::unique_ptr<ManifestAnswer> createManifestAnswer(const std::string& code, const std::string& source)
{
    LogTrace(TRACE5) << __FUNCTION__;
    Opt<APPSMessage> msg = APPSMessage::readAppsMsg(readAnswerMsgId(source));
    if(!msg){
        return nullptr;
    }

    size_t pos = source.find("MAK");
    if (pos != string::npos)
    {
        vector<std::string> tmp;
        std::string text = source.substr(pos);
        boost::split(tmp, text, boost::is_any_of("/"));
        int fld_count = getInt(tmp[1]); /* 2 */
        if (fld_count != fieldCount("MAK", msg->getVersion())) {
            throw Exception("Incorrect fld_count: %d", fld_count);
        }
        std::string country = tmp[2]; /* 3 */
        int resp_code  = getInt(tmp[3]); /* 4 */
        int error_code = getInt(tmp[4]); /* 5 */
        std::string error_text = tmp[5]; /* 6 */
        return make_unique<ManifestAnswer>(code, source, move(*msg), country, resp_code, error_code, error_text);
    }
    return nullptr;
}

std::string PaxReqAnswer::toString() const
{
    std::ostringstream res;
    res << APPSAnswer::toString()
        << "pax_id: " << pax_id.get() << std::endl << "family_name: " << family_name << std::endl;
    for (const auto & pax : passengers) {
        res << pax.toString();
    }
    return res.str();
}

void PaxReqAnswer::processErrors() const
{
    LogTrace(TRACE5) << __FUNCTION__ << " code: " << code;
    if(errors.empty()) {
        //LogTrace(TRACE5) << __FUNCTION__ << " errors empty";
        return;
    }

    // Сообщение не обработано системой. Разберем ошибки.
    for(const auto & err : errors) {
        logAnswer(err.country, ASTRA::NoExists, err.error_code, err.error_text);
    }

    if(/*!CheckIfNeedResend() &&*/ code == "CIRS") {
        LogTrace(TRACE5) << __FUNCTION__ << " if code = CIRS";
        addAlarm(pax_id, {Alarm::APPSConflict}); // рассинхронизация
        // удаляем apps_pax_data cirq_msg_id
        AppsPaxDTO::deleteAPPSData("CIRQ_MSG_ID", msg.getMsgId());
        deleteMsg(msg.getMsgId());
    }
}



void PaxReqAnswer::processAnswer() const
{
    LogTrace(TRACE5) << __FUNCTION__;
    if (passengers.empty()) {
        return;
    }

    // Сообщение обработано системой. Каждому пассажиру присвоен статус.
    bool result = true;
    std::string status = "B";
    std::string apps_pax_id;
    for (const auto& pax : passengers)
    {
        // запишем в журнал операций присланный статус
        logAnswer(pax.country, pax.code, pax.error_code1, pax.error_text1);
        if (pax.error_code2 != ASTRA::NoExists) {
            logAnswer(pax.country, pax.code, pax.error_code2, pax.error_text2);
        }
        if (pax.error_code3 != ASTRA::NoExists) {
            logAnswer(pax.country, pax.code, pax.error_code3, pax.error_text3);
        }
        if (code == "CICC") {
            // Ответ на отмену
            // Определим статус пассажира. true - отменен или не найден, false - ошибка.
            if(pax.status == "E" || pax.status == "T") {
                result = false;
                LogTrace(TRACE5) << " result = " << result << " status = " << pax.status;
            }
            continue;
        }

        // Ответ на запрос статуса
        if (&pax != &passengers[0]) {
            LogTrace(TRACE5) << __FUNCTION__ << " ответ на запрос статуса";
            if (apps_pax_id != pax.apps_pax_id)
                throw Exception("apps_pax_id is not the same");
        } else {
            apps_pax_id = pax.apps_pax_id;
        }
        // включим тревоги
        if (pax.status == "D" || pax.status == "X") {
            addAlarm(pax_id, {Alarm::APPSNegativeDirective});
            LogTrace(TRACE5) << __FUNCTION__ << " включим тревоги статус=D||X";
        }
        else if (pax.status == "U" || pax.status == "I" || pax.status == "T" || pax.status == "E") {
            addAlarm(pax_id, {Alarm::APPSError});
            LogTrace(TRACE5) << __FUNCTION__ << " включим тревоги статус=U||I||T||E";
        }
        /* Каждая страна участник APPS присылает свой статус пассажира, т.е.
     * один пассажир может иметь несколько статусов. Получим по каждому пассажиру
     * обобщенный статус для записи в apps_pax_data. Если все "B" -> "B", если хотя бы
     * один "X" -> "X", если хотябы один "U","I","T" или "E" -> "P" (Problem) */
        if (status == "B" && pax.status != "B") {
            status = (pax.status == "X") ? pax.status : "P";
        }
        else if (status == "P" && pax.status == "X") {
            status = "X";
        }
    }
    LogTrace(TRACE5) << " IF code = CICC Delete apps data" << " code: " << code;
    if (code == "CICC") {
        if(result) {
            // Пассажир отменен. Удалим его из apps_pax_data
            AppsPaxDTO::deleteAPPSData("CICX_MSG_ID", msg.getMsgId());
        }
        deleteMsg(msg.getMsgId());
        return;
    }
    // сохраним полученный статус
    auto paxData = AppsPaxDTO::load(msg.getMsgId());
    paxData->updateStatus(status, apps_pax_id, msg.getMsgId());

    // погасим тревоги
    if (status == "B") {
        LogTrace(TRACE5) << __FUNCTION__ << " погасим тревоги";
        deleteAlarm(pax_id, {Alarm::APPSNegativeDirective, Alarm::APPSError});
    }
    // проверим, нужно ли гасить тревогу "рассинхронизация"
    Opt<PaxRequest> actual;
    actual = isAlreadyCheckedIn(pax_id) ? PaxRequest::createFromPaxDB(Pax, pax_id)
                                        : PaxRequest::createFromPaxDB(Crs, pax_id);
    if(!actual || actual->msg(msg.getVersion(), CIRQ) == msg.getMsg()) {
        deleteAlarm(pax_id, {Alarm::APPSConflict});
    }
    deleteMsg(msg.getMsgId());
}



std::set<std::string> needFltCloseout(const std::set<std::string>& countries, const AirlineCode_t& airline)
{
    LogTrace(TRACE5) << __FUNCTION__;
    std::set<std::string> countries_need_req;
    if(countries.size() > 1)
    {
        for (const auto &country : countries)
        {
            Opt<AppsSettings> sets = AppsSettings::readSettings(airline, CountryCode_t(country));
            if(sets && sets->getFltCloseout()) {
                countries_need_req.insert(country);
            }
        }
    }
    return countries_need_req;
}

std::vector<PaxId_t> paxesToCancel(const PointId_t& point_dep, const PointId_t& point_arv)
{
    std::vector<PaxId_t> pax_ids;
    /* Определим, есть ли пассажиры не прошедшие посадку,
 * информация о которых была отправлена в SITA.
 * Для таких пассажиров нужно послать отмену */

    int pax_id;
    auto cur = make_curs("select PAX_ID from PAX_GRP, PAX "
                         "where PAX_GRP.GRP_ID = PAX.GRP_ID and "
                         "PAX_GRP.POINT_DEP = :point_dep    and "
                         "PAX_GRP.POINT_ARV = :point_arv    and "
                         "(PAX.NAME is null or PAX.NAME<>'CBBG') and "
                         "PR_BRD=0 and PAX_GRP.STATUS != 'E' " );
    cur
        .def(pax_id)
        .bind(":point_dep", point_dep)
        .bind(":point_arv", point_arv)
        .exec();

    //Возвращаем вектор pax_id по которым надо слать отмену
    while(!cur.fen()) {
        pax_ids.push_back(PaxId_t(pax_id));
    }
    return pax_ids;
}

int msgIdToCancel(const PaxId_t &pax_id, const std::string & status)
{
    int cirq_msg_id;
    auto cur = get_pg_curs("select CIRQ_MSG_ID from APPS_PAX_DATA "
                         "where PAX_ID = :pax_id and STATUS = :status and CICX_MSG_ID is NULL");
    cur
        .def(cirq_msg_id)
        .bind(":pax_id", pax_id)
        .bind(":status", status)
        .exfet();
    return cirq_msg_id;
}

void appsFlightCloseout(const PointId_t& point_id)
{
    ProgTrace(TRACE5, "appsFlightCloseout: point_id = %d", point_id.get());
    if (!checkTime(point_id)) {
        return;
    }

    TAdvTripRoute route;
    route.GetRouteAfter(NoExists, point_id.get(), trtWithCurrent, trtNotCancelled);
    if (route.empty()) {
        return;
    }
    TAdvTripRoute::const_iterator r = route.begin();
    std::set<std::string> countries;
    countries.insert(getCountryByAirp(r->airp).code);
    for(r++; r!=route.end(); r++)
    {
        // определим, нужно ли отправлять данные
        if(!checkAPPSSets(point_id, PointId_t(r->point_id))) {
            continue;
        }
        string country = getCountryByAirp(r->airp).code;
        countries.insert(country);
        Opt<AppsSettings> settings = AppsSettings::readSettings(
                    AirlineCode_t(route.front().airline_out), CountryCode_t(country));
        if(!settings) {
            continue;
        }

        for(const PaxId_t& pax_id : paxesToCancel(point_id, PointId_t(r->point_id)) ) {
            Opt<AppsPaxDTO> paxData = AppsPaxDTO::load(msgIdToCancel(pax_id, "B"));
            if(paxData) {
                int cicx_msg_id = getIdent();
                paxData->updateCicxMsgId(cicx_msg_id);
                APPSMessage msg(*settings, paxData->createCicxText(), point_id, cicx_msg_id);
                msg.send();
                msg.save();
            }
        }
    }

    for(const auto& country : needFltCloseout(countries, AirlineCode_t(route.front().airline_out)))
    {
        //ToDo
        std::string country_lat = BaseTables::Country(country)->lcode();
        Opt<AppsSettings> settings = AppsSettings::readSettings(
                    AirlineCode_t(route.front().airline_out), CountryCode_t(country));
        Opt<ManifestRequest> close_flt = createManifestReq(
                    TPaxSegmentPair{point_id.get(), route.back().airp}, country_lat);
        if (close_flt && settings) {
            APPSMessage msg(*settings, close_flt->msg(settings->version(), CIMR), point_id, getIdent());//  make_unique<ManifestRequest>(*close_flt));
            msg.send();
            msg.save();
        }
    }
}

Opt<ManifestRequest> createManifestReq(const TPaxSegmentPair &flt, const std::string& country_lat)
{
    Opt<TTripInfo> point_info = getPointInfo(PointId_t(flt.point_dep));
    if(!point_info) {
        return boost::none;
    }
    TransactionData trs = createTransactionData(CIMR, false, AirlineCode_t(point_info->airline));
    auto route = getTransitRoute(flt);
    if(route.empty()) {
        return boost::none;
    }
    FlightData flight = createFlightData(flt, "INM");
    ManifestData mft(country_lat);
    return ManifestRequest(trs, flight, mft);
}

bool isAPPSAnswText(const std::string& tlg_body)
{
    return ((tlg_body.find(std::string("CIRS:")) != std::string::npos) ||
            (tlg_body.find(std::string("CICC:")) != std::string::npos) ||
            (tlg_body.find(std::string("CIMA:")) != std::string::npos));
}

std::string emulateAnswer(const std::string& request)
{
    return "";
}



void deleteMsg(int msg_id)
{
    LogTrace(TRACE5) << __FUNCTION__ << " msg_id: " << msg_id;
    auto cur = get_pg_curs(
               "delete from APPS_MESSAGES where MSG_ID = :msg_id ");
    cur
        .bind(":msg_id", msg_id)
        .exec();
}

bool isNeedAddTask(const PointId_t& point_id, const std::string &task_name)
{
    if(task_name != SEND_NEW_APPS_INFO && task_name != SEND_ALL_APPS_INFO) {
        throw Exception("Unknown task_name");
        return false;
    }
    if(task_name == SEND_ALL_APPS_INFO) {
        if(!checkAPPSSets(point_id)) {
            LogTrace(TRACE5) << __FUNCTION__ << " Not apps settings ";
            return false;
        }
    }
    return APPS::checkTime(point_id) || getStartTime(point_id) != ASTRA::NoExists;
}

void sendAppsTask(const PointId_t& point_id, const std::string &task_name)
{
    add_trip_task(point_id.get(), task_name, "", getStartTime(point_id));
}

void ifNeedAddTaskSendApps(const PointId_t& point_id, const std::string &task_name)
{
    if(isNeedAddTask(point_id, task_name)) {
        sendAppsTask(point_id, task_name);
    }
}

void sendInfo(const PointId_t& point_id, const PointIdTlg_t& point_id_tlg)
{
    ProgTrace(TRACE5, "sendAPPSInfo: point_id %d, point_id_tlg: %d", point_id.get(), point_id_tlg.get());
    auto cur = make_curs(
               "select PAX_ID, AIRP_ARV from CRS_PNR, CRS_PAX "
               "where CRS_PNR.PNR_ID=CRS_PAX.PNR_ID and POINT_ID=:point_id_tlg");
    std::string airp_arv;
    int pax_id;
    cur.def(pax_id).def(airp_arv).bind(":point_id_tlg", point_id_tlg.get()).exec();

    TFlights flightsForLock;
    flightsForLock.Get(point_id.get(), ftTranzit);
    flightsForLock.Lock(__FUNCTION__);

    while(!cur.fen()) {
         if (checkAPPSSets(point_id, AirportCode_t(airp_arv))) {
             processCrsPax(PaxId_t(pax_id));
         }
    }
}

void sendAllInfo(const TTripTaskKey &task)
{
    LogTrace(TRACE5) << __FUNCTION__;
    auto cur = make_curs(
               "select POINT_ID_TLG "
               "from TLG_BINDING, POINTS "
               "where TLG_BINDING.POINT_ID_SPP=POINTS.POINT_ID and "
               "   POINT_ID_SPP=:point_id_spp and "
               "   POINTS.PR_DEL=0 AND POINTS.PR_REG<>0");
    int point_id_tlg;
    cur.def(point_id_tlg).bind(":point_id_spp", task.point_id).exec();
    while(!cur.fen()) {
        sendInfo(PointId_t(task.point_id), PointIdTlg_t(point_id_tlg));
    }
}

void sendNewInfo(const TTripTaskKey &task)
{
    LogTrace(TRACE5) << __FUNCTION__;
    auto paxIds = getAlarmByPointId(PointId_t(task.point_id), Alarm::SyncAPPS, PaxOrigin::paxPnl);
    TFlights flightsForLock;
    flightsForLock.Get(task.point_id, ftTranzit);
    flightsForLock.Lock(__FUNCTION__);
    for(const auto& pax_id : paxIds) {
        processCrsPax(pax_id);
        deleteAlarmByPaxId(pax_id.get(), {Alarm::SyncAPPS}, {paxPnl});
    }
}

int testAppsTlg(int argc, char **argv)
{
    for (std::string text : {
         /* correct */
         "VDCICC:6225330/PCC/26/001/AE/P/RUS/RUS/715961901//P/20210830////KRASNOBORODKIN/DMITRY/19860613/M///8505/C///////",
         /* correct with excess field */
         "VDCICC:6225330/PCC/26/001/AE/P/RUS/RUS/715961901//P//20210830////KRASNOBORODKIN/DMITRY/19860613/M///8505/C///////",
         /* incorrect */
         "VDZZZZ:6225330/PCC/26/001/AE/P/RUS/RUS/715961901//P//20210830////KRASNOBORODKIN/DMITRY/19860613/M///8505/C///////",
         "VDCICC:6225330/ZZZ/26/001/AE/P/RUS/RUS/715961901//P//20210830////KRASNOBORODKIN/DMITRY/19860613/M///8505/C///////",
         "VDCICC:6225330/PCC/99/001/AE/P/RUS/RUS/715961901//P//20210830////KRASNOBORODKIN/DMITRY/19860613/M///8505/C///////",})
    {
        int tlg_num = saveTlg(OWN_CANON_NAME(), "zzzzz", "IAPP", text);
        putTlg2OutQueue_wrap(OWN_CANON_NAME(), "zzzzz", "IAPP", text, 0, tlg_num, 0);
        sendCmd("CMD_APPS_HANDLER", "H");
    }
    return 0;
}

std::string appsTextAsHumanReadable(const std::string& apps)
{
    std::string text = apps;
    text = StrUtils::replaceSubstrCopy(text, "\x02", "");
    text = StrUtils::replaceSubstrCopy(text, "\x03", "\n");
    return text;
}

void PaxReqAnswer::logAnswer(const std::string &country, int status_code,
                             int error_code, const std::string& error_text) const
{
    LEvntPrms params = getLogParams(country, status_code, error_code, error_text);
    PrmLexema lexema("passenger", "MSG.APPS_PASSENGER");
    lexema.prms << PrmSmpl<std::string>("passenger", family_name);
    params << lexema;

    CheckIn::TSimplePaxItem pax;
    if (pax.getByPaxId(pax_id.get())) {
        TReqInfo::Instance()->LocaleToLog("MSG.APPS_RESP", params, evtPax, msg.getPointId().get(), pax.reg_no);
    }
}

void ManifestAnswer::logAnswer(const std::string &country, int status_code,
                               int error_code, const std::string& error_text) const
{
    LEvntPrms params = getLogParams(country, status_code, error_code, error_text);
    params << PrmSmpl<std::string>("passenger", "");
    TReqInfo::Instance()->LocaleToLog("MSG.APPS_RESP", params, evtFlt, msg.getPointId().get());
}

void ManifestAnswer::processErrors() const
{
    if(errors.empty()) {
        return;
    }
    for(const auto & err : errors) {
        logAnswer(err.country, ASTRA::NoExists, err.error_code, err.error_text);
    }
    // CheckIfNeedResend();
    deleteMsg(msg.getMsgId());
}

void ManifestAnswer::processAnswer() const
{
    if(resp_code == ASTRA::NoExists) {
        return;
    }
    // Сообщение обработано системой. Залогируем ответ и удалим сообщение из apps_messages.
    logAnswer(country, resp_code, error_code, error_text);
    deleteMsg(msg.getMsgId());
}

std::string ManifestAnswer::toString() const
{
    std::ostringstream res;
    res << APPSAnswer::toString()
        << "country: " << country << std::endl << "resp_code: " << resp_code << std::endl
        << "error_code: " << error_code << std::endl << "error_text: " << error_text << std::endl;
    return res.str();
}

std::string getAnsText(const std::string& tlg)
{
    std::string::size_type pos1 = tlg.find("\x02");
    std::string::size_type pos2 = tlg.find("\x03");
    if (pos1 == std::string::npos) {
        throw Exception("Wrong answer format");
    }
    return tlg.substr(pos1 + 1, pos2 - pos1 - 1);
}

class AppsSetsCallbacksCache
{
public:
    static AppsSetsCallbacksCache & Instanse()
    {
        static AppsSetsCallbacksCache instance;
        return instance;
    }
    void clear() {
        cache.clear();
    }

    bool find(const TPaxSegmentPair& segmentPair) {
        LogTrace(TRACE5) << __FUNCTION__ ;
        auto it = cache.find(segmentPair);
        if(it != cache.end()) {
            return it->second;
        }
        bool is_need_apps = checkAPPSSegment(segmentPair);
        cache.emplace(segmentPair, is_need_apps);
        return is_need_apps;
    }

private:
    std::map<TPaxSegmentPair, bool> cache;
    AppsSetsCallbacksCache() = default;
};

class AppsTasks
{
public:
    static AppsTasks & Instanse()
    {
        static AppsTasks instance;
        return instance;
    }
    void clear() {
        m_tasks.clear();
    }
    void add(const TTripTaskKey& task) {
        m_tasks.insert(task);
    }
    const std::set<TTripTaskKey>& tasks() const {
        return m_tasks;
    }

private:
    std::set<TTripTaskKey> m_tasks;
    AppsTasks() = default;
};

void paxPnlOnChange(const PaxOrigin& paxOrigin, const PaxIdWithSegmentPair& paxId)
{
    auto paxSeg = paxId.getSegmentPair();
    if(!paxSeg) {
        return;
    }
    if(AppsSetsCallbacksCache::Instanse().find(*paxSeg)) {
        PointId_t point_id(paxSeg->point_dep);
        if(isNeedAddTask(point_id, SEND_NEW_APPS_INFO)) {
            addAlarmByPaxId(paxId().get(), {Alarm::SyncAPPS}, {paxOrigin});
            AppsTasks::Instanse().add(TTripTaskKey(point_id, SEND_NEW_APPS_INFO, ""));
        }
    }
}

class PaxChangesCallbacks : public PaxEventCallbacks<PaxChanges>
{
public:
    //ByPaxId
    void initialize(TRACE_SIGNATURE, const PaxOrigin& paxOrigin)
    {
        LogTrace(TRACE5) << __FUNCTION__;
        AppsTasks::Instanse().clear();
        AppsSetsCallbacksCache::Instanse().clear();
    }
    void finalize(TRACE_SIGNATURE, const PaxOrigin& paxOrigin)
    {
        LogTrace(TRACE5) << __FUNCTION__;
        for(const auto & t : AppsTasks::Instanse().tasks()) {
            sendAppsTask(PointId_t(t.point_id), t.name);
        }
    }

    void onChange(TRACE_SIGNATURE,
                const PaxOrigin& paxOrigin,
                const PaxIdWithSegmentPair& paxId,
                const std::set<PaxChanges>& paxChanges)
    {
        try
        {
            if(paxOrigin == paxPnl) {
                paxPnlOnChange(paxOrigin,paxId);
            } else if(paxOrigin == paxCheckIn) {

            }
        }
        catch(std::exception &e)
        {
            ProgError(STDLOG,"std::exception: %s", e.what());
        }
    }
};

class PaxRemCallbacks : public PaxEventCallbacks<TRemCategory>
{
public:

    //ByPaxId
    void initialize(TRACE_SIGNATURE, const PaxOrigin& paxOrigin)
    {
        LogTrace(TRACE5) << __FUNCTION__;
        AppsTasks::Instanse().clear();
        AppsSetsCallbacksCache::Instanse().clear();
    }
    void finalize(TRACE_SIGNATURE, const PaxOrigin& paxOrigin)
    {
        LogTrace(TRACE5) << __FUNCTION__;
        for(const auto & t : AppsTasks::Instanse().tasks()) {
            sendAppsTask(PointId_t(t.point_id), t.name);
        }
    }
    void onChange(TRACE_SIGNATURE,
                const PaxOrigin& paxOrigin,
                const PaxIdWithSegmentPair& paxId,
                const std::set<TRemCategory>& remCategories)
    {
        try
        {
            if(paxOrigin == paxPnl) {
                paxPnlOnChange(paxOrigin, paxId);
            } else if(paxOrigin == paxCheckIn) {

            }
        }
        catch(std::exception &e)
        {
            ProgError(STDLOG,"std::exception: %s", e.what());
        }
    }
};

void init_callbacks()
{
    static bool init = false;
    if (init) return;
    CallbacksSuite<PaxEventCallbacks<PaxChanges>>::Instance()->addCallbacks(new PaxChangesCallbacks);
    CallbacksSuite<PaxEventCallbacks<TRemCategory>>::Instance()->addCallbacks(new PaxRemCallbacks);
    init = true;
}

} //namespace APPS
