#include "ffp_sirena.h"
#include "sirena_exchange.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace std;

namespace SirenaExchange
{

const std::string TFFPInfoExchange::id="ffp_info";

const TFFPItem& TFFPItem::toXML(xmlNodePtr node, const std::string &lang) const
{
  if (node==NULL) return *this;

  xmlNodePtr ffpNode = NewTextChild(node, "ffp", card_number);
  SetProp(ffpNode, "company", airlineToXML(company, lang));

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

  TFFPItem::toXML(node, LANG_EN);
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
      if (string((char*)node->name)!="name") continue;
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
    SendRequest(req, res);
  }
  catch(Exception &e)
  {
    if (res.error() &&
        (res.error_code=="3" ||
         res.error_code=="4" ||
         res.error_code=="5" ||
         res.error_code=="6")) throw UserException(res.error_message);
    throw;
  }
}

int ffp(int argc,char **argv)
{
  if (argc<1) return 1;
  if (argc<3)
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

  req.set(company, argv[2]);
  printf("TFFPInfoReq: \n%s\n", req.traceStr().c_str());
  get_ffp_status(req, res);
  printf("TFFPInfoRes: \n%s\n", res.traceStr().c_str());
  return 1;
}

void ffp_help(const char *name)
{
  printf("  %-15.15s ", name);
  puts("<company> <card_number>");
};



