#ifndef APPS_INTERACTION_H
#define APPS_INTERACTION_H

#include <string>
#include <vector>
#include <iostream>
#include "passenger.h"
#include "checkin.h"
#include "points.h"

const int NumSendAttempts = 5; // количество попыток до включения тревоги "Нет связи с APPS"
const int MaxSendAttempts = 99; // максимальное количество попыток

const char* const APPSAddr = "APIGT";

enum APPSAction { NoAction, NeedUpdate, NeedNew, NeedCancel };

void processPax( const int pax_id, const std::string& override_type = "", const bool is_forced = false );
void APPSFlightCloseout( const int point_id );
std::string getAnsText( const std::string& tlg );
bool processReply( const std::string& source );
bool checkAPPSSets(const int point_dep, const int point_arv );
bool checkAPPSSets( const int point_dep, const std::string& airp_arv );
bool checkAPPSSets( const int point_dep, const std::string& airp_arv, bool& tansit );
bool checkTime( const int point_id );
bool checkTime( const int point_id, BASIC::TDateTime& start_time );
std::string emulateAnswer( const std::string& request );
bool IsAPPSAnswText( const std::string& tlg_body );
std::set<std::string> needFltCloseout( const std::set<std::string>& countries, const std::string airline );
void sendAllAPPSInfo( const int point_id, const std::string& task_name, const std::string& params );
void sendNewAPPSInfo( const int point_id, const std::string& task_name, const std::string& params );
void reSendMsg( const int send_attempts, const std::string& msg_text, const int msg_id );
void deleteMsg( const int msg_id );

struct TTransData
{
  int msg_id;
  std::string code;
  std::string user_id;
  bool type; // '' = Check-in Transaction. "P" = Pre-Check-in Vetting
  int ver; // для Арабских Эмиратов версия 21

  TTransData() : type( false ), ver( ASTRA::NoExists ) {}
  void init( const bool pre_ckin, const std::string& trans_code, const std::string& id );
  bool operator == ( const TTransData& data ) const
  {
    return type == data.type &&
           user_id == data.user_id &&
           ver == data.ver;
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
  BASIC::TDateTime date;
  std::string arv_port;  // IATA Airport Code
  BASIC::TDateTime arv_date;

  TFlightData() : point_id( ASTRA::NoExists ), date( ASTRA::NoExists ),
                  arv_date( ASTRA::NoExists ) {}
  void init( const int point_id, const std::string& type );
  void init( const int point_id, const std::string& type, const std::string& num, const std::string& airp, const std::string& arv_airp,
             BASIC::TDateTime dep, BASIC::TDateTime arv );
  bool operator == ( const TFlightData& data ) const
  {
    return type == data.type &&
           flt_num == data.flt_num &&
           port == data.port &&
           date == data.date &&
           arv_port == data.arv_port &&
           arv_date == data.arv_date;
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

  TPaxData () : pax_id(ASTRA::NoExists) {}
  void init( const int pax_id, const std::string& surname, const std::string name,
             const bool is_crew, const int transfer, const std::string& override );
  void init( TQuery &Qry );
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
        pnr_locator == data.pnr_locator;
  }
  void check_data() const;
  std::string msg() const;
};

struct TMftData
{
  std::string country; // 2-character IATA Country Code of the participating country.
  std::string mft_pax; // The only option supported for UAE will be "C".
  std::string mft_crew; // The only option supported for UAE will be "C".

  void init( const std::string& code )
  {
    country = code;
    mft_pax = "C";
    mft_crew = "C";
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
  bool getByPaxId( const int pax_id, const std::string& override_type );
  bool getByCrsPaxId( const int pax_id, const std::string& override_type );
  void saveData() const;

public:
  void init( const int pax_id, const std::string& override_type = "" )
  {
    if ( !getByPaxId( pax_id, override_type ) )
      getByCrsPaxId( pax_id, override_type );
  }
  bool fromDBByPaxId( const int pax_id );
  bool fromDBByMsgId( const int msg_id );
  bool operator == (const TPaxRequest &c) const
  {
    return trans == c.trans &&
           ckin_flt == c.ckin_flt &&
           int_flt == c.int_flt &&
           pax == c.pax;
  }
  bool operator != (const TPaxRequest &c) const
  {
    return !(*this == c);
  }
  APPSAction typeOfAction( const bool is_exists, const std::string& status,
                           const bool is_the_same, const bool is_forced ) const;
  std::string msg() const;
  void sendReq() const;
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
public:
  void init( const int point_id, const std::string& country );
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

  TAnsPaxData() : code(ASTRA::NoExists), error_code1(ASTRA::NoExists),
                  error_code2(ASTRA::NoExists), error_code3(ASTRA::NoExists) {}
  std::string toString() const;
  void init( std::string source );
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
