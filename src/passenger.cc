#include "passenger.h"
#include "pax_db.h"
#include "misc.h"
#include "astra_consts.h"
#include "astra_misc.h"
#include "astra_utils.h"
#include "astra_locale.h"
#include "term_version.h"
#include "baggage.h"
#include "qrys.h"
#include "exceptions.h"
#include "jxtlib/jxt_cont.h"
#include "astra_elem_utils.h"
#include "apis_utils.h"
#include "base_tables.h"
#include "checkin.h"
#include <serverlib/algo.h>
#include <serverlib/dbcpp_cursctl.h>
#include "PgOraConfig.h"
#include "db_tquery.h"
#include "arx_daily_pg.h"
#include "tlg/typeb_db.h"

#include <regex>

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"
#include "serverlib/slogger.h"
#include <boost/utility/in_place_factory.hpp>

using namespace std;
using namespace BASIC::date_time;
using namespace AstraLocale;

std::vector<std::pair<std::string, int>> loadCrsPaxET(const PaxId_t& pax_id);

std::ostream& operator << (std::ostream& os, const PaxChanges& value)
{
  switch(value)
  {
    case PaxChanges::New:    os << "PaxChanges::New"; break;
    case PaxChanges::Cancel: os << "PaxChanges::Cancel"; break;
  }
  return os;
}

void addPaxEvent(const PaxIdWithSegmentPair& paxId,
                 const bool cancelledOrMissingBefore,
                 const bool cancelledAfter,
                 ModifiedPax& modifiedPax)
{
  if (cancelledOrMissingBefore==cancelledAfter) return;

  if (!modifiedPax.remove(cancelledAfter?PaxChanges::New:PaxChanges::Cancel, paxId))
    modifiedPax.add(cancelledAfter?PaxChanges::Cancel:PaxChanges::New, paxId);
}

void synchronizePaxEvents(const ModifiedPax& modifiedPax,
                          ModifiedPaxRem& modifiedPaxRem)
{
  for(const PaxIdWithSegmentPair& paxId : modifiedPax.getPaxIds(PaxChanges::New))
    modifiedPaxRem.remove(paxId);
  for(const PaxIdWithSegmentPair& paxId : modifiedPax.getPaxIds(PaxChanges::Cancel))
    modifiedPaxRem.remove(paxId);
}

namespace APIS
{

char ReplacePunctSymbol(char c)
{
  ByteReplace(&c,1,".,:;'\"\\/",
                   "      --");
  return c;
};

char ReplaceDigit(char c)
{
  ByteReplace(&c,1,"1234567890",
                   "----------");
  return c;
};

}; //namespace APIS

bool isTestPaxId(int id)
{
  return id!=ASTRA::NoExists && id>=TEST_ID_BASE && id<=TEST_ID_LAST;
}

int getEmptyPaxId()
{
  return EMPTY_ID;
}

bool isEmptyPaxId(int id)
{
  return id!=ASTRA::NoExists && id==EMPTY_ID;
}

std::set<PaxId_t> loadPaxIdSet(GrpId_t grp_id)
{
  LogTrace(TRACE6) << __func__
                   << ": grp_id=" << grp_id;
  std::set<PaxId_t> result;
  int pax_id = ASTRA::NoExists;
  auto cur = make_db_curs(
        "SELECT pax_id "
        "FROM pax "
        "WHERE grp_id=:grp_id ",
        PgOra::getROSession("PAX"));

  cur.stb()
      .def(pax_id)
      .bind(":grp_id", grp_id.get())
      .exec();

  while (!cur.fen()) {
    result.emplace(pax_id);
  }
  LogTrace(TRACE6) << __func__
                   << ": count=" << result.size();
  return result;
}

bool existsPax(PaxId_t pax_id)
{
  LogTrace(TRACE6) << __func__
                   << ": pax_id=" << pax_id;
  int count = 0;
  auto cur = make_db_curs(
        "SELECT count(1) "
        "FROM pax "
        "WHERE pax.pax_id=:pax_id ",
        PgOra::getROSession("PAX"));

  cur.stb()
      .def(count)
      .bind(":pax_id", pax_id.get())
      .EXfet();

  LogTrace(TRACE6) << __func__
                   << ": count=" << count;
  return count > 0;
}

namespace CheckIn
{

std::optional<TPaxSegmentPair> buildSegment(DbCpp::CursCtl & cur, const PaxId_t pax_id)
{
    int point_id;
    std::string airp_arv;
    cur
        .stb()
        .def(point_id)
        .def(airp_arv)
        .bind(":pax_id", pax_id.get())
        .exfet();
    if(cur.err() == DbCpp::ResultCode::NoDataFound) {
        return std::nullopt;
    }
    return TPaxSegmentPair{point_id, airp_arv};
}

std::optional<TPaxSegmentPair> paxSegment(const PaxId_t &pax_id)
{
    auto cur = make_db_curs(
               "select POINT_DEP, AIRP_ARV "
               "from PAX_GRP, PAX "
               "where PAX_ID=:pax_id and PAX_GRP.GRP_ID=PAX.GRP_ID",
               PgOra::getROSession({"PAX_GRP", "PAX"}));
    return buildSegment(cur, pax_id);
}

std::optional<TPaxSegmentPair> crsSegment(const PaxId_t& pax_id)
{
    auto cur = make_db_curs(
               "select POINT_ID_SPP, AIRP_ARV "
               "from CRS_PAX, CRS_PNR, TLG_BINDING "
               "where PAX_ID=:pax_id and CRS_PAX.PNR_ID = CRS_PNR.PNR_ID "
               "and CRS_PNR.POINT_ID = TLG_BINDING.POINT_ID_TLG",
               PgOra::getROSession({"CRS_PAX", "CRS_PNR", "TLG_BINDING"}));
    return buildSegment(cur, pax_id);
}

std::optional<TPaxSegmentPair> ckinSegment(const GrpId_t& grp_id)
{
    TCkinRoute tckin_route;
    tckin_route.getRouteBefore(grp_id,
                               TCkinRoute::NotCurrent,
                               TCkinRoute::IgnoreDependence,
                               TCkinRoute::WithoutTransit);
    if(tckin_route.empty()) {
        return std::nullopt;
    }
    return TPaxSegmentPair{tckin_route.front().point_dep, tckin_route.front().airp_arv};
}

std::vector<TPaxSegmentPair> paxRouteSegments(const PaxId_t &pax_id)
{
    TCkinRoute tCkinRoute;
    bool get = tCkinRoute.getRoute(pax_id);
    if(!get) {
        std::optional<TPaxSegmentPair> singleFlight = CheckIn::paxSegment(pax_id);
        if(!singleFlight){
            LogTrace(TRACE5) << __FUNCTION__ << " Not found any route for pax: " << pax_id.get();
            return {};
        } else {
            return {*singleFlight};
        }
    }
    std::vector<TPaxSegmentPair> res;
    for(const TCkinRouteItem& seg : tCkinRoute) {
        res.emplace_back(seg.point_dep, seg.airp_arv);
    }
    return res;
}

std::vector<TPaxSegmentPair> crsRouteSegments(const PaxId_t &pax_id)
{
    std::vector<TPaxSegmentPair> res;
    auto crs_route_map = CheckInInterface::getCrsTransferMap(pax_id);
    for(const auto& pair : crs_route_map) {
        const CheckIn::TTransferItem& seg = pair.second;
        auto info = CheckIn::routeInfoFromTrfr(seg);
        res.emplace_back(info.point_id, seg.airp_arv);
    }
    return res;
}

std::vector<int> routePoints(const PaxId_t &pax_id, PaxOrigin checkinType)
{
    std::vector<TPaxSegmentPair> route =
            (checkinType == PaxOrigin::paxCheckIn) ? paxRouteSegments(pax_id)
                                                   : crsRouteSegments(pax_id);
    std::vector<int> res;
    for(const auto& seg : route) {
        std::vector<int> transitRoute = segPoints(seg);
        algo::append(res, transitRoute);
    }
    return res;
}

std::vector<std::string> routeAirps(const PaxId_t &pax_id, PaxOrigin checkinType)
{
    std::vector<TPaxSegmentPair> route =
            (checkinType == PaxOrigin::paxCheckIn) ? paxRouteSegments(pax_id)
                                                   : crsRouteSegments(pax_id);
    std::vector<std::string> res;
    for(const auto& seg : route) {
        std::vector<std::string> transitRoute = segAirps(seg);
        algo::append(res, transitRoute);
    }
    return res;
}

const TPaxGrpCategoriesView& PaxGrpCategories()
{
  static TPaxGrpCategoriesView paxGrpCategories;
  return paxGrpCategories;
}

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
}

const TPaxTknItem& TPaxTknItem::toDB(DB::TQuery &Qry) const
{
  Qry.SetVariable("ticket_no", no);
  if (coupon!=ASTRA::NoExists)
    Qry.SetVariable("coupon_no", coupon);
  else
    Qry.SetVariable("coupon_no", FNull);
  Qry.SetVariable("ticket_rem", rem);
  Qry.SetVariable("ticket_confirm", (int)confirm);
  return *this;
}

TPaxTknItem& TPaxTknItem::fromDB(DB::TQuery &Qry)
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
}

std::vector<TPaxTknItem> TPaxTknItem::loadTKNE(const PaxId_t& pax_id)
{
  std::vector<TPaxTknItem> result;
  const std::vector<std::pair<std::string, int>> ets = loadCrsPaxET(pax_id);
  for (const auto& et: ets) {
    TPaxTknItem item;
    item.no = et.first;
    item.coupon = et.second;
    item.rem = "TKNE";
    result.push_back(item);
  }
  return result;
}

long int TPaxTknItem::getNotEmptyFieldsMask() const
{
  long int result=NO_FIELDS;

  if (!no.empty()) result|=TKN_TICKET_NO_FIELD;
  return result;
};

std::string TPaxTknItem::get_rem_text(bool inf_indicator,
                                      const std::string& lang,
                                      bool strictly_lat,
                                      bool translit_lat,
                                      bool language_lat,
                                      TOutput output) const
{
  ostringstream result;
  result << ElemIdToPrefferedElem(etCkinRemType, rem_code(), efmtCodeNative, lang)
         << " HK1 " << (inf_indicator?"INF":"")
         << (strictly_lat?transliter(convert_char_view(no, strictly_lat), TranslitFormat::V1, strictly_lat):no);
  if (coupon!=ASTRA::NoExists)
    result << "/" << coupon;
  return result.str();
}

int TPaxTknItem::checkedInETCount() const
{
  if (!validET()) return 0;

  auto cur=make_db_curs("SELECT COUNT(*) FROM pax_grp, pax "
                     "WHERE pax.grp_id=pax_grp.grp_id AND "
                     "      pax.ticket_no=:ticket_no AND pax.coupon_no=:coupon_no AND "
                     "      pax_grp.status<>:transit",
                     PgOra::getROSession({"PAX_GRP", "PAX"}));
  int result=0;

  cur
     .stb()
     .bind(":ticket_no", no)
     .bind(":coupon_no", coupon)
     .bind(":transit", EncodePaxStatus(ASTRA::psTransit))
     .def(result)
     .EXfet();

  return result;
}

TPaxTknItem fromPax(const dbo::PAX &pax){
    TPaxTknItem paxTknItem;
    paxTknItem.no = pax.ticket_no;
    paxTknItem.coupon = pax.coupon_no;
    paxTknItem.rem = pax.ticket_rem;
    paxTknItem.confirm = pax.ticket_confirm!=0;
    return paxTknItem;
}

bool LoadPaxTkn(TDateTime part_key, int pax_id, TPaxTknItem &tkn)
{
    if(part_key == ASTRA::NoExists) {
        return LoadPaxTkn(pax_id, tkn);
    }
    tkn.clear();

    dbo::Session session;
    std::optional<dbo::ARX_PAX> pax = session.query<dbo::ARX_PAX>()
            .where("part_key=:part_key AND pax_id=:pax_id")
            .setBind({{"pax_id", pax_id}, {":part_key", DateTimeToBoost(part_key)}});
    if(pax) {
        tkn = fromPax(*pax);
    }
    return !tkn.empty();
};

bool LoadPaxTkn(int pax_id, TPaxTknItem &tkn)
{
    tkn.clear();
    DB::TCachedQuery PaxTknQry(
          PgOra::getROSession("PAX"),
          "SELECT ticket_no, coupon_no, ticket_rem, ticket_confirm "
          "FROM pax "
          "WHERE pax_id=:pax_id",
          QParams() << QParam("pax_id", otInteger, pax_id),
          STDLOG);
    PaxTknQry.get().Execute();
    if (!PaxTknQry.get().Eof) {
      tkn.fromDB(PaxTknQry.get());
    }
    return !tkn.empty();
}

bool LoadCrsPaxTkn(int pax_id, TPaxTknItem &tkn)
{
  tkn.clear();
  const char* sql=
    "SELECT ticket_no, coupon_no, rem_code AS ticket_rem, 0 AS ticket_confirm "
    "FROM crs_pax_tkn "
    "WHERE pax_id=:pax_id "
    "ORDER BY "
    "CASE WHEN rem_code='TKNE' THEN 0 "
    "     WHEN rem_code='TKNA' THEN 1 "
    "     WHEN rem_code='TKNO' THEN 2 "
    "     ELSE 3 END, "
    "     ticket_no,coupon_no ";

  QParams QryParams;
  QryParams << QParam("pax_id", otInteger, pax_id);
  DB::TCachedQuery PaxTknQry(PgOra::getROSession("CRS_PAX_TKN"), sql, QryParams, STDLOG);
  PaxTknQry.get().Execute();
  if (!PaxTknQry.get().Eof) {
    tkn.fromDB(PaxTknQry.get());
  }
  return !tkn.empty();
}

string PaxDocGenderNormalize(const string &pax_doc_gender)
{
  if (pax_doc_gender.empty()) return "";
  int is_female=CheckIn::is_female(pax_doc_gender, "");
  if (is_female!=ASTRA::NoExists)
    return (is_female==0?"M":"F");
  else
    return "";
};

const TPaxDocCompoundType& TPaxDocCompoundType::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;

  if (TReqInfo::Instance()->client_type == ASTRA::ctTerm)
  {
    if (!TReqInfo::Instance()->desk.compatible(DOCO_ADD_TYPES_VERSION))
      NewTextChild(node, "type", type, "");
    else
      NewTextChild(node, "type", type.empty()?"":type+subtype, "");
  }
  else
  {
    NewTextChild(node, "type", type, "");
    NewTextChild(node, "subtype", subtype, "");
  };
  return *this;
}

std::string paxDocCountryToWebXML(const std::string &code,
                                  const boost::optional<AstraLocale::OutputLang>& lang)
{
  std::string result;
  if (!code.empty())
  {
    try
    {
      if (TReqInfo::Instance()->client_type == ASTRA::ctWeb ||
          TReqInfo::Instance()->client_type == ASTRA::ctMobile)
      {
        result=getBaseTable(etPaxDocCountry).get_row("code",code).AsString("country");
        result=lang?ElemIdToPrefferedElem(etCountry, result, efmtCodeNative, lang->get()):result;
      }
    }
    catch (const EBaseTableError&) {};
    if (result.empty()) result=lang?ElemIdToPrefferedElem(etPaxDocCountry, code, efmtCodeNative, lang->get()):code;
  };
  return result;
}

const TPaxDocCompoundType& TPaxDocCompoundType::toWebXML(xmlNodePtr node,
                                                         const boost::optional<AstraLocale::OutputLang>& lang) const
{
  if (node==NULL) return *this;

  NewTextChild(node, "type", lang?ElemIdToPrefferedElem(etPaxDocType, type, efmtCodeNative, lang->get()):type);
  return *this;
}

const TPaxDocItem& TPaxDocItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  //документ
  xmlNodePtr docNode=NewTextChild(node,"document");

  TPaxDocCompoundType::toXML(docNode);
  NewTextChild(docNode, "issue_country", issue_country, "");
  NewTextChild(docNode, "no", no, "");
  NewTextChild(docNode, "nationality", nationality, "");
  if (birth_date!=ASTRA::NoExists)
    NewTextChild(docNode, "birth_date", DateTimeToStr(birth_date, ServerFormatDateTimeAsString));
  NewTextChild(docNode, "gender", gender, "");
  if (expiry_date!=ASTRA::NoExists)
    NewTextChild(docNode, "expiry_date", DateTimeToStr(expiry_date, ServerFormatDateTimeAsString));
  NewTextChild(docNode, "surname", surname, "");
  NewTextChild(docNode, "first_name", first_name, "");
  NewTextChild(docNode, "second_name", second_name, "");
  NewTextChild(docNode, "pr_multi", (int)pr_multi, (int)false);
  NewTextChild(docNode, "scanned_attrs", scanned_attrs, (int)NO_FIELDS);
  return *this;
}

const TPaxDocItem& TPaxDocItem::toWebXML(xmlNodePtr node,
                                         const boost::optional<AstraLocale::OutputLang>& lang) const
{
  if (node==NULL) return *this;
  //документ
  xmlNodePtr docNode=NewTextChild(node,"document");

  TPaxDocCompoundType::toWebXML(docNode, lang);
  NewTextChild(docNode, "issue_country", paxDocCountryToWebXML(issue_country, lang));
  NewTextChild(docNode, "no", no);
  NewTextChild(docNode, "nationality", paxDocCountryToWebXML(nationality, lang));
  if (birth_date!=ASTRA::NoExists)
    NewTextChild(docNode, "birth_date", DateTimeToStr(birth_date, ServerFormatDateTimeAsString));
  else
    NewTextChild(docNode, "birth_date");
  NewTextChild(docNode, "gender", lang?ElemIdToPrefferedElem(etGenderType, gender, efmtCodeNative, lang->get()):gender);
  if (expiry_date!=ASTRA::NoExists)
    NewTextChild(docNode, "expiry_date", DateTimeToStr(expiry_date, ServerFormatDateTimeAsString));
  else
    NewTextChild(docNode, "expiry_date");
  NewTextChild(docNode, "surname", surname);
  NewTextChild(docNode, "first_name", first_name);
  NewTextChild(docNode, "second_name", second_name);
  return *this;
}

TPaxDocCompoundType& TPaxDocCompoundType::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (node2==NULL) return *this;

  if (TReqInfo::Instance()->client_type == ASTRA::ctTerm)
  {
    string compound = NodeAsStringFast("type",node2,"");
    if (!compound.empty())
    {
      TElemFmt fmt;
      ElemToElemId(etPaxDocType, compound, fmt);
      if (fmt!=efmtUnknown) type=compound;
      else
      {
        type=compound.substr(0,1);
        subtype=compound.substr(1);
      };
    }
  }
  else
  {
    type=NodeAsStringFast("type",node2,"");
    subtype=NodeAsStringFast("subtype",node2,"");
  }
  return *this;
}

TPaxDocCompoundType& TPaxDocCompoundType::fromWebXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=NodeAsNode("type", node);

  type=NodeAsStringFast("type",node2);
  return *this;
}

TPaxDocCompoundType& TPaxDocCompoundType::fromMeridianXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (node2==NULL) return *this;

  type=NodeAsStringFast("TYPE", node2, "");
  return *this;
}

TPaxDocItem& TPaxDocItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (node2==NULL) return *this;

  TPaxDocCompoundType::fromXML(node);
  issue_country=NodeAsStringFast("issue_country",node2,"");
  no=NodeAsStringFast("no",node2,"");
  nationality=NodeAsStringFast("nationality",node2,"");
  birth_date = date_fromXML(NodeAsStringFast("birth_date",node2,""));
  gender=PaxDocGenderNormalize(NodeAsStringFast("gender",node2,""));
  expiry_date = date_fromXML(NodeAsStringFast("expiry_date",node2,""));
  surname=NodeAsStringFast("surname",node2,"");
  first_name=NodeAsStringFast("first_name",node2,"");
  second_name=NodeAsStringFast("second_name",node2,"");
  pr_multi=NodeAsIntegerFast("pr_multi",node2,0)!=0;
  scanned_attrs=NodeAsIntegerFast("scanned_attrs",node2,NO_FIELDS);
  return *this;
}

