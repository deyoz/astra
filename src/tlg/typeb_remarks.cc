#include <set>
#include "typeb_remarks.h"

#define STDLOG NICKNAME,__FILE__,__LINE__
#define NICKNAME "VLAD"
#include "serverlib/slogger.h"
#include "db_tquery.h"
#include "PgOraConfig.h"

using namespace ASTRA;
using namespace std;

namespace TypeB
{

static bool doNothing()
{
  return true;
}

template<class ... ContainterPairs>
static bool doNothing(map<string, TDocExtraItem>& prior,
                      const map<string, TDocExtraItem>& curr,
                      ContainterPairs& ... containerPairs)
{
  if (prior==curr) return doNothing(containerPairs...);

  return false;
}

template<class ... ContainterPairs>
static bool doNothing(set<TFQTExtraItem>& prior,
                      const set<TFQTExtraItem>& curr,
                      ContainterPairs& ... containerPairs)
{
  if (prior==curr) return doNothing(containerPairs...);

  return false;
}

template<class ContainerT, class ... ContainterPairs>
static bool doNothing(ContainerT& prior, const ContainerT& curr,
                      ContainterPairs& ... containerPairs)
{
  bool modified=false;
  for(const auto& item : curr)
  {
    if (!item.suitableForDB()) continue;

    auto i=std::find(prior.begin(), prior.end(), item);
    if (i==prior.end())
    {
      modified=true;
      break;
    }
    prior.erase(i);
  }

  if (!modified && prior.empty()) return doNothing(containerPairs...);

  return false;
}

static void LoadDOCSRem(const PaxId_t& paxId,
                        vector<TDocItem> &doc,
                        map<string, TDocExtraItem> &doc_extra)
{
  doc.clear();
  doc_extra.clear();

  DB::TQuery Qry(PgOra::getROSession("CRS_PAX_DOC"));
  Qry.Clear();
  Qry.SQLText="SELECT * FROM crs_pax_doc WHERE pax_id=:pax_id";
  Qry.CreateVariable("pax_id",otInteger,paxId.get());
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
  {
    TDocItem item;
    strcpy(item.rem_code, Qry.FieldAsString("rem_code").c_str());
    strcpy(item.rem_status, Qry.FieldAsString("rem_status").c_str());
    strcpy(item.type, Qry.FieldAsString("type").c_str());
    strcpy(item.issue_country, Qry.FieldAsString("issue_country").c_str());
    strcpy(item.no, Qry.FieldAsString("no").c_str());
    strcpy(item.nationality, Qry.FieldAsString("nationality").c_str());
    if (!Qry.FieldIsNULL("birth_date"))
      item.birth_date=Qry.FieldAsDateTime("birth_date");
    strcpy(item.gender, Qry.FieldAsString("gender").c_str());
    if (!Qry.FieldIsNULL("expiry_date"))
      item.expiry_date=Qry.FieldAsDateTime("expiry_date");
    item.surname=Qry.FieldAsString("surname");
    item.first_name=Qry.FieldAsString("first_name");
    item.second_name=Qry.FieldAsString("second_name");
    item.pr_multi=Qry.FieldAsInteger("pr_multi")!=0;
    doc.push_back(item);

    TDocExtraItem extraItem;
    strcpy(extraItem.no, Qry.FieldAsString("no").c_str());
    strcpy(extraItem.type_rcpt, Qry.FieldAsString("type_rcpt").c_str());
    if (extraItem.valid())
      doc_extra.emplace(extraItem.no, extraItem);
  }
}

void SaveDOCSRem(const PaxIdWithSegmentPair& paxId,
                 const bool paxIdUsedBefore,
                 const vector<TDocItem> &doc,
                 const map<string, TDocExtraItem> &doc_extra,
                 ModifiedPaxRem& modifiedPaxRem)
{
  bool deleteFromDB=paxIdUsedBefore;
  if (!doc.empty())
  {
    map<string, TDocExtraItem> doc_extra_filtered;
    for(const TDocItem& item : doc)
    {
      auto j=doc_extra.find(item.no);
      if (j!=doc_extra.end() && j->second.valid()) doc_extra_filtered.emplace(*j);
    }

    vector<TDocItem> prior;
    map<string, TDocExtraItem> prior_extra;
    if (paxIdUsedBefore)
      LoadDOCSRem(paxId(), prior, prior_extra);
    if (prior.empty()) deleteFromDB=false;
    if (doNothing(prior, doc, prior_extra, doc_extra_filtered)) return;
  }

  bool modified=false;

  DB::TQuery Qry(PgOra::getRWSession("CRS_PAX_DOC"));
  Qry.Clear();
  Qry.CreateVariable("pax_id",otInteger,paxId().get());
  if (deleteFromDB)
  {
    Qry.SQLText="DELETE FROM crs_pax_doc WHERE pax_id=:pax_id";
    Qry.Execute();
    if (Qry.RowsProcessed()!=0) modified=true;
  }

  if (!doc.empty())
  {
    Qry.SQLText=
        "INSERT INTO crs_pax_doc "
        "  (pax_id,rem_code,rem_status,type,issue_country,no,nationality, "
        "   birth_date,gender,expiry_date,surname,first_name,second_name,pr_multi, "
        "   type_rcpt) "
        "VALUES "
        "  (:pax_id,:rem_code,:rem_status,:type,:issue_country,:no,:nationality, "
        "   :birth_date,:gender,:expiry_date,:surname,:first_name,:second_name,:pr_multi, "
        "   :type_rcpt) ";
    Qry.DeclareVariable("rem_code",otString);
    Qry.DeclareVariable("rem_status",otString);
    Qry.DeclareVariable("type",otString);
    Qry.DeclareVariable("issue_country",otString);
    Qry.DeclareVariable("no",otString);
    Qry.DeclareVariable("nationality",otString);
    Qry.DeclareVariable("birth_date",otDate);
    Qry.DeclareVariable("gender",otString);
    Qry.DeclareVariable("expiry_date",otDate);
    Qry.DeclareVariable("surname",otString);
    Qry.DeclareVariable("first_name",otString);
    Qry.DeclareVariable("second_name",otString);
    Qry.DeclareVariable("pr_multi",otInteger);
    Qry.DeclareVariable("type_rcpt",otString);
    for(const TDocItem& item : doc)
    {
      if (!item.suitableForDB()) continue;
      Qry.SetVariable("rem_code",item.rem_code);
      Qry.SetVariable("rem_status",item.rem_status);
      Qry.SetVariable("type",item.type);
      Qry.SetVariable("issue_country",item.issue_country);
      Qry.SetVariable("no",item.no);
      Qry.SetVariable("nationality",item.nationality);
      if (item.birth_date!=NoExists)
        Qry.SetVariable("birth_date",item.birth_date);
      else
        Qry.SetVariable("birth_date",FNull);
      Qry.SetVariable("gender",item.gender);
      if (item.expiry_date!=NoExists)
        Qry.SetVariable("expiry_date",item.expiry_date);
      else
        Qry.SetVariable("expiry_date",FNull);
      Qry.SetVariable("surname",item.surname.substr(0,64));
      Qry.SetVariable("first_name",item.first_name.substr(0,64));
      Qry.SetVariable("second_name",item.second_name.substr(0,64));
      Qry.SetVariable("pr_multi",(int)item.pr_multi);
      Qry.SetVariable("type_rcpt", FNull);
      auto j=doc_extra.find(item.no);
      if (j!=doc_extra.end() && j->second.valid())
        Qry.SetVariable("type_rcpt", j->second.type_rcpt);
      Qry.Execute();
      if (Qry.RowsProcessed()!=0) modified=true;
    }
  }

  if (modified)
    modifiedPaxRem.add(remDOC, paxId);
}

static void LoadDOCORem(const PaxId_t& paxId,
                        vector<TDocoItem> &doc)
{
  doc.clear();

  DB::TQuery Qry(PgOra::getROSession("CRS_PAX_DOCO"));
  Qry.Clear();
  Qry.SQLText="SELECT * FROM crs_pax_doco WHERE pax_id=:pax_id";
  Qry.CreateVariable("pax_id",otInteger,paxId.get());
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
  {
    TDocoItem item;
    strcpy(item.rem_code, Qry.FieldAsString("rem_code").c_str());
    strcpy(item.rem_status, Qry.FieldAsString("rem_status").c_str());
    item.birth_place=Qry.FieldAsString("birth_place");
    strcpy(item.type, Qry.FieldAsString("type").c_str());
    strcpy(item.no, Qry.FieldAsString("no").c_str());
    item.issue_place=Qry.FieldAsString("issue_place");
    if (!Qry.FieldIsNULL("issue_date"))
      item.issue_date=Qry.FieldAsDateTime("issue_date");
    strcpy(item.applic_country, Qry.FieldAsString("applic_country").c_str());

    doc.push_back(item);
  }
}

void SaveDOCORem(const PaxIdWithSegmentPair& paxId,
                 const bool paxIdUsedBefore,
                 const vector<TDocoItem> &doc,
                 ModifiedPaxRem& modifiedPaxRem)
{
  bool deleteFromDB=paxIdUsedBefore;
  if (!doc.empty())
  {
    vector<TDocoItem> prior;
    if (paxIdUsedBefore)
      LoadDOCORem(paxId(), prior);
    if (prior.empty()) deleteFromDB=false;
    if (doNothing(prior, doc)) return;
  }

  bool modified=false;

  DB::TQuery Qry(PgOra::getRWSession("CRS_PAX_DOCO"));
  Qry.Clear();
  Qry.CreateVariable("pax_id",otInteger,paxId().get());
  if (deleteFromDB)
  {
    Qry.SQLText="DELETE FROM crs_pax_doco WHERE pax_id=:pax_id";
    Qry.Execute();
    if (Qry.RowsProcessed()!=0) modified=true;
  }

  if (!doc.empty())
  {
    Qry.SQLText=
        "INSERT INTO crs_pax_doco "
        "  (pax_id,rem_code,rem_status,birth_place,type,no,issue_place,issue_date, "
        "   applic_country) "
        "VALUES "
        "  (:pax_id,:rem_code,:rem_status,:birth_place,:type,:no,:issue_place,:issue_date, "
        "   :applic_country) ";
    Qry.CreateVariable("pax_id",otInteger,paxId().get());
    Qry.DeclareVariable("rem_code",otString);
    Qry.DeclareVariable("rem_status",otString);
    Qry.DeclareVariable("birth_place",otString);
    Qry.DeclareVariable("type",otString);
    Qry.DeclareVariable("no",otString);
    Qry.DeclareVariable("issue_place",otString);
    Qry.DeclareVariable("issue_date",otDate);
    Qry.DeclareVariable("applic_country",otString);
    for(const TDocoItem& item : doc)
    {
      if (!item.suitableForDB()) continue;
      Qry.SetVariable("rem_code",item.rem_code);
      Qry.SetVariable("rem_status",item.rem_status);
      Qry.SetVariable("birth_place",item.birth_place.substr(0,35));
      Qry.SetVariable("type",item.type);
      Qry.SetVariable("no",item.no);
      Qry.SetVariable("issue_place",item.issue_place.substr(0,35));
      if (item.issue_date!=NoExists)
        Qry.SetVariable("issue_date",item.issue_date);
      else
        Qry.SetVariable("issue_date",FNull);
      Qry.SetVariable("applic_country",item.applic_country);
      Qry.Execute();
      if (Qry.RowsProcessed()!=0) modified=true;
    }
  }

  if (modified)
    modifiedPaxRem.add(remDOCO, paxId);
}

static void LoadDOCARem(const PaxId_t& paxId,
                        vector<TDocaItem> &doca)
{
  doca.clear();

  DB::TQuery Qry(PgOra::getROSession("CRS_PAX_DOCA"));
  Qry.Clear();
  Qry.SQLText="SELECT * FROM crs_pax_doca WHERE pax_id=:pax_id";
  Qry.CreateVariable("pax_id",otInteger,paxId.get());
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
  {
    TDocaItem item;
    strcpy(item.rem_code, Qry.FieldAsString("rem_code").c_str());
    strcpy(item.rem_status, Qry.FieldAsString("rem_status").c_str());
    strcpy(item.type, Qry.FieldAsString("type").c_str());
    strcpy(item.country, Qry.FieldAsString("country").c_str());
    item.address=Qry.FieldAsString("address");
    item.city=Qry.FieldAsString("city");
    item.region=Qry.FieldAsString("region");
    item.postal_code=Qry.FieldAsString("postal_code");

    doca.push_back(item);
  }
}

void SaveDOCARem(const PaxIdWithSegmentPair& paxId,
                 const bool paxIdUsedBefore,
                 const vector<TDocaItem> &doca,
                 ModifiedPaxRem& modifiedPaxRem)
{
  bool deleteFromDB=paxIdUsedBefore;
  if (!doca.empty())
  {
    vector<TDocaItem> prior;
    if (paxIdUsedBefore)
      LoadDOCARem(paxId(), prior);
    if (prior.empty()) deleteFromDB=false;
    if (doNothing(prior, doca)) return;
  }

  bool modified=false;

  DB::TQuery Qry(PgOra::getRWSession("CRS_PAX_DOCA"));
  Qry.Clear();
  Qry.CreateVariable("pax_id",otInteger,paxId().get());
  if (deleteFromDB)
  {
    Qry.SQLText="DELETE FROM crs_pax_doca WHERE pax_id=:pax_id";
    Qry.Execute();
    if (Qry.RowsProcessed()!=0) modified=true;
  }

  if (!doca.empty())
  {
    Qry.SQLText=
        "INSERT INTO crs_pax_doca "
        "  (pax_id,rem_code,rem_status,type,country,address,city,region,postal_code) "
        "VALUES "
        "  (:pax_id,:rem_code,:rem_status,:type,:country,:address,:city,:region,:postal_code) ";
    Qry.CreateVariable("pax_id",otInteger,paxId().get());
    Qry.DeclareVariable("rem_code",otString);
    Qry.DeclareVariable("rem_status",otString);
    Qry.DeclareVariable("type",otString);
    Qry.DeclareVariable("country",otString);
    Qry.DeclareVariable("address",otString);
    Qry.DeclareVariable("city",otString);
    Qry.DeclareVariable("region",otString);
    Qry.DeclareVariable("postal_code",otString);
    for(const TDocaItem& item : doca)
    {
      if (!item.suitableForDB()) continue;
      Qry.SetVariable("rem_code",item.rem_code);
      Qry.SetVariable("rem_status",item.rem_status);
      Qry.SetVariable("type",item.type);
      Qry.SetVariable("country",item.country);
      Qry.SetVariable("address",item.address.substr(0,35));
      Qry.SetVariable("city",item.city.substr(0,35));
      Qry.SetVariable("region",item.region.substr(0,35));
      Qry.SetVariable("postal_code",item.postal_code.substr(0,17));
      Qry.Execute();
      if (Qry.RowsProcessed()!=0) modified=true;
    }
  }

  if (modified)
    modifiedPaxRem.add(remDOCA, paxId);
}

void TTKNItem::toDB(TQuery &Qry) const
{
  Qry.SetVariable("rem_code", rem_code);
  Qry.SetVariable("ticket_no", ticket_no);
  if (coupon_no!=ASTRA::NoExists)
    Qry.SetVariable("coupon_no", coupon_no);
  else
    Qry.SetVariable("coupon_no", FNull);
}

void TTKNItem::fromDB(TQuery &Qry)
{
  Clear();
  strcpy(rem_code, Qry.FieldAsString("rem_code"));
  strcpy(ticket_no, Qry.FieldAsString("ticket_no"));
  if (!Qry.FieldIsNULL("coupon_no"))
    coupon_no=Qry.FieldAsInteger("coupon_no");
}

static void LoadTKNRem(const PaxId_t& paxId,
                       vector<TTKNItem> &tkn)
{
  tkn.clear();

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT * FROM crs_pax_tkn WHERE pax_id=:pax_id";
  Qry.CreateVariable("pax_id",otInteger,paxId.get());
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
    tkn.emplace_back(Qry);
}

void SaveTKNRem(const PaxIdWithSegmentPair& paxId,
                const bool paxIdUsedBefore,
                const vector<TTKNItem> &tkn,
                ModifiedPaxRem& modifiedPaxRem)
{
  bool deleteFromDB=paxIdUsedBefore;
  if (!tkn.empty())
  {
    vector<TTKNItem> prior;
    if (paxIdUsedBefore)
      LoadTKNRem(paxId(), prior);
    if (prior.empty()) deleteFromDB=false;
    if (doNothing(prior, tkn)) return;
  }

  bool modified=false;

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.CreateVariable("pax_id",otInteger,paxId().get());
  if (deleteFromDB)
  {
    Qry.SQLText="DELETE FROM crs_pax_tkn WHERE pax_id=:pax_id";
    Qry.Execute();
    if (Qry.RowsProcessed()!=0) modified=true;
  }

  if (!tkn.empty())
  {
    Qry.SQLText=
        "INSERT INTO crs_pax_tkn "
        "  (pax_id,rem_code,ticket_no,coupon_no) "
        "VALUES "
        "  (:pax_id,:rem_code,:ticket_no,:coupon_no) ";
    Qry.DeclareVariable("rem_code",otString);
    Qry.DeclareVariable("ticket_no",otString);
    Qry.DeclareVariable("coupon_no",otInteger);
    for(const TTKNItem& item : tkn)
    {
      if (!item.suitableForDB()) continue;
      item.toDB(Qry);
      Qry.Execute();
      if (Qry.RowsProcessed()!=0) modified=true;
    }
  }

  if (modified)
    modifiedPaxRem.add(remTKN, paxId);
}

static void LoadFQTRem(const PaxId_t& paxId,
                       vector<TFQTItem> &fqt,
                       set<TFQTExtraItem> &fqt_extra)
{
  fqt.clear();
  fqt_extra.clear();

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT * FROM crs_pax_fqt WHERE pax_id=:pax_id";
  Qry.CreateVariable("pax_id",otInteger,paxId.get());
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
  {
    TFQTItem item;
    strcpy(item.rem_code, Qry.FieldAsString("rem_code"));
    strcpy(item.airline, Qry.FieldAsString("airline"));
    strcpy(item.no, Qry.FieldAsString("no"));
    item.extra=Qry.FieldAsString("extra");
    fqt.push_back(item);

    TFQTExtraItem extraItem;
    extraItem.tier_level=Qry.FieldAsString("tier_level");
    if (extraItem.valid())
      fqt_extra.emplace(extraItem);
  }
}

void SaveFQTRem(const PaxIdWithSegmentPair& paxId,
                const bool paxIdUsedBefore,
                const vector<TFQTItem> &fqt,
                const set<TFQTExtraItem> &fqt_extra,
                ModifiedPaxRem& modifiedPaxRem)
{
  bool deleteFromDB=paxIdUsedBefore;

  set<TFQTExtraItem> fqt_extra_filtered;
  if (!fqt.empty())
  {
    if (fqt_extra.size()==1)
    {
      if (algo::all_of(fqt, [&fqt](const TFQTItem& item)
                            { return strcmp(fqt.begin()->airline, item.airline)==0 &&
                                     strcmp(fqt.begin()->no, item.no)==0; }))
        fqt_extra_filtered=fqt_extra;
    }

    vector<TFQTItem> prior;
    set<TFQTExtraItem> prior_extra;
    if (paxIdUsedBefore)
      LoadFQTRem(paxId(), prior, prior_extra);
    if (prior.empty()) deleteFromDB=false;
    if (doNothing(prior, fqt, prior_extra, fqt_extra_filtered)) return;
  }

  bool modified=false;

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.CreateVariable("pax_id",otInteger,paxId().get());
  if (deleteFromDB)
  {
    Qry.SQLText="DELETE FROM crs_pax_fqt WHERE pax_id=:pax_id";
    Qry.Execute();
    if (Qry.RowsProcessed()!=0) modified=true;
  }

  if (!fqt.empty())
  {
    Qry.SQLText=
        "INSERT INTO crs_pax_fqt "
        "  (pax_id, rem_code, airline, no, extra, tier_level) "
        "VALUES "
        "  (:pax_id, :rem_code, :airline, :no, :extra, :tier_level) ";
    Qry.DeclareVariable("rem_code",otString);
    Qry.DeclareVariable("airline",otString);
    Qry.DeclareVariable("no",otString);
    Qry.DeclareVariable("extra",otString);
    if (fqt_extra_filtered.size()==1)
      Qry.CreateVariable("tier_level", otString, fqt_extra_filtered.begin()->tier_level);
    else
      Qry.CreateVariable("tier_level", otString, FNull);

    for(const TFQTItem& item : fqt)
    {
      if (!item.suitableForDB()) continue;

      Qry.SetVariable("rem_code",item.rem_code);
      Qry.SetVariable("airline",item.airline);
      Qry.SetVariable("no",item.no);
      Qry.SetVariable("extra", item.extra.substr(0,250));
      Qry.Execute();
      if (Qry.RowsProcessed()!=0) modified=true;
    }
  }

  if (modified)
    modifiedPaxRem.add(remFQT, paxId);
}

bool SaveCHKDRem(const PaxIdWithSegmentPair& paxId, const vector<TCHKDItem> &chkd)
{
  bool result=false;
  if (chkd.empty()) return result;
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "INSERT INTO crs_pax_chkd "
    "  (pax_id,rem_status,reg_no) "
    "VALUES "
    "  (:pax_id,:rem_status,:reg_no) ";
  Qry.CreateVariable("pax_id",otInteger,paxId().get());
  Qry.DeclareVariable("rem_status",otString);
  Qry.DeclareVariable("reg_no",otInteger);
  for(vector<TCHKDItem>::const_iterator i=chkd.begin();i!=chkd.end();++i)
  {
    if (i->Empty() || string(i->rem_status).empty()) continue;
    Qry.SetVariable("rem_status",i->rem_status);
    Qry.SetVariable("reg_no",(int)i->reg_no);
    Qry.Execute();
    result=true;
  }
  if (result)
  {
    Qry.Clear();
    Qry.SQLText="UPDATE crs_pax SET sync_chkd=1 WHERE pax_id=:pax_id";
    Qry.CreateVariable("pax_id",otInteger,paxId().get());
    Qry.Execute();
  }
  return result;
}

static void LoadASVCRem(const PaxId_t& paxId, vector<TASVCItem> &asvc)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << paxId.get();
  asvc.clear();

