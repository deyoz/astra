#include "payment.h"
#include "xml_unit.h"
#include "date_time.h"
#include "exceptions.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "base_tables.h"
#include "print.h"
#include "tripinfo.h"
#include "oralib.h"
#include "astra_service.h"
#include "term_version.h"
#include "misc.h"
#include "astra_misc.h"
#include "checkin.h"
#include "baggage.h"
#include "passenger.h"
#include "points.h"
#include "emdoc.h"
#include "baggage_calc.h"
#include "events.h"

#define NICKNAME "VLAD"
#include "serverlib/test.h"

using namespace ASTRA;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace std;

void PaymentInterface::BuildTransfer(const TTrferRoute &trfer, xmlNodePtr transferNode)
{
  //��饭��� ��楤�� �� �᭮�� CheckInInterface::BuildTransfer
  if (transferNode==NULL) return;

  xmlNodePtr node=NewTextChild(transferNode,"transfer");

  int iDay,iMonth,iYear;
  for(TTrferRoute::const_iterator t=trfer.begin();t!=trfer.end();t++)
  {
    xmlNodePtr trferNode=NewTextChild(node,"segment");
    NewTextChild(trferNode,"airline",t->operFlt.airline);
    NewTextChild(trferNode,"aircode",
                 base_tables.get("airlines").get_row("code",t->operFlt.airline).AsString("aircode"));

    NewTextChild(trferNode,"flt_no",t->operFlt.flt_no);
    NewTextChild(trferNode,"suffix",t->operFlt.suffix);

    //���
    DecodeDate(t->operFlt.scd_out,iYear,iMonth,iDay);
    NewTextChild(trferNode,"local_date",iDay);

    NewTextChild(trferNode,"airp_dep",t->operFlt.airp);
    NewTextChild(trferNode,"airp_arv",t->airp_arv);

    NewTextChild(trferNode,"city_dep",
                 base_tables.get("airps").get_row("code",t->operFlt.airp).AsString("city"));
    NewTextChild(trferNode,"city_arv",
                 base_tables.get("airps").get_row("code",t->airp_arv).AsString("city"));
  };
};

namespace RCPT_PAX_DOC {

    void find_doc_type(const string &pax_doc, string &from_code, string &to_code, const string &lang)
    {
        try {
            from_code = pax_doc.substr(0, 3);
            const TRcptDocTypesRow &row=(const TRcptDocTypesRow&)(base_tables.get("rcpt_doc_types").get_row("code/code_lat",from_code));
            to_code = (lang != AstraLocale::LANG_RU ? row.code_lat : row.code);
        } catch(EBaseTableError &E) {
            try {
                from_code = pax_doc.substr(0, 2);
                const TRcptDocTypesRow &row=(const TRcptDocTypesRow&)(base_tables.get("rcpt_doc_types").get_row("code/code_lat",from_code));
                to_code = (lang != AstraLocale::LANG_RU ? row.code_lat : row.code);
            } catch(EBaseTableError &E) {
                from_code.erase();
            }
        }
    }

    bool find_doc_type(const string &pax_doc) {
        string from_code, to_code, lang;
        find_doc_type(pax_doc, from_code, to_code, lang);
        return not from_code.empty();
    }

    string transliter(const string &lang, const string &pax_doc)
    {
        string result = pax_doc;
        string from_code, to_code;
        find_doc_type(result, from_code, to_code, lang);
        if(not from_code.empty() and from_code != to_code)
            result.replace(0, from_code.size(), to_code);
        return result;
    }

    void check(const string &form_type, const string &pax_doc)
    {
        if(form_type != FT_M61) return;
        if(not find_doc_type(pax_doc))
            throw UserException("MSG.PAX_NAME", LParams() << LParam("msg", getLocaleText("MSG.PAX_NAME.DOC_TYPE_NOT_FOUND")));
    }

    string get(int pax_id)
    {
        ostringstream result;

        CheckIn::TPaxDocItem doc;
        if (LoadPaxDoc(std::nullopt, PaxId_t(pax_id), doc) && !doc.no.empty())
        {
            if (!doc.type_rcpt.empty())
              result << doc.type_rcpt;
            else if (doc.type=="P")
              result << (TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU ? "���" : "��");
            result << doc.no;

        };
        return transliter(TReqInfo::Instance()->desk.lang, result.str());
    }
}

namespace RCPT_PAX_NAME {

    bool check_title(const string &title)
    {
        return pax_name_titles().find(title)!=pax_name_titles().end();
    }

    string compile_pax_name(const vector<string> &lex)
    {
        string result;

        for(size_t i = 0; i < lex.size(); i++) {
            if(i == 1) result += "/";
            if(i > 1) result += " ";
            result += lex[i];
        }

        return result;
    }

    vector<string> split_pax_name(const string &pax_name, bool except = false)
    {
        string buf;
        vector<string> result;
        vector<char> delim;
        for(string::const_iterator si = pax_name.begin(); si != pax_name.end(); si++)
            if(*si != ' ' and *si != '/')
                buf.push_back(*si);
            else {
                delim.push_back(*si);
                if(except and buf.empty())
                    throw "MSG.PAX_NAME.FORMAT_HINT";
                if(not buf.empty()) {
                    result.push_back(buf);
                    buf.erase();
                }
            }
        if(not buf.empty())
            result.push_back(buf);
        if(except) {
            if((result.size() > 4 or result.size() < 3)) throw "MSG.PAX_NAME.FORMAT_HINT";
            for(vector<char>::iterator iv = delim.begin(); iv != delim.end(); iv++)
                if(
                        (iv == delim.begin() and *iv != '/') or
                        (iv != delim.begin() and *iv != ' ')
                  )
                    throw "MSG.PAX_NAME.FORMAT_HINT";
            if (result.empty()) throw Exception("split_pax_name: lex.empty()");
            if(not check_title(result.back())) throw "MSG.PAX_NAME.PAX_TYPE_NOT_SET";
            if(result.size() == 4 and result[2].size() != 1) throw "MSG.PAX_NAME.WRONG_MID_NAME";
        }
        /*
           int i = 0;
           for(vector<string>::iterator si = result.begin(); si != result.end(); si++, i++)
           ProgTrace(TRACE5, "result[%d] = '%s'", i, si->c_str());
           */

        return result;
    }

    string get_pax_name(DB::TQuery &Qry)
    {
        string pax_name = (string)Qry.FieldAsString("surname") + " " + Qry.FieldAsString("name");
        vector<string> lex = split_pax_name(pax_name);
        if(lex.size() == 1)
            lex.push_back(string()); // ������塞 ���⮥ ���
        int is_female=CheckIn::is_female( Qry.FieldAsString("gender"), "");
        if(is_female != NoExists) {
            string db_title = (is_female != 0 ? "�-��" : "�-�");
            db_title = getLocaleText(db_title);
            if (lex.empty()) throw Exception("get_pax_name: lex.empty()");
            if(check_title(lex.back()))
                lex.back() = db_title;
            else
                lex.push_back(db_title);
        }
        return compile_pax_name(lex);
    }

    void check_pax_name(const string &form_type, const string &pax_name)
    {
        if(form_type != FT_M61) return;

        try {
            split_pax_name(pax_name, true);
        } catch(const char *e) {
            throw UserException("MSG.PAX_NAME", LParams() << LParam("msg", getLocaleText(e)));
        }
    }

    string transliter_pax_name(TTagLang &tag_lang, string pax_name)
    {
        if(pax_name.empty()) return pax_name;

        vector<string> lex = split_pax_name(pax_name);
        if(lex.size() == 1) lex.push_back(string());
        size_t num_translited = 0;
        if (lex.empty()) throw Exception("transliter_pax_name: lex.empty()");
        if(check_title(lex.back())) {
            lex.back() = getLocaleText(upperc(lex.back()), tag_lang.GetLang());
            num_translited++;
            if(lex.size() > 4)
                lex.erase(lex.begin() + 3, lex.begin() + 3 + lex.size() - 4);
            if(lex.size() > 3) {
                lex[2] = transliter(lex[2], TranslitFormat::V1, tag_lang.GetLang() != AstraLocale::LANG_RU).substr(0, 1);
                num_translited++;
            }
        } else if(lex.size() >= 3) {
            lex.erase(lex.begin() + 3, lex.end());
            lex[2] = transliter(lex[2], TranslitFormat::V1, tag_lang.GetLang() != AstraLocale::LANG_RU).substr(0, 1);
            num_translited++;
        }

        for(size_t i = 0; i < lex.size() - num_translited; i++)
            lex[i] = transliter(lex[i],TranslitFormat::V1,tag_lang.GetLang() != AstraLocale::LANG_RU);

        return compile_pax_name(lex);
    }

}