TScannedPaxDocItem& TScannedPaxDocItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (node2==NULL) return *this;

  TPaxDocItem::fromXML(node);
  extra=NodeAsStringFast("extra",node2,"");
  return *this;
}

TPaxDocItem& TPaxDocItem::fromWebXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;

  TPaxDocCompoundType::fromWebXML(node);

  xmlNodePtr node2=NodeAsNode("issue_country", node);
  issue_country=NodeAsStringFast("issue_country",node2);
  no=NodeAsStringFast("no",node2);
  nationality=NodeAsStringFast("nationality",node2);
  if (!NodeIsNULLFast("birth_date",node2))
    birth_date=NodeAsDateTimeFast("birth_date",node2);
  gender=NodeAsStringFast("gender",node2);
  if (!NodeIsNULLFast("expiry_date",node2))
    expiry_date=NodeAsDateTimeFast("expiry_date",node2);
  surname=NodeAsStringFast("surname",node2);
  first_name=NodeAsStringFast("first_name",node2);
  second_name=NodeAsStringFast("second_name",node2);
  return *this;
}

TPaxDocItem& TPaxDocItem::fromMeridianXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (node2==NULL) return *this;

  TPaxDocCompoundType::fromMeridianXML(node);
  issue_country=NodeAsStringFast("ISSUE_COUNTRY", node2, "");
  no=NodeAsStringFast("NO", node2, "");
  nationality=NodeAsStringFast("NATIONALITY",node2,"");
  birth_date = date_fromXML(NodeAsStringFast("BIRTH_DATE", node2, ""));
  gender=PaxDocGenderNormalize(NodeAsStringFast("GENDER", node2, ""));
  expiry_date = date_fromXML(NodeAsStringFast("EXPIRY_DATE", node2, ""));
  surname = upperc(NodeAsStringFast("SURNAME", node2, ""));
  first_name = upperc(NodeAsStringFast("FIRST_NAME", node2, ""));
  second_name = upperc(NodeAsStringFast("SECOND_NAME", node2, ""));
  return *this;
}

const TPaxDocCompoundType& TPaxDocCompoundType::toDB(DB::TQuery &Qry) const
{
  Qry.SetVariable("type", type);
  Qry.SetVariable("subtype", subtype);
  return *this;
}

const TPaxDocItem& TPaxDocItem::toDB(DB::TQuery &Qry) const
{
  TPaxDocCompoundType::toDB(Qry);
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
  Qry.SetVariable("type_rcpt", type_rcpt);
  if (Qry.GetVariableIndex("scanned_attrs")>=0)
    Qry.SetVariable("scanned_attrs", (int)scanned_attrs);
  return *this;
}

TPaxDocCompoundType& TPaxDocCompoundType::fromDB(DB::TQuery &Qry)
{
  clear();
  type=Qry.FieldAsString("type");
  if (Qry.GetFieldIndex("subtype")>=0)
    subtype=Qry.FieldAsString("subtype");
  return *this;
}

TPaxDocItem& TPaxDocItem::fromDB(DB::TQuery &Qry)
{
  clear();
  TPaxDocCompoundType::fromDB(Qry);
  issue_country=Qry.FieldAsString("issue_country");
  no=Qry.FieldAsString("no");
  nationality=Qry.FieldAsString("nationality");
  if (!Qry.FieldIsNULL("birth_date"))
    birth_date=Qry.FieldAsDateTime("birth_date");
  else
    birth_date=ASTRA::NoExists;
  gender=PaxDocGenderNormalize(Qry.FieldAsString("gender"));
  if (!Qry.FieldIsNULL("expiry_date"))
    expiry_date=Qry.FieldAsDateTime("expiry_date");
  else
    expiry_date=ASTRA::NoExists;
  surname=Qry.FieldAsString("surname");
  first_name=Qry.FieldAsString("first_name");
  second_name=Qry.FieldAsString("second_name");
  pr_multi=Qry.FieldAsInteger("pr_multi")!=0;
  type_rcpt=Qry.FieldAsString("type_rcpt");
  if (Qry.GetFieldIndex("scanned_attrs")>=0)
    scanned_attrs=Qry.FieldAsInteger("scanned_attrs");
  return *this;
}

long int TPaxDocItem::getNotEmptyFieldsMask() const
{
  long int result=NO_FIELDS;

  if (!type.empty())                result|=DOC_TYPE_FIELD;
  // if (!subtype.empty())             result|=DOC_SUBTYPE_FIELD;
  if (!issue_country.empty())       result|=DOC_ISSUE_COUNTRY_FIELD;
  if (!no.empty())                  result|=DOC_NO_FIELD;
  if (!nationality.empty())         result|=DOC_NATIONALITY_FIELD;
  if (birth_date!=ASTRA::NoExists)  result|=DOC_BIRTH_DATE_FIELD;
  if (!gender.empty())             result|=DOC_GENDER_FIELD;
  if (expiry_date!=ASTRA::NoExists) result|=DOC_EXPIRY_DATE_FIELD;
  if (!surname.empty())             result|=DOC_SURNAME_FIELD;
  if (!first_name.empty())          result|=DOC_FIRST_NAME_FIELD;
  if (!second_name.empty())         result|=DOC_SECOND_NAME_FIELD;
  return result;
};

long int TPaxDocItem::getEqualAttrsFieldsMask(const TPaxDocItem &item) const
{
  long int result=NO_FIELDS;

  if (type == item.type)                   result|=DOC_TYPE_FIELD;
  // if (subtype == item.subtype)             result|=DOC_SUBTYPE_FIELD;
  if (issue_country == item.issue_country) result|=DOC_ISSUE_COUNTRY_FIELD;
  if (no == item.no)                       result|=DOC_NO_FIELD;
  if (nationality == item.nationality)     result|=DOC_NATIONALITY_FIELD;
  if (birth_date == item.birth_date)       result|=DOC_BIRTH_DATE_FIELD;
  if (gender == item.gender)             result|=DOC_GENDER_FIELD;
  if (expiry_date == item.expiry_date)     result|=DOC_EXPIRY_DATE_FIELD;
  if (surname == item.surname)             result|=DOC_SURNAME_FIELD;
  if (first_name == item.first_name)       result|=DOC_FIRST_NAME_FIELD;
  if (second_name == item.second_name)     result|=DOC_SECOND_NAME_FIELD;
  return result;
};

std::string TPaxDocItem::get_rem_text(bool inf_indicator,
                                      const std::string& lang,
                                      bool strictly_lat,
                                      bool translit_lat,
                                      bool language_lat,
                                      TOutput output) const
{
  ostringstream result;
  result << ElemIdToPrefferedElem(etCkinRemType, rem_code(), efmtCodeNative, lang)
         << " HK1"
         << "/" << (type.empty()?"":ElemIdToPrefferedElem(etPaxDocType, type, efmtCodeNative, lang))
         << "/" << (issue_country.empty()?"":PaxDocCountryIdToPrefferedElem(issue_country, efmtCodeISOInter, lang))
         << "/" << (strictly_lat?transliter(convert_char_view(no, strictly_lat), TranslitFormat::V1, strictly_lat):no)
         << "/" << (nationality.empty()?"":PaxDocCountryIdToPrefferedElem(nationality, efmtCodeISOInter, lang))
         << "/" << (birth_date!=ASTRA::NoExists?DateTimeToStr(birth_date, "ddmmmyy", language_lat):"")
         << "/" << (gender.empty()?"":ElemIdToPrefferedElem(etGenderType, gender, efmtCodeNative, lang)) << (inf_indicator?"I":"")
         << "/" << (expiry_date!=ASTRA::NoExists?DateTimeToStr(expiry_date, "ddmmmyy", language_lat):"")
         << "/" << transliter(surname, TranslitFormat::V1, translit_lat)
         << "/" << transliter(first_name, TranslitFormat::V1, translit_lat)
         << "/" << transliter(second_name, TranslitFormat::V1, translit_lat)
         << "/" << (pr_multi?"H":"");

  return RemoveTrailingChars(result.str(), "/");
}

std::string TPaxDocItem::logStr(const std::string &lang) const
{
  ostringstream result;
  result << ElemIdToPrefferedElem(etPaxDocType, type, efmtCodeNative, lang)
         << "/" << ElemIdToPrefferedElem(etPaxDocCountry, issue_country, efmtCodeNative, lang)
         << "/" << no
         << "/" << ElemIdToPrefferedElem(etPaxDocCountry, nationality, efmtCodeNative, lang)
         << "/" << (birth_date!=ASTRA::NoExists?DateTimeToStr(birth_date, "ddmmmyy", lang!=AstraLocale::LANG_RU):"")
         << "/" << ElemIdToPrefferedElem(etGenderType, gender, efmtCodeNative, lang)
         << "/" << (expiry_date!=ASTRA::NoExists?DateTimeToStr(expiry_date, "ddmmmyy", lang!=AstraLocale::LANG_RU):"")
         << "/" << surname
         << "/" << first_name
         << "/" << second_name;
  return result.str();
}

std::string TPaxDocItem::full_name() const
{
  ostringstream s;
  s << surname;
  if (!first_name.empty())
    s << " " << first_name;
  if (!second_name.empty())
    s << " " << second_name;
  return s.str();
}

std::string TPaxDocItem::getSurnameWithInitials() const
{
  ostringstream s;
  s << surname << " ";
  if (!first_name.empty())
    s << first_name.substr(0, 1) << ".";
  if (!second_name.empty())
    s << second_name.substr(0, 1) << ".";
  return s.str();
}

bool TPaxDocoItem::needPseudoType() const
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  return (reqInfo->client_type==ASTRA::ctTerm && !reqInfo->desk.compatible(DOCO_CONFIRM_VERSION) &&
          doco_confirm && getNotEmptyFieldsMask()==NO_FIELDS);
}

const TPaxDocoItem& TPaxDocoItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  xmlNodePtr docNode=NewTextChild(node,"doco");
  NewTextChild(docNode, "birth_place", birth_place, "");
  TPaxDocCompoundType::toXML(docNode);
  if (needPseudoType())
    ReplaceTextChild(docNode, "type", DOCO_PSEUDO_TYPE);
  NewTextChild(docNode, "no", no, "");
  NewTextChild(docNode, "issue_place", issue_place, "");
  if (issue_date!=ASTRA::NoExists)
    NewTextChild(docNode, "issue_date", DateTimeToStr(issue_date, ServerFormatDateTimeAsString));
  if (expiry_date!=ASTRA::NoExists)
    NewTextChild(docNode, "expiry_date", DateTimeToStr(expiry_date, ServerFormatDateTimeAsString));
  NewTextChild(docNode, "applic_country", applic_country, "");
  NewTextChild(docNode, "scanned_attrs", scanned_attrs, (int)NO_FIELDS);
  return *this;
}

const TPaxDocoItem& TPaxDocoItem::toWebXML(xmlNodePtr node,
                                           const boost::optional<AstraLocale::OutputLang>& lang) const
{
  if (node==NULL) return *this;

  xmlNodePtr docNode=NewTextChild(node,"doco");
  NewTextChild(docNode, "birth_place", birth_place);
  TPaxDocCompoundType::toWebXML(docNode, lang);
  NewTextChild(docNode, "no", no);
  NewTextChild(docNode, "issue_place", issue_place);
  if (issue_date!=ASTRA::NoExists)
    NewTextChild(docNode, "issue_date", DateTimeToStr(issue_date, ServerFormatDateTimeAsString));
  else
    NewTextChild(docNode, "issue_date");
  if (expiry_date!=ASTRA::NoExists)
    NewTextChild(docNode, "expiry_date", DateTimeToStr(expiry_date, ServerFormatDateTimeAsString));
  else
    NewTextChild(docNode, "expiry_date");
  NewTextChild(docNode, "applic_country", paxDocCountryToWebXML(applic_country, lang));
  return *this;
}

TPaxDocoItem& TPaxDocoItem::fromXML(xmlNodePtr node)
{
  clear();
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (!(reqInfo->client_type==ASTRA::ctTerm && !reqInfo->desk.compatible(DOCO_CONFIRM_VERSION)))
    doco_confirm=true;
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (node2==NULL) return *this;

  TPaxDocCompoundType::fromXML(node);
  birth_place=NodeAsStringFast("birth_place",node2,"");
  no=NodeAsStringFast("no",node2,"");
  issue_place=NodeAsStringFast("issue_place",node2,"");
  issue_date = date_fromXML(NodeAsStringFast("issue_date",node2,""));
  expiry_date=date_fromXML(NodeAsStringFast("expiry_date",node2,""));
  applic_country=NodeAsStringFast("applic_country",node2,"");
  scanned_attrs=NodeAsIntegerFast("scanned_attrs",node2,NO_FIELDS);
  if (reqInfo->client_type==ASTRA::ctTerm && !reqInfo->desk.compatible(DOCO_CONFIRM_VERSION))
  {
    if (type==DOCO_PSEUDO_TYPE && getNotEmptyFieldsMask()==DOCO_TYPE_FIELD)
      doco_confirm=true;
    else
      doco_confirm=(getNotEmptyFieldsMask()!=NO_FIELDS);
    if (type==DOCO_PSEUDO_TYPE) type.clear();
  }
  else doco_confirm=true;
  return *this;
}

TPaxDocoItem& TPaxDocoItem::fromWebXML(const xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;

  TPaxDocCompoundType::fromWebXML(node);

  xmlNodePtr node2=NodeAsNode("birth_place", node);
  birth_place=NodeAsStringFast("birth_place",node2);
  no=NodeAsStringFast("no",node2);
  issue_place=NodeAsStringFast("issue_place",node2);
  if (!NodeIsNULLFast("issue_date",node2))
    issue_date=NodeAsDateTimeFast("issue_date",node2);
  if (!NodeIsNULLFast("expiry_date",node2))
    expiry_date=NodeAsDateTimeFast("expiry_date",node2);
  applic_country=NodeAsStringFast("applic_country",node2);
  return *this;
};

TPaxDocoItem& TPaxDocoItem::fromMeridianXML(const xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (node2==NULL) return *this;

  TPaxDocCompoundType::fromMeridianXML(node);
  birth_place=NodeAsStringFast("BIRTH_PLACE", node2, "");
  no=NodeAsStringFast("NO", node2, "");
  issue_place=NodeAsStringFast("ISSUE_PLACE", node2, "");
  issue_date = date_fromXML(NodeAsStringFast("ISSUE_DATE", node2, ""));
  expiry_date = date_fromXML(NodeAsStringFast("EXPIRY_DATE", node2, ""));
  applic_country=NodeAsStringFast("APPLIC_COUNTRY", node2, "");
  doco_confirm=true;
  return *this;
}


const TPaxDocoItem& TPaxDocoItem::toDB(DB::TQuery &Qry) const
{
  TPaxDocCompoundType::toDB(Qry);
  Qry.SetVariable("birth_place", birth_place);
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
  if (Qry.GetVariableIndex("scanned_attrs")>=0)
    Qry.SetVariable("scanned_attrs", (int)scanned_attrs);
  return *this;
}

TPaxDocoItem& TPaxDocoItem::fromDB(DB::TQuery &Qry)
{
  clear();
  TPaxDocCompoundType::fromDB(Qry);
  birth_place=Qry.FieldAsString("birth_place");
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
  applic_country=Qry.FieldAsString("applic_country");
  if (Qry.GetFieldIndex("scanned_attrs")>=0)
    scanned_attrs=Qry.FieldAsInteger("scanned_attrs");
  return *this;
}

long int TPaxDocoItem::getNotEmptyFieldsMask() const
{
  long int result=NO_FIELDS;

  if (!birth_place.empty())         result|=DOCO_BIRTH_PLACE_FIELD;
  if (!type.empty())                result|=DOCO_TYPE_FIELD;
  // if (!subtype.empty())             result|=DOCO_SUBTYPE_FIELD;
  if (!no.empty())                  result|=DOCO_NO_FIELD;
  if (!issue_place.empty())         result|=DOCO_ISSUE_PLACE_FIELD;
  if (issue_date!=ASTRA::NoExists)  result|=DOCO_ISSUE_DATE_FIELD;
  if (expiry_date!=ASTRA::NoExists) result|=DOCO_EXPIRY_DATE_FIELD;
  if (!applic_country.empty())      result|=DOCO_APPLIC_COUNTRY_FIELD;
  return result;
};

long int TPaxDocoItem::getEqualAttrsFieldsMask(const TPaxDocoItem &item) const
{
  long int result=NO_FIELDS;

  if (birth_place == item.birth_place)       result|=DOCO_BIRTH_PLACE_FIELD;
  if (type == item.type)                     result|=DOCO_TYPE_FIELD;
  // if (subtype == item.subtype)               result|=DOCO_SUBTYPE_FIELD;
  if (no == item.no)                         result|=DOCO_NO_FIELD;
  if (issue_place == item.issue_place)       result|=DOCO_ISSUE_PLACE_FIELD;
  if (issue_date == item.issue_date)         result|=DOCO_ISSUE_DATE_FIELD;
  if (expiry_date == item.expiry_date)       result|=DOCO_EXPIRY_DATE_FIELD;
  if (applic_country == item.applic_country) result|=DOCO_APPLIC_COUNTRY_FIELD;
  return result;
};

void TPaxDocoItem::ReplacePunctSymbols()
{
  transform(birth_place.begin(), birth_place.end(), birth_place.begin(), APIS::ReplacePunctSymbol);
  transform(issue_place.begin(), issue_place.end(), issue_place.begin(), APIS::ReplacePunctSymbol);
}

std::string TPaxDocoItem::get_rem_text(bool inf_indicator,
                                       const std::string& lang,
                                       bool strictly_lat,
                                       bool translit_lat,
                                       bool language_lat,
                                       TOutput output) const
{
  ostringstream result;
  result << ElemIdToPrefferedElem(etCkinRemType, rem_code(), efmtCodeNative, lang)
         << " HK1"
         << "/" << transliter(birth_place, TranslitFormat::V1, translit_lat)
         << "/" << (type.empty()?"":ElemIdToPrefferedElem(etPaxDocType, type, efmtCodeNative, lang))
         << "/" << (strictly_lat?transliter(convert_char_view(no, strictly_lat), TranslitFormat::V1, strictly_lat):no)
         << "/" << transliter(issue_place, TranslitFormat::V1, translit_lat)
         << "/" << (issue_date!=ASTRA::NoExists?DateTimeToStr(issue_date, "ddmmmyy", language_lat):"")
         << "/" << (applic_country.empty()?"":PaxDocCountryIdToPrefferedElem(applic_country, efmtCodeISOInter, lang))
         << "/" << (inf_indicator?"I":"");

  return RemoveTrailingChars(result.str(), "/");
}

std::string TPaxDocoItem::logStr(const std::string &lang) const
{
  ostringstream result;
  result << birth_place
         << "/" << ElemIdToPrefferedElem(etPaxDocType, type, efmtCodeNative, lang)
         << "/" << no
         << "/" << issue_place
         << "/" << (issue_date!=ASTRA::NoExists?DateTimeToStr(issue_date, "ddmmmyy", lang!=AstraLocale::LANG_RU):"")
         << "/" << ElemIdToPrefferedElem(etPaxDocCountry, applic_country, efmtCodeNative, lang)
         << "/" << (expiry_date!=ASTRA::NoExists?DateTimeToStr(expiry_date, "ddmmmyy", lang!=AstraLocale::LANG_RU):"");
  return result.str();
}

const TPaxDocaItem& TPaxDocaItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  xmlNodePtr docaNode=NewTextChild(node,"doca");
  NewTextChild(docaNode, "type", type);
  NewTextChild(docaNode, "country", country, "");
  NewTextChild(docaNode, "address", address, "");
  NewTextChild(docaNode, "city", city, "");
  NewTextChild(docaNode, "region", region, "");
  NewTextChild(docaNode, "postal_code", postal_code, "");
  return *this;
};

