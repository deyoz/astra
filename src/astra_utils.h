#ifndef _ASTRA_UTILS_H_
#define _ASTRA_UTILS_H_

#include <string>
#include <map>
#include <set>
#include <vector>
#include <optional>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include "astra_consts.h"
#include "astra_locale.h"
#include "astra_locale_adv.h"
#include "date_time.h"
#include "exceptions.h"
#include "oralib.h"
#include "jxtlib/JxtInterface.h"
#include "jxtlib/jxt_xml_cont.h"
#include <libtlg/tlgnum.h>
#include "astra_types.h"
#include <serverlib/dbcpp_cursctl.h>
#include <serverlib/str_utils.h>

using BASIC::date_time::TDateTime;

std::string AlignString(std::string str, int len, std::string align);

struct TLogMsg {
  public:
    TDateTime ev_time;
    int ev_order;
    std::string msg;
    ASTRA::TEventType ev_type;
    int id1,id2,id3;
    TLogMsg() {
      Clear();
    };
    void Clear() {
      ev_time = ASTRA::NoExists;
      ev_order = ASTRA::NoExists;
      ev_type = ASTRA::evtUnknown;
      msg = "";
      id1 = ASTRA::NoExists;
      id2 = ASTRA::NoExists;
      id3 = ASTRA::NoExists;
    };
};

struct TLogLocale {
  protected:
    std::vector<std::string> vlangs;
  public:
    TDateTime ev_time;
    int ev_order;
    ASTRA::TEventType ev_type;
    std::string sub_type;
    std::string lexema_id;
    LEvntPrms prms;
    int id1,id2,id3;
    TLogLocale()
    {
      clear();
    }
    void clear()
    {
      ev_time=ASTRA::NoExists;
      ev_order=ASTRA::NoExists;
      ev_type=ASTRA::evtUnknown;
      sub_type.clear();
      lexema_id.clear();
      prms.clearPrms();
      id1=ASTRA::NoExists;
      id2=ASTRA::NoExists;
      id3=ASTRA::NoExists;
      vlangs.clear();
      vlangs.push_back(AstraLocale::LANG_RU);
      vlangs.push_back(AstraLocale::LANG_EN);
    }

    void toXML(xmlNodePtr node) const;
    void fromXML(xmlNodePtr node);
    void toDB(const std::string &screen,
              const std::string &user_descr,
              const std::string &desk_code);
};

enum TUserType { utSupport=0, utAirport=1, utAirline=2 };
enum TUserSettingType { ustTimeUTC=0, ustTimeLocalDesk=1, ustTimeLocalAirp=2,
                        ustCodeNative=5, ustCodeInter=6,
                        ustCodeICAONative=7, ustCodeICAOInter=8, ustCodeMixed=9,
                        ustEncNative=15, ustEncLatin=16, ustEncMixed=17 };

template <class T>
class BitSet
{
  std::map<T,bool> flags; /* � �� ���� ������ �� �� */
  public:
  void setFlag( T key ) {
    flags[ key ] = true;
  }
  void clearFlag( T key ) {
    if ( isFlag( key ) )
      flags[ key ] = false;
  }
  bool isFlag( T key ) const {
   typename std::map<T,bool>::const_iterator pos = flags.find( key );
   if ( pos == flags.end() )
     return false;
   return pos->second;
  }
  void clearFlags( ) {
    flags.clear();
  }
  bool emptyFlags() const {
    for (typename std::map<T,bool>::const_iterator pos = flags.begin(); pos!=flags.end(); pos++) {
      if ( pos->second )
        return false;
    }
    return true;
  }
  void operator += (const BitSet &bs) {
    for (typename std::map<T,bool>::const_iterator pos = bs.flags.begin(); pos!=bs.flags.end(); pos++) {
      if ( pos->second ) {
        setFlag( pos->first );
      }
    }
  }
  bool operator == (const BitSet &bs) const {
    for (typename std::map<T,bool>::const_iterator pos = bs.flags.begin(); pos!=bs.flags.end(); pos++) {
      if ( isFlag( pos->first ) != bs.isFlag( pos->first ) )
        return false;
    }
    for (typename std::map<T,bool>::const_iterator pos = flags.begin(); pos!=flags.end(); pos++) {
      if ( isFlag( pos->first ) != bs.isFlag( pos->first ) )
        return false;
    }
    return true;
  }
};