void PaymentInterface::LoadPax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->desk.compatible(PAX_SERVICE_VERSION))
    throw UserException("MSG.TERM_VERSION.NOT_SUPPORTED");

  enum TSearchType {searchByPaxId,
                    searchByGrpId,
                    searchByRegNo,
                    searchByReceiptNo,
                    searchByScanData};

  TSearchType search_type;
  if( strcmp((const char*)reqNode->name, "PaxByPaxId") == 0) search_type=searchByPaxId;
  else
    if( strcmp((const char*)reqNode->name, "PaxByGrpId") == 0) search_type=searchByGrpId;
    else
      if( strcmp((const char*)reqNode->name, "PaxByRegNo") == 0) search_type=searchByRegNo;
      else
        if( strcmp((const char*)reqNode->name, "PaxByReceiptNo") == 0) search_type=searchByReceiptNo;
        else
          if( strcmp((const char*)reqNode->name, "PaxByScanData") == 0) search_type=searchByScanData;
          else return;

  int point_id=NodeAsInteger("point_id",reqNode);

  TPrnParams prnParams(reqNode);

  xmlNodePtr dataNode=GetNode("data",resNode);
  if (dataNode==NULL)
    dataNode = NewTextChild(resNode, "data");

  int pax_id=NoExists;
  int grp_id=NoExists;
  bool pr_unaccomp=false;
  bool pr_annul_rcpt=false;
  //᭠砫� ���஡㥬 ������� pax_id
  if (search_type==searchByReceiptNo)
  {
    DB::TQuery Qry(PgOra::getROSession({"PAX", "BAG_RECEIPTS"}), STDLOG);
    Qry.SQLText =
        "SELECT receipt_id,annul_date,point_id,grp_id, "
        "("
        "SELECT pax_id "
        "FROM pax "
        "WHERE grp_id = bag_receipts.grp_id "
        "ORDER BY "
        "  CASE WHEN bag_pool_num IS NULL THEN 1 ELSE 0 END, "
        "  CASE WHEN pers_type = '��' THEN 0 "
        "       WHEN pers_type = '��' THEN 0 "
        "       ELSE 1 END, "
        "  CASE WHEN seats = 0 THEN 1 ELSE 0 END, "
        "  CASE WHEN refuse IS NULL THEN 0 ELSE 1 END, "
        "  CASE WHEN pers_type = '��' THEN 0 "
        "       WHEN pers_type = '��' THEN 1 "
        "       ELSE 2 END, "
        "  reg_no "
        "FETCH FIRST 1 ROWS ONLY "
        ") AS pax_id "
        "FROM bag_receipts "
        "WHERE no=:no ";
    Qry.CreateVariable("no",otFloat,NodeAsFloat("receipt_no",reqNode));
    Qry.Execute();
    if (Qry.Eof)
      throw AstraLocale::UserException("MSG.RECEIPT_NOT_FOUND");
    pr_annul_rcpt=!Qry.FieldIsNULL("annul_date");
    if (Qry.FieldIsNULL("grp_id"))
    {
      //NULL �᫨ ��㯯� ࠧॣ����஢��� �� �訡�� �����
      //⮣�� �뢮��� ⮫쪮 ���⠭��
      LoadReceipts(Qry.FieldAsInteger("receipt_id"),false,prnParams.pr_lat,dataNode);
    }
    else
    {
      grp_id=Qry.FieldAsInteger("grp_id");
      if (!Qry.FieldIsNULL("pax_id"))
        pax_id=Qry.FieldAsInteger("pax_id");
      else
        //NULL �᫨ ��ᮯ஢������� �����
        pr_unaccomp=true;
    };
  };
  if (search_type==searchByRegNo)
  {
    int reg_no=NodeAsInteger("reg_no",reqNode);
    DB::TQuery Qry(PgOra::getROSession({"PAX_GRP", "PAX"}), STDLOG);
    Qry.SQLText=
      "SELECT pax.grp_id,pax.pax_id, pax.seats "
      "FROM pax_grp,pax "
      "WHERE pax_grp.grp_id=pax.grp_id AND "
      "      pax_grp.point_dep=:point_id AND reg_no=:reg_no "
      "ORDER BY pax.seats DESC";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("reg_no",otInteger,reg_no);
    Qry.Execute();
    if (Qry.Eof)
      throw AstraLocale::UserException("MSG.PASSENGER.NOT_CHECKIN");
    grp_id=Qry.FieldAsInteger("grp_id");
    pax_id=Qry.FieldAsInteger("pax_id");
    bool exists_with_seat=false;
    bool exists_without_seat=false;
    for(;!Qry.Eof;Qry.Next())
    {
      if (grp_id!=Qry.FieldAsInteger("grp_id"))
        throw EXCEPTIONS::Exception("Duplicate reg_no (point_id=%d reg_no=%d)",point_id,reg_no);
      int seats=Qry.FieldAsInteger("seats");
      if ((seats>0 && exists_with_seat) ||
          (seats<=0 && exists_without_seat))
        throw EXCEPTIONS::Exception("Duplicate reg_no (point_id=%d reg_no=%d)",point_id,reg_no);
      if (seats>0)
        exists_with_seat=true;
      else
        exists_without_seat=true;
    };
  };
  if (search_type==searchByGrpId)
  {
    DB::TQuery Qry(PgOra::getROSession("PAX"), STDLOG);
    Qry.SQLText=
      "SELECT pax_id "
      "FROM pax "
      "WHERE grp_id = :grp_id "
      "ORDER BY "
      "  CASE WHEN bag_pool_num IS NULL THEN 1 ELSE 0 END, "
      "  CASE WHEN pers_type = '��' THEN 0 "
      "       WHEN pers_type = '��' THEN 0 "
      "       ELSE 1 END, "
      "  CASE WHEN seats = 0 THEN 1 ELSE 0 END, "
      "  CASE WHEN refuse IS NULL THEN 0 ELSE 1 END, "
      "  CASE WHEN pers_type = '��' THEN 0 "
      "       WHEN pers_type = '��' THEN 1 "
      "       ELSE 2 END, "
      "  reg_no "
      "FETCH FIRST 1 ROWS ONLY ";
    Qry.CreateVariable("grp_id",otInteger,NodeAsInteger("grp_id",reqNode));
    Qry.Execute();
    if (Qry.Eof)
      throw AstraLocale::UserException("MSG.PAX_GRP_OR_LUGGAGE_NOT_CHECKED_IN");
    grp_id=Qry.FieldAsInteger("grp_id");
    if (!Qry.FieldIsNULL("pax_id"))
      pax_id=Qry.FieldAsInteger("pax_id");
    else
      //NULL �᫨ ��ᮯ஢������� �����
      pr_unaccomp=true;
  };
  if (search_type==searchByPaxId)
  {
    pax_id=NodeAsInteger("pax_id",reqNode);
  };
  if (search_type==searchByScanData)
  {
    int pax_point_id, reg_no;
    SearchPaxByScanData(reqNode, pax_point_id, reg_no, pax_id);
    if (pax_point_id==NoExists || reg_no==NoExists || pax_id==NoExists)
      throw AstraLocale::UserException("MSG.WRONG_DATA_RECEIVED");
  };

  int point_dep;

  DB::TQuery Qry(PgOra::getROSession({"PAX_GRP","PAX","AIRPS","PAX_DOC"}), STDLOG);
  if (!(search_type==searchByReceiptNo && grp_id==NoExists))
  {
    if (!pr_unaccomp)
    {
      Qry.SQLText=
        "SELECT pax_grp.grp_id, pax.pax_id, "
        "       point_dep,airp_dep,airp_arv,airps.city AS city_arv, "
        "       class, bag_refuse, piece_concept, pax_grp.tid, "
        "       pax.reg_no, "
        "       pax.surname, "
        "       pax.name, "
        "       pax_doc.gender "
          "FROM pax_grp "
          "  JOIN pax ON pax_grp.grp_id = pax.grp_id "
          "  JOIN airps ON pax_grp.airp_arv = airps.code "
          "  JOIN pax_doc ON pax_grp.grp_id = pax.grp_id "
          "WHERE pax.pax_id = :pax_id ";
      Qry.CreateVariable("pax_id",otInteger,pax_id);
      Qry.Execute();
      if (Qry.Eof)
        throw AstraLocale::UserException("MSG.PASSENGER.NOT_CHECKIN");
    }
    else
    {
      Qry.SQLText=
        "SELECT pax_grp.grp_id, NULL AS pax_id, "
        "       point_dep,airp_dep,airp_arv,airps.city AS city_arv, "
        "       class, bag_refuse, piece_concept, pax_grp.tid, "
        "       NULL AS reg_no, "
        "       NULL AS surname, "
        "       NULL AS name, "
        "       NULL AS gender "
        "FROM pax_grp,airps "
        "WHERE pax_grp.airp_arv=airps.code AND grp_id=:grp_id ";
      Qry.CreateVariable("grp_id",otInteger,grp_id);
      Qry.Execute();
      if (Qry.Eof)
        throw AstraLocale::UserException("MSG.LUGGAGE_NOT_CHECKED_IN");
    };
    point_dep=Qry.FieldAsInteger("point_dep");
  }
  else
  {
    point_dep=Qry.FieldAsInteger("point_id");
  };

  TTripInfo operFlt;
  bool operFltExists=operFlt.getByPointId(point_id);
  if (point_id!=point_dep)
  {
    point_id=point_dep;
    if (!TripsInterface::readTripHeader( point_id, dataNode) && !pr_annul_rcpt)
    {
      string msg;
      if (!operFltExists)
      {
        if (!pr_unaccomp)
          msg=getLocaleText("MSG.PASSENGER.FROM_FLIGHT", LParams() << LParam("flt", GetTripName(operFlt,ecCkin)));
        else
          msg=getLocaleText("MSG.BAGGAGE.FROM_FLIGHT", LParams() << LParam("flt", GetTripName(operFlt,ecCkin)));
      }
      else
      {
        if (!pr_unaccomp)
          msg=getLocaleText("MSG.PASSENGER.FROM_OTHER_FLIGHT");
        else
          msg=getLocaleText("MSG.BAGGAGE.FROM_OTHER_FLIGHT");
      };
      if (!pr_annul_rcpt)
        throw AstraLocale::UserException(msg);
      else
        AstraLocale::showErrorMessage(msg); //� �� ��砥 �����뢠�� ���㫨஢����� ���⠭��
    };
  };

  if (search_type==searchByReceiptNo && grp_id==NoExists) return;

  grp_id=Qry.FieldAsInteger("grp_id");
  bool piece_concept=!Qry.FieldIsNULL("piece_concept") &&
                     Qry.FieldAsInteger("piece_concept")!=0;
  if (piece_concept)
    throw UserException("MSG.PIECE_CONCEPT_PAYMENT_NOT_SUPPORTED_BY_DCS");
  NewTextChild(dataNode,"grp_id",grp_id);
  NewTextChild(dataNode,"point_dep",Qry.FieldAsInteger("point_dep"));
  NewTextChild(dataNode,"airp_dep",Qry.FieldAsString("airp_dep"));
  NewTextChild(dataNode,"airp_arv",Qry.FieldAsString("airp_arv"));
  NewTextChild(dataNode,"city_arv",Qry.FieldAsString("city_arv"));
  NewTextChild(dataNode,"class",Qry.FieldAsString("class"));
  NewTextChild(dataNode,"pr_refuse",(int)(Qry.FieldAsInteger("bag_refuse")!=0));
  NewTextChild(dataNode, "bag_types_id", grp_id);
  NewTextChild(dataNode, "piece_concept", (int)piece_concept);
  NewTextChild(dataNode,"reg_no",Qry.FieldAsInteger("reg_no"));
  NewTextChild(dataNode,"pax_name", RCPT_PAX_NAME::get_pax_name(Qry));
  if (!Qry.FieldIsNULL("pax_id"))
    NewTextChild(dataNode,"pax_doc",RCPT_PAX_DOC::get(Qry.FieldAsInteger("pax_id")));
  else
    NewTextChild(dataNode,"pax_doc");
  NewTextChild(dataNode,"tid",Qry.FieldAsInteger("tid"));
  //���ଠ�� �� �࠭���� ������
  TTrferRoute trfer;
  trfer.GetRoute(grp_id, trtNotFirstSeg);
  if (!trfer.empty())
    BuildTransfer(trfer, dataNode);

  if (!pr_unaccomp)
  {
    string subcl;
    DB::TQuery QrySubclass(PgOra::getROSession("PAX"), STDLOG);
    QrySubclass.SQLText=
      "SELECT DISTINCT subclass FROM pax "
      "WHERE grp_id=:grp_id "
      "AND subclass IS NOT NULL";
    QrySubclass.CreateVariable("grp_id",otInteger,grp_id);
    QrySubclass.Execute();
    if (!QrySubclass.Eof)
    {
      subcl=QrySubclass.FieldAsString("subclass");
      QrySubclass.Next();
      if (!QrySubclass.Eof) subcl="";
    };

    DB::TQuery QryTicket(PgOra::getROSession("PAX"), STDLOG);
    QryTicket.SQLText=
      "SELECT ticket_no FROM pax "
      "WHERE grp_id=:grp_id "
      "AND refuse IS NULL "
      "AND ticket_no IS NOT NULL "
      "ORDER BY ticket_no";
    QryTicket.CreateVariable("grp_id",otInteger,grp_id);
    QryTicket.Execute();


    string tickets,ticket,first_part;
    for(;!QryTicket.Eof;QryTicket.Next())
    {
      if (!tickets.empty()) tickets+='/';
      ticket=QryTicket.FieldAsString("ticket_no");
      if (ticket.size()<=2)
      {
        tickets+=ticket;
        first_part="";
      }
      else
      {
        if (first_part==ticket.substr(0,ticket.size()-2))
          tickets+=ticket.substr(ticket.size()-2);
        else
        {
          tickets+=ticket;
          first_part=ticket.substr(0,ticket.size()-2);
        };
      };
    };

    NewTextChild(dataNode,"subclass",subcl);
    NewTextChild(dataNode,"tickets",tickets);
  }
  else
  {
    NewTextChild(dataNode,"subclass");
    NewTextChild(dataNode,"tickets");
  };

  TGrpMktFlight mktFlight;
  if (mktFlight.getByGrpId(grp_id))
  {
    mktFlight.toXML(dataNode);
    xmlNodePtr markFltNode=NodeAsNode("mark_flight", dataNode);
    NewTextChild(markFltNode,"aircode",
                 base_tables.get("airlines").get_row("code",mktFlight.airline).AsString("aircode"));
    NewTextChild(markFltNode,"pr_mark_rates",(int)mktFlight.pr_mark_norms);
  }

  if (!pr_unaccomp)
  {
    //����㧪� ������� ���ᠦ�஢ �������� �㫮�
    DB::TQuery Qry(PgOra::getROSession("PAX"), STDLOG);
    Qry.SQLText=
      "SELECT pax_id, bag_pool_num, surname, name, pers_type, seats, refuse "
      "FROM pax "
      "WHERE grp_id=:grp_id AND bag_pool_num IS NOT NULL ";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.Execute();
    xmlNodePtr paxsNode=NewTextChild(dataNode,"passengers");
    for(;!Qry.Eof;Qry.Next())
    {
      xmlNodePtr paxNode=NewTextChild(paxsNode,"pax");
      NewTextChild(paxNode,"pax_id",Qry.FieldAsInteger("pax_id"));
      NewTextChild(paxNode,"surname",Qry.FieldAsString("surname"));
      NewTextChild(paxNode,"name",Qry.FieldAsString("name"),"");
      NewTextChild(paxNode,"pers_type",Qry.FieldAsString("pers_type"),EncodePerson(ASTRA::adult));
      NewTextChild(paxNode,"seats",Qry.FieldAsInteger("seats"),1);
      NewTextChild(paxNode,"refuse",Qry.FieldAsString("refuse"),"");
      NewTextChild(paxNode,"bag_pool_num",Qry.FieldAsInteger("bag_pool_num"));
    };
  };

  CheckIn::TGroupBagItem group_bag;
  group_bag.fromDB(grp_id, ASTRA::NoExists, false);
  group_bag.toXML(dataNode, pr_unaccomp);
  WeightConcept::TPaidBagList paid;
  WeightConcept::PaidBagFromDB(NoExists, grp_id, paid);

  TPaidBagViewMap PaidBagViewMap;
  TPaidBagViewMap TrferBagViewMap;
  map<int/*id*/, TEventsBagItem> tmp_bag;
  GetBagToLogInfo(grp_id, tmp_bag);
  WeightConcept::CalcPaidBagView(WeightConcept::TAirlines(grp_id,
                                                          mktFlight.pr_mark_norms?mktFlight.airline:operFlt.airline,
                                                          "CalcPaidBagView"),
                                 tmp_bag,
                                 WeightConcept::AllPaxNormContainer(),
                                 paid,
                                 CheckIn::TServicePaymentListWithAuto(),
                                 "",
                                 PaidBagViewMap, TrferBagViewMap);
  WeightConcept::PaidBagViewToXML(PaidBagViewMap, TrferBagViewMap, dataNode);

  LoadReceipts(grp_id,true,prnParams.pr_lat,dataNode);
  //ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
};

