#include "ffp_sirena.h"
#include "sirena_exchange.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace std;

namespace SirenaExchange
{

const std::string TFFPInfoExchange::id="ffp_info";

const TFFPItem& TFFPItem::toXML(xmlNodePtr node, const OutputLang &lang) const
{
  if (node==NULL) return *this;

  xmlNodePtr ffpNode = NewTextChild(node, "ffp", card_number);
  SetProp(ffpNode, "company", airlineToPrefferedCode(company, lang));
  SetProp(ffpNode, "surname", surname);
  SetProp(ffpNode, "name", name); //обязательно передаем, даже если пустое имя, а иначе Сирена ругается
  if (birth_date!=ASTRA::NoExists)
    SetProp(ffpNode, "birthdate", DateTimeToStr(birth_date, "yyyy-mm-dd"));

  return *this;
}

TFFPItem& TFFPItem::fromXML(xmlNodePtr node)
{
  clear();

  try
  {
    if (node==NULL) throw Exception("node not defined");

    xmlNodePtr ffpNode = NodeAsNode("ffp", node);

    string str;
    TElemFmt company_fmt;
    str = NodeAsString("@company", ffpNode, "");
    if (str.empty()) throw Exception("Empty @company");
    company = ElemToElemId( etAirline, str, company_fmt );
    if (company_fmt==efmtUnknown) throw Exception("Unknown @company '%s'", str.c_str());

    card_number = NodeAsString(ffpNode);
    if (card_number.empty()) throw Exception("Empty <ffp>");
    if (card_number.size()>25) throw Exception("Wrong <ffp> '%s'", card_number.c_str());
  }
  catch(Exception &e)
  {
    throw Exception("TFFPItem::fromXML: %s", e.what());
  };
  return *this;
}

TFFPInfoNameItem& TFFPInfoNameItem::fromXML(xmlNodePtr node)
{
  clear();

  try
  {
    if (node==NULL) throw Exception("node not defined");

    first = NodeAsString("@first", node, "");
    if (first.empty()) throw Exception("Empty @first");
    if (first.size()>70) throw Exception("Wrong @first='%s'", first.c_str());

    last = NodeAsString("@last", node, "");
    if (last.empty()) throw Exception("Empty @last");
    if (last.size()>70) throw Exception("Wrong @last='%s'", last.c_str());
  }
  catch(Exception &e)
  {
    throw Exception("TFFPInfoNameItem::fromXML: %s", e.what());
  };
  return *this;
}

void TFFPInfoReq::toXML(xmlNodePtr node) const
{
  if (node==NULL) return;

  TFFPItem::toXML(node, OutputLang(LANG_EN, {OutputLang::OnlyTrueIataCodes}));
}

void TFFPInfoRes::fromXML(xmlNodePtr node)
{
  clear();

  try
  {
    if (node==NULL) throw Exception("node not defined");

    TFFPItem::fromXML(node);

    xmlNodePtr infoNode=NodeAsNode("info", node);

    status = NodeAsString("@status", infoNode, "");
    if (status.empty()) throw Exception("Empty @status");
    if (status.size()>20) throw Exception("Wrong @status='%s'", status.c_str());

    for(node=infoNode->children; node!=NULL; node=node->next)
    {
      if (string((const char*)node->name)!="name") continue;
      names.push_back(TFFPInfoNameItem().fromXML(node));
    };

    if (names.empty()) throw Exception("<name> tag not found" );
  }
  catch(Exception &e)
  {
    throw Exception("TFFPInfoRes::fromXML: %s", e.what());
  };
}

} //namespace SirenaExchange

void get_ffp_status(const SirenaExchange::TFFPInfoReq &req, SirenaExchange::TFFPInfoRes &res)
{
  res.clear();
  try
  {
    SirenaExchange::SendRequest(req, res);
  }
  catch(Exception &e)
  {
    if (res.error() &&
        (res.error_code=="3" ||
         res.error_code=="4" ||
         res.error_code=="5" ||
         res.error_code=="6")) return;
    throw;
  }
}

int ffp(int argc,char **argv)
{
  if (argc<1) return 1;
  if (argc<4 || argc>6)
  {
    printf("Usage:\n");
    ffp_help(argv[0]);
    return 1;
  }
  SirenaExchange::TFFPInfoReq req;
  SirenaExchange::TFFPInfoRes res;
  TElemFmt company_fmt;
  string company = ElemToElemId( etAirline, argv[1], company_fmt );
  if (company.empty()) company=argv[1];
  string card_number=argv[2];
  string surname=argv[3];
  string name;
  TDateTime birth_date=ASTRA::NoExists;

  if (argc>4)
  {
    if (StrToDateTime(argv[argc-1], birth_date)==EOF)
      birth_date=ASTRA::NoExists;

    if (!(argc==5 && birth_date!=ASTRA::NoExists))
      name=argv[4];
    if (argc==6 && birth_date==ASTRA::NoExists)
    {
      printf("Usage:\n");
      ffp_help(argv[0]);
      return 1;
    }
  }

  req.set(company, card_number, surname, name, birth_date);
  printf("TFFPInfoReq: \n%s\n", req.traceStr().c_str());
  get_ffp_status(req, res);
  printf("TFFPInfoRes: \n%s\n", res.traceStr().c_str());
  return 1;
}

void ffp_help(const char *name)
{
  printf("  %-15.15s ", name);
  puts("<company> <card_number> <surname> [<name>] [<birth_date(dd.mm.yyyy)>]");
};



