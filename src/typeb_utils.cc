
#include "typeb_utils.h"
#include "basic.h"
#include "exceptions.h"
#include "astra_locale.h"
#include "misc.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"
#include <cxxabi.h>

using namespace std;
using namespace ASTRA;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace AstraLocale;

std::string typeid_name( const std::type_info& tinfo )
{
    enum { MAX_LEN = 4096 } ;
    char buffer[ MAX_LEN ] ;
    std::size_t size = MAX_LEN ;
    int status ;
    __cxxabiv1::__cxa_demangle( tinfo.name(), buffer, &size, &status ) ;
    return status==0 ? buffer : "__cxa_demangle error" ;
}

namespace TypeB
{

void TErrLst::fetch_err(set<int> &txt_errs, string body)
{
    size_t idx = body.find("<" + ERR_TAG_NAME);
    while(idx != string::npos) {
        size_t end_idx = body.find('>', idx);
        size_t start_i = idx + ERR_TAG_NAME.size() + 1;
        txt_errs.insert(ToInt(body.substr(start_i, end_idx - start_i)));
        body.erase(0, end_idx + 1);
        idx = body.find("<" + ERR_TAG_NAME);
    }
}

void TErrLst::fix(vector<TDraftPart> &parts)
{
    set<int> txt_errs; // ошибки, содержащиеся в тексте телеграммы
    for(vector<TDraftPart>::iterator iv = parts.begin(); iv != parts.end(); iv++){
        fetch_err(txt_errs, iv->addr);
        fetch_err(txt_errs, iv->heading);
        fetch_err(txt_errs, iv->body);
        fetch_err(txt_errs, iv->ending);
    }
    vector<int> unused_errors; // ошибки, отсутствующие в тексте телеграммы
    for(map<int, string>::iterator im = begin(); im != end(); im++)
        if(txt_errs.find(im->first) == txt_errs.end())
            unused_errors.push_back(im->first);
    for(vector<int>::iterator iv = unused_errors.begin(); iv != unused_errors.end(); iv++) {
        ProgTrace(TRACE5, "delete unused error %d", *iv);
        erase(*iv);
    }
}

void TErrLst::dump()
{
    ProgTrace(TRACE5, "-----------TLG ERR_LST-----------");
    for(map<int, string>::iterator im = begin(); im != end(); im++)
        ProgTrace(TRACE5, "ERR_NO: %d, %s", im->first, im->second.c_str());
    ProgTrace(TRACE5, "--------END OF TLG ERR_LST-------");
}

bool TDetailCreateInfo::operator == (const TMktFlight &s) const
{
    return
        airline == s.airline and
        flt_no == s.flt_no and
        suffix == s.suffix and
        airp_dep == s.airp_dep and
        scd_local_day == s.scd_day_local;
}

string TDetailCreateInfo::add_err(string err, std::string val)
{
    return add_err(err, val.c_str());
}

string TDetailCreateInfo::add_err(string err, const char *format, ...)
{
    char Message[500];
    va_list ap;
    va_start(ap, format);
    vsnprintf(Message, sizeof(Message), format, ap);
    Message[sizeof(Message)-1]=0;
    va_end(ap);
    err_lst[err_lst.size() + 1] = Message;
    ostringstream buf;
    buf << "<ERROR" << err_lst.size() << ">" << err << "</ERROR" << err_lst.size() << ">";
    return buf.str();
}

string TlgElemIdToElem(TElemType type, int id, TElemFmt fmt, string lang)
{
  if(not(fmt==efmtCodeNative || fmt==efmtCodeInter))
      throw Exception("TlgElemIdToElem: Wrong fmt");

  vector< pair<TElemFmt,string> > fmts;
  fmts.push_back( make_pair(fmt, lang) );
  if(fmt == efmtCodeNative)
      fmts.push_back( make_pair(efmtCodeInter, lang) );

  string result = ElemIdToElem(type, id, fmts);
  if(result.empty() || (fmt==efmtCodeInter && !IsAscii7(result))) {
      string code_name;
      switch(type)
      {
          case etClsGrp:
              code_name = "CLS_GRP";
              break;
          default:
              throw Exception("TlgElemIdToElem: Unsupported int elem type");
      }
      throw AstraLocale::UserException((string)"MSG." + code_name + "_LAT_CODE_NOT_FOUND", LParams() << LParam("id", id));
  }
  return result;
};

string TlgElemIdToElem(TElemType type, string id, TElemFmt fmt, string lang)
{
  if(id.empty())
      throw Exception("TlgElemIdToElem: id is empty.");
  if(not(fmt==efmtCodeNative || fmt==efmtCodeInter))
      throw Exception("TlgElemIdToElem: Wrong fmt.");

  vector< pair<TElemFmt,string> > fmts;
  fmts.push_back( make_pair(fmt, lang) );
  if(fmt == efmtCodeNative)
      fmts.push_back( make_pair(efmtCodeInter, lang) );

  string result = ElemIdToElem(type, id, fmts);
  if(result.empty() || (fmt==efmtCodeInter && !IsAscii7(result))) {
      string code_name;
      switch(type)
      {
          case etCountry:
              code_name="COUNTRY";
              break;
          case etCity:
              code_name="CITY";
              break;
          case etAirline:
              code_name="AIRLINE";
              break;
          case etAirp:
              code_name="AIRP";
              break;
          case etCraft:
              code_name="CRAFT";
              break;
          case etClass:
              code_name="CLS";
              break;
          case etSubcls:
              code_name="SUBCLS";
              break;
          case etPersType:
              code_name="PAX_TYPE";
              break;
          case etGenderType:
              code_name="GENDER";
              break;
          case etPaxDocType:
              code_name="DOC";
              break;
          case etPayType:
              code_name="PAY_TYPE";
              break;
          case etCurrency:
              code_name="CURRENCY";
              break;
          case etSuffix:
              code_name="SUFFIX";
              break;
          default:
              throw Exception("TlgElemIdToElem: Unsupported elem type.");
      };
      throw AstraLocale::UserException((string)"MSG." + code_name + "_LAT_CODE_NOT_FOUND", LParams() << LParam("id", id));
  }
  return result;
};

string TDetailCreateInfo::TlgElemIdToElem(TElemType type, int id, TElemFmt fmt)
{
    if(fmt == efmtUnknown)
        fmt = elem_fmt;
    try {
        return TypeB::TlgElemIdToElem(type, id, fmt, lang);
    } catch(UserException &E) {
        return add_err(DEFAULT_ERR, getLocaleText(E.getLexemaData()));
    } catch(exception &E) {
        return add_err(DEFAULT_ERR, "TTlgInfo::TlgElemIdToElem: tlg_type: %s, elem_type: %s, fmt: %s, what: %s", get_tlg_type().c_str(), EncodeElemType(type), EncodeElemFmt(fmt), E.what());
    } catch(...) {
        return add_err(DEFAULT_ERR, "TTlgInfo::TlgElemIdToElem: unknown except caught. tlg_type: %s, elem_type: %s, fmt: %s", get_tlg_type().c_str(), EncodeElemType(type), EncodeElemFmt(fmt));
    }
}

string TDetailCreateInfo::TlgElemIdToElem(TElemType type, string id, TElemFmt fmt)
{
    if(fmt == efmtUnknown)
        fmt = elem_fmt;
    try {
        return TypeB::TlgElemIdToElem(type, id, fmt, lang);
    } catch(UserException &E) {
        return add_err(id, getLocaleText(E.getLexemaData()));
    } catch(exception &E) {
        return add_err(id, "TTlgInfo::TlgElemIdToElem: tlg_type: %s, elem_type: %s, fmt: %s, what: %s", get_tlg_type().c_str(), EncodeElemType(type), EncodeElemFmt(fmt), E.what());
    } catch(...) {
        return add_err(id, "TTlgInfo::TlgElemIdToElem: unknown except caught. tlg_type: %s, elem_type: %s, fmt: %s", get_tlg_type().c_str(), EncodeElemType(type), EncodeElemFmt(fmt));
    }
}

bool TDetailCreateInfo::is_lat()
{
  return get_options().is_lat;
};

string TDetailCreateInfo::airline_view()
{
  const TMarkInfoOptions *markOptions = dynamic_cast<const TMarkInfoOptions*>(&get_options());
  if (markOptions==NULL || markOptions->mark_info.empty() || !markOptions->pr_mark_header)
    return TlgElemIdToElem(etAirline, airline);
  else
    return TlgElemIdToElem(etAirline, markOptions->mark_info.airline);
};

string TDetailCreateInfo::flt_no_view()
{
  int flt_no_tmp;
  const TMarkInfoOptions *markOptions = dynamic_cast<const TMarkInfoOptions*>(&get_options());
  if (markOptions==NULL || markOptions->mark_info.empty() || !markOptions->pr_mark_header)
    flt_no_tmp=flt_no;
  else
    flt_no_tmp=markOptions->mark_info.flt_no;

  if (flt_no_tmp==NoExists) return "???";
  ostringstream flt;
  flt << setw(3) << setfill('0') << flt_no_tmp;
  return flt.str();
};

string TDetailCreateInfo::suffix_view()
{
  const TMarkInfoOptions *markOptions = dynamic_cast<const TMarkInfoOptions*>(&get_options());
  if (markOptions==NULL || markOptions->mark_info.empty() || !markOptions->pr_mark_header)
    return suffix.empty()?"":TlgElemIdToElem(etSuffix, suffix);
  else
    return markOptions->mark_info.suffix.empty()?"":TlgElemIdToElem(etSuffix, markOptions->mark_info.suffix);
};

string TDetailCreateInfo::airp_dep_view()
{
  return TlgElemIdToElem(etAirp, airp_dep);
};

string TDetailCreateInfo::airp_trfer_view()
{
  const TAirpTrferOptions *trferOptions = dynamic_cast<const TAirpTrferOptions*>(&get_options());
  if (trferOptions==NULL || trferOptions->airp_trfer.empty()) return "???";
  return TlgElemIdToElem(etAirp, trferOptions->airp_trfer);
};

string TDetailCreateInfo::airp_arv_view()
{
  if (airp_arv.empty()) return "???";
  return TlgElemIdToElem(etAirp, airp_arv);
};

string TDetailCreateInfo::flight_view()
{
  ostringstream flt;
  flt << airline_view()
      << flt_no_view()
      << suffix_view();
  return flt.str();
};

string TDetailCreateInfo::scd_local_view()
{
  if (scd_local==NoExists) return "?????";
  return DateTimeToStr(scd_local, "ddmmm", 1);
};

string TDetailCreateInfo::airline_mark() const
{
    const TMarkInfoOptions *markOptions = dynamic_cast<const TMarkInfoOptions*>(&get_options());
    if (markOptions==NULL || markOptions->mark_info.empty() || !markOptions->pr_mark_header)
        return "";
    else
        return markOptions->mark_info.airline;
}

string fetch_addr(string &addr, TDetailCreateInfo *info)
{
    string result;
    // пропускаем все символы не относящиеся к слову
    size_t i = 0;
    size_t len = 0;
    try {
        for(i = 0; i < addr.size() && (u_char)addr[i] <= 0x20; i++) ;
        for(len = 0; i + len < addr.size() && (u_char)addr[i + len] > 0x20; len++) ;
        result = addr.substr(i, len);
        if(addr.size() == len)
            addr.erase();
        else
            addr = addr.substr(len + i);
        if(not(result.empty() or result.size() == 7))
            throw AstraLocale::UserException("MSG.TLG.INVALID_SITA_ADDR", LParams() << LParam("addr", result));
        for(i = 0; i < result.size(); i++) {
            // c BETWEEN 'A' AND 'Z' OR c BETWEEN '0' AND '9'
            u_char c = result[i];
            if((IsUpperLetter(c) and IsAscii7(c)) or IsDigit(c))
                continue;
            throw AstraLocale::UserException("MSG.TLG.INVALID_SITA_ADDR", LParams() << LParam("addr", result));
        }
    } catch(UserException &E) {
        if(not info)
            throw;
        else
            result = info->add_err(result, getLocaleText(E.getLexemaData()));
    } catch(exception &E) {
        if(not info)
            throw;
        else
            result = info->add_err(result, "fetch_addr failed: %s", E.what());
    } catch(...) {
        if(not info)
            throw;
        else
            result = info->add_err(result, "fetch_addr failed. unknown exception");
    }
    return result;
}

string format_addr_line(string vaddrs, TDetailCreateInfo *info)
{
    string result, addr_line;
    try {
        int n = 0;
        string addr = fetch_addr(vaddrs, info);
        while(!addr.empty()) {
            if(result.find(addr) == string::npos && addr_line.find(addr) == string::npos) {
                n++;
                if(n > 32)
                    throw AstraLocale::UserException("MSG.TLG.MORE_THEN_32_ADDRS");
                if(addr_line.size() + addr.size() + 1 > 64) {
                    result += addr_line + endl;
                    addr_line = addr;
                } else {
                    if(addr_line.empty())
                        addr_line = addr;
                    else
                        addr_line += " " + addr;
                }
            }
            addr = fetch_addr(vaddrs, info);
        }
        if(!addr_line.empty())
            result += addr_line;
    } catch(UserException &E) {
        if(not info)
            throw;
        else
            result = info->add_err(DEFAULT_ERR, getLocaleText(E.getLexemaData()));
    } catch(exception &E) {
        if(not info)
            throw;
        else
            result = info->add_err(DEFAULT_ERR, "format_addr_line failed: %s", E.what());
    } catch(...) {
        if(not info)
            throw;
        else
            result = info->add_err(DEFAULT_ERR, "format_addr_line failed. unknown exception");
    }
    result += endl;
    return result;
}

TOriginatorInfo getOriginator(const string &airline,
                              const string &airp_dep,
                              const string &tlg_type,
                              const TDateTime &time_create,
                              bool with_exception)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT id, addr, "
    "       DECODE(airline,NULL,0,8) + "
    "       DECODE(airp_dep,NULL,0,4) + "
    "       DECODE(tlg_type,NULL,0,2) AS priority "
    "FROM typeb_originators "
    "WHERE first_date<=:time_create AND "
    "      (last_date IS NULL OR last_date>:time_create) AND "
    "      (airline IS NULL OR airline=:airline) AND "
    "      (airp_dep IS NULL OR airp_dep=:airp_dep) AND "
    "      (tlg_type IS NULL OR tlg_type=:tlg_type) "
    "ORDER BY priority DESC";
  Qry.CreateVariable("airline", otString, airline);
  Qry.CreateVariable("airp_dep", otString, airp_dep);
  Qry.CreateVariable("tlg_type", otString, tlg_type);
  Qry.CreateVariable("time_create", otDate, time_create);
  Qry.Execute();
  TOriginatorInfo originator;
  if (!Qry.Eof)
  {
    originator.id=Qry.FieldAsInteger("id");
    originator.addr=Qry.FieldAsString("addr");
  };