TPaxDocaItem& TPaxDocaItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (node2==NULL) return *this;
  type=NodeAsStringFast("type",node2);
  country=NodeAsStringFast("country",node2,"");
  address=NodeAsStringFast("address",node2,"");
  city=NodeAsStringFast("city",node2,"");
  region=NodeAsStringFast("region",node2,"");
  postal_code=NodeAsStringFast("postal_code",node2,"");
  return *this;
};

TPaxDocaItem& TPaxDocaItem::fromMeridianXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (node2==NULL) return *this;

  type = upperc(NodeAsStringFast("TYPE", node2, ""));
  country=NodeAsStringFast("COUNTRY", node2, "");
  address=NodeAsStringFast("ADDRESS", node2, "");
  city=NodeAsStringFast("CITY", node2, "");
  region=NodeAsStringFast("REGION", node2, "");
  postal_code=NodeAsStringFast("POSTAL_CODE", node2, "");
  return *this;
}

const TPaxDocaItem& TPaxDocaItem::toDB(DB::TQuery &Qry) const
{
  Qry.SetVariable("type", type);
  Qry.SetVariable("country", country);
  Qry.SetVariable("address", address);
  Qry.SetVariable("city", city);
  Qry.SetVariable("region", region);
  Qry.SetVariable("postal_code", postal_code);
  return *this;
}

TPaxDocaItem& TPaxDocaItem::fromDB(DB::TQuery &Qry)
{
  clear();
  type=Qry.FieldAsString("type");
  country=Qry.FieldAsString("country");
  address=Qry.FieldAsString("address");
  city=Qry.FieldAsString("city");
  region=Qry.FieldAsString("region");
  postal_code=Qry.FieldAsString("postal_code");
  return *this;
}

long int TPaxDocaItem::getNotEmptyFieldsMask() const
{
  long int result=NO_FIELDS;

  if (!type.empty())        result|=DOCA_TYPE_FIELD;
  if (!country.empty())     result|=DOCA_COUNTRY_FIELD;
  if (!address.empty())     result|=DOCA_ADDRESS_FIELD;
  if (!city.empty())        result|=DOCA_CITY_FIELD;
  if (!region.empty())      result|=DOCA_REGION_FIELD;
  if (!postal_code.empty()) result|=DOCA_POSTAL_CODE_FIELD;
  return result;
};

long int TPaxDocaItem::getEqualAttrsFieldsMask(const TPaxDocaItem &item) const
{
  long int result=NO_FIELDS;

  if (type == item.type)               result|=DOCA_TYPE_FIELD;
  if (country == item.country)         result|=DOCA_COUNTRY_FIELD;
  if (address == item.address)         result|=DOCA_ADDRESS_FIELD;
  if (city == item.city)               result|=DOCA_CITY_FIELD;
  if (region == item.region)           result|=DOCA_REGION_FIELD;
  if (postal_code == item.postal_code) result|=DOCA_POSTAL_CODE_FIELD;
  return result;
};

void TPaxDocaItem::ReplacePunctSymbols()
{
  transform(address.begin(), address.end(), address.begin(), APIS::ReplacePunctSymbol);
  transform(city.begin(), city.end(), city.begin(), APIS::ReplacePunctSymbol);
  transform(region.begin(), region.end(), region.begin(), APIS::ReplacePunctSymbol);
  transform(postal_code.begin(), postal_code.end(), postal_code.begin(), APIS::ReplacePunctSymbol);
}

std::string TPaxDocaItem::get_rem_text(bool inf_indicator,
                                       const std::string& lang,
                                       bool strictly_lat,
                                       bool translit_lat,
                                       bool language_lat,
                                       TOutput output) const
{
  ostringstream result;
  result << ElemIdToPrefferedElem(etCkinRemType, rem_code(), efmtCodeNative, lang)
         << " HK1"
         << "/" << type
         << "/" << (country.empty()?"":PaxDocCountryIdToPrefferedElem(country, efmtCodeISOInter, lang))
         << "/" << transliter(address, TranslitFormat::V1, translit_lat)
         << "/" << transliter(city, TranslitFormat::V1, translit_lat)
         << "/" << transliter(region, TranslitFormat::V1, translit_lat)
         << "/" << transliter(postal_code, TranslitFormat::V1, translit_lat)
         << "/" << (inf_indicator?"I":"");

  return RemoveTrailingChars(result.str(), "/");
}

std::string TPaxDocaItem::logStr(const std::string &lang) const
{
  ostringstream result;
  result << type
         << "/" << ElemIdToPrefferedElem(etPaxDocCountry, country, efmtCodeNative, lang)
         << "/" << address
         << "/" << city
         << "/" << region
         << "/" << postal_code;
  return result.str();
}

TPaxDocaItem::TPaxDocaItem(const TAPIType& apiType)
{
  clear();
  if (apiType==apiDocaB) type="B";
  if (apiType==apiDocaR) type="R";
  if (apiType==apiDocaD) type="D";
}

TAPIType TPaxDocaItem::apiType() const
{
  if (type == "B") return apiDocaB;
  if (type == "R") return apiDocaR;
  if (type == "D") return apiDocaD;
  return apiUnknown;
}

boost::optional<TPaxDocItem> TPaxDocItem::get(const PaxOrigin& origin, const PaxId_t& paxId)
{
  boost::optional<TPaxDocItem> result;
  result=boost::in_place();
  switch(origin)
  {
    case paxCheckIn:
      if (LoadPaxDoc(paxId.get(), result.get())) return result;
      break;
    case paxPnl:
      if (LoadCrsPaxDoc(paxId.get(), result.get())) return result;
      break;
    default:
      break;
  }

  return boost::none;
}

TPaxDocItem fromPaxDoc(dbo::PAX_DOC & pax_doc)
{
  TPaxDocItem doc;
  doc.type = pax_doc.type;
  //if(pax_doc.subtype >= 0) {
      doc.subtype = pax_doc.subtype;
  //}

  doc.issue_country = pax_doc.issue_country;
  doc.no = pax_doc.no;
  doc.nationality = pax_doc.nationality;
  doc.birth_date = pax_doc.birth_date==Dates::not_a_date_time ? ASTRA::NoExists
                                       : BoostToDateTime( pax_doc.birth_date);

  doc.gender = PaxDocGenderNormalize(pax_doc.gender);
  doc.expiry_date = pax_doc.expiry_date==Dates::not_a_date_time ? ASTRA::NoExists
                                       : BoostToDateTime( pax_doc.expiry_date);
  doc.surname = pax_doc.surname;
  doc.first_name = pax_doc.first_name;
  doc.second_name = pax_doc.second_name;
  doc.pr_multi = pax_doc.pr_multi != 0;
  doc.type_rcpt= pax_doc.type_rcpt;
  if(pax_doc.scanned_attrs >= 0) {
      doc.scanned_attrs = pax_doc.scanned_attrs;
  }
  return doc;
}

bool LoadPaxDoc(std::optional<Dates::DateTime_t> part_key, PaxId_t pax_id, TPaxDocItem &doc)
{
  if(!part_key)
  {
      return LoadPaxDoc(pax_id.get(), doc);
  }
  doc.clear();
  dbo::Session session;
  std::optional<dbo::ARX_PAX_DOC> pax_doc = session.query<dbo::ARX_PAX_DOC>()
          .where("part_key=:part_key AND pax_id=:pax_id")
          .setBind({{"pax_id", pax_id.get()}, {":part_key", *part_key}});
  if(pax_doc) {
      doc = fromPaxDoc(*pax_doc);
  }
  return !doc.empty();
};

bool LoadPaxDoc(int pax_id, TPaxDocItem &doc)
{
    doc.clear();
    const char* sql = "SELECT * FROM pax_doc WHERE pax_id=:pax_id";
    QParams QryParams;
    QryParams << QParam("pax_id", otInteger, pax_id);
    DB::TCachedQuery PaxDocQry(PgOra::getROSession("PAX_DOC"),sql, QryParams, STDLOG);
    PaxDocQry.get().Execute();
    if (!PaxDocQry.get().Eof) doc.fromDB(PaxDocQry.get());
    return !doc.empty();
}


std::string GetPaxDocStr(std::optional<Dates::DateTime_t> part_key,
                         int pax_id,
                         bool with_issue_country,
                         const string &lang)
{
  ostringstream result;

  TPaxDocItem doc;
  if (LoadPaxDoc(part_key, PaxId_t(pax_id), doc) && !doc.no.empty())
  {
    result << doc.no;
    if (with_issue_country && !doc.issue_country.empty())
    {
      if (lang.empty())
        result << " " << ElemIdToPrefferedElem(etPaxDocCountry, doc.issue_country, efmtCodeNative, TReqInfo::Instance()->desk.lang, true);
      else
        result << " " << ElemIdToPrefferedElem(etPaxDocCountry, doc.issue_country, efmtCodeNative, lang, true);
    };
  };
  return result.str();
};

bool LoadCrsPaxDoc(int pax_id, TPaxDocItem &doc)
{
  doc.clear();
  const char* sql=
    "SELECT * "
    "FROM crs_pax_doc "
    "WHERE pax_id=:pax_id "
    "ORDER BY "
    "CASE WHEN type='P' THEN 0 "
    "     WHEN type IS NOT NULL THEN 1 "
    "     ELSE 2 END, "
    "CASE WHEN rem_code='DOCS' THEN 0 "
    "     ELSE 1 END, "
    "no NULLS LAST";


  QParams QryParams;
  QryParams << QParam("pax_id", otInteger, pax_id);
  DB::TCachedQuery PaxDocQry(PgOra::getROSession("CRS_PAX_DOC"), sql, QryParams, STDLOG);
  PaxDocQry.get().Execute();
  if (!PaxDocQry.get().Eof) doc.fromDB(PaxDocQry.get());
  return !doc.empty();
}

boost::optional<TPaxDocoItem> TPaxDocoItem::get(const PaxOrigin& origin, const PaxId_t& paxId)
{
  boost::optional<TPaxDocoItem> result;
  result=boost::in_place();
  switch(origin)
  {
    case paxCheckIn:
      if (LoadPaxDoco(ASTRA::NoExists, paxId.get(), result.get())) return result;
      break;
    case paxPnl:
      if (LoadCrsPaxDoco(paxId.get(), result.get())) return result;
      break;
    default:
      break;
  }

  return boost::none;
}

TPaxDocoItem fromPaxDoco(dbo::PAX_DOCO & pax_doco, std::optional<int> doco_confirm)
{
  TPaxDocoItem doco;

  doco.doco_confirm = false;
  if(doco_confirm){
      doco.doco_confirm = *doco_confirm != 0;
  }

  doco.type = pax_doco.type;
  //if(pax_doco.subtype >= 0) {
      doco.subtype = pax_doco.subtype;
  //}
  doco.birth_place = pax_doco.birth_place;
  doco.no = pax_doco.no;
  doco.issue_place = pax_doco.issue_place;
  doco.issue_date = pax_doco.issue_date==Dates::not_a_date_time ? ASTRA::NoExists
                                                           : BoostToDateTime( pax_doco.issue_date);
  doco.expiry_date = pax_doco.expiry_date==Dates::not_a_date_time ? ASTRA::NoExists
                                                           : BoostToDateTime( pax_doco.expiry_date);

  doco.applic_country = pax_doco.applic_country;
  if(pax_doco.scanned_attrs >= 0) {
      doco.scanned_attrs = pax_doco.scanned_attrs;
  }
  return doco;
}

bool LoadPaxDoco(TDateTime part_key, int pax_id, TPaxDocoItem &doc)
{
    doc.clear();
    if(part_key == ASTRA::NoExists) {
        return LoadPaxDoco(pax_id, doc);
    }
    dbo::Session session;
    std::optional<int> doco_confirm = session.query<int>("SELECT doco_confirm").from("ARX_PAX")
          .where("part_key=:part_key AND pax_id=:pax_id")
          .setBind({{"pax_id", pax_id}, {":part_key", DateTimeToBoost(part_key)}});

    std::optional<dbo::ARX_PAX_DOCO> pax_doco = session.query<dbo::ARX_PAX_DOCO>()
          .where("part_key=:part_key AND pax_id=:pax_id")
          .setBind({{"pax_id", pax_id}, {":part_key", DateTimeToBoost(part_key)}});

    if(pax_doco) {
        doc = fromPaxDoco(*pax_doco, doco_confirm);
    }
    return !doc.empty();
}

bool LoadPaxDoco(int pax_id, TPaxDocoItem &doc)
{
    doc.clear();
    QParams QryParams;
    QryParams << QParam("pax_id", otInteger, pax_id);
    DB::TCachedQuery PaxQry(
          PgOra::getROSession("PAX"),
          "SELECT doco_confirm FROM pax WHERE pax_id=:pax_id",
          QryParams, STDLOG);
    doc.doco_confirm=false;
    PaxQry.get().Execute();
    if (!PaxQry.get().Eof)
    doc.doco_confirm=PaxQry.get().FieldIsNULL("doco_confirm") ||
                 PaxQry.get().FieldAsInteger("doco_confirm")!=0;

    DB::TCachedQuery PaxDocQry(
          PgOra::getROSession("PAX_DOCO"),
          "SELECT * FROM pax_doco WHERE pax_id=:pax_id",
          QryParams, STDLOG);
    PaxDocQry.get().Execute();
    if (!PaxDocQry.get().Eof) doc.fromDB(PaxDocQry.get());
    return !doc.empty();
}

bool LoadCrsPaxDoco(int pax_id, TPaxDocoItem &doc)
{
  doc.clear();
  const char* sql=
    "SELECT birth_place, type, no, issue_place, issue_date, NULL AS expiry_date, applic_country "
    "FROM crs_pax_doco "
    "WHERE pax_id=:pax_id AND rem_code='DOCO' "
    "ORDER BY "
    "CASE WHEN type='V' THEN 0 "
    "     WHEN type IS NOT NULL THEN 1 "
    "     ELSE 2 END, "
    "no NULLS LAST";

  QParams QryParams;
  QryParams << QParam("pax_id", otInteger, pax_id);
  DB::TCachedQuery PaxDocQry(PgOra::getROSession("CRS_PAX_DOCO"),
                             sql, QryParams, STDLOG);
  PaxDocQry.get().Execute();
  if (!PaxDocQry.get().Eof) doc.fromDB(PaxDocQry.get());
  return !doc.empty();
};

bool LoadCrsPaxVisa(int pax_id, TPaxDocoItem &doc)
{
  if (LoadCrsPaxDoco(pax_id, doc))
  {
    if (doc.type!="V") doc.clear();
  }
  return !doc.empty();
}

boost::optional<TPaxDocaItem> TDocaMap::get(const TAPIType& type) const
{
  const auto i = find(type);
  if (i!=end()) return i->second;
  return boost::none;
}

void TDocaMap::get(const TAPIType& type, TPaxDocaItem& doca) const
{
  boost::optional<CheckIn::TPaxDocaItem> docaOpt=get(type);
  doca=docaOpt?docaOpt.get():TPaxDocaItem(type);
}

void ConvertDoca(const CheckIn::TDocaMap& doca_map,
                 TPaxDocaItem &docaB,
                 TPaxDocaItem &docaR,
                 TPaxDocaItem &docaD)
{
  doca_map.get(apiDocaB, docaB);
  doca_map.get(apiDocaR, docaR);
  doca_map.get(apiDocaD, docaD);
}

const TDocaMap& TDocaMap::toXML(xmlNodePtr node) const
{
  if (node==nullptr) return *this;
  xmlNodePtr docaNode=NewTextChild(node, "addresses");
  for(const auto& d : *this)
    d.second.toXML(docaNode);

  return *this;
}

TDocaMap& TDocaMap::fromXML(xmlNodePtr node)
{
  clear();
  if (node==nullptr) return *this;

  for(xmlNodePtr docaNode=node->children; docaNode!=nullptr; docaNode=docaNode->next)
  {
    TPaxDocaItem docaItem;
    docaItem.fromXML(docaNode);
    if (docaItem.empty()) continue;
    if (docaItem.apiType() != apiUnknown) (*this)[docaItem.apiType()] = docaItem;
  };

  return *this;
}

TDocaMap TDocaMap::get(const PaxOrigin& origin, const PaxId_t& paxId)
{
  TDocaMap result;
  switch(origin)
  {
    case paxCheckIn:
      LoadPaxDoca(ASTRA::NoExists, paxId.get(), result);
      break;
    case paxPnl:
      LoadCrsPaxDoca(paxId.get(), result);
      break;
    default:
      break;
  }

  return result;
}

TPaxDocaItem fromPaxDoca(const dbo::PAX_DOCA & pax_doca)
{
    TPaxDocaItem doca_item;
    doca_item.type = pax_doca.type;
    doca_item.country = pax_doca.country;
    doca_item.address = pax_doca.address;
    doca_item.city = pax_doca.city;
    doca_item.region = pax_doca.region;
    doca_item.postal_code = pax_doca.postal_code;
    return doca_item;
}

bool LoadPaxDoca(TDateTime part_key, int pax_id, CheckIn::TDocaMap &doca_map)
{
  if(part_key == ASTRA::NoExists) {
      return LoadPaxDoca(pax_id, doca_map);
  }
  doca_map.clear();
  dbo::Session session;
  std::vector<dbo::ARX_PAX_DOCA> pax_docs = session.query<dbo::ARX_PAX_DOCA>()
          .where("part_key=:part_key AND pax_id=:pax_id")
          .setBind({{"pax_id", pax_id}, {":part_key", DateTimeToBoost(part_key)}});

  for(const auto & doca : pax_docs)
  {
    TPaxDocaItem docaItem = fromPaxDoca(doca);
    doca_map[docaItem.apiType()] = docaItem;
  }
  return !doca_map.empty();
}

bool LoadPaxDoca(int pax_id, CheckIn::TDocaMap &doca_map)
{
    doca_map.clear();
    const char* sql = "SELECT * FROM pax_doca WHERE pax_id=:pax_id";
    QParams QryParams;
    QryParams << QParam("pax_id", otInteger, pax_id);
    DB::TCachedQuery PaxDocQry(PgOra::getROSession("PAX_DOCA"), sql, QryParams, STDLOG);
    for(PaxDocQry.get().Execute(); !PaxDocQry.get().Eof; PaxDocQry.get().Next())
    {
        TPaxDocaItem docaItem;
        docaItem.fromDB(PaxDocQry.get());
        doca_map[docaItem.apiType()] = docaItem;
    }
    return !doca_map.empty();
}

bool LoadCrsPaxDoca(int pax_id, CheckIn::TDocaMap &doca_map)
{
  doca_map.clear();
  const char* sql=
    "SELECT type, country, address, city, region, postal_code "
    "FROM crs_pax_doca "
    "WHERE pax_id=:pax_id AND rem_code='DOCA' AND type IS NOT NULL "
    "ORDER BY type, address ";
  QParams QryParams;
  QryParams << QParam("pax_id", otInteger, pax_id);
  DB::TCachedQuery PaxDocaQry(PgOra::getROSession("CRS_PAX_DOCA"), sql, QryParams, STDLOG);
  PaxDocaQry.get().Execute();
  string prior_type;
  for(;!PaxDocaQry.get().Eof;PaxDocaQry.get().Next())
  {
    if (prior_type!=PaxDocaQry.get().FieldAsString("type"))
    {
      TPaxDocaItem docaItem;
      docaItem.fromDB(PaxDocaQry.get());
      if (docaItem.suitableForDB())
      {
        doca_map[docaItem.apiType()] = docaItem;
        prior_type=PaxDocaQry.get().FieldAsString("type");
      }
    };
  };
  return !doca_map.empty();
};

