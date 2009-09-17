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
#include "oralib.h"
#include "jxtlib/JxtInterface.h"
#include "jxtlib/jxt_xml_cont.h"

std::string AlignString(std::string str, int len, std::string align);

struct TLogMsg {
  std::string msg;
  ASTRA::TEventType ev_type;
  int id1,id2,id3;
  TLogMsg() {
    Clear();
  };
  void Clear() {
    ev_type = ASTRA::evtUnknown;
    msg = "";
    id1 = 0;
    id2 = 0;
    id3 = 0;
  };
};

enum TUserType { utSupport=0, utAirport=1, utAirline=2 };
enum TUserSettingType { ustTimeUTC=0, ustTimeLocalDesk=1, ustTimeLocalAirp=2,
                        ustCodeNative=5, ustCodeIATA=6,
                        ustCodeNativeICAO=7, ustCodeIATAICAO=8, ustCodeMixed=9,
                        ustEncNative=15, ustEncLatin=16, ustEncMixed=17 };

enum TElemType { etCountry,etCity,etAirline,etAirp,etCraft,etClass,etSubcls,
                 etPersType,etGenderType,etPaxDocType,etPayType,etCurrency,
                 etSuffix,etClsGrp };
enum TElemContext { ecDisp, ecCkin, ecTrfer, ecTlgTypeB };
//форматы:
//  fmt=0 вн.код (рус. кодировка)
//  fmt=1 IATA код (лат. кодировка)
//  fmt=2 код ИКАО вн.
//  fmt=3 код ИKAO IATA
//  fmt=4 код ISO
std::string ElemToElemId(TElemType type, std::string code, int &fmt, bool with_deleted=false);
std::string ElemIdToElem(TElemType type, int id, int fmt, bool with_deleted=true);
std::string ElemIdToElem(TElemType type, std::string id, int fmt, bool with_deleted=true);
std::string ElemCtxtToElemId(TElemContext ctxt,TElemType type, std::string code,
                              int &fmt, bool hard_verify, bool with_deleted=false);
std::string ElemIdToElemCtxt( TElemContext ctxt,TElemType type, std::string id,
                             int fmt, bool with_deleted=true);

template <class T>
class BitSet
{
  std::map<T,bool> flags; /* то что надо вытащить из БД */
  public:
  void setFlag( T key ) {
    flags[ key ] = true;
  }
  bool isFlag( T key ) {
   typename std::map<T,bool>::iterator pos = flags.find( key );
   if ( pos == flags.end() )
     return false;
   return flags[ key ];
  }
  void clearFlags( ) {
    flags.clear();
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
    BASIC::TDateTime time;
    ASTRA::TOperMode mode;
    int grp_id;
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
      time = 0;
      mode = ASTRA::omSTAND;
      grp_id = -1;
    };
};

class TScreen {
  public:
    int id;
    int version;
    bool pr_logon;
    std::string name;
    TScreen()
    {
      clear();
    };
    void clear()
    {
      id=0;
      version=0;
      pr_logon = true;
      name.clear();
    };
};

class TReqInfo
{
	private:
		boost::posix_time::ptime execute_time;
  public:
    TUser user;
    TDesk desk;
    TScreen screen;
    void clear()
    {
      desk.clear();
      user.clear();
      screen.clear();
    };
    virtual ~TReqInfo() {}
    static TReqInfo *Instance();
    void Initialize( const std::string &city );
    void Initialize( const std::string &vscreen, const std::string &vpult, const std::string &vopr,
                     const std::string &vmode, bool checkUserLogon );
    void MsgToLog(TLogMsg &msg);
    void MsgToLog(std::string msg, ASTRA::TEventType ev_type, int id1, int id2, int id3);
    void MsgToLog(std::string msg, ASTRA::TEventType ev_type) {
      MsgToLog(msg, ev_type,0,0,0);
    }
    void MsgToLog(std::string msg, ASTRA::TEventType ev_type, int id1) {
      MsgToLog(msg, ev_type,id1,0,0);
    }
    void MsgToLog(std::string msg, ASTRA::TEventType ev_type, int id1, int id2) {
      MsgToLog(msg, ev_type,id1,id2,0);
    }
    void setPerform();
    void clearPerform();
    long getExecuteMSec();