  if (with_exception)
  {
    if(originator.addr.empty())
      throw AstraLocale::UserException("MSG.TLG.SRC_ADDR_NOT_SET");
    if(originator.addr.size() != 7)
      throw AstraLocale::UserException("MSG.TLG.SRC_ADDR_WRONG_SET");
    for(string::const_iterator c=originator.addr.begin(); c!=originator.addr.end(); c++)
      if (!(IsAscii7(*c) && (IsDigit(*c) || IsUpperLetter(*c))))
        throw AstraLocale::UserException("MSG.TLG.SRC_ADDR_WRONG_SET");
  };
  return originator;
};

tr1::shared_ptr<TCreateOptions> make_options(const string &tlg_type)
{
  string basic_type;
  if (!tlg_type.empty())
  try
  {
    const TTypeBTypesRow& row = (TTypeBTypesRow&)(base_tables.get("typeb_types").get_row("code",tlg_type));
    basic_type=row.basic_type;
  }
  catch(EBaseTableError)
  {
    throw Exception("TypeB::make_options: unknown telegram type %s", tlg_type.c_str());
  };

  if      (basic_type=="PTM" ||
           basic_type=="BTM")
    return tr1::shared_ptr<TCreateOptions>(new TAirpTrferOptions);
  else if (basic_type=="PFS" ||
           basic_type=="PRL" ||
           basic_type=="ETL" ||
           basic_type=="FTL")
    return tr1::shared_ptr<TCreateOptions>(new TMarkInfoOptions);
  else if (basic_type=="LDM")
    return tr1::shared_ptr<TCreateOptions>(new TLDMOptions);
  else if (basic_type=="LCI")
    return tr1::shared_ptr<TCreateOptions>(new TLCIOptions);
  else if (basic_type=="???")
    return tr1::shared_ptr<TCreateOptions>(new TUnknownFmtOptions);
  else
    return tr1::shared_ptr<TCreateOptions>(new TCreateOptions);
};