void SaveCrsPaxTranslit(const PointIdTlg_t& point_id, const PaxId_t& pax_id,
                        const std::string& surname, const std::string& name, TranslitFormat translit_format)
{
  QParams params;
  params << QParam("pax_id", otInteger, pax_id.get())
         << QParam("surname", otString, surname)
         << QParam("name", otString, name)
         << QParam("translit_format", otInteger, int(translit_format));
  DB::TCachedQuery UpdQry(
        PgOra::getRWSession("CRS_PAX_TRANSLIT"),
        "UPDATE crs_pax_translit "
        "SET surname = :surname, "
        "    name = :name "
        "WHERE pax_id = :pax_id "
        "AND translit_format = :translit_format ",
        params,
        STDLOG);
  UpdQry.get().Execute();

  if (UpdQry.get().RowsProcessed() == 0) {
    DB::TCachedQuery InsQry(
          PgOra::getRWSession("CRS_PAX_TRANSLIT"),
          "INSERT INTO crs_pax_translit (point_id, pax_id, surname, name, translit_format) "
          "VALUES (:point_id, :pax_id, :surname, :name, :translit_format) ",
          params << QParam("point_id", otInteger, point_id.get()),
          STDLOG);
    InsQry.get().Execute();
  }
}

void SaveCrsPaxTranslit(const PointIdTlg_t& point_id, const PaxId_t& pax_id,
                        const std::string& surname, const std::string& name)
{
  for (int fmt = 1; fmt <= int(TranslitFormat::V3); fmt++) {
    SaveCrsPaxTranslit(point_id, pax_id,
                       transliter(surname, TranslitFormat(fmt), true),
                       transliter(name, TranslitFormat(fmt), true),
                       TranslitFormat(fmt));
  }
}

void SavePaxTranslit(const PointId_t& point_id, const PaxId_t& pax_id, const GrpId_t& grp_id,
                     const std::string& surname, const std::string& name, TranslitFormat translit_format)
{
  QParams params;
  params << QParam("pax_id", otInteger, pax_id.get())
         << QParam("surname", otString, surname)
         << QParam("name", otString, name)
         << QParam("translit_format", otInteger, int(translit_format));
  DB::TCachedQuery UpdQry(
        PgOra::getRWSession("PAX_TRANSLIT"),
        "UPDATE pax_translit "
        "SET surname = :surname, "
        "    name = :name "
        "WHERE pax_id = :pax_id "
        "AND translit_format = :translit_format ",
        params,
        STDLOG);
  UpdQry.get().Execute();

  if (UpdQry.get().RowsProcessed() == 0) {
    DB::TCachedQuery InsQry(
          PgOra::getRWSession("PAX_TRANSLIT"),
          "INSERT INTO pax_translit (point_id, pax_id, grp_id, surname, name, translit_format) "
          "VALUES (:point_id, :pax_id, :grp_id, :surname, :name, :translit_format) ",
          params << QParam("point_id", otInteger, point_id.get())
                 << QParam("grp_id", otInteger, grp_id.get()),
          STDLOG);
    InsQry.get().Execute();
  }
}

void SavePaxTranslit(const PointId_t& point_id, const PaxId_t& pax_id, const GrpId_t& grp_id,
                     const std::string& surname, const std::string& name)
{
  for (int fmt = 1; fmt <= int(TranslitFormat::V3); fmt++) {
    SavePaxTranslit(point_id, pax_id, grp_id,
                    transliter(surname, TranslitFormat(fmt), true),
                    transliter(name, TranslitFormat(fmt), true),
                    TranslitFormat(fmt));
  }
}

void SavePaxDoc(const PaxIdWithSegmentPair& paxId,
                const bool paxIdUsedBefore,
                const TPaxDocItem& doc,
                ModifiedPaxRem& modifiedPaxRem)
{
  TPaxDocItem priorDoc;
  if (paxIdUsedBefore)
    LoadPaxDoc(paxId().get(), priorDoc);
  SavePaxDoc(paxId, doc, priorDoc, modifiedPaxRem);
}

bool DeletePaxDoc(PaxId_t pax_id)
{
  LogTrace(TRACE6) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM pax_doc "
        "WHERE pax_id=:pax_id ",
        PgOra::getRWSession("PAX_DOC"));
  cur.stb()
      .bind(":pax_id", pax_id.get())
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool SavePaxDoc(const TPaxDocItem& item, PaxId_t pax_id)
{
  LogTrace(TRACE6) << __func__
                   << ": pax_id=" << pax_id;
  short notNull = 0;
  short null = -1;
  auto cur = make_db_curs(
        "INSERT INTO pax_doc ( "
        "pax_id,type,subtype,issue_country,no,nationality,birth_date,gender,expiry_date, "
        "surname,first_name,second_name,pr_multi,type_rcpt,scanned_attrs "
        ") VALUES ( "
        ":pax_id,:type,:subtype,:issue_country,:no,:nationality,:birth_date,:gender,:expiry_date, "
        ":surname,:first_name,:second_name,:pr_multi,:type_rcpt,:scanned_attrs) ",
        PgOra::getRWSession("PAX_DOC"));
  cur.stb()
      .bind(":pax_id", pax_id.get())
      .bind(":type", item.type)
      .bind(":subtype", item.subtype)
      .bind(":issue_country", item.issue_country)
      .bind(":no", item.no)
      .bind(":nationality", item.nationality)
      .bind(":birth_date",
            item.birth_date!=ASTRA::NoExists ? DateTimeToBoost(item.birth_date)
                                             : boost::posix_time::ptime(),
            item.birth_date!=ASTRA::NoExists ? &notNull : &null)
      .bind(":gender", item.gender)
      .bind(":expiry_date",
            item.expiry_date!=ASTRA::NoExists ? DateTimeToBoost(item.expiry_date)
                                              : boost::posix_time::ptime(),
            item.expiry_date!=ASTRA::NoExists ? &notNull : &null)
      .bind(":surname", item.surname)
      .bind(":first_name", item.first_name)
      .bind(":second_name", item.second_name)
      .bind(":pr_multi", int(item.pr_multi))

      .bind(":type_rcpt", item.type_rcpt)
      .bind(":scanned_attrs", int(item.scanned_attrs))
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

std::string LoadCrsTypeRcpt(PaxId_t pax_id, const std::string& no)
{
  LogTrace(TRACE6) << __func__
                   << ": pax_id=" << pax_id
                   << ", no=" << no;
  std::string type_rcpt;
  auto cur = make_db_curs(
        "SELECT MAX(type_rcpt) "
        "FROM crs_pax_doc "
        "WHERE pax_id=:pax_id "
        "AND no=:no "
        "AND type_rcpt IS NOT NULL ",
        PgOra::getROSession("CRS_PAX_DOC"));
  cur.stb()
      .defNull(type_rcpt, std::string())
      .bind(":pax_id", pax_id.get())
      .bind(":no", no)
      .EXfet();

  LogTrace(TRACE6) << __func__
                   << ": found="
                   << (cur.err() != DbCpp::ResultCode::NoDataFound);

  return type_rcpt;
}

void SavePaxDoc(const PaxIdWithSegmentPair& paxId,
                const TPaxDocItem& doc,
                const TPaxDocItem& priorDoc,
                ModifiedPaxRem& modifiedPaxRem)
{
  bool deleteOld=!priorDoc.empty();
  bool insertNew=!doc.empty();

  if ((deleteOld || insertNew) &&
      !doc.equal(priorDoc))
  {
    if (deleteOld) {
      DeletePaxDoc(paxId());
    }
    if (insertNew) {
      TPaxDocItem doc4save = doc;
      doc4save.type_rcpt = LoadCrsTypeRcpt(paxId(), doc.no);
      SavePaxDoc(doc4save, paxId());
    }

    modifiedPaxRem.add(remDOC, paxId);
  }
}

void SavePaxDoco(const PaxIdWithSegmentPair& paxId,
                 const bool paxIdUsedBefore,
                 const TPaxDocoItem& doc,
                 ModifiedPaxRem& modifiedPaxRem)
{
  TPaxDocoItem priorDoc;
  if (paxIdUsedBefore)
    LoadPaxDoco(paxId().get(), priorDoc);
  SavePaxDoco(paxId, doc, priorDoc, modifiedPaxRem);
}

void SavePaxDoco(const PaxIdWithSegmentPair& paxId,
                 const TPaxDocoItem& doc,
                 const TPaxDocoItem& priorDoc,
                 ModifiedPaxRem& modifiedPaxRem)
{
  bool deleteOld=!priorDoc.empty();
  bool insertNew=!doc.empty();

  if ((deleteOld || insertNew) &&
      !doc.equal(priorDoc))
  {
    if (deleteOld != 0) {
      DB::TCachedQuery QryDel(
            PgOra::getRWSession("PAX_DOCO"),
            "DELETE FROM pax_doco WHERE pax_id=:pax_id",
            QParams() << QParam("pax_id",otInteger,paxId().get()),
            STDLOG);
      QryDel.get().Execute();
    }
    if (insertNew != 0) {
      DB::TCachedQuery QryIns(
            PgOra::getRWSession("PAX_DOCO"),
            "INSERT INTO pax_doco "
            "  (pax_id,birth_place,type,subtype,no,issue_place,issue_date,expiry_date,applic_country,scanned_attrs) "
            "VALUES "
            "  (:pax_id,:birth_place,:type,:subtype,:no,:issue_place,:issue_date,:expiry_date,:applic_country,:scanned_attrs) ",
            QParams() << QParam("pax_id",otInteger, paxId().get())
                      << QParam("birth_place",otString)
                      << QParam("type",otString)
                      << QParam("subtype",otString)
                      << QParam("no",otString)
                      << QParam("issue_place",otString)
                      << QParam("issue_date",otDate)
                      << QParam("expiry_date",otDate)
                      << QParam("applic_country",otString)
                      << QParam("scanned_attrs",otInteger),
            STDLOG);
      doc.toDB(QryIns.get());
      QryIns.get().Execute();
    }

    modifiedPaxRem.add(remDOCO, paxId);
  }

  DB::TCachedQuery PaxQry(
        PgOra::getRWSession("PAX"),
        "UPDATE pax SET "
        "doco_confirm=:doco_confirm "
        "WHERE pax_id=:pax_id",
        QParams() << QParam("pax_id", otInteger, paxId().get())
                  << QParam("doco_confirm", otInteger, (int)doc.doco_confirm),
        STDLOG);
  PaxQry.get().Execute();
}

void SavePaxDoca(const PaxIdWithSegmentPair& paxId,
                 const bool paxIdUsedBefore,
                 const CheckIn::TDocaMap& doca,
                 ModifiedPaxRem& modifiedPaxRem)
{
  CheckIn::TDocaMap priorDoca;
  if (paxIdUsedBefore)
    LoadPaxDoca(paxId().get(), priorDoca);
  SavePaxDoca(paxId, doca, priorDoca, modifiedPaxRem);
}

void SavePaxDoca(const PaxIdWithSegmentPair& paxId,
                 const TDocaMap& doca,
                 const TDocaMap& priorDoca,
                 ModifiedPaxRem& modifiedPaxRem)
{
  const TDocaMap newDoca=algo::filter(doca, [](const auto& item) { return item.second.suitableForDB(); } );

  bool deleteOld=!priorDoca.empty();
  bool insertNew=!newDoca.empty();

  if ((deleteOld || insertNew) &&
       priorDoca!=newDoca)
  {
    if (deleteOld)
    {
      DB::TCachedQuery Qry(
            PgOra::getRWSession("PAX_DOCA"),
            "DELETE FROM pax_doca WHERE pax_id=:pax_id",
            QParams() << QParam("pax_id", otInteger, paxId().get()),
            STDLOG);
      Qry.get().Execute();
    }
    if (insertNew)
    {
      DB::TCachedQuery Qry(
            PgOra::getRWSession("PAX_DOCA"),
            "INSERT INTO pax_doca "
            "  (pax_id,type,country,address,city,region,postal_code) "
            "VALUES "
            "  (:pax_id,:type,:country,:address,:city,:region,:postal_code)",
            QParams() << QParam("pax_id", otInteger, paxId().get())
                      << QParam("type", otString)
                      << QParam("country", otString)
                      << QParam("address", otString)
                      << QParam("city", otString)
                      << QParam("region", otString)
                      << QParam("postal_code", otString),
            STDLOG);
      for(const auto& i : newDoca)
      {
        i.second.toDB(Qry.get());
        Qry.get().Execute();
      }
    }

    modifiedPaxRem.add(remDOCA, paxId);
  }
}

void SavePaxFQT(const PaxIdWithSegmentPair& paxId,
                const bool paxIdUsedBefore,
                const std::set<TPaxFQTItem>& fqts,
                ModifiedPaxRem& modifiedPaxRem)
{
  std::set<TPaxFQTItem> prior;
  if (paxIdUsedBefore)
    LoadPaxFQT(paxId().get(), prior);
  SavePaxFQT(paxId, fqts, prior, modifiedPaxRem);
}

void SavePaxFQT(const PaxIdWithSegmentPair& paxId,
                const std::set<TPaxFQTItem>& fqts,
                const std::set<TPaxFQTItem>& prior,
                ModifiedPaxRem& modifiedPaxRem)
{
  bool deleteOld=!prior.empty();
  bool insertNew=!fqts.empty();

  if ((deleteOld || insertNew) &&
       prior!=fqts)
  {
    if (deleteOld)
    {
      deletePaxFQT(paxId());
    }

    if (insertNew) {
      for(const TPaxFQTItem& fqt : fqts)
        fqt.saveDb(paxId());
    }

    modifiedPaxRem.add(remFQT, paxId);
  }
}

void SavePaxRem(const PaxIdWithSegmentPair& paxId,
                const bool paxIdUsedBefore,
                const std::multiset<TPaxRemItem>& rems,
                ModifiedPaxRem& modifiedPaxRem)
{
  std::multiset<TPaxRemItem> prior;
  if (paxIdUsedBefore)
    LoadPaxRem(paxId().get(), prior);
  SavePaxRem(paxId, rems, prior, modifiedPaxRem);
}

void SavePaxRem(const PaxIdWithSegmentPair& paxId,
                const std::multiset<TPaxRemItem>& rems,
                const std::multiset<TPaxRemItem>& prior,
                ModifiedPaxRem& modifiedPaxRem)
{
  std::multiset<TPaxRemItem> remsToDB=
      algo::filter(rems, [&paxId, &modifiedPaxRem] (const TPaxRemItem& rem)
                         {
                           if (rem.text.empty()) return false; //защита от пустой ремарки (иногда может почему то приходить с терминала)
                           TRemCategory cat=getRemCategory(rem);
                           if (cat==remAPPSOverride || cat==remAPPSStatus)
                           {
                             modifiedPaxRem.add(cat, paxId);
                             return false;  // не храним информацию о перезаписи и повторной отправке в APPS
                           }
                           return true;
                         });

  bool deleteOld=!prior.empty();
  bool insertNew=!remsToDB.empty();

  if ((deleteOld || insertNew) &&
       prior!=remsToDB)
  {
    DB::TQuery RemQry(PgOra::getRWSession("PAX_REM"), STDLOG);
    RemQry.CreateVariable("pax_id",otInteger,paxId().get());
    if (deleteOld)
    {
      RemQry.SQLText="DELETE FROM pax_rem WHERE pax_id=:pax_id";
      RemQry.Execute();
    }

    if (insertNew)
    {
      RemQry.SQLText=
        "INSERT INTO pax_rem(pax_id,rem,rem_code) VALUES(:pax_id,:rem,:rem_code)";
      RemQry.DeclareVariable("rem",otString);
      RemQry.DeclareVariable("rem_code",otString);

      for(const TPaxRemItem& rem : remsToDB)
      {
        rem.toDB(RemQry);
        RemQry.Execute();
      }
    }

    modifiedPaxRem.add(remUnknown, paxId);
  }
}

const TSimplePaxItem& TSimplePaxItem::toEmulXML(xmlNodePtr node, bool PaxUpdatesPending) const
{
  if (node==nullptr) return *this;

  xmlNodePtr paxNode=node;
  NewTextChild(paxNode, "pax_id", id);
  NewTextChild(paxNode, "surname", surname);
  NewTextChild(paxNode, "name", name);
  if (PaxUpdatesPending)
  {
    //были ли изменения по пассажиру CheckInInterface::SavePax определяет по наличию тега refuse
    NewTextChild(paxNode, "pers_type", EncodePerson(pers_type));
    NewTextChild(paxNode,"refuse", refuse);
    if (bag_pool_num!=ASTRA::NoExists)
      NewTextChild(paxNode, "bag_pool_num", bag_pool_num);
    else
      NewTextChild(paxNode, "bag_pool_num");

    if (TknExists) tkn.toXML(paxNode);
  }
  NewTextChild(paxNode, "tid", tid);

  return *this;
}

const TPaxItem& TPaxItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;

  xmlNodePtr paxNode=node;
  NewTextChild(paxNode, "pax_id", id);
  NewTextChild(paxNode, "surname", surname);
  NewTextChild(paxNode, "name", name);
  NewTextChild(paxNode, "pers_type", EncodePerson(pers_type));
  NewTextChild(paxNode, "crew_type", CrewTypes().encode(crew_type));
  NewTextChild(paxNode, "seat_no", seat_no);
  NewTextChild(paxNode, "seat_type", seat_type);
  NewTextChild(paxNode, "seats", seats);
  NewTextChild(paxNode, "refuse", refuse);
  NewTextChild(paxNode, "reg_no", reg_no);
  NewTextChild(paxNode, "subclass", subcl);
  cabin.toXML(paxNode, "cabin_");
  if (bag_pool_num!=ASTRA::NoExists)
    NewTextChild(paxNode, "bag_pool_num", bag_pool_num);
  else
    NewTextChild(paxNode, "bag_pool_num");
  NewTextChild(paxNode, "tid", tid);

  if (TknExists) tkn.toXML(paxNode);
  if (DocExists) doc.toXML(paxNode);
  if (DocoExists || doco.needPseudoType()) doco.toXML(paxNode);
  if (DocaExists) doca_map.toXML(paxNode);
  return *this;
};

TPaxItem& TPaxItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;

  TReqInfo *reqInfo=TReqInfo::Instance();

  if (!NodeIsNULLFast("pax_id",node2))
    id=NodeAsIntegerFast("pax_id",node2);
  surname=NodeAsStringFast("surname",node2);
  name=NodeAsStringFast("name",node2);
  tid=NodeAsIntegerFast("tid",node2,ASTRA::NoExists);
  PaxUpdatesPending=GetNodeFast("refuse",node2)!=NULL;
  if (tid==ASTRA::NoExists)
  {
    //новая регистрация
    seat_no=NodeAsStringFast("seat_no",node2);
    seat_type=NodeAsStringFast("seat_type",node2);
    seats=NodeAsIntegerFast("seats",node2);
    if (reqInfo->client_type==ASTRA::ctPNL)
      reg_no=NodeAsIntegerFast("reg_no",node2);
  };

  if (tid==ASTRA::NoExists || PaxUpdatesPending)
  {
    pers_type=DecodePerson(NodeAsStringFast("pers_type",node2));
    if (!NodeIsNULLFast("crew_type",node2, true))
      crew_type=CrewTypes().decode(NodeAsStringFast("crew_type",node2));
    if (PaxUpdatesPending)
      refuse=NodeAsStringFast("refuse",node2);
    tkn.fromXML(node);
    TknExists=true;
    if (!NodeIsNULLFast("bag_pool_num",node2))
      bag_pool_num=NodeAsIntegerFast("bag_pool_num",node2);

    if (refuse.empty() && api_doc_applied())
    {
      if (reqInfo->client_type==ASTRA::ctTerm)
      {
        //терминал
        doc.fromXML(GetNodeFast("document",node2));
        doco.fromXML(GetNodeFast("doco",node2));
        doca_map.fromXML(GetNodeFast("addresses",node2));
        DocExists=true;
        DocoExists=true;
        DocaExists=true;
      }
      else
      {
        //киоски и веб
        xmlNodePtr docNode=GetNodeFast("document",node2);
        doc.fromXML(docNode);
        xmlNodePtr docoNode=GetNodeFast("doco",node2);
        doco.fromXML(docoNode);
        xmlNodePtr docaNode=GetNodeFast("addresses",node2);
        doca_map.fromXML(docaNode);
        DocExists=(tid==ASTRA::NoExists || docNode!=NULL);
        DocoExists=(tid==ASTRA::NoExists || docoNode!=NULL);
        DocaExists=(tid==ASTRA::NoExists || docaNode!=NULL);
      };
    };
  };

  subcl=NodeAsStringFast("subclass",node2,"");
  return *this;
}