  DB::TQuery Qry(PgOra::getROSession("CRS_PAX_ASVC"));
  Qry.Clear();
  Qry.SQLText="SELECT * FROM crs_pax_asvc WHERE pax_id=:pax_id";
  Qry.CreateVariable("pax_id",otInteger,paxId.get());
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
  {
    TASVCItem item;
    strcpy(item.rem_code, "ASVC");
    strcpy(item.rem_status, Qry.FieldAsString("rem_status").c_str());
    strcpy(item.RFIC, Qry.FieldAsString("rfic").c_str());
    strcpy(item.RFISC, Qry.FieldAsString("rfisc").c_str());
    if (!Qry.FieldIsNULL("service_quantity"))
      item.service_quantity=Qry.FieldAsInteger("service_quantity");
    strcpy(item.ssr_code, Qry.FieldAsString("ssr_code").c_str());
    strcpy(item.service_name, Qry.FieldAsString("service_name").c_str());
    strcpy(item.emd_type, Qry.FieldAsString("emd_type").c_str());
    strcpy(item.emd_no, Qry.FieldAsString("emd_no").c_str());
    if (!Qry.FieldIsNULL("emd_coupon"))
      item.emd_coupon=Qry.FieldAsInteger("emd_coupon");
    asvc.push_back(item);
  }
  LogTrace(TRACE5) << __func__
                   << ": count=" << asvc.size();
}