const int EMD_RCPT_ID=-2;

void PaymentInterface::LoadReceipts(int id, bool pr_grp, bool pr_lat, xmlNodePtr dataNode)
{
  if (dataNode==NULL) return;

  if (pr_grp)
  {
    DB::TQuery Qry(PgOra::getROSession("BAG_PREPAY"), STDLOG);
    xmlNodePtr node=NewTextChild(dataNode,"prepayment");

    //���⠭樨 EMD, MCO
    CheckIn::TServicePaymentListWithAuto payment;
    payment.fromDB(id);
    for(CheckIn::TServicePaymentListWithAuto::const_iterator i=payment.begin(); i!=payment.end(); ++i)
    {
      const CheckIn::TServicePaymentItem &item=*i;
      if (item.trfer_num!=0) continue;
      if (item.pc && !item.pc->isBaggageOrCarryOn("PaymentInterface::LoadReceipts")) continue;
      xmlNodePtr receiptNode=NewTextChild(node,"receipt");
      NewTextChild(receiptNode,"id",EMD_RCPT_ID);
      if (item.isEMD())
      {
        NewTextChild(receiptNode,"no",item.emd_no_str());
        NewTextChild(receiptNode,"aircode");
      }
      else
      {
        NewTextChild(receiptNode,"no",item.doc_no);
        NewTextChild(receiptNode,"aircode",item.doc_aircode);
      };
      NewTextChild(receiptNode,"ex_weight", item.doc_weight!=ASTRA::NoExists?item.doc_weight:0);
      NewTextChild(receiptNode,"bag_type", item.key_str_compatible());
    }

    //���⠭樨 �।������
    Qry.SQLText="SELECT * FROM bag_prepay "
                "WHERE grp_id=:grp_id";
    Qry.CreateVariable("grp_id", otInteger, id);
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      xmlNodePtr receiptNode=NewTextChild(node,"receipt");
      NewTextChild(receiptNode,"id",Qry.FieldAsInteger("receipt_id"));
      NewTextChild(receiptNode,"no",Qry.FieldAsString("no"));
      NewTextChild(receiptNode,"aircode",Qry.FieldAsString("aircode"));
      if (!Qry.FieldIsNULL("ex_weight"))
        NewTextChild(receiptNode,"ex_weight",Qry.FieldAsInteger("ex_weight"));
      if (!Qry.FieldIsNULL("bag_type"))
      {
        ostringstream s;
        s << setw(2) << setfill('0') << Qry.FieldAsInteger("bag_type");
        NewTextChild(receiptNode,"bag_type",s.str());
      }
      else
        NewTextChild(receiptNode,"bag_type");
      if (!Qry.FieldIsNULL("value"))
      {
        NewTextChild(receiptNode,"value",Qry.FieldAsFloat("value"));
        NewTextChild(receiptNode,"value_cur",Qry.FieldAsString("value_cur"));
      };
    };
  };

  DB::TQuery Qry(PgOra::getROSession("BAG_RECEIPTS"), STDLOG);
  if (pr_grp)
  {
    Qry.SQLText=
      "SELECT * FROM bag_receipts "
      "WHERE grp_id=:grp_id "
      "ORDER BY issue_date";
    Qry.CreateVariable("grp_id", otInteger, id);
  }
  else
  {
    Qry.SQLText=
      "SELECT * FROM bag_receipts "
      "WHERE receipt_id=:receipt_id";
    Qry.CreateVariable("receipt_id", otInteger, id);
  };
  Qry.Execute();
  xmlNodePtr node=NewTextChild(dataNode,"payment");
  for(;!Qry.Eof;Qry.Next())
  {
    int rcpt_id=Qry.FieldAsInteger("receipt_id");
    TBagReceipt rcpt;
    GetReceiptFromDB(Qry,rcpt);
    xmlNodePtr rcptNode=NewTextChild(node,"receipt");
    PutReceiptToXML(rcpt,rcpt_id,pr_lat,rcptNode);
  }
}

