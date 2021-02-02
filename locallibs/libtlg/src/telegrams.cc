#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
//  implementation requires it
#include <serverlib/cursctl.h>
#include <serverlib/posthooks.h>
#include <serverlib/daemon_kicker.h>
#include <serverlib/dates_oci.h>
#include <serverlib/timer.h>
#include <serverlib/tcl_utils.h>
#include <serverlib/int_parameters_oci.h>
#include <serverlib/EdiHelpManager.h>
#include <serverlib/oci_selector_char.h>

#define NICKNAME "NONSTOP"
#define NICKTRACE NONSTOP_TRACE
#include <serverlib/test.h>
#include <serverlib/slogger.h>
#include <serverlib/rip_oci.h>

#include "telegrams.h"
#include "express.h"
#include "hth.h"
#include "types.h"
#include "gateway.h"


static bool tlgnumFillLeadingZeroes()
{
    static const bool res = readIntFromTcl("TLGNUM_FILL_LEADING_ZEROES", 0);
    return res;
}

static tlgnum_t::num_t initTlgNum(const std::string& str)
{
    tlgnum_t::num_t num(str);
    if (!tlgnumFillLeadingZeroes())
        return num;

    std::string withLeadingZeros = str;
    withLeadingZeros.insert(0, telegrams::TLG_NUM_LENGTH - num.get().size(), '0');
    return tlgnum_t::num_t(withLeadingZeros);
}

boost::optional<tlgnum_t> tlgnum_t::create(const std::string& v, bool express)
{
    if (!num_t::validate(v))
        return boost::optional<tlgnum_t>();

    return tlgnum_t(v, express);
}

tlgnum_t::tlgnum_t(const std::string& v, bool express)
    : num(initTlgNum(v)), express(express)
{
}