void SaveASVCRem(const PaxIdWithSegmentPair& paxId,
                 const vector<TASVCItem> &asvc,
                 ModifiedPaxRem& modifiedPaxRem)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << paxId().get();
  bool deleteFromDB=true;
  if (!asvc.empty())
  {
    vector<TASVCItem> prior;
    LoadASVCRem(paxId(), prior);
    if (prior.empty()) deleteFromDB=false;
    if (doNothing(prior, asvc)) return;
  }

  if (deleteFromDB)
  {
    LogTrace(TRACE5) << __func__
                     << ": deleteFromDB=" << deleteFromDB;
    DB::TQuery Qry(PgOra::getRWSession("CRS_PAX_ASVC"));
    Qry.Clear();
    Qry.CreateVariable("pax_id",otInteger,paxId().get());
    Qry.SQLText="DELETE FROM crs_pax_asvc WHERE pax_id=:pax_id";
    Qry.Execute();
    if (asvc.empty() && Qry.RowsProcessed()==0) return;
  }

  if (!asvc.empty())
  {
    DB::TQuery Qry(PgOra::getRWSession("CRS_PAX_ASVC"));
    Qry.Clear();
    Qry.SQLText=
        "INSERT INTO crs_pax_asvc "
        "  (pax_id,rem_status,rfic,rfisc,service_quantity,ssr_code,service_name,emd_type,emd_no,emd_coupon) "
        "VALUES "
        "  (:pax_id,:rem_status,:rfic,:rfisc,:service_quantity,:ssr_code,:service_name,:emd_type,:emd_no,:emd_coupon) ";
    Qry.CreateVariable("pax_id",otInteger,paxId().get());
    Qry.DeclareVariable("rem_status",otString);
    Qry.DeclareVariable("rfic",otString);
    Qry.DeclareVariable("rfisc",otString);
    Qry.DeclareVariable("service_quantity",otInteger);
    Qry.DeclareVariable("ssr_code",otString);
    Qry.DeclareVariable("service_name",otString);
    Qry.DeclareVariable("emd_type",otString);
    Qry.DeclareVariable("emd_no",otString);
    Qry.DeclareVariable("emd_coupon",otInteger);
    int saved = 0;
    for(const TASVCItem& item : asvc)
    {
      if (!item.suitableForDB()) continue;
      Qry.SetVariable("rem_status",item.rem_status);
      Qry.SetVariable("rfic",item.RFIC);
      Qry.SetVariable("rfisc",item.RFISC);
      item.service_quantity!=NoExists?Qry.SetVariable("service_quantity",item.service_quantity):
                                      Qry.SetVariable("service_quantity",FNull);
      Qry.SetVariable("ssr_code",item.ssr_code);
      Qry.SetVariable("service_name",item.service_name);
      Qry.SetVariable("emd_type",item.emd_type);
      Qry.SetVariable("emd_no",item.emd_no);
      item.emd_coupon!=NoExists?Qry.SetVariable("emd_coupon",item.emd_coupon):
                                Qry.SetVariable("emd_coupon",FNull);
      Qry.Execute();
      saved += Qry.RowsProcessed();
    }
    LogTrace(TRACE5) << __func__
                     << ": saved=" << saved;
  }

  modifiedPaxRem.add(remASVC, paxId);
}