int PaymentInterface::LockAndUpdTid(int point_dep, int grp_id, int tid)
{
  TFlights flights;
  flights.Get( point_dep, ftTranzit );
  flights.Lock(__FUNCTION__);
  DB::TQuery QryPoint(PgOra::getROSession("POINTS"), STDLOG);
  //��稬 ३�
  QryPoint.SQLText=
    "SELECT point_id "
    "FROM points "
    "WHERE point_id=:point_id AND pr_del=0 AND pr_reg<>0 ";
  QryPoint.CreateVariable("point_id",otInteger,point_dep);
  QryPoint.Execute();
  if (QryPoint.Eof) throw AstraLocale::UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
  int new_tid = PgOra::getSeqNextVal_int("CYCLE_TID__SEQ");

  DB::TQuery QryGrp(PgOra::getRWSession("PAX_GRP"), STDLOG);
  QryGrp.SQLText=
    "UPDATE pax_grp "
    "SET tid=:new_tid "
    "WHERE grp_id=:grp_id AND tid=:tid";
  QryGrp.CreateVariable("grp_id",otInteger,grp_id);
  QryGrp.CreateVariable("tid",otInteger,tid);
  QryGrp.CreateVariable("new_tid",otInteger,new_tid);
  QryGrp.Execute();
  if (QryGrp.RowsProcessed()<=0)
    throw AstraLocale::UserException("MSG.CHECKIN.GRP.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
  return new_tid;
}

void PaymentInterface::SaveBag(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  if (TReqInfo::Instance()->desk.compatible(PAX_SERVICE_VERSION))
    throw UserException("MSG.TERM_VERSION.NOT_SUPPORTED");
  int point_dep=NodeAsInteger("point_dep",reqNode);
  int grp_id=NodeAsInteger("grp_id",reqNode);
  int tid=LockAndUpdTid(point_dep,grp_id,NodeAsInteger("tid",reqNode));
  NewTextChild(resNode,"tid",tid);

  CheckIn::TSimplePaxGrpItem grp;
  if (!grp.getByGrpId(grp_id)) return;

  CheckIn::TGroupBagItem group_bag;
  if (group_bag.fromXML(reqNode, grp_id, ASTRA::NoExists,
                           grp.is_unaccomp(), grp.baggage_pc, grp.trfer_confirm))
  {
    group_bag.checkAndGenerateTags(point_dep, grp_id);
    group_bag.toDB(grp_id, point_dep);
  };

  boost::optional< WeightConcept::TPaidBagList > paid;
  WeightConcept::PaidBagFromXML(reqNode, grp_id, grp.is_unaccomp(), grp.trfer_confirm, paid);
  if (paid)
    WeightConcept::PaidBagToDB(grp_id, grp.is_unaccomp(), paid.get());

  TReqInfo::Instance()->LocaleToLog("EVT.LUGGAGE.SAVE_DATA", ASTRA::evtPay,point_dep,0,grp_id);
};

void PaymentInterface::UpdPrepay(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    if (reqInfo->desk.compatible(PAX_SERVICE_VERSION))
      throw UserException("MSG.TERM_VERSION.NOT_SUPPORTED");

    int point_dep=NodeAsInteger("point_dep",reqNode);
    int grp_id=NodeAsInteger("grp_id",reqNode);
    int tid=LockAndUpdTid(point_dep,grp_id,NodeAsInteger("tid",reqNode));
    NewTextChild(resNode,"tid",tid);

    xmlNodePtr node=NodeAsNode("receipt",reqNode)->children;
    bool pr_del=NodeAsIntegerFast("pr_del",node);
    xmlNodePtr idNode=GetNodeFast("id",node);

    string old_aircode, old_no;
    int receipt_id = ASTRA::NoExists;
    if (idNode!=NULL)
    {
        if (NodeAsInteger(idNode)==EMD_RCPT_ID )
          throw AstraLocale::UserException("MSG.UNABLE_CHANGE_DELETE_EMD");

        DB::TQuery Qry(PgOra::getROSession("BAG_PREPAY"), STDLOG);
        receipt_id = NodeAsInteger(idNode);
        Qry.CreateVariable("receipt_id",otInteger,receipt_id);
        Qry.SQLText="SELECT * FROM bag_prepay "
                    "WHERE receipt_id=:receipt_id";
        Qry.Execute();
        if (Qry.Eof) throw AstraLocale::UserException("MSG.RECEIPT_NOT_FOUND.REFRESH_DATA");
        old_aircode=Qry.FieldAsString("aircode");
        old_no=Qry.FieldAsString("no");
    }

    if (!pr_del)
    {
        string aircode = NodeAsStringFast("aircode",node);
        string no = NodeAsStringFast("no",node);

        DB::TQuery Qry(PgOra::getRWSession("BAG_PREPAY"), STDLOG);
        if (idNode==NULL) {
            receipt_id = PgOra::getSeqNextVal_int("CYCLE_ID__SEQ");
            Qry.SQLText=
                "INSERT INTO bag_prepay ("
                "receipt_id,grp_id,no,aircode,ex_weight,bag_type,value,value_cur "
                ") VALUES("
                ":receipt_id,:grp_id,:no,:aircode,:ex_weight,:bag_type,:value,:value_cur"
                ") ";
        } else {
            Qry.SQLText=
                "UPDATE bag_prepay SET "
                "   no=:no,aircode=:aircode, "
                "   ex_weight=:ex_weight, "
                "   bag_type=:bag_type, "
                "   value=:value,"
                "   value_cur=:value_cur "
                "WHERE receipt_id=:receipt_id "
                "AND grp_id=:grp_id";
        }
        if (receipt_id == ASTRA::NoExists) {
            Qry.CreateVariable("receipt_id",otInteger,FNull);
        } else {
            Qry.CreateVariable("receipt_id",otInteger,receipt_id);
        }
        Qry.CreateVariable("grp_id",otInteger,grp_id);
        Qry.CreateVariable("no",otString,no);
        Qry.CreateVariable("aircode",otString,aircode);
        if (!NodeIsNULLFast("ex_weight",node,true))
            Qry.CreateVariable("ex_weight",otInteger,NodeAsIntegerFast("ex_weight",node));
        else
            Qry.CreateVariable("ex_weight",otInteger,FNull);
        if (!NodeIsNULLFast("bag_type",node))
            Qry.CreateVariable("bag_type",otInteger,NodeAsIntegerFast("bag_type",node));
        else
            Qry.CreateVariable("bag_type",otInteger,FNull);
        if (!NodeIsNULLFast("value",node,true))
            Qry.CreateVariable("value",otFloat,NodeAsFloatFast("value",node));
        else
            Qry.CreateVariable("value",otFloat,FNull);
        Qry.CreateVariable("value_cur",otString,NodeAsStringFast("value_cur",node,""));
        Qry.Execute();
        if (idNode!=NULL && Qry.RowsProcessed()<=0)
            throw AstraLocale::UserException("MSG.RECEIPT_NOT_FOUND.REFRESH_DATA");

        NewTextChild(resNode,"receipt_id",receipt_id);

        std::string lexema_id;
        LEvntPrms params;
        PrmLexema lexema1("rcpt_param1", ""), lexema2("rcpt_param2", "");
        if (idNode==NULL)
        {
            lexema_id = "EVT.RCPT_ADD";
            params << PrmSmpl<std::string>("rcpt", aircode) << PrmSmpl<std::string>("num", no);
        }
        else
        {
          if (aircode==old_aircode && no==old_no)
          {
              lexema_id = "EVT.RCPT_CHANGE";
              params << PrmSmpl<std::string>("rcpt", aircode) << PrmSmpl<std::string>("num", no);
          }
          else
          {
              reqInfo->LocaleToLog("EVT.RCPT_REMOVE", LEvntPrms() << PrmSmpl<std::string>("rcpt", old_aircode)
                                   << PrmSmpl<std::string>("num", old_no), ASTRA::evtPay,point_dep,0,grp_id);
              lexema_id = "EVT.RCPT_ADD";
              params << PrmSmpl<std::string>("rcpt", aircode) << PrmSmpl<std::string>("num", no);
          };
        };
        if (!NodeIsNULLFast("ex_weight",node,true))
        {
          if (!NodeIsNULLFast("bag_type",node)) {
              lexema1.ChangeLexemaId("EVT.LUGGAGE.TYPE_WEIGHT");
              lexema1.prms << PrmSmpl<int>("type", NodeAsIntegerFast("bag_type",node))
                          << PrmSmpl<int>("weight", NodeAsIntegerFast("ex_weight",node));
          }
          else {
              lexema1.ChangeLexemaId("EVT.LUGGAGE.WEIGHT");
              lexema1.prms << PrmSmpl<int>("weight", NodeAsIntegerFast("ex_weight",node));
          }
        }
        else
        if (!NodeIsNULLFast("value",node,true))
        {
          lexema2.ChangeLexemaId("EVT.LUGGAGE.VALUE");
          lexema2.prms << PrmSmpl<double>("value", NodeAsFloatFast("value",node))
                       << PrmElem<std::string>("cur", etCurrency, NodeAsStringFast("value_cur",node,""));
        }
        reqInfo->LocaleToLog(lexema_id, params << lexema1 << lexema2, ASTRA::evtPay,point_dep,0,grp_id);
    }
    else
    {
        DB::TQuery Qry(PgOra::getRWSession("BAG_PREPAY"), STDLOG);
        Qry.SQLText=
            "DELETE FROM bag_prepay WHERE receipt_id=:receipt_id";
        Qry.Execute();
        if (Qry.RowsProcessed()<=0)
            throw AstraLocale::UserException("MSG.RECEIPT_NOT_FOUND.REFRESH_DATA");
        reqInfo->LocaleToLog("EVT.RCPT_REMOVE", LEvntPrms() << PrmSmpl<std::string>("rcpt", old_aircode)
                             << PrmSmpl<std::string>("num", old_no), ASTRA::evtPay,point_dep,0,grp_id);
    };
}

//�� ���� � ��������
bool PaymentInterface::GetReceiptFromDB(int id, TBagReceipt &rcpt)
{
  DB::TQuery Qry(PgOra::getRWSession("BAG_RECEIPTS"), STDLOG);
  Qry.SQLText="SELECT * FROM bag_receipts "
              "WHERE receipt_id=:id";
  Qry.CreateVariable("id",otInteger,id);
  Qry.Execute();

  return GetReceiptFromDB(Qry,rcpt);
}

//�� ���� � ��������
bool PaymentInterface::GetReceiptFromDB(DB::TQuery &Qry, TBagReceipt &rcpt)
{
  if (Qry.Eof) return false;
  rcpt.form_type=Qry.FieldAsString("form_type");
  rcpt.no=Qry.FieldAsFloat("no");
  rcpt.pax_name=Qry.FieldAsString("pax_name");
  rcpt.pax_doc=Qry.FieldAsString("pax_doc");
  rcpt.service_type=Qry.FieldAsInteger("service_type");
  if (!Qry.FieldIsNULL("bag_type"))
    rcpt.bag_type=Qry.FieldAsInteger("bag_type");
  else
    rcpt.bag_type=-1;
  rcpt.bag_name=Qry.FieldAsString("bag_name");
  rcpt.tickets=Qry.FieldAsString("tickets");
  rcpt.prev_no=Qry.FieldAsString("prev_no");
  rcpt.airline=Qry.FieldAsString("airline");
  rcpt.aircode=Qry.FieldAsString("aircode");
  if (!Qry.FieldIsNULL("flt_no"))
    rcpt.flt_no=Qry.FieldAsInteger("flt_no");
  else
    rcpt.flt_no=-1;
  rcpt.suffix=Qry.FieldAsString("suffix");
  rcpt.scd_local_date=Qry.FieldIsNULL("scd_local_date")?NoExists:Qry.FieldAsDateTime("scd_local_date");
  rcpt.airp_dep=Qry.FieldAsString("airp_dep");
  rcpt.airp_arv=Qry.FieldAsString("airp_arv");
  if (!Qry.FieldIsNULL("ex_amount"))
    rcpt.ex_amount=Qry.FieldAsInteger("ex_amount");
  else
    rcpt.ex_amount=-1;
  if (!Qry.FieldIsNULL("ex_weight"))
    rcpt.ex_weight=Qry.FieldAsInteger("ex_weight");
  else
    rcpt.ex_weight=-1;
  if (!Qry.FieldIsNULL("value_tax"))
    rcpt.value_tax=Qry.FieldAsFloat("value_tax");
  else
    rcpt.value_tax=-1.0;
  rcpt.rate=Qry.FieldAsFloat("rate");
  rcpt.rate_cur=Qry.FieldAsString("rate_cur");
  rcpt.exch_rate=Qry.FieldAsInteger("exch_rate");
  rcpt.exch_pay_rate=Qry.FieldAsFloat("exch_pay_rate");
  rcpt.pay_rate_cur=Qry.FieldAsString("pay_rate_cur");
  rcpt.remarks=Qry.FieldAsString("remarks");
  rcpt.issue_date=Qry.FieldAsDateTime("issue_date");
  rcpt.issue_desk=Qry.FieldAsString("issue_desk");
  rcpt.issue_place=Qry.FieldAsString("issue_place");
  if (!Qry.FieldIsNULL("annul_date"))
    rcpt.annul_date=Qry.FieldAsDateTime("annul_date");
  else
    rcpt.annul_date=NoExists;
  rcpt.annul_desk=Qry.FieldAsString("annul_desk");

  rcpt.is_inter=Qry.FieldAsInteger("is_inter")!=0;
  rcpt.desk_lang=Qry.FieldAsString("desk_lang");

  if(Qry.FieldIsNULL("nds")) {
      rcpt.nds = NoExists;
      rcpt.nds_cur.clear();
  } else {
      rcpt.nds = Qry.FieldAsFloat("nds");
      rcpt.nds_cur = Qry.FieldAsString("nds_cur");
  }

  //��� ������
  rcpt.pay_types.clear();
  DB::TQuery PayTypesQry(PgOra::getROSession("BAG_PAY_TYPES"), STDLOG);
  PayTypesQry.SQLText=
    "SELECT * FROM bag_pay_types "
    "WHERE receipt_id=:receipt_id "
    "ORDER BY num";
  PayTypesQry.CreateVariable("receipt_id",otInteger,Qry.FieldAsInteger("receipt_id"));
  PayTypesQry.Execute();
  TBagPayType payType;
  for(;!PayTypesQry.Eof;PayTypesQry.Next())
  {
    payType.pay_type=PayTypesQry.FieldAsString("pay_type");
    payType.pay_rate_sum=PayTypesQry.FieldAsFloat("pay_rate_sum");
    payType.extra=PayTypesQry.FieldAsString("extra");
    rcpt.pay_types.push_back(payType);
  };

  rcpt.kit_id=Qry.FieldIsNULL("kit_id")?NoExists:Qry.FieldAsInteger("kit_id");
  rcpt.kit_num=Qry.FieldIsNULL("kit_num")?NoExists:Qry.FieldAsInteger("kit_num");
  rcpt.kit_items.clear();
  if (rcpt.kit_id!=NoExists)
  {
    DB::TQuery KitQry(PgOra::getROSession("BAG_RCPT_KITS"), STDLOG);
    KitQry.SQLText="SELECT * FROM bag_rcpt_kits "
                   "WHERE kit_id=:kit_id "
                   "ORDER BY kit_num";
    KitQry.CreateVariable("kit_id", otInteger, rcpt.kit_id);
    KitQry.Execute();
    for(int k=0;!KitQry.Eof;KitQry.Next(),k++)
    {
      if (k!=KitQry.FieldAsInteger("kit_num"))
        throw Exception("PaymentInterface::GetReceiptFromDB: Data integrity is broken (kit_id=%d)",rcpt.kit_id);
      TBagReceiptKitItem item;
      item.form_type=KitQry.FieldAsString("form_type");
      item.no=KitQry.FieldAsFloat("no");
      item.aircode=KitQry.FieldAsString("aircode");
      rcpt.kit_items.push_back(item);
    };
  };

  return true;
};

//�� �������� � ����

void saveBagReceipts(const TBagReceipt &rcpt, int point_id, int grp_id, int receipt_id, int user_id)
{
  DB::TQuery Qry(PgOra::getRWSession("BAG_RECEIPTS"), STDLOG);
  Qry.SQLText =
    "INSERT INTO bag_receipts ( "
    "receipt_id,point_id,grp_id,status,is_inter,desk_lang,form_type,no,pax_name,pax_doc,service_type,bag_type,bag_name, "
    "tickets,prev_no,airline,aircode,flt_no,suffix,scd_local_date,airp_dep,airp_arv,ex_amount,ex_weight,value_tax, "
    "rate,rate_cur,exch_rate,exch_pay_rate,pay_rate_cur,remarks, "
    "issue_date,issue_place,issue_user_id,issue_desk,kit_id,kit_num,nds,nds_cur "
    ") VALUES( "
    ":receipt_id,:point_id,:grp_id,:status,:is_inter,:desk_lang,:form_type,:no,:pax_name,:pax_doc,:service_type,:bag_type,:bag_name, "
    ":tickets,:prev_no,:airline,:aircode,:flt_no,:suffix,:scd_local_date,:airp_dep,:airp_arv,:ex_amount,:ex_weight,:value_tax, "
    ":rate,:rate_cur,:exch_rate,:exch_pay_rate,:pay_rate_cur,:remarks, "
    ":issue_date,:issue_place,:issue_user_id,:issue_desk,:kit_id,:kit_num,:nds,:nds_cur) ";
  Qry.CreateVariable("receipt_id",otInteger,receipt_id);
  Qry.CreateVariable("issue_date",otDate,NowUTC());
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.CreateVariable("status",otString,"�");
  Qry.CreateVariable("is_inter",otInteger,(int)rcpt.is_inter);
  Qry.CreateVariable("desk_lang",otString,rcpt.desk_lang);
  Qry.CreateVariable("form_type",otString,rcpt.form_type);
  Qry.CreateVariable("no",otFloat,rcpt.no);
  Qry.CreateVariable("pax_name",otString,rcpt.pax_name);
  Qry.CreateVariable("pax_doc",otString,rcpt.pax_doc);
  Qry.CreateVariable("service_type",otInteger,rcpt.service_type);
  if (rcpt.bag_type!=-1) {
    Qry.CreateVariable("bag_type",otInteger,rcpt.bag_type);
  } else {
    Qry.CreateVariable("bag_type",otInteger,FNull);
  }
  Qry.CreateVariable("bag_name",otString,rcpt.bag_name);
  Qry.CreateVariable("tickets",otString,rcpt.tickets);
  Qry.CreateVariable("prev_no",otString,rcpt.prev_no);

  Qry.CreateVariable("airline",otString,rcpt.airline);
  Qry.CreateVariable("aircode",otString,rcpt.aircode);
  if (rcpt.flt_no!=-1)
    Qry.CreateVariable("flt_no",otInteger,rcpt.flt_no);
  else
    Qry.CreateVariable("flt_no",otInteger,FNull);
  Qry.CreateVariable("suffix",otString,rcpt.suffix);
  rcpt.scd_local_date==NoExists?Qry.CreateVariable("scd_local_date",otDate,FNull):
                                Qry.CreateVariable("scd_local_date",otDate,rcpt.scd_local_date);
  Qry.CreateVariable("airp_dep",otString,rcpt.airp_dep);
  Qry.CreateVariable("airp_arv",otString,rcpt.airp_arv);
  if (rcpt.ex_amount!=-1) {
    Qry.CreateVariable("ex_amount",otInteger,rcpt.ex_amount);
  } else {
    Qry.CreateVariable("ex_amount",otInteger,FNull);
  }
  if (rcpt.ex_weight!=-1) {
    Qry.CreateVariable("ex_weight",otInteger,rcpt.ex_weight);
  } else {
    Qry.CreateVariable("ex_weight",otInteger,FNull);
  }
  if (rcpt.value_tax!=-1.0) {
    Qry.CreateVariable("value_tax",otFloat,rcpt.value_tax);
  } else {
    Qry.CreateVariable("value_tax",otFloat,FNull);
  }
  Qry.CreateVariable("rate",otFloat,rcpt.rate);
  Qry.CreateVariable("rate_cur",otString,rcpt.rate_cur);
  if (rcpt.exch_rate!=-1) {
    Qry.CreateVariable("exch_rate",otInteger,rcpt.exch_rate);
  } else {
    Qry.CreateVariable("exch_rate",otInteger,FNull);
  }
  if (rcpt.exch_pay_rate!=-1.0) {
    Qry.CreateVariable("exch_pay_rate",otFloat,rcpt.exch_pay_rate);
  } else {
    Qry.CreateVariable("exch_pay_rate",otFloat,FNull);
  }
  Qry.CreateVariable("pay_rate_cur",otString,rcpt.pay_rate_cur);
  Qry.CreateVariable("remarks",otString,rcpt.remarks);
  Qry.CreateVariable("issue_date",otDate,rcpt.issue_date);
  Qry.CreateVariable("issue_place",otString,rcpt.issue_place);
  Qry.CreateVariable("issue_user_id",otInteger,user_id);
  Qry.CreateVariable("issue_desk",otString,rcpt.issue_desk);
  if (rcpt.kit_id!=NoExists && rcpt.kit_num!=NoExists)
  {
    Qry.CreateVariable("kit_id",otInteger,rcpt.kit_id);
    Qry.CreateVariable("kit_num",otInteger,rcpt.kit_num);
  }
  else
  {
    Qry.CreateVariable("kit_id",otInteger,FNull);
    Qry.CreateVariable("kit_num",otInteger,FNull);
  }
  if(rcpt.nds == NoExists) {
      Qry.CreateVariable("nds", otFloat, FNull);
      Qry.CreateVariable("nds_cur", otString, FNull);
  } else {
      Qry.CreateVariable("nds", otFloat, rcpt.nds);
      Qry.CreateVariable("nds_cur", otString, rcpt.nds_cur);
  }
  try
  {
    Qry.Execute();
  }
  catch(const EOracleError& E)
  {
    if (E.Code==1)
      throw AstraLocale::UserException("MSG.RECEIPT_BLANK_NO_ALREADY_USED", LParams() << LParam("no", rcpt.no));
    else
      throw;
  }
}

void saveBagPayTypes(const TBagReceipt &rcpt, int receipt_id)
{
  DB::TQuery Qry(PgOra::getRWSession("BAG_PAY_TYPES"), STDLOG);
  Qry.SQLText=
    "INSERT INTO bag_pay_types( "
    "receipt_id,num,pay_type,pay_rate_sum,extra "
    ") VALUES ( "
    ":receipt_id,:num,:pay_type,:pay_rate_sum,:extra"
    ")";
  Qry.CreateVariable("receipt_id",otInteger,receipt_id);
  Qry.DeclareVariable("num",otInteger);
  Qry.DeclareVariable("pay_type",otString);
  Qry.DeclareVariable("pay_rate_sum",otFloat);
  Qry.DeclareVariable("extra",otString);
  int num=1;
  for(vector<TBagPayType>::const_iterator i=rcpt.pay_types.begin();i!=rcpt.pay_types.end();++i,num++)
  {
    Qry.SetVariable("num",num);
    Qry.SetVariable("pay_type",i->pay_type);
    Qry.SetVariable("pay_rate_sum",i->pay_rate_sum);
    Qry.SetVariable("extra",i->extra);
    Qry.Execute();
  }
}

void checkBagReceiptsDuplicates(const TBagReceipt &rcpt)
{
  DB::TQuery Qry(PgOra::getRWSession("BAG_RECEIPTS"), STDLOG);
  Qry.SQLText =
    "SELECT receipt_id FROM bag_receipts "
    "WHERE form_type=:form_type AND no=:no";
  Qry.DeclareVariable("form_type", otString);
  Qry.DeclareVariable("no", otFloat);
  for(vector<TBagReceiptKitItem>::const_iterator i=rcpt.kit_items.begin();i!=rcpt.kit_items.end();++i)
  {
    if (i->form_type==rcpt.form_type && i->no==rcpt.no) continue;
    Qry.SetVariable("form_type", i->form_type);
    Qry.SetVariable("no", i->no);
    Qry.Execute();
    if (!Qry.Eof) {
      throw AstraLocale::UserException("MSG.RECEIPT_BLANK_NO_ALREADY_USED", LParams() << LParam("no", i->no));
    }
  }
}

void updateBagReceipts_KitNum(int receipt_id, int kit_num)
{
  DB::TQuery Qry(PgOra::getRWSession("BAG_RECEIPTS"), STDLOG);
  Qry.SQLText =
    "UPDATE bag_receipts SET "
    "kit_id=:receipt_id, "
    "kit_num=:kit_num "
    "WHERE receipt_id=:receipt_id";
  Qry.CreateVariable("receipt_id", otInteger, receipt_id);
  Qry.CreateVariable("kit_num", otInteger, kit_num);
  Qry.Execute();
}

void saveBagRcptKits(const TBagReceipt &rcpt, int receipt_id)
{
  DB::TQuery Qry(PgOra::getRWSession("BAG_RCPT_KITS"), STDLOG);
  Qry.SQLText=
    "INSERT INTO bag_rcpt_kits( "
    "kit_id, kit_num, form_type, no, aircode "
    ") VALUES( "
    ":receipt_id, :kit_num, :form_type, :no, :aircode "
    ")";
  Qry.CreateVariable("receipt_id", otInteger, receipt_id);
  Qry.DeclareVariable("kit_num", otInteger);
  Qry.DeclareVariable("form_type", otString);
  Qry.DeclareVariable("no", otFloat);
  Qry.DeclareVariable("aircode", otString);
  int num=0;
  for(vector<TBagReceiptKitItem>::const_iterator i=rcpt.kit_items.begin();i!=rcpt.kit_items.end();++i,num++)
  {
    Qry.SetVariable("kit_num", num);
    Qry.SetVariable("form_type", i->form_type);
    Qry.SetVariable("no", i->no);
    Qry.SetVariable("aircode", i->aircode);
    Qry.Execute();
  }
}

int PaymentInterface::PutReceiptToDB(const TBagReceipt &rcpt, int point_id, int grp_id)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  const int user_id = reqInfo->user.user_id;
  const int receipt_id = PgOra::getSeqNextVal_int("CYCLE_ID__SEQ");
  saveBagReceipts(rcpt, point_id, grp_id, receipt_id, user_id);
  saveBagPayTypes(rcpt, receipt_id);

  //�஢�ઠ �㡫�஢���� ����஢ ���⠭権
  if (rcpt.kit_id==NoExists && rcpt.kit_num!=NoExists)
  {
    checkBagReceiptsDuplicates(rcpt);

    //���� �������� �������� � bag_rcpt_kits
    saveBagRcptKits(rcpt, receipt_id);
    updateBagReceipts_KitNum(receipt_id, rcpt.kit_num);
  }

  return receipt_id;
}


