#include "passenger.h"
#include "astra_consts.h"
#include "astra_misc.h"
#include "term_version.h"
#include "baggage.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace std;
using namespace BASIC;

namespace CheckIn
{

const TPaxTknItem& TPaxTknItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  NewTextChild(node,"ticket_no",no);
  if (coupon!=ASTRA::NoExists)
    NewTextChild(node,"coupon_no",coupon);
  else
    NewTextChild(node,"coupon_no");
  NewTextChild(node,"ticket_rem",rem);
  NewTextChild(node,"ticket_confirm",(int)confirm);
  return *this;
};

TPaxTknItem& TPaxTknItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;

  no=NodeAsStringFast("ticket_no",node2);
  if (!NodeIsNULLFast("coupon_no",node2))
    coupon=NodeAsIntegerFast("coupon_no",node2);
  rem=NodeAsStringFast("ticket_rem",node2);
  confirm=NodeAsIntegerFast("ticket_confirm",node2)!=0;
  return *this;
};

const TPaxTknItem& TPaxTknItem::toDB(TQuery &Qry) const
{
  Qry.SetVariable("ticket_no", no);
  if (coupon!=ASTRA::NoExists)
    Qry.SetVariable("coupon_no", coupon);
  else
    Qry.SetVariable("coupon_no", FNull);
  Qry.SetVariable("ticket_rem", rem);
  Qry.SetVariable("ticket_confirm", (int)confirm);
  return *this;
};

TPaxTknItem& TPaxTknItem::fromDB(TQuery &Qry)
{
  clear();
  no=Qry.FieldAsString("ticket_no");
  if (!Qry.FieldIsNULL("coupon_no"))
    coupon=Qry.FieldAsInteger("coupon_no");
  else
    coupon=ASTRA::NoExists;
  rem=Qry.FieldAsString("ticket_rem");
  confirm=Qry.FieldAsInteger("ticket_confirm")!=0;
  return *this;
};


string PaxDocCountryToTerm(const string &pax_doc_country)
{
  if (TReqInfo::Instance()->client_type!=ASTRA::ctTerm ||
      TReqInfo::Instance()->desk.compatible(SCAN_DOC_VERSION) ||
      pax_doc_country.empty())
    return pax_doc_country;
  else
    return getBaseTable(etPaxDocCountry).get_row("code",pax_doc_country).AsString("country");
};

string PaxDocCountryFromTerm(const string &doc_code)
{
  if (TReqInfo::Instance()->client_type!=ASTRA::ctTerm ||
      TReqInfo::Instance()->desk.compatible(SCAN_DOC_VERSION) ||
      doc_code.empty())
    return doc_code;
  else
    try
    {
      return getBaseTable(etPaxDocCountry).get_row("country",doc_code).AsString("code");
    }
    catch (EBaseTableError)
    {
      return "";
    };
};

const TPaxDocItem& TPaxDocItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  //документ
  xmlNodePtr docNode=NewTextChild(node,"document");
  NewTextChild(docNode, "type", type, "");
  NewTextChild(docNode, "issue_country", PaxDocCountryToTerm(issue_country), "");
  NewTextChild(docNode, "no", no, "");
  NewTextChild(docNode, "nationality", PaxDocCountryToTerm(nationality), "");
  if (birth_date!=ASTRA::NoExists)
    NewTextChild(docNode, "birth_date", DateTimeToStr(birth_date, ServerFormatDateTimeAsString));
  NewTextChild(docNode, "gender", gender, "");
  if (expiry_date!=ASTRA::NoExists)
    NewTextChild(docNode, "expiry_date", DateTimeToStr(expiry_date, ServerFormatDateTimeAsString));
  NewTextChild(docNode, "surname", surname, "");
  NewTextChild(docNode, "first_name", first_name, "");
  NewTextChild(docNode, "second_name", second_name, "");
  NewTextChild(docNode, "pr_multi", (int)pr_multi, (int)false);
  return *this;
};

