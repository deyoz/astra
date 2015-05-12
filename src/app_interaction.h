#ifndef APP_UAE_H
#define APP_UAE_H

#include <string>
#include <vector>
#include <iostream>
#include "passenger.h"

/* Вопросы:
   1. user_id
   2. Pre-Check-in Vetting */

int toInt(const std::string& str);

struct TReqTrans
{
  std::string code;
  std::string airl_data;
  std::string user_id;
  char multi_resp; // 'Y' = the user location can handle duplicate names (endorsees). 'N' otherwise.
  char type; // '' = Check-in Transaction. 'P' = Pre-Check-in Vetting
  int ver; // для нас 21

  TReqTrans() {
    multi_resp = 0;
    type = 0;
    ver = 0;
  }
  TReqTrans ( const std::string& trans_type, const std::string& id, const std::string& data = "" );
  void check_data();
  std::string msg();
};

struct TFlightData
{
  std::string grp_id;
  int flds_count;  // кол-во полей после текущего
  char is_scheduled; /* 'S'  для scheduled. 'U' для unscheduled. 'U' в настоящее время не поддерживается.
  Все рейсы должны быть либо в расписании полетов Official Airline Guide (OAG),
  либо в ручную добавлены в APP (для чартерных и задержанных рейсов) */
  std::string flt_num; // Airline and flight number.
  std::string port; // IATA Airport Code
  std::string date; // CCYYMMDD
  std::string time; // HHMMSS
  std::string arr_port;  // IATA Airport Code
  std::string arr_date; // CCYYMMDD
  std::string arr_time; // HHMMSS

  TFlightData() {
    flds_count = 0;
    is_scheduled = 0;
  }
  TFlightData(std::pair<std::string, int> id, std::string flt, std::string airp_dep,
              BASIC::TDateTime dep, std::string airp_arr, BASIC::TDateTime arr);
  void check_data();
  std::string msg();
  inline bool empty() {
    return grp_id.empty();
  }
};

struct TReqPaxData //repeating
{
  std::string grp_id;
  int flds_count; // кол-во полей после текущего
  int seq_num; // порядковый номер пассажира в сообщении, максимум 5 для "PRQ", 10 для "PCX"
  int passenger_id; // только для "PCX". Уникальный ID пассажира. Получаем в ответ на CIRQ запрос
  char pax_crew; // 'C' или 'P'
  std::string nationality;
  std::string issuing_state; // optional для "PCX"
  std::string passport;
  char check_char; // optional
  char doc_type; // optional, 'P' - пасспорт, 'O' - другой документ, 'N' - нет документа
  std::string expiry_date; //CCYYMMDD
  std::string sup_doc_type; // в настоящее время не используется
  std::string sup_passport; // в настоящее время не используется
  char sup_check_char; // в настоящее время не используется
  std::string family_name; // от 2 до 24 символов
  std::string given_names; // до 24 символов, если нет - '-'.
  std::string birth_date; // CCYYMMDD. If the day is not known, use CCYYMM00
  char sex; // "M" = Male, "F" = Female, "U" = Unspecified, "X" = Unspecified
  std::string birth_country; // optional
  char endorsee; // Blank for passport holder, "S" = Endorsee. Optional
  char trfer_at_origin; // optional
  char trfer_at_dest; // optional
  std::string override_codes; /* Optional. Format is: CxxCxxCxxCxxCxx,
  where "C" = Override code ('A' = airline override, 'G' = government override),
  "xx" = IATA country code */
  std::string pnr_source; // conditional
  std::string pnr_locator; // conditional

  TReqPaxData () {
    flds_count = 0;
    seq_num = 0;
    passenger_id = 0;
    pax_crew = 0;
    check_char = 0;
    doc_type = 0;
    sup_check_char = 0;
    sex = 0;
    endorsee = 0;
    trfer_at_origin = 0;
    trfer_at_dest = 0;
  }
  TReqPaxData(const int num, const bool is_crew, const CheckIn::TPaxDocItem& pax_doc, const int app_pax_id = 0);
  void check_data();
  std::string msg();
};

struct TReqMft
{
  std::string grp_id;
  int flds_count; // кол-во полей после текущего
  std::string country; // 2-character IATA Country Code of the participating country.
  char mft_pax; // The only option supported for UAE will be "C".
  char mft_crew; // The only option supported for UAE will be "C".

  TReqMft() {
    flds_count = 0;
    mft_pax = 0;
    mft_crew = 0;
  }
  TReqMft(std::string state, char pax_req = 'C', char crew_req = 'C');
  void check_data();
  std::string msg();
};

