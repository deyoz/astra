#ifndef APPS_INTERACTION_H
#define APPS_INTERACTION_H

#include <string>
#include <vector>
#include <iostream>
#include "passenger.h"
#include "checkin.h"
#include "points.h"
#include "date_time.h"
#include "qrys.h"
#include "trip_tasks.h"
#include "counters.h"

const int NumSendAttempts = 5; // количество попыток до включения тревоги "Нет связи с APPS"
const int MaxSendAttempts = 99; // максимальное количество попыток

const std::string APPS_FORMAT_21 = "APPS_21";
const std::string APPS_FORMAT_26 = "APPS_26";
const int APPS_VERSION_21 = 21;
const int APPS_VERSION_26 = 26;
const int APPS_VERSION_27 = 27;

enum APPSAction { NoAction, NeedUpdate, NeedNew, NeedCancel };

void processPax( const int pax_id, Timing::Points& timing, const std::string& override_type = "", const bool is_forced = false );
void APPSFlightCloseout( const int point_id );
std::string getAnsText( const std::string& tlg );
bool processReply( const std::string& source_raw );
bool checkAPPSSets(const int point_dep, const int point_arv );
bool checkAPPSSets( const int point_dep, const std::string& airp_arv, std::set<std::string>* pFormats = nullptr );
bool checkAPPSSets( const int point_dep, const std::string& airp_arv, bool& transit, std::set<std::string>* pFormats = nullptr );
bool checkTime( const int point_id );
bool checkTime( const int point_id, TDateTime& start_time );
std::string emulateAnswer( const std::string& request );
bool IsAPPSAnswText( const std::string& tlg_body );
std::set<std::string> needFltCloseout( const std::set<std::string>& countries, const std::string airline );
void sendAllAPPSInfo(const TTripTaskKey &task);
void sendNewAPPSInfo(const TTripTaskKey &task);
void reSendMsg( const int send_attempts, const std::string& msg_text, const int msg_id );
void deleteMsg( const int msg_id );
void deleteAPPSData( const int pax_id );
void deleteAPPSAlarms( const int pax_id );
const char* getAPPSRotName();

int test_apps_tlg(int argc, char **argv);

struct TAppsSets
{
  TAppsSets(const std::string& airline, const std::string& country);
  bool get_country();
  bool get_inbound_outbound(int& inbound, int& outbound);
  bool get_flt_closeout(int& flt_closeout);
  std::string get_format();
  int get_version();
private:
  TAppsSets();
  void init_qry(TQuery& AppsSetsQry, const std::string& select_string);
  std::string _airline;
  std::string _country;
};

struct TTransData
{
  int msg_id;
  std::string code;
  std::string user_id;
  bool type; // '' = Check-in Transaction. "P" = Pre-Check-in Vetting
  int version = 0;

  TTransData() : type( false ) {}
  void init( const bool pre_ckin, const std::string& trans_code, const std::string& id, int ver );
  bool operator == ( const TTransData& data ) const
  {
    return type == data.type &&
           user_id == data.user_id &&
           version == data.version;
  }
  void check_data() const;
  std::string msg() const;
};

struct TFlightData
{
  std::string type;
  int point_id;
  std::string flt_num; // Airline and flight number.
  std::string port; // IATA Airport Code
  TDateTime date;
  std::string arv_port;  // IATA Airport Code
  TDateTime arv_date;
  int version = 0;

  TFlightData() : point_id( ASTRA::NoExists ), date( ASTRA::NoExists ),
                  arv_date( ASTRA::NoExists ) {}
  void init( const int point_id, const std::string& type, int ver );
  void init( const int point_id, const std::string& type, const std::string& num, const std::string& airp, const std::string& arv_airp,
             TDateTime dep, TDateTime arv, int ver );
  bool operator == ( const TFlightData& data ) const
  {
    return type == data.type &&
           flt_num == data.flt_num &&
           port == data.port &&
           date == data.date &&
           arv_port == data.arv_port &&
           arv_date == data.arv_date &&
           version == data.version;
  }
  void check_data() const;
  std::string msg() const;
};

