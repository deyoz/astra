#include "smtp_dbora_callbacks.h"
#include "serverlib/cursctl.h"
#include "dates_oci.h"
#include "oci8.h"
#include "oci8cursor.h"
#define NICKNAME "EMAIL"
#include "serverlib/slogger.h"

namespace SMTP {

EmailMsgDbOraCallbacks::EmailMsgDbOraCallbacks(MakeCursFunction specialCurs_, MakeCurs8Function specialCurs8_, CommitFunction specialCommit_)
:specialCurs_(specialCurs_), specialCurs8_(specialCurs8_), specialCommit_(specialCommit_)
{
}

OciCpp::CursCtl EmailMsgDbOraCallbacks::makeSpecialCursor(const std::string& query) const
{
  if (specialCurs_)
    return specialCurs_(STDLOG, query);
  else
    return make_curs(query);
}
OciCpp::Curs8Ctl EmailMsgDbOraCallbacks::makeSpecialCursor8(const std::string& query) const
{
  if (specialCurs8_)
    return specialCurs8_(STDLOG, query);
  else
    return OciCpp::Curs8Ctl(STDLOG, query);
}
void EmailMsgDbOraCallbacks::specialCommit() const
{
  if (specialCommit_!=nullptr)
    specialCommit_();
  else
    commit();
}

std::list<msg_id_length> EmailMsgDbOraCallbacks::msgListToSend(time_t interval,const size_t loop_max_count) const
{
    std::list<msg_id_length> res;

    boost::posix_time::ptime current_date_time = Dates::currentDateTime();
    OciCpp::CursCtl cur = makeSpecialCursor(
      "select id, l1 from ("
      "    select id, length(data) l1 from email_msg where created > :current_date_time - :t/86400 "
      "      and sent!=1 and  err_code!=1 and ready!=0 order by created desc"
      "  ) where  rownum<:loop_max_count+1");

    std::string id;
    unsigned dlen;
    cur
        .bind(":t", interval)
        .bind(":loop_max_count", loop_max_count)
        .bind(":current_date_time", current_date_time)
        .def(id)
        .def(dlen)
        .exec();
    while(!cur.fen()) {
        res.push_back({id, dlen});
    }
    return res;
}

std::vector<char> EmailMsgDbOraCallbacks::readMsg(const std::string &id, unsigned dlen) const
{
    static const int clob_len=1000000;
    std::vector<char> data;
    int rest = dlen;
    data.resize(dlen);
    for( int current_len = std::min(rest, clob_len),
         start_index = 0; rest > 0;
         rest -= current_len, start_index += current_len,
         current_len = std::min(clob_len,rest))
    {
        OciCpp::CursCtl c2 = makeSpecialCursor("select substr(data,:pos+1,:len) from email_msg where id=:id");
        c2.throwAll().bind(":id",id).bind(":pos",start_index).bind(":len",current_len)
            .defFull(&data[start_index],current_len,0,0,SQLT_LNG).EXfet();
    }
    return data;
}
void EmailMsgDbOraCallbacks::markMsgSent(const std::string &id) const
{
    OciCpp::CursCtl c3 = makeSpecialCursor("update email_msg set sent=1 where id=:id" );
    c3.bind(":id",id).exec();
}
void EmailMsgDbOraCallbacks::markMsgSentError(const std::string &id, int err_code, const std::string &error_text) const
{
    OciCpp::CursCtl c = makeSpecialCursor("update email_msg set err_code=1, err_text=:txt where id=:id");
    c.bind(":id",id).bind(":txt",error_text).exec();
}
std::string EmailMsgDbOraCallbacks::saveMsg(const std::string& txt, const std::string& type, bool send_now) const
{
    int id = 0;
    boost::posix_time::ptime current_date_time = Dates::currentDateTime();
    OciCpp::Curs8Ctl c = makeSpecialCursor8(
      "begin insert into email_msg(type,id,created,data,sent,err_code,ready) "
      "  values(:type,email_seq.nextval,:current_date_time,:data,0,0,:ready) "
      "returning id into :id; end;");

    c.bind(":type",type).bindClob(":data",txt).bind(":ready",send_now)
      .bindOut(":id",id).bind(":current_date_time",current_date_time);
    c.EXfet(OCI_COMMIT_ON_SUCCESS);

    return std::to_string(id);
}
void EmailMsgDbOraCallbacks::markMsgForSend(const std::string &id) const
{
    makeSpecialCursor("update email_msg set ready=1 where id=:id").bind(":id",id).exec();
}
void EmailMsgDbOraCallbacks::deleteDelayed(const std::string &id) const
{
    makeSpecialCursor("delete from email_msg where id=:id and ready!=1").bind(":id",id).exec();
}
void EmailMsgDbOraCallbacks::commit() const
{
    specialCommit();
}

bool EmailMsgDbOraCallbacks::processEnable() const
{
    bool res=false;
    try {
        OciCpp::CursCtl c = makeSpecialCursor("select 1 from email_msg where rownum<2");
        int dummy;
        c.noThrowError(CERR_TABLE_NOT_EXISTS).def(dummy).exec();
        if (c.err() == CERR_OK)
            res=true;
    } catch (...) {
        tst();
        res=false;
    }
    LogTrace(TRACE5)<<"smtp::process_enable()=" << std::boolalpha << res;
    return res;
}
} // namespace SMTP