void SavePNLADLRemarks(const PaxIdWithSegmentPair& paxId, const vector<TRemItem> &rem)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << paxId().get();
  if (rem.empty()) return;
  DB::TQuery CrsPaxRemQry(PgOra::getRWSession("CRS_PAX_REM"));
  CrsPaxRemQry.Clear();
  CrsPaxRemQry.SQLText=
    "INSERT INTO crs_pax_rem(pax_id,rem,rem_code) "
    "VALUES(:pax_id,:rem,:rem_code)";
  CrsPaxRemQry.DeclareVariable("pax_id",otInteger);
  CrsPaxRemQry.DeclareVariable("rem",otString);
  CrsPaxRemQry.DeclareVariable("rem_code",otString);
  //ремарки пассажира
  CrsPaxRemQry.SetVariable("pax_id",paxId().get());
  int saved = 0;
  for(vector<TRemItem>::const_iterator iRemItem=rem.begin();iRemItem!=rem.end();++iRemItem)
  {
    if (iRemItem->text.empty()) continue;
    CrsPaxRemQry.SetVariable("rem",iRemItem->text.substr(0,250));
    CrsPaxRemQry.SetVariable("rem_code",iRemItem->code);
    CrsPaxRemQry.Execute();
    saved += CrsPaxRemQry.RowsProcessed();
  }
  LogTrace(TRACE5) << __func__
                   << ": saved=" << saved;
}

