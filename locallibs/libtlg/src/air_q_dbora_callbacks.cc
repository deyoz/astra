#include "air_q_dbora_callbacks.h"
#include "consts.h"

#include <serverlib/cursctl.h>
#include <serverlib/dates_oci.h>
#include <serverlib/int_parameters_oci.h>
#include <serverlib/rip_oci.h>
#include <serverlib/exception.h>
#include <serverlib/testmode.h>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace telegrams {

namespace {

struct AirQDefSupport
{
    tlgnum_t::num_t::base_type MsgId;
    int                        QNum;
    int                        SysQ;
    int                        OldId;
    int                        Attempts;

    static constexpr const char* Fields = "MSG_ID, Q_NUM, SYS_Q, OLD_ID, ATTEMPTS";

    AirQ make() const
    {
        return { tlgnum_t(MsgId),
                 QNum_t(QNum),
                 SysQ_t(SysQ),
                 OldId,
                 Attempts };
    }

    void def(OciCpp::CursCtl& cur)
    {
        cur
                .def(MsgId)
                .def(QNum)
                .def(SysQ)
                .defNull(OldId, 0)
                .def(Attempts);
    }
};

//---------------------------------------------------------------------------------------

struct AirQPartDefSupport
{
    int                        Part;
    int                        Peer;
    tlgnum_t::num_t::base_type MsgIdLocal;
    int                        EndFlag;
    std::string                MsgUid;

    static constexpr const char* Fields = "END_FLAG, MSG_ID_LOCAL, PART, PEER, MSG_UID";

    AirQPart make() const
    {
        return { Part,
                 Peer,
                 tlgnum_t(MsgIdLocal),
                 EndFlag,
                 MsgUid };
    }

    void def(OciCpp::CursCtl& cur)
    {
        cur
                .defNull(EndFlag, 0)
                .defNull(MsgIdLocal, "")
                .defNull(Part, 0)
                .defNull(Peer, 0)
                .def(MsgUid);
    }
};

//---------------------------------------------------------------------------------------

void bind(OciCpp::CursCtl& cur, const AirQFilter& f)
{
    cur.stb();
    if(f.MsgId)     cur.bind(":msg_id",      f.MsgId->num);
    if(f.MsgIdFrom) cur.bind(":msg_id_from", f.MsgIdFrom->num);
    if(f.OldId)     cur.bind(":old_id",      *f.OldId);
    if(f.QNum)      cur.bind(":q_num",       f.QNum->get());
    if(f.SysQ)      cur.bind(":sys_q",       f.SysQ->get());
    if(f.RDateFrom) cur.bind(":rdate_from",  *f.RDateFrom);
    if(f.RDateTo)   cur.bind(":rdate_to",    *f.RDateTo);
}

//---------------------------------------------------------------------------------------

void bind(OciCpp::CursCtl& cur, const AirQ& r)
{
    cur.stb()
            .bind(":msg_id",   r.MsgId.num)
            .bind(":old_id",   r.OldId)
            .bind(":q_num",    r.QNum.get())
            .bind(":sys_q",    r.SysQ.get());
}

//---------------------------------------------------------------------------------------

void bind(OciCpp::CursCtl& cur, const std::list<QNum_t>& qNums)
{
    cur.stb();
    unsigned k = 0;
    for(auto qNum: qNums) {
        cur.bind(":qnum" + std::to_string(++k), qNum.get());
    }
}

//---------------------------------------------------------------------------------------

void bind(OciCpp::CursCtl& cur, const AirQPartFilter& f)
{
    cur.stb();
    if(f.MsgUid)    cur.bind(":msg_uid",  *f.MsgUid);
    if(f.Peer)      cur.bind(":peer",     *f.Peer);
    if(f.EndFlag)   cur.bind(":end_flag", *f.EndFlag);
}

//---------------------------------------------------------------------------------------

void bind(OciCpp::CursCtl& cur, const AirQPart& r)
{
    cur.stb()
            .bind(":end_flag",     r.EndFlag)
            .bind(":msg_id_local", r.MsgIdLocal.num)
            .bind(":part",         r.Part)
            .bind(":peer",         r.Peer)
            .bind(":msg_uid",      r.MsgUid);
}

}//namespace