//�� �������� � XML
void PaymentInterface::PutReceiptToXML(const TBagReceipt &rcpt, int rcpt_id, bool pr_lat, xmlNodePtr rcptNode)
{
  if (rcptNode==NULL) return;

  if (rcpt_id!=-1)
    NewTextChild(rcptNode,"id",rcpt_id);
  TTagLang tag_lang;
  tag_lang.Init(rcpt, pr_lat);
  NewTextChild(rcptNode,"pr_lat",(int)(tag_lang.IsInter()));
  NewTextChild(rcptNode,"form_type",rcpt.form_type);
  NewTextChild(rcptNode,"no",rcpt.no);
  NewTextChild(rcptNode,"pax_name",rcpt.pax_name);
  NewTextChild(rcptNode,"pax_doc",rcpt.pax_doc);
  NewTextChild(rcptNode,"service_type",rcpt.service_type);
  if (rcpt.bag_type!=-1)
  {
    ostringstream s;
    s << setw(2) << setfill('0') << rcpt.bag_type;
    NewTextChild(rcptNode,"bag_type",s.str());
    s << ": " << ElemIdToNameLong(etBagType, rcpt.bag_type);
    NewTextChild(rcptNode,"bag_type_view",s.str());
  }
  else
    NewTextChild(rcptNode,"bag_type");
  NewTextChild(rcptNode,"bag_name",rcpt.bag_name,"");
  NewTextChild(rcptNode,"tickets",rcpt.tickets);
  NewTextChild(rcptNode,"prev_no",rcpt.prev_no,"");
  NewTextChild(rcptNode,"airline",rcpt.airline);
  NewTextChild(rcptNode,"aircode",rcpt.aircode);
  if (rcpt.flt_no!=-1)
  {
    NewTextChild(rcptNode,"flt_no",rcpt.flt_no);
    NewTextChild(rcptNode,"suffix",rcpt.suffix);
  };
  NewTextChild(rcptNode,"airp_dep",rcpt.airp_dep);
  NewTextChild(rcptNode,"airp_arv",rcpt.airp_arv);
  NewTextChild(rcptNode,"ex_amount",rcpt.ex_amount,-1);
  NewTextChild(rcptNode,"ex_weight",rcpt.ex_weight,-1);
  NewTextChild(rcptNode,"value_tax",rcpt.value_tax,-1.0);
  NewTextChild(rcptNode,"rate",rcpt.rate);
  NewTextChild(rcptNode,"rate_cur",rcpt.rate_cur);
  NewTextChild(rcptNode,"exch_rate",rcpt.exch_rate,-1);
  NewTextChild(rcptNode,"exch_pay_rate",rcpt.exch_pay_rate,-1.0);
  NewTextChild(rcptNode,"pay_rate_cur",rcpt.pay_rate_cur);
  NewTextChild(rcptNode,"remarks",rcpt.remarks,"");
  NewTextChild(rcptNode,"issue_date",DateTimeToStr(rcpt.issue_date));
  NewTextChild(rcptNode,"issue_place",rcpt.issue_place);
  if (rcpt.annul_date!=NoExists)
    NewTextChild(rcptNode,"annul_date",DateTimeToStr(rcpt.annul_date));

  xmlNodePtr payTypesNode=NewTextChild(rcptNode,"pay_types");
  for(vector<TBagPayType>::const_iterator i=rcpt.pay_types.begin();i!=rcpt.pay_types.end();++i)
  {
    xmlNodePtr node=NewTextChild(payTypesNode,"pay_type");
    NewTextChild(node,"pay_type",i->pay_type);
    NewTextChild(node,"pay_rate_sum",i->pay_rate_sum);
    NewTextChild(node,"extra",i->extra,"");
  };

  NewTextChild(rcptNode,"kit_id",rcpt.kit_id,NoExists);
  NewTextChild(rcptNode,"kit_num",rcpt.kit_num,NoExists);
  if (rcpt.kit_num!=NoExists)
  {
    xmlNodePtr kitItemsNode=NewTextChild(rcptNode,"kit_items");
    for(vector<TBagReceiptKitItem>::const_iterator i=rcpt.kit_items.begin();i!=rcpt.kit_items.end();++i)
    {
      xmlNodePtr node=NewTextChild(kitItemsNode,"item");
      NewTextChild(node,"form_type",i->form_type);
      NewTextChild(node,"no",i->no);
      NewTextChild(node,"aircode",i->aircode);
    };
  };
};

double PaymentInterface::GetCurrNo(int user_id, const string &form_type)
{
  DB::TQuery Qry(PgOra::getROSession({"FORM_TYPES", "FORM_PACKS"}), STDLOG);
  Qry.SQLText=
    "SELECT curr_no "
    "FROM form_types "
    "LEFT OUTER JOIN form_packs "
    "ON (form_types.code=form_packs.type AND form_packs.user_id=:user_id) "
    "WHERE form_types.code=:form_type";
  Qry.CreateVariable("form_type",otString,form_type);
  Qry.CreateVariable("user_id",otInteger,user_id);
  Qry.Execute();
  if (Qry.Eof)
    throw AstraLocale::UserException("MSG.BLANK_SET_PRINTING_UNAVAILABLE");
  if (!Qry.FieldIsNULL("curr_no"))
    return Qry.FieldAsFloat("curr_no");
  else
    return -1.0;
};

double PaymentInterface::GetNextNo(const string &form_type, double no)
{
  double result=-1.0;
  try
  {
    const TFormTypesRow &form=(const TFormTypesRow&)(base_tables.get("form_types").get_row("code",form_type));
    no+=1;
    ostringstream no_str;
    no_str << fixed << setw(form.no_len) << setfill('0') << setprecision(0) << no;
    if ((int)no_str.str().size()==form.no_len)
    {
      if ((int)form.code.size()>form.series_len)
      {
        if (no_str.str().find(form.code.substr(form.series_len))==0) result=no;
      }
      else result=no;
    };
  }
  catch(const EBaseTableError&) {};
  return result;
};

