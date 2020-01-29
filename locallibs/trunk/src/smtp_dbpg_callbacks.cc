#ifdef ENABLE_PG
#include "smtp_dbpg_callbacks.h"
#include "serverlib/cursctl.h"
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "serverlib/dates.h"
#include "serverlib/pg_cursctl.h"
#include "serverlib/exception.h"
#define NICKNAME "EMAIL"
#include "serverlib/slogger.h"

namespace SMTP {

std::list<msg_id_length> EmailMsgDbPgCallbacks::msgListToSend(time_t interval,const size_t loop_max_count) const
{
    std::list<msg_id_length> res;

    boost::posix_time::ptime start_date_time = Dates::currentDateTime() - Dates::seconds(interval);
    auto cur = make_pg_curs(sd,
      "select id, length(data) l1 from email_msg where created > :start_date "
      "and sent!=1 and  err_code!=1 and ready!=0 order by id asc "
      "limit :loop_max_count");

    std::string id;
    unsigned dlen;
    cur
        .bind(":loop_max_count", loop_max_count)
        .bind(":start_date", start_date_time)
        .def(id)
        .def(dlen)
        .exec();
    while(!cur.fen()) {
        res.push_back({id, dlen});
    }
    return res;
}

static void throw_no_data_found(PgCpp::CursCtl &cur)
{
    if(cur.err() == PgCpp::NoDataFound) {
        PgCpp::details::dumpCursor(cur);
        throw ServerFramework::Exception("email_msg: No data found");
    }
}

std::vector<char> EmailMsgDbPgCallbacks::readMsg(const std::string &id, unsigned dlen) const
{
    std::string data;
    auto cur = make_pg_curs(sd, "select data from email_msg where id=:id");
    cur
        .bind(":id", id)
        .def(data)
        .EXfet();
    throw_no_data_found(cur);
    ASSERT(data.length() == dlen);

    std::vector<char> result;
    result.resize(dlen);
    std::memmove(result.data(), data.data(), dlen);

    return result;
}
void EmailMsgDbPgCallbacks::markMsgSent(const std::string &id) const
{
    auto cur = make_pg_curs(sd, "update email_msg set sent=1 where id=:id" );
    cur.bind(":id",id).exec();
}
void EmailMsgDbPgCallbacks::markMsgSentError(const std::string &id, int err_code, const std::string &error_text) const
{
    auto cur = make_pg_curs(sd, "update email_msg set err_code=1, err_text=:txt where id=:id");
    cur.bind(":id",id).bind(":txt",error_text).exec();
}
std::string EmailMsgDbPgCallbacks::saveMsg(const std::string& txt, const std::string& type, bool send_now) const
{
    std::string id;
    boost::posix_time::ptime current_date_time = Dates::currentDateTime();
    auto cur = make_pg_curs(sd,
      "insert into email_msg(type, id, created, data, sent, err_code, ready) "
      "  values(:type, ltrim(to_char(nextval('email_seq'), '9999999999'), ' '), "
      "         :current_date_time, :data, 0, 0, :ready) "
      "returning id");

    cur.
        bind(":type",type).
        bind(":data",txt).
        bind(":ready",send_now ? 1 : 0).
        bind(":current_date_time",current_date_time).
        def(id);
    cur.EXfet();

    return id;
}
void EmailMsgDbPgCallbacks::markMsgForSend(const std::string &id) const
{
    make_pg_curs(sd, "update email_msg set ready=1 where id=:id").bind(":id",id).exec();
}
void EmailMsgDbPgCallbacks::deleteDelayed(const std::string &id) const
{
    make_pg_curs(sd, "delete from email_msg where id=:id and ready!=1").bind(":id",id).exec();
}
void EmailMsgDbPgCallbacks::commit() const
{
    PgCpp::commit();
}

bool EmailMsgDbPgCallbacks::processEnable() const
{
    return true; // check table existance if needed
}
} // namespace SMTP
#endif//ENABLE_PG
