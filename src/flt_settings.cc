#include "flt_settings.h"
#include "exceptions.h"
#include "qrys.h"
#include "serverlib/cursctl.h"

#define NICKNAME "VLAD"
#include "serverlib/slogger.h"

using namespace EXCEPTIONS;
using namespace ASTRA;
using namespace std;

bool DefaultTripSets( const TTripSetType setType )
{
  return setType==tsOverloadReg ||
         setType==tsAPISControl;
}

bool GetTripSets( const TTripSetType setType,
                  const TTripInfo &info )
{
  if (!(setType>=0 && setType<100))
    throw Exception("%s: wrong setType=%d", __FUNCTION__, (int)setType);

  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText=
    "SELECT pr_misc, "
    "    DECODE(airline,NULL,0,8)+ "
    "    DECODE(flt_no,NULL,0,2)+ "
    "    DECODE(airp_dep,NULL,0,4) AS priority "
    "FROM misc_set "
    "WHERE type=:type AND "
    "      (airline IS NULL OR airline=:airline) AND "
    "      (flt_no IS NULL OR flt_no=:flt_no) AND "
    "      (airp_dep IS NULL OR airp_dep=:airp_dep) "
    "ORDER BY priority DESC";
  Qry.CreateVariable("type",otInteger,(int)setType);
  Qry.CreateVariable("airline",otString,info.airline);
  Qry.CreateVariable("flt_no",otInteger,info.flt_no);
  Qry.CreateVariable("airp_dep",otString,info.airp);
  Qry.Execute();
  if (Qry.Eof) return DefaultTripSets(setType);
  return Qry.FieldAsInteger("pr_misc")!=0;
}

bool GetSelfCkinSets( const TTripSetType setType,
                      const int point_id,
                      const ASTRA::TClientType client_type )
{
  if (point_id==ASTRA::NoExists)
    throw Exception("%s: wrong point_id=NoExists", __FUNCTION__);
  TTripInfo info;
  if (!info.getByPointId(point_id)) return DefaultTripSets(setType);
  return GetSelfCkinSets(setType, info, client_type);
}

bool GetSelfCkinSets(const TTripSetType setType,
                     const TTripInfo &info,
                     const ASTRA::TClientType client_type )
{
  if (!(setType>=200 && setType<300))
    throw Exception("%s: wrong setType=%d", __FUNCTION__, (int)setType);
  if (!(client_type==ctWeb ||
        client_type==ctKiosk ||
        client_type==ctMobile))
    throw Exception("%s: wrong client_type=%s (setType=%d)", __FUNCTION__, EncodeClientType(client_type), (int)setType);
  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText=
    "SELECT value, "
    "    DECODE(client_type,NULL,0,1)+ "
    "    DECODE(airline,NULL,0,8)+ "
    "    DECODE(flt_no,NULL,0,2)+ "
    "    DECODE(airp_dep,NULL,0,4) AS priority "
    "FROM self_ckin_set "
    "WHERE type=:type AND "
    "      (airline IS NULL OR airline=:airline) AND "
    "      (flt_no IS NULL OR flt_no=:flt_no) AND "
    "      (airp_dep IS NULL OR airp_dep=:airp_dep) AND "
    "      (client_type IS NULL OR client_type=:client_type) "
    "ORDER BY priority DESC";
  Qry.CreateVariable("type",otInteger,(int)setType);
  Qry.CreateVariable("airline",otString,info.airline);
  Qry.CreateVariable("flt_no",otInteger,info.flt_no);
  Qry.CreateVariable("airp_dep",otString,info.airp);
  Qry.CreateVariable("client_type",otString,EncodeClientType(client_type));
  Qry.Execute();
  if (Qry.Eof)
  {
    switch(setType)
    {
      default: return false;
    };
  };
  return Qry.FieldAsInteger("value")!=0;
}

bool TTripSetListItemLess(const std::pair<TTripSetType, boost::any> &a, const std::pair<TTripSetType, boost::any> &b)
{
  if (a.first!=b.first)
    return a.first<b.first;
  if (a.second.type().name()!=b.second.type().name())
    return a.second.type().name()<b.second.type().name();
  try
  {
    return boost::any_cast<bool>(a.second)<boost::any_cast<bool>(b.second);
  }
  catch(const boost::bad_any_cast&) {};
  try
  {
    return boost::any_cast<int>(a.second)<boost::any_cast<int>(b.second);
  }
  catch(const boost::bad_any_cast&) {};
  return false;
}

std::string TTripSetList::toString(const TTripSetType settingType) const
{
  auto i=_settingTypes.find(settingType);
  if (i==_settingTypes.end())
    throw Exception("%s: wrong settingType=%d", __FUNCTION__, (int)settingType);

  return i->second;
}