bool TSendInfo::isSend() const
{
  bool pr_dep=false,pr_arv=false;
  try
  {
    const TTypeBTypesRow& row = (TTypeBTypesRow&)(base_tables.get("typeb_types").get_row("code",tlg_type));
    pr_dep=row.pr_dep==NoExists || row.pr_dep!=0;
    pr_arv=row.pr_dep==NoExists || row.pr_dep==0;
  }
  catch(EBaseTableError)
  {
    return false;
  };

  ostringstream sql;
  TQuery SendQry(&OraSession);
  SendQry.Clear();

  if (pr_dep && pr_arv)
  {
    //не привязывается к прилету и вылету
    sql << "SELECT pr_denial, "
           "       DECODE(airline,NULL,0,8)+ "
           "       DECODE(flt_no,NULL,0,1)+ "
           "       DECODE(airp_dep,NULL,0,4)+ "
           "       DECODE(airp_arv,NULL,0,2) AS priority ";
  }
  else
  {
    if (pr_dep)
      sql << "SELECT pr_denial, "
             "       DECODE(airline,NULL,0,8)+ "
             "       DECODE(flt_no,NULL,0,1)+ "
             "       DECODE(airp_dep,NULL,0,4)+ "
             "       DECODE(airp_arv,NULL,0,2) AS priority ";
    else
      sql << "SELECT pr_denial, "
             "       DECODE(airline,NULL,0,8)+ "
             "       DECODE(flt_no,NULL,0,1)+ "
             "       DECODE(airp_dep,NULL,0,2)+ "
             "       DECODE(airp_arv,NULL,0,4) AS priority ";
  };

  sql << "FROM typeb_send "
         "WHERE tlg_type=:tlg_type AND "
         "      (airline IS NULL OR airline=:airline) AND "
         "      (flt_no IS NULL OR flt_no=:flt_no) ";
  SendQry.CreateVariable("tlg_type",otString,tlg_type);
  SendQry.CreateVariable("airline",otString,airline);
  SendQry.CreateVariable("flt_no",otInteger,flt_no);

  if (pr_dep)
  {
    //привязывается к вылету
    sql << " AND "
           "(airp_dep=:airp_dep OR airp_dep IS NULL) AND "
           "(airp_arv IN "
           "  (SELECT airp FROM points "
           "   WHERE first_point=:first_point AND point_num>:point_num AND pr_del=0) OR airp_arv IS NULL)";
    SendQry.CreateVariable("airp_dep",otString,airp_dep);
  };
  if (pr_arv)
  {
    //привязывается к прилету
    sql << " AND "
           "(airp_arv=:airp_arv OR airp_arv IS NULL) AND "
           "(airp_dep IN "
           "  (SELECT airp FROM points "
           "   WHERE :first_point IN (point_id,first_point) AND point_num<=:point_num AND pr_del=0) OR airp_dep IS NULL)";
    SendQry.CreateVariable("airp_arv",otString,airp_arv);
    if (airp_arv.empty())
    {
      TTripRoute route;
      route.GetRouteAfter(NoExists,
                          point_id,
                          point_num,
                          first_point,
                          pr_tranzit,
                          trtNotCurrent,trtNotCancelled);
      if (!route.empty())
        SendQry.SetVariable("airp_arv", route.begin()->airp);
    };
  };

  SendQry.CreateVariable("first_point", otInteger, pr_tranzit ? first_point : point_id);
  SendQry.CreateVariable("point_num", otInteger, point_num);

  sql << "ORDER BY priority DESC";

  SendQry.SQLText=sql.str().c_str();
  SendQry.Execute();
  if (SendQry.Eof||SendQry.FieldAsInteger("pr_denial")!=0) return false;
  return true;
};