TPaxDocItem& TPaxDocItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (node2==NULL) return *this;
  
  type=NodeAsStringFast("type",node2,"");
  issue_country=PaxDocCountryFromTerm(NodeAsStringFast("issue_country",node2,""));
  no=NodeAsStringFast("no",node2,"");
  nationality=PaxDocCountryFromTerm(NodeAsStringFast("nationality",node2,""));
  if (!NodeIsNULLFast("birth_date",node2,true))
    birth_date=NodeAsDateTimeFast("birth_date",node2);
  gender=NodeAsStringFast("gender",node2,"");
  if (!NodeIsNULLFast("expiry_date",node2,true))
    expiry_date=NodeAsDateTimeFast("expiry_date",node2);
  surname=NodeAsStringFast("surname",node2,"");
  first_name=NodeAsStringFast("first_name",node2,"");
  second_name=NodeAsStringFast("second_name",node2,"");
  pr_multi=NodeAsIntegerFast("pr_multi",node2,0)!=0;
  return *this;
};

const TPaxDocItem& TPaxDocItem::toDB(TQuery &Qry) const
{
  Qry.SetVariable("type", type);
  Qry.SetVariable("issue_country", issue_country);
  Qry.SetVariable("no", no);
  Qry.SetVariable("nationality", nationality);
  if (birth_date!=ASTRA::NoExists)
    Qry.SetVariable("birth_date", birth_date);
  else
    Qry.SetVariable("birth_date", FNull);
  Qry.SetVariable("gender", gender);
  if (expiry_date!=ASTRA::NoExists)
    Qry.SetVariable("expiry_date", expiry_date);
  else
    Qry.SetVariable("expiry_date", FNull);
  Qry.SetVariable("surname", surname);
  Qry.SetVariable("first_name", first_name);
  Qry.SetVariable("second_name", second_name);
  Qry.SetVariable("pr_multi", (int)pr_multi);
  return *this;
};

TPaxDocItem& TPaxDocItem::fromDB(TQuery &Qry)
{
  clear();
  type=Qry.FieldAsString("type");
  issue_country=GetPaxDocCountryCode(Qry.FieldAsString("issue_country"));
  no=Qry.FieldAsString("no");
  nationality=GetPaxDocCountryCode(Qry.FieldAsString("nationality"));
  if (!Qry.FieldIsNULL("birth_date"))
    birth_date=Qry.FieldAsDateTime("birth_date");
  else
    birth_date=ASTRA::NoExists;
  gender=Qry.FieldAsString("gender");
  if (!Qry.FieldIsNULL("expiry_date"))
    expiry_date=Qry.FieldAsDateTime("expiry_date");
  else
    expiry_date=ASTRA::NoExists;
  surname=Qry.FieldAsString("surname");
  first_name=Qry.FieldAsString("first_name");
  second_name=Qry.FieldAsString("second_name");
  pr_multi=Qry.FieldAsInteger("pr_multi")!=0;
  return *this;
};

const TPaxDocoItem& TPaxDocoItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  xmlNodePtr docNode=NewTextChild(node,"doco");
  NewTextChild(docNode, "birth_place", birth_place, "");
  NewTextChild(docNode, "type", type, "");
  NewTextChild(docNode, "no", no, "");
  NewTextChild(docNode, "issue_place", issue_place, "");
  if (issue_date!=ASTRA::NoExists)
    NewTextChild(docNode, "issue_date", DateTimeToStr(issue_date, ServerFormatDateTimeAsString));
  if (expiry_date!=ASTRA::NoExists)
    NewTextChild(docNode, "expiry_date", DateTimeToStr(expiry_date, ServerFormatDateTimeAsString));
  NewTextChild(docNode, "applic_country", PaxDocCountryToTerm(applic_country), "");
  NewTextChild(docNode, "pr_inf", (int)pr_inf, (int)false);
  return *this;
};