const TTripSetList& TTripSetList::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  for(const auto& i : *this)
  {
    try
    {
      if (isBool(i.first))
        NewTextChild(node, toString(i.first).c_str(), (int)boost::any_cast<bool>(i.second));
      else if (isInt(i.first))
        NewTextChild(node, toString(i.first).c_str(), (int)boost::any_cast<int>(i.second));
    }
    catch(const boost::bad_any_cast&)
    {
      throwBadCastException(i.first, __FUNCTION__);
    };
  };
  return *this;
}

TTripSetList& TTripSetList::fromXML(xmlNodePtr node, const bool pr_tranzit, const TTripSetList& priorSettings)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  for(const auto& i : _settingTypes)
  {
    const TTripSetType& setting=i.first;
    xmlNodePtr n=GetNodeFast(i.second.c_str(), node2);
    if (n!=NULL)
    {
      if (isBool(setting))
        emplace(setting, NodeAsInteger(n)!=0);
      else if (isInt(setting))
        emplace(setting, NodeAsInteger(n));
    };
  }

  append(priorSettings);

  checkAndCorrectTransitSets(pr_tranzit);

  return *this;
}

const TTripSetList& TTripSetList::initDB(int point_id, int f, int c, int y) const
{
  TQuery Qry(&OraSession);

  ostringstream sql;
  sql << "INSERT INTO trip_sets "
         " (point_id, f, c, y, max_commerce, pr_etstatus, et_final_attempt, "
         "  pr_stat, pr_basel_stat, crc_comp, auto_comp_chg";
  for(const auto& i : _settingTypes)
    sql << ", " << i.second;
  sql << ") "
         "VALUES "
         " (:point_id, :f, :c, :y, NULL, 0, 0, "
         "  0, 0, 0, 1";
  for(const auto& i : _settingTypes)
  {
    const TTripSetType& setting=i.first;
    try
    {
      sql << ", :" << i.second;
      if (isBool(setting))
        Qry.CreateVariable(i.second, otInteger, (int)boost::any_cast<bool>(defaultValue(setting)));
      else if (isInt(setting))
        Qry.CreateVariable(i.second, otInteger, boost::any_cast<int>(defaultValue(setting)));
    }
    catch(const boost::bad_any_cast&)
    {
      throwBadCastException(setting, __FUNCTION__);
    };
  };
  sql << ") ";

  Qry.SQLText=sql.str().c_str();
  Qry.CreateVariable("point_id", otInteger, point_id);
  f!=NoExists?Qry.CreateVariable("f", otInteger, f):
              Qry.CreateVariable("f", otInteger, FNull);
  c!=NoExists?Qry.CreateVariable("c", otInteger, c):
              Qry.CreateVariable("c", otInteger, FNull);
  y!=NoExists?Qry.CreateVariable("y", otInteger, y):
              Qry.CreateVariable("y", otInteger, FNull);
  Qry.Execute();
  return *this;
}