void TSendInfo::getCreateInfo(const vector<TSimpleMktFlight> &mktFlights,
                              bool onlyOneFlight,
                              vector<TCreateInfo> &info) const
{
  info.clear();
  bool pr_dep=false,pr_arv=false;
  try
  {
    const TTypeBTypesRow& row = (TTypeBTypesRow&)(base_tables.get("typeb_types").get_row("code",tlg_type));
    pr_dep=row.pr_dep==NoExists || row.pr_dep!=0;
    pr_arv=row.pr_dep==NoExists || row.pr_dep==0;
  }
  catch(EBaseTableError)
  {
    return;
  };

  ostringstream sql;
  TQuery OptionsQry(&OraSession);
  TQuery AddrQry(&OraSession);
  AddrQry.Clear();
  sql << "SELECT * FROM typeb_addrs "
         "WHERE pr_mark_flt=:pr_mark_flt AND tlg_type=:tlg_type AND "
         "      (airline=:airline OR airline IS NULL) AND "
         "      (flt_no=:flt_no OR flt_no IS NULL) ";
  AddrQry.CreateVariable("tlg_type",otString,tlg_type);
  AddrQry.DeclareVariable("pr_mark_flt",otInteger);
  AddrQry.DeclareVariable("airline",otString);
  AddrQry.DeclareVariable("flt_no",otInteger);

  if (pr_dep)
  {
    //привязывается к вылету
    sql << " AND "
           "(airp_dep=:airp_dep OR airp_dep IS NULL) AND "
           "(airp_arv IN "
           "  (SELECT airp FROM points "
           "   WHERE first_point=:first_point AND point_num>:point_num AND pr_del=0) OR airp_arv IS NULL)";
    AddrQry.CreateVariable("airp_dep",otString,airp_dep);
  };
  if (pr_arv)
  {
    //привязывается к прилету
    sql << " AND "
           "(airp_arv=:airp_arv OR airp_arv IS NULL) AND "
           "(airp_dep IN "
           "  (SELECT airp FROM points "
           "   WHERE :first_point IN (point_id,first_point) AND point_num<=:point_num AND pr_del=0) OR airp_dep IS NULL)";
    AddrQry.CreateVariable("airp_arv",otString,airp_arv);
    if (airp_arv.empty())
    {
      TTripRoute route;
      route.GetRouteAfter(NoExists,
                          point_id,
                          point_num,
                          first_point,
                          pr_tranzit,
                          trtNotCurrent,trtNotCancelled);
      if (!route.empty())
        AddrQry.SetVariable("airp_arv", route.begin()->airp);
    };
  };
  AddrQry.CreateVariable("first_point", otInteger, pr_tranzit ? first_point : point_id);
  AddrQry.CreateVariable("point_num",otInteger,point_num);
  AddrQry.SQLText=sql.str();

  for(vector<TSimpleMktFlight>::const_iterator f=mktFlights.begin();;++f)
  {
    if (f!=mktFlights.end())
    {
      AddrQry.SetVariable("pr_mark_flt", (int)true);
      AddrQry.SetVariable("airline", f->airline);
      if (f->flt_no!=NoExists)
        AddrQry.SetVariable("flt_no", f->flt_no);
      else
        AddrQry.SetVariable("flt_no", FNull);
    }
    else
    {
      AddrQry.SetVariable("pr_mark_flt", (int)false);
      AddrQry.SetVariable("airline", airline);
      if (flt_no!=NoExists)
        AddrQry.SetVariable("flt_no", flt_no);
      else
        AddrQry.SetVariable("flt_no", FNull);
    };

    AddrQry.Execute();
    for(;!AddrQry.Eof;AddrQry.Next())
    {
      TCreateInfo createInfo;
      createInfo.fromDB(AddrQry, OptionsQry);
      info.push_back(createInfo);
    };
    if (onlyOneFlight) break;
    if (f==mktFlights.end()) break;
  };
};

