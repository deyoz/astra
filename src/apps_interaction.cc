#include <boost/algorithm/string.hpp>
#include <ctime>
#include <deque>
#include <boost/regex.hpp>
#include <sstream>
#include "alarms.h"
#include "apps_interaction.h"
#include "astra_misc.h"
#include "exceptions.h"
#include "points.h"
#include "qrys.h"
#include "trip_tasks.h"
#include "tlg/tlg.h"
#include <boost/scoped_ptr.hpp>
#include "apis_utils.h"

#include <serverlib/str_utils.h>
#include <serverlib/tcl_utils.h>
#include <serverlib/cursctl.h>
#include <serverlib/dump_table.h>
#include <serverlib/algo.h>

#define NICKNAME "FELIX"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>
#include <serverlib/dates_oci.h>
#include <serverlib/dates_io.h>
#include "ckin_search.h"
#include <functional>

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC::date_time;
using namespace ASTRA;

// ॠ�������� ���ᨨ ᮮ�饭�� 21 � 26

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

// ����� ᯥ�䨪�樨 ���㬥��
enum ESpecVer { SPEC_6_76, SPEC_6_83 };

const int pax_seq_num = 1;

static const int MaxCirqPaxNum = 5;
static const int MaxCicxPaxNum = 10;

const std::string APPS_FORMAT_21 = "APPS_21";
const std::string APPS_FORMAT_26 = "APPS_26";
const int APPS_VERSION_21 = 21;
const int APPS_VERSION_26 = 26;
const int APPS_VERSION_27 = 27;

const std::string AppsPaxDataReadQuery = "select PAX_ID, APPS_PAX_ID, STATUS, PAX_CREW, "
                                         " NATIONALITY, ISSUING_STATE, PASSPORT, CHECK_CHAR, DOC_TYPE, DOC_SUBTYPE, EXPIRY_DATE, "
                                         " SUP_CHECK_CHAR, SUP_DOC_TYPE, SUP_PASSPORT, FAMILY_NAME, GIVEN_NAMES, "
                                         " DATE_OF_BIRTH, SEX, BIRTH_COUNTRY, IS_ENDORSEE, TRANSFER_AT_ORGN, "
                                         " TRANSFER_AT_DEST, PNR_SOURCE, PNR_LOCATOR, PASS_REF, "
                                         " PRE_CKIN, "
                                         " POINT_ID, FLT_NUM, DEP_PORT, DEP_DATE, ARV_PORT, ARV_DATE, "
                                         " CKIN_FLT_NUM, CKIN_PORT, CKIN_POINT_ID,"
                                         " COUNTRY_FOR_DATA, DOCO_TYPE, DOCO_NO, COUNTRY_ISSUANCE, DOCO_EXPIRY_DATE, "
                                         " NUM_STREET, CITY, STATE, POSTAL_CODE, REDRESS_NUMBER, TRAVELLER_NUMBER "
                                         "from APPS_PAX_DATA ";


template <typename Container>
void traceContainer(const Container& cont, const std::string& hint)
{
    LogTrace(TRACE5) << __FUNCTION__ << " " << hint << " size: " << cont.size();
    for(const auto & item : cont) {
        LogTrace(TRACE5) << item << " ";
    }
}

// ����� � ���ᨮ�������
int fieldCount(std::string data_group, int version);
ESpecVer getSpecVer(int msg_ver);
static int versionByFormat(const std::string& fmt);
ASTRA::TGender::Enum femaleToGender(int is_female);

//����祭�� APPS ����஥�
std::set<AppsSettings> appsSetsForSegment(const TPaxSegmentPair &flt);
std::set<AppsSettings> appsSetsForSegment(const TAdvTripRouteItem& dep, const TAdvTripRouteItem& arv);
Opt<AppsSettings> appsSets(const std::string& airline, const std::string& country);
Opt<AppsSettings> appsSetsForTripItem(const TAdvTripRouteItem& item);
std::set<AppsSettings> appsSetsForRoute(const TAdvTripRoute& route);
std::set<AppsSettings> getAppsSetsByPaxId(const int pax_id);
std::set<AppsSettings> getAppsSetsByCrsPaxId(const int pax_id);
std::set<AppsSettings> filterByPreCheckin(const std::set<AppsSettings> & sets);
std::set<AppsSettings> filterByInboundOutbound(const std::set<AppsSettings> &sets);
std::set<std::string> needFltCloseout(const std::set<std::string>& countries, const string &airline);

// ��ࠡ�⪠ �࠭����� ���ᠦ�஢
TransferFlag getTransferFlagFromTransit(int point_id, const std::string& airp_arv);
TransferFlag getPaxTransferFlag(int point_id, int pax_grp_id, const std::string& airp_dep,
                                const std::string& airp_arv);
TransferFlag getCrsTransferFlag(const map<int, CheckIn::TTransferItem> & trfer, int point_id,
                                const std::string& airp_arv);
bool isPointTransferDestination(const int pax_grp_id, const std::string& airp_arv);
bool isPointTransferOrigin(const int pax_grp_id, const std::string& airp_dep);

// ����� � ���ﬨ APPS_PAX_DATA
void saveAppsStatus(const std::string& status, const std::string& apps_pax_id, const int msg_id);
void deleteAPPSData(const std::string & field, const int value);
void updateAppsCicxMsgId(int msg_id, const std::string& apps_pax_id);

// ����� � �ॢ�����
static void syncAPPSAlarms(const int pax_id, const int point_id_spp);
static void addAPPSAlarm(const int pax_id, const std::initializer_list<Alarm::Enum>& alarms,
                         const int point_id_spp=ASTRA::NoExists);
static void deleteAPPSAlarm(const int pax_id, const std::initializer_list<Alarm::Enum>& alarms,
                            const int point_id_spp=ASTRA::NoExists);
void deleteAPPSAlarms(const int pax_id, const int point_id_spp);
bool checkNeedAlarmScdIn(const int point_id);

/* �⨫��� ��� ���������� ������ PaxRequest � ��.*/
// TransactionData
static int getIdent();
static std::string getUserId(const std::string &airl);
static std::string makeHeader(const std::string& airl, int msgId);
static std::string getH2HReceiver();
static std::string getH2HTpr(int msgId);

// FlightData
void checkRouteSuffix(const TAdvTripRoute &route);

// PaxData
std::string outDocType(int version, const std::string &doc_type);
ASTRA::TTrickyGender::Enum getTrickyGender(const std::string &pers_type, ASTRA::TGender::Enum gender);
// PaxAddData
std::string issuePlaceToCountry(const std::string& issue_place);
// ��騥
bool isInternationalFlight(const std::string airp_dep, const std::string airp_arv);
bool isInternationalFlight(const TPaxSegmentPair &seg);
static Opt<PaxRequest> paxRequestFromDb(OciCpp::CursCtl & cur);
/***********************************************************/

// ������� ������ �� ����⠭⭮�� ����� AppsReqQuery
void defPaxAdd(PaxAddData &res, OciCpp::CursCtl & cur);
void defPax(PaxData & res, OciCpp::CursCtl & cur);
void defIntFlight(FlightData& res, OciCpp::CursCtl & cur);
void defTrans(TransactionData &res, OciCpp::CursCtl & cur);
void defCkinFlight(FlightData &res, OciCpp::CursCtl & cur);


// ��ࠢ�� APPS ᮮ�饭��
void updateCrsNeedApps(int pax_id);
void sendAppsMessages(const int pax_id, const std::set<AppsSettings> &apps_settings,
                      PaxRequest::RequestType reqType, const std::string& override_type,
                      const bool is_forced=false);
bool isAlreadyCheckedIn(const int pax_id);
static void sendAPPSInfo(const int point_id, const int point_id_tlg);

class APPSMessage
{
public:
    APPSMessage(const AppsSettings & settings, unique_ptr<IRequest> req)
        : settings(settings), request(move(req))
    {
        msg = request->msg(settings.version());
        msg_id = request->getMsgId();
        point_id = request->getPointId();
    }
    APPSMessage(const APPSMessage & msg)           = delete;
    APPSMessage& operator=(const APPSMessage &msg) = delete;
    APPSMessage(APPSMessage && other)              = default;
    APPSMessage & operator = (APPSMessage &&msg)   = default;

    void send() const
    {
        // ��ࠢ�� ⥫��ࠬ��
        sendTlg(settings.getRouter().c_str(), OWN_CANON_NAME(), qpOutApp, 20, getMsg(),
                ASTRA::NoExists, ASTRA::NoExists);
        sendCmd("CMD_APPS_HANDLER","H");
        ProgTrace(TRACE5, "New APPS request generated: %s", getMsg().c_str());
    }

    void save() const
    {
        request->save(settings.version());
        saveAppsMsg(getMsg(),  msg_id, point_id, settings.version());
    }
    void saveAppsMsg(const std::string& text, const int msg_id,
                     const int point_id, const int version) const;

    std::string getMsg() const
    {
        return msg;
    }
    int getMsgId() const
    {
        return msg_id;
    }
    void setMsgId(const int id)
    {
        msg_id = id;
    }

private:
    AppsSettings settings;
    unique_ptr<IRequest> request;
    std::string msg;
    int msg_id;
    int point_id;
};

void APPSMessage::saveAppsMsg(const std::string& text, const int msg_id,
                              const int point_id, const int version) const
{
    // ��࠭�� ���ଠ�� � ᮮ�饭��
    auto cur = make_curs(
               "insert into APPS_MESSAGES(MSG_ID, MSG_TEXT, SEND_ATTEMPTS, SEND_TIME, POINT_ID, VERSION) "
               "values (:msg_id, :msg_text, :send_attempts, :send_time, :point_id, :version)");
    cur
            .bind(":msg_id",msg_id)
            .bind(":msg_text",text)
            .bind(":send_attempts",1)
            .bind(":send_time",Dates::second_clock::universal_time()) // ⮦� ᠬ�� DateTimeToBoost(NowUTC())
            .bind(":point_id",point_id)
            .bind(":version",version)
            .exec();
}