/////////////////////////////////////////////////////////////////////////////////////////

boost::optional<AirQ> AirQOraCallbacks::selectNext(const AirQFilter& f,
                                                   const AirQOrder& o) const
{
    AirQDefSupport defSupport = {};
    auto query = AirQDbSupport::selectQuery(AirQDefSupport::Fields, f, o);
    auto cur = make_curs(query);
    bind(cur, f);
    defSupport.def(cur);
    if(cur.exfet() != NO_DATA_FOUND) {
        return defSupport.make();
    }
    return boost::none;
}

std::list<AirQ> AirQOraCallbacks::select(const AirQFilter& f,
                                         const AirQOrder& o) const
{
    AirQDefSupport defSupport = {};
    auto query = AirQDbSupport::selectQuery(AirQDefSupport::Fields, f, o);
    LogTrace(TRACE3) << "select query: " << query;
    auto cur = make_curs(query);
    bind(cur, f);
    defSupport.def(cur);
    cur.exec();
    std::list<AirQ> airQs = {};
    while(!cur.fen()) {
        airQs.push_back(defSupport.make());
    }
    return airQs;
}

std::list<QNum_t> AirQOraCallbacks::selectQNums(SysQ_t sysQ) const
{
    AirQFilter f = {};
    f.setSysQ(sysQ);

    int qNum = 0;
    auto query = AirQDbSupport::selectQuery("distinct Q_NUM", f, {});
    auto cur = make_curs(query);
    bind(cur, f);
    cur.def(qNum);
    cur.exec();
    std::list<QNum_t> qNums;
    while(!cur.fen()) {
        qNums.push_back(QNum_t(qNum));
    }
    return qNums;
}

void AirQOraCallbacks::updateQNum(const tlgnum_t& msgId, QNum_t qNum) const
{
    AirQFilter f = {};
    f.setMsgId(msgId);

    auto query = AirQDbSupport::updateQuery("Q_NUM = :qnum_new", f, {}, inTestMode());
    auto cur = make_curs(query);
    bind(cur, f);
    cur.bind(":qnum_new", qNum.get()).exec();
}

void AirQOraCallbacks::updateSysQ(const tlgnum_t& msgId, SysQ_t sysQ) const
{
    AirQFilter f = {};
    f.setMsgId(msgId);

    auto query = AirQDbSupport::updateQuery("SYS_Q = :sysq_new", f, {}, inTestMode());
    auto cur = make_curs(query);
    bind(cur, f);
    cur.bind(":sysq_new", sysQ.get()).exec();
}

void AirQOraCallbacks::incAttempts(const tlgnum_t& msgId) const
{
    AirQFilter f = {};
    f.setMsgId(msgId);

    auto query = AirQDbSupport::updateQuery("ATTEMPTS = ATTEMPTS + 1", f, {}, inTestMode());
    auto cur = make_curs(query);
    bind(cur, f);
    cur.exec();
}

void AirQOraCallbacks::decAttempts(const tlgnum_t& msgId) const
{
    AirQFilter f = {};
    f.setMsgId(msgId);

    auto query = AirQDbSupport::updateQuery("ATTEMPTS = ATTEMPTS - 1", f, {}, inTestMode());
    auto cur = make_curs(query);
    bind(cur, f);
    cur.exec();
}

bool AirQOraCallbacks::insert(const AirQ& rec) const
{
    auto query = AirQDbSupport::insertQuery(inTestMode());
    auto cur = make_curs(query);
    bind(cur, rec);
    cur.noThrowError(CERR_U_CONSTRAINT).exec();
    return (cur.err() != CERR_U_CONSTRAINT);
}

void AirQOraCallbacks::remove(const AirQFilter& f) const
{
    auto query = AirQDbSupport::deleteQuery(f, inTestMode());
    auto cur = make_curs(query);
    bind(cur, f);
    cur.exec();
}

void AirQOraCallbacks::remove(const std::list<QNum_t>& qNums) const
{
    auto query = AirQDbSupport::deleteQuery(qNums, inTestMode());
    auto cur = make_curs(query);
    bind(cur, qNums);
    cur.exec();
}