const TTripSetList& TTripSetList::toDB(int point_id) const
{
  if (empty()) return *this;

  TQuery Qry(&OraSession);
  list< pair<string, LEvntPrms> > msgs;

  ostringstream sql;
  sql << "UPDATE trip_sets SET ";
  for(TTripSetList::const_iterator i=begin(); i!=end(); ++i)
  {
    try
    {
      if (i!=begin()) sql << ", ";
      sql << toString(i->first) << "=:" << toString(i->first);
      if (isBool(i->first))
        Qry.CreateVariable(toString(i->first), otInteger, (int)boost::any_cast<bool>(i->second));
      else if (isInt(i->first))
        Qry.CreateVariable(toString(i->first), otInteger, boost::any_cast<int>(i->second));

      switch (i->first)
      {
        case tsCheckLoad:
          msgs.emplace_back(boost::any_cast<bool>(i->second)?
                              "EVT.SET_MODE_CHECK_LOAD":
                              "EVT.SET_MODE_WITHOUT_CHECK_LOAD",
                            LEvntPrms());
          break;
        case tsOverloadReg:
          msgs.emplace_back(boost::any_cast<bool>(i->second)?
                              "EVT.SET_MODE_OVERLOAD_REG_PERMISSION":
                              "EVT.SET_MODE_OVERLOAD_REG_PROHIBITION",
                            LEvntPrms());
          break;
        case tsExam:
          msgs.emplace_back(boost::any_cast<bool>(i->second)?
                              "EVT.SET_MODE_EXAM":
                              "EVT.SET_MODE_WITHOUT_EXAM",
                            LEvntPrms());
          break;
        case tsCheckPay:
          msgs.emplace_back(boost::any_cast<bool>(i->second)?
                              "EVT.SET_MODE_CHECK_PAY":
                              "EVT.SET_MODE_WITHOUT_CHECK_PAY",
                            LEvntPrms());
          break;
        case tsExamCheckPay:
          msgs.emplace_back(boost::any_cast<bool>(i->second)?
                              "EVT.SET_MODE_EXAM_CHACK_PAY":
                              "EVT.SET_MODE_WITHOUT_EXAM_CHACK_PAY",
                            LEvntPrms());
          break;
        case tsRegWithTkn:
          msgs.emplace_back(boost::any_cast<bool>(i->second)?
                              "EVT.SET_MODE_REG_WITHOUT_TKN_PROHIBITION":
                              "EVT.SET_MODE_REG_WITHOUT_TKN_PERMISSION",
                            LEvntPrms());
          break;
        case tsRegWithDoc:
          msgs.emplace_back(boost::any_cast<bool>(i->second)?
                              "EVT.SET_MODE_REG_WITHOUT_DOC_PROHIBITION":
                              "EVT.SET_MODE_REG_WITHOUT_DOC_PERMISSION",
                            LEvntPrms());
          break;
        case tsRegWithoutTKNA:
          msgs.emplace_back(boost::any_cast<bool>(i->second)?
                              "EVT.SET_MODE_REG_WITHOUT_TKNA":
                              "EVT.CANCEL_MODE_REG_WITHOUT_TKNA",
                            LEvntPrms());
          break;
        case tsAutoWeighing:
          msgs.emplace_back(boost::any_cast<bool>(i->second)?
                              "EVT.SET_AUTO_WEIGHING":
                              "EVT.CANCEL_AUTO_WEIGHING",
                            LEvntPrms());
          break;
        case tsFreeSeating:
          msgs.emplace_back(boost::any_cast<bool>(i->second)?
                              "EVT.SET_FREE_SEATING":
                              "EVT.CANCEL_FREE_SEATING",
                            LEvntPrms());
          break;
        case tsAPISControl:
          msgs.emplace_back(boost::any_cast<bool>(i->second)?
                              "EVT.SET_APIS_DATA_CONTROL":
                              "EVT.CANCELED_APIS_DATA_CONTROL",
                            LEvntPrms());
          break;
        case tsAPISManualInput:
          msgs.emplace_back(boost::any_cast<bool>(i->second)?
                              "EVT.ALLOWED_APIS_DATA_MANUAL_INPUT":
                              "EVT.NOT_ALLOWED_APIS_DATA_MANUAL_INPUT",
                            LEvntPrms());
          break;
        case tsPieceConcept:
          msgs.emplace_back(boost::any_cast<bool>(i->second)?
                              "EVT.SET_BAGGAGE_PIECE_CONCEPT":
                              "EVT.SET_BAGGAGE_WEIGHT_CONCEPT",
                            LEvntPrms());
          break;
        case tsUseJmp:
          msgs.emplace_back(boost::any_cast<bool>(i->second)?
                              "EVT.ALLOWED_JUMP_SEAT_CHECKIN":
                              "EVT.NOT_ALLOWED_JUMP_SEAT_CHECKIN",
                            LEvntPrms());
          break;
        case tsJmpCfg:
          msgs.emplace_back("EVT.JUMP_SEAT_CFG",
                            LEvntPrms() << PrmSmpl<int>("jmp_cfg", boost::any_cast<int>(i->second)));
          break;
        case tsTransitReg:
          msgs.emplace_back(boost::any_cast<bool>(i->second)?
                              "EVT.TRANSIT_REG.SET":
                              "EVT.TRANSIT_REG.CANCEL",
                            LEvntPrms());
          break;
        case tsTransitBortChanging:
          msgs.emplace_back(boost::any_cast<bool>(i->second)?
                              "EVT.TRANSIT_BORT_CHANGING.SET":
                              "EVT.TRANSIT_BORT_CHANGING.CANCEL",
                            LEvntPrms());
          break;
        case tsTransitBrdWithAutoreg:
          msgs.emplace_back(boost::any_cast<bool>(i->second)?
                              "EVT.TRANSIT_BRD_WITH_AUTOREG.SET":
                              "EVT.TRANSIT_BRD_WITH_AUTOREG.CANCEL",
                            LEvntPrms());
          break;
        case tsTransitBlocking:
          msgs.emplace_back(boost::any_cast<bool>(i->second)?
                              "EVT.TRANSIT_BLOCKING.SET":
                              "EVT.TRANSIT_BLOCKING.CANCEL",
                            LEvntPrms());
          break;
        default:
          break;
      };
    }
    catch(const boost::bad_any_cast&)
    {
      throwBadCastException(i->first, __FUNCTION__);
    }
  };
  sql << " WHERE point_id=:point_id";

  Qry.SQLText=sql.str().c_str();
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  if (Qry.RowsProcessed()>0)
  {
    //запись в журнал операций
    TLogLocale locale;
    locale.ev_type=evtFlt;
    locale.id1=point_id;
    for(list< pair<string, LEvntPrms> >::const_iterator i=msgs.begin(); i!=msgs.end(); ++i)
    {
      locale.lexema_id=i->first;
      locale.prms=i->second;
      TReqInfo::Instance()->LocaleToLog(locale);
    };
  };

  return *this;
}