std::string TAddrInfo::getAddrs() const
{
  //  ProgTrace(TRACE5,"GetTypeBAddrs: tlg_type=%s flt_no=%d first_point=%d point_num=%d airp_dep=%s airp_arv=%s",
  //                   tlg_type.c_str(),flt_no,first_point,point_num,airp_dep.c_str(),airp_arv.c_str());

  TMarkInfoOptions *markOptions=NULL;
  if (optionsIs<TMarkInfoOptions>())
    markOptions=optionsAs<TMarkInfoOptions>();

  vector<TCreateInfo> info;
  if (markOptions==NULL || markOptions->mark_info.empty())
    sendInfo.getCreateInfo(vector<TSimpleMktFlight>(), true, info);
  else
    sendInfo.getCreateInfo(vector<TSimpleMktFlight>(1,markOptions->mark_info), true, info);

  TCreateInfo ci;
  for(vector<TCreateInfo>::const_iterator i=info.begin(); i!=info.end(); ++i)
  {
    if (!i->get_options().similar(get_options())) continue;
    ci.addrs.insert(i->addrs.begin(), i->addrs.end());
  };

  return ci.get_addrs();
};

TCreator::TCreator(int point_id)
{
  airps_init=false;
  crs_init=false;
  mkt_flights_init=false;

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT airline,flt_no,suffix,airp,scd_out,act_out, "
    "       point_id,point_num,first_point,pr_tranzit "
    "FROM points WHERE point_id=:point_id AND pr_del>=0";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (!Qry.Eof) flt.Init(Qry);
};