void processCrsPax(const int pax_id, const std::string& override_type)
{
    ProgTrace(TRACE5, "processCrsPax: %d", pax_id);
    auto settings = getAppsSetsByCrsPaxId(pax_id);
    settings = filterByInboundOutbound(settings);
    settings = filterByPreCheckin(settings);
    if(!settings.empty() && !isAlreadyCheckedIn(pax_id)) {
        sendAppsMessages(pax_id, settings, PaxRequest::Crs, override_type);
    }
}

void processPax(const int pax_id, const std::string& override_type, const bool is_forced)
{
    ProgTrace(TRACE5, "processPax: %d", pax_id);
    auto settings = getAppsSetsByPaxId(pax_id);
    settings = filterByInboundOutbound(settings);
    if(!getAppsSetsByPaxId(pax_id).empty()) {
        sendAppsMessages(pax_id, settings, PaxRequest::Pax, override_type, is_forced);
    }
}

void sendAppsMessages(const int pax_id, const std::set<AppsSettings> & apps_settings,
                      PaxRequest::RequestType reqType, const std::string& override_type,
                      const bool is_forced)
{
    std::vector<APPSMessage> messages;
    LogTrace(TRACE5) << __FUNCTION__ <<" pax_id: "<<pax_id;
    for(const auto & country_settings : apps_settings)
    {
        Opt<PaxRequest> new_pax = PaxRequest::createFromDB(reqType, pax_id, override_type);
        if(!new_pax) {
            continue;
        }
        //LogTrace(TRACE5) << __FUNCTION__ << " new_pax trans_code: " << new_pax->getTrans().code;
        Opt<PaxRequest> actual_pax = PaxRequest::fromDBByPaxId(pax_id);
        bool is_exists = actual_pax.is_initialized();
        bool is_the_same = (actual_pax == new_pax);
        bool needNew=false, needCancel=false, needUpdate=false;
        const std::string status = (is_exists) ? actual_pax->getStatus() : "";
        needNew = new_pax->isNeedNew(is_exists, status, is_the_same, is_forced);
        new_pax->ifNeedDeleteAlarms(is_exists, status);
        if(is_exists) {
            needCancel = new_pax->isNeedCancel(status);
            needUpdate = new_pax->isNeedUpdate(status, is_the_same);
            new_pax->ifNeedDeleteApps(status);
            new_pax->ifOutOfSyncSendAlarm(status, is_the_same);
        }
//        LogTrace(TRACE5)<<__FUNCTION__
//                        <<" is_exists: "    << is_exists  <<" is_the_same: "<< is_the_same
//                        <<" is_the_cancel: "<< (is_exists && actual_pax->getTrans().code=="CICX")
//                        <<" status: "       << status    <<" needNew: "<< needNew
//                        <<" needCancel: "   << needCancel <<" needUpdate: "<<needUpdate;

        if(needCancel || needUpdate) {
             messages.emplace_back(country_settings, make_unique<PaxRequest>(actual_pax.get()));
        }
        if(needNew || needUpdate) {
            messages.emplace_back(country_settings, make_unique<PaxRequest>(new_pax.get()));
        }
    }
    //LogTrace(TRACE5) << __FUNCTION__ << " Messages size: " << messages.size();
    for(const auto & msg : messages) {
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
    } else {
        return SPEC_6_83;
    }
}