string GetKitPrevNoStr(const vector<TBagReceiptKitItem> &items)
{
  ostringstream result;
  vector<string> rcpts;
  for(vector<TBagReceiptKitItem>::const_iterator i=items.begin();i!=items.end();++i)
  {
    int no_len=10;
    try
    {
      no_len=base_tables.get("form_types").get_row("code",i->form_type).AsInteger("no_len");
    }
    catch(const EBaseTableError&) {};
    ostringstream no_str;
    no_str << fixed << setw(no_len) << setfill('0') << setprecision(0) << i->no;
    rcpts.push_back(no_str.str());
    if (i==items.begin()) result << i->aircode;
  };
  result << ::GetBagRcptStr(rcpts);
  return result.str();
};

namespace RCPT_TICKETS {
    void check(const string &form_type, const string &airline_fact, const string &airline_mark, const string &tickets)
    {
        if(form_type == FT_M61) {
            try {
                string buf;
                vector<string> lex;
                for(string::const_iterator is = tickets.begin(); is != tickets.end(); is++) {
                    if(*is == ' ')
                        throw "MSG.TICKETS.FORMAT_HINT";
                    if(*is != '/')
                        buf.push_back(*is);
                    else {
                        lex.push_back(buf);
                        buf.erase();
                    }
                }
                if(not buf.empty())
                    lex.push_back(buf);
                TAirlines &airlines = (TAirlines&)base_tables.get("airlines");
                string aircode_mark = airlines.get_row("code",airline_mark).AsString("aircode");
                string aircode_fact = airlines.get_row("code",airline_fact).AsString("aircode");
                ProgTrace(TRACE5, "aircode_mark: %s", aircode_mark.c_str());
                ProgTrace(TRACE5, "aircode_fact: %s", aircode_fact.c_str());
                if(
                        not lex.empty() and
                        (lex[0].substr(0, aircode_fact.size()) != aircode_fact) and
                        (lex[0].substr(0, aircode_mark.size()) != aircode_mark)
                  )
                    throw "MSG.TICKETS.AIRCODE_NOT_FOUND";
            } catch(const char *e) {
                throw UserException("MSG.TICKETS", LParams() << LParam("msg", getLocaleText(e)));
            }
        }
    }

    string get(const string &form_type, const string &tickets)
    {
        string result;
        if(form_type == FT_M61) {
            string buf;
            vector<string> lex;
            for(string::const_iterator is = tickets.begin(); is != tickets.end(); is++) {
                if(*is == ' ') continue;
                if(*is != '/')
                    buf.push_back(*is);
                else if( not buf.empty()) {
                    lex.push_back(buf);
                    buf.erase();
                }
            }
            if(not buf.empty())
                lex.push_back(buf);
            for(vector<string>::iterator iv = lex.begin(); iv != lex.end(); iv++) {
                if(iv == lex.begin())
                    result += *iv;
                else {
                    result += "/" + iv->substr(iv->size() - 2);
                }
            }
        } else
            result = tickets;
        return result;
    }
}

//�� XML � ��������
void PaymentInterface::GetReceiptFromXML(xmlNodePtr reqNode, TBagReceipt &rcpt)
{
  TReqInfo *reqInfo = TReqInfo::Instance();

  TPrnParams prnParams(reqNode);

  xmlNodePtr rcptNode=NodeAsNode("receipt",reqNode);

  int grp_id=NoExists;
  if (GetNode("kit_id",rcptNode)!=NULL)
  {
    //⥫��ࠬ�� �� ��������
    int kit_id=NodeAsInteger("kit_id",rcptNode);
    DB::TQuery Qry(PgOra::getROSession("BAG_RECEIPTS"), STDLOG);
    Qry.SQLText="SELECT * FROM bag_receipts "
                "WHERE kit_id=:kit_id "
                "FETCH FIRST 1 ROWS ONLY ";
    Qry.CreateVariable("kit_id",otInteger,kit_id);
    Qry.Execute();
    if (Qry.Eof)
      throw AstraLocale::UserException("MSG.FLT_OR_PAX_INFO_CHANGED.REFRESH_DATA");
    GetReceiptFromDB(Qry,rcpt);
    if (Qry.FieldIsNULL("grp_id"))
      throw AstraLocale::UserException("MSG.FLT_OR_PAX_INFO_CHANGED.REFRESH_DATA");
    grp_id=Qry.FieldAsInteger("grp_id");
    rcpt.kit_num=NodeAsInteger("kit_num",rcptNode); //��易⥫쭮 ���������
  }
  else
  {
    rcpt.kit_id=NoExists;
    grp_id=NodeAsInteger("grp_id",reqNode);
    if (GetNode("kit_num",rcptNode)!=NULL)
      rcpt.kit_num=NodeAsInteger("kit_num",rcptNode);
    else
      rcpt.kit_num=NoExists;
    rcpt.kit_items.clear();
    if (rcpt.kit_num!=NoExists)
    {
      //⥫��ࠬ�� �� ��������, �� ��. �������� �� �� ��।����
      //� �⮬ ��砥 ������ ������� ᯨ᮪ ����஢, �������� � ��������
      xmlNodePtr node=NodeAsNode("kit_items",rcptNode)->children;
      for(;node!=NULL;node=node->next)
      {
        TBagReceiptKitItem item;
        item.form_type=NodeAsString("form_type",node);
        item.no=NodeAsFloat("no",node);
        item.aircode=NodeAsString("aircode",node);
        rcpt.kit_items.push_back(item);
      };
    };
  };

  TTrferRoute route;
  if (!route.GetRoute(grp_id, trtWithFirstSeg) || route.empty())
    throw AstraLocale::UserException("MSG.FLT_OR_PAX_INFO_CHANGED.REFRESH_DATA");

  DB::TQuery Qry(PgOra::getROSession({"PAX_GRP","MARK_TRIPS"}), STDLOG);
  Qry.SQLText=
    "SELECT point_dep, point_arv, pr_mark_norms, "
    "       mark_trips.airline AS airline_mark, "
    "       mark_trips.flt_no AS flt_no_mark, "
    "       mark_trips.suffix AS suffix_mark "
    "FROM pax_grp,mark_trips "
    "WHERE pax_grp.point_id_mark=mark_trips.point_id AND "
    "      pax_grp.grp_id=:grp_id";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.Execute();
  if (Qry.Eof)
    throw AstraLocale::UserException("MSG.FLT_OR_PAX_INFO_CHANGED.REFRESH_DATA");

  string airline_mark = Qry.FieldAsString("airline_mark");
  string airline_fact;

  if (rcpt.kit_num==NoExists || rcpt.kit_num==0)
  {
    if (Qry.FieldAsInteger("pr_mark_norms")!=0)
    {
      rcpt.airline=Qry.FieldAsString("airline_mark");
      rcpt.flt_no=Qry.FieldAsInteger("flt_no_mark");
      rcpt.suffix=Qry.FieldAsString("suffix_mark");
    }
    else
    {
      rcpt.airline=route.begin()->operFlt.airline;
      rcpt.flt_no=route.begin()->operFlt.flt_no;
      rcpt.suffix=route.begin()->operFlt.suffix;

    };
    airline_fact = route.begin()->operFlt.airline;
    rcpt.scd_local_date=route.begin()->operFlt.scd_out; //TTrferRoute ᮤ�ন� ������� ����
    rcpt.airp_dep=route.begin()->operFlt.airp;
    rcpt.airp_arv=route.begin()->airp_arv;
  }
  else
  {
    if (!(rcpt.kit_num>=0 && rcpt.kit_num<(int)route.size()))
      throw AstraLocale::UserException("MSG.FLT_OR_PAX_INFO_CHANGED.REFRESH_DATA");
    const TTrferRouteItem &item=route.at(rcpt.kit_num);
    rcpt.airline=item.operFlt.airline;
    rcpt.flt_no=item.operFlt.flt_no;
    rcpt.suffix=item.operFlt.suffix;
    rcpt.scd_local_date=item.operFlt.scd_out;
    rcpt.airp_dep=item.operFlt.airp;
    rcpt.airp_arv=item.airp_arv;
    airline_fact = item.operFlt.airline;
  };

  if (rcpt.kit_num==NoExists)
  {
    rcpt.aircode=base_tables.get("airlines").get_row("code", rcpt.airline).AsString("aircode");
    rcpt.form_type=NodeAsString("form_type",rcptNode);
    if (rcpt.form_type.empty())
      throw AstraLocale::UserException("MSG.RECEIPT_BLANK_TYPE_NOT_SET");
    if (NodeIsNULL("no",rcptNode) )
      //�ॢ�� � ��������� ����஬ ���⠭樨 (� �.�. ��ࢮ� �ॢ��)
      rcpt.no=GetCurrNo(reqInfo->user.user_id,rcpt.form_type);
    else
      rcpt.no=NodeAsFloat("no",rcptNode);
  }
  else
  {
    if (!(rcpt.kit_num>=0 && rcpt.kit_num<(int)rcpt.kit_items.size()))
      throw AstraLocale::UserException("MSG.FLT_OR_PAX_INFO_CHANGED.REFRESH_DATA");
    const TBagReceiptKitItem &item=rcpt.kit_items.at(rcpt.kit_num);
    rcpt.aircode=item.aircode;
    rcpt.form_type=item.form_type;
    rcpt.no=item.no;
  };

  if (rcpt.kit_id==NoExists)
  {
    TTagLang tag_lang;
    //�� �� ��⠭�����
    tag_lang.Init(rcpt.airp_dep,
                  rcpt.airp_arv,
                  prnParams.pr_lat);
    rcpt.is_inter=tag_lang.IsInter();  //�����, �� ���樠�����㥬 ���砫�
    rcpt.route_country = tag_lang.getRouteCountry();
    rcpt.desk_lang=tag_lang.GetLang();

    if (NodeIsNULL("no",rcptNode) )
      //�ॢ�� � ��������� ����஬ ���⠭樨 (� �.�. ��ࢮ� �ॢ��)
      rcpt.pax_name=RCPT_PAX_NAME::transliter_pax_name(tag_lang, NodeAsString("pax_name",rcptNode));
    else {
      rcpt.pax_name=NodeAsString("pax_name",rcptNode);
      RCPT_PAX_NAME::check_pax_name(rcpt.form_type, rcpt.pax_name);
    }

    if (NodeIsNULL("no",rcptNode) )
        rcpt.pax_doc=RCPT_PAX_DOC::transliter(tag_lang.GetLang(), NodeAsString("pax_doc",rcptNode));
    else {
        rcpt.pax_doc=NodeAsString("pax_doc",rcptNode);
        RCPT_PAX_DOC::check(rcpt.form_type, rcpt.pax_doc);
    }
    rcpt.bag_name=NodeAsString("bag_name",rcptNode);
    if (NodeIsNULL("no",rcptNode) )
        rcpt.tickets=RCPT_TICKETS::get(rcpt.form_type, NodeAsString("tickets",rcptNode));
    else {
        rcpt.tickets=NodeAsString("tickets",rcptNode);
        RCPT_TICKETS::check(rcpt.form_type, airline_fact, airline_mark, rcpt.tickets);
    }
    if (rcpt.kit_num!=NoExists)
      rcpt.prev_no=GetKitPrevNoStr(rcpt.kit_items).substr(0,50);
    else
    {
      rcpt.prev_no=NodeAsString("prev_no",rcptNode);
      if (NodeIsNULL("no",rcptNode) && !rcpt.prev_no.empty())
      {
        //�஢�ਬ �� �� �� ����� EMD
        CheckIn::TServicePaymentListWithAuto payment;
        payment.fromDB(grp_id);
        for(CheckIn::TServicePaymentListWithAuto::const_iterator i=payment.begin(); i!=payment.end(); ++i)
        {
          const CheckIn::TServicePaymentItem &item=*i;
          if (item.trfer_num!=0) continue;
          if (item.isEMD() && rcpt.prev_no==item.emd_no_str())
          {
            rcpt.prev_no.clear();
            break;
          };
        };
      };
    };

    rcpt.service_type=NodeAsInteger("service_type",rcptNode);
    if (!NodeIsNULL("bag_type",rcptNode))
      rcpt.bag_type=NodeAsInteger("bag_type",rcptNode);
    else
      rcpt.bag_type=-1;

    if (rcpt.service_type!=3)
    {
      rcpt.ex_amount=NodeAsInteger("ex_amount",rcptNode);
      rcpt.ex_weight=NodeAsInteger("ex_weight",rcptNode);
      rcpt.value_tax=-1.0;
    }
    else
    {
      rcpt.ex_amount=-1;
      rcpt.ex_weight=-1;
      rcpt.value_tax=NodeAsFloat("value_tax",rcptNode);
    };
    rcpt.rate=NodeAsFloat("rate",rcptNode);
    rcpt.rate_cur=NodeAsString("rate_cur",rcptNode);
    rcpt.pay_rate_cur=NodeAsString("pay_rate_cur",rcptNode);
    if (rcpt.rate_cur!=rcpt.pay_rate_cur)
    {
      rcpt.exch_rate=NodeAsInteger("exch_rate",rcptNode);
      rcpt.exch_pay_rate=NodeAsFloat("exch_pay_rate",rcptNode);
    }
    else
    {
      rcpt.exch_rate=-1;
      rcpt.exch_pay_rate=-1.0;
    };
    // ���
    if(rcpt.service_type == 1 and rcpt.route_country == "��") {
        rcpt.nds = rcpt.rate_sum() / 110. * 10.;
        rcpt.nds_cur = rcpt.rate_cur;
    } else {
        rcpt.nds = NoExists;
        rcpt.nds_cur.clear();
    }
    //��� ������
    rcpt.pay_types.clear();
    unsigned int none_count=0;
    unsigned int cash_count=0;

    xmlNodePtr node=NodeAsNode("pay_types",rcptNode)->children;
    for(;node!=NULL;node=node->next)
    {
      TBagPayType payType;
      payType.pay_type=NodeAsString("pay_type",node);
      payType.pay_rate_sum=NodeAsFloat("pay_rate_sum",node);
      payType.extra=NodeAsString("extra",node);
      rcpt.pay_types.push_back(payType);

      if (payType.pay_type==NONE_PAY_TYPE_ID) none_count++;
      if (payType.pay_type==CASH_PAY_TYPE_ID) cash_count++;
    };
    if (rcpt.pay_types.size() > 2)
      throw AstraLocale::UserException("MSG.PAY_TYPE.NO_MORE_2");
    if (none_count > 1)
      throw AstraLocale::UserException("MSG.PAY_TYPE.ONLY_ONCE", LParams() << LParam("pay_type", ElemIdToCodeNative(etPayType,NONE_PAY_TYPE_ID)));
    if (none_count > 0 && rcpt.pay_types.size() > none_count)
      throw AstraLocale::UserException("MSG.PAY_TYPE.NO_MIX", LParams() << LParam("pay_type", ElemIdToCodeNative(etPayType,NONE_PAY_TYPE_ID)));
    if (cash_count > 1)
      throw AstraLocale::UserException("MSG.PAY_TYPE.ONLY_ONCE", LParams() << LParam("pay_type", ElemIdToCodeNative(etPayType,CASH_PAY_TYPE_ID)));


    if (rcpt.pay_types.empty())
    {
      //�� ������ ��� ������ - ᪮॥ �ᥣ� ��ࢮ� �ॢ��
      TBagPayType payType;
      payType.pay_type=CASH_PAY_TYPE_ID;
      payType.pay_rate_sum=CalcPayRateSum(rcpt);
      payType.extra="";
      rcpt.pay_types.push_back(payType);
    };
  };

  rcpt.remarks=""; //�ନ����� ������

  rcpt.issue_date=NowUTC();
  rcpt.issue_desk=reqInfo->desk.code;
  rcpt.issue_place=get_validator(rcpt, prnParams.pr_lat);

  rcpt.annul_date=NoExists;
  rcpt.annul_desk="";
}