template <class T>
std::string GetSQLEnum(const T &values)
{
  std::ostringstream res;
  bool first_iteration=true;
  for(typename T::const_iterator i=values.begin();i!=values.end();++i)
  {
    if (i->empty()) continue;
    if (!first_iteration) res << ", ";
    res << "'" << *i << "'";
    first_iteration=false;
  };
  if (!first_iteration)
    return " ("+res.str()+") ";
  else
    return "";
}

template <class T>
std::string getSQLEnum(const T &values)
{
  std::ostringstream res;
  bool first_iteration=true;
  for(typename T::const_iterator i=values.begin();i!=values.end();++i)
  {
    if (!first_iteration) res << ", ";
    res << "'" << *i << "'";
    first_iteration=false;
  };
  if (!first_iteration)
    return " ("+res.str()+") ";
  else
    return "";
}

template<typename T>
class TAccessElems
{
  private:
    std::set<T> _elems;
    bool _elems_permit;
  public:
    TAccessElems():_elems_permit(true) {}
    TAccessElems(const T &elem, bool elems_permit)
    {
      _elems.insert(elem);
      _elems_permit=elems_permit;
    }
    void clear()
    {
      //����� �����
      _elems.clear();
      _elems_permit=true;
    }
    void add_elem(const T &elem)
    {
      _elems.insert(elem);
    }
    void set_elems_permit(bool elems_permit)
    {
      _elems_permit=elems_permit;
    }
    void merge(const TAccessElems &e)
    {
      std::set<T> dest;
      if (_elems_permit)
      {
        if (e._elems_permit)
        {
          set_intersection(_elems.begin(), _elems.end(),
                           e._elems.begin(), e._elems.end(),
                           inserter(dest, dest.begin()));
        }
        else
        {
          set_difference(_elems.begin(), _elems.end(),
                         e._elems.begin(), e._elems.end(),
                         inserter(dest, dest.begin()));
        }
      }
      else
      {
        if (e._elems_permit)
        {
          set_difference(e._elems.begin(), e._elems.end(),
                         _elems.begin(), _elems.end(),
                         inserter(dest, dest.begin()));
          _elems_permit=true;
        }
        else
        {
          set_union(_elems.begin(), _elems.end(),
                    e._elems.begin(), e._elems.end(),
                    inserter(dest, dest.begin()));
        }
      };
      _elems=dest;
    }
    void set_total_permit()
    {
      //����� �����
      _elems.clear();
      _elems_permit=false;
    }

    bool permitted(const T &elem) const
    {
      if (_elems_permit)
        return _elems.find(elem)!=_elems.end();
      else
        return _elems.find(elem)==_elems.end();
    }
    bool totally_permitted() const { return _elems.empty() && !_elems_permit; }
    bool totally_not_permitted() const { return _elems.empty() && _elems_permit; }
    bool only_single_permit() const { return _elems.size()==1 && _elems_permit; }
    bool operator==(const TAccessElems<T>& e) const
    {
      return _elems==e._elems &&
             _elems_permit==e._elems_permit;
    }

    const std::set<T>& elems() const { return _elems; }
    bool elems_permit() const { return _elems_permit; }

    void build_test(int iter, const T &e1, const T &e2, const T &e3)
    {
      clear();
      if ((iter&0x0001)!=0x0000)
        set_elems_permit(true);
      else
        set_elems_permit(false);
      if ((iter&0x0002)!=0x0000) add_elem(e1);
      if ((iter&0x0004)!=0x0000) add_elem(e2);
      if ((iter&0x0008)!=0x0000) add_elem(e3);
    }
};

template<typename T>
std::ostream& operator<<(std::ostream& os, const TAccessElems<T>& e)
{
  if (!e.elems().empty())
    os << (e.elems_permit()?"permitted: ":"not permitted: ")
       << GetSQLEnum(e.elems());
  else
    os << (e.elems_permit()?"not permitted all ":"permitted all ");
  return os;
}

class TAccess {
  private:
    TAccessElems<int> _rights;
    TAccessElems<std::string> _airlines;
    TAccessElems<std::string> _airps;
  public:
    TAccess() { clear(); }
    void clear()
    {
      _rights.clear();
      _airlines.clear();
      _airps.clear();
    }
    const TAccessElems<int>& rights() const { return _rights; }
    const TAccessElems<std::string>& airlines() const { return _airlines; }
    const TAccessElems<std::string>& airps() const { return _airps; }
    void merge_airlines(const TAccessElems<std::string> &airlines)
    {
      _airlines.merge(airlines);
    }
    void merge_airps(const TAccessElems<std::string> &airps)
    {
      _airps.merge(airps);
    }
    void set_total_permit()
    {
      _rights.set_total_permit();
      _airlines.set_total_permit();
      _airps.set_total_permit();
    }

