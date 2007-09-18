#ifndef _ASTRA_UTILS_H_
#define _ASTRA_UTILS_H_

#include <string>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include "astra_consts.h"
#include "basic.h"
#include "exceptions.h"
#include "oralib.h"
#include <map>
#include "jxt_xml_cont.h"
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "JxtInterface.h"

std::string AlignString(std::string str, int len, std::string align);

struct TLogMsg {
  std::string msg;
  ASTRA::TEventType ev_type;
  int id1,id2,id3;
  TLogMsg() {
    ev_type = ASTRA::evtUnknown;
    msg = "";
    id1 = 0;
    id2 = 0;
    id3 = 0;
  }
};

enum TUserType { utSupport=0, utAirport=1, utAirline=2 };
enum TTimeForm { tfUTC, tfLocalDesk, tfLocalAll, tfUnknown };

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
    void clear();
};

class TUser {
  public:
    int user_id;
    std::string login;
    std::string descr;
    TUserType user_type;
    TAccess access;
    TTimeForm time_form;
    TUser();
    void clear();
};

class TDesk {
  public:
    std::string code;
    std::string city;
    std::string tz_region;
    BASIC::TDateTime time;
    TDesk();
    void clear();
};

class TScreen {
  public:
    int id;
    int version;
    std::string name;
    TScreen();
    void clear();
};

class TReqInfo
{
	private:
		boost::posix_time::ptime execute_time;
  public:
    void clear();
    TReqInfo();
    virtual ~TReqInfo() {}
    TUser user;
    TDesk desk;
    TScreen screen;
    static TReqInfo *Instance();
    void Initialize( const std::string &vscreen, const std::string &vpult, const std::string &vopr,
                     bool checkUserLogon );
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
};

ASTRA::TDocType DecodeDocType(char* s);
char* EncodeDocType(ASTRA::TDocType doc);
ASTRA::TClass DecodeClass(char* s);
char* EncodeClass(ASTRA::TClass cl);
ASTRA::TPerson DecodePerson(char* s);
char* EncodePerson(ASTRA::TPerson p);
ASTRA::TQueue DecodeQueue(int q);
int EncodeQueue(ASTRA::TQueue q);
ASTRA::TPaxStatus DecodePaxStatus(char* s);
char* EncodePaxStatus(ASTRA::TPaxStatus s);


char DecodeStatus(char* s);


#define sign( x ) ( ( x ) > 0 ? 1 : ( x ) < 0 ? -1 : 0 )
BASIC::TDateTime DecodeTimeFromSignedWord( signed short int Value );
signed short int EncodeTimeToSignedWord( BASIC::TDateTime Value );

void showProgError(const std::string &message );
void showError(const std::string &message, int code = 0 );
void showErrorMessage( const std::string &message );
void showMessage( const std::string &message );
void showErrorMessageAndRollback(const std::string &message );
void showBasicInfo(void);

void MsgToLog(std::string msg, ASTRA::TEventType ev_type,
        int id1 = 0,
        int id2 = 0,
        int id3 = 0);
void MsgToLog(TLogMsg &msg);

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

std::string convert_seat_no(const std::string &value, bool pr_lat);
std::string convert_pnr_addr(const std::string &value, bool pr_lat);
std::string convert_suffix(const std::string &value, bool pr_lat);
std::string transliter(const std::string &value, bool pr_lat);

#endif /*_ASTRA_UTILS_H_*/