const TComplexClass& TComplexClass::toDB(DB::TQuery &Qry, const std::string& fieldPrefix) const
{
  if (Qry.GetVariableIndex(fieldPrefix+"subclass")>=0)
    Qry.SetVariable(fieldPrefix+"subclass", subcl);
  if (Qry.GetVariableIndex(fieldPrefix+"class")>=0)
    Qry.SetVariable(fieldPrefix+"class", cl);
  if (Qry.GetVariableIndex(fieldPrefix+"class_grp")>=0)
    cl_grp!=ASTRA::NoExists?Qry.SetVariable(fieldPrefix+"class_grp", cl_grp):
                            Qry.SetVariable(fieldPrefix+"class_grp", FNull);
  return *this;
}

const TComplexClass& TComplexClass::toXML(xmlNodePtr node, const std::string& fieldPrefix) const
{
  if (node==NULL) return *this;

  NewTextChild(node, string(fieldPrefix+"subclass").c_str(), subcl);
  NewTextChild(node, string(fieldPrefix+"class").c_str(), cl);

  return *this;
}

const TPaxItem& TPaxItem::toDB(DB::TQuery &Qry) const
{
  id!=ASTRA::NoExists?Qry.SetVariable("pax_id", id):
                      Qry.SetVariable("pax_id", FNull);
  if (Qry.GetVariableIndex("surname")>=0)
    Qry.SetVariable("surname", surname);
  if (Qry.GetVariableIndex("name")>=0)
    Qry.SetVariable("name", name);
  if (Qry.GetVariableIndex("pers_type")>=0)
    Qry.SetVariable("pers_type", EncodePerson(pers_type));
  if (Qry.GetVariableIndex("crew_type")>=0)
    Qry.SetVariable("crew_type", CrewTypes().encode(crew_type));
  if (Qry.GetVariableIndex("is_jmp")>=0)
    Qry.SetVariable("is_jmp", (int)is_jmp);
  if (Qry.GetVariableIndex("seat_type")>=0)
    Qry.SetVariable("seat_type", seat_type);
  if (Qry.GetVariableIndex("seats")>=0)
    Qry.SetVariable("seats", seats);
  if (Qry.GetVariableIndex("refuse")>=0)
    Qry.SetVariable("refuse", refuse);
  if (Qry.GetVariableIndex("pr_brd")>=0)
    Qry.SetVariable("pr_brd", (int)pr_brd);
  if (Qry.GetVariableIndex("pr_exam")>=0)
    Qry.SetVariable("pr_exam", (int)pr_exam);
  if (Qry.GetVariableIndex("wl_type")>=0)
    Qry.SetVariable("wl_type", wl_type);
  if (Qry.GetVariableIndex("reg_no")>=0)
    Qry.SetVariable("reg_no", reg_no);
  if (Qry.GetVariableIndex("subclass")>=0)
    Qry.SetVariable("subclass", subcl);
  cabin.toDB(Qry, "cabin_");
  if (Qry.GetVariableIndex("bag_pool_num")>=0)
    bag_pool_num!=ASTRA::NoExists?Qry.SetVariable("bag_pool_num", bag_pool_num):
                                  Qry.SetVariable("bag_pool_num", FNull);
  if (Qry.GetVariableIndex("tid")>=0)
    Qry.SetVariable("tid", tid);
  if (Qry.GetVariableIndex("ticket_no")>=0)
    tkn.toDB(Qry);
  return *this;
}

ASTRA::TGender::Enum TSimplePaxItem::genderFromDB(DB::TQuery &Qry)
{
  if (Qry.FieldIsNULL("is_female"))
    return ASTRA::TGender::Unknown;
  else if (Qry.FieldAsInteger("is_female")!=0)
    return ASTRA::TGender::Female;
  else
    return ASTRA::TGender::Male;
}

ASTRA::TTrickyGender::Enum TSimplePaxItem::getTrickyGender(ASTRA::TPerson pers_type, ASTRA::TGender::Enum gender)
{
  ASTRA::TTrickyGender::Enum result=ASTRA::TTrickyGender::Male;
  switch(pers_type)
  {
    case ASTRA::adult:
      if (gender==ASTRA::TGender::Female) result=ASTRA::TTrickyGender::Female;
      break;
    case ASTRA::child:
      result=ASTRA::TTrickyGender::Child;
      break;
    case ASTRA::baby:
      result=ASTRA::TTrickyGender::Infant;
      break;
    default:
      break;
  };
  return result;
}

const std::string& TSimplePaxItem::origClassFromCrsSQL()
{
  static const std::string result=" COALESCE(NULLIF(crs_pax.etick_class, ''), NULLIF(crs_pax.orig_class, ''), crs_pnr.class) ";
  return result;
}

const std::string& TSimplePaxItem::origSubclassFromCrsSQL()
{
  static const std::string result=" CASE WHEN crs_pnr.class="+origClassFromCrsSQL()+
                                  "      THEN crs_pnr.subclass "
                                  "      ELSE COALESCE(NULLIF(crs_pax.etick_subclass, ''), NULLIF(crs_pax.orig_subclass, ''), crs_pnr.subclass) END ";
  return result;
}

const std::string& TSimplePaxItem::cabinClassFromCrsSQL()
{
  static const std::string result=" CASE WHEN crs_pax.cabin_class IS NOT NULL THEN crs_pax.cabin_class ELSE crs_pnr.class END ";
  return result;
}

const std::string& TSimplePaxItem::cabinSubclassFromCrsSQL()
{
  static const std::string result=" crs_pnr.subclass ";
  return result;
}

TComplexClass& TComplexClass::fromDB(DB::TQuery &Qry, const std::string& fieldPrefix)
{
  clear();
  subcl=Qry.FieldAsString(fieldPrefix+"subclass");
  cl=Qry.FieldAsString(fieldPrefix+"class");
  cl_grp=Qry.FieldIsNULL(fieldPrefix+"class_grp")?ASTRA::NoExists:Qry.FieldAsInteger(fieldPrefix+"class_grp");
  return *this;
}

TSimplePaxItem& TSimplePaxItem::fromDB(DB::TQuery &Qry)
{
  clear();
  id=Qry.FieldAsInteger("pax_id");
  grp_id=Qry.FieldAsInteger("grp_id");
  surname=Qry.FieldAsString("surname");
  name=Qry.FieldAsString("name");
  pers_type=DecodePerson(Qry.FieldAsString("pers_type").c_str());
  if(Qry.GetFieldIndex("crew_type") >= 0)
      crew_type = CrewTypes().decode(Qry.FieldAsString("crew_type"));
  is_jmp=Qry.FieldAsInteger("is_jmp")!=0;
  if (Qry.GetFieldIndex("seat_no") >= 0)
    seat_no=Qry.FieldAsString("seat_no");
  seat_type=Qry.FieldAsString("seat_type");
  seats=Qry.FieldAsInteger("seats");
  refuse=Qry.FieldAsString("refuse");
  pr_brd=Qry.FieldAsInteger("pr_brd")!=0;
  pr_exam=Qry.FieldAsInteger("pr_exam")!=0;
  reg_no=Qry.FieldAsInteger("reg_no");
  subcl=Qry.FieldAsString("subclass");
  cabin.fromDB(Qry, "cabin_");
  bag_pool_num=Qry.FieldIsNULL("bag_pool_num")?ASTRA::NoExists:Qry.FieldAsInteger("bag_pool_num");
  tid=Qry.FieldAsInteger("tid");
  tkn.fromDB(Qry);
  TknExists=true;
  gender=genderFromDB(Qry);
  return *this;
}

TSimplePaxItem& TSimplePaxItem::fromPax(const dbo::PAX &pax)
{
  clear();
  id = pax.pax_id;
  grp_id = pax.grp_id;
  surname = pax.surname;
  name = pax.name;
  pers_type = DecodePerson(pax.pers_type.c_str());
  crew_type = CrewTypes().decode(pax.crew_type);
  is_jmp= pax.is_jmp!=0;

  seat_type= pax.seat_type;
  seats = pax.seats;
  refuse = pax.refuse;
  pr_brd = pax.pr_brd!=0;
  pr_exam = pax.pr_exam!=0;
  reg_no = pax.reg_no;
  subcl = pax.subclass;

  cabin.subcl= pax.cabin_subclass;
  cabin.cl=pax.cabin_class;
  cabin.cl_grp = pax.cabin_class_grp ;

  bag_pool_num = pax.bag_pool_num;
  tid = pax.tid;

  tkn.no = pax.ticket_no;
  tkn.coupon = pax.coupon_no;
  tkn.rem = pax.ticket_rem;
  tkn.confirm = pax.ticket_confirm!=0;

  TknExists = true;

  if(pax.is_female == ASTRA::NoExists) {
      gender = ASTRA::TGender::Unknown;
  } else if(pax.is_female !=0) {
      gender = ASTRA::TGender::Female;
  } else {
      gender = ASTRA::TGender::Male;
  }
  return *this;
}

TSimplePaxItem& TSimplePaxItem::fromPax(const dbo::ARX_PAX &arx_pax)
{
    this->fromPax(static_cast<dbo::PAX>(arx_pax));
    seat_no = arx_pax.seat_no;
    return *this;
}

TSimplePaxItem& TSimplePaxItem::fromDBCrs(DB::TQuery &Qry, bool withTkn)
{
  clear();
  id=Qry.FieldAsInteger("pax_id");
  pnr_id=Qry.FieldAsInteger("pnr_id");
  surname=Qry.FieldAsString("surname");
  name=Qry.FieldAsString("name");
  if (isTest())
  {
    pers_type=ASTRA::adult;
    seats=1;
  }
  else
  {
    pers_type=DecodePerson(Qry.FieldAsString("pers_type").c_str());
    seat_type=Qry.FieldAsString("seat_type");
    seats=Qry.FieldAsInteger("seats");
  }
  if (Qry.GetFieldIndex("seat_no")>=0)
    seat_no = Qry.FieldAsString("seat_no");
  if (Qry.GetFieldIndex("reg_no")>=0)
    reg_no = Qry.FieldIsNULL("reg_no")?ASTRA::NoExists:Qry.FieldAsInteger("reg_no");
  if (Qry.GetFieldIndex("tid")>=0)
    tid = Qry.FieldIsNULL("tid")?ASTRA::NoExists:Qry.FieldAsInteger("tid");

  subcl = Qry.FieldAsString("subclass");
  cabin.fromDB(Qry, "cabin_");

  if (withTkn)
  {
    if (isTest())
    {
      tkn.no=Qry.FieldAsString("tkn_no");
      if (!tkn.no.empty())
      {
        tkn.coupon=1;
        tkn.rem="TKNE";
      };
    }
    else LoadCrsPaxTkn(id, tkn);
  }
  TknExists=withTkn;
  return *this;
}

bool TSimplePaxItem::getByPaxId(int pax_id, TDateTime part_key)
{
  clear();
  if(part_key != ASTRA::NoExists) {
      dbo::Session session;
      std::optional<dbo::ARX_PAX> arx_pax = session.query<dbo::ARX_PAX>()
              .where("part_key=:part_key AND pax_id=:pax_id")
              .setBind({{"pax_id", pax_id}, {":part_key", DateTimeToBoost(part_key)}});
      if(!arx_pax) {
          return false;
      }
      fromPax(*arx_pax);

  } else {
      DB::TCachedQuery PaxQry(
            PgOra::getROSession("PAX"),
            "SELECT * FROM pax WHERE pax_id=:pax_id",
            QParams() << QParam("pax_id", otInteger, pax_id),
            STDLOG);
      PaxQry.get().Execute();
      if (PaxQry.get().Eof) return false;
      fromDB(PaxQry.get());
  }
  return true;
}

bool TSimplePaxItem::getCrsByPaxId(PaxId_t pax_id, bool skip_deleted)
{
  clear();
  QParams QryParams;
  QryParams << QParam("pax_id", otInteger, pax_id.get());
  DB::TCachedQuery PaxQry(
        PgOra::getROSession({"CRS_PAX","CRS_PNR"}),
        "SELECT crs_pax.*, "
        + CheckIn::TSimplePaxItem::origSubclassFromCrsSQL() + " AS subclass, "
        + CheckIn::TSimplePaxItem::cabinSubclassFromCrsSQL() + " AS cabin_subclass, "
        + CheckIn::TSimplePaxItem::cabinClassFromCrsSQL() + " AS cabin_class, "
        "NULL AS cabin_class_grp "
        "FROM crs_pax, crs_pnr "
        "WHERE crs_pax.pax_id=:pax_id "
        "AND crs_pax.pnr_id=crs_pnr.pnr_id "
        "AND crs_pnr.system='CRS' "
        + std::string(skip_deleted ? "AND pr_del=0 " : ""),
        QryParams, STDLOG);
  PaxQry.get().Execute();
  if (PaxQry.get().Eof) return false;
  fromDBCrs(PaxQry.get(), false /*withTkn*/);
  return true;
}

std::list<TSimplePaxItem> TSimplePaxItem::getByGrpId(GrpId_t grp_id)
{
  TSimplePaxList result;
  DB::TQuery Qry(PgOra::getROSession("PAX"), STDLOG);
  Qry.SQLText = "SELECT * FROM pax "
                "WHERE grp_id=:grp_id";
  Qry.CreateVariable("grp_id", otInteger, grp_id.get());
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next()) {
    TSimplePaxItem item;
    item.fromDB(Qry);
    result.push_back(item);
  }
  return std::move(result);
}

std::list<TSimplePaxItem> TSimplePaxItem::getByDepPointId(const PointId_t& point_id)
{
  std::list<TSimplePaxItem> result;
  DB::TCachedQuery Qry(
        PgOra::getROSession({"PAX","PAX_GRP"}),
        "SELECT pax.* "
        "FROM pax, pax_grp "
        "WHERE pax_grp.point_dep=:point_id "
        "AND pax.grp_id = pax_grp.grp_id ",
        QParams() << QParam("point_id", otInteger, point_id.get()),
        STDLOG);
  Qry.get().Execute();
  for(; !Qry.get().Eof; Qry.get().Next()) {
    TSimplePaxItem item;
    item.fromDB(Qry.get());
    result.push_back(item);
  }
  return result;
}

TPaxItem& TPaxItem::fromDB(DB::TQuery &Qry)
{
  clear();
  TSimplePaxItem::fromDB(Qry);
  DocExists=CheckIn::LoadPaxDoc(id, doc);
  DocoExists=CheckIn::LoadPaxDoco(id, doco);
  DocaExists=CheckIn::LoadPaxDoca(id, doca_map);
  return *this;
}

int TPaxItem::is_female() const
{
  if (!DocExists) return ASTRA::NoExists;
  return CheckIn::is_female(doc.gender, name);
};

int is_female(const string &pax_doc_gender, const string &pax_name)
{
  int result=ASTRA::NoExists;
  if (!pax_doc_gender.empty())
  {
    if (pax_doc_gender.substr(0,1)=="M") result=0;
    if (pax_doc_gender.substr(0,1)=="F") result=1;
  };
  if (result==ASTRA::NoExists)
  {
    TPaxNameTitle info;
    string name_tmp(pax_name);
    GetPaxNameTitle(name_tmp, false, info);
    if (!info.empty()) result=(int)info.is_female;
  };
  return result;
};

std::string TSimplePaxItem::full_name() const
{
  ostringstream s;
  s << surname;
  if (!name.empty())
    s << " " << name;
  return s.str();
};

bool TSimplePaxItem::isCBBG() const
{
  return name=="CBBG";
}

bool TSimplePaxItem::api_doc_applied() const
{
  return !isCBBG();
}

bool TSimplePaxItem::upward_within_bag_pool(const TSimplePaxItem& pax) const
{
  int res;
  res=int(refuse!=ASTRA::refuseAgentError)-int(pax.refuse!=ASTRA::refuseAgentError);
  if (res==0) res=int(pers_type==ASTRA::adult || pers_type==ASTRA::child)-
                  int(pax.pers_type==ASTRA::adult || pax.pers_type==ASTRA::child);
  if (res==0) res=int(seats>0)-int(pax.seats>0);
  if (res==0) res=int(refuse.empty())-int(pax.refuse.empty());
  //специально чтобы при прочих равных выбрать ВЗ:
  if (res==0) res=-(int(pers_type)-int(pax.pers_type));

  return res>0;
}

void TSimplePaxItem::UpdTid(int pax_id)
{
  DB::TCachedQuery Qry(
        PgOra::getRWSession("PAX"),
        "UPDATE pax SET "
        "tid=:tid "
        "WHERE pax_id=:pax_id",
        QParams() << QParam("pax_id", otInteger, pax_id)
                  << QParam("tid", otInteger, PgOra::getSeqNextVal_int("CYCLE_TID__SEQ")),
        STDLOG);
  Qry.get().Execute();
}

std::string TSimplePaxItem::checkInStatus() const
{
  if (id==ASTRA::NoExists) return "unknown";
  if (!refuse.empty()) return "refused";
  if (pr_brd) return "boarded";
  if (grp_id!=ASTRA::NoExists) return "checked";
  return "not_checked";
}

boost::optional<TComplexClass> TSimplePaxItem::getCrsClass(const PaxId_t& paxId, bool onlyIfClassChange)
{
  DB::TCachedQuery Qry(PgOra::getROSession({"CRS_PNR","CRS_PAX"}),
    "SELECT "+origClassFromCrsSQL()+" AS orig_class, "
    "       "+cabinSubclassFromCrsSQL()+" AS subclass, "
    "       "+cabinClassFromCrsSQL()+" AS class "
    "FROM crs_pnr, crs_pax "
    "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
    "      crs_pnr.system='CRS' AND "
    "      crs_pax.pax_id=:pax_id AND "
    "      crs_pax.pr_del=0",
    QParams() << QParam("pax_id", otInteger, paxId.get()),
    STDLOG);
  Qry.get().Execute();
  if (!Qry.get().Eof)
  {
    std::string currentClass=Qry.get().FieldAsString("class");
    std::string originalClass=Qry.get().FieldAsString("orig_class");
    if (!onlyIfClassChange || currentClass!=originalClass)
    {
      TComplexClass result;
      result.subcl=Qry.get().FieldAsString("subclass");
      result.cl=currentClass;
      return result;
    }
  }
  return {};
}

std::string TSimplePaxItem::getCabinClass() const
{
  if (cabin.cl.empty() && grp_id!=ASTRA::NoExists)
  {
    TSimplePaxGrpItem grp;
    if (grp.getByGrpId(grp_id)) return grp.cl;
  }

  return cabin.cl;
}

std::string TSimplePaxItem::getCabinSubclass() const
{
  return cabin.subcl.empty()?subcl:cabin.subcl;
}

std::string TSimplePaxItem::getSeatNo(const std::string& fmt) const
{
  if (id==ASTRA::NoExists) return "";
  DB::TCachedQuery Qry(PgOra::getROSession("PAX_GRP"), // get_seat_no
    "SELECT salons.get_seat_no(:pax_id, :seats, :is_jmp, pax_grp.status, pax_grp.point_dep, :fmt, rownum) seat_no " /// ROWNUM
    "FROM pax_grp WHERE grp_id=:grp_id",
    QParams() << QParam("pax_id", otInteger, id)
              << QParam("grp_id", otInteger, grp_id)
              << QParam("seats", otInteger, seats)
              << QParam("is_jmp", otInteger, (int)is_jmp)
              << QParam("fmt", otString, fmt),
              STDLOG);
  Qry.get().Execute();
  if (!Qry.get().Eof)
    return Qry.get().FieldAsString("seat_no");
  return "";
}

