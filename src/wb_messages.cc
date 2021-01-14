#include "wb_messages.h"
#include "astra_utils.h"
#include "pg_tquery.h"
#include "PgOraConfig.h"
#include "date_time.h"


namespace WBMessages {

using BASIC::date_time::NowUTC;


const std::list< std::pair<TMsgType::Enum, std::string> >& TMsgType::pairs()
{
    static std::list< std::pair<Enum, std::string> > l;
    if (l.empty())
    {
        l.push_back(std::make_pair(mtLOADSHEET, "LOADSHEET"));
        l.push_back(std::make_pair(mtNOTOC,     "NOTOC"));
        l.push_back(std::make_pair(mtLIR,       "LIR"));
    }
    return l;
}

const TMsgTypes& MsgTypes()
{
  static TMsgTypes msgTypes;
  return msgTypes;
}

void toDB(int point_id, TMsgType::Enum msg_type,
          const std::string &content, const std::string &source)
{
    PG::TQuery Qry;
    Qry.SQLText =
"insert into WB_MSG(ID, MSG_TYPE, POINT_ID, TIME_RECEIVE, SOURCE) values "
"(:id, :msg_type, :point_id, :nowutc, :source)";

    int id = PgOra::getSeqNextVal("CYCLE_ID__SEQ");
    Qry.CreateVariable("id",       otInteger, id);
    Qry.CreateVariable("msg_type", otString,  MsgTypes().encode(msg_type));
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("nowutc",   otDate,    NowUTC());
    Qry.CreateVariable("source",   otString,  source);
    Qry.Execute();

    PG::TQuery TxtQry;
    TxtQry.SQLText =
"insert into WB_MSG_TEXT(ID, PAGE_NO, TEXT) values(:id, :page_no, :text)";

    TxtQry.CreateVariable("id", otInteger, id);
    TxtQry.DeclareVariable("page_no", otInteger);
    TxtQry.DeclareVariable("text", otString);

    longToDB(TxtQry, "text", content);
    TReqInfo::Instance()->LocaleToLog("EVT.WB.PRINT",
            LEvntPrms() << PrmSmpl<std::string>("msg_type", MsgTypes().encode(msg_type)),
            ASTRA::evtFlt, point_id);
}

}//namespace WBMessages