void PaymentInterface::PutReceiptFields(const TBagReceipt &rcpt, bool pr_lat, xmlNodePtr node)
{
  if (node==NULL) return;

  PrintDataParser parser(rcpt, pr_lat);

  //�ନ�㥬 � �⢥� ��ࠧ ⥫��ࠬ��
  xmlNodePtr fieldsNode=NewTextChild(node,"fields");
  NewTextChild(fieldsNode,"pax_doc",         parser.pts.get_tag_no_err(TAG::PAX_DOC));
  NewTextChild(fieldsNode,"pax_name",        parser.pts.get_tag_no_err(TAG::PAX_NAME));
  NewTextChild(fieldsNode,"tickets",         parser.pts.get_tag_no_err(TAG::TICKETS));
  NewTextChild(fieldsNode,"prev_no",         parser.pts.get_tag_no_err(TAG::PREV_NO));
  NewTextChild(fieldsNode,"aircode",         parser.pts.get_tag_no_err(TAG::AIRCODE));
  NewTextChild(fieldsNode,"issue_place1",    parser.pts.get_tag_no_err(TAG::ISSUE_PLACE1));
  NewTextChild(fieldsNode,"issue_place2",    parser.pts.get_tag_no_err(TAG::ISSUE_PLACE2));
  NewTextChild(fieldsNode,"issue_place3",    parser.pts.get_tag_no_err(TAG::ISSUE_PLACE3));
  NewTextChild(fieldsNode,"issue_place4",    parser.pts.get_tag_no_err(TAG::ISSUE_PLACE4));
  NewTextChild(fieldsNode,"issue_place5",    parser.pts.get_tag_no_err(TAG::ISSUE_PLACE5));
  NewTextChild(fieldsNode,"SkiBT",           parser.pts.get_tag_no_err(TAG::SKI_BT));
  NewTextChild(fieldsNode,"GolfBT",          parser.pts.get_tag_no_err(TAG::GOLF_BT));
  NewTextChild(fieldsNode,"PetBT",           parser.pts.get_tag_no_err(TAG::PET_BT));
  NewTextChild(fieldsNode,"BulkyBT",         parser.pts.get_tag_no_err(TAG::BULKY_BT));
  NewTextChild(fieldsNode,"BulkyBTLetter",   parser.pts.get_tag_no_err(TAG::BULKY_BT_LETTER));
  NewTextChild(fieldsNode,"OtherBT",         parser.pts.get_tag_no_err(TAG::OTHER_BT));
  NewTextChild(fieldsNode,"ValueBT",         parser.pts.get_tag_no_err(TAG::VALUE_BT));
  NewTextChild(fieldsNode,"ValueBTLetter",   parser.pts.get_tag_no_err(TAG::VALUE_BT_LETTER));
  NewTextChild(fieldsNode,"OtherBTLetter",   parser.pts.get_tag_no_err(TAG::OTHER_BT_LETTER));
  NewTextChild(fieldsNode,"service_type",    parser.pts.get_tag_no_err(TAG::SERVICE_TYPE));
  NewTextChild(fieldsNode,"bag_name",        parser.pts.get_tag_no_err(TAG::BAG_NAME));
  NewTextChild(fieldsNode,"amount_letters",  parser.pts.get_tag_no_err(TAG::AMOUNT_LETTERS));
  NewTextChild(fieldsNode,"amount_figures",  parser.pts.get_tag_no_err(TAG::AMOUNT_FIGURES));
  NewTextChild(fieldsNode,"currency",        parser.pts.get_tag_no_err(TAG::CURRENCY));
  NewTextChild(fieldsNode,"issue_date",      parser.pts.get_tag_no_err(TAG::ISSUE_DATE, "ddmmmyy"));
  NewTextChild(fieldsNode,"to",              parser.pts.get_tag_no_err(TAG::TO));
  NewTextChild(fieldsNode,"remarks1",        parser.pts.get_tag_no_err(TAG::REMARKS1));
  NewTextChild(fieldsNode,"remarks2",        parser.pts.get_tag_no_err(TAG::REMARKS2));
  NewTextChild(fieldsNode,"remarks3",        parser.pts.get_tag_no_err(TAG::REMARKS3));
  NewTextChild(fieldsNode,"remarks4",        parser.pts.get_tag_no_err(TAG::REMARKS4));
  NewTextChild(fieldsNode,"remarks5",        parser.pts.get_tag_no_err(TAG::REMARKS5));
  NewTextChild(fieldsNode,"exchange_rate",   parser.pts.get_tag_no_err(TAG::EXCHANGE_RATE));
  NewTextChild(fieldsNode,"total",           parser.pts.get_tag_no_err(TAG::TOTAL));
  NewTextChild(fieldsNode,"airline",         parser.pts.get_tag_no_err(TAG::AIRLINE));
  NewTextChild(fieldsNode,"airline_code",    parser.pts.get_tag_no_err(TAG::AIRLINE_CODE));
  NewTextChild(fieldsNode,"point_dep",       parser.pts.get_tag_no_err(TAG::POINT_DEP));
  NewTextChild(fieldsNode,"point_arv",       parser.pts.get_tag_no_err(TAG::POINT_ARV));
  NewTextChild(fieldsNode,"rate",            parser.pts.get_tag_no_err(TAG::RATE));
  NewTextChild(fieldsNode,"charge",          parser.pts.get_tag_no_err(TAG::CHARGE));
  NewTextChild(fieldsNode,"ex_weight",       parser.pts.get_tag_no_err(TAG::EX_WEIGHT));
  NewTextChild(fieldsNode,"pay_form",        parser.pts.get_tag_no_err(TAG::PAY_FORM));

  if (rcpt.no!=-1.0)
  {
    ostringstream no;
    no << fixed << setw(10) << setfill('0') << setprecision(0) << rcpt.no;
    NewTextChild(fieldsNode,"no",no.str());
  }
  else
  {
    NewTextChild(fieldsNode,"no");
  };
  NewTextChild(fieldsNode,"annul_str");
};

void PaymentInterface::PutReceiptFields(int id, bool pr_lat, xmlNodePtr node)
{
  if (node==NULL) return;

  TBagReceipt rcpt;

  DB::TQuery Qry(PgOra::getROSession("BAG_RECEIPTS"), STDLOG);
  Qry.SQLText="SELECT * FROM bag_receipts "
              "WHERE receipt_id=:id";
  Qry.CreateVariable("id",otInteger,id);
  Qry.Execute();
  if (Qry.Eof)
      throw AstraLocale::UserException("MSG.RECEIPT_NOT_FOUND.REFRESH_DATA");

  GetReceiptFromDB(Qry,rcpt);
  PutReceiptToXML(rcpt,id,pr_lat,node);
  PutReceiptFields(rcpt,pr_lat,node);

  string status=Qry.FieldAsString("status");

  if (rcpt.annul_date!=NoExists && !rcpt.annul_desk.empty())
  {
    TDateTime annul_date_local = UTCToLocal(rcpt.annul_date, CityTZRegion(DeskCity(rcpt.annul_desk)));
    ostringstream annul_str;

    if (status=="�")
      annul_str << getLocaleText("MSG.RECEIPT.REPLACEMENT") << " "
                << DateTimeToStr(annul_date_local, (string)"ddmmmyy", TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU);
    if (status=="�")
      annul_str << getLocaleText("MSG.RECEIPT.REFUND") << " "
                << DateTimeToStr(annul_date_local, (string)"ddmmmyy", TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU);
    ReplaceTextChild(NodeAsNode("fields",node),"annul_str",annul_str.str());
  }
}

void PaymentInterface::ViewReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  if (TReqInfo::Instance()->desk.compatible(PAX_SERVICE_VERSION))
    throw UserException("MSG.TERM_VERSION.NOT_SUPPORTED");

  xmlNodePtr rcptNode=NewTextChild(resNode,"receipt");

  TPrnParams prnParams(reqNode);

  if (GetNode("receipt/id",reqNode)==NULL)
  {
    //��� ��� �믮������ �� �뢮�� �� �� �����⠭��� ���⠭樨
    TBagReceipt rcpt;
    GetReceiptFromXML(reqNode,rcpt);
    PutReceiptToXML(rcpt,-1,prnParams.pr_lat,rcptNode);
    PutReceiptFields(rcpt,prnParams.pr_lat,rcptNode);
  }
  else
  {
    //��� ��� �믮������ �� �뢮�� �����⠭��� ���⠭樨
    PutReceiptFields(NodeAsInteger("receipt/id",reqNode),prnParams.pr_lat,rcptNode);
  };
};