    bool totally_not_permitted() const
    {
      return _rights.totally_not_permitted() ||
             _airlines.totally_not_permitted() ||
             _airps.totally_not_permitted();
    }

    bool operator==(const TAccess& access) const
    {
      return _rights==access._rights &&
             _airlines==access._airlines &&
             _airps==access._airps;
    }

    void fromDB(int user_id, TUserType user_type);
    void toXML(xmlNodePtr accessNode);
    void fromXML(xmlNodePtr accessNode);
    static bool profiledRightPermittedForCrsPax(const PaxId_t& paxId, const int right_id);
    static bool profiledRightPermitted(const PointId_t& pointId, const int right_id);
    static bool profiledRightPermitted(const AirportCode_t &airp, const AirlineCode_t &airline, const int right_id);
};

class TUserSettings {
  public:
    TUserSettingType time,disp_airline,disp_airp,disp_craft,disp_suffix;
    TUserSettingType      ckin_airline,ckin_airp,ckin_craft,ckin_suffix;
    TUserSettings()
    {
      clear();
    };
    void clear()
    {
      time=ustTimeLocalDesk;
      disp_airline=ustCodeMixed;
      disp_airp=ustCodeMixed;
      disp_craft=ustCodeMixed;
      disp_suffix=ustEncMixed;
      ckin_airline=ustCodeNative;
      ckin_airp=ustCodeNative;
      ckin_craft=ustCodeNative;
      ckin_suffix=ustEncNative;
    };
};

class TUser {
  public:
    int user_id;
    std::string login;
    std::string descr;
    TUserType user_type;
    TAccess access;
    TUserSettings sets;
    TUser()
    {
      clear();
    };
    void clear()
    {
      user_id=-1;
      login.clear();
      descr.clear();
      user_type=utSupport;
      access.clear();
      sets.clear();
    };
};

class TDesk {
  public:
    static const std::string system_code;

    std::string code;
    std::string city,airp,airline;
    std::string tz_region;
    std::string lang;
    std::string version;
    std::string currency;
    TDateTime time;
    ASTRA::TOperMode mode;
    int grp_id;
    double term_id;
    TDesk()
    {
      clear();
    };
    void clear()
    {
      code.clear();
      city.clear();
      airp.clear();
      airline.clear();
      tz_region.clear();
      lang.clear();
      version.clear();
      currency.clear();
      time = 0;
      mode = ASTRA::omSTAND;
      grp_id = -1;
      term_id = ASTRA::NoExists;
    };
    bool compatible(const std::string &ver);
    static bool isValidVersion(const std::string &ver);
};

class TScreen {
  public:
    int id;
    bool pr_logon;
    std::string name;
    TScreen()
    {
      clear();
    };
    void clear()
    {
      id=0;
      pr_logon = true;
      name.clear();
    };
};

struct TReqInfoInitData {
  std::string screen;
  std::string pult;
  std::string opr;
  std::string mode;
  std::string lang;
  double term_id;
  bool checkUserLogon;
  bool checkCrypt;
  bool pr_web;
  bool duplicate;
  TReqInfoInitData() {
    term_id = ASTRA::NoExists;
    checkUserLogon = false;
    checkCrypt = false;
    pr_web = false;
    duplicate = false;
  }
};

class TReqInfo
{
    private:
        boost::posix_time::ptime execute_time;
        bool vtracing, vtracing_init;
        bool tracing();
  public:
    TUser user;
    TDesk desk;
    TScreen screen;
    ASTRA::TClientType client_type;
    bool duplicate;
    bool api_mode;
    void clear()
    {
      desk.clear();
      user.clear();
      screen.clear();
      client_type = ASTRA::ctTerm;
      duplicate = false;
      vtracing=true;
      vtracing_init=false;
      api_mode = false;
    }
    virtual ~TReqInfo() {}
    static TReqInfo *Instance();
    void Initialize( const std::string &city );
    void Initialize( TReqInfoInitData &InitData );
    void LocaleToLog(const std::string &vlexema, ASTRA::TEventType ev_type, int id1 = ASTRA::NoExists, int id2 = ASTRA::NoExists, int id3 = ASTRA::NoExists);
    void LocaleToLog(const std::string &vlexema, const LEvntPrms &prms, ASTRA::TEventType ev_type, const std::string &sub_type, int id1 = ASTRA::NoExists, int id2 = ASTRA::NoExists, int id3 = ASTRA::NoExists);
    void LocaleToLog(const std::string &vlexema, const LEvntPrms &prms, ASTRA::TEventType ev_type, int id1 = ASTRA::NoExists, int id2 = ASTRA::NoExists, int id3 = ASTRA::NoExists);
    void LocaleToLog(TLogLocale &msg);
    void setPerform();
    void clearPerform();
    long getExecuteMSec();
    bool isSelfCkinClientType() const
    {
      return client_type==ASTRA::ctWeb ||
             client_type==ASTRA::ctKiosk ||
             client_type==ASTRA::ctMobile;
    }