static void LoadPDRem(const PaxIdWithSegmentPair& paxId, multiset<TPDRemItem> &pdRems)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << paxId().get();
  pdRems.clear();

  DB::TQuery Qry(PgOra::getROSession("CRS_PAX_REM"));
  Qry.Clear();
  Qry.SQLText="SELECT * FROM crs_pax_rem WHERE pax_id=:pax_id AND rem_code LIKE 'PD__'";
  Qry.CreateVariable("pax_id",otInteger,paxId().get());
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
  {
    TPDRemItem item;
    strcpy(item.code, Qry.FieldAsString("rem_code").c_str());
    item.text=Qry.FieldAsString("rem");
    if (item.isPDRem())
      pdRems.insert(item);
  }
}

bool DeleteFreeRem(int pax_id)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM crs_pax_rem "
        "WHERE pax_id=:pax_id "
        "AND rem_code NOT LIKE 'PD__' ",
        PgOra::getRWSession("CRS_PAX_REM"));
  cur.stb()
      .bind(":pax_id", pax_id)
      .exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool DeleteCrsChkd(int pax_id)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM crs_pax_chkd "
        "WHERE pax_id=:pax_id ",
        PgOra::getRWSession("CRS_PAX_CHKD"));
  cur.stb()
      .bind(":pax_id", pax_id)
      .exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

void DeletePDRem(const PaxIdWithSegmentPair& paxId,
                 const vector<TRemItem> &rem1,
                 const vector<TRemItem> &rem2,
                 ModifiedPaxRem& modifiedPaxRem)
{
  multiset<TPDRemItem> curr;
  for(const TRemItem& r : rem1)
    if (r.isPDRem()) curr.insert(r);
  for(const TRemItem& r : rem2)
    if (r.isPDRem()) curr.insert(r);

  bool deletePD=true;
  bool modified=true;
  if (!curr.empty())
  {
    multiset<TPDRemItem> prior;
    LoadPDRem(paxId, prior);
    if (prior.empty()) deletePD=false;
    if (prior==curr) modified=false;
  }

  DB::TQuery Qry(PgOra::getRWSession("CRS_PAX_REM"));
  Qry.Clear();
  Qry.CreateVariable("pax_id",otInteger,paxId().get());
  if (deletePD)
  {
    Qry.SQLText="DELETE FROM crs_pax_rem WHERE pax_id=:pax_id AND rem_code LIKE 'PD__'";
    Qry.Execute();
    if (curr.empty() && Qry.RowsProcessed()==0) modified=false;
  }

  if (modified)
    modifiedPaxRem.add(remPD, paxId);
}

}