TPaxDocoItem& TPaxDocoItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (node2==NULL) return *this;

  birth_place=NodeAsStringFast("birth_place",node2,"");
  type=NodeAsStringFast("type",node2,"");
  no=NodeAsStringFast("no",node2,"");
  issue_place=NodeAsStringFast("issue_place",node2,"");
  if (!NodeIsNULLFast("issue_date",node2,true))
    issue_date=NodeAsDateTimeFast("issue_date",node2);
  if (!NodeIsNULLFast("expiry_date",node2,true))
    expiry_date=NodeAsDateTimeFast("expiry_date",node2);
  applic_country=PaxDocCountryFromTerm(NodeAsStringFast("applic_country",node2,""));
  pr_inf=NodeAsIntegerFast("pr_inf",node2,0)!=0;
  return *this;
};

const TPaxDocoItem& TPaxDocoItem::toDB(TQuery &Qry) const
{
  Qry.SetVariable("birth_place", birth_place);
  Qry.SetVariable("type", type);
  Qry.SetVariable("no", no);
  Qry.SetVariable("issue_place", issue_place);
  if (issue_date!=ASTRA::NoExists)
    Qry.SetVariable("issue_date", issue_date);
  else
    Qry.SetVariable("issue_date", FNull);
  if (expiry_date!=ASTRA::NoExists)
    Qry.SetVariable("expiry_date", expiry_date);
  else
    Qry.SetVariable("expiry_date", FNull);
  Qry.SetVariable("applic_country", applic_country);
  Qry.SetVariable("pr_inf", (int)pr_inf);
  return *this;
};

TPaxDocoItem& TPaxDocoItem::fromDB(TQuery &Qry)
{
  clear();
  birth_place=Qry.FieldAsString("birth_place");
  type=Qry.FieldAsString("type");
  no=Qry.FieldAsString("no");
  issue_place=Qry.FieldAsString("issue_place");
  if (!Qry.FieldIsNULL("issue_date"))
    issue_date=Qry.FieldAsDateTime("issue_date");
  else
    issue_date=ASTRA::NoExists;
  if (!Qry.FieldIsNULL("expiry_date"))
    expiry_date=Qry.FieldAsDateTime("expiry_date");
  else
    expiry_date=ASTRA::NoExists;
  applic_country=GetPaxDocCountryCode(Qry.FieldAsString("applic_country"));
  pr_inf=Qry.FieldAsInteger("pr_inf")!=0;
  return *this;
};

void LoadPaxDoc(TQuery& PaxDocQry, xmlNodePtr paxNode)
{
  if (PaxDocQry.Eof || paxNode==NULL) return;
  TPaxDocItem().fromDB(PaxDocQry).toXML(paxNode);
};

void LoadPaxDoco(TQuery& PaxDocQry, xmlNodePtr paxNode)
{
  if (PaxDocQry.Eof || paxNode==NULL) return;
  TPaxDocoItem().fromDB(PaxDocQry).toXML(paxNode);
};

bool LoadPaxDoc(int pax_id, TPaxDocItem &doc, TQuery& PaxDocQry)
{
  doc.clear();
  const char* sql=
    "SELECT * FROM pax_doc WHERE pax_id=:pax_id";
  if (strcmp(PaxDocQry.SQLText.SQLText(),sql)!=0)
  {
    PaxDocQry.Clear();
    PaxDocQry.SQLText=sql;
    PaxDocQry.DeclareVariable("pax_id",otInteger);
  };
  PaxDocQry.SetVariable("pax_id",pax_id);
  PaxDocQry.Execute();
  if (!PaxDocQry.Eof) doc.fromDB(PaxDocQry);
  return !doc.empty();
};