std::list<AirQStat> AirQOraCallbacks::getQStats(SysQ_t sysQ) const
{
    AirQFilter f = {};
    f.setSysQ(sysQ);

    int qNum = 0, cnt = 0;
    tlgnum_t::num_t::base_type msgId;
    auto query =
"select Q_NUM, count(*), min(MSG_ID) from " + AirQManager::tableWithSchema() + " "
"where SYS_Q = :sys_q group by Q_NUM";
    auto cur = make_curs(query);
    cur.def(qNum).def(cnt).def(msgId);
    bind(cur, f);
    cur.exec();

    std::list<AirQStat> stats;
    while(!cur.fen()) {
        stats.push_back(AirQStat(tlgnum_t(msgId), qNum, cnt));
    }
    return stats;
}

boost::optional<AirQStat> AirQOraCallbacks::getQStat(SysQ_t sysQ, QNum_t qNum) const
{
    AirQFilter f = {};
    f.setSysQ(sysQ).setQNum(qNum);

    int qnum = 0, cnt = 0;
    tlgnum_t::num_t::base_type msgId;
    auto query =
"select Q_NUM, count(*), min(MSG_ID) from "+ AirQManager::tableWithSchema() + " "
"where SYS_Q = :sys_q and Q_NUM = :q_num group by Q_NUM";
     auto cur = make_curs(query);
     cur.def(qnum).def(cnt).def(msgId);
     bind(cur, f);
     if(cur.EXfet() == NO_DATA_FOUND) {
         return boost::none;
     }
     return AirQStat(tlgnum_t(msgId), qnum, cnt);
}

std::list<AirQOutStat> AirQOraCallbacks::getQOutStats() const
{
    const int BadQueueNum = -1;
    int qnum = BadQueueNum;
    tlgnum_t::num_t::base_type tpaNum;
    tlgnum_t::num_t::base_type tpbNum;
    tlgnum_t::num_t::base_type dlvNum;
    int tpaCnt = 0;
    int tpbCnt = 0;
    int dlvCnt = 0;

    auto query =
"select IDA, TPA.CNT, TPA.MSG_ID, OTH.CNT, OTH.MSG_ID, DELIV.CNT, DELIV.MSG_ID \n"
"from \n"
"(select Q_NUM as IDA from " + AirQManager::tableWithSchema() + " where SYS_Q in (:tpa, :tpb, :dlv) group by Q_NUM) ROTS \n"
"left outer join \n"
"(select Q_NUM, COUNT(*) as CNT, min(MSG_ID) as MSG_ID from " + AirQManager::tableWithSchema() + " where SYS_Q = :tpa group by Q_NUM) TPA \n"
"on IDA = TPA.Q_NUM \n"
"left outer join \n"
"(select Q_NUM, COUNT(*) as CNT, min(MSG_ID) as MSG_ID from " + AirQManager::tableWithSchema() + " where SYS_Q = :tpb group by Q_NUM) OTH \n"
"on IDA = OTH.Q_NUM \n"
"left outer join \n"
"(select Q_NUM, COUNT(*) as CNT, min(MSG_ID) as MSG_ID from " + AirQManager::tableWithSchema() + " where SYS_Q = :dlv group by Q_NUM) DELIV \n"
"on IDA = DELIV.Q_NUM \n"
"where \n"
"TPA.CNT is not null \n"
"OR OTH.CNT is not null \n"
"OR DELIV.CNT is not null \n"
"order by IDA";

    auto cur = make_curs(query);
    cur.defNull(qnum, BadQueueNum)
       .defNull(tpaCnt, 0).defNull(tpaNum, "")
       .defNull(tpbCnt, 0).defNull(tpbNum, "")
       .defNull(dlvCnt, 0).defNull(dlvNum, "")
            .bind(":tpa", (int)TYPEA_OUT)
            .bind(":tpb", (int)OTHER_OUT)
            .bind(":dlv", (int)WAIT_DELIV)
       .exec();

    std::list<AirQOutStat> outStats;
    while(!cur.fen())
    {
        if(qnum == BadQueueNum)
        {
            LogWarning(STDLOG) << "Undefined Q_NUM!";
            continue;
        }

        AirQOutStat outStat = {};
        if(auto tlgNum = tlgnum_t::create(tpaNum)) {
            outStat.tpa = AirQStat(*tlgNum, qnum, tpaCnt);
        }
        if(auto tlgNum = tlgnum_t::create(tpbNum)) {
            outStat.tpb = AirQStat(*tlgNum, qnum, tpbCnt);
        }
        if(auto tlgNum = tlgnum_t::create(dlvNum)) {
            outStat.dlv = AirQStat(*tlgNum, qnum, dlvCnt);
        }
        outStats.push_back(outStat);
    }

    return outStats;
}