    void traceToMonitor( TRACE_SIGNATURE, const char *format,  ...);
};

ASTRA::TRptType DecodeRptType(const std::string s);
const std::string EncodeRptType(ASTRA::TRptType s);
ASTRA::TClientType DecodeClientType(const char* s);
const char* EncodeClientType(ASTRA::TClientType s);
ASTRA::TClass DecodeClass(const char* s);
const char* EncodeClass(ASTRA::TClass cl);
ASTRA::TPerson DecodePerson(const char* s);
ASTRA::TPerson DecodePerson(const std::string &s);
const char* EncodePerson(ASTRA::TPerson p);
ASTRA::TQueue DecodeQueue(int q);
int EncodeQueue(ASTRA::TQueue q);
ASTRA::TPaxStatus DecodePaxStatus(const char* s);
ASTRA::TPaxStatus DecodePaxStatus(const std::string &s);
const char* EncodePaxStatus(ASTRA::TPaxStatus s);
ASTRA::TCompLayerType DecodeCompLayerType(const char* s);
const char* EncodeCompLayerType(ASTRA::TCompLayerType s);
ASTRA::TBagNormType DecodeBagNormType(const char* s);
const char* EncodeBagNormType(ASTRA::TBagNormType s);

char DecodeStatus(char* s);

#define sign( x ) ( ( x ) > 0 ? 1 : ( x ) < 0 ? -1 : 0 )
TDateTime DecodeTimeFromSignedWord( signed short int Value );
signed short int EncodeTimeToSignedWord( TDateTime Value );

void showBasicInfo(void);

namespace AstraLocale {
std::string getLocaleText(const LexemaData &lexemaData, const std::string &lang = "");
std::string getLocaleText(const std::string &vlexema, const std::string &lang = "");
std::string getLocaleText(const std::string &vlexema, const LParams &aparams, const std::string &lang = "");

void showError(const LexemaData &lexemaData, int code = 0);
void showError(const std::string &lexema_id, int code = 0);
void showError(const std::string &vlexema, const LParams &aparams, int code = 0);

void showErrorMessage(const LexemaData &lexemaData, int code = 0);
void showErrorMessage(const std::string &lexema_id, int code = 0);
void showErrorMessage(const std::string &vlexema, const LParams &aparams, int code = 0);

void showMessage(const LexemaData &lexemaData, int code = 0);
void showMessage(const std::string &lexema_id, int code = 0);
void showMessage(const std::string &vlexema, const LParams &aparams, int code = 0);

void showProgError(const LexemaData &lexemaData, int code = 0);
void showProgError(const std::string &lexema_id, int code = 0);
void showProgError(const std::string &vlexema, const LParams &aparams, int code = 0);

void showErrorMessageAndRollback(const LexemaData &lexemaData, int code = 0 );
void showErrorMessageAndRollback(const std::string &lexema_id, int code = 0);
void showErrorMessageAndRollback(const std::string &vlexema, const LParams &aparams, int code = 0);

std::string getLocaleText(xmlNodePtr lexemeNode);
void LexemeDataToXML(const AstraLocale::LexemaData &lexemeData, xmlNodePtr lexemeNode);
void LexemeDataFromXML(xmlNodePtr lexemeNode, AstraLocale::LexemaData &lexemeData);

void getLexemaText( LexemaData lexemaData, std::string &text, std::string &master_lexema_id, std::string lang = "" );

xmlNodePtr selectPriorityMessage(xmlNodePtr resNode, std::string& error_code, std::string& error_message);

} // end namespace AstraLocale