const set<string>& TCreator::airps()
{
  if (!airps_init)
  {
    //получим все аэропорты по маршруту
    TTripRoute route;
    route.GetRouteAfter(NoExists,
                        flt.point_id,
                        flt.point_num,
                        flt.first_point,
                        flt.pr_tranzit,
                        trtNotCurrent,trtNotCancelled);
    for(TTripRoute::const_iterator r=route.begin();r!=route.end();++r)
      p_airps.insert(r->airp);

    airps_init=true;
  };
  return p_airps;
};

const vector<string>& TCreator::crs()
{
  if (!crs_init)
  {
    //получим все системы бронирования из кот. была посылка
    GetCrsList(flt.point_id, p_crs);
    crs_init=true;
  };
  return p_crs;
};

const vector<TSimpleMktFlight>& TCreator::mkt_flights()
{
  if (!mkt_flights_init)
  {
    //получим список коммерческих рейсов если таковые имеются
    vector<TTripInfo> markFltInfo;
    GetMktFlights(flt,markFltInfo);
    for(vector<TTripInfo>::const_iterator f=markFltInfo.begin(); f!=markFltInfo.end(); ++f)
    {
      TSimpleMktFlight mktFlight;
      mktFlight.airline=f->airline;
      mktFlight.flt_no=f->flt_no;
      mktFlight.suffix=f->suffix;
      p_mkt_flights.push_back(mktFlight);
    };
    mkt_flights_init=true;
  };
  return p_mkt_flights;
};