struct TPaxData
{
  int pax_id;
  std::string apps_pax_id; // только для "PCX". Уникальный ID пассажира. Получаем в ответ на CIRQ запрос
  std::string status;
  std::string pax_crew; // 'C' или 'P'
  std::string nationality;
  std::string issuing_state; // optional для "PCX"
  std::string passport;
  std::string check_char; // optional
  std::string doc_type; // optional, 'P' - пасспорт, 'O' - другой документ, 'N' - нет документа
  std::string doc_subtype;
  std::string expiry_date; //CCYYMMDD
  std::string sup_doc_type; // в настоящее время не используется
  std::string sup_passport; // в настоящее время не используется
  std::string sup_check_char; // в настоящее время не используется
  std::string family_name; // от 2 до 24 символов
  std::string given_names; // до 24 символов, если нет - '-'.
  std::string birth_date; // CCYYMMDD. If the day is not known, use CCYYMM00
  std::string sex; // "M" = Male, "F" = Female, "U" = Unspecified, "X" = Unspecified
  std::string birth_country; // optional
  std::string endorsee; // Blank for passport holder, "S" = Endorsee. Optional
  std::string trfer_at_origin; // optional
  std::string trfer_at_dest; // optional
  std::string override_codes; /* Optional. Format is: CxxCxxCxxCxxCxx,
  where "C" = Override code ('A' = airline override, 'G' = government override),
  "xx" = IATA country code */
  std::string pnr_source; // conditional
  std::string pnr_locator; // conditional
  std::string reference; // conditional, version 24
  int version = 0;

  TPaxData () : pax_id(ASTRA::NoExists) {}
  void init( const int pax_id, const std::string& surname, const std::string name,
             const bool is_crew, const int transfer, const std::string& override_type,
             const int reg_no, const ASTRA::TTrickyGender::Enum tricky_gender, int ver );
  void init( TQuery &Qry, int ver );
  bool operator == ( const TPaxData& data ) const
  {
    return pax_id == data.pax_id &&
        pax_crew == data.pax_crew &&
        nationality == data.nationality &&
        issuing_state == data.issuing_state &&
        passport == data.passport &&
        check_char == data.check_char &&
        expiry_date == data.expiry_date &&
        sup_check_char == data.sup_check_char &&
        sup_doc_type == data.sup_doc_type &&
        sup_passport == data.sup_passport &&
        family_name == data.family_name &&
        given_names == data.given_names &&
        birth_date == data.birth_date &&
        sex == data.sex &&
        birth_country == data.birth_country &&
        endorsee == data.endorsee &&
        trfer_at_origin == data.trfer_at_origin &&
        trfer_at_dest == data.trfer_at_dest &&
        pnr_source == data.pnr_source &&
        pnr_locator == data.pnr_locator &&
        reference == data.reference &&
        version == data.version;
  }
  void check_data() const;
  std::string msg() const;
};

struct TPaxAddData // Passenger Additional Data
{
  int version = APPS_VERSION_26;
  std::string country_for_data; // Country for Additional Data // 4
  std::string doco_type; // Document Type // 5
  std::string doco_no; // Document Number X(20) // 6
  std::string country_issuance; // Country of Issuance // 7
  std::string doco_expiry_date; // Expiration Date CCYYMMDD // 8
  std::string num_street; // Address: Number and Street // 9
  std::string city; // Address: City // 10
  std::string state; // Address: State // 11
  std::string postal_code; // Address: Postal Code // 12
  std::string redress_number; // Passenger Redress Number // 13 // omit
  std::string traveller_number; // Known Traveller Number // 14 // omit

  void init( const int pax_id, const int ver );
  void init( TQuery &Qry, int ver );
  bool operator == (const TPaxAddData& d) const
  {
    return version == d.version &&
      country_for_data == d.country_for_data &&
      doco_type == d.doco_type &&
      doco_no == d.doco_no &&
      country_issuance == d.country_issuance &&
      doco_expiry_date == d.doco_expiry_date &&
      num_street == d.num_street &&
      city == d.city &&
      state == d.state &&
      postal_code == d.postal_code &&
      redress_number == d.redress_number &&
      traveller_number == d.traveller_number;
  }
  void check_data() const;
  std::string msg() const;
};

struct TMftData
{
  std::string country; // 2-character IATA Country Code of the participating country.
  std::string mft_pax; // The only option supported for UAE will be "C".
  std::string mft_crew; // The only option supported for UAE will be "C".
  int version = 0;

  void init( const std::string& code, int ver )
  {
    country = code;
    mft_pax = "C";
    mft_crew = "C";
    version = ver;
  }
  void check_data() const;
  std::string msg() const;
};

class TPaxRequest
{
  std::string header;
  TTransData trans;
  TFlightData int_flt;
  TFlightData ckin_flt;
  TPaxData pax;
  TPaxAddData pax_add;
  int version = 0;
  bool getByPaxId( const int pax_id, Timing::Points& timing, const std::string& override_type );
  bool getByCrsPaxId( const int pax_id, Timing::Points& timing, const std::string& override_type );
  void saveData() const;

public:
  void init( const int pax_id, Timing::Points& timing, const std::string& override_type = "" );
  bool fromDBByPaxId( const int pax_id );
  bool fromDBByMsgId( const int msg_id );
  bool operator == (const TPaxRequest &c) const
  {
    return trans == c.trans &&
           ckin_flt == c.ckin_flt &&
           int_flt == c.int_flt &&
           pax == c.pax &&
           version == c.version;
  }
  bool operator != (const TPaxRequest &c) const
  {
    return !(*this == c);
  }
  APPSAction typeOfAction( const bool is_exists, const std::string& status,
                           const bool is_the_same, const bool is_forced ) const;
  std::string msg() const;
  void sendReq(Timing::Points& timing) const;
  std::string getStatus() const {
    return pax.status;
  }
};