bool LoadPaxDoco(int pax_id, TPaxDocoItem &doc, TQuery& PaxDocQry)
{
  doc.clear();
  const char* sql=
    "SELECT * FROM pax_doco WHERE pax_id=:pax_id";
  if (strcmp(PaxDocQry.SQLText.SQLText(),sql)!=0)
  {
    PaxDocQry.Clear();
    PaxDocQry.SQLText=sql;
    PaxDocQry.DeclareVariable("pax_id",otInteger);
  };
  PaxDocQry.SetVariable("pax_id",pax_id);
  PaxDocQry.Execute();
  if (!PaxDocQry.Eof) doc.fromDB(PaxDocQry);
  return !doc.empty();
};

void SavePaxDoc(int pax_id, xmlNodePtr docNode, TQuery& PaxDocQry)
{
  const char* sql=
        "BEGIN "
        "  DELETE FROM pax_doc WHERE pax_id=:pax_id; "
        "  IF :only_delete=0 THEN "
        "    INSERT INTO pax_doc "
        "      (pax_id,type,issue_country,no,nationality,birth_date,gender,expiry_date, "
        "       surname,first_name,second_name,pr_multi) "
        "    VALUES "
        "      (:pax_id,:type,:issue_country,:no,:nationality,:birth_date,:gender,:expiry_date, "
        "       :surname,:first_name,:second_name,:pr_multi); "
        "  END IF; "
        "END;";
  if (strcmp(PaxDocQry.SQLText.SQLText(),sql)!=0)
  {
    PaxDocQry.Clear();
    PaxDocQry.SQLText=sql;
    PaxDocQry.DeclareVariable("pax_id",otInteger);
    PaxDocQry.DeclareVariable("type",otString);
    PaxDocQry.DeclareVariable("issue_country",otString);
    PaxDocQry.DeclareVariable("no",otString);
    PaxDocQry.DeclareVariable("nationality",otString);
    PaxDocQry.DeclareVariable("birth_date",otDate);
    PaxDocQry.DeclareVariable("gender",otString);
    PaxDocQry.DeclareVariable("expiry_date",otDate);
    PaxDocQry.DeclareVariable("surname",otString);
    PaxDocQry.DeclareVariable("first_name",otString);
    PaxDocQry.DeclareVariable("second_name",otString);
    PaxDocQry.DeclareVariable("pr_multi",otInteger);
    PaxDocQry.DeclareVariable("only_delete",otInteger);
  };

  TPaxDocItem doc;
  doc.fromXML(docNode).toDB(PaxDocQry);
  PaxDocQry.SetVariable("pax_id",pax_id);
  PaxDocQry.SetVariable("only_delete",(int)doc.empty());
  PaxDocQry.Execute();
};

void SavePaxDoco(int pax_id, xmlNodePtr docNode, TQuery& PaxDocQry)
{
  const char* sql=
        "BEGIN "
        "  DELETE FROM pax_doco WHERE pax_id=:pax_id; "
        "  IF :only_delete=0 THEN "
        "    INSERT INTO pax_doco "
        "      (pax_id,birth_place,type,no,issue_place,issue_date,expiry_date,applic_country,pr_inf) "
        "    VALUES "
        "      (:pax_id,:birth_place,:type,:no,:issue_place,:issue_date,:expiry_date,:applic_country,:pr_inf); "
        "  END IF; "
        "END;";
  if (strcmp(PaxDocQry.SQLText.SQLText(),sql)!=0)
  {
    PaxDocQry.Clear();
    PaxDocQry.SQLText=sql;
    PaxDocQry.DeclareVariable("pax_id",otInteger);
    PaxDocQry.DeclareVariable("birth_place",otString);
    PaxDocQry.DeclareVariable("type",otString);
    PaxDocQry.DeclareVariable("no",otString);
    PaxDocQry.DeclareVariable("issue_place",otString);
    PaxDocQry.DeclareVariable("issue_date",otDate);
    PaxDocQry.DeclareVariable("expiry_date",otDate);
    PaxDocQry.DeclareVariable("applic_country",otString);
    PaxDocQry.DeclareVariable("pr_inf",otInteger);
    PaxDocQry.DeclareVariable("only_delete",otInteger);
  };

  TPaxDocoItem doc;
  doc.fromXML(docNode).toDB(PaxDocQry);
  PaxDocQry.SetVariable("pax_id",pax_id);
  PaxDocQry.SetVariable("only_delete",(int)doc.empty());
  PaxDocQry.Execute();
};

