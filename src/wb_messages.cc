#include "wb_messages.h"
#include "astra_utils.h"
#include "qrys.h"


namespace WBMessages {

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
    TCachedQuery Qry(
            "begin "
            "   insert into wb_msg(id, msg_type, point_id, time_receive, source) values "
            "      (cycle_id__seq.nextval, :msg_type, :point_id, system.utcsysdate, :source) "
            "      returning id into :id; "
            "end; ",
            QParams()
            << QParam("point_id", otInteger, point_id)
            << QParam("msg_type", otString, MsgTypes().encode(msg_type))
            << QParam("source", otString, source)
            << QParam("id", otInteger)
            );
    Qry.get().Execute();
    int id = Qry.get().GetVariableAsInteger("id");
    TCachedQuery txtQry(
            "INSERT INTO wb_msg_text(id, page_no, text) VALUES(:id, :page_no, :text)",
            QParams()
            << QParam("id", otInteger, id)
            << QParam("page_no", otInteger)
            << QParam("text", otString)
            );
    longToDB(txtQry.get(), "text", content);
    TReqInfo::Instance()->LocaleToLog("EVT.WB.PRINT",
            LEvntPrms() << PrmSmpl<std::string>("msg_type", MsgTypes().encode(msg_type)),
            ASTRA::evtFlt, point_id);
}

}//namespace WBMessages
