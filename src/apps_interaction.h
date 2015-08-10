#ifndef APPS_INTERACTION_H
#define APPS_INTERACTION_H

#include <string>
#include <vector>
#include <iostream>
#include "passenger.h"
#include "checkin.h"

const int MaxSendAttempts = 5;
enum APPSAction { NoAction, NeedUpdate, NeedNew, NeedCancel };

struct TSegment
{
  int point_dep;
  int point_arv;
  std::string airp_dep;
  std::string airp_arv;
  std::string country_dep;
  std::string country_arv;
  TTripInfo flt;
  void init(const int dep, const int arv, const std::string& port_dep,
            const std::string& port_arv, const TTripInfo& info);
};

struct TPaxData
{
  int pax_id;
  bool is_cancel;
  CheckIn::TPaxDocItem doc;

  TPaxData(const int pax_id, const bool is_cancel, const CheckIn::TPaxDocItem& doc)
    :pax_id(pax_id), is_cancel(is_cancel), doc(doc) {}
};

typedef std::vector<TSegment> TSegmentsList;
typedef std::vector<TPaxData> TPaxDocList;

struct TReqTrans
{
  std::string code;
  std::string user_id;
  std::string multi_resp; // "Y" = the user location can handle duplicate names (endorsees). "N" otherwise.
  std::string  type; // '' = Check-in Transaction. "P" = Pre-Check-in Vetting
  int ver; // для Арабских Эмиратов версия 21

  TReqTrans() : ver(0) {}
  TReqTrans ( const std::string& trans_type, const std::string& id );
  void check_data() const;
  std::string msg( const std::string& msg_id ) const;
};

struct TFlightData
{
  std::string grp_id;
  int flds_count;  // кол-во полей после текущего
  std::string is_scheduled; /* "S"  для scheduled. "U" для unscheduled. "U" в настоящее время не поддерживается.
  Все рейсы должны быть либо в расписании полетов Official Airline Guide (OAG),
  либо в ручную добавлены в APPS (для чартерных и задержанных рейсов) */
  std::string flt_num; // Airline and flight number.
  std::string port; // IATA Airport Code
  std::string date; // CCYYMMDD
  std::string time; // HHMMSS
  std::string arv_port;  // IATA Airport Code
  std::string arr_date; // CCYYMMDD
  std::string arr_time; // HHMMSS

  TFlightData() : flds_count(0) {}
  TFlightData(const std::pair<std::string, int>& id, const std::string& flt, const std::string& airp_dep,
              const BASIC::TDateTime dep, const std::string& airp_arv, const BASIC::TDateTime arr);
  void init(const std::pair<std::string, int>& id, const int point_id);
  void init(const TFlightData& data);

  void check_data() const;
  std::string msg() const;
  inline bool empty() const {
    return grp_id.empty();
  }
};

struct TReqPaxData //repeating
{
  int pax_id;
  std::string grp_id;
  int flds_count; // кол-во полей после текущего
  int apps_pax_id; // только для "PCX". Уникальный ID пассажира. Получаем в ответ на CIRQ запрос
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
  std::string status;

  TReqPaxData () : pax_id(0), flds_count(0), apps_pax_id(0) {}
  TReqPaxData( const bool is_crew, const TPaxData& pax_doc, const int transfer,
               const std::string& override = "" );
  bool fromDB( const int pax_ident );
  bool equalAttrs( const TReqPaxData& data ) const
  {
    return pax_crew == data.pax_crew &&
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
  APPSAction typeOfAction( const std::string& status, const bool is_the_same,
                           const bool is_cancel, const bool is_forced ) const;
  void check_data() const;
  std::string msg( const int seq_num ) const;
  void savePaxData( const std::string& code, const std::string& msg_id, const int seq_num ) const;
};

struct TReqMft
{
  std::string grp_id;
  int flds_count; // кол-во полей после текущего
  std::string country; // 2-character IATA Country Code of the participating country.
  std::string mft_pax; // The only option supported for UAE will be "C".
  std::string mft_crew; // The only option supported for UAE will be "C".

  TReqMft() : flds_count(0) {}
  void init(const std::string& state, const std::string& pax_req = "C", const std::string& crew_req ="C");
  void check_data() const;
  std::string msg() const;
};

struct APPSRequest
{
  int point_id;
  TReqTrans trans;
  TFlightData ckin_flt;
  TFlightData int_flt;
  std::vector<TFlightData> exp_flt;
  std::vector<TReqPaxData> passengers;
  TFlightData inm_flt;
  TReqMft mft_req;

  APPSRequest() : point_id(0) {}
  APPSRequest( const std::string& trans_type, const std::string& id, const int point_id )
    : point_id(point_id), trans(trans_type, id) { }
  void sendReq() const;
  void sendMsg( const std::string& text, const std::string& msg_id ) const;
  void addPax( const TReqPaxData& pax_data );
};

struct TAnsTrans
{
  std::string code;
  std::string msg_ident;