namespace telegrams
{
using hth::HthInfo;


std::ostream& operator<< (std::ostream& os, const RouterInfo& ri)
{
    os << "RouterInfo: ida=" << ri.ida
        << " canonName=" << ri.canonName
        << " senderName=" << ri.senderName
        << " h2h=" << ri.hth
        << " typeB=" << ri.tpb
        << " true_tpb=" << ri.tpb
        << " translit=" << ri.translit
        << " censor=" << ri.censor
        << " max_part_size=" << ri.max_part_size
        << " max_h2h_part_size=" << ri.max_hth_part_size
        << " max_typeb_part_size=" << ri.max_typeb_part_size
        << " block_answers=" << ri.blockAnswers
        << " block_requests=" << ri.blockRequests
        << " address=" << ri.address
        << " port=" << ri.port;

    return os;
}

RouterInfo::RouterInfo()
    : ida(0), hth(false), tpb(false), true_tpb(false), translit(false), censor(false),
    blockAnswers(false), blockRequests(false),
    max_part_size(RouterInfo::defaultMaxPartSize),
    max_hth_part_size(RouterInfo::defaultMaxHthPartSize),
    max_typeb_part_size(RouterInfo::defaultMaxTypeBPartSize),
    port(0)
{}

size_t RouterInfo::defaultMaxPartSize = MAX_TLG_SIZE;
size_t RouterInfo::defaultMaxHthPartSize = 4096;
size_t RouterInfo::defaultMaxTypeBPartSize = 3072;

bool operator<(const int lhs, const RouterInfo& rhs)
{
    return lhs < rhs.ida;
}

bool operator<(const RouterInfo& lhs, const int rhs)
{
    return lhs.ida < rhs;
}

TlgCallbacksAbstractDb::TlgCallbacksAbstractDb()
        : m_defaultHandler(1)
{
    m_defaultHandler = readIntFromTcl("DEF_TLG_OBRZAP", 1);
}

bool TlgCallbacks::saveBadTlg(const AIRSRV_MSG& tlg, int error)
{
    std::string err;
    switch (error) {
    case ROT_ERR:
        err = "Ž˜ˆŠ€ ’€‹ˆ–› ROT";
        break;
    case POINTS_ERR:
        err = "Ž˜ˆŠ€ ’€‹ˆ–› POINTS";
        break;
    case WRITE_HTH_LONG_ERR:
        err = "Ž˜ˆŠ€ ‡€ˆ‘ˆ ‘Ž€Ž‰ HTH ’‹ƒ";
        break;
    case WRITE_HTH_PART_ERR:
        err = "Ž˜ˆŠ€ ‡€ˆ‘ˆ —€‘’ˆ HTH ’‹ƒ";
        break;
    case WRITE_LONG_ERR:
        err = "Ž˜ˆŠ€ ‡€ˆ‘ˆ ‘Ž€Ž‰ HTH ’‹ƒ";
        break;
    case WRITE_PART_ERR:
        err = "Ž˜ˆŠ€ ‡€ˆ‘ˆ —€‘’ˆ ’‹ƒ";
        break;
    case TYPE_ERR:
        err = "…ˆ‡‚…‘’›‰ ’ˆ ’‹ƒ";
        break;
    case BAD_EDIFACT:
        err = "BAD_EDIFACT";
        break;
    default:
        err = "Ž˜ˆŠ€ ‡€ˆ‘ˆ ’‹ƒ";
    }

    try {
        Dates::ptime curr_tm = Dates::currentDateTime();
        make_curs("insert into text_tlg_bad(rem_num, type, ttl, sender, text, date1, errtext) "
            "values (:num, :tp, :ttl, :sndr, :body, :curr_tm, :err)")
            .bind(":num", tlg.num).bind(":tp", tlg.type).bind(":ttl", tlg.TTL)
            .bind(":sndr", tlg.Sender).bind(":err", err)
            .bind(":curr_tm", OciCpp::to_oracle_datetime(curr_tm))
            .bindFull(":body", const_cast<char*>(tlg.body), strlen(tlg.body), 0, 0, SQLT_LNG)
            .exec();
        return true;
    } catch (const OciCpp::ociexception& e) {
        ProgError(STDLOG, "SqlErr while save_bad_tlg() %s", e.what());
        return false;
    }
}

tlgnum_t TlgCallbacks::nextExpressNum()
{
    tlgnum_t::num_t::base_type tlgNum;
    HelpCpp::Timer tm("TlgCallbacks::nextExpressNum");
    OciCpp::CursCtl cr = make_curs("SELECT NUMXPRTLG_SEQ.NEXTVAL FROM DUAL");
    cr.def(tlgNum);
    cr.EXfet();
    LogTrace(TRACE1) << tm;
    return tlgnum_t(tlgNum, true);
}

boost::optional<tlgnum_t> TlgCallbacks::nextNum()
{
    HelpCpp::Timer tm("TlgCallbacks::nextNum");
    OciCpp::CursCtl cr = make_curs("SELECT NUMTLG_SEQ.NEXTVAL FROM DUAL");
    std::string num;
    cr.def(num);
    cr.EXfet();
    LogTrace(TRACE1) << tm;
    return tlgnum_t(num);
}

static bool validHth(const HthInfo& hth)
{
    if (!hth.type)
        return false;
    if (!hth.qri5)
        return false;
    return true;
}

Expected<TlgResult, int> TlgCallbacks::putTlg(const std::string& body,
        tlg_text_filter filter,
        int from_addr, int to_addr, hth::HthInfo *hth)
{
    boost::optional<tlgnum_t> nextNum = telegrams::callbacks()->nextNum();
    if (!nextNum) {
        ProgTrace(TRACE5, "nextNum failed");
        return -1;
    }

    tlgnum_t tlgNum = *nextNum;

    int part;
    const size_t MAX_FRAG_TLG_LEN = 2000;
    std::string curPart;

    const std::string tlgText = (filter.empty() ? body : filter(tlgNum, body));
    const size_t tlgLength = tlgText.size();
    if (tlgLength <= MAX_FRAG_TLG_LEN) {
        part = 0;
        curPart = tlgText;
    } else {
        part = 1;
        curPart.assign(tlgText, 0, MAX_FRAG_TLG_LEN);
    }

    try {
        ProgTrace(TRACE5, "tlgText=[%s]", tlgText.c_str());
        LogTrace(TRACE5) << "inserting tlg with tlgNum=" << tlgNum << " size=" << tlgText.size();
        Dates::ptime curr_tm = Dates::currentDateTime();
        make_curs(
                "INSERT INTO text_tlg_new(msg_id, text, date1, multy_part, from_addr, to_addr, instance) "
                "VALUES(:tlgNum, :cur_frag, :curr_tm, :multi, :from_addr, :to_addr, :instance)")
            .bind("tlgNum", tlgNum.num).bind(":cur_frag", curPart).bind(":multi", part)
            .bind(":from_addr", from_addr)
            .bind(":to_addr", to_addr)
            .bind(":curr_tm", OciCpp::to_oracle_datetime(curr_tm))
            .bind(":instance", ServerFramework::EdiHelpManager::instanceName())
            .exec();

        if (hth) {
            if (!validHth(*hth)) {
                hth::trace(TRACE1, *hth);
                ProgError(STDLOG, "invalid HthInfo");
                return -1;
            }

            int ret = writeHthInfo(tlgNum, *hth);
            if (ret) {
                LogError(STDLOG) << tlgNum << ": writeHthInfo failed";
                return -1;
            }
        }

        if (!part)
            return TlgResult(tlgNum);

        size_t pos = MAX_FRAG_TLG_LEN;
        do {
            part++;
            curPart.assign(tlgText, pos, MAX_FRAG_TLG_LEN);

            make_curs("INSERT INTO text_tlg_part(msg_id, text, part) VALUES(:tlgNum, :cur_frag, :frag_part)")
                .bind(":tlgNum", tlgNum.num).bind(":cur_frag", curPart).bind(":frag_part", part)
                .exec();
            pos += MAX_FRAG_TLG_LEN;
        } while (pos < tlgLength);
        LogTrace(TRACE5) << "Tlg " << tlgNum << " fragmented in " << part << " parts";
        return TlgResult(tlgNum);
    } catch (const OciCpp::ociexception& e) {
        LogError(STDLOG) << tlgNum << " putTlg filed: " << e.what();
        return -1;
    }
}

Expected<TlgResult, int> TlgCallbacksAbstractDb::writeTlg(INCOMING_INFO* ii, const char* body, bool kickAirimp)
{
    int router = 0;
    int handlerNum = 0;
    int ttlval = 0;

    if (!body || !strlen(body)) {
        ProgError(STDLOG, "writeTlg: body null-pointer or empty");
        return -1;
    }

    if (ii) {
        handlerNum = ii->q_num;
        ttlval = ii->ttl;

        if (!handlerNum)
            ProgError(STDLOG, "q_num in INCOMING_INFO is 0");
        router = ii->router;
    } else {
        handlerNum = m_defaultHandler;
        router = 0;
    }

    hth::HthInfo *hth = 0;
    if(ii && ii->hthInfo)
        hth = &ii->hthInfo.get();
    Expected<TlgResult, int> result = putTlg(body, tlg_text_filter(), router, 0, hth);
    if (!result) {
        ProgError(STDLOG, "SqlErr in putTlg()");
        return -1;
    }

    LogTrace(TRACE0) << "inserting tlg for handling: number[" << result->tlgNum << "], handler[" << handlerNum << "], router[" << router << "]";
    handlerQueue().addTlg(result->tlgNum, handlerNum, ttlval);
    if (!kickAirimp)
        return result;

    registerHandlerHook(handlerNum);
    return result;
}

int TlgCallbacks::writeHthInfo(const tlgnum_t& msgId, const hth::HthInfo& hthInfo)
{
    LogTrace(TRACE5) << hthInfo;
    try {
        Dates::ptime curr_tm = Dates::currentDateTime();
        make_curs(
                "INSERT INTO text_tlg_h2h (msg_id, type, rcvr, sndr, tpr, qri5, qri6,"
                " part, end, rem_addr_num, err, timestamp) "
                "VALUES (:id1, :hth_type, :hth_rcvr, :hth_sndr, :hth_tpr, :hth_qri5, :hth_qri6,"
                " :hth_part, :hth_end, :rem_addr_num, :hth_err, :curr_tm)")
            .bind(":id1", msgId.num).bind(":hth_type", std::string(1, hthInfo.type))
            .bind(":hth_rcvr", hthInfo.receiver).bind(":hth_sndr", hthInfo.sender)
            .bind(":hth_tpr", hthInfo.tpr)
            .bind(":hth_part", std::string(1, hthInfo.part))
            .bind(":hth_end", (int)hthInfo.end)
            .bind(":hth_qri5", std::string(1, hthInfo.qri5))
            .bind(":hth_qri6", std::string(1, hthInfo.qri6))
            .bind(":rem_addr_num", std::string(1, hthInfo.remAddrNum))
            .bind(":hth_err", hthInfo.why)
            .bind(":curr_tm", OciCpp::to_oracle_datetime(curr_tm))
            .exec();
    } catch (const OciCpp::ociexception& e) {
        LogTrace(TRACE0) << hthInfo;
        LogError(STDLOG) << "writeHthInfo failed " << msgId << " : " << e.what();
        return 1;
    }
    return 0;
}

void TlgCallbacks::deleteHth(const tlgnum_t& msgId)
{
    make_curs("delete from TEXT_TLG_H2H where MSG_ID = :msg_id")
        .bind(":msg_id", msgId.num.get()).exec();
}

void TlgCallbacks::splitError(const tlgnum_t& num)
{
    make_curs("UPDATE TEXT_TLG_NEW SET ERRTEXT = :err WHERE MSG_ID = :id")
        .bind(":id", num.num.get())
        .bind(":err", "SPLIT FAILED")
        .exec();

    errorQueue().addTlg(num, "SPLITTER", 101, "SPLIT FAILED");
}

void TlgCallbacksAbstractDb::joinError(const tlgnum_t& num)
{
    errorQueue().addTlg(num, "JOINER", 100, "NOT JOINED - MISSING PARTS");
}

std::list<tlgnum_t> TlgCallbacks::partsReadAll(int rtr, const std::string& uid)
{
    telegrams::AirQPartFilter filter;
    filter.setMsgUid(uid).setPeer(rtr);
    auto parts = telegrams::airQPartManager()->select(filter, {telegrams::AirQPartOrder::Part});
    std::list<tlgnum_t> tlgnums;
    for(const AirQPart& part: parts) {
        tlgnums.push_back(part.MsgIdLocal);
    }

    return tlgnums;
}

int TlgCallbacks::partsCountAll(int rtr, const std::string& uid)
{
    telegrams::AirQPartFilter filter;
    filter.setMsgUid(uid).setPeer(rtr);
    return telegrams::airQPartManager()->count(filter);
}

void TlgCallbacks::partsDeleteAll(int rtr, const std::string& msgUid)
{
    telegrams::AirQPartFilter filter;
    filter.setMsgUid(msgUid).setPeer(rtr);
    std::list<AirQPart> parts = airQPartManager()->select(filter);
    for(const AirQPart& part: parts) {
        airQManager()->remove(part.MsgIdLocal);
    }
    airQPartManager()->remove(filter);
}

void TlgCallbacks::partsAdd(int rtr, const tlgnum_t& localMsgId, int partnumber, bool endFlag, const std::string& msgUid)
{
    telegrams::airQPartManager()->insert(partnumber, rtr, localMsgId, endFlag, msgUid);
}

int TlgCallbacks::partsEndNum(int rtr, const std::string& msgUid)
{
    telegrams::AirQPartFilter filter;
    filter.setMsgUid(msgUid).setPeer(rtr).setEndFlag(1);
    auto recs = telegrams::airQPartManager()->select(filter);
    if(recs.size() != 1 ) {
        LogTrace(TRACE3) << "Warning! Loaded " << recs.size()
            << " end parts for msgUid[" << msgUid << "]"
            << " and peer[" << rtr << "]!";
        return -1;
    }
    return recs.front().Part;
}

int TlgCallbacks::readHthInfo(const tlgnum_t& msgId, HthInfo& hthInfo)
{
    std::string type, qri5, qri6, remAddrNum, part;
    int end = 0;
    OciCpp::CursCtl cr = make_curs(
            "SELECT TYPE, SNDR, RCVR, TPR, ERR, PART, END, QRI5, QRI6, REM_ADDR_NUM "
            "FROM TEXT_TLG_H2H WHERE MSG_ID=:id");
    cr.bind(":id", msgId.num.get())
        .autoNull()
        .def(type).def(hthInfo.sender).def(hthInfo.receiver)
        .def(hthInfo.tpr).def(hthInfo.why).defNull(part, std::string())
        .defNull(end, 0).def(qri5).def(qri6).def(remAddrNum)
        .EXfet();
    if (cr.err() == NO_DATA_FOUND) {
        LogTrace(TRACE5) << "Can't get header param of HTH tlg NO_DATA_FOUND for msg_id: " <<  msgId;
        return -1;
    }
    ASSERT(part.length() == 1 || part.length() == 0);
    hthInfo.type = type[0];
    hthInfo.part = part.empty() ? 0 : part[0];
    hthInfo.end  = end;
    hthInfo.qri5 = qri5[0];
    hthInfo.qri6 = qri6[0];
    hthInfo.remAddrNum = remAddrNum[0];

    hth::trace(TRACE5, hthInfo);

    return 0;
}

bool TlgCallbacks::tlgIsHth(const tlgnum_t& msgId)
{
    OciCpp::CursCtl cur = make_curs("SELECT 1 FROM TEXT_TLG_H2H WHERE MSG_ID=:id");
    cur.bind(":id", msgId.num.get()).EXfet();
    if (cur.err() == NO_DATA_FOUND)
        return false;

    return true;
}

int TlgCallbacksAbstractDb::getTlg(const tlgnum_t& tlgNum, TlgInfo& info, std::string& tlgText)
{
    const int diffs = readTlg(tlgNum, tlgText);

    if (diffs < 0) {
        LogError(STDLOG) << "read_tlg(" << tlgNum << ") - failed";
        return -1;
    }

    if (info.ttl > 0) {
        const int oldTtl = info.ttl;
        if (size_t(diffs) >= info.ttl)
            info.ttl = 1;
        else
            info.ttl = info.ttl - diffs;
        LogTrace(TRACE1) << "tlg[" << tlgNum << "] with TTL=" << oldTtl
            << ", TTL was decreased by " << diffs << ", new TTL=" << info.ttl;
    }
    LogTrace(TRACE5) << "getTlg: " << tlgNum << " queueType=" << info.queueType
        << " queueNum=" << info.queueNum << " ttl=" << info.ttl;
    return 0;
}

int TlgCallbacksAbstractDb::readTlg(const tlgnum_t& msg_id, std::string& tlgText)
{
    boost::posix_time::ptime t;
    int rtr(0);
    return readTlg(msg_id, t, rtr, tlgText);
}

int TlgCallbacks::readTlg(const tlgnum_t& msg_id, std::string& tlgText)
{
    return TlgCallbacksAbstractDb::readTlg(msg_id, tlgText);
}

void TlgCallbacks::savepoint(const std::string &sp_name) const
{
    make_curs("savepoint " + sp_name).exec();
}

void TlgCallbacks::rollback(const std::string &sp_name) const
{
    make_curs("rollback to savepoint " + sp_name).exec();
}

void TlgCallbacks::rollback() const
{
    ::rollback();
}

Expected<tlgnum_t, int> TlgCallbacksAbstractDb::sendExpressTlg(const char* expressSenderSockName, const OUT_INFO& oi, const char* body)
{
    if (!body) {
        LogError(STDLOG) << "bad body";
        return -1;
    }
    const size_t bodySz = strlen(body);
    if ((bodySz > MAX_TLG_SIZE) || (!bodySz)) {
        LogError(STDLOG) << "bad body size";
        return -1;
    }

    char tlgBody[MAX_TLG_LEN] = {};
    strncpy(tlgBody, body, bodySz);
    if (oi.isHth) {
        hth::createTlg(tlgBody, oi.hthInfo);
    }

    tlgnum_t tlgNum = nextExpressNum();
    const std::string buff = telegrams::express::makeSendHeader(oi.q_num, oi.ttl, tlgNum) + tlgBody;
    LogTrace(TRACE1) << "Sending Express tlg: " << tlgNum << " to " << oi.q_num << " TTL " << oi.ttl;
    sethAfter(ServerFramework::TcpDaemonKicker(expressSenderSockName, buff.c_str(), buff.size()));
    return tlgNum;
}

Telegrams::Telegrams()
{
    m_tc = NULL;
    m_airq_mgr = NULL;
    m_airqpart_mgr = NULL;
}

Telegrams* Telegrams::Instance()
{
    static Telegrams *t = NULL;
    if (!t)
        t = new Telegrams;
    return t;
}

TlgCallbacksAbstractDb* Telegrams::callbacks()
{
    if (m_tc)
        return m_tc;
    else
        throw std::logic_error("TlgCallbacks not initialized");
}

void Telegrams::setCallbacks(TlgCallbacksAbstractDb *tc)
{
    if (m_tc)
        delete m_tc;
    m_tc = tc;
}

AirQManager* Telegrams::airQManager()
{
    if (m_airq_mgr)
        return m_airq_mgr;
    else
        throw std::logic_error("AirQCallbacks not initialized");
}

void Telegrams::setAirQCallbacks(AirQCallbacks *cb)
{
    if (m_airq_mgr)
        delete m_airq_mgr;
    m_airq_mgr = new AirQManager(cb);
}

AirQPartManager* Telegrams::airQPartManager()
{
    if (m_airqpart_mgr)
       return m_airqpart_mgr;
    else
        throw std::logic_error("AirQPartCallbacks not initialized");
}

void Telegrams::setAirQPartCallbacks(AirQPartCallbacks *cb)
{
    if (m_airqpart_mgr)
        delete m_airqpart_mgr;
    m_airqpart_mgr = new AirQPartManager(cb);
}

}//namespace telegrams