ASTRA::TOperMode DecodeOperMode( const std::string mode );
std::string EncodeOperMode(const ASTRA::TOperMode mode );
ASTRA::TEventType DecodeEventType( const std::string ev_type );
std::string EncodeEventType( const ASTRA::TEventType ev_type );

const std::string& AirpTZRegion(std::string airp, bool with_exception=true);
const std::string& CityTZRegion(std::string city, bool with_exception=true);
std::string DeskCity(std::string desk, bool with_exception=true);

CityCode_t getCityByAirp(const AirportCode_t& airp);

TCountriesRow getCountryByAirp(const std::string& airp);
CountryCode_t getCountryByAirp(const AirportCode_t& airp);

class SysReqInterface : public JxtInterface
{
public:
  SysReqInterface() : JxtInterface("","SysRequest")
  {
     Handler *evHandle;
     evHandle=JxtHandler<SysReqInterface>::CreateHandler(&SysReqInterface::ClientError);
     AddEvent("ClientError",evHandle);
     evHandle=JxtHandler<SysReqInterface>::CreateHandler(&SysReqInterface::GetBasicInfo);
     AddEvent("GetBasicInfo",evHandle);
  }

  static void ErrorToLog(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ClientError(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetBasicInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode){};
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode){};
};

enum class TranslitFormat: int
{
  V1 = 1,
  V2 = 2,
  V3 = 3
};

std::string transliter(const std::string &value, TranslitFormat fmt, bool pr_lat);
bool transliter_equal(const std::string &value1, const std::string &value2, TranslitFormat fmt);
bool transliter_equal(const std::string &value1, const std::string &value2);
bool transliter_equal_begin(const std::string &str, const std::string &substr, TranslitFormat fmt);
bool transliter_equal_begin(const std::string &str, const std::string &substr);
int best_transliter_similarity(const std::string &value1, const std::string &value2);
std::string convert_char_view(const std::string &value, bool pr_lat);

int getTCLParam(const char* name, int min, int max, int def);
std::string getTCLParam(const char* name, const char* def);
const char* OWN_POINT_ADDR();
const char* SERVER_ID();
int ARX_TRIP_DATE_RANGE();
int ARX_EVENTS_DISABLED();
bool get_test_server();

const char* ORDERS_PATH();
int ORDERS_BLOCK_SIZE();
double ORDERS_MAX_SIZE();
double ORDERS_MAX_TOTAL_SIZE();
int ORDERS_TIMEOUT();

bool DEMO_MODE();

AstraLocale::UserException EOracleError2UserException(std::string& msg);
bool isIgnoredEOracleError(const std::exception& e);
bool clearResponseAndRollbackIfDeadlock(const std::exception& e, xmlNodePtr resNode);

std::string get_internal_msgid_hex();

template <class T>
void MergeSortedRanges(std::vector< std::pair<T,T> > &ranges, const std::pair<T,T> &range)
{
  if (range.first>=range.second)
  {
    std::ostringstream err;
    err << "Wrong range [" << range.first << ", " << range.second << ")";
    throw EXCEPTIONS::Exception("MergeSortedRanges: %s", err.str().c_str());
  };

  if (!ranges.empty())
  {
    std::pair<T,T> &last_range=ranges.back();
    if (range.first<last_range.first)
    {
      std::ostringstream err;
      err << "Not sorted range [" << range.first << ", " << range.second << ")";
      throw EXCEPTIONS::Exception("MergeSortedRanges: %s", err.str().c_str());
    };

    if (range.first<=last_range.second)
    {
      if (range.second>last_range.second) last_range.second=range.second;
    }
    else
      ranges.push_back( range );
  }
  else
    ranges.push_back( range );
}

/////////////////////////////////////////////////////////////////////////////////////////

namespace ASTRA
{
void dumpTable(const std::string& table,
               int loglevel, const char* nick, const char* file, int line);

void commit();
void rollback();

void commitAndCallCommitHooks();
void rollbackAndCallRollbackHooks();

void rollbackSavePax();

void beforeSoftError();
void afterSoftError();

tlgnum_t make_tlgnum(int n);

XMLDoc createXmlDoc(const std::string& xml);
XMLDoc createXmlDoc2(const std::string& xml);

}//namespace ASTRA

struct TRegEvents:public  std::map< std::pair<int, int>, std::pair<TDateTime, TDateTime> > {
    void fromDB(TDateTime part_key, int point_id);
    void fromArxDB(TDateTime part_key, int point_id);
};