  std::string toString() const;
};

struct TAnsPaxData
{
  int pax_id;
  std::string grp_id;
  int flds_count; // кол-во полей после текущего
  int seq_num; // порядковый номер пассажира. оответствует порядковому номеру в запросе "CIRQ"
  std::string country; // 2-character IATA Country Code of the country generating the response.
  std::string pax_crew; // "C" или "P"
  std::string nationality;
  std::string issuing_state; // optional
  std::string passport; // optional
  std::string check_char; // optional
  std::string doc_type; // "P" - пасспорт, "O" - другой документ, "N" - нет документа
  std::string expiry_date; // optional, CCYYMMDD
  std::string sup_doc_type; // в настоящее время не используется
  std::string sup_passport; // в настоящее время не используется
  std::string sup_check_char; // в настоящее время не используется
  std::string family_name;  // optional, от 2 до 24 символов
  std::string given_names; // optional, до 24 символов, если нет - '-'.
  std::string birth_date; // optional, CCYYMMDD. If the day is not known, CCYYMM00
  std::string sex; // "M" = Male, "F" = Female, "U" = Unspecified, "X" = Unspecified
  std::string birth_country; // optional
  std::string endorsee; // optional. Blank for passport holder, "S" = Endorsee.
  int code; /* Статусы для CIRS: 8501 OK to Board, 8502 Do Not Board, 8503 Board if Docs OK, 8517 Override Accepted, 8630 Contact UAE govt.
  Статусы для CIСС: 8505 Cancelled, 8506 No Record */
  std::string status; /* Статусы для CIRS: "B" = OK to Board, "D" = Do Not Board (override is possible), "X" = Do Not Board (override is impossible),
  "U" = Unable to determine status, "I" = Not enought data, "T" = Timeout, "E" = Error.
  Статусы для CIСС: "C" = Cancelled, "N" = Not found, "T" = Timeout, "E" = Error. */
  int apps_pax_id; // Уникальный ID пассажира. Используется в случае отмены пассажира с положительной директивой. Только для CIRS
  int error_code1; // Conditional (on error condition)
  std::string error_text1; // Conditional (on error condition)
  int error_code2; // Conditional (on error condition)
  std::string error_text2; // Conditional (on error condition)
  int error_code3; // Conditional (on error condition)
  std::string error_text3; // Conditional (on error condition)

  TAnsPaxData() : pax_id(0), flds_count(0), seq_num(0), code(0), apps_pax_id(0),
                  error_code1(0), error_code2(0), error_code3(0) {}
  void getPaxId( const TAnsTrans& trans );
  std::string toString() const;
  void logAPPSPaxStatus( const std::string& trans_code, const int point_id ) const;
};

struct TError
{
  std::string country; /* 2-character IATA Country Code of the country generating the response.
  The field may be null if the error is not specific to any country. */
  int error_code; // Conditional (on error condition)
  std::string error_text; // Conditional (on error condition)

  TError() : error_code(0) {}
};

struct TAnsError
{
  std::string  grp_id;
  int flds_count; // кол-во полей после текущего
  std::vector<TError> errors;

  TAnsError() : flds_count(0) {}
  std::string toString() const;
};

struct TAnsMft
{
  std::string grp_id;
  int flds_count; // кол-во полей после текущего
  std::string country; // 2-character IATA Country Code of the participating country.
  int resp_code; // 8700 = Request processed, 8701 = Request rejected
  int error_code; // Conditional (on error condition)
  std::string error_text; // Conditional (on error condition)

  TAnsMft() : flds_count(0), resp_code(0), error_code(0) {}
  std::string toString() const;
};

struct APPSAnswer
{
  int point_id;
  int send_attempts;
  std::string msg_text;
  TAnsTrans trans;
  // PassengerData. Applicable to "CIRS" and "CIСС". Conditional (required if ERR data group is not present)
  std::vector<TAnsPaxData> passengers;
  // Error (repeating). Conditional (required if PRS data group is not present)
  TAnsError err_grp;
  // Manifest response. Applicable to "CIMA". Conditional (required if ERR data group is not present)
  TAnsMft mft_grp;

  APPSAnswer():point_id(0), send_attempts(0) {}
  bool init(const std::string& source);
  std::string toString() const;
  void processReply() const;
  bool getMsgInfo(const std::string& msg_id);
  void processError() const;
  std::string getPaxsStr() const;
};

bool isNeedAPPSReq( const int point_dep, const int point_arv );
bool isNeedAPPSReq( const int point_dep, const std::string& airp_arv );
bool isNeedAPPSReq( const std::set<std::string>& countries );
void composeAPPSReq( const TSegmentsList& segs, const int point_dep, const TPaxDocList& paxs,
                     const bool is_crew, const std::string& override_type = "",
                     const bool is_forced = false );
bool needFltCloseout( const std::set<std::string>& countries, std::set<std::string>& countries_need_req );
void APPSFlightCloseout( const int point_id );
void processPax( const int pax_id, const std::string& override_type, const bool is_cancel );
void processCrsPax( const int pnr_id, const std::string& override_type, const bool is_cancel );
bool IsAPPSAnswText( const std::string& tlg_body );
std::string emulateAnswer( const std::string& request );
void reSendMsg( const int send_attempts, const std::string& msg_text, const std::string& msg_id );
void deleteMsg( const std::string& msg_id );
std::string getAPPSAddr();

#endif // APPS_INTERACTION_H
