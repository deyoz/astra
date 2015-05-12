#ifndef APP_UAE_H
#define APP_UAE_H

#include <string>
#include <vector>
#include <iostream>
#include "passenger.h"

/* ������:
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
  int ver; // ��� ��� 21

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
  int flds_count;  // ���-�� ����� ��᫥ ⥪�饣�
  char is_scheduled; /* 'S'  ��� scheduled. 'U' ��� unscheduled. 'U' � �����饥 �६� �� �����ন������.
  �� ३�� ������ ���� ���� � �ᯨᠭ�� ����⮢ Official Airline Guide (OAG),
  ���� � ����� ��������� � APP (��� ������ � ����ঠ���� ३ᮢ) */
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
  int flds_count; // ���-�� ����� ��᫥ ⥪�饣�
  int seq_num; // ���浪��� ����� ���ᠦ�� � ᮮ�饭��, ���ᨬ� 5 ��� "PRQ", 10 ��� "PCX"
  int passenger_id; // ⮫쪮 ��� "PCX". �������� ID ���ᠦ��. ����砥� � �⢥� �� CIRQ �����
  char pax_crew; // 'C' ��� 'P'
  std::string nationality;
  std::string issuing_state; // optional ��� "PCX"
  std::string passport;
  char check_char; // optional
  char doc_type; // optional, 'P' - ���ᯮ��, 'O' - ��㣮� ���㬥��, 'N' - ��� ���㬥��
  std::string expiry_date; //CCYYMMDD
  std::string sup_doc_type; // � �����饥 �६� �� �ᯮ������
  std::string sup_passport; // � �����饥 �६� �� �ᯮ������
  char sup_check_char; // � �����饥 �६� �� �ᯮ������
  std::string family_name; // �� 2 �� 24 ᨬ�����
  std::string given_names; // �� 24 ᨬ�����, �᫨ ��� - '-'.
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
  int flds_count; // ���-�� ����� ��᫥ ⥪�饣�
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
  int flds_count; // ���-�� ����� ��᫥ ⥪�饣�
  int seq_num; // ���浪��� ����� ���ᠦ��. ��⢥����� ���浪����� ������ � ����� "CIRQ"
  std::string country; // 2-character IATA Country Code of the country generating the response.
  char pax_crew; // 'C' ��� 'P'
  std::string nationality;
  std::string issuing_state; // optional
  std::string passport; // optional
  char check_char; // optional
  char doc_type; // 'P' - ���ᯮ��, 'O' - ��㣮� ���㬥��, 'N' - ��� ���㬥��
  std::string expiry_date; // optional, CCYYMMDD
  std::string sup_doc_type; // � �����饥 �६� �� �ᯮ������
  std::string sup_passport; // � �����饥 �६� �� �ᯮ������
  char sup_check_char; // � �����饥 �६� �� �ᯮ������
  std::string family_name;  // optional, �� 2 �� 24 ᨬ�����
  std::string given_names; // optional, �� 24 ᨬ�����, �᫨ ��� - '-'.
  std::string birth_date; // optional, CCYYMMDD. If the day is not known, CCYYMM00
  char sex; // "M" = Male, "F" = Female, "U" = Unspecified, "X" = Unspecified
  std::string birth_country; // optional
  char endorsee; // optional. Blank for passport holder, "S" = Endorsee.
  int ckin_code; /* ������ ��� CIRS: 8501 OK to Board, 8502 Do Not Board, 8503 Board if Docs OK, 8517 Override Accepted, 8630 Contact UAE govt.
  ������ ��� CI��: 8505 Cancelled, 8506 No Record */
  char ckin_status; /* ������ ��� CIRS: 'B' = OK to Board, 'D' = Do Not Board (override is possible), 'X' = Do Not Board (override is impossible),
  'U' = Unable to determine status, 'I' = Not enought data, 'T' = Timeout, 'E' = Error.
  ������ ��� CI��: 'C' = Cancelled, 'N' = Not found, 'T' = Timeout, 'E' = Error. */
  int passenger_id; // �������� ID ���ᠦ��. �ᯮ������ � ��砥 �⬥�� ���ᠦ�� � ������⥫쭮� ��४⨢��. ���쪮 ��� CIRS
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
  int flds_count; // ���-�� ����� ��᫥ ⥪�饣�
  std::vector<TError> errors;

  TAnsError() {
    flds_count = 0;
  }
  std::string toString();
};

struct TAnsMft
{
  std::string grp_id;
  int flds_count; // ���-�� ����� ��᫥ ⥪�饣�
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
  // PassengerData. Applicable to "CIRS" and "CI��". Conditional (required if ERR data group is not present)
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