TTripSetList& TTripSetList::fromDB(int point_id)
{
  clear();
  ostringstream sql;
  sql << "SELECT ";
  for(auto i=_settingTypes.cbegin(); i!=_settingTypes.cend(); ++i)
  {
    if (i!=_settingTypes.cbegin()) sql << ", ";
    sql << i->second;
  };
  sql << " FROM trip_sets WHERE point_id=:point_id";

  TCachedQuery SetsQry(sql.str(), QParams() << QParam("point_id", otInteger, point_id));
  SetsQry.get().Execute();
  if (SetsQry.get().Eof) return *this;
  for(const auto& i : _settingTypes)
  {
    const TTripSetType& setting=i.first;
    if (!SetsQry.get().FieldIsNULL(i.second))
    {
      if (isBool(setting))
        emplace(setting, SetsQry.get().FieldAsInteger(i.second)!=0);
      else if (isInt(setting))
        emplace(setting, SetsQry.get().FieldAsInteger(i.second));
    }
    else
      emplace(setting, defaultValue(setting));
  };
  return *this;
}

void TTripSetList::checkAndCorrectTransitSets(const boost::optional<bool>& pr_tranzit)
{
  bool trueTransitReg=(!pr_tranzit || pr_tranzit.get())?value<bool>(tsTransitReg):false;
  bool trueTransitBortChanging=trueTransitReg?value<bool>(tsTransitBortChanging):false;
  bool trueTransitBrdWithAutoreg=trueTransitBortChanging?value<bool>(tsTransitBrdWithAutoreg):false;
  bool trueTransitBlocking=(!pr_tranzit || pr_tranzit.get()) && !trueTransitReg?value<bool>(tsTransitBlocking):false;

  (*this)[tsTransitReg]=trueTransitReg;
  (*this)[tsTransitBortChanging]=trueTransitBortChanging;
  (*this)[tsTransitBrdWithAutoreg]=trueTransitBrdWithAutoreg;
  (*this)[tsTransitBlocking]=trueTransitBlocking;
}

TTripSetList& TTripSetList::getTransitSets(const TTripInfo &flt, boost::optional<bool>& pr_tranzit)
{
  clear();
  pr_tranzit=boost::none;

  auto cur=make_curs("SELECT pr_tranzit, pr_reg, bort_changing, brd_with_autoreg, "
                     "       DECODE(airline,NULL,0,8)+ "
                     "       DECODE(flt_no,NULL,0,2)+ "
                     "       DECODE(airp_dep,NULL,0,4) AS priority "
                     "FROM tranzit_set "
                     "WHERE (airline IS NULL OR airline=:airline) AND "
                     "      (flt_no IS NULL OR flt_no=:flt_no) AND "
                     "      (airp_dep IS NULL OR airp_dep=:airp_dep) "
                     "ORDER BY priority DESC ");

  bool pr_tranzit_tmp=false;
  bool pr_reg=DefaultTripSets(tsTransitReg);
  bool bort_changing=DefaultTripSets(tsTransitBortChanging);
  bool brd_with_autoreg=DefaultTripSets(tsTransitBrdWithAutoreg);
  cur.bind(":airline", flt.airline)
     .bind(":flt_no", flt.flt_no)
     .bind(":airp_dep", flt.airp)
     .def(pr_tranzit_tmp)
     .def(pr_reg)
     .def(bort_changing)
     .def(brd_with_autoreg)
     .exfet();

  if (cur.err() != NO_DATA_FOUND)
    pr_tranzit=pr_tranzit_tmp;

  emplace(tsTransitReg, pr_reg);
  emplace(tsTransitBortChanging, bort_changing);
  emplace(tsTransitBrdWithAutoreg, brd_with_autoreg);
  emplace(tsTransitBlocking, DefaultTripSets(tsTransitBlocking));

  checkAndCorrectTransitSets(pr_tranzit);

  return *this;
}

TTripSetList& TTripSetList::fromDB(const TTripInfo &info)
{
  clear();
  for(const auto& i : _settingTypes)
  {
    const TTripSetType& setting=i.first;
    if (setting==tsTransitReg ||
        setting==tsTransitBortChanging ||
        setting==tsTransitBrdWithAutoreg ||
        setting==tsTransitBlocking) continue;
    if (isBool(setting))
      emplace(setting, GetTripSets(setting, info));
    else if (isInt(setting))
      emplace(setting, defaultValue(setting));
  }

  return *this;
}

void TTripSetList::append(const TTripSetList &list)
{
  for(const auto& i : list) insert(i);
}

