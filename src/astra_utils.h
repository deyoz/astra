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
                 etRefusalType,etSuffix,etClsGrp,etTripType, etCompElemType,
                 etGrpStatusType,etClientType,etCompLayerType,etCrs,
                 etDevModel,etDevSessType,etDevFmtType,etDevOperType };
enum TElemContext { ecDisp, ecCkin, ecTrfer, ecTlgTypeB, ecNone };
//форматы:
//  fmt=0 вн.код (рус. кодировка)
//  fmt=1 IATA код (лат. кодировка)
//  fmt=2 код ИКАО вн.
//  fmt=3 код ИKAO IATA
//  fmt=4 код ISO
std::string ElemToElemId(TElemType type, std::string code, int &fmt, bool with_deleted=false);


//1. пытается найти по id значение code, если не найдено, возвращается id
//   Если язык не русский то fmt приводится соответственно к 1 и 3
std::string ElemIdToElem(TElemType type, std::string id, int fmt, bool with_deleted=true);
//2. вызывается 1. с параметром fmt=0, with_deleted-по умолчанию=true */
std::string ElemIdToElem(TElemType type, std::string id);
// внимание 3+4 было объединено в одну функцию
//3. пытается найти по id значение name, если не найдено, возвращается id
std::string ElemIdToElemName(TElemType type, std::string id);
//>>>std::string ElemIdToElemShortName(TElemType type, int id);
//4. пытается найти по id значение short_name, если не найдено, возвращается id
std::string ElemIdToElemShortName(TElemType type, std::string id);


std::string ElemIdToElem(TElemType type, int id, int fmt, bool with_deleted=true);

//перекодировки в контексте
std::string ElemCtxtToElemId(TElemContext ctxt,TElemType type, std::string code,
                              int &fmt, bool hard_verify, bool with_deleted=false);
std::string ElemIdToElemCtxt( TElemContext ctxt,TElemType type, std::string id,
                             int fmt, bool with_deleted=true);

std::string ElemIdToElem(TElemType type, std::string id, int fmt, int only_lat, bool with_deleted=true);

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
      ckin_suffix=ustCodeNative;
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
    bool compatible(std::string ver);
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

struct TReqInfoInitData {
  std::string screen;
  std::string pult;
  std::string opr;
  std::string mode;
  std::string lang;
  bool checkUserLogon;
  bool checkCrypt;
  bool pr_web;
  TReqInfoInitData() {
  	checkUserLogon = false;
  	checkCrypt = false;
  	pr_web = false;
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
    void clear()
    {
      desk.clear();
      user.clear();
      screen.clear();
      client_type = ASTRA::ctTerm;
    };
    virtual ~TReqInfo() {}
    static TReqInfo *Instance();
    void Initialize( const std::string &city );
    void Initialize( TReqInfoInitData &InitData );
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

std::string GetSQLEnum(const std::vector<std::string> &values);
void MsgToLog(TLogMsg &msg,
              const std::string &screen,
              const std::string &user,
              const std::string &desk);
void MsgToLog(std::string msg,
              ASTRA::TEventType ev_type,
              int id1 = 0,
              int id2 = 0,
              int id3 = 0);

ASTRA::TRptType DecodeRptType(const std::string s);
const std::string EncodeRptType(ASTRA::TRptType s);
ASTRA::TClientType DecodeClientType(const char* s);
const char* EncodeClientType(ASTRA::TClientType s);
ASTRA::TDocType DecodeDocType(const char* s);
const char* EncodeDocType(ASTRA::TDocType doc);
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

namespace ASTRA {
void showProgError(const std::string &message, int code = 0  );
void showError(const std::string &message, int code = 0 );
void showErrorMessage( const std::string &message, int code = 0 );
void showMessage( const std::string &message, int code = 0  );
void showErrorMessageAndRollback(const std::string &message, int code = 0  );
}
void showBasicInfo(void);

namespace AstraLocale {
void showError(LexemaData lexemaData, int code = 0);
void showError(const std::string &lexema_id, int code = 0);
void showErrorMessage(LexemaData lexemaData, int code = 0);
void showErrorMessage(const std::string &lexema_id, int code = 0);
void showErrorMessage( std::string vlexema, LParams &aparams, int code = 0);
void showProgError(LexemaData lexemaData, int code = 0);
void showProgError(const std::string &lexema_id, int code = 0);
void showErrorMessageAndRollback(const std::string &lexema_id, int code = 0 );
void showErrorMessageAndRollback(LexemaData lexemaData, int code = 0 );
void showMessage( const std::string &lexema_id, int code = 0 );
void showMessage( LexemaData lexemaData, int code = 0);
void showMessage( std::string vlexema, LParams &aparams, int code = 0);
std::string getLocaleText(LexemaData lexemaData);
std::string getLocaleText(const std::string &vlexema, std::string lang = "");
std::string getLocaleText(const std::string &vlexema, LParams &aparams, std::string lang = "");
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
     evHandle=JxtHandler<SysReqInterface>::CreateHandler(&SysReqInterface::ErrorToLog);
     AddEvent("ClientError",evHandle);
     evHandle=JxtHandler<SysReqInterface>::CreateHandler(&SysReqInterface::GetBasicInfo);
     AddEvent("GetBasicInfo",evHandle);
  };

  void ErrorToLog(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetBasicInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode){};
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode){};
};

class UserException2:public AstraLocale::UserException
{
  public:
    UserException2(): UserException(""){};
};

std::string convert_pnr_addr(const std::string &value, bool pr_lat);
std::string convert_suffix(const std::string &value, bool pr_lat);
std::string transliter(const std::string &value, int fmt, bool pr_lat);
bool transliter_equal(const std::string &value1, const std::string &value2, int fmt);
bool transliter_equal(const std::string &value1, const std::string &value2);
bool is_lat(const std::string &value);

int getTCLParam(const char* name, int min, int max, int def);
const char* OWN_POINT_ADDR();
const char* SERVER_ID();
const bool USE_SEANCES();
bool get_test_server();

std::string& EOracleError2UserException(std::string& msg);

std::string get_internal_msgid_hex();

#endif /*_ASTRA_UTILS_H_*/