struct APPRequest
{
  TReqTrans trans;
  TFlightData ckin_flt;
  TFlightData int_flt;
  std::vector<TFlightData> exp_flt;
  std::vector<TReqPaxData> passengers;
  TFlightData inm_flt;
  TReqMft mft_req;

  std::string ComposeMsg();
};

struct TAnsTrans
{
  std::string code;
  std::string airl_data;

  std::string toString();
};

struct TAnsPaxData
{
  std::string grp_id;
  int flds_count; // кол-во полей после текущего
  int seq_num; // порядковый номер пассажира. оответствует порядковому номеру в запросе "CIRQ"
  std::string country; // 2-character IATA Country Code of the country generating the response.
  char pax_crew; // 'C' или 'P'
  std::string nationality;
  std::string issuing_state; // optional
  std::string passport; // optional
  char check_char; // optional
  char doc_type; // 'P' - пасспорт, 'O' - другой документ, 'N' - нет документа
  std::string expiry_date; // optional, CCYYMMDD
  std::string sup_doc_type; // в настоящее время не используется
  std::string sup_passport; // в настоящее время не используется
  char sup_check_char; // в настоящее время не используется
  std::string family_name;  // optional, от 2 до 24 символов
  std::string given_names; // optional, до 24 символов, если нет - '-'.
  std::string birth_date; // optional, CCYYMMDD. If the day is not known, CCYYMM00
  char sex; // "M" = Male, "F" = Female, "U" = Unspecified, "X" = Unspecified
  std::string birth_country; // optional
  char endorsee; // optional. Blank for passport holder, "S" = Endorsee.
  int ckin_code; /* Статусы для CIRS: 8501 OK to Board, 8502 Do Not Board, 8503 Board if Docs OK, 8517 Override Accepted, 8630 Contact UAE govt.
  Статусы для CIСС: 8505 Cancelled, 8506 No Record */
  char ckin_status; /* Статусы для CIRS: 'B' = OK to Board, 'D' = Do Not Board (override is possible), 'X' = Do Not Board (override is impossible),
  'U' = Unable to determine status, 'I' = Not enought data, 'T' = Timeout, 'E' = Error.
  Статусы для CIСС: 'C' = Cancelled, 'N' = Not found, 'T' = Timeout, 'E' = Error. */
  int passenger_id; // Уникальный ID пассажира. Используется в случае отмены пассажира с положительной директивой. Только для CIRS
  int error_code1; // Conditional (on error condition)
  std::string error_text1; // Conditional (on error condition)
  int error_code2; // Conditional (on error condition)
  std::string error_text2; // Conditional (on error condition)
  int error_code3; // Conditional (on error condition)
  std::string error_text3; // Conditional (on error condition)

  TAnsPaxData() {
    flds_count = 0;
    seq_num = 0;
    pax_crew = 0;
    check_char = 0;
    doc_type = 0;
    sup_check_char = 0;
    sex = 0;
    endorsee = 0;
    ckin_code = 0;
    ckin_status = 0;
    passenger_id = 0;
    error_code1 = 0;
    error_code2 = 0;
    error_code3 = 0;
  }

  std::string toString();
};

struct TError
{
  std::string country; /* 2-character IATA Country Code of the country generating the response.
  The field may be null if the error is not specific to any country. */
  int error_code; // Conditional (on error condition)
  std::string error_text; // Conditional (on error condition)

  TError() {
    error_code = 0;
  }
};

struct TAnsError
{
  std::string  grp_id;
  int flds_count; // кол-во полей после текущего
  std::vector<TError> errors;

  TAnsError() {
    flds_count = 0;
  }
  std::string toString();
};

struct TAnsMft
{
  std::string grp_id;
  int flds_count; // кол-во полей после текущего
  std::string country; // 2-character IATA Country Code of the participating country.
  int resp_code; // 8700 = Request processed, 8701 = Request rejected
  int error_code; // Conditional (on error condition)
  std::string error_text; // Conditional (on error condition)

  TAnsMft() {
    flds_count = 0;
    resp_code = 0;
    error_code = 0;
  }
  std::string toString();
};

struct APPAnswer
{
  TAnsTrans trans;
  // PassengerData. Applicable to "CIRS" and "CIСС". Conditional (required if ERR data group is not present)
  std::vector<TAnsPaxData> passengers;
  // Error (repeating). Conditional (required if PRS data group is not present)
  TAnsError err_grp;
  // Manifest response. Applicable to "CIMA". Conditional (required if ERR data group is not present)
  TAnsMft mft_grp;
  std::string source;

  APPAnswer(const std::string& source) : source(source) {}
  void parse();
  std::string toString();
};

int test_app_interaction(int argc,char **argv);

#endif // APP_UAE_H