void PaymentInterface::ReplaceReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->desk.compatible(PAX_SERVICE_VERSION))
    throw UserException("MSG.TERM_VERSION.NOT_SUPPORTED");

  TBagReceipt rcpt;
  int rcpt_id=NodeAsInteger("receipt/id",reqNode);

  if (!GetReceiptFromDB(rcpt_id,rcpt))
    throw AstraLocale::UserException("MSG.RECEIPT_TO_REPLACE_NOT_FOUND.REFRESH_DATA");

  if (rcpt.kit_id!=NoExists)
    //�� �।�ᬮ�७� ��楤�� ������ ������ ��� ���⠭権 �� ��������
    throw AstraLocale::UserException("MSG.RECEIPT_REPLACEMENT_DENIAL");

  TPrnParams prnParams(reqNode);

  PutReceiptToXML(rcpt,rcpt_id,prnParams.pr_lat,NewTextChild(resNode,"receipt"));

  rcpt.no=GetCurrNo(reqInfo->user.user_id,rcpt.form_type);
  rcpt.issue_date=NowUTC();
  rcpt.issue_desk=reqInfo->desk.code;
  rcpt.issue_place=get_validator(rcpt, prnParams.pr_lat);
  rcpt.annul_date=NoExists;
  rcpt.annul_desk="";

  xmlNodePtr rcptNode=NewTextChild(resNode,"new_receipt");
  PutReceiptToXML(rcpt,-1,prnParams.pr_lat,rcptNode);
  PutReceiptFields(rcpt,prnParams.pr_lat,rcptNode); //��ࠧ ���⠭樨
};

void PaymentInterface::AnnulReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->desk.compatible(PAX_SERVICE_VERSION))
    throw UserException("MSG.TERM_VERSION.NOT_SUPPORTED");

  TBagReceipt rcpt;

  int point_dep=NoExists;
  int grp_id=NoExists;
  if (GetNode("point_dep",reqNode)!=NULL)
  {
    //���⠭�� �ਢ易�� � ��㯯�
    point_dep=NodeAsInteger("point_dep",reqNode);
    grp_id=NodeAsInteger("grp_id",reqNode);
    int tid=LockAndUpdTid(point_dep,grp_id,NodeAsInteger("tid",reqNode));
    NewTextChild(resNode,"tid",tid);
  };

  TPrnParams prnParams(reqNode);

  vector<int> rcpt_ids;
  xmlNodePtr rcptsNode;
  string new_status=NodeAsInteger("replacement",reqNode)!=0?"�":"�";
  xmlNodePtr node=NodeAsNode("receipt",reqNode)->children;
  for(;node!=NULL;node=node->next)
    rcpt_ids.push_back(NodeAsInteger(node));
  rcptsNode=NewTextChild(resNode,"receipts");

  for(vector<int>::const_iterator rcpt_id=rcpt_ids.begin();rcpt_id!=rcpt_ids.end();++rcpt_id)
  {
    if (GetNode("point_dep",reqNode)==NULL)
    {
      //���⠭�� �� �ਢ易�� � ��㯯�
      DB::TQuery Qry(PgOra::getROSession("BAG_RECEIPTS"), STDLOG);
      Qry.SQLText="SELECT point_id FROM bag_receipts "
                  "WHERE receipt_id=:receipt_id";
      Qry.CreateVariable("receipt_id",otInteger,*rcpt_id);
      Qry.Execute();
      if (Qry.Eof)
      {
        if (rcpt_ids.size()>1)
          throw AstraLocale::UserException("MSG.RECEIPTS_TO_ANNUL_NOT_FOUND.REFRESH_DATA");
        else
          throw AstraLocale::UserException("MSG.RECEIPT_TO_ANNUL_NOT_FOUND.REFRESH_DATA");
      };
      point_dep=Qry.FieldAsInteger("point_id");
    };

    DB::TQuery Qry(PgOra::getRWSession("BAG_RECEIPTS"), STDLOG);
    Qry.SQLText=
        "UPDATE bag_receipts SET "
        "status=:new_status, "
        "annul_date=:annul_date, "
        "annul_user_id=:user_id, "
        "annul_desk=:desk "
        "WHERE receipt_id=:receipt_id "
        "AND status=:old_status";
    Qry.CreateVariable("receipt_id",otInteger,*rcpt_id);
    Qry.CreateVariable("old_status",otString,"�");
    Qry.CreateVariable("new_status",otString,new_status);
    Qry.CreateVariable("annul_date",otDate,NowUTC());
    Qry.CreateVariable("user_id",otInteger,reqInfo->user.user_id);
    Qry.CreateVariable("desk",otString,reqInfo->desk.code);
    Qry.Execute();
    if (Qry.RowsProcessed()<=0)
    {
      if (rcpt_ids.size()>1)
        throw AstraLocale::UserException("MSG.RECEIPTS_TO_ANNUL_CHANGED.REFRESH_DATA");
      else
        throw AstraLocale::UserException("MSG.RECEIPT_TO_ANNUL_CHANGED.REFRESH_DATA");
    };

    if (!GetReceiptFromDB(*rcpt_id,rcpt))
    {
      if (rcpt_ids.size()>1)
          throw AstraLocale::UserException("MSG.RECEIPTS_TO_ANNUL_NOT_FOUND.REFRESH_DATA");
        else
          throw AstraLocale::UserException("MSG.RECEIPT_TO_ANNUL_NOT_FOUND.REFRESH_DATA");
    };

    PutReceiptToXML(rcpt,*rcpt_id,prnParams.pr_lat,NewTextChild(rcptsNode,"receipt"));

    ostringstream logmsg;
    logmsg << fixed << setw(10) << setfill('0') << setprecision(0) << rcpt.no;
    reqInfo->LocaleToLog("EVT.RCPT_ANNUL", LEvntPrms() << PrmSmpl<std::string>("rcpt", rcpt.form_type)
                         << PrmSmpl<std::string>("num", logmsg.str()), ASTRA::evtPay,
                                   point_dep!=NoExists?point_dep:0, 0, grp_id!=NoExists?grp_id:0);
  };
};

void PaymentInterface::PrintReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    if (reqInfo->desk.compatible(PAX_SERVICE_VERSION))
      throw UserException("MSG.TERM_VERSION.NOT_SUPPORTED");

    int point_dep=NodeAsInteger("point_dep",reqNode);
    int grp_id=NodeAsInteger("grp_id",reqNode);
    int tid=LockAndUpdTid(point_dep,grp_id,NodeAsInteger("tid",reqNode));
    NewTextChild(resNode,"tid",tid);

    TPrnParams prnParams(reqNode);

    TBagReceipt rcpt;
    int rcpt_id;
    xmlNodePtr rcptNode;
    std::string lexema_id;
    LEvntPrms params;
    if (GetNode("receipt/no",reqNode)!=NULL)
    {
        if (GetNode("receipt/id",reqNode)==NULL)
        {
            //����� ����� ���⠭樨
            GetReceiptFromXML(reqNode,rcpt);
            rcptNode=NewTextChild(resNode,"receipt");
            ostringstream logmsg;
            logmsg << fixed << setw(10) << setfill('0') << setprecision(0) << rcpt.no;
            lexema_id = "EVT.RCPT_PRINT";
            params << PrmSmpl<std::string>("rcpt", rcpt.form_type)
                   << PrmSmpl<std::string>("num", logmsg.str());
        }
        else
        {
            //������ ������
            rcpt_id=NodeAsInteger("receipt/id",reqNode);

            DB::TQuery Qry(PgOra::getRWSession("BAG_RECEIPTS"), STDLOG);
            Qry.SQLText=
                "UPDATE bag_receipts SET "
                "status=:new_status, "
                "annul_date=:annul_date, "
                "annul_user_id=:user_id, "
                "annul_desk=:desk "
                "WHERE receipt_id=:receipt_id "
                "AND status=:old_status";
            Qry.CreateVariable("receipt_id",otInteger,rcpt_id);
            Qry.CreateVariable("old_status",otString,"�");
            Qry.CreateVariable("new_status",otString,"�");
            Qry.CreateVariable("annul_date",otDate,NowUTC());
            Qry.CreateVariable("user_id",otInteger,reqInfo->user.user_id);
            Qry.CreateVariable("desk",otString,reqInfo->desk.code);
            Qry.Execute();
            if (Qry.RowsProcessed()<=0)
                throw AstraLocale::UserException("MSG.RECEIPT_TO_REPLACE_CHANGED.REFRESH_DATA");

            if (!GetReceiptFromDB(rcpt_id,rcpt))
                throw AstraLocale::UserException("MSG.RECEIPT_TO_REPLACE_NOT_FOUND.REFRESH_DATA");

            if (rcpt.kit_id!=NoExists)
              //�� �।�ᬮ�७� ��楤�� ������ ������ ��� ���⠭権 �� ��������
              throw AstraLocale::UserException("MSG.RECEIPT_REPLACEMENT_DENIAL");

            PutReceiptToXML(rcpt,rcpt_id,prnParams.pr_lat,NewTextChild(resNode,"receipt"));

            double old_no=rcpt.no;
            rcpt.no=NodeAsFloat("receipt/no",reqNode);
            rcpt.issue_date=rcpt.annul_date;
            rcpt.issue_desk=reqInfo->desk.code;
            rcpt.issue_place=get_validator(rcpt, prnParams.pr_lat);
            rcpt.annul_date=NoExists;
            rcpt.annul_desk="";

            rcptNode=NewTextChild(resNode,"new_receipt");
            ostringstream logmsg1, logmsg2;
            logmsg1 << fixed << setw(10) << setfill('0') << setprecision(0) << old_no;
            logmsg2 << fixed << setw(10) << setfill('0') << setprecision(0) << rcpt.no;
            lexema_id = "EVT.RCPT_REPLACE";
            params << PrmSmpl<std::string>("rcpt", rcpt.form_type)
                   << PrmSmpl<std::string>("old_num", logmsg1.str())
                   << PrmSmpl<std::string>("num", logmsg2.str());
        };

        rcpt_id=PutReceiptToDB(rcpt,point_dep,grp_id);  //������ ���⠭樨 � ����
        createSofiFileDATA( rcpt_id );
        //�뢮�
        if (!GetReceiptFromDB(rcpt_id,rcpt)) //�����⠥� �� ���� ��-�� kit_id
          throw AstraLocale::UserException("MSG.RECEIPT_NOT_FOUND.REFRESH_DATA");
        PutReceiptToXML(rcpt,rcpt_id,prnParams.pr_lat,rcptNode); //���⠭��
        PutReceiptFields(rcpt,prnParams.pr_lat,rcptNode); //��ࠧ ���⠭樨

        double next_no=GetNextNo(rcpt.form_type, rcpt.no);
        //�����塞 ����� ������ � ��窥
        DB::TQuery Qry(PgOra::getRWSession("FORM_PACKS"), STDLOG);
        Qry.CreateVariable("user_id",otInteger,reqInfo->user.user_id);
        Qry.CreateVariable("type",otString,rcpt.form_type);
        if (next_no!=-1.0)
        {
            Qry.CreateVariable("next_no",otFloat,next_no);
            Qry.SQLText = "INSERT INTO form_packs(user_id,curr_no,type) "
                          "VALUES(:user_id,:next_no,:type) "
                          "ON CONFLICT (user_id,type) DO UPDATE "
                          "SET curr_no=:next_no";
        }
        else
        {
            Qry.CreateVariable("next_no",otFloat,FNull);
            Qry.SQLText = "DELETE FROM form_packs WHERE user_id=:user_id AND type=:type";
        }
        Qry.Execute();
    }
    else
    {
        //����� ����
        rcpt_id=NodeAsInteger("receipt/id",reqNode);
        if (!GetReceiptFromDB(rcpt_id,rcpt))
          throw AstraLocale::UserException("MSG.RECEIPT_NOT_FOUND.REFRESH_DATA");
        rcptNode=NewTextChild(resNode,"receipt");
        ostringstream logmsg;
        logmsg << fixed << setw(10) << setfill('0') << setprecision(0) << rcpt.no;
        lexema_id = "EVT.RCPT_PRINT_REPEAT";
        params << PrmSmpl<std::string>("rcpt", rcpt.form_type)
               << PrmSmpl<std::string>("num", logmsg.str());
    };

    //��᫥����⥫쭮��� ��� �ਭ��
    PrintDataParser parser(rcpt, prnParams.pr_lat);
    string data;
    bool hex;
    PrintInterface::GetPrintDataBR(
            rcpt.form_type,
            parser,
            data,
            hex,
            reqNode
            ); //��᫥����⥫쭮��� ��� �ਭ��
    SetProp( NewTextChild(rcptNode, "form", data), "hex", (int)hex);
    reqInfo->LocaleToLog(lexema_id, params, ASTRA::evtPay, point_dep, 0, grp_id);
};