bool TSimplePaxItem::cabinClassToDB() const
{
  if (id==ASTRA::NoExists) return false;

  DB::TCachedQuery UpdQry(PgOra::getRWSession("pax"),
                   "UPDATE pax "
                   "SET cabin_subclass=:cabin_subclass, "
                   "    cabin_class=:cabin_class, "
                   "    cabin_class_grp=:cabin_class_grp, "
                   "    tid=:new_tid "
                   "WHERE pax_id=:pax_id AND tid=:tid",
                   QParams() << QParam("pax_id", otInteger, id)
                             << QParam("tid", otInteger, tid)
                             << QParam("new_tid", otInteger, PgOra::getSeqNextVal_int("CYCLE_TID__SEQ"))
                             << QParam("cabin_subclass", otString)
                             << QParam("cabin_class", otString)
                             << QParam("cabin_class_grp", otInteger),
                   STDLOG);
  cabin.toDB(UpdQry.get(), "cabin_");
  UpdQry.get().Execute();

  return UpdQry.get().RowsProcessed()>0;
}

boost::optional<WeightConcept::TPaxNormComplex> TSimplePaxItem::getRegularNorm() const
{
  using namespace WeightConcept;

  if (id==ASTRA::NoExists) return boost::none;

  TPaxNormComplexContainer norms;
  PaxNormsFromDB(ASTRA::NoExists, id, norms);
  return algo::find_opt_if<boost::optional>(norms, [](const TPaxNormComplex& n) { return n.isRegular(); });
}

template<class T>
static void getBaggageInHoldList(int id, T& list)
{
  list.clear();

  if (id==ASTRA::NoExists) return;

  TPaxServiceLists serviceLists;
  serviceLists.fromDB(id, false);
  for(const TPaxServiceListsItem& i : serviceLists)
    if (i.trfer_num==0 && i.category==TServiceCategory::BaggageInHold)
    {
      list.fromDB(i.primary(), true);
      break;
    }

  for(typename T::iterator i=list.begin(); i!=list.end();)
  {
    auto& item=i->second;
    item.isBaggageInHold()?++i:i=list.erase(i);
  }
}

void TSimplePaxItem::getBaggageListForSBDO(TRFISCListWithProps& list) const
{
  std::string rfiscs=Sirena::getRFISCsFromBaggageNorm(PaxId_t(id));
  if (rfiscs.empty())
  {
    list.clear();
    return;
  }

  getBaggageInHoldList(id, list);
  list.setPriority();
  //отфильтруем только те типы, которые входят в норму
  for(TRFISCListWithProps::iterator i=list.begin(); i!=list.end();)
  {
    std::string line_regex("(^|,)\\s*"+i->first.RFISC+"\\s*(,|$)");
    if (!std::regex_search(rfiscs, std::regex(line_regex)))
      i=list.erase(i);
    else
      ++i;
  }
}

void TSimplePaxItem::getBaggageListForSBDO(TBagTypeList& list) const
{
  getBaggageInHoldList(id, list);
}

boost::optional<TBagQuantity> TSimplePaxItem::getCrsBagNorm(const PaxId_t& paxId)
{
  int dbBagNorm=ASTRA::NoExists;
  string dbBagNormUnit;

  auto cur = make_db_curs("SELECT bag_norm, bag_norm_unit FROM crs_pax WHERE pax_id=:pax_id AND pr_del=0",
                          PgOra::getROSession("CRS_PAX"));
  cur
      .stb()
      .bind(":pax_id", paxId.get())
      .defNull(dbBagNorm, ASTRA::NoExists)
      .defNull(dbBagNormUnit, "")
      .exfet();

  if (dbBagNorm!=ASTRA::NoExists && dbBagNorm>=0 && !dbBagNormUnit.empty())
    return TBagQuantity(dbBagNorm, dbBagNormUnit);

  return {};
}

TAPISItem& TAPISItem::fromDB(int pax_id)
{
  clear();
  CheckIn::LoadPaxDoc(pax_id, doc);
  CheckIn::LoadPaxDoco(pax_id, doco);
  CheckIn::LoadPaxDoca(pax_id, doca_map);
  return *this;
};

const TAPISItem& TAPISItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;

  xmlNodePtr apisNode=NewTextChild(node, "apis");
  doc.toXML(apisNode);
  doco.toXML(apisNode);
  doca_map.toXML(apisNode);
  return *this;
};

TPaxListItem& TPaxListItem::fromXML(xmlNodePtr paxNode, bool trfer_confirm)
{
  TReqInfo *reqInfo=TReqInfo::Instance();

  clear();
  if (paxNode==NULL) return *this;
  xmlNodePtr node2=paxNode->children;

  node=paxNode;
  pax.fromXML(paxNode);
  dont_check_payment=NodeAsIntegerFast("dont_check_payment", node2, 0)!=0;
  crs_seat_no=NodeAsStringFast("preseat_no", node2, "");
  if (GetNodeFast("original_crs_pax_id", node2)!=nullptr)
    originalCrsPaxId=boost::in_place(NodeAsIntegerFast("original_crs_pax_id", node2));
  //ремарки
  xmlNodePtr remNode=GetNodeFast("rems",node2);
  if (remNode!=NULL)
  {
    remsExists=true;
    for(remNode=remNode->children; remNode!=NULL; remNode=remNode->next)
    {
      CheckIn::TPaxRemItem rem;
      rem.fromXML(remNode);
      TRemCategory cat=getRemCategory(rem);
      if (cat==remASVC) continue; //пропускаем переданные ASVC
      rems.insert(rem);
    };
    if (reqInfo->api_mode ||
            (reqInfo->client_type==ASTRA::ctTerm && reqInfo->desk.compatible(FQT_TIER_LEVEL_VERSION)))
    {
      //ремарки FQT
      for(remNode=NodeAsNodeFast("fqt_rems",node2)->children; remNode!=NULL; remNode=remNode->next)
        addFQT(CheckIn::TPaxFQTItem().fromXML(remNode));
      checkFQTTierLevel();
    };
  };

  if (trfer_confirm)
  {
    //нормы
    xmlNodePtr normNode=GetNodeFast("norms",node2);
    if (normNode!=NULL)
    {
      norms=boost::in_place();
      for(normNode=normNode->children; normNode!=NULL; normNode=normNode->next)
        norms.get().push_back(WeightConcept::TPaxNormItem().fromXML(normNode));
      for(WeightConcept::TPaxNormItem& n : norms.get())
        n.getListKeyByPaxId(pax.id, 0, boost::none, "TPaxListItem::fromXML");
    }
  }

  return *this;
};

void TPaxListItem::addFQT(const CheckIn::TPaxFQTItem &fqt)
{
  if (!fqts.insert(fqt).second)
  {
    ostringstream rem;
    rem << fqt.rem_code() << " " << fqt.logStr(TReqInfo::Instance()->desk.lang);
    throw UserException("MSG.REMARK.DUPLICATED",
                        LParams() << LParam("surname", pax.full_name())
                                  << LParam("remark", rem.str()));
  };
}

void TPaxListItem::checkFQTTierLevel()
{
  TPaxFQTCards cards;
  GetPaxFQTCards(fqts, cards);

  //простановка tier_level
  set<TPaxFQTItem> tmp_fqts;
  for(set<TPaxFQTItem>::const_iterator f=fqts.begin(); f!=fqts.end(); ++f)
  {
    TPaxFQTItem item=*f;
    TPaxFQTCards::const_iterator i=cards.find(*f);
    if (i==cards.end())
      throw EXCEPTIONS::Exception("%s: i==cards.end() strange situation!", __FUNCTION__);
    if (!item.tier_level.empty() &&
        !i->second.tier_level.empty() &&
        item.tier_level!=i->second.tier_level)
      throw UserException("MSG.REMARK.DIFFERENT_TIER_LEVEL",
                          LParams() << LParam("surname", pax.full_name())
                                    << LParam("fqt_card", i->first.no_str(TReqInfo::Instance()->desk.lang)));
    item.copyIfBetter(i->second);
    tmp_fqts.insert(item);
  };

  fqts=tmp_fqts;
}

void TPaxListItem::checkImportantRems(bool new_checkin, ASTRA::TPaxStatus grp_status)
{
  for(int pass=0; pass<2; pass++)
  {
    Statistic<string> crewRemsStat;
    for(multiset<CheckIn::TPaxRemItem>::iterator r=rems.begin(); r!=rems.end(); ++r)
    {
      if (pass==0)
      {
        TRemCategory cat=getRemCategory(*r);
        if (!r->code.empty() && cat==remCREW)
          crewRemsStat.add(r->code);
      }
      else
      {
        if (r->code=="JMP")
          crewRemsStat.add(r->code);
      }
    };
    if (!crewRemsStat.empty())
    {
      //есть ремарки, связанные с экипажем
      //пороверяем дублирование ремарок и взаимоислючающие ремарки
      if (crewRemsStat.size()>1)
      {
        //взаимоисключающие
        throw UserException("MSG.REMARK.MUTUALLY_EXCLUSIVE",
                            LParams() << LParam("surname", pax.full_name())
                            << LParam("remark1", crewRemsStat.begin()->first)
                            << LParam("remark2", crewRemsStat.rbegin()->first));
      }
      else
      {
        if (crewRemsStat.begin()->second>1)
          //повторяющиеся
          throw UserException("MSG.REMARK.DUPLICATED",
                              LParams() << LParam("surname", pax.full_name())
                              << LParam("remark", crewRemsStat.begin()->first));

        if (new_checkin)
        {
          if (pass==0)
          {
            if (grp_status==ASTRA::psCrew)
              pax.crew_type=ASTRA::TCrewType::Unknown;
            else
              pax.crew_type=CrewTypes().decode(crewRemsStat.begin()->first);
          }
          else
          {
            if (grp_status==ASTRA::psCrew || pax.pers_type==ASTRA::baby)
              pax.is_jmp=false;
            else
              pax.is_jmp=(crewRemsStat.begin()->first=="JMP");
          }
        };

        CheckIn::TPaxRemItem rem=(pass==0?CalcCrewRem(grp_status, pax.crew_type):
                                          CalcJmpRem(grp_status, pax.is_jmp));
        if (crewRemsStat.begin()->first!=rem.code)
        {
          //удаляем неправильную ремарку
          for(multiset<CheckIn::TPaxRemItem>::iterator r=rems.begin(); r!=rems.end();)
          {
            if (r->code==crewRemsStat.begin()->first) r=Erase(rems, r); else ++r;
          }
          //добавляем правильную ремарку
          if (!rem.code.empty())
            rems.insert(CheckIn::TPaxRemItem(rem.code,rem.text));
        }
      };
    }
    else
    {
      //нет ремарок, связанных с экипажем
      CheckIn::TPaxRemItem rem=(pass==0?CalcCrewRem(grp_status, pax.crew_type):
                                        CalcJmpRem(grp_status, pax.is_jmp));
      if (!rem.code.empty())
        rems.insert(CheckIn::TPaxRemItem(rem.code,rem.text));
    };
  };
}

int TPaxListItem::getExistingPaxIdOrSwear() const
{
  if (pax.id!=ASTRA::NoExists) return pax.id;
  if (generatedPaxId) return generatedPaxId.get().get();
  throw EXCEPTIONS::Exception("%s: strange situation!", __FUNCTION__);
}

int TPaxList::getBagPoolMainPaxId(int bag_pool_num) const
{
  if (bag_pool_num==ASTRA::NoExists) return ASTRA::NoExists;
  boost::optional<TSimplePaxItem> pax;
  for(TPaxList::const_iterator i=begin(); i!=end(); ++i)
  {
    if (i->pax.bag_pool_num!=bag_pool_num) continue;
    if (!pax || i->pax.upward_within_bag_pool(pax.get())) pax=i->pax;
  };
  return pax?pax.get().id:ASTRA::NoExists;
}

const TSimplePaxGrpItem& TSimplePaxGrpItem::toXML(xmlNodePtr node) const
{
  if (node==nullptr) return *this;

  xmlNodePtr grpNode=node;
  NewTextChild(grpNode, "grp_id", id);
  NewTextChild(grpNode, "point_dep", point_dep);
  NewTextChild(grpNode, "airp_dep", airp_dep);
  NewTextChild(grpNode, "point_arv", point_arv);
  NewTextChild(grpNode, "airp_arv", airp_arv);
  NewTextChild(grpNode, "class", cl);
  NewTextChild(grpNode, "status", EncodePaxStatus(status));
  NewTextChild(grpNode, "bag_refuse", bag_refuse);
  if (TReqInfo::Instance()->client_type==ASTRA::ctTerm &&
      !TReqInfo::Instance()->desk.compatible(PAX_SERVICE_VERSION))
  {
    NewTextChild(grpNode, "bag_types_id", id);
    NewTextChild(grpNode, "piece_concept", (int)baggage_pc);
  };
  NewTextChild(grpNode, "tid", tid);
  return *this;
}

const TSimplePaxGrpItem& TSimplePaxGrpItem::toEmulXML(xmlNodePtr emulReqNode, xmlNodePtr emulSegNode) const
{
  if (emulReqNode!=nullptr)
  {
    NewTextChild(emulReqNode,"hall");
    NewTextChild(emulReqNode,"bag_refuse", bag_refuse);
  }

  toXML(emulSegNode);
  return *this;
}

const TPaxGrpItem& TPaxGrpItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;

  TSimplePaxGrpItem::toXML(node);

  xmlNodePtr grpNode=node;
  NewTextChild(grpNode, "show_ticket_norms", (int)true);
  NewTextChild(grpNode, "show_wt_norms", (int)wt);
  return *this;
};

bool TPaxGrpItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return true;
  xmlNodePtr node2=node->children;

  int grp_id=NodeAsIntegerFast("grp_id",node2,ASTRA::NoExists);
  if (grp_id!=ASTRA::NoExists)
  {
    //запись изменений
    if (!getByGrpIdWithBagConcepts(grp_id)) return false;
    if (tid!=NodeAsIntegerFast("tid",node2)) return false;
  };

  id=NodeAsIntegerFast("grp_id",node2,ASTRA::NoExists);
  point_dep=NodeAsIntegerFast("point_dep",node2);
  airp_dep=NodeAsStringFast("airp_dep",node2);
  point_arv=NodeAsIntegerFast("point_arv",node2);
  airp_arv=NodeAsStringFast("airp_arv",node2);
  cl=NodeAsStringFast("class",node2);
  if (id==ASTRA::NoExists)
    status=DecodePaxStatus(NodeAsStringFast("status",node2));

  if (trfer_confirm)
  {
    xmlNodePtr normNode=GetNodeFast("norms",node2);
    if (normNode!=NULL)
    {
      norms=boost::in_place();
      for(normNode=normNode->children; normNode!=NULL; normNode=normNode->next)
        norms.get().push_back(WeightConcept::TPaxNormItem().fromXML(normNode));
      for(WeightConcept::TPaxNormItem& n : norms.get())
        n.getListKeyUnaccomp(grp_id, 0, boost::none, "TPaxGrpItem::fromXML");
    }
  }
  return true;
};

TPaxGrpItem& TPaxGrpItem::fromXMLadditional(xmlNodePtr node, xmlNodePtr firstSegNode, bool is_unaccomp)
{
  hall=ASTRA::NoExists;
  bag_refuse.clear();

  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;

  TReqInfo *reqInfo=TReqInfo::Instance();

  if (tid!=ASTRA::NoExists)
  {
    //запись изменений
    bag_refuse=NodeAsStringFast("bag_refuse",node2);
  };

  //зал
  if (reqInfo->client_type == ASTRA::ctTerm)
  {
    hall=NodeAsIntegerFast("hall",node2);
  };

  if (wt) WeightConcept::PaidBagFromXML(node, id, is_unaccomp, trfer_confirm, paid);

  group_bag=TGroupBagItem();
  if (!group_bag.get().fromXML(node, id, hall, is_unaccomp, baggage_pc, trfer_confirm)) group_bag=boost::none;

  TGrpServiceListWithAuto list;
  if (!list.fromXML(node))
  {
    svc=boost::none;
    svc_auto=boost::none;
  }
  else
  {
    svc=TGrpServiceList();
    svc_auto=TGrpServiceAutoList();
    list.split(id, svc.get(), svc_auto.get());
  }

  ServicePaymentFromXML(firstSegNode, id, is_unaccomp, baggage_pc, payment);

  return *this;
}

const TPaxGrpItem& TPaxGrpItem::toDB(DB::TQuery &Qry) const
{
  id!=ASTRA::NoExists?Qry.SetVariable("grp_id", id):
                      Qry.SetVariable("grp_id", FNull);
  if (Qry.GetVariableIndex("point_dep")>=0)
    Qry.SetVariable("point_dep", point_dep);
  if (Qry.GetVariableIndex("point_arv")>=0)
    Qry.SetVariable("point_arv", point_arv);
  if (Qry.GetVariableIndex("airp_dep")>=0)
    Qry.SetVariable("airp_dep", airp_dep);
  if (Qry.GetVariableIndex("airp_arv")>=0)
    Qry.SetVariable("airp_arv", airp_arv);
  if (Qry.GetVariableIndex("class")>=0)
    Qry.SetVariable("class", cl);
  if (Qry.GetVariableIndex("status")>=0)
    Qry.SetVariable("status", EncodePaxStatus(status));
  if (Qry.GetVariableIndex("hall")>=0)
    hall!=ASTRA::NoExists?Qry.SetVariable("hall", hall):
                          Qry.SetVariable("hall", FNull);
  if (Qry.GetVariableIndex("bag_refuse")>=0)
    Qry.SetVariable("bag_refuse",(int)(!bag_refuse.empty()));
  if (Qry.GetVariableIndex("tid")>=0)
    Qry.SetVariable("tid", tid);
  return *this;
}

TSimplePnrItem& TSimplePnrItem::fromDB(DB::TQuery &Qry)
{
  clear();
  id=Qry.FieldAsInteger("pnr_id");
  airp_arv=Qry.FieldAsString("airp_arv");
  cl=Qry.FieldAsString("class");
  cabin_cl=Qry.FieldAsString("cabin_class");
  status=Qry.FieldAsString("status");
  if(Qry.GetFieldIndex("point_id") >= 0) {
    point_id_tlg=Qry.FieldAsInteger("point_id");
  }
  return *this;
}

TSimplePaxGrpItem& TSimplePaxGrpItem::fromDB(DB::TQuery &Qry)
{
  clear();
  id=Qry.FieldAsInteger("grp_id");
  point_dep=Qry.FieldAsInteger("point_dep");
  point_arv=Qry.FieldAsInteger("point_arv");
  airp_dep=Qry.FieldAsString("airp_dep");
  airp_arv=Qry.FieldAsString("airp_arv");
  cl=Qry.FieldAsString("class");
  status=DecodePaxStatus(Qry.FieldAsString("status").c_str());
  if (!Qry.FieldIsNULL("hall"))
    hall=Qry.FieldAsInteger("hall");
  if (Qry.FieldAsInteger("bag_refuse")!=0)
    bag_refuse=ASTRA::refuseAgentError;
  trfer_confirm=Qry.FieldAsInteger("trfer_confirm")!=0;
  is_mark_norms=Qry.FieldAsInteger("pr_mark_norms")!=0;
  if (Qry.GetFieldIndex("client_type")>=0)
    client_type = DecodeClientType(Qry.FieldAsString("client_type").c_str());
  tid=Qry.FieldAsInteger("tid");
  if (Qry.GetFieldIndex("excess_pc")>=0)
    excess_pc = Qry.FieldAsInteger("excess_pc");
  if (Qry.GetFieldIndex("excess_wt")>=0)
    excess_wt = Qry.FieldAsInteger("excess_wt");

  baggage_pc=Qry.FieldAsInteger("piece_concept")!=0;
  return *this;
}