struct TEncodedFileStream
{
    std::string codepage;
    std::string filename;
    std::ofstream of;
    TEncodedFileStream(const std::string &acodepage, const std::string &afilename):
        codepage(acodepage),
        filename(afilename)
    {
    }

    void open();
    template<typename T>
        TEncodedFileStream &operator << (const T &val)
    {
        open();
        std::ostringstream buf;
        buf << val;
        of << ConvertCodepage(buf.str(), "CP866", codepage);
        return *this;
    }


    // operator for iomanips (such as endl)
    TEncodedFileStream &operator << (std::ostream &(*os)(std::ostream &));
};

// �����頥� int (����. point_id), �易��� � ������襩 � �᪮��� (src_date) ��⮩.
// ��। get ����室��� ��������� map
struct TNearestDate
{
    std::map<TDateTime, int> sorted_points;
    TDateTime src_date;
    TNearestDate(TDateTime asrc_date);
    int get();
};

template <class T>
typename T::iterator Erase(T &c, typename T::iterator i)
{
  typename T::iterator curr=i;
  ++i;
  c.erase(curr);
  return i;
};

template <typename T>
class Statistic : public std::map<T, int>
{
  public:
    void add(const T &elem)
    {
      (this->insert(std::pair<T, int>(elem, 0)).first)->second++;
    }

    const T frequent(const T &nvl) const
    {
      std::pair<T, int> result=std::make_pair(nvl, 0);
      for(typename std::map<T, int>::const_iterator i=this->begin(); i!=this->end(); ++i)
        if (i->second > result.second) result=*i;
      return result.first;
    }
};

template<class TQueryT>
void longToDB(TQueryT &Qry, const std::string &column_name, const std::string &src,
              bool nullable=false, int len=4000)
{
    if (!src.empty())
    {
      std::string::const_iterator ib,ie;
      ib=src.begin();
      for(int page_no=1;ib<src.end();page_no++)
      {
        ie=ib+len;
        if (ie>src.end()) ie=src.end();
        Qry.SetVariable("page_no", page_no);
        Qry.SetVariable(column_name, std::string(ib,ie));
        Qry.Execute();
        ib=ie;
      };
    }
    else
    {
      if (nullable)
      {
        Qry.SetVariable("page_no", 1);
        Qry.SetVariable(column_name, FNull);
        Qry.Execute();
      };
    }
}

void traceXML(const xmlDocPtr doc);

class TProfiledRights
{
  private:
    std::set<int> items;
    void fromDB(const AirlineCode_t &airline, const AirportCode_t &airp);
  public:

    TProfiledRights(const AirlineCode_t &airline, const AirportCode_t &airp);
    TProfiledRights(const PointId_t& pointId);
    void toXML(xmlNodePtr node) const;
};

bool rus_airp(const std::string &airp);
std::string get_airp_country(const std::string &airp);

std::string getDocMonth(int month, bool pr_lat);
std::string getDocMonth(TDateTime claim_date, bool pr_lat);

bool isDoomedToWait();

namespace ASTRA
{

template <typename KeyT, typename ItemT>
class Cache
{
  private:
    mutable int totalGet=0;
    mutable int totalAdd=0;
    mutable std::map<KeyT, ItemT> items;
  public:
    static std::string traceTitle();
    const ItemT& add(const KeyT& key) const;
    const ItemT& get(const KeyT& key) const
    {
      totalGet++;

      auto i=items.find(key);
      if (i!=items.end()) return i->second;

      totalAdd++;

      return add(key);
    }
    std::string traceTotals() const
    {
      std::ostringstream s;
      s << traceTitle() << " total: get - " << totalGet << ", add - " << totalAdd;
      return s.str();
    }
    void clear()
    {
      totalGet=0;
      totalAdd=0;
      items.clear();
    }

    class NotFound : public EXCEPTIONS::Exception
    {
      public:
        NotFound() : EXCEPTIONS::Exception("%s: not found", traceTitle().c_str()) {}
    };
};

void syncHistory(const std::string& table_name, int id,
                 const std::string& sys_user_descr, const std::string& sys_desk_code);

} //namespace ASTRA

std::optional<int> getDeskGroupByCode(const std::string& desk);

std::optional<char> invalidSymbolInName(const std::string &value,
                                        const bool latinOnly,
                                        const std::string &additionalSymbols=" -");

bool isValidName(const std::string &value, const bool latinOnly, const std::string &additionalSymbols=" -");

bool isValidAirlineName(const std::string &value, const bool latinOnly);


#endif /*_ASTRA_UTILS_H_*/