const TPaxNormItem& TPaxNormItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  if (bag_type!=ASTRA::NoExists)
    NewTextChild(node,"bag_type",bag_type);
  else
    NewTextChild(node,"bag_type");
  if (norm_id!=ASTRA::NoExists)
  {
    NewTextChild(node,"norm_id",norm_id);
    NewTextChild(node,"norm_trfer",(int)norm_trfer);
  }
  else
  {
    NewTextChild(node,"norm_id");
    NewTextChild(node,"norm_trfer");
  };
  return *this;
};

TPaxNormItem& TPaxNormItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (!NodeIsNULLFast("bag_type",node2))
    bag_type=NodeAsIntegerFast("bag_type",node2);
  if (!NodeIsNULLFast("norm_id",node2))
  {
    norm_id=NodeAsIntegerFast("norm_id",node2);
    norm_trfer=NodeAsIntegerFast("norm_trfer",node2,0)!=0;
  };
  return *this;
};

const TPaxNormItem& TPaxNormItem::toDB(TQuery &Qry) const
{
  if (bag_type!=ASTRA::NoExists)
    Qry.SetVariable("bag_type",bag_type);
  else
    Qry.SetVariable("bag_type",FNull);
  if (norm_id!=ASTRA::NoExists)
  {
    Qry.SetVariable("norm_id",norm_id);
    Qry.SetVariable("norm_trfer",(int)norm_trfer);
  }
  else
  {
    Qry.SetVariable("norm_id",FNull);
    Qry.SetVariable("norm_trfer",FNull);
  };
  return *this;
};

TPaxNormItem& TPaxNormItem::fromDB(TQuery &Qry)
{
  clear();
  if (!Qry.FieldIsNULL("bag_type"))
    bag_type=Qry.FieldAsInteger("bag_type");
  if (!Qry.FieldIsNULL("norm_id"))
  {
    norm_id=Qry.FieldAsInteger("norm_id");
    norm_trfer=Qry.FieldAsInteger("norm_trfer")!=0;
  };
  return *this;
};

bool LoadPaxNorms(int pax_id, vector< pair<TPaxNormItem, TNormItem> > &norms, TQuery& NormQry)
{
  norms.clear();
  const char* sql=
    "SELECT pax_norms.bag_type, pax_norms.norm_id, pax_norms.norm_trfer, "
    "       bag_norms.norm_type, bag_norms.amount, bag_norms.weight, bag_norms.per_unit "
    "FROM pax_norms,bag_norms "
    "WHERE pax_norms.norm_id=bag_norms.id(+) AND pax_norms.pax_id=:pax_id ";
  if (strcmp(NormQry.SQLText.SQLText(),sql)!=0)
  {
    NormQry.Clear();
    NormQry.SQLText=sql;
    NormQry.DeclareVariable("pax_id",otInteger);
  };
  NormQry.SetVariable("pax_id",pax_id);
  NormQry.Execute();
  for(;!NormQry.Eof;NormQry.Next())
    norms.push_back( make_pair(TPaxNormItem().fromDB(NormQry),
                               TNormItem().fromDB(NormQry)) );
  return !norms.empty();
};