bool TSimplePaxGrpItem::getByGrpId(int grp_id)
{
  clear();
  DB::TCachedQuery Qry(
        PgOra::getROSession("PAX_GRP"),
        "SELECT * FROM pax_grp "
        "WHERE grp_id=:grp_id",
        QParams() << QParam("grp_id", otInteger, grp_id),
        STDLOG);
  Qry.get().Execute();
  if (Qry.get().Eof) return false;
  fromDB(Qry.get());
  return true;
}

bool TSimplePaxGrpItem::getByPaxId(int pax_id)
{
  clear();
  DB::TCachedQuery Qry(
        PgOra::getROSession({"PAX_GRP","PAX"}),
        "SELECT pax_grp.* "
        "FROM pax_grp, pax "
        "WHERE pax_grp.grp_id=pax.grp_id AND pax.pax_id=:pax_id",
        QParams() << QParam("pax_id", otInteger, pax_id),
        STDLOG);
  Qry.get().Execute();
  if (Qry.get().Eof) return false;
  fromDB(Qry.get());
  return true;
}

std::list<TSimplePaxGrpItem> TSimplePaxGrpItem::getByDepPointId(const PointId_t& point_id)
{
  std::list<TSimplePaxGrpItem> result;
  DB::TCachedQuery Qry(
        PgOra::getROSession("PAX_GRP"),
        "SELECT * FROM pax_grp "
        "WHERE point_dep=:point_id ",
        QParams() << QParam("point_id", otInteger, point_id.get()),
        STDLOG);
  Qry.get().Execute();
  for(; !Qry.get().Eof; Qry.get().Next()) {
    TSimplePaxGrpItem item;
    item.fromDB(Qry.get());
    result.push_back(item);
  }
  return result;
}

ASTRA::TCompLayerType TSimplePaxGrpItem::getCheckInLayerType() const
{
  return DecodeCompLayerType(getBaseTable(etGrpStatusType).get_row("code", EncodePaxStatus(status)).AsString("layer_type").c_str());
}

bool TSimplePnrItem::getByPaxId(int pax_id)
{
  clear();
  DB::TCachedQuery Qry(
        PgOra::getROSession({"CRS_PNR", "CRS_PAX"}),
        "SELECT crs_pnr.pnr_id, "
        "       crs_pnr.airp_arv, " +
        TSimplePaxItem::origClassFromCrsSQL()+" AS class, " +
        TSimplePaxItem::cabinClassFromCrsSQL()+" AS cabin_class, "
        "       crs_pnr.status, "
        "       crs_pnr.point_id "
        "FROM crs_pnr, crs_pax "
        "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND crs_pax.pax_id=:pax_id",
        QParams() << QParam("pax_id", otInteger, pax_id),
        STDLOG);
  Qry.get().Execute();
  if (Qry.get().Eof) return false;
  fromDB(Qry.get());
  return true;
}

bool TSimplePnrItem::getByPnrId(const PnrId_t& pnr_id, const std::string& system)
{
  clear();
  QParams params;
  params << QParam("pnr_id", otInteger, pnr_id.get());
  if (!system.empty()) {
    params << QParam("system", otString, system);
  }
  DB::TCachedQuery Qry(
        PgOra::getROSession("CRS_PNR"),
        "SELECT pnr_id, airp_arv, class, cabin_class, status, point_id "
        "FROM crs_pnr "
        "WHERE pnr_id=:pnr_id "
        + (system.empty() ? std::string("") : "AND system=:system "),
        params, STDLOG);
  Qry.get().Execute();
  if (Qry.get().Eof) return false;
  fromDB(Qry.get());
  return true;
}

TPaxGrpItem& TPaxGrpItem::fromDBWithBagConcepts(DB::TQuery &Qry)
{
  clear();
  TSimplePaxGrpItem::fromDB(Qry);
  GetBagConcepts(id, pc, wt, rfisc_used);
  return *this;
}

bool TPaxGrpItem::getByGrpIdWithBagConcepts(int grp_id)
{
  clear();
  if (!TSimplePaxGrpItem::getByGrpId(grp_id)) return false;
  GetBagConcepts(id, pc, wt, rfisc_used);
  return true;
}

TPaxGrpCategory::Enum TSimplePaxGrpItem::grpCategory() const
{
  if (status==ASTRA::psCrew) {
    return TPaxGrpCategory::Crew;
  }
  if (cl.empty()) {
    return TPaxGrpCategory::UnnacompBag;
  }
  return TPaxGrpCategory::Passenges;
}

std::set<PaxId_t> loadInfIdSet(PaxId_t pax_id, bool lock)
{
  LogTrace(TRACE6) << __func__
                   << ": pax_id=" << pax_id;
  std::set<PaxId_t> result;
  int inf_id = ASTRA::NoExists;
  auto cur = make_db_curs(
        "SELECT inf_id "
        "FROM crs_inf "
        "WHERE pax_id=:pax_id "
        + std::string(lock ? "FOR UPDATE " : ""),
        lock ? PgOra::getRWSession("CRS_INF")
             : PgOra::getROSession("CRS_INF"));

  cur.stb()
      .def(inf_id)
      .bind(":pax_id", pax_id.get())
      .exec();

  while (!cur.fen()) {
    result.emplace(inf_id);
  }
  LogTrace(TRACE6) << __func__
                   << ": count=" << result.size();
  return result;
}

std::set<PaxId_t> loadSeatIdSet(PaxId_t pax_id, bool lock)
{
  LogTrace(TRACE6) << __func__
                   << ": pax_id=" << pax_id;
  std::set<PaxId_t> result;
  int seat_id = ASTRA::NoExists;
  auto cur = make_db_curs(
        "SELECT seat_id "
        "FROM crs_seats_blocking "
        "WHERE pax_id=:pax_id "
        "AND pr_del=0 "
        + std::string(lock ? "FOR UPDATE " : ""),
        lock ? PgOra::getRWSession("CRS_SEATS_BLOCKING")
             : PgOra::getROSession("CRS_SEATS_BLOCKING"));

  cur.stb()
      .def(seat_id)
      .bind(":pax_id", pax_id.get())
      .exec();

  while (!cur.fen()) {
    result.emplace(seat_id);
  }
  LogTrace(TRACE6) << __func__
                   << ": count=" << result.size();
  return result;
}

TGrpServiceAutoList loadCrsPaxAsvc(PaxId_t pax_id, const std::optional<TTripInfo>& flt = {})
{
  LogTrace(TRACE6) << __func__
                   << ": pax_id=" << pax_id;
  TGrpServiceAutoList result;
  TGrpServiceAutoItem item;
  item.clear();
  auto cur = make_db_curs(
        "SELECT "
        "rfic, rfisc, service_quantity, ssr_code, service_name, emd_type, emd_no, emd_coupon "
        "FROM crs_pax_asvc "
        "WHERE pax_id=:pax_id AND "
        "      rfic IS NOT NULL AND "
        "      rfisc IS NOT NULL AND "
        "      service_quantity IS NOT NULL AND "
        "      service_name IS NOT NULL AND "
        "      (rem_status='HI' AND "
        "       emd_type IS NOT NULL AND "
        "       emd_no IS NOT NULL AND "
        "       emd_coupon IS NOT NULL OR "
        "       rem_status='HK' AND "
        "       emd_type IS NULL AND "
        "       emd_no IS NULL AND "
        "       emd_coupon IS NULL) ",
        PgOra::getROSession("CRS_PAX_ASVC"));

  cur.stb()
      .def(item.RFIC)
      .def(item.RFISC)
      .def(item.service_quantity)
      .defNull(item.ssr_code, "")
      .def(item.service_name)
      .defNull(item.emd_type, "")
      .defNull(item.emd_no, "")
      .defNull(item.emd_coupon, ASTRA::NoExists)
      .bind(":pax_id", pax_id.get())
      .exec();

  while (!cur.fen()) {
    item.pax_id=pax_id.get();
    item.trfer_num=0;
    if (flt) {
      if (!item.isSuitableForAutoCheckin()) continue;
      if (!item.permittedForAutoCheckin(*flt)) continue;
    }
    result.push_back(item);
  }
  LogTrace(TRACE6) << __func__
                   << ": count=" << result.size();
  return result;
}

void TPaxGrpItem::SyncServiceAuto(const TTripInfo& flt)
{
  const std::set<PaxId_t> paxIdSet = loadPaxIdSet(GrpId_t(id));
  for (PaxId_t pax_id: paxIdSet) {
    const TGrpServiceAutoList asvcList = loadCrsPaxAsvc(pax_id, flt);
    if (asvcList.empty()) {
      continue;
    }
    if (!svc_auto) svc_auto=TGrpServiceAutoList();
    svc_auto.get().insert(svc_auto.get().begin(), asvcList.begin(), asvcList.end());
  }
}

void TPaxGrpItem::checkInfantsCount(const TSimplePaxList &prior_paxs,
                                    const TSimplePaxList &curr_paxs) const
{
  if (prior_paxs.empty() && !curr_paxs.empty())
  {
    //new_checkin
    if (curr_paxs.infantsMoreThanAdults())
    {
      LogTrace(TRACE0) << "curr_paxs.infantsMoreThanAdults()";
      throw UserException("MSG.CHECKIN.BABY_WO_SEATS_MORE_ADULT_FOR_GRP");
    }
    return;
  }

  bool needCheck=false;
  if (prior_paxs.size()!=curr_paxs.size())
    throw EXCEPTIONS::Exception("%s: strange situation: prior_paxs.size()!=curr_paxs.size()!", __FUNCTION__);

  TSimplePaxList::const_iterator iPrior=prior_paxs.begin();
  for(; iPrior!=prior_paxs.end(); ++iPrior)
  {
    TSimplePaxList::const_iterator iCurr=curr_paxs.begin();
    for(; iCurr!=curr_paxs.end(); ++iCurr)
      if (iCurr->id==iPrior->id) break;
    if (iCurr==curr_paxs.end()) break;
    if (iCurr->pers_type!=iPrior->pers_type ||
        iCurr->seats!=iPrior->seats ||
        iCurr->refuse!=iPrior->refuse) break;
  }
  if (iPrior!=prior_paxs.end()) needCheck=true;


  if (!needCheck) return;

  DB::TCachedQuery Qry(
        PgOra::getROSession("PAX"),
        "SELECT SUM(CASE WHEN pers_type=:adult THEN 1 ELSE 0 END) AS adults, "
        "       SUM(CASE WHEN pers_type=:infant AND seats=0 THEN 1 ELSE 0 END) AS infants "
        "FROM pax "
        "WHERE grp_id=:grp_id AND refuse IS NULL ",
        QParams() << QParam("grp_id", otInteger, id)
                  << QParam("adult", otString, EncodePerson(ASTRA::adult))
                  << QParam("infant", otString, EncodePerson(ASTRA::baby)),
        STDLOG);

  Qry.get().Execute();
  if (Qry.get().Eof ||
      Qry.get().FieldIsNULL("adults") ||
      Qry.get().FieldIsNULL("infants")) return;

  int adults=Qry.get().FieldAsInteger("adults");
  int infants=Qry.get().FieldAsInteger("infants");

  if (adults < infants)
  {
    LogTrace(TRACE0) << "adults=" << adults << " infants=" << infants;
    throw UserException("MSG.CHECKIN.BABY_WO_SEATS_MORE_ADULT_FOR_GRP");
  }
}

void TPaxGrpItem::UpdTid(int grp_id)
{
  DB::TCachedQuery Qry(
        PgOra::getRWSession("PAX_GRP"),
        "UPDATE pax_grp SET tid=:tid "
        "WHERE grp_id=:grp_id",
        QParams() << QParam("grp_id", otInteger, grp_id)
                  << QParam("tid", otInteger, PgOra::getSeqNextVal_int("CYCLE_TID__SEQ")),
        STDLOG);
  Qry.get().Execute();
}

void TPaxGrpItem::setRollbackGuaranteedTo(int grp_id, bool value)
{
  DB::TCachedQuery Qry(
        PgOra::getRWSession("PAX_GRP"),
        "UPDATE pax_grp SET rollback_guaranteed=:value "
        "WHERE grp_id=:grp_id",
        QParams() << QParam("grp_id", otInteger, grp_id)
                  << QParam("value", otInteger, (int)value),
        STDLOG);
  Qry.get().Execute();
}

bool TPaxGrpItem::allPassengersRefused(int grp_id)
{
  DB::TCachedQuery Qry(
        PgOra::getROSession("PAX"),
        "SELECT 1 FROM pax "
        "WHERE grp_id=:grp_id AND refuse IS NULL "
        "FETCH FIRST 1 ROWS ONLY",
        QParams() << QParam("grp_id", otInteger, grp_id),
        STDLOG);
  Qry.get().Execute();
  return Qry.get().Eof;
}

template<class T>
map<SegNo_t, T> getTCkinInfo(const PaxId_t& paxId)
{
  map<SegNo_t, T> result;

  DB::TQuery Qry(PgOra::getROSession({"PAX", "TCKIN_PAX_GRP"}), STDLOG);
  Qry.SQLText=
    "SELECT pax.*, tckin_pax_grp.seg_no "
    "FROM pax, tckin_pax_grp, "
    "     (SELECT tckin_pax_grp.tckin_id, "
    "             tckin_pax_grp.first_reg_no-pax.reg_no AS distance "
    "      FROM pax, tckin_pax_grp "
    "      WHERE pax.grp_id=tckin_pax_grp.grp_id AND pax.pax_id=:pax_id AND tckin_pax_grp.transit_num=0) p "
    "WHERE tckin_pax_grp.tckin_id=p.tckin_id AND "
    "      pax.grp_id=tckin_pax_grp.grp_id AND "
    "      tckin_pax_grp.first_reg_no-pax.reg_no=p.distance AND tckin_pax_grp.transit_num=0";
  Qry.CreateVariable("pax_id", otInteger, paxId.get());
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
    result.emplace(SegNo_t(Qry.FieldAsInteger("seg_no")), T(Qry));

  return result;
}

map<SegNo_t, TSimplePaxItem> GetTCkinPassengers(const PaxId_t& paxId)
{
  return getTCkinInfo<TSimplePaxItem>(paxId);
}

map<SegNo_t, TCkinPaxTknItem> GetTCkinTickets(const PaxId_t& paxId)
{
  return getTCkinInfo<TCkinPaxTknItem>(paxId);
}

// Ф-я возвращает непрерывный список сегментов до или после pax_id, в зав-ти от after_current
// Если встречается номер сегмента, не идущий подряд, он и следующие за ним элементы отсекаются.
// Напр. если список такой 1, 2, 4, 5, 7, 8
// То в случае after_current = true вернется 7, 8 (непрерывность с конца)
// При after_current = false вернется 1, 2 (с начала)
map<SegNo_t, TCkinPaxTknItem> GetTCkinTickets(const PaxId_t& paxId, bool after_current)
{
    map<SegNo_t, TCkinPaxTknItem> result;
    map<SegNo_t, TCkinPaxTknItem> tkns_=GetTCkinTickets(paxId);

    // Находим в мепе элемент с тек. pax_id
    auto current = tkns_.begin();
    for(; current != tkns_.end(); current++) {
        if(current->second.paxId() == paxId) break;
    }

    if(current != tkns_.end()) {
        auto curr_idx = current->first.get();
        // в зав-ти от after_current в result помещаем часть слева или справа от pax_id
        if(after_current) {
            current++;
            result.insert(current, tkns_.end());
        } else {
            result.insert(tkns_.begin(), current);
        }

        // а здесь происходит удаление элементов, seg_no которых не подряд
        // Напр.:
        // (1,2,3,5,6,10) -> (1,2,3)
        if(after_current) {
            auto ri = result.begin();
            curr_idx++;
            for(; ri != result.end(); ri++, curr_idx++) {
                if(ri->first.get() != curr_idx) break;
            }
            if(ri != result.end())
                result.erase(ri, result.end());
        } else {
            auto ri = result.rbegin();
            curr_idx--;
            for(; ri != result.rend(); ri++, curr_idx--) {
                if(ri->first.get() != curr_idx) break;
            }
            if(ri != result.rend()) {
                auto i = ri.base();
                result.erase(result.begin(), i);
            }
        }
    }
    return result;
}

map<SegNo_t, TCkinPaxTknItem> GetTCkinTicketsBefore(const PaxId_t& paxId)
{
    return GetTCkinTickets(paxId, false);
}

map<SegNo_t, TCkinPaxTknItem> GetTCkinTicketsAfter(const PaxId_t& paxId)
{
    return GetTCkinTickets(paxId, true);
}

std::string isFemaleStr( int is_female )
{
  switch (is_female) {
    case ASTRA::NoExists:
      return "";
      break;
    case 0:
      return "M";
      break;
    default:
      return "F";
      break;
  };
}

bool TPaxTknItem::validForSearch() const
{
  return !no.empty();
}

void TPaxTknItem::addSQLTablesForSearch(const PaxOrigin& origin, std::set<std::string>& tables) const
{
  switch(origin)
  {
    case paxCheckIn:
      break;
    case paxPnl:
      break;
    case paxTest:
      break;
  }
}

void TPaxTknItem::addSQLConditionsForSearch(const PaxOrigin& origin, std::list<std::string>& conditions) const
{
  switch(origin)
  {
    case paxCheckIn:
      if (no.size()==14)
        conditions.push_back("pax.ticket_no IN (:tkn_no, :tkn_no_13)");
      else
        conditions.push_back("pax.ticket_no=:tkn_no");
      if (coupon!=ASTRA::NoExists)
        conditions.push_back("pax.coupon_no=:tkn_coupon");
      break;
    case paxPnl:
      conditions.push_back("crs_pax.pax_id=:pax_id");
      break;
    case paxTest:
      if (no.size()==14)
        conditions.push_back("test_pax.tkn_no IN (:tkn_no, :tkn_no_13)");
      else
        conditions.push_back("test_pax.tkn_no=:tkn_no");
      break;
  }
}

void TPaxTknItem::addSQLParamsForSearch(const PaxOrigin& origin, QParams& params) const
{
  if (origin == paxPnl) {
    return;
  }
  params << QParam("tkn_no", otString, no);
  if (no.size()==14)
    params << QParam("tkn_no_13", otString, no.substr(0,13));
  if (coupon!=ASTRA::NoExists)
    params << QParam("tkn_coupon", otInteger, coupon);
}

std::set<PaxId_t> loadCrsPaxTKN_ext(const std::string& tick_no, int coupon_no)
{
  LogTrace(TRACE6) << __func__
                   << ": tick_no=" << tick_no
                   << ", coupon_no=" << coupon_no;
  std::set<PaxId_t> result;
  DB::TQuery Qry(PgOra::getROSession("CRS_PAX_TKN"), STDLOG);
  Qry.SQLText="SELECT pax_id "
              "FROM crs_pax_tkn "
              "WHERE ticket_no IN (:ticket_no, :ticket_no_13) "
              + std::string(coupon_no != ASTRA::NoExists
              ? "AND coupon_no=:coupon_no "
              : "");
  Qry.CreateVariable("ticket_no", otString, tick_no);
  Qry.CreateVariable("ticket_no_13", otString, tick_no.size() == 14 ? tick_no.substr(0,13)
                                                                    : tick_no);
  if (coupon_no != ASTRA::NoExists) {
    Qry.CreateVariable("coupon_no", otInteger, coupon_no);
  }
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next()) {
    result.emplace(Qry.FieldAsInteger("pax_id"));
  }
  return result;
}

void TPaxTknItem::addSearchPaxIds(const PaxOrigin& origin, std::set<PaxId_t>& searchPaxIds) const
{
  searchPaxIds = loadCrsPaxTKN_ext(no, coupon);
}

bool TPaxTknItem::useSearchPaxIds(const PaxOrigin& origin) const
{
  return origin == paxPnl;
}

bool TPaxTknItem::suitable(const TPaxTknItem& tkn) const
{
  return (no.empty() || (no.size()==14?no.substr(0,13):no) == tkn.no) &&
         (coupon==ASTRA::NoExists || coupon==tkn.coupon);
}

