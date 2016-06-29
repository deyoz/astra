#ifndef _ASTRA_UTILS_H_
#define _ASTRA_UTILS_H_

#include <string>
#include <map>
#include <set>
#include <vector>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include "astra_consts.h"
#include "astra_locale.h"
#include "astra_locale_adv.h"
#include "basic.h"
#include "exceptions.h"
#include "oralib.h"
#include "jxtlib/JxtInterface.h"
#include "jxtlib/jxt_xml_cont.h"

std::string AlignString(std::string str, int len, std::string align);

struct TLogMsg {
  public:
    BASIC::TDateTime ev_time;
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
    BASIC::TDateTime ev_time;
    int ev_order;
    ASTRA::TEventType ev_type;
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
  void operator = (const BitSet &bs) {
    flags = bs.flags;
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
};

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
    std::string code;
    std::string city,airp,airline;
    std::string tz_region;
    std::string lang;
    std::string version;
    std::string currency;
    BASIC::TDateTime time;
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
    void LocaleToLog(const std::string &vlexema, const LEvntPrms &prms, ASTRA::TEventType ev_type, int id1 = ASTRA::NoExists, int id2 = ASTRA::NoExists, int id3 = ASTRA::NoExists);
    void LocaleToLog(TLogLocale &msg);
    void setPerform();
    void clearPerform();
    long getExecuteMSec();

    void traceToMonitor( TRACE_SIGNATURE, const char *format,  ...);
};

ASTRA::TRptType DecodeRptType(const std::string s);
const std::string EncodeRptType(ASTRA::TRptType s);
ASTRA::TClientType DecodeClientType(const char* s);
const char* EncodeClientType(ASTRA::TClientType s);
ASTRA::TClass DecodeClass(const char* s);
const char* EncodeClass(ASTRA::TClass cl);
ASTRA::TPerson DecodePerson(const char* s);
const char* EncodePerson(ASTRA::TPerson p);
ASTRA::TQueue DecodeQueue(int q);
int EncodeQueue(ASTRA::TQueue q);
ASTRA::TPaxStatus DecodePaxStatus(const char* s);
const char* EncodePaxStatus(ASTRA::TPaxStatus s);
ASTRA::TCompLayerType DecodeCompLayerType(const char* s);
const char* EncodeCompLayerType(ASTRA::TCompLayerType s);
ASTRA::TBagNormType DecodeBagNormType(const char* s);
const char* EncodeBagNormType(ASTRA::TBagNormType s);
ASTRA::TCrewType DecodeCrewType(const char* s);
const char* EncodeCrewType(ASTRA::TCrewType s);

char DecodeStatus(char* s);

#define sign( x ) ( ( x ) > 0 ? 1 : ( x ) < 0 ? -1 : 0 )
BASIC::TDateTime DecodeTimeFromSignedWord( signed short int Value );
signed short int EncodeTimeToSignedWord( BASIC::TDateTime Value );

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

std::string& AirpTZRegion(std::string airp, bool with_exception=true);
std::string& CityTZRegion(std::string city, bool with_exception=true);
std::string DeskCity(std::string desk, bool with_exception=true);

TCountriesRow getCountryByAirp( const std::string& airp);

boost::local_time::tz_database &get_tz_database();
BASIC::TDateTime UTCToLocal(BASIC::TDateTime d, std::string region);
BASIC::TDateTime LocalToUTC(BASIC::TDateTime d, std::string region, int is_dst=ASTRA::NoExists);
BASIC::TDateTime UTCToClient(BASIC::TDateTime d, std::string region);
BASIC::TDateTime ClientToUTC(BASIC::TDateTime d, std::string region, int is_dst=ASTRA::NoExists);

bool is_dst(BASIC::TDateTime d, std::string region);

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

std::string convert_pnr_addr(const std::string &value, bool pr_lat);
std::string transliter(const std::string &value, int fmt, bool pr_lat);
bool transliter_equal(const std::string &value1, const std::string &value2, int fmt);
bool transliter_equal(const std::string &value1, const std::string &value2);
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

std::string& EOracleError2UserException(std::string& msg);

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
}//namespace ASTRA

struct TRegEvents:public  std::map< std::pair<int, int>, std::pair<BASIC::TDateTime, BASIC::TDateTime> > {
    void fromDB(BASIC::TDateTime part_key, int point_id);
};

#endif /*_ASTRA_UTILS_H_*/