bool LoadGrpNorms(int grp_id, vector< pair<TPaxNormItem, TNormItem> > &norms, TQuery& NormQry)
{
  norms.clear();
  const char* sql=
    "SELECT grp_norms.bag_type, grp_norms.norm_id, grp_norms.norm_trfer, "
    "       bag_norms.norm_type, bag_norms.amount, bag_norms.weight, bag_norms.per_unit "
    "FROM grp_norms,bag_norms "
    "WHERE grp_norms.norm_id=bag_norms.id(+) AND grp_norms.grp_id=:grp_id ";
  if (strcmp(NormQry.SQLText.SQLText(),sql)!=0)
  {
    NormQry.Clear();
    NormQry.SQLText=sql;
    NormQry.DeclareVariable("grp_id",otInteger);
  };
  NormQry.SetVariable("grp_id",grp_id);
  NormQry.Execute();
  for(;!NormQry.Eof;NormQry.Next())
    norms.push_back( make_pair(TPaxNormItem().fromDB(NormQry),
                               TNormItem().fromDB(NormQry)) );
  return !norms.empty();
};

void LoadNorms(xmlNodePtr node, bool pr_unaccomp, TQuery& NormQry)
{
  if (node==NULL) return;
  xmlNodePtr node2=node->children;

  vector< pair<TPaxNormItem, TNormItem> > norms;
  if (!pr_unaccomp)
  {
    int pax_id=NodeAsIntegerFast("pax_id",node2);
    LoadPaxNorms(pax_id, norms, NormQry);
  }
  else
  {
    int grp_id=NodeAsIntegerFast("grp_id",node2);
    LoadGrpNorms(grp_id, norms, NormQry);
  };
  
  xmlNodePtr normsNode=NewTextChild(node,"norms");
  vector< pair<TPaxNormItem, TNormItem> >::const_iterator i=norms.begin();
  for(;i!=norms.end();++i)
  {
    xmlNodePtr normNode=NewTextChild(normsNode,"norm");
    i->first.toXML(normNode);
    i->second.toXML(normNode);
  };
};

void SaveNorms(xmlNodePtr node, bool pr_unaccomp)
{
  if (node==NULL) return;
  xmlNodePtr node2=node->children;

  xmlNodePtr normNode=GetNodeFast("norms",node2);
  if (normNode==NULL) return;

  TQuery NormQry(&OraSession);
  NormQry.Clear();
  if (!pr_unaccomp)
  {
    int pax_id;
    if (GetNodeFast("generated_pax_id",node2)!=NULL)
      pax_id=NodeAsIntegerFast("generated_pax_id",node2);
    else
      pax_id=NodeAsIntegerFast("pax_id",node2);
    NormQry.SQLText="DELETE FROM pax_norms WHERE pax_id=:pax_id";
    NormQry.CreateVariable("pax_id",otInteger,pax_id);
    NormQry.Execute();
    NormQry.SQLText=
      "INSERT INTO pax_norms(pax_id,bag_type,norm_id,norm_trfer) "
      "VALUES(:pax_id,:bag_type,:norm_id,:norm_trfer)";
  }
  else
  {
    int grp_id;
    if (GetNodeFast("generated_grp_id",node2)!=NULL)
      grp_id=NodeAsIntegerFast("generated_grp_id",node2);
    else
      grp_id=NodeAsIntegerFast("grp_id",node2);
    NormQry.SQLText="DELETE FROM grp_norms WHERE grp_id=:grp_id";
    NormQry.CreateVariable("grp_id",otInteger,grp_id);
    NormQry.Execute();
    NormQry.SQLText=
      "INSERT INTO grp_norms(grp_id,bag_type,norm_id,norm_trfer) "
      "VALUES(:grp_id,:bag_type,:norm_id,:norm_trfer)";
  };
  NormQry.DeclareVariable("bag_type",otInteger);
  NormQry.DeclareVariable("norm_id",otInteger);
  NormQry.DeclareVariable("norm_trfer",otInteger);
  for(normNode=normNode->children;normNode!=NULL;normNode=normNode->next)
  {
    TPaxNormItem().fromXML(normNode).toDB(NormQry);
    NormQry.Execute();
  };
};

};


