#ifndef _ASTRA_UTILS_H_
#define _ASTRA_UTILS_H_

#include <string>
#include <map>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include "astra_consts.h"
#include "basic.h"
#include "exceptions.h"
#include "astra_locale.h"
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

enum TUserType { utSupport=0, utAirport=1, utAirline=2 };
enum TUserSettingType { ustTimeUTC=0, ustTimeLocalDesk=1, ustTimeLocalAirp=2,
                        ustCodeNative=5, ustCodeInter=6,
                        ustCodeICAONative=7, ustCodeICAOInter=8, ustCodeMixed=9,
                        ustEncNative=15, ustEncLatin=16, ustEncMixed=17 };

template <class T>
class BitSet
{
  std::map<T,bool> flags; /* то что надо вытащить из БД */
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

class TAccess {
  public:
    std::vector<int> rights;
    std::vector<std::string> airlines;
    std::vector<std::string> airps;
    bool airlines_permit,airps_permit;
    void clear()
    {
      rights.clear();
      airlines.clear();
      airps.clear();
      airlines_permit=true;
      airps_permit=true;
    };
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
  public:
    TUser user;
    TDesk desk;
    TScreen screen;
    ASTRA::TClientType client_type;
    bool duplicate;
    void clear()
    {
      desk.clear();
      user.clear();
      screen.clear();
      client_type = ASTRA::ctTerm;
      duplicate = false;
    };
    virtual ~TReqInfo() {}
    static TReqInfo *Instance();
    void Initialize( const std::string &city );
    void Initialize( TReqInfoInitData &InitData );
    void MsgToLog(TLogMsg &msg);
    void MsgToLog(std::string msg, ASTRA::TEventType ev_type, int id1, int id2, int id3);
    void MsgToLog(std::string msg, ASTRA::TEventType ev_type) {
      MsgToLog(msg, ev_type, ASTRA::NoExists, ASTRA::NoExists, ASTRA::NoExists);
    }
    void MsgToLog(std::string msg, ASTRA::TEventType ev_type, int id1) {
      MsgToLog(msg, ev_type, id1, ASTRA::NoExists, ASTRA::NoExists);
    }
    void MsgToLog(std::string msg, ASTRA::TEventType ev_type, int id1, int id2) {
      MsgToLog(msg, ev_type, id1, id2, ASTRA::NoExists);
    }
    void setPerform();
    void clearPerform();
    long getExecuteMSec();

    bool CheckAirline(const std::string &airline);
    bool CheckAirp(const std::string &airp);
};

void MergeAccess(std::vector<std::string> &a, bool &ap,
                 std::vector<std::string> b, bool bp);

void MsgToLog(TLogMsg &msg,
              const std::string &screen,
              const std::string &user,
              const std::string &desk);
void MsgToLog(std::string msg,
              ASTRA::TEventType ev_type,
              int id1 = ASTRA::NoExists,
              int id2 = ASTRA::NoExists,
              int id3 = ASTRA::NoExists);

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

char DecodeStatus(char* s);

#define sign( x ) ( ( x ) > 0 ? 1 : ( x ) < 0 ? -1 : 0 )
BASIC::TDateTime DecodeTimeFromSignedWord( signed short int Value );
signed short int EncodeTimeToSignedWord( BASIC::TDateTime Value );
BASIC::TDateTime JulianDateToDateTime( int jdate, int year);

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
} // end namespace astraLocale



ASTRA::TOperMode DecodeOperMode( const std::string mode );
std::string EncodeOperMode(const ASTRA::TOperMode mode );
ASTRA::TEventType DecodeEventType( const std::string ev_type );
std::string EncodeEventType( const ASTRA::TEventType ev_type );

std::string& AirpTZRegion(std::string airp, bool with_exception=true);
std::string& CityTZRegion(std::string city, bool with_exception=true);
std::string DeskCity(std::string desk, bool with_exception=true);

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
  };

  static void ErrorToLog(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ClientError(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetBasicInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode){};
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode){};
};

class UserException2:public AstraLocale::UserException
{
  public:
    UserException2(): UserException(""){};
};

std::string convert_pnr_addr(const std::string &value, bool pr_lat);
std::string transliter(const std::string &value, int fmt, bool pr_lat);
bool transliter_equal(const std::string &value1, const std::string &value2, int fmt);
bool transliter_equal(const std::string &value1, const std::string &value2);
std::string convert_char_view(const std::string &value, bool pr_lat);

int getTCLParam(const char* name, int min, int max, int def);
std::string getTCLParam(const char* name, const char* def);
const char* OWN_POINT_ADDR();
const char* SERVER_ID();
const bool USE_SEANCES();
const int ARX_TRIP_DATE_RANGE();
bool get_test_server();

std::string& EOracleError2UserException(std::string& msg);

std::string get_internal_msgid_hex();

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
};

namespace ASTRA
{
void commit();
void rollback();
};

#endif /*_ASTRA_UTILS_H_*/