void TCreateInfo::dump() const
{
  localizedstream extra(AstraLocale::LANG_EN);
  ProgTrace(TRACE5, "point_id=%d tlg_type=%s options.typeName=%s",
                    point_id, get_tlg_type().c_str(),
                    get_options().typeName().c_str());
  ProgTrace(TRACE5, "     extra=%s", get_options().logStr(extra).str().c_str());
  ProgTrace(TRACE5, "     addrs=%s", get_addrs().c_str());
};

void TCreator::getInfo(vector<TCreateInfo> &info)
{
  info.clear();
  if (flt.point_id==NoExists) return;

  for(set<string>::const_iterator t=tlg_types.begin(); t!=tlg_types.end(); ++t)
  {
    TSendInfo si(*t, flt);
    if (!si.isSend()) continue;

    vector<TCreateInfo> ci;

    if (TAddrInfo(si).optionsIs<TMarkInfoOptions>())
      si.getCreateInfo(mkt_flights(), false, ci);
    else
      si.getCreateInfo(vector<TSimpleMktFlight>(), false, ci);

    for(vector<TCreateInfo>::const_iterator i=ci.begin(); i!=ci.end(); ++i)
    {
      bool isMarkInfoOptions=i->optionsIs<TMarkInfoOptions>();
      bool isCrsOptions=i->optionsIs<TCrsOptions>();
      bool isAirpTrferOptions=i->optionsIs<TAirpTrferOptions>();

      TCreateInfo createInfo;
      createInfo.copy(*i);
      createInfo.point_id=flt.point_id;

      if (createInfo.addrs.empty()) continue;
      //цикл по коммерческим рейсам
      vector<TSimpleMktFlight>::const_iterator f;
      for(isMarkInfoOptions?f=mkt_flights().begin():f;;isMarkInfoOptions?++f:f)
      {
        if (isMarkInfoOptions)
        {
          TMarkInfoOptions &markOptions=*(createInfo.optionsAs<TMarkInfoOptions>());
          if (f!=mkt_flights().end())
          {
            if (i->optionsAs<TMarkInfoOptions>()->pr_mark_header==NoExists) continue;
            markOptions.mark_info.airline=f->airline;
            markOptions.mark_info.flt_no=f->flt_no;
            markOptions.mark_info.suffix=f->suffix;
          }
          else
          {
            if (i->optionsAs<TMarkInfoOptions>()->pr_mark_header!=NoExists) break;
            markOptions.mark_info.clear();
          };
        };
        //цикл по центрам бронирования
        vector<string>::const_iterator c;
        for(isCrsOptions?c=crs().begin():c;;isCrsOptions?++c:c)
        {
          if (isCrsOptions)
            createInfo.optionsAs<TCrsOptions>()->crs=(c!=crs().end())?*c:"";

          //цикл по аэропортам маршрута
          set<string>::const_iterator a;
          for(isAirpTrferOptions?a=airps().begin():a;
              isAirpTrferOptions?a!=airps().end():true;
              isAirpTrferOptions?++a:a)
          {
            if (isAirpTrferOptions)
              createInfo.optionsAs<TAirpTrferOptions>()->airp_trfer=*a;

            //здесь сформирован createInfo полностью
            if (i->get_options().similar(createInfo.get_options()))
            {
              if (validInfo(createInfo))
              {

                vector<TCreateInfo>::iterator i2=info.begin();
                for(; i2!=info.end(); ++i2)
                  if (i2->canMerge(createInfo)) break;

                if (i2!=info.end())
                  //объединяем адреса
                  i2->addrs.insert(createInfo.addrs.begin(), createInfo.addrs.end());
                else
                {
                  TCreateInfo tmp;
                  tmp.copy(createInfo); //надо создать копию
                  info.push_back(tmp);
                };
              };
            };

            if (isAirpTrferOptions?a==airps().end():true) break;
          };
          if (isCrsOptions?c==crs().end():true) break;
        };
        if (isMarkInfoOptions?f==mkt_flights().end():true) break;
      };
    };
  };
};

}