    void MergeAccess(std::vector<std::string> &a, bool &ap,
                     std::vector<std::string> b, bool bp);
    bool CheckAirline(const std::string &airline);
    bool CheckAirp(const std::string &airp);
};

std::string GetSQLEnum(std::vector<std::string> &values);
void MsgToLog(TLogMsg &msg,
              const std::string &screen,
              const std::string &user,
              const std::string &desk);
void MsgToLog(std::string msg,
              ASTRA::TEventType ev_type,
              int id1 = 0,
              int id2 = 0,
              int id3 = 0);

ASTRA::TDocType DecodeDocType(const char* s);
char* EncodeDocType(ASTRA::TDocType doc);
ASTRA::TClass DecodeClass(const char* s);
char* EncodeClass(ASTRA::TClass cl);
ASTRA::TPerson DecodePerson(const char* s);
char* EncodePerson(ASTRA::TPerson p);
ASTRA::TQueue DecodeQueue(int q);
int EncodeQueue(ASTRA::TQueue q);
ASTRA::TPaxStatus DecodePaxStatus(const char* s);
char* EncodePaxStatus(ASTRA::TPaxStatus s);
ASTRA::TCompLayerType DecodeCompLayerType(const char* s);
char* EncodeCompLayerType(ASTRA::TCompLayerType s);

char DecodeStatus(char* s);

#define sign( x ) ( ( x ) > 0 ? 1 : ( x ) < 0 ? -1 : 0 )
BASIC::TDateTime DecodeTimeFromSignedWord( signed short int Value );
signed short int EncodeTimeToSignedWord( BASIC::TDateTime Value );

void showProgError(const std::string &message );
void showError(const std::string &message, int code = 0 );
void showErrorMessage( const std::string &message, bool pr_dialog=false );
void showMessage( const std::string &message );
void showErrorMessageAndRollback(const std::string &message );
void showBasicInfo(void);



ASTRA::TOperMode DecodeOperMode( const std::string mode );
std::string EncodeOperMode(const ASTRA::TOperMode mode );
ASTRA::TEventType DecodeEventType( const std::string ev_type );
std::string EncodeEventType( const ASTRA::TEventType ev_type );

std::string& AirpTZRegion(std::string airp, bool with_exception=true);
std::string& CityTZRegion(std::string city, bool with_exception=true);
std::string DeskCity(std::string desk, bool with_exception=true);

boost::local_time::tz_database &get_tz_database();
BASIC::TDateTime UTCToLocal(BASIC::TDateTime d, std::string region);
BASIC::TDateTime LocalToUTC(BASIC::TDateTime d, std::string region);
BASIC::TDateTime UTCToClient(BASIC::TDateTime d, std::string region);
BASIC::TDateTime ClientToUTC(BASIC::TDateTime d, std::string region);

bool is_dst(BASIC::TDateTime d, std::string region);

class SysReqInterface : public JxtInterface
{
public:
  SysReqInterface() : JxtInterface("","SysRequest")
  {
     Handler *evHandle;
     evHandle=JxtHandler<SysReqInterface>::CreateHandler(&SysReqInterface::ErrorToLog);
     AddEvent("ClientError",evHandle);
     evHandle=JxtHandler<SysReqInterface>::CreateHandler(&SysReqInterface::GetBasicInfo);
     AddEvent("GetBasicInfo",evHandle);
  };

  void ErrorToLog(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetBasicInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode){};
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode){};
};

class UserException2:public EXCEPTIONS::UserException
{
  public:
    UserException2(): UserException(""){};
};

std::string convert_pnr_addr(const std::string &value, bool pr_lat);
std::string convert_suffix(const std::string &value, bool pr_lat);
std::string transliter(const std::string &value, bool pr_lat);
bool is_lat(const std::string &value);
bool get_test_server();

std::string& EOracleError2UserException(std::string& msg);

#endif /*_ASTRA_UTILS_H_*/