bool TPaxDocItem::validForSearch() const
{
  return !no.empty();
}

void TPaxDocItem::addSQLTablesForSearch(const PaxOrigin& origin, std::set<std::string>& tables) const
{
  switch(origin)
  {
    case paxCheckIn:
      tables.insert("pax_doc");
      break;
    case paxPnl:
      break;
    case paxTest:
      break;
  }
}

void TPaxDocItem::addSQLConditionsForSearch(const PaxOrigin& origin, std::list<std::string>& conditions) const
{
  switch(origin)
  {
    case paxCheckIn:
      conditions.push_back("pax.pax_id=pax_doc.pax_id");
      conditions.push_back("pax_doc.no=:doc_no");
      if (birth_date!=ASTRA::NoExists)
        conditions.push_back("(pax_doc.birth_date=:doc_birth_date OR pax_doc.birth_date IS NULL)");
      break;
    case paxPnl:
      conditions.push_back("crs_pax.pax_id=:pax_id");
      break;
    case paxTest:
      conditions.push_back("test_pax.doc_no=:doc_no");
      break;
  }
}

void TPaxDocItem::addSQLParamsForSearch(const PaxOrigin& origin, QParams& params) const
{
  if (origin == paxPnl) {
    return;
  }
  params << QParam("doc_no", otString, no);
  if (birth_date!=ASTRA::NoExists)
    params << QParam("doc_birth_date", otDate, birth_date);
}

std::set<PaxId_t> loadPaxIdSetByDocs(const std::string& doc_no, TDateTime birth_date)
{
  LogTrace(TRACE6) << __func__
                   << ": doc_no=" << doc_no
                   << ", birth_date=" << birth_date;
  std::set<PaxId_t> result;
  int pax_id = 0;
  auto cur = make_db_curs(
        "SELECT pax_id "
        "FROM crs_pax_doc "
        "WHERE no=:doc_no "
        + std::string(
          birth_date!=ASTRA::NoExists
          ? "AND (crs_pax_doc.birth_date=:doc_birth_date "
            "OR crs_pax_doc.birth_date IS NULL) "
          : ""),
        PgOra::getROSession("CRS_PAX_DOC"));
  cur.stb()
      .def(pax_id)
      .bind(":doc_no", doc_no);
  if (birth_date!=ASTRA::NoExists) {
    cur.bind(":doc_birth_date", DateTimeToBoost(birth_date));
  }
  cur.exec();

  while (!cur.fen()) {
    result.emplace(pax_id);
  }
  LogTrace(TRACE6) << __func__
                   << ": count=" << result.size();
  return result;
}

void TPaxDocItem::addSearchPaxIds(const PaxOrigin& origin, std::set<PaxId_t>& searchPaxIds) const
{
  searchPaxIds = loadPaxIdSetByDocs(no, birth_date);
}

bool TPaxDocItem::useSearchPaxIds(const PaxOrigin& origin) const
{
  return origin == paxPnl;
}

bool TPaxDocItem::suitable(const TPaxDocItem& doc) const
{
  return (no.empty() || no==doc.no) &&
         (birth_date==ASTRA::NoExists || doc.birth_date==ASTRA::NoExists || birth_date==doc.birth_date);
}

std::string TScannedPaxDocItem::getTrueNo() const
{
  if (isNationalRussianPassport() && no.size()==9 && !extra.empty())
    return no.substr(0,3)+extra.substr(0,1)+no.substr(3);

  return no;
}

void TScannedPaxDocItem::addSQLParamsForSearch(const PaxOrigin& origin, QParams& params) const
{
  if (origin == paxPnl) {
    return;
  }
  LogTrace(TRACE5) << __FUNCTION__ << ": doc_no=" << getTrueNo();
  params << QParam("doc_no", otString, getTrueNo());
  if (birth_date!=ASTRA::NoExists)
  {
    LogTrace(TRACE5) << __FUNCTION__ << ": doc_birth_date=" << DateTimeToStr(birth_date);
    params << QParam("doc_birth_date", otDate, birth_date);
  }
}

void TScannedPaxDocItem::addSearchPaxIds(const PaxOrigin& origin, std::set<PaxId_t>& searchPaxIds) const
{
  searchPaxIds = loadPaxIdSetByDocs(getTrueNo(), birth_date);
}

bool TScannedPaxDocItem::useSearchPaxIds(const PaxOrigin& origin) const
{
  return origin == paxPnl;
}

bool TScannedPaxDocItem::suitable(const TPaxDocItem& doc) const
{
  return (no.empty() || getTrueNo()==doc.no) &&
         (birth_date==ASTRA::NoExists || doc.birth_date==ASTRA::NoExists || birth_date==doc.birth_date);
}

bool TSimplePaxList::infantsMoreThanAdults() const
{
  return CheckIn::infantsMoreThanAdults(*this);
}

}; //namespace CheckIn

namespace Sirena
{

void PaxBrandsNormsToStream(const TTrferRoute &trfer, const CheckIn::TPaxItem &pax, ostringstream &s)
{
  s << "#" << setw(3) << setfill('0') << pax.reg_no << " "
    << pax.full_name() << "(" << ElemIdToCodeNative(etPersType, EncodePerson(pax.pers_type)) << "):" << endl;

  TPaxNormList norms;
  PaxNormsFromDB(PaxId_t(pax.id), norms);
  TPaxBrandList brands;
  PaxBrandsFromDB(PaxId_t(pax.id), brands);

  for(int pass=0; pass<2; pass++)
  {
    string prior_text;
    int trfer_num=0;
    for(TTrferRoute::const_iterator t=trfer.begin(); ; ++t, trfer_num++)
    {
      //ищем норму, соответствующую сегменту
      string curr_text;
      if (t!=trfer.end())
      {
        if (pass==0)
        {
          //бренды

          for(TPaxBrandList::const_iterator b=brands.begin(); b!=brands.end(); ++b)
          {
            if (b->first.trfer_num!=trfer_num) continue;

            const TSimplePaxBrandItem &brand=b->second;

            TSimplePaxBrandItem::const_iterator i=brand.find(TReqInfo::Instance()->desk.lang);
            if (i==brand.end() && !brand.empty()) i=brand.begin(); //первая попавшаяся

            if (i!=brand.end()) curr_text=i->second.text;
            break;
          }
        }
        else
        {
          boost::optional<bool> carry_on=false;
          carry_on=boost::none;
          //нормы
          for(TPaxNormList::const_iterator n=norms.begin(); n!=norms.end(); ++n)
          {
            if (n->first.trfer_num!=trfer_num) continue;

            const TSimplePaxNormItem &norm=n->second;

            TSimplePaxNormItem::const_iterator i=norm.find(TReqInfo::Instance()->desk.lang);
            if (i==norm.end() && !norm.empty()) i=norm.begin(); //первая попавшаяся

            if (i!=norm.end())
            {
              if (!curr_text.empty()) curr_text+='\n';

              if (!carry_on || carry_on.get()!=norm.carry_on)
              {
                curr_text+=getLocaleText(norm.carry_on?"MSG.CARRY_ON_NORM_OF_THE_AIRLINE_APPLIES":
                                                       "MSG.BAGGAGE_NORM_OF_THE_AIRLINE_APPLIES",
                                         LParams() << LParam("airline", ElemIdToCodeNative(etAirline, norm.airline)));
                curr_text+='\n';
                carry_on=norm.carry_on;
              };

              curr_text+=i->second.text;
            };
          };
        };
      };

      if (t!=trfer.begin())
      {
        if (t==trfer.end() || curr_text!=prior_text)
        {
          if (pass==1 || !prior_text.empty())
          {
            s << ":" << endl //закончили секцию сегментов
              << (prior_text.empty()?getLocaleText(pass==0?"MSG.UNKNOWN_BRAND":
                                                           "MSG.LUGGAGE.UNKNOWN_BAG_NORM"):prior_text) << endl;  //записали соответствующую норму
          };
        }
        else
        {
          if (pass==1 || !prior_text.empty()) s << " -> ";
        }
      };

      if (t==trfer.end()) break;

      if (t==trfer.begin() || curr_text!=prior_text)
      {
        if (pass==1 || t==trfer.begin() || !prior_text.empty()) s << endl;
      };

      if (pass==1 || !curr_text.empty())
      {
        const TTrferRouteItem &item=*t;
        s << ElemIdToCodeNative(etAirline, item.operFlt.airline)
          << setw(2) << setfill('0') << item.operFlt.flt_no
          << ElemIdToCodeNative(etSuffix, item.operFlt.suffix)
          << " " << ElemIdToCodeNative(etAirp, item.operFlt.airp);
      };

      prior_text=curr_text;
    }
  }

  s << string(100,'=') << endl; //подведем черту :)
}

} //namespace Sirena

const char rus_pnr[]="0123456789БВГДКЛМНПРСТФХЦЖШ";
const char lat_pnr[]="0123456789BVGDKLMNPRSTFXCZW";

static char toLatPnrCharStrictly(char c)
{
  if (!IsAscii7(c))
  {
    ByteReplace(&c,1,rus_pnr,lat_pnr);
    if (!IsAscii7(c)) c='?';
  };
  return c;
}

static char toLatPnrChar(char c)
{
  ByteReplace(&c,1,rus_pnr,lat_pnr);
  return c;
}

static char toRusPnrChar(char c)
{
  ByteReplace(&c,1,lat_pnr,rus_pnr);
  return c;
}

string convert_pnr_addr(const string &value, bool pr_lat)
{
  string result = value;
  if (pr_lat)
    transform(result.begin(), result.end(), result.begin(), toLatPnrCharStrictly);
  return result;

}

const TPnrAddrInfo& TPnrAddrInfo::toXML(xmlNodePtr addrParentNode,
                                        const boost::optional<AstraLocale::OutputLang>& lang) const
{
  if (addrParentNode==nullptr) return *this;

  xmlNodePtr addrNode=NewTextChild(addrParentNode, "pnr_addr");
  NewTextChild(addrNode, "airline", lang?airlineToPrefferedCode(airline, lang.get()):airline);
  NewTextChild(addrNode, "addr", lang?convert_pnr_addr(addr, lang->isLatin()):addr);

  return *this;
}

bool TPnrAddrInfo::validForSearch() const
{
  return !addr.empty();
}

void TPnrAddrInfo::addSQLTablesForSearch(const PaxOrigin& origin, std::set<std::string>& tables) const
{
}

void TPnrAddrInfo::addSQLConditionsForSearch(const PaxOrigin& origin, std::list<std::string>& conditions) const
{
  switch(origin)
  {
    case paxCheckIn:
      conditions.push_back("pax.pax_id=:pax_id");
      break;
    case paxPnl:
      conditions.push_back("crs_pax.pax_id=:pax_id");
      break;
    case paxTest:
      conditions.push_back("test_pax.pnr_addr IN (:addr_rus, :addr_lat)");
      break;
  }
}

void TPnrAddrInfo::addSQLParamsForSearch(const PaxOrigin& origin, QParams& params) const
{
  if (origin == paxCheckIn || origin == paxPnl) {
    return;
  }
  std::string addr_rus(addr), addr_lat(addr);
  transform(addr_rus.begin(), addr_rus.end(), addr_rus.begin(), toRusPnrChar);
  transform(addr_lat.begin(), addr_lat.end(), addr_lat.begin(), toLatPnrChar);
  params << QParam("addr_rus", otString, addr_rus)
         << QParam("addr_lat", otString, addr_lat);
}

std::set<PnrId_t> loadPnrIdSetByPnrAddrs(const std::string& addr)
{
  LogTrace(TRACE6) << __func__
                   << ": addr=" << addr;
  std::string addr_rus(addr), addr_lat(addr);
  transform(addr_rus.begin(), addr_rus.end(), addr_rus.begin(), toRusPnrChar);
  transform(addr_lat.begin(), addr_lat.end(), addr_lat.begin(), toLatPnrChar);
  std::set<PnrId_t> result;
  DB::TQuery Qry(PgOra::getROSession("pnr_addrs"), STDLOG);
  Qry.SQLText="SELECT pnr_id "
              "FROM pnr_addrs "
              "WHERE addr IN (:addr_rus, :addr_lat) ";
  Qry.CreateVariable("addr_rus", otString, addr_rus);
  Qry.CreateVariable("addr_lat", otString, addr_lat);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next()) {
    result.emplace(Qry.FieldAsInteger("pnr_id"));
  }
  return result;
}

std::set<PaxId_t> loadCrsPnrPaxIdSet(const PnrId_t& pnr_id)
{
  std::set<PaxId_t> result;
  DB::TQuery Qry(PgOra::getROSession({"CRS_PAX","CRS_PNR"}), STDLOG);
  Qry.SQLText="SELECT pax_id "
              "FROM crs_pax, crs_pnr "
              "WHERE crs_pax.pnr_id=crs_pnr.pnr_id "
              "AND crs_pnr.system='CRS' "
              "AND crs_pax.pr_del=0 "
              "AND crs_pax.pnr_id=:pnr_id ";
  Qry.CreateVariable("pnr_id", otInteger, pnr_id.get());
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next()) {
    result.emplace(Qry.FieldAsInteger("pax_id"));
  }
  return result;
}

std::set<PaxId_t> loadCrsPnrPaxIdSetByPnrAddrs(const std::string& addr)
{
  std::set<PaxId_t> result;
  const std::set<PnrId_t> pnr_id_set = loadPnrIdSetByPnrAddrs(addr);
  for (const PnrId_t& pnr_id: pnr_id_set) {
    const std::set<PaxId_t> pax_id_set = loadCrsPnrPaxIdSet(pnr_id);
    result.insert(pax_id_set.begin(), pax_id_set.end());
  }
  return result;
}

void TPnrAddrInfo::addSearchPaxIds(const PaxOrigin& origin, std::set<PaxId_t>& searchPaxIds) const
{
  if (origin == paxCheckIn || origin == paxPnl) {
    searchPaxIds = loadCrsPnrPaxIdSetByPnrAddrs(addr);
    return;
  }
}

bool TPnrAddrInfo::useSearchPaxIds(const PaxOrigin& origin) const
{
  return origin == paxCheckIn || origin == paxPnl;
}

bool TPnrAddrInfo::suitable(const TPnrAddrInfo& pnr) const
{
  return addr.empty() || convert_pnr_addr(addr, true)==convert_pnr_addr(pnr.addr, true);
}

bool TPnrAddrInfo::suitable(const TPnrAddrs& pnrs) const
{
  if (addr.empty()) return true;
  for(const TPnrAddrInfo& pnr : pnrs)
    if (suitable(pnr)) return true;
  return false;
}

const TPnrAddrs& TPnrAddrs::toXML(xmlNodePtr addrsParentNode) const
{
  if (addrsParentNode==nullptr) return *this;

  if (!empty())
  {
    if (size()==1)
      front().toXML(addrsParentNode, boost::none);
    else
    {
      xmlNodePtr addrsNode=NewTextChild(addrsParentNode, "pnr_addrs");
      for(const TPnrAddrInfo& addr : *this) addr.toXML(addrsNode, boost::none);
    }
  }

  return *this;
}

const TPnrAddrs& TPnrAddrs::toWebXML(xmlNodePtr addrsParentNode,
                                     const boost::optional<AstraLocale::OutputLang>& lang) const
{
  if (addrsParentNode==nullptr) return *this;

  xmlNodePtr addrsNode=NewTextChild(addrsParentNode, "pnr_addrs");
  for(const TPnrAddrInfo& addr : *this) addr.toXML(addrsNode, lang);

  return *this;
}
const TPnrAddrs& TPnrAddrs::toSirenaXML(xmlNodePtr addrParentNode,
                                        const AstraLocale::OutputLang& lang) const
{
  if (addrParentNode==nullptr) return *this;

  for(const TPnrAddrInfo& addr : *this)
    SetProp(NewTextChild(addrParentNode, "recloc", addr.addr), "crs", airlineToPrefferedCode(addr.airline, lang));

  return *this;
}

TPnrAddrs& TPnrAddrs::getByPnrIdFast(int pnr_id)
{
  string airline("No matter");
  getByPnrId(pnr_id, airline);
  return *this;
}

std::optional<AirlineCode_t> getTlgAirline(const PnrId_t& pnr_id)
{
  LogTrace(TRACE6) << __func__ << ": pnr_id=" << pnr_id;
  DB::TQuery Qry(PgOra::getROSession("CRS_PNR"), STDLOG);
  Qry.SQLText =
      "SELECT point_id "
      "FROM crs_pnr "
      "WHERE pnr_id=:pnr_id ";
  Qry.CreateVariable("pnr_id", otInteger, pnr_id.get());
  Qry.Execute();

  if (Qry.Eof) return {};

  PointIdTlg_t pointIdTlg(Qry.FieldAsInteger("point_id"));

  const std::optional<TlgTripsData> tlg_trips_data = TlgTripsData::loadFromPnl(pointIdTlg);

  if (!tlg_trips_data) return {};

  return tlg_trips_data->airline;
}

std::string TPnrAddrs::getByPnrId(int pnr_id, string &airline)
{
  LogTrace(TRACE6) << __func__
                   << ": pnr_id=" << pnr_id
                   << ", airline=" << airline;
  clear();

  if (airline.empty())
  {
    const std::optional<AirlineCode_t> tlgAirline=getTlgAirline(PnrId_t(pnr_id));
    if (!tlgAirline) return "";
    airline=tlgAirline.value().get();
  }

  QParams QryParams;
  QryParams << QParam("pnr_id", otInteger, pnr_id);
  QryParams << QParam("airline", otString, airline);

  DB::TCachedQuery CachedQry(
        PgOra::getROSession("PNR_ADDRS"),
        "SELECT airline, addr "
        "FROM pnr_addrs "
        "WHERE pnr_id=:pnr_id "
        "ORDER BY CASE WHEN airline=:airline THEN 0 ELSE 1 END, "
        "         airline",
        QryParams, STDLOG);
  DB::TQuery &Qry=CachedQry.get();
  Qry.Execute();
  for (;!Qry.Eof;Qry.Next()) {
    emplace_back(Qry.FieldAsString("airline"),
                 Qry.FieldAsString("addr"));
  }

  if (!empty() && begin()->airline==airline)
    return begin()->addr;
  else
    return "";
}

TPnrAddrs& TPnrAddrs::getByPaxIdFast(int pax_id)
{
  string airline("No matter");
  getByPaxId(pax_id, airline);
  return *this;
}

std::string TPnrAddrs::getByPaxId(int pax_id, string &airline)
{
  clear();
  QParams QryParams;
  QryParams << QParam("pax_id", otInteger, pax_id);

  DB::TCachedQuery Qry(
        PgOra::getROSession("CRS_PAX"),
        "SELECT pnr_id FROM crs_pax WHERE pax_id=:pax_id AND pr_del=0",
        QryParams, STDLOG);
  Qry.get().Execute();
  if (!Qry.get().Eof) {
    return getByPnrId(Qry.get().FieldAsInteger("pnr_id"), airline);
  } else {
    return "";
  }
}

namespace ASTRA
{

template<> const CheckIn::TSimplePaxGrpItem& PaxGrpCache::add(const GrpId_t& grpId) const
{
  CheckIn::TSimplePaxGrpItem grp;
  if (!grp.getByGrpId(grpId.get())) throw NotFound();

  return items.emplace(grpId, grp).first->second;
}

template<> std::string PaxGrpCache::traceTitle()
{
  return "PaxGrpCache";
}

} //namespace ASTRA


void updatePaxChange(const PointId_t& pointDep,
                     const GrpId_t& grpId,
                     const TermWorkingMode::Enum workMode)
{
  const auto paxList=CheckIn::TSimplePaxItem::getByGrpId(grpId);

  for(const CheckIn::TSimplePaxItem& pax : paxList)
    updatePaxChange(pointDep, PaxId_t(pax.id), RegNo_t(pax.reg_no), workMode);
}