int fieldCount(std::string data_group, int version)
{
    // ������ᨬ� �� ���ᨨ
    if (data_group == "CHK") return 2;
    if (data_group == "INT") return 8;
    if (data_group == "INM") return 3;
    if (data_group == "MRQ") return 3;
    if (data_group == "PCC") return 26;
    if (data_group == "EXO") return 4;
    if (data_group == "EXD") return 4;
    if (data_group == "MAK") return 4;
    // ����ᨬ� �� ���ᨨ
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
        if (data_group == "PCX") return 21; // NOTE � ᯥ�䨪�樨 ��� ���ᨨ 26 ��� �⮩ ��㯯� ������
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

int AppsSettings::version() const
{
    return versionByFormat(format);
}

std::string getH2HReceiver()
{
    static std::string var = readStringFromTcl("APPS_H2H_ADDR");
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

std::string makeHeader(const std::string& airl, int msgId)
{
    return std::string("V.\rVHLG.WA/E5" + std::string(OWN_CANON_NAME()) + "/I5" +
                       getH2HReceiver() + "/P" + getH2HTpr(msgId) + "\rVGZ.\rV" +
                       BaseTables::Company(airl)->lcode() + "/MOW/////////RU\r");
}

std::string getUserId(const std::string &airl)
{
    std::string user_id;
    BaseTables::Company comp(airl);
    if (!comp->lcode().empty() && !comp->lcodeIcao().empty()) {
        user_id =  comp->lcode() + comp->lcodeIcao() + "1";
    }
    else user_id = "AST1H";
    return user_id;
}

int getIdent()
{
    int vid = 0;
    auto cur = make_curs(
"select APPS_MSG_ID__SEQ.NEXTVAL VID from DUAL");
    cur
       .def(vid)
       .EXfet();
    return vid;
}

const char* getAPPSRotName()
{
    static std::string var = readStringFromTcl("APPS_ROT_NAME");
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

bool checkAPPSFormats(const int point_dep, const std::string& airp_arv, std::set<std::string>& pFormats)
{
    TAdvTripRoute route = getTransitRoute(TPaxSegmentPair{point_dep,airp_arv});
    auto sets = appsSetsForRoute(route);
    for(const auto& set: sets) {
        pFormats.insert(set.getFormat());
    }
    return !sets.empty();
}

template <typename Func>
bool checkAppsOnRouteWithPredicate(const TAdvTripRoute & route, Func func)
{
    for(auto it = begin(route); it != prev(end(route)); it++) {
        if(!appsSetsForSegment(*it, *next(it)).empty() && func(*it)) {
            return true;
        }
    }
    return false;
}

bool checkNeedAlarmScdIn(const int point_id)
{
    return checkAppsOnRouteWithPredicate(getTransitRoute(TPaxSegmentPair{point_id,""}),
                               [](const TAdvTripRouteItem & r){return r.scd_in == ASTRA::NoExists;});
}

bool checkAPPSSets(const int point_dep, const std::string& airp_arv)
{
    TAdvTripRoute route = getTransitRoute(TPaxSegmentPair{point_dep,airp_arv});
    if(route.size() < 2) {
        return false;
    }
    return checkAppsOnRouteWithPredicate(route, [](auto &r){return true;});
}

bool checkAPPSSets(const int point_dep, const int point_arv)
{
    auto point_info = getPointInfo(point_arv);
    if(!point_info) {
        return false;
    }
    std::string airp_arv = point_info->airp;
    return checkAPPSSets(point_dep, airp_arv);
}

bool checkTime(const int point_id)
{
    TDateTime start_time = ASTRA::NoExists;
    return checkTime(point_id, start_time);
}

bool checkTime(const int point_id, TDateTime& start_time)
{
    start_time = ASTRA::NoExists;
    TDateTime now = NowUTC();
    auto point_info = getPointInfo(point_id);
    if(!point_info) {
        return false;
    }
    // The APP System only allows transactions on [- 2 days] TODAY [+ 10 days].
    if ((now - point_info->scd_out) > 2)
        return false;
    if ((point_info->scd_out - now) > 10) {
        start_time = point_info->scd_out - 10;
        return false;
    }
    return true;
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

void deleteAPPSData(const std::string & field,  const int value)
{
    LogTrace(TRACE5) << __FUNCTION__ << " field: " << field << " value: " << value;
    auto cur = make_curs(
               "delete from APPS_PAX_DATA where "+field+ "= :val");
    cur.bind(":val", value).exec();
}

void syncAPPSAlarms(const int pax_id, const int point_id_spp)
{
    if (point_id_spp != ASTRA::NoExists) {
        TTripAlarmHook::set(Alarm::APPSProblem, point_id_spp);
    }
    else {
        if (pax_id != ASTRA::NoExists) {
            TPaxAlarmHook::set(Alarm::APPSProblem, pax_id);
            TCrsPaxAlarmHook::set(Alarm::APPSProblem, pax_id);
        }
    }
}

void addAPPSAlarm(const int pax_id,
                  const std::initializer_list<Alarm::Enum>& alarms,
                  const int point_id_spp)
{
    LogTrace(TRACE5)<<__FUNCTION__<<" pax_id: "<< pax_id;
    if (addAlarmByPaxId(pax_id, alarms, {paxCheckIn, paxPnl})) {
        syncAPPSAlarms(pax_id, point_id_spp);
    }
}

void deleteAPPSAlarm(const int pax_id,
                     const std::initializer_list<Alarm::Enum>& alarms,
                     const int point_id_spp)
{
    LogTrace(TRACE5)<<__FUNCTION__<<" pax_id: "<< pax_id;
    if (deleteAlarmByPaxId(pax_id, alarms, {paxCheckIn, paxPnl})) {
        syncAPPSAlarms(pax_id, point_id_spp);
    }
}

void deleteAPPSAlarms(const int pax_id, const int point_id_spp)
{
    LogTrace(TRACE5)<<__FUNCTION__<<" pax_id: "<<pax_id;
    if (deleteAlarmByPaxId(
                pax_id,
                {Alarm::APPSNegativeDirective, Alarm::APPSError,Alarm::APPSConflict},
                {paxCheckIn, paxPnl}))
    {
        if (point_id_spp!=ASTRA::NoExists) {
            syncAPPSAlarms(pax_id, point_id_spp);
        }
        else {
            LogError(STDLOG) << __FUNCTION__ << ": point_id_spp==ASTRA::NoExists";
        }
    }
}

void AppsCollector::addPaxItem(int pax_id, const std::string &override, bool is_forced)
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

Opt<AppsSettings> AppsSettings::readSettings(const std::string& airline,
                                             const std::string& country)
{
    LogTrace(TRACE5)<<__FUNCTION__<< "Airline: "<<airline << " country: "<<country;
    AppsSettings settings = {};
    auto cur = make_curs(
               "select FORMAT, ID, FLT_CLOSEOUT, INBOUND, OUTBOUND, PR_DENIAL, AIRLINE, "
               "APPS_COUNTRY, PRE_CHECKIN "
               "from APPS_SETS "
               "where AIRLINE=:airline and APPS_COUNTRY=:country and PR_DENIAL=0");
    cur
            .def(settings.format)
            .def(settings.id)
            .def(settings.flt_closeout)
            .def(settings.inbound)
            .def(settings.outbound)
            .def(settings.denial)
            .def(settings.airline)
            .def(settings.country)
            .def(settings.pre_checkin)
            .bind(":airline", airline)
            .bind(":country", country)
            .EXfet();
    if(cur.err() == NO_DATA_FOUND) {
        return boost::none;
    }

    settings.setRouter(std::string(getAPPSRotName()));
    return settings;
}

TransactionData& TransactionData::init(const std::string& trans_code, const std::string& airline)
{
    //LogTrace(TRACE5) << __FUNCTION__ << " code=" << trans_code;
    code = trans_code;
    msg_id = getIdent();
    user_id = getUserId(airline);
    header = makeHeader(airline, msg_id);
    return *this;
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

std::string TransactionData::msg(int version) const
{
    validateData();
    std::ostringstream msg;
    /* 1 */ msg << code << ':';
    /* 2 */ msg << msg_id << '/';
    /* 3 */ msg << user_id << '/';
    if (code == "CIRQ" || code == "CICX")
    {
        /* 4 */ msg << "N" << '/';
        /* 5 */ msg << (type ? "P" : "") << '/';
        /* 6 */ msg << version;
    }
    else if (code == "CIMR")
    {
        /* 4 */ msg << version;
    }
    return msg.str();
}

FlightData& FlightData::init(const TPaxSegmentPair & flt, const std::string& flt_type)
{
    ProgTrace(TRACE5, "TFlightData::init: %d %s", flt.point_dep, flt.airp_arv.c_str());
//    if (flt.point_id == ASTRA::NoExists) {
//        return *this;
//    }
    this->point_id = flt.point_dep;
    this->arv_port = flt.airp_arv;
    return init(flt_type);
}

FlightData &FlightData::init(const std::string &flt_type)
{
    setType(flt_type);
    auto route = getTransitRoute(TPaxSegmentPair{point_id, arv_port});
    setFltNum(route.front().airline_out, route);
    setDepPortAndDate(route);
    setArvPortAndDate(route);
    return *this;
}

FlightData& FlightData::setFltNum(const std::string &airline, const TAdvTripRoute &route)
{
    std::ostringstream trip;
    trip << BaseTables::Company(airline)->lcode()
         << setw(3) << setfill('0') << route.front().flt_num_out
         << route.front().suffix_out;

    flt_num = trip.str();
    return *this;
}

FlightData& FlightData::setDepPortAndDate(const TAdvTripRoute &route)
{
    ASSERT(!route.empty());
    LogTrace(TRACE5)<<__FUNCTION__;
    BaseTables::Port port(route.front().airp);
    dep_port = port->lcode();

    if (dep_port.empty()) {
        throw Exception("airp_dep.code_lat empty (code=%s)", dep_port.c_str());
    }
    if(type != "CHK" && route.front().scd_out != ASTRA::NoExists) {
        dep_date = DateTimeToBoost(UTCToLocal(route.front().scd_out, port->city()->tzRegion()));
    }
    return *this;
}

FlightData& FlightData::setArvPortAndDate(const TAdvTripRoute &route)
{
    ASSERT(!route.empty());
    LogTrace(TRACE5)<<__FUNCTION__;
    if(type == "INT") {
        BaseTables::Port port(route.back().airp);
        arv_port = port->lcode();
        if (arv_port.empty()) {
            throw Exception("arv_port empty (code=%s)",arv_port.c_str());
        }
        if(route.back().scd_in != ASTRA::NoExists) {
            arv_date = DateTimeToBoost(UTCToLocal(route.back().scd_in, port->city()->tzRegion()));
        }
    }
    return *this;
}



void FlightData::validateData() const
{
    if(type != "CHK" && type != "INM" && type != "INT")
        throw Exception("Incorrect type of flight: %s", type.c_str());
    if (flt_num.empty() || flt_num.size() > 8)
        throw Exception(" Incorrect flt_num: %s", flt_num.c_str());
    if (dep_port.empty() || dep_port.size() > 5)
        throw Exception("Incorrect airport: %s", dep_port.c_str());
    if (type != "CHK" && dep_date.is_not_a_date_time())
        throw Exception("Date empty");
    if (type == "INT") {
        if (arv_port.empty() || arv_port.size() > 5)
            throw Exception("Incorrect arrival airport: %s", arv_port.c_str());
        if (arv_date.is_not_a_date_time()) {
            TTripAlarmHook::setAlways(Alarm::APPSNotScdInTime, point_id);
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
        /* 3  */ msg << dep_port << '/';
        /* 4  */ msg << flt_num;
    }
    else if (type == "INT")
    {
        /* 3  */ msg << "S" << '/';
        /* 4  */ msg << flt_num << '/';
        /* 5  */ msg << dep_port << '/';
        /* 6  */ msg << arv_port << '/';
        /* 7  */ msg << HelpCpp::string_cast(dep_date, "%Y%m%d") << '/';
        /* 8  */ msg << HelpCpp::string_cast(dep_date, "%H%M%S") << '/';
        /* 9  */ msg << HelpCpp::string_cast(arv_date, "%Y%m%d") << '/';
        /* 10 */ msg << HelpCpp::string_cast(arv_date, "%H%M%S");
    }
    else if (type == "INM")
    {
        /* 3  */ msg << flt_num << '/';
        /* 4  */ msg << dep_port << '/';
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

static CheckIn::TPaxDocItem loadPaxDoc(int pax_id)
{
    CheckIn::TPaxDocItem doc;
    if (!CheckIn::LoadPaxDoc(pax_id, doc))
        CheckIn::LoadCrsPaxDoc(pax_id, doc);
    return doc;
}

void PaxData::init(const int pax_id, const std::string& surname, const std::string& name,
                   const bool is_crew, const std::string& override_type,
                   const int reg_no, const ASTRA::TTrickyGender::Enum tricky_gender,
                   TransferFlag transfer)
{
    LogTrace(TRACE5) << __FUNCTION__ << " pax_id: " << pax_id << " surname: " << surname;
    const CheckIn::TPaxDocItem doc = loadPaxDoc(pax_id);
    setPaxId(pax_id);
    setTypeOfPax(is_crew);
    setDocInfo(doc);
    setNames(doc, surname, name);
    setTransfer(transfer);
    setOverrideType(override_type);
    setPassReference(tricky_gender, reg_no);
}

void PaxData::setOverrideType(const std::string& override_type)
{
    if(!override_type.empty()) {
        override_codes = override_type;
    }
}

void PaxData::setTypeOfPax(const bool is_crew)
{
    pax_crew = is_crew ? "C" : "P";
}

void PaxData::setTransfer(const TransferFlag trfer)
{
    LogTrace(TRACE5) << __FUNCTION__ << " Transfer = " << trfer;
    trfer_at_origin = (trfer == TransferFlag::Origin || trfer == TransferFlag::Both) ? "Y" : "N";
    trfer_at_dest = (trfer == TransferFlag::Dest || trfer == TransferFlag::Both) ? "Y" : "N";
}

void PaxData::setDocInfo(const CheckIn::TPaxDocItem& doc)
{
    nationality = doc.nationality;
    issuing_state = doc.issue_country;
    passport = doc.no.substr(0, 14);

    doc_type = doc.type;
    doc_subtype = doc.subtype;
    if (doc.expiry_date != ASTRA::NoExists) {
        expiry_date = DateTimeToStr(doc.expiry_date, "yyyymmdd");
    }
    if (doc.birth_date != ASTRA::NoExists) {
        birth_date = DateTimeToStr(doc.birth_date, "yyyymmdd");
    }
    sex = (doc.gender == "M" || doc.gender == "F") ? doc.gender : "U";
}

void PaxData::setNames(const CheckIn::TPaxDocItem& doc, const std::string& surname, const string &name)
{
    if(!doc.surname.empty()) {
        family_name = transliter(doc.surname, 1, 1);
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
        family_name = surname;
        family_name = transliter(family_name, 1, 1);
        if(!name.empty()) {
            given_names = (transliter(name, 1, 1));
        }
    }
}

void PaxData::setPassReference(const ASTRA::TTrickyGender::Enum tricky_gender,
                               const int reg_no)
{
    int seq_num = (reg_no == ASTRA::NoExists) ? 0 : reg_no;
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
    reference = ref.str();
}

bool PaxData::operator ==(const PaxData &data) const
{
    return  pax_id           == data.pax_id          &&
            pax_crew         == data.pax_crew        &&
            nationality      == data.nationality     &&
            issuing_state    == data.issuing_state   &&
            passport         == data.passport        &&
            check_char       == data.check_char      &&
            expiry_date      == data.expiry_date     &&
            sup_check_char   == data.sup_check_char  &&
            sup_doc_type     == data.sup_doc_type    &&
            sup_passport     == data.sup_passport    &&
            family_name      == data.family_name     &&
            given_names      == data.given_names     &&
            birth_date       == data.birth_date      &&
            sex              == data.sex             &&
            birth_country    == data.birth_country   &&
            endorsee         == data.endorsee        &&
            trfer_at_origin  == data.trfer_at_origin &&
            trfer_at_dest    == data.trfer_at_dest   &&
            pnr_source       == data.pnr_source      &&
            pnr_locator      == data.pnr_locator     &&
            reference        == data.reference;
}

void PaxData::validateData() const
{
    if (pax_id == ASTRA::NoExists)
        throw Exception("Empty pax_id");
    if (pax_crew != "C" && pax_crew != "P")
        throw Exception("Incorrect pax_crew %s", pax_crew.c_str());
    if(passport.size() > 14)
        throw Exception("Passport number too long: %s", passport.c_str());
    if(!doc_type.empty() && doc_type != "P" && doc_type != "O" && doc_type != "N" && doc_type != "I")
        //!!!!throw Exception("Incorrect doc_type: %s", doc_type.c_str());
        LogTrace(TRACE5)<<__FUNCTION__<<" doc_type member = "<<doc_type;
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

std::string PaxData::msg(int version) const
{
    validateData();
    std::ostringstream msg;
    std::string data_group;
    if (apps_pax_id.empty())
    {
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
    else
    {
        /* PCX */
        data_group = "PCX";
        /* 1  */ msg << data_group << '/';
        /* 2  */ msg << fieldCount(data_group, version) << '/';
        /* 3  */ msg << pax_seq_num << '/';
        /* 4  */ msg << apps_pax_id << '/';
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

Opt<PaxAddData> PaxAddData::createByPaxId(const int pax_id)
{
    PaxAddData data;
    CheckIn::TPaxDocoItem doco;
    if (!CheckIn::LoadPaxDoco(pax_id, doco))
    {
        return boost::none;
    }
    data.init(pax_id, doco);
    return data;
}

Opt<PaxAddData> PaxAddData::createByCrsPaxId(const int pax_id)
{
    PaxAddData data;
    CheckIn::TPaxDocoItem doco;
    if (!CheckIn::LoadCrsPaxVisa(pax_id, doco))
    {
        return boost::none;
    }
    data.init(pax_id, doco);
    return data;
}

void PaxAddData::init(const int pax_id, const CheckIn::TPaxDocoItem &doco)
{
    ProgTrace(TRACE5, "TPaxAddData::init: %d", pax_id);
    CheckIn::TPaxDocaItem docaD;
    if (!CheckIn::LoadPaxDoca(pax_id, CheckIn::docaDestination, docaD))
    {
        CheckIn::TDocaMap doca_map;
        CheckIn::LoadCrsPaxDoca(pax_id, doca_map);
        docaD = doca_map[apiDocaD];
    }

    if (!doco.applic_country.empty())
    {
        //ToDo
        //BaseTables::Country country_code
        std::string country_code = ((const TPaxDocCountriesRow&)base_tables.get("pax_doc_countries").get_row("code", doco.applic_country)).country;
        country_for_data = country_code.empty() ? "" : BaseTables::Country(country_code)->lcode();
    }
    doco_type = doco.type;
    doco_no = doco.no.substr(0, 20); // � �� doco.no VARCHAR2(25 BYTE)
    country_issuance = issuePlaceToCountry(doco.issue_place);

    if (doco.expiry_date != ASTRA::NoExists) {
        doco_expiry_date = DateTimeToStr(doco.expiry_date, "yyyymmdd");
    }

    num_street = docaD.address;
    city = docaD.city;
    state = docaD.region.substr(0, 20); // ��筨��
    postal_code = docaD.postal_code;

    redress_number = "";
    traveller_number = "";
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
    /* 6  */ msg << '/'; // subtype ���� �� �ᯮ������
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

bool PaxRequest::isNeedNew(const bool is_exists, const std::string& status, bool is_the_same, bool is_forced) const
{
    return trans.code!="CICX" && (!is_exists || (is_exists && status=="P" && (!is_the_same || is_forced)));
}

void PaxRequest::ifNeedDeleteAlarms(const bool is_exists, const std::string& status) const
{
    if(trans.code=="CICX" && (!is_exists || !status.empty())) {
        deleteAPPSAlarms(pax.pax_id,  int_flt.point_id);
    }
}

void PaxRequest::ifOutOfSyncSendAlarm(const std::string& status, const bool is_the_same) const
{
    if(status.empty() && !is_the_same) {
        addAPPSAlarm(pax.pax_id, {Alarm::APPSConflict}, int_flt.point_id);
    }
}

void PaxRequest::ifNeedDeleteApps(const std::string& status) const
{
    if(trans.code=="CICX" && status!="B") {
        deleteAPPSData("PAX_ID", pax.pax_id);
    }
}

bool isAlreadyCheckedIn(const int pax_id)
{
    auto cur = make_curs(
               "select 1 from PAX where PAX_ID=:pax_id and REFUSE is null");
    cur
        .bind(":pax_id", pax_id)
        .exfet();
    return cur.err() != NO_DATA_FOUND;
}

Opt<AppsSettings> appsSetsForTripItem(const TAdvTripRouteItem& item)
{
    LogTrace(TRACE5) << __FUNCTION__ << " airline: " << item.airline_out
                     << " country: " << getCountryByAirp(item.airp).code << " airp: "
                     << item.airp;
    return appsSets(item.airline_out, getCountryByAirp(item.airp).code);
}

Opt<AppsSettings> appsSets(const std::string& airline, const std::string& country)
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
    std::set<AppsSettings> res;
    for(auto it = begin(route); it != prev(end(route)); it++) {
        auto seg_sets = appsSetsForSegment(*it, *next(it));
        res.insert(begin(seg_sets), end(seg_sets));
    }
    return res;
}

std::set<AppsSettings> appsSetsForSegment(const TAdvTripRouteItem& dep, const TAdvTripRouteItem& arv)
{
    const std::string airline = dep.airline_out;
    std::set<AppsSettings> res;
    if(auto set_dep = AppsSettings::readSettings(airline, getCountryByAirp(dep.airp).code)) {
        if(set_dep->getOutbound()) {
            res.insert(*set_dep);
        }
    }
    if(auto set_arv = AppsSettings::readSettings(airline, getCountryByAirp(arv.airp).code)) {
        if(set_arv->getInbound()) {
            res.insert(*set_arv);
        }
    }
    return res;
}

std::set<AppsSettings> appsSetsForSegment(const TPaxSegmentPair & flt)
{
    auto info = getPointInfo(flt.point_dep);
    if(!info) {
        return {};
    }

//    LogTrace(TRACE5) << __FUNCTION__ << " Airline: " << info->airline << " airp_dep: "
//                     << info->airp << " airp_arv: " << flt.airp_arv;
    std::set<AppsSettings> res;
    if(auto set_dep = AppsSettings::readSettings(info->airline, getCountryByAirp(info->airp).code)) {
        if(set_dep->getOutbound()) {
            res.insert(*set_dep);
        }
    }
    if(auto set_arv = AppsSettings::readSettings(info->airline, getCountryByAirp(flt.airp_arv).code)) {
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

std::set<AppsSettings> filterByInboundOutbound(const std::set<AppsSettings> & sets)
{
    return algo::filter(sets, [&](auto set)
    {
        if(set.getInbound() || set.getOutbound()) {
            return true;
        } else {
            if(!set.getInbound()) {
                LogTrace(TRACE3) << "Filtered by inbound";
            }
            if(!set.getOutbound()) {
                LogTrace(TRACE3) << "Filtered by outbound";
            }
            return false;
        }
    });
}

std::set<AppsSettings> getAppsSetsByPaxId(const int pax_id)
{
    auto seg = CheckIn::paxSegment(pax_id);
    if(!seg) {
        return {};
    }
    return appsSetsForSegment(*seg);
}

std::set<AppsSettings> getAppsSetsByCrsPaxId(const int pax_id)
{
    auto seg = CheckIn::crsSegment(pax_id);
    if(!seg) {
        return {};
    }
    return appsSetsForSegment(*seg);
}

void defPaxAdd(PaxAddData &res, OciCpp::CursCtl & cur)
{
    cur
        .defNull(res.country_for_data,"")
        .defNull(res.doco_type,"")
        .defNull(res.doco_no,"")
        .defNull(res.country_issuance,"")
        .defNull(res.doco_expiry_date,"")
        .defNull(res.num_street,"")
        .defNull(res.city,"")
        .defNull(res.state,"")
        .defNull(res.postal_code,"")
        .defNull(res.redress_number,"")
        .defNull(res.traveller_number,"");
}

void defPax(PaxData & res, OciCpp::CursCtl & cur)
{
    cur
        .def    (res.pax_id)
        .defNull(res.apps_pax_id,"")
        .defNull(res.status,"")
        .def    (res.pax_crew)
        .defNull(res.nationality,"")
        .defNull(res.issuing_state,"")
        .defNull(res.passport,"")
        .defNull(res.check_char,"")
        .defNull(res.doc_type,"")
        .defNull(res.doc_subtype,"")
        .defNull(res.expiry_date,"")
        .defNull(res.sup_check_char,"")
        .defNull(res.sup_doc_type,"")
        .defNull(res.sup_passport,"")
        .def    (res.family_name)
        .defNull(res.given_names,"")
        .defNull(res.birth_date,"")
        .defNull(res.sex,"")
        .defNull(res.birth_country,"")
        .defNull(res.endorsee,"")
        .defNull(res.trfer_at_origin,"")
        .defNull(res.trfer_at_dest,"")
        .defNull(res.pnr_source,"")
        .defNull(res.pnr_locator,"")
        .defNull(res.reference,"");
}
void defIntFlight(FlightData& res, OciCpp::CursCtl & cur)
{
    cur
        .def(res.point_id)
        .def(res.flt_num)
        .def(res.dep_port)
        .def(res.dep_date)
        .def(res.arv_port)
        .def(res.arv_date);
}

void defTrans(TransactionData &res, OciCpp::CursCtl & cur)
{
    cur
        .def(res.type);    //pre_ckin
}

void defCkinFlight(FlightData &res, OciCpp::CursCtl & cur)
{
    cur
        .defNull(res.flt_num,"")
        .defNull(res.dep_port,"")
        .defNull(res.point_id, ASTRA::NoExists);
}

static Opt<PaxRequest> paxRequestFromDb(OciCpp::CursCtl & cur)
{
    LogTrace(TRACE5) << __FUNCTION__;
    PaxData pax;
    TransactionData trs;
    FlightData intFlight;
    FlightData ckinFlight;
    PaxAddData paxAddData;

    defPax(pax, cur);
    defTrans(trs, cur);
    defIntFlight(intFlight, cur);
    defCkinFlight(ckinFlight, cur);
    defPaxAdd(paxAddData, cur);
    cur.EXfet();
    if(cur.err() == NO_DATA_FOUND){
        return boost::none;
    }
    auto point_info = getPointInfo(intFlight.point_id);
    if(!point_info) {
        return boost::none;
    }
    trs.init("CICX", point_info->airline);
    LogTrace(TRACE5) << __FUNCTION__ << " Port " << intFlight.arv_port;

    std::string int_arv_port, ckin_arv_port;
    ASSERT(intFlight.point_id != ASTRA::NoExists);
    BaseTables::Port int_port(intFlight.arv_port);
    int_arv_port = int_port->rcode();
    intFlight.init(TPaxSegmentPair{intFlight.point_id, int_arv_port},  "INT");
    if(ckinFlight.point_id != ASTRA::NoExists) {
        BaseTables::Port ckin_port(ckinFlight.arv_port);
        ckin_arv_port = ckin_port->rcode();
        ckinFlight.init(TPaxSegmentPair{ckinFlight.point_id, ckin_arv_port}, "CHK");
    }
    Opt<PaxAddData> pax_add;
    if(!paxAddData.country_for_data.empty()) {
        pax_add = paxAddData;
    }
    return PaxRequest(trs, intFlight, ckinFlight, pax, pax_add);
}

Opt<PaxRequest> PaxRequest::fromDBByPaxId(const int pax_id)
{
    LogTrace(TRACE3) << __FUNCTION__ << " by pax_id:" << pax_id;
    auto cur = make_curs(
               AppsPaxDataReadQuery + "where PAX_ID = :pax_id");
    cur.bind(":pax_id",pax_id);
    return paxRequestFromDb(cur);
}

Opt<PaxRequest> PaxRequest::fromDBByMsgId(const int msg_id)
{
    LogTrace(TRACE3) << __FUNCTION__ << " by msg_id:" << msg_id;
    auto cur = make_curs(
               AppsPaxDataReadQuery + "where CIRQ_MSG_ID = :msg_id");
    cur.bind(":msg_id",msg_id);
    return paxRequestFromDb(cur);
}

ASTRA::TTrickyGender::Enum getTrickyGender(const std::string &pers_type, ASTRA::TGender::Enum gender)
{
    return CheckIn::TSimplePaxItem::getTrickyGender(DecodePerson(pers_type.c_str()), gender);
}

bool isInternationalFlight(const std::string airp_dep, const std::string airp_arv)
{
    return getCountryByAirp(airp_dep).code != getCountryByAirp(airp_arv).code;
}

bool isInternationalFlight(const TPaxSegmentPair &seg)
{
    Opt<TTripInfo> point_info = getPointInfo(seg.point_dep);
    if(!point_info) {
        return false;
    }
    return isInternationalFlight(point_info->airp, seg.airp_arv);
}

Opt<PaxRequest> PaxRequest::createByPaxId(const int pax_id, const std::string& override_type)
{
    LogTrace(TRACE5)<<__FUNCTION__<<" pax_id: "<<pax_id;
    auto cur = make_curs(
                "select SURNAME, NAME, PAX.GRP_ID, STATUS, POINT_DEP, REFUSE, AIRP_DEP, AIRP_ARV "
                ", PERS_TYPE, IS_FEMALE, REG_NO "
                "from PAX_GRP, PAX "
                "where PAX_ID = :pax_id and PAX_GRP.GRP_ID = PAX.GRP_ID");
    MetaInfo info = {};
    std::string status, refuse;
    int point_dep = 0, reg_no = 0, is_female = 0;
    const int NullFemale = -1;
    cur
        .def(info.surname)
        .def(info.name)
        .def(info.grp_id)
        .def(status)
        .def(point_dep)
        .defNull(refuse,"")
        .def(info.airp_dep)
        .def(info.airp_arv)
        .def(info.pers_type)
        .defNull(is_female, NullFemale)
        .def(reg_no)
        .bind(":pax_id", pax_id)
        .EXfet();
    if(cur.err() == NO_DATA_FOUND) {
        return boost::none;
    }
    LogTrace(TRACE5) << __FUNCTION__ << " airp_dep : " << info.airp_dep;
    Opt<TTripInfo> point_info = getPointInfo(point_dep);
    if(!point_info) {
        return boost::none;
    }
    if(!isInternationalFlight(info.airp_dep, info.airp_arv)) {
        return boost::none;
    }
    ASTRA::TGender::Enum gender =
            (is_female == NullFemale) ? ASTRA::TGender::Unknown
                                      : femaleToGender(is_female);
    return createByFields(PaxRequest::Pax, pax_id, point_dep, info, false, status, !refuse.empty(),
                          gender, override_type, point_info->airline, reg_no);
}

Opt<PaxRequest> PaxRequest::createByFields(
        const PaxRequest::RequestType reqType, const int pax_id, const int point_id,
        const MetaInfo& info, bool pre_checkin, const std::string& status,
        const bool need_del, ASTRA::TGender::Enum gender, const std::string& override_type,
        const std::string& airline, int reg_no)
{
    PaxData pax;
    TransactionData trs;
    FlightData intFlight;
    FlightData ckinFlight;

    trs.init(need_del ? "CICX":"CIRQ", airline).setPreCheckin(pre_checkin);
    intFlight.init(TPaxSegmentPair{point_id, info.airp_arv}, "INT");
    Opt<TPaxSegmentPair> ckinSeg = CheckIn::ckinSegment(info.grp_id);
    if(ckinSeg){
        ckinFlight.init(*ckinSeg, "CHK");
    }
    // �������� ���ଠ�� � ���ᠦ��
    auto tricky_gender = getTrickyGender(info.pers_type, gender);
    bool isCrew = false;
    if(reqType==PaxRequest::Pax) {
        TPaxStatus pax_status = DecodePaxStatus(status.c_str());
        isCrew = pax_status==psCrew;
    }
    TransferFlag trfer_flg;
    if(reqType==PaxRequest::Pax) {
        trfer_flg = getPaxTransferFlag(point_id, info.grp_id, info.airp_dep, info.airp_arv);
    } else {
        trfer_flg = getCrsTransferFlag(CheckInInterface::getCrsTransferMap(pax_id), point_id, info.airp_arv);
    }
    pax.init(pax_id, info.surname, info.name, isCrew, override_type, reg_no, tricky_gender, trfer_flg);
    return PaxRequest(trs, intFlight, ckinFlight, pax, reqType==Pax ? PaxAddData::createByPaxId(pax_id)
                                                                    : PaxAddData::createByCrsPaxId(pax_id));
}

Opt<PaxRequest> PaxRequest::createByCrsPaxId(const int pax_id, const std::string& override_type)
{
    ProgTrace(TRACE5, "TPaxRequest::createByCrsPaxId: %d", pax_id);
    MetaInfo info;
    int pr_del = 0, point_id = 0;
    auto cur = make_curs(
               "select SURNAME, NAME, POINT_ID_SPP, PR_DEL, AIRP_ARV "
               ", PERS_TYPE "
               "from CRS_PAX, CRS_PNR, TLG_BINDING "
               "where PAX_ID = :pax_id and CRS_PAX.PNR_ID = CRS_PNR.PNR_ID and "
               "      CRS_PNR.POINT_ID = TLG_BINDING.POINT_ID_TLG");
    cur
        .def(info.surname)
        .defNull(info.name,"")
        .def(point_id)
        .def(pr_del)
        .def(info.airp_arv)
        .def(info.pers_type)
        .bind(":pax_id", pax_id)
        .EXfet();
    if(cur.err() == NO_DATA_FOUND) {
        return boost::none;
    }
    auto point_info = getPointInfo(point_id);
    if(!point_info) {
        throw Exception("Not point info");
    }
    if(!isInternationalFlight(TPaxSegmentPair{point_id, info.airp_arv}))
    {
        return boost::none;
    }
    return createByFields(PaxRequest::Crs, pax_id, point_id, info, true, "", pr_del,
                          ASTRA::TGender::Unknown, override_type, point_info->airline);
}

Opt<PaxRequest> PaxRequest::createFromDB(RequestType type, const int pax_id,
                                         const std::string& override_type)
{
    switch (type) {
    case RequestType::Pax:
        return PaxRequest::createByPaxId(pax_id, override_type);
    case RequestType::Crs:
        return PaxRequest::createByCrsPaxId(pax_id, override_type);
    default:
        throw Exception("Invalid RequestType: %d ", static_cast<int>(type));
    }
}

TransferFlag getTransferFlagFromTransit(int point_id, const std::string& airp_arv)
{
    /* �஢�ਬ �࠭���. � ��砥 �࠭��� �१ ��࠭�-���⭨�� APPS, ���⠢��
     䫠� "transfer at destination". �� ��⨢���� ⮬�, �� ����ᠭ� �
     ᯥ�䨪�樨, ������ �� �஢������ ���䨪�樨 SITA ���ॡ����� ������
     ⠪ ��ࠡ��뢠�� �࠭���� ३��. */

    TAdvTripRoute route = getTransitRoute(TPaxSegmentPair(point_id, airp_arv));
    bool transit = isTransit(route);

    return transit ? TransferFlag::Dest : TransferFlag::None;
}

TransferFlag getCrsTransferFlag(const map<int, CheckIn::TTransferItem> & trfer, int point_id,
                                const std::string& airp_arv)
{
    LogTrace(TRACE5) << __FUNCTION__ << "point_id: " << point_id << " airp_arv: " << airp_arv;
    TransferFlag transfer = getTransferFlagFromTransit(point_id, airp_arv);
    if (!trfer.empty() && !trfer.at(1).airp_arv.empty()) {
        // ᪢����� ॣ������
        transfer = TransferFlag::Dest; // ��室�騩 �࠭���
    }
    return transfer;
}

TransferFlag getPaxTransferFlag(int point_id, int pax_grp_id, const std::string& airp_dep,
                       const std::string& airp_arv)
{
    LogTrace(TRACE5) << __FUNCTION__ << "point_id: " << point_id << " pax_grp_id: "
                     << pax_grp_id << " airp_dep: " << airp_dep << " airp_arv: " <<airp_arv;
    TransferFlag transfer = getTransferFlagFromTransit(point_id, airp_arv);
    if(isPointTransferDestination(pax_grp_id, airp_arv)) {
        transfer = TransferFlag::Dest;
    }

    if(isPointTransferOrigin(pax_grp_id, airp_dep)) {
        transfer = ((transfer == TransferFlag::Dest) ? TransferFlag::Both : TransferFlag::Origin);
    }
    return transfer;
}

bool isPointTransferDestination(const int pax_grp_id, const std::string& airp_arv)
{
    TCkinRouteItem next;
    bool findSeg = TCkinRoute().GetNextSeg(pax_grp_id, crtIgnoreDependent, next);
    if(!findSeg) {
        LogTrace(TRACE5) << "Not found next segment";
        return false;
    }
    if (!next.airp_arv.empty()) {
        std::string country_arv = getCountryByAirp(airp_arv).code;
        if (getCountryByAirp(next.airp_arv).code != country_arv) {
            return true;
        }
    }
    return false;
}

bool isPointTransferOrigin(const int pax_grp_id, const std::string& airp_dep)
{
    TCkinRouteItem prev;
    bool findSeg = TCkinRoute().GetPriorSeg(pax_grp_id, crtIgnoreDependent, prev);
    if(!findSeg) {
        LogTrace(TRACE5) << "Not found prev segment";
        return false;
    }
    if (!prev.airp_dep.empty()) {
        std::string country_dep = getCountryByAirp(airp_dep).code;
        if (getCountryByAirp(prev.airp_dep).code != country_dep) {
            return true;
        }
    }
    return false;
}

//std::map<int, CheckIn::TTransferItem> getCrsTransferMap(const int pax_id)
//{
//    LogTrace(TRACE5) << __FUNCTION__ << " pax_id: " << pax_id;
//    int pnr_id = 0, point_id = 0;
//    std::string airp_arv;
//    auto cur = make_curs(
//               "select POINT_ID_SPP, CRS_PAX.PNR_ID, AIRP_ARV "
//               ", PERS_TYPE "
//               "from CRS_PAX, CRS_PNR, TLG_BINDING "
//               "where PAX_ID = :pax_id and CRS_PAX.PNR_ID = CRS_PNR.PNR_ID and "
//               "      CRS_PNR.POINT_ID = TLG_BINDING.POINT_ID_TLG");
//    cur
//        .def(point_id)
//        .def(pnr_id)
//        .def(airp_arv)
//        .bind(":pax_id", pax_id)
//        .EXfet();
//    if(cur.err() == NO_DATA_FOUND) {
//        LogTrace(TRACE5) << __FUNCTION__ << " No data found";
//        return {};
//    }
//    map<int, CheckIn::TTransferItem> trfer;
//    CheckInInterface::GetOnwardCrsTransfer(pnr_id, true, *getPointInfo(point_id), airp_arv, trfer);
//    return trfer;
//}

std::vector<std::string> statusesFromDb(const int pax_id)
{
    std::vector<std::string> res;
    auto cur = make_curs(
               "select STATUS from APPS_PAX_DATA "
               "where PAX_ID = :pax_id");
    std::string status;
    cur
        .defNull(status,"").bind(":pax_id",pax_id).exec();

    while(!cur.fen()) {
        res.emplace_back(status);
    }
    return res;
}

bool PaxRequest::operator==(const PaxRequest &c) const
{
    return  trans == c.trans       &&
            ckin_flt == c.ckin_flt &&
            int_flt == c.int_flt   &&
            pax == c.pax;
}

std::string PaxRequest::msg(const int version) const
{
    std::ostringstream msg;
    msg << trans.msg(version) << "/";
    if (trans.code == "CIRQ" && ckin_flt.point_id != ASTRA::NoExists) {
        msg << ckin_flt.msg(version) << "/";
    }
    msg << int_flt.msg(version) << "/";
    msg << pax.msg(version) << "/";
    if(version >= APPS_VERSION_26 && trans.code == "CIRQ" && pax_add) {
        msg << pax_add->msg(version) << "/";
    }
    return std::string(trans.header + "\x02" + msg.str() + "\x03");
}

void updateAppsCicxMsgId(int msg_id, const std::string& apps_pax_id)
{
    auto cur = make_curs(
               "update APPS_PAX_DATA set CICX_MSG_ID = :cicx_msg_id "
               "where APPS_PAX_ID = :apps_pax_id");
    cur
        .bind(":cicx_msg_id",msg_id)
        .bind(":apps_pax_id",apps_pax_id)
        .exec();
}

void PaxRequest::save(const int version) const
{
    if(trans.code == "CICX") {
        updateAppsCicxMsgId(trans.msg_id, pax.apps_pax_id);
        return;
    }
    short null = -1, nnull = 0;
    auto cur = make_curs(
               "INSERT INTO apps_pax_data "
               "(pax_id, cirq_msg_id, pax_crew, nationality, issuing_state, "
               " passport, check_char, doc_type, doc_subtype, expiry_date, sup_check_char, "
               " sup_doc_type, sup_passport, family_name, given_names, "
               " date_of_birth, sex, birth_country, is_endorsee, transfer_at_orgn, "
               " transfer_at_dest, pnr_source, pnr_locator, send_time, pre_ckin, "
               " flt_num, dep_port, dep_date, arv_port, arv_date, ckin_flt_num, ckin_port, "
               " point_id, ckin_point_id, pass_ref, version, "
               " country_for_data, doco_type, doco_no, country_issuance, doco_expiry_date, "
               " num_street, city, state, postal_code, redress_number, traveller_number) "
               "VALUES (:pax_id, :cirq_msg_id, :pax_crew, :nationality, :issuing_state, :passport, "
               "        :check_char, :doc_type, :doc_subtype, :expiry_date, :sup_check_char, :sup_doc_type, :sup_passport, "
               "        :family_name, :given_names, :date_of_birth, :sex, :birth_country, :is_endorsee, "
               "        :transfer_at_orgn, :transfer_at_dest, :pnr_source, :pnr_locator, :send_time, :pre_ckin, "
               "        :flt_num, :dep_port, :dep_date, :arv_port, :arv_date, :ckin_flt_num, :ckin_port, "
               "        :point_id, :ckin_point_id, :pass_ref, :version, "
               "        :country_for_data, :doco_type, :doco_no, :country_issuance, :doco_expiry_date, "
               "        :num_street, :city, :state, :postal_code, :redress_number, :traveller_number)");
    cur
        .bind(":cirq_msg_id",      trans.msg_id)
        .bind(":pax_id",           pax.pax_id)
        .bind(":pax_crew",         pax.pax_crew)
        .bind(":nationality",      pax.nationality)
        .bind(":issuing_state",    pax.issuing_state)
        .bind(":passport",         pax.passport)
        .bind(":check_char",       pax.check_char)
        .bind(":doc_type",         pax.doc_type)
        .bind(":doc_subtype",      pax.doc_subtype)
        .bind(":expiry_date",      pax.expiry_date)
        .bind(":sup_check_char",   pax.sup_check_char)
        .bind(":sup_doc_type",     pax.sup_doc_type)
        .bind(":sup_passport",     pax.sup_passport)
        .bind(":family_name",      pax.family_name)
        .bind(":given_names",      pax.given_names)
        .bind(":date_of_birth",    pax.birth_date)
        .bind(":sex",              pax.sex)
        .bind(":birth_country",    pax.birth_country)
        .bind(":is_endorsee",      pax.endorsee)
        .bind(":transfer_at_orgn", pax.trfer_at_origin)
        .bind(":transfer_at_dest", pax.trfer_at_dest)
        .bind(":pnr_source",       pax.pnr_source)
        .bind(":pnr_locator",      pax.pnr_locator)
        .bind(":send_time",        Dates::second_clock::universal_time())
        .bind(":pre_ckin",         trans.type)
        .bind(":flt_num",          int_flt.flt_num)
        .bind(":dep_port",         int_flt.dep_port)
        .bind(":dep_date"    ,     int_flt.dep_date)
        .bind(":arv_port"    ,     int_flt.arv_port)
        .bind(":arv_date"    ,     int_flt.arv_date)
        .bind(":ckin_flt_num",     ckin_flt.flt_num)
        .bind(":ckin_port"   ,     ckin_flt.dep_port)
        .bind(":point_id"    ,     int_flt.point_id)
        .bind(":ckin_point_id",    ckin_flt.point_id, ckin_flt.point_id != ASTRA::NoExists ? &nnull : &null)
        .bind(":pass_ref",         pax.reference)
        .bind(":version",          version)
        .bind(":country_for_data", pax_add ? pax_add->country_for_data : "")
        .bind(":doco_type",        pax_add ? pax_add->doco_type : "")
        .bind(":doco_no",          pax_add ? pax_add->doco_no : "")
        .bind(":country_issuance", pax_add ? pax_add->country_issuance : "")
        .bind(":doco_expiry_date", pax_add ? pax_add->doco_expiry_date : "")
        .bind(":num_street",       pax_add ? pax_add->num_street : "")
        .bind(":city",             pax_add ? pax_add->city : "")
        .bind(":state",            pax_add ? pax_add->state : "")
        .bind(":postal_code",      pax_add ? pax_add->postal_code : "")
        .bind(":redress_number",   pax_add ? pax_add->redress_number : "")
        .bind(":traveller_number", pax_add ? pax_add->traveller_number : "")
        .exec();
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

bool ManifestRequest::init(const TPaxSegmentPair& flt, const std::string& country_lat)
{  
    Opt<TTripInfo> point_info = getPointInfo(flt.point_dep);
    if(!point_info) {
        return false;
    }
    trans.init("CIMR", point_info->airline).setPreCheckin(false);
    int_flt.init(flt, "INM");
    mft_req.init(country_lat);
    return true;
}

std::string ManifestRequest::msg(const int version) const
{
    std::string msg  = trans.msg(version)   + "/"
                     + int_flt.msg(version) + "/"
                     + mft_req.msg(version) + "/";
    return std::string(trans.header + "\x02" + msg + "\x03");
}

//void ManifestRequest::sendReqAndSave(const AppsSettings &settings) const
//{
//    sendNewReq(msg(settings.version()), settings.getRouter());
//    saveAppsMsg(msg(settings.version()), trans.msg_id, int_flt.point_id, settings.version());
//}

static int getInt(const std::string& val)
{
    if (val.empty()) {
        return ASTRA::NoExists;
    }
    return ToInt(val);
}

void AnsPaxData::init(std::string source, int ver)
{
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
        int i = 22; // ��砫쭠� ������ ����� (���� 23)
        if (version < APPS_VERSION_27) --i;
        if (version < APPS_VERSION_26) --i;
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
        int i = 20; // ��砫쭠� ������ ����� (���� 21)
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

void APPSAnswer::getLogParams(LEvntPrms& params, const std::string& country, const int status_code,
                             const int error_code, const std::string& error_text) const
{
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
        // �㦭� ���-� ࠧ����� Timeout � Error condition. ��� ����� status_code 0000
        if (status_code == 0)
            params << PrmLexema("result", (error_code != ASTRA::NoExists)?
                                     "MSG.APPS_STATUS_1111":"MSG.APPS_STATUS_2222");
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
}

bool APPSAnswer::init(const std::string& trans_type, const std::string& source)
{
    vector<std::string> tmp;
    boost::split(tmp, source, boost::is_any_of("/"));
    vector<std::string>::const_iterator it = tmp.begin();
    code = trans_type;
    msg_id = getInt(*(it++));

    auto cur = make_curs(
                "SELECT send_attempts, msg_text, point_id, version "
                "FROM apps_messages "
                "WHERE msg_id = :msg_id");
    cur
       .def(send_attempts)
       .def(msg_text)
       .def(point_id)
       .def(version)
       .bind(":msg_id",msg_id)
       .EXfet();
    if(cur.err() == NO_DATA_FOUND) {
        return false;
    }

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
    return true;
}

std::string APPSAnswer::toString() const
{
    std::ostringstream res;
    res << "code: " << code << std::endl << "msg_id: " << msg_id << std::endl
        << "point_id: " << point_id << std::endl << "msg_text: " << msg_text << std::endl
        << "send_attempts: " << send_attempts << std::endl;
    for (const auto & err: errors) {
        res << "country: " << err.country << std::endl << "error_code: " << err.error_code
            << std::endl << "error_text: " << err.error_text << std::endl;
    }
    return res.str();
}

void APPSAnswer::beforeProcessAnswer() const
{
    TFlights flightsForLock;
    flightsForLock.Get(point_id, ftTranzit);
    flightsForLock.Lock(__FUNCTION__);

    // �몫�稬 �ॢ��� "��� �裡 � APPS"
    set_alarm(point_id, Alarm::APPSOutage, false);
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
    // �᫨ �㦭� ��ࠢ��� ����୮, �� 㤠�塞 ᮮ�饭�� �� apps_messages, �⮡� resend_tlg ��� ��襫
    if (!need_resend) {
        deleteMsg(msg_id);
    }
    return need_resend;
}

bool PaxReqAnswer::init(const std::string& code, const std::string& source)
{
    if(!APPSAnswer::init(code, source)) {
        return false;
    }
    // ����⠥��� ���� ���ᠦ�� �।� ��ࠢ������

    std::ostringstream sql;
    sql << "select PAX_ID, VERSION, FAMILY_NAME "
           "from APPS_PAX_DATA ";
    if (code == "CIRS") {
        sql << "where CIRQ_MSG_ID = :msg_id";
    } else {
        sql << "where CICX_MSG_ID = :msg_id";
    }
    auto cur = make_curs(sql.str());
    cur
       .def(pax_id)
       .def(version)
       .def(family_name)
       .bind(":msg_id",msg_id)
       .EXfet();
    if(cur.err() == NO_DATA_FOUND) {
        return false;
    }

    std::string delim = (code == "CIRS") ? "PRS" : "PCC";
    delim = std::string("/") + delim + std::string("/");
    std::size_t pos1 = source.find(delim);
    while (pos1 != std::string::npos) {
        std::size_t pos2 = source.find(delim, pos1 + delim.size());
        AnsPaxData data;
        data.init(source.substr(pos1 + 1, pos2 - pos1), version);
        passengers.push_back(data);
        pos1 = pos2;
    }
    return true;
}

std::string PaxReqAnswer::toString() const
{
    std::ostringstream res;
    res << APPSAnswer::toString()
        << "pax_id: " << pax_id << std::endl << "family_name: " << family_name << std::endl;
    for (const auto & pax : passengers) {
        res << pax.toString();
    }
    return res.str();
}

void PaxReqAnswer::processErrors() const
{
    if(errors.empty()) {
        LogTrace(TRACE5)<<__FUNCTION__<< " errors empty";
        return;
    }

    // ����饭�� �� ��ࠡ�⠭� ��⥬��. �����६ �訡��.
    for(const auto & err : errors) {
        logAnswer(err.country, ASTRA::NoExists, err.error_code, err.error_text);
    }

    if(/*!CheckIfNeedResend() &&*/ code == "CIRS") {
        LogTrace(TRACE5)<<__FUNCTION__<<" if code = CIRS";
        addAPPSAlarm(pax_id, {Alarm::APPSConflict}); // ��ᨭ�஭�����
        // 㤠�塞 apps_pax_data cirq_msg_id
        deleteAPPSData("CIRQ_MSG_ID", msg_id);
        deleteMsg(msg_id);
    }
}

void saveAppsStatus(const std::string& status, const std::string& apps_pax_id, const int msg_id)
{
    auto cur = make_curs(
               "update APPS_PAX_DATA set STATUS = :status, APPS_PAX_ID = :apps_pax_id "
               "where CIRQ_MSG_ID = :cirq_msg_id");
    cur.bind(":status",status)
       .bind(":apps_pax_id",apps_pax_id)
       .bind(":cirq_msg_id", msg_id)
       .exec();
}

void PaxReqAnswer::processAnswer() const
{
    if (passengers.empty()) {
        return;
    }

    // ����饭�� ��ࠡ�⠭� ��⥬��. ������� ���ᠦ��� ��᢮�� �����.
    bool result = true;
    std::string status = "B";
    std::string apps_pax_id;
    for (const auto& pax : passengers)
    {
        // ����襬 � ��ୠ� ����権 ��᫠��� �����
        logAnswer(pax.country, pax.code, pax.error_code1, pax.error_text1);
        if (pax.error_code2 != ASTRA::NoExists) {
            logAnswer(pax.country, pax.code, pax.error_code2, pax.error_text2);
        }
        if (pax.error_code3 != ASTRA::NoExists) {
            logAnswer(pax.country, pax.code, pax.error_code3, pax.error_text3);
        }
        if (code == "CICC") {
            // �⢥� �� �⬥��
            // ��।���� ����� ���ᠦ��. true - �⬥��� ��� �� ������, false - �訡��.
            if(pax.status == "E" || pax.status == "T") {
                result = false;
            }
            continue;
        }

        // �⢥� �� ����� �����
        if (&pax != &passengers[0]) {
            LogTrace(TRACE5)<<__FUNCTION__<<" �⢥� �� ����� �����";
            if (apps_pax_id != pax.apps_pax_id)
                throw Exception("apps_pax_id is not the same");
        } else {
            apps_pax_id = pax.apps_pax_id;
        }
        // ����稬 �ॢ���
        if (pax.status == "D" || pax.status == "X") {
            addAPPSAlarm(pax_id, {Alarm::APPSNegativeDirective});
            LogTrace(TRACE5)<< __FUNCTION__<< " ����稬 �ॢ��� �����=D||X";
        }
        else if (pax.status == "U" || pax.status == "I" || pax.status == "T" || pax.status == "E") {
            addAPPSAlarm(pax_id, {Alarm::APPSError});
            LogTrace(TRACE5)<<__FUNCTION__<<" ����稬 �ॢ��� �����=U||I||T||E";
        }
        /* ������ ��࠭� ���⭨� APPS ���뫠�� ᢮� ����� ���ᠦ��, �.�.
     * ���� ���ᠦ�� ����� ����� ��᪮�쪮 ����ᮢ. ����稬 �� ������� ���ᠦ���
     * ����饭�� ����� ��� ����� � apps_pax_data. �᫨ �� "B" -> "B", �᫨ ��� ��
     * ���� "X" -> "X", �᫨ ���� ���� "U","I","T" ��� "E" -> "P" (Problem) */
        if (status == "B" && pax.status != "B") {
            status = (pax.status == "X") ? pax.status : "P";
        }
        else if (status == "P" && pax.status == "X") {
            status = "X";
        }
    }

    if (code == "CICC") {
        if(result) {
            // ���ᠦ�� �⬥���. ������ ��� �� apps_pax_data
            deleteAPPSData("CICX_MSG_ID", msg_id);
        }
        deleteMsg(msg_id);
        return;
    }
    // ��࠭�� ����祭�� �����
    saveAppsStatus(status, apps_pax_id, msg_id);

    // ����ᨬ �ॢ���
    if (status == "B") {
        LogTrace(TRACE5)<<__FUNCTION__<<" ����ᨬ �ॢ���";
        deleteAPPSAlarm(pax_id, {Alarm::APPSNegativeDirective, Alarm::APPSError});
    }
    // �஢�ਬ, �㦭� �� ����� �ॢ��� "��ᨭ�஭�����"
    Opt<PaxRequest> actual;
    Opt<PaxRequest> received;
    actual = isAlreadyCheckedIn(pax_id) ? PaxRequest::createFromDB(PaxRequest::Pax, pax_id)
                                        : PaxRequest::createFromDB(PaxRequest::Crs, pax_id);
    if(!actual) {
        ProgTrace(TRACE5, "Pax has not been found");
    }
    else {
        LogTrace(TRACE5)<<__FUNCTION__<<" Pax found";
        received = PaxRequest::fromDBByMsgId(msg_id);
    }
    if (!actual || received == actual) {
        deleteAPPSAlarm(pax_id, {Alarm::APPSConflict});
    }
    deleteMsg(msg_id);
}

std::set<std::string> needFltCloseout(const std::set<std::string>& countries, const std::string& airline)
{
    std::set<std::string> countries_need_req;
    if(countries.size() > 1)
    {
        for (const auto &country : countries)
        {
            Opt<AppsSettings> sets = AppsSettings::readSettings(airline, country);
            if(sets && sets->getFltCloseout()) {
                countries_need_req.insert(country);
            }
        }
    }
    return countries_need_req;
}

void appsFlightCloseout(const int point_id)
{
    ProgTrace(TRACE5, "appsFlightCloseout: point_id = %d", point_id);
    if (!checkTime(point_id)) {
        return;
    }

    TAdvTripRoute route;
    route.GetRouteAfter(NoExists, point_id, trtWithCurrent, trtNotCancelled);
    if (route.empty()) {
        return;
    }
    TAdvTripRoute::const_iterator r = route.begin();
    std::set<std::string> countries;
    countries.insert(getCountryByAirp(r->airp).code);
    for(r++; r!=route.end(); r++)
    {
        // ��।����, �㦭� �� ��ࠢ���� �����
        if(!checkAPPSSets(point_id, r->point_id)) {
            continue;
        }
        string country = getCountryByAirp(r->airp).code;
        countries.insert(country);
        Opt<AppsSettings> settings = AppsSettings::readSettings(route.front().airline_out, country);
        if(!settings) {
            continue;
        }
        /* ��।����, ���� �� ���ᠦ��� �� ��襤訥 ��ᠤ��,
     * ���ଠ�� � ������ �뫠 ��ࠢ���� � SITA.
     * ��� ⠪�� ���ᠦ�஢ �㦭� ��᫠�� �⬥�� */
        auto cur = make_curs(
                   "SELECT cirq_msg_id, pax_grp.status "
                   "FROM pax_grp, pax, apps_pax_data "
                   "WHERE pax_grp.grp_id=pax.grp_id AND apps_pax_data.pax_id = pax.pax_id AND "
                   "pax_grp.point_dep=:point_dep AND pax_grp.point_arv=:point_arv AND "
                   "(pax.name IS NULL OR pax.name<>'CBBG') AND "
                   "pr_brd=0 AND apps_pax_data.status = 'B'  and pax_crew != 'C'");
        int cirq_msg_id;
        std::string pax_grp_status;
        cur
            .def(cirq_msg_id)
            .def(pax_grp_status)
            .bind(":point_dep",point_id)
            .bind(":point_arv",r->point_id)
            .exec();
        while(!cur.fen()) {
            Opt<PaxRequest> pax = PaxRequest::fromDBByMsgId(cirq_msg_id);
            if(pax && pax->getStatus() != "B") {
                continue; // CICX request has already been send
            }
            APPSMessage msg(*settings, make_unique<PaxRequest>(pax.get()));
            msg.send();
            msg.save();
        }
    }
    for(const auto& country : needFltCloseout(countries, route.front().airline_out))
    {
        std::string country_lat = BaseTables::Country(country)->lcode();
        ManifestRequest close_flt;
        Opt<AppsSettings> settings = AppsSettings::readSettings(route.front().airline_out, country);
        if (close_flt.init(TPaxSegmentPair{point_id, route.back().airp}, country_lat) && settings) {
            APPSMessage msg(*settings, make_unique<ManifestRequest>(close_flt));
            msg.send();
            msg.save();
        }
    }
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

void reSendMsg(const int send_attempts, const std::string& msg_text, const int msg_id)
{
    sendTlg(getAPPSRotName(), OWN_CANON_NAME(), qpOutApp, 20, msg_text,
             ASTRA::NoExists, ASTRA::NoExists);

    //  sendCmd("CMD_APPS_ANSWER_EMUL","H");
    sendCmd("CMD_APPS_HANDLER","H");

    auto cur = make_curs(
               "update APPS_MESSAGES "
               "set SEND_ATTEMPTS = :send_attempts, SEND_TIME = :send_time "
               "where MSG_ID = :msg_id ");
    cur
        .bind(":send_attempts", send_attempts+1)
        .bind(":send_time", Dates::second_clock::universal_time())
        .bind(":msg_id", msg_id)
        .exec();
    ProgTrace(TRACE5, "Message id=%d was re-sent. Send attempts: %d", msg_id, send_attempts);
}

void deleteMsg(const int msg_id)
{
    auto cur = make_curs(
               "delete from APPS_MESSAGES where MSG_ID = :msg_id ");
    cur
        .bind(":msg_id", msg_id)
        .exec();
}

void sendAPPSInfo(const int point_id, const int point_id_tlg)
{
    ProgTrace(TRACE5, "sendAPPSInfo: point_id %d, point_id_tlg: %d", point_id, point_id_tlg);
    auto cur = make_curs(
               "select PAX_ID, AIRP_ARV from CRS_PNR, CRS_PAX "
               "where CRS_PNR.PNR_ID=CRS_PAX.PNR_ID and POINT_ID=:point_id_tlg");
    std::string airp_arv;
    int pax_id;
    cur.def(pax_id).def(airp_arv).bind(":point_id_tlg", point_id_tlg).exec();

    TFlights flightsForLock;
    flightsForLock.Get(point_id, ftTranzit);
    flightsForLock.Lock(__FUNCTION__);

    while(!cur.fen()) {
         if (checkAPPSSets(point_id, airp_arv)) {
             processCrsPax(pax_id);
         }
    }
}

void sendAllAPPSInfo(const TTripTaskKey &task)
{
    LogTrace(TRACE5)<<__FUNCTION__;
    auto cur = make_curs(
               "select POINT_ID_TLG "
               "from TLG_BINDING, POINTS "
               "where TLG_BINDING.POINT_ID_SPP=POINTS.POINT_ID and "
               "   POINT_ID_SPP=:point_id_spp and "
               "   POINTS.PR_DEL=0 AND POINTS.PR_REG<>0");
    int point_id_tlg;
    cur.def(point_id_tlg).bind(":point_id_spp", task.point_id).exec();
    while(!cur.fen()){
        sendAPPSInfo(task.point_id, point_id_tlg);
    }
}

void sendNewAPPSInfo(const TTripTaskKey &task)
{
    auto cur = make_curs(
               "select PAX_ID "
               "from CRS_PAX, CRS_PNR, TLG_BINDING "
               "where NEED_APPS <> 0 and POINT_ID_SPP = :point_id_spp and "
               "   CRS_PAX.PNR_ID = CRS_PNR.PNR_ID and "
               "   CRS_PNR.POINT_ID = TLG_BINDING.POINT_ID_TLG");
    int pax_id;
    cur.def(pax_id).bind(":point_id_spp", task.point_id).exec();

    TFlights flightsForLock;
    flightsForLock.Get(task.point_id, ftTranzit);
    flightsForLock.Lock(__FUNCTION__);

    while(!cur.fen()){
        processCrsPax(pax_id);
        updateCrsNeedApps(pax_id);
    }
}

void updateCrsNeedApps(int pax_id)
{
    auto cache_cur = make_curs(
                     "UPDATE crs_pax SET need_apps=0 WHERE pax_id=:val");
    cache_cur.bind(":val",pax_id);
    cache_cur.exec();
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

void PaxReqAnswer::logAnswer(const std::string& country, const int status_code,
                             const int error_code, const std::string& error_text) const
{
    LEvntPrms params;
    getLogParams(params, country, status_code, error_code, error_text);
    PrmLexema lexema("passenger", "MSG.APPS_PASSENGER");
    lexema.prms << PrmSmpl<std::string>("passenger", family_name);
    params << lexema;

    CheckIn::TSimplePaxItem pax;
    if (pax.getByPaxId(pax_id)) {
        TReqInfo::Instance()->LocaleToLog("MSG.APPS_RESP", params, evtPax, point_id, pax.reg_no);
    }
}

void ManifestAnswer::logAnswer(const std::string& country, const int status_code,
                               const int error_code, const std::string& error_text) const
{
    LEvntPrms params;
    getLogParams(params, country, status_code, error_code, error_text);
    params << PrmSmpl<std::string>("passenger", "");
    TReqInfo::Instance()->LocaleToLog("MSG.APPS_RESP", params, evtFlt, point_id);
}

bool ManifestAnswer::init(const std::string& code, const std::string& source)
{
    if(!APPSAnswer::init(code, source)) {
        return false;
    }
    size_t pos = source.find("MAK");
    if (pos != string::npos)
    {
        vector<std::string> tmp;
        std::string text = source.substr(pos);
        boost::split(tmp, text, boost::is_any_of("/"));
        int fld_count = getInt(tmp[1]); /* 2 */
        if (fld_count != fieldCount("MAK", version)) {
            throw Exception("Incorrect fld_count: %d", fld_count);
        }
        country = tmp[2]; /* 3 */
        resp_code = getInt(tmp[3]); /* 4 */
        error_code = getInt(tmp[4]); /* 5 */
        error_text = tmp[5]; /* 6 */
    }
    return true;
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
    deleteMsg(msg_id);
}

void ManifestAnswer::processAnswer() const
{
    if(resp_code == ASTRA::NoExists) {
        return;
    }
    // ����饭�� ��ࠡ�⠭� ��⥬��. �������㥬 �⢥� � 㤠��� ᮮ�饭�� �� apps_messages.
    logAnswer(country, resp_code, error_code, error_text);
    deleteMsg(msg_id);
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

bool processReply(const std::string& source_raw)
{
    LogTrace(TRACE5)<<__FUNCTION__ << " source_raw: " << source_raw;
    try
    {
        std::string source = getAnsText(source_raw);
        if(source.empty()) {
            throw Exception("Answer is empty");
        }
        std::string code = source.substr(0, 4);
        std::string answer = source.substr(5, source.size() - 6); // ��१��� ��� �࠭���樨 � ���몠�騩 '/' (XXXX:text_to_parse/)
        boost::scoped_ptr<APPSAnswer> res;
        if (code == "CIRS" || code == "CICC") {
            res.reset(new PaxReqAnswer());
        }
        else if (code == "CIMA") {
            res.reset(new ManifestAnswer());
        }
        else {
            throw Exception(std::string("Unknown transaction code: " + code));
        }

        if (!res->init(code, answer)) {
            return false;
        }
        ProgTrace(TRACE5, "Result: %s", res->toString().c_str());
        res->beforeProcessAnswer();
        res->processErrors();
        res->processAnswer();
        return true;
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

} //namespace APPS
