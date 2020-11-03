#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

#include "air_q_callbacks.h"

#include <serverlib/cursctl.h>
#include <serverlib/dates_oci.h>
#include <serverlib/posthooks.h>
#include <serverlib/tcl_utils.h>
#include <serverlib/int_parameters_oci.h>
#include <serverlib/rip_oci.h>
#include <serverlib/oracle_connection_param.h>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace telegrams
{

std::string AirQOrder::fieldPlaceholder(OrderField of)
{
    switch(of)
    {
    case MsgId:
        return "MSG_ID";
    case Rdate:
        return "RDATE";
    default:
        return "";
    };
}

//---------------------------------------------------------------------------------------

std::string AirQPartOrder::fieldPlaceholder(OrderField of)
{
    switch(of)
    {
    case Part:
        return "PART";
    default:
        return "";
    };
}
    
//---------------------------------------------------------------------------------------
    
AirQOutStat::AirQOutStat()
{
    const tlgnum_t dummy("0");
    tpa = boost::make_optional(false, AirQStat(dummy, 0, 0));
    tpb = boost::make_optional(false, AirQStat(dummy, 0, 0));
    dlv = boost::make_optional(false, AirQStat(dummy, 0, 0));
}

//---------------------------------------------------------------------------------------

static std::string autonomousQuery(const std::string& query)
{
    return
"declare\n"
"pragma autonomous_transaction;\n"
"begin\n" + query +";\n"
"commit;\n"
"end;";
}

//---------------------------------------------------------------------------------------

static const std::string& airQConnectString()
{
    static std::string airQcs = readStringFromTcl("AIRQ_CONNECT_STRING", "");
    return airQcs;
}

static std::string airQUser()
{
    static std::string user = "";
    if(user.empty())
    {
        std::string cs = airQConnectString();
        LogTrace(TRACE0) << "AIRQ_CONNECT_STRING is " << cs;
        if(!cs.empty())
        {
            oracle_connection_param ora_param;
            split_connect_string(cs, ora_param);
            user = ora_param.login;
        }
    }
    return user;
}

static std::string airQSchema()
{
    std::string user = airQUser();
    if(!user.empty()) {
        user += ".";
    }
    return user;
}

//---------------------------------------------------------------------------------------

std::string AirQDbSupport::selectQuery(const std::string& selExpr,
                                       const AirQFilter& filter,
                                       const AirQOrder& order)
{
    return selectText(selExpr) + whereText(filter) + orderByText(order);
}

std::string AirQDbSupport::updateQuery(const std::string& updExpr,
                                       const AirQFilter& filter,
                                       const AirQOrder& order,
                                       bool autonomous)
{
    auto query = updateText(updExpr) + whereText(filter) + orderByText(order);
    return autonomous ? autonomousQuery(query) : query;
}

std::string AirQDbSupport::insertQuery(bool autonomous)
{
    auto query =
"insert into " + AirQManager::tableWithSchema() + " (MSG_ID, OLD_ID, Q_NUM, SYS_Q) "
"values (:msg_id, :old_id, :q_num, :sys_q)";
    return autonomous ? autonomousQuery(query) : query;
}

std::string AirQDbSupport::deleteQuery(bool autonomous)
{
    auto query = "delete from " + AirQManager::tableWithSchema();
    return autonomous ? autonomousQuery(query) : query;
}

std::string AirQDbSupport::deleteQuery(const AirQFilter& filter, bool autonomous)
{
    auto query = deleteQuery() + whereText(filter);
    return autonomous ? autonomousQuery(query) : query;
}

std::string AirQDbSupport::deleteQuery(const std::list<QNum_t>& qNums, bool autonomous)
{
    auto query = "delete from " + AirQManager::tableWithSchema() + whereText(qNums);
    return autonomous ? autonomousQuery(query) : query;
}

std::string AirQDbSupport::countQuery()
{
    return "select count(*) from " + AirQManager::tableWithSchema();
}

std::string AirQDbSupport::countQuery(const AirQFilter& filter)
{
    return countQuery() + whereText(filter);
}

std::string AirQDbSupport::selectText(const std::string& selExpr)
{
    return "select " + selExpr + " from " + AirQManager::tableWithSchema();
}

std::string AirQDbSupport::updateText(const std::string& updExpr)
{
    return "update " + AirQManager::tableWithSchema() + " set " + updExpr;
}

std::string AirQDbSupport::whereText(const AirQFilter& filter)
{
    std::string where = " where 1=1";
    if(filter.MsgId)     where += " and MSG_ID = :msg_id";
    if(filter.MsgIdFrom) where += " and MSG_ID > :msg_id_from";
    if(filter.OldId)     where += " and OLD_ID = :old_id";
    if(filter.QNum)      where += " and Q_NUM = :q_num";
    if(filter.SysQ)      where += " and SYS_Q = :sys_q";
    if(filter.RDateFrom) where += " and RDATE > :rdate_from";
    if(filter.RDateTo)   where += " and RDATE < :rdate_to";
    return where;
}

std::string AirQDbSupport::whereText(const std::list<QNum_t>& qNums)
{
    if(qNums.empty()) return "";
    std::string where = " where 1=1 and Q_NUM in (";
    unsigned k = 0;
    for(auto qNum: qNums) {
        (void)qNum;
        where += ":qnum" + std::to_string(++k) + ", ";
    }
    where = where.substr(0, where.length() - 2) + ")";
    return where;
}

std::string AirQDbSupport::orderByText(const AirQOrder& order)
{
    if(order.fields().empty()) return "";
    std::string orderBy = " order by ";
    for(AirQOrder::OrderField of: order.fields()) {
        orderBy += AirQOrder::fieldPlaceholder(of) + ", ";
    }
    return orderBy.substr(0, orderBy.length() - 2);
}

//---------------------------------------------------------------------------------------

std::string AirQPartDbSupport::selectQuery(const std::string& selExpr,
                                           const AirQPartFilter& filter,
                                           const AirQPartOrder& order)
{
    return selectText(selExpr) + whereText(filter) + orderByText(order);
}

std::string AirQPartDbSupport::insertQuery(bool autonomous)
{
    auto query =
"insert into " + AirQPartManager::tableWithSchema() + " (END_FLAG, MSG_ID_LOCAL, PART, PEER, MSG_UID) "
"values (:end_flag, :msg_id_local, :part, :peer, :msg_uid)";
    return autonomous ? autonomousQuery(query) : query;
}

std::string AirQPartDbSupport::deleteQuery(bool autonomous)
{
    auto query = "delete from " + AirQPartManager::tableWithSchema();
    return autonomous ? autonomousQuery(query) : query;
}

std::string AirQPartDbSupport::deleteQuery(const AirQPartFilter& filter, bool autonomous)
{
    auto query = deleteQuery() + whereText(filter);
    return autonomous ? autonomousQuery(query) : query;
}

std::string AirQPartDbSupport::countQuery()
{
    return "select count(*) from " + AirQPartManager::tableWithSchema();
}

std::string AirQPartDbSupport::countQuery(const AirQPartFilter& filter)
{
    return countQuery() + whereText(filter);
}

std::string AirQPartDbSupport::selectText(const std::string& selExpr)
{
    return "select " + selExpr + " from " + AirQPartManager::tableWithSchema();
}

std::string AirQPartDbSupport::whereText(const AirQPartFilter& filter)
{
    std::string where = " where 1=1";
    if(filter.MsgUid)  where += " and MSG_UID = :msg_uid";
    if(filter.Peer)    where += " and PEER = :peer";
    if(filter.EndFlag) where += " and END_FLAG = :end_flag";
    return where;
}

std::string AirQPartDbSupport::orderByText(const AirQPartOrder& order)
{
    if(order.fields().empty()) return "";
    std::string orderBy = " order by ";
    for(AirQPartOrder::OrderField of: order.fields()) {
        orderBy += AirQPartOrder::fieldPlaceholder(of) + ", ";
    }
    return orderBy.substr(0, orderBy.length() - 2);
}

//---------------------------------------------------------------------------------------

std::string AirQManager::schema()
{
    static std::string airqschema = airQSchema();
    return airqschema;
}

std::string AirQManager::tableWithSchema()
{
    static std::string airqTws = "";
    if(airqTws.empty())
    {
        airqTws = schema() + "AIR_Q";
        LogTrace(TRACE0) << "Full air_q name is " << airqTws;
    }
    return airqTws;
}

AirQManager::AirQManager(AirQCallbacks* cb)
    : m_cb(cb)
{
}

AirQCallbacks* AirQManager::callbacks() const
{
    return m_cb;
}

boost::optional<AirQ> AirQManager::selectNext(const AirQFilter& f, const AirQOrder& o) const
{
    return m_cb->selectNext(f, o);
}

std::list<AirQ> AirQManager::select(const AirQFilter& f, const AirQOrder& o) const
{
    return m_cb->select(f, o);
}

boost::optional<AirQ> AirQManager::find(const AirQFilter& f) const
{
    return selectNext(f, {});
}

std::list<QNum_t> AirQManager::selectQNums(SysQ_t sysQ) const
{
    return m_cb->selectQNums(sysQ);
}

std::list<QNum_t> AirQManager::selectQNums(int sysQ) const
{
    return selectQNums(getSysQ(sysQ));
}

void AirQManager::updateQNum(const tlgnum_t& msgId, QNum_t qNum) const
{
    m_cb->updateQNum(msgId, qNum);
}

void AirQManager::updateQNum(const tlgnum_t& msgId, int qNum) const
{
    updateQNum(msgId, getQNum(qNum));
}

void AirQManager::updateSysQ(const tlgnum_t& msgId, SysQ_t sysQ) const
{
    m_cb->updateSysQ(msgId, sysQ);
}

void AirQManager::updateSysQ(const tlgnum_t& msgId, int sysQ) const
{
    updateSysQ(msgId, getSysQ(sysQ));
}

void AirQManager::incAttempts(const tlgnum_t& msgId) const
{
    m_cb->incAttempts(msgId);
}

void AirQManager::decAttempts(const tlgnum_t& msgId) const
{
    m_cb->decAttempts(msgId);
}

bool AirQManager::insert(const AirQ& rec) const
{
    return m_cb->insert(rec);
}

bool AirQManager::insert(const tlgnum_t& msgId, int qNum, int sysQ, int oldId, int attempts) const
{
    AirQ rec = { msgId, getQNum(qNum), getSysQ(sysQ), oldId, attempts };
    return insert(rec);
}

void AirQManager::remove(const AirQFilter& f) const
{
    m_cb->remove(f);
}

void AirQManager::remove(const std::list<QNum_t>& qNums) const
{
    m_cb->remove(qNums);
}

std::list<AirQStat> AirQManager::getQStats(SysQ_t sysQ) const
{
    return m_cb->getQStats(sysQ);
}

std::list<AirQStat> AirQManager::getQStats(int sysQ) const
{
    return getQStats(getSysQ(sysQ));
}

boost::optional<AirQStat> AirQManager::getQStat(SysQ_t sysQ, QNum_t qNum) const
{
    return m_cb->getQStat(sysQ, qNum);
}

std::list<AirQOutStat> AirQManager::getQOutStats() const
{
    return m_cb->getQOutStats();
}

MsgProcessStatus AirQManager::checkProcessStatus(const tlgnum_t& msgId) const
{
    return m_cb->checkProcessStatus(msgId);
}

void AirQManager::commit() const
{
    m_cb->commit();
}

void AirQManager::rollback() const
{
    m_cb->rollback();
}

#ifdef XP_TESTING
void AirQManager::clear() const
{
    m_cb->clear();;
}

size_t AirQManager::size() const
{
    return m_cb->size();
}
#endif //XP_TESTING

AirQManager::~AirQManager()
{
    delete m_cb;
}

//---------------------------------------------------------------------------------------

std::string AirQPartManager::schema()
{
    static std::string airqpartschema = airQSchema();
    return airqpartschema;
}

std::string AirQPartManager::tableWithSchema()
{
    static std::string airqPartTws = "";
    if(airqPartTws.empty())
    {
        airqPartTws = schema() + "AIR_Q_PART";
        LogTrace(TRACE0) << "Full air_q_part name is " << airqPartTws;
    }
    return airqPartTws;
}

AirQPartManager::AirQPartManager(AirQPartCallbacks* cb)
    : m_cb(cb)
{
}

AirQPartCallbacks* AirQPartManager::callbacks() const
{
    return m_cb;
}

std::list<AirQPart> AirQPartManager::select(const AirQPartFilter& f, const AirQPartOrder& o) const
{
    return m_cb->select(f, o);
}

bool AirQPartManager::insert(const AirQPart& rec) const
{
    return m_cb->insert(rec);
}

bool AirQPartManager::insert(int part, int peer, const tlgnum_t& msgIdLocal, bool endFlag, const std::string& msgUid) const
{
    AirQPart rec = { part, peer, msgIdLocal, endFlag, msgUid };
    return insert(rec);
}

void AirQPartManager::remove(const AirQPartFilter& f) const
{
    m_cb->remove(f);
}

unsigned AirQPartManager::count(const AirQPartFilter& f) const
{
    return m_cb->count(f);
}

void AirQPartManager::commit() const
{
    m_cb->commit();
}

void AirQPartManager::rollback() const
{
    m_cb->rollback();
}

#ifdef XP_TESTING
void AirQPartManager::clear() const
{
    m_cb->clear();
}

size_t AirQPartManager::size() const
{
    return m_cb->size();
}
#endif //XP_TESTING

AirQPartManager::~AirQPartManager()
{
    delete m_cb;
}

}//namespace telegrams