MsgProcessStatus AirQOraCallbacks::checkProcessStatus(const tlgnum_t& msgId) const
{
    const int TLG_ALREADY_PROCESSED = 20039;
    const int TLG_LAST_ATTEMPT_TO_PROCESS = 20040;

    auto query = "begin " + AirQManager::schema() + "nextTlgFromQueue(:tlgNum); end;";
    auto cur = make_curs(query);
    cur.bind(":tlgNum", msgId.num)
            .noThrowError(TLG_LAST_ATTEMPT_TO_PROCESS)
            .noThrowError(TLG_ALREADY_PROCESSED);
    cur.exec();
    if(cur.err() == TLG_LAST_ATTEMPT_TO_PROCESS) {
        return MsgProcessStatus::LastAttempt;
    }
    if(cur.err() == TLG_ALREADY_PROCESSED) {
        return MsgProcessStatus::AlreadyProcessed;
    }
    return MsgProcessStatus::None;
}

void AirQOraCallbacks::commit() const
{
    OciCpp::mainSession().commit();
}

void AirQOraCallbacks::rollback() const
{
    OciCpp::mainSession().rollback();
}

#ifdef XP_TESTING
void AirQOraCallbacks::clear() const
{
    make_curs(AirQDbSupport::deleteQuery(inTestMode())).exec();
}

size_t AirQOraCallbacks::size() const
{
    size_t sz = 0;
    auto cur = make_curs(AirQDbSupport::countQuery());
    cur.def(sz).EXfet();
    return sz;
}
#endif //XP_TESTING

//---------------------------------------------------------------------------------------

std::list<AirQPart> AirQPartOraCallbacks::select(const AirQPartFilter& f,
                                                 const AirQPartOrder& o) const
{
    AirQPartDefSupport defSupport = {};
    auto query = AirQPartDbSupport::selectQuery(AirQPartDefSupport::Fields, f, o);
    auto cur = make_curs(query);
    bind(cur, f);
    defSupport.def(cur);
    cur.exec();
    std::list<AirQPart> airQParts = {};
    while(!cur.fen()) {
        airQParts.push_back(defSupport.make());
    }
    return airQParts;
}

bool AirQPartOraCallbacks::insert(const AirQPart& rec) const
{
    auto query = AirQPartDbSupport::insertQuery(inTestMode());
    auto cur = make_curs(query);
    bind(cur, rec);
    cur.noThrowError(CERR_U_CONSTRAINT).exec();
    return (cur.err() != CERR_U_CONSTRAINT);
}

void AirQPartOraCallbacks::remove(const AirQPartFilter& f) const
{
    auto query = AirQPartDbSupport::deleteQuery(f, inTestMode());
    auto cur = make_curs(query);
    bind(cur, f);
    cur.exec();
}

unsigned AirQPartOraCallbacks::count(const AirQPartFilter& f) const
{
    unsigned cnt = 0;
    auto query = AirQPartDbSupport::countQuery(f);
    OciCpp::CursCtl cur = make_curs(query);
    bind(cur, f);
    cur.def(cnt).EXfet();
    return cnt;
}

void AirQPartOraCallbacks::commit() const
{
    OciCpp::mainSession().commit();
}

void AirQPartOraCallbacks::rollback() const
{
    OciCpp::mainSession().rollback();
}

#ifdef XP_TESTING
void AirQPartOraCallbacks::clear() const
{
    make_curs(AirQPartDbSupport::deleteQuery(inTestMode())).exec();
}

size_t AirQPartOraCallbacks::size() const
{
    size_t sz = 0;
    auto cur = make_curs(AirQPartDbSupport::countQuery());
    cur.def(sz).EXfet();
    return sz;
}
#endif //XP_TESTING

}//namespace telegrams