class TManifestRequest
{
  std::string header;
  TTransData trans;
  TFlightData int_flt;
  TMftData mft_req;
  int version = 0;
public:
  bool init( const int point_id, const std::string& country_lat, const std::string& country_code );
  std::string msg() const;
  void sendReq() const;
};

struct TAnsPaxData
{
  std::string country; // 2-character IATA Country Code of the country generating the response.
  int code; /* Статусы для CIRS: 8501 OK to Board, 8502 Do Not Board, 8503 Board if Docs OK, 8517 Override Accepted, 8630 Contact UAE govt.
  Статусы для CIСС: 8505 Cancelled, 8506 No Record */
  std::string status; /* Статусы для CIRS: "B" = OK to Board, "D" = Do Not Board (override is possible), "X" = Do Not Board (override is impossible),
  "U" = Unable to determine status, "I" = Not enought data, "T" = Timeout, "E" = Error.
  Статусы для CIСС: "C" = Cancelled, "N" = Not found, "T" = Timeout, "E" = Error. */
  std::string apps_pax_id;
  int error_code1; // Conditional (on error condition)
  std::string error_text1; // Conditional (on error condition)
  int error_code2; // Conditional (on error condition)
  std::string error_text2; // Conditional (on error condition)
  int error_code3; // Conditional (on error condition)
  std::string error_text3; // Conditional (on error condition)
  int version = 0;

  TAnsPaxData() : code(ASTRA::NoExists), error_code1(ASTRA::NoExists),
                  error_code2(ASTRA::NoExists), error_code3(ASTRA::NoExists) {}
  std::string toString() const;
  void init( std::string source, int ver );
};

struct TError
{
  std::string country; /* 2-character IATA Country Code of the country generating the response.
  The field may be null if the error is not specific to any country. */
  int error_code; // Conditional (on error condition)
  std::string error_text; // Conditional (on error condition)

  TError() : error_code(ASTRA::NoExists) {}
};

class TAPPSAns
{
protected:
  std::string code;
  int msg_id;
  int send_attempts;
  std::string msg_text;
  int point_id;
  std::vector<TError> errors; // Error (repeating). Conditional
  int version = 0;

  bool CheckIfNeedResend() const;
  virtual void getLogParams( LEvntPrms& params, const std::string& country, const int status_code,
                  const int error_code, const std::string& error_text ) const;
public:
  TAPPSAns() : send_attempts(ASTRA::NoExists), point_id(ASTRA::NoExists) {}
  virtual ~TAPPSAns() {}
  virtual bool init( const std::string& code, const std::string& source );
  virtual void beforeProcessAnswer() const;
  virtual std::string toString() const;
  virtual void processErrors() const = 0;
  virtual void processAnswer() const = 0;
  virtual void logAnswer( const std::string& country, const int status_code,
                  const int error_code, const std::string& error_text ) const = 0;
};

class TPaxReqAnswer : public TAPPSAns
{
  int pax_id;
  std::string family_name;
  // PassengerData. Applicable to "CIRS" and "CIСС". Conditional (required if ERR data group is not present)
  std::vector<TAnsPaxData> passengers;
public:
  TPaxReqAnswer() : TAPPSAns(), pax_id(ASTRA::NoExists) {}
  virtual ~TPaxReqAnswer() {}
  virtual bool init( const std::string& code, const std::string& source );
  virtual void processErrors() const;
  virtual void processAnswer() const;
  virtual std::string toString() const;
  virtual void logAnswer( const std::string& country, const int status_code,
                  const int error_code, const std::string& error_text ) const;
};

class TMftAnswer : public TAPPSAns
{
  // Manifest response. Applicable to "CIMA". Conditional (required if ERR data group is not present)
  std::string country; // 2-character IATA Country Code of the participating country.
  int resp_code; // 8700 = Request processed, 8701 = Request rejected
  int error_code; // Conditional (on error condition)
  std::string error_text; // Conditional (on error condition)

public:
  TMftAnswer() : TAPPSAns(), resp_code(ASTRA::NoExists), error_code(ASTRA::NoExists) {}
  virtual ~TMftAnswer() {}
  virtual bool init( const std::string& code, const std::string& source );
  virtual void processErrors() const;
  virtual void processAnswer() const;
  virtual std::string toString() const;
  virtual void logAnswer( const std::string& country, const int status_code,
                  const int error_code, const std::string& error_text ) const;
};

#endif // APPS_INTERACTION_H
