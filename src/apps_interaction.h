#pragma once

#include <string>
#include <vector>
#include <iostream>
#include "passenger.h"
#include "checkin.h"
#include "points.h"
#include "date_time.h"
#include "qrys.h"
#include "trip_tasks.h"
#include "serverlib/dates.h"

//#include "boost/date_time/s
namespace APPS {

const int NumSendAttempts = 5; // количество попыток до включения тревоги "Нет связи с APPS"
const int MaxSendAttempts = 99; // максимальное количество попыток

enum class TransferFlag
{
    None,
    Origin,
    Dest,
    Both
};

template<typename T>
using Opt = boost::optional<T>;

std::ostream& operator << (std::ostream& os, const TransferFlag& trfer);
void processPax(const int pax_id, const std::string& override_type = "", const bool is_forced = false);
void processCrsPax(const int pax_id, const std::string& override_type = "");
std::vector<std::string> statusesFromDb(const int pax_id);
void appsFlightCloseout(const int point_id);
bool processReply(const std::string& source_raw);
bool checkAPPSSets(const int point_dep, const int point_arv);
bool checkAPPSSets(const int point_dep, const std::string& airp_arv, std::set<std::string>* pFormats = nullptr);
bool checkAPPSSets(const int point_dep, const std::string& airp_arv, bool& transit, std::set<std::string>* pFormats = nullptr);
bool checkTime(const int point_id);
bool checkTime(const int point_id, TDateTime& start_time);
bool isAPPSAnswText(const std::string& tlg_body);
void sendAllAPPSInfo(const TTripTaskKey &task);
void sendNewAPPSInfo(const TTripTaskKey &task);
void reSendMsg(const int send_attempts, const std::string& msg_text, const int msg_id);
void deleteMsg(const int msg_id);
const char* getAPPSRotName();
std::string appsTextAsHumanReadable(const std::string& apps);
std::string emulateAnswer(const std::string& request);

class AppsCollector
{
public:
    struct PaxItem
    {
        int         pax_id;
        std::string override;
        bool        is_forced;
    };
    void addPaxItem(int pax_id, const std::string& override = "", bool is_forced = false);
    void send();

private:
    std::vector<PaxItem> m_paxItems;
};

class AppsSettings
{
public:
    static Opt<AppsSettings> readSettings(const std::string& airline, const std::string& country);
    int version() const;
    std::string getAirline() const { return airline;      }
    std::string getCountry() const { return country;      }
    std::string getFormat()  const { return format;       }
    std::string getRouter()  const { return router;       }
    int  getId()             const { return id;           }
    bool getFltCloseout()    const { return flt_closeout; }
    bool getInbound()        const { return inbound;      }
    bool getOutbound()       const { return outbound;     }
    bool getDenial()         const { return denial;       }
    bool getPreCheckin()     const { return pre_checkin;  }
    bool operator < (const AppsSettings& rhs) const
    {
        return this->id < rhs.id;
    }
    void setRouter(std::string router)
    {
        this->router = router;
    }
private:
    AppsSettings() = default;
    std::string airline;
    std::string country;
    std::string format;
    std::string router;  //for future
    int id;
    bool flt_closeout;
    bool inbound;
    bool outbound;
    bool denial;
    bool pre_checkin;
};

class TransactionData
{
public:
  int msg_id;
  std::string header;
  std::string code;
  std::string user_id;
  bool type; // '' = Check-in Transaction. "P" = Pre-Check-in Vetting

  TransactionData() : type(false) {}
  TransactionData& init(const std::string& trans_code, const std::string &airline);
  bool operator == (const TransactionData& data) const
  {
    return  type == data.type && user_id == data.user_id;
  }
  void validateData() const;
  std::string msg(int version) const;
  TransactionData & setPreCheckin(bool pre_checkin)      { this->type    = pre_checkin; return *this; }

};

class FlightData
{
public:
    std::string type;
    int point_id;
    std::string flt_num; // Airline and flight number.
    std::string dep_port; // IATA Airport Code
    Dates::DateTime_t dep_date;
    std::string arv_port;  // IATA Airport Code
    Dates::DateTime_t arv_date;

    FlightData() : point_id(ASTRA::NoExists){}
    FlightData& init(const std::string& flt_type);
    FlightData& init(const CheckIn::TPaxSegmentPair &flt, const std::string& type);
    bool operator == (const FlightData& data) const
    {
    return type == data.type &&
           flt_num == data.flt_num &&
           dep_port == data.dep_port &&
           dep_date == data.dep_date &&
           arv_port == data.arv_port &&
           arv_date == data.arv_date;
    }
    void validateData() const;
    std::string msg(int version) const;
    FlightData& setType(const std::string& type)      {this->type     = type;     return *this;}
    FlightData& setFltNum(const std::string &flt_num) {this->flt_num  = flt_num;  return *this;}
    FlightData& setFltNum(const std::string &airline, const TAdvTripRoute &route);
    FlightData& setDepPortAndDate(const TAdvTripRoute &route);
    FlightData& setArvPortAndDate(const TAdvTripRoute &route);
};

class PaxData
{
public:
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

    PaxData () : pax_id(ASTRA::NoExists) {}
    void init(const int pax_id, const std::string& surname, const std::string &name,
              const bool is_crew, const std::string& override_type,
              const int reg_no, const ASTRA::TTrickyGender::Enum tricky_gender,
              TransferFlag transfer = TransferFlag::None);
    void setNames(const CheckIn::TPaxDocItem& doc, const std::string& surname, const std::string& name);
    void setPassReference(const ASTRA::TTrickyGender::Enum tricky_gender, const int reg_no);
    inline void setPaxId(int pax_id)
    {
        this->pax_id = pax_id;
    }
    void setTypeOfPax(const bool is_crew);
    void setDocInfo(const CheckIn::TPaxDocItem& doc);
    void setTransfer(const TransferFlag trfer);
    void setOverrideType(const std::string& override_type);
    bool operator == (const PaxData& data) const;
    void validateData() const;
    std::string msg(int version) const;
};

class PaxAddData // Passenger Additional Data
{
public:
    //int version = APPS_VERSION_26;
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

    //friend void initFromDb(PaxAddData &res, OciCpp::CursCtl &cur);
    static Opt<PaxAddData> createByPaxId(const int pax_id);
    static Opt<PaxAddData> createByCrsPaxId(const int pax_id);
    void init(const int pax_id, const CheckIn::TPaxDocoItem &doco);
    bool operator == (const PaxAddData& d) const
    {
      return
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
    void validateData() const;
    std::string msg(int version) const;
};

class ManifestData
{
public:
    std::string country; // 2-character IATA Country Code of the participating country.
    std::string mft_pax; // The only option supported for UAE will be "C".
    std::string mft_crew; // The only option supported for UAE will be "C".

    void init(const std::string& code)
    {
        country = code;
        mft_pax = "C";
        mft_crew = "C";
    }
    void validateData() const;
    std::string msg(int version) const;
};

class MetaInfo
{
public:
    int grp_id;
    std::string surname;
    std::string name;
    std::string pers_type;
    std::string subclass;
    std::string airp_dep;
    std::string airp_arv;
    int seats;
};

class PaxRequest
{
public:
    enum RequestType{
    Crs,
    Pax
    };

    std::string getStatus() const
    {
        return pax.status;
    }

    PaxRequest(TransactionData trs, FlightData inflight, FlightData ckinflight,
             PaxData pax, Opt<PaxAddData> paxadd)
             : trans(trs), int_flt(inflight), ckin_flt(ckinflight), pax(pax), pax_add(paxadd)
    {
    }

    friend void defFromDB(PaxRequest &res, OciCpp::CursCtl & cur);
    static Opt<PaxRequest> createFromDB(RequestType type, const int pax_id, const std::string& override_type="");
    static Opt<PaxRequest> fromDBByPaxId(const int pax_id);
    static Opt<PaxRequest> fromDBByMsgId(const int msg_id);

    bool operator == (const PaxRequest &c) const;
    bool operator != (const PaxRequest &c) const
    {
    return !(*this == c);
    }
    bool isNeedCancel(const std::string& status) const;
    bool isNeedUpdate(const std::string& status, bool is_the_same) const;
    bool isNeedNew(const bool is_exists, const std::string& status, bool is_the_same, bool is_forced) const;
    void ifNeedDeleteAlarms(const bool is_exists, const std::string& status) const;
    void ifOutOfSyncSendAlarm(const std::string& status, const bool is_the_same) const;
    void ifNeedDeleteApps(const std::string& status) const;

    std::string msg(int version) const;
    void sendReqAndSave(const AppsSettings & settings) const;
    const TransactionData& getTrans() const
    {
      return trans;
    }
    const PaxData& getPax() const
    {
      return pax;
    }

private:
    PaxRequest() = default;
    TransactionData trans;
    FlightData int_flt;
    FlightData ckin_flt;
    PaxData pax;
    Opt<PaxAddData> pax_add;
    MetaInfo meta_info;
    void savePaxRequest(int version) const;
    static Opt<PaxRequest> createByPaxId(const int pax_id, const std::string& override_type);
    static Opt<PaxRequest> createByCrsPaxId(const int pax_id, const std::string& override_type);
    static Opt<PaxRequest> createByFields(const RequestType reqType, const int pax_id, const int point_id,
                                                      const MetaInfo& info, bool pre_checkin, const std::string &status,
                                                      const bool need_del, ASTRA::TGender::Enum gender, const std::string& override_type,
                                                      const std::string &airline, int reg_no=ASTRA::NoExists);
};

class ManifestRequest
{
private:
    TransactionData trans;
    FlightData int_flt;
    ManifestData mft_req;
public:
    bool init(const CheckIn::TPaxSegmentPair& flt, const std::string& country_lat);
    std::string msg(int version) const;
    void sendReqAndSave(const AppsSettings &settings) const;
};

class AnsPaxData
{
public:
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

    AnsPaxData() : code(ASTRA::NoExists), error_code1(ASTRA::NoExists),
                  error_code2(ASTRA::NoExists), error_code3(ASTRA::NoExists) {}
    std::string toString() const;
    void init(std::string source, int ver);
};

class Error
{
public:
    std::string country; /* 2-character IATA Country Code of the country generating the response.
    The field may be null if the error is not specific to any country. */
    int error_code; // Conditional (on error condition)
    std::string error_text; // Conditional (on error condition)
    Error() : error_code(ASTRA::NoExists) {}
};

class APPSAnswer
{
protected:
    std::string code;
    int msg_id;
    int send_attempts;
    std::string msg_text;
    int point_id;
    std::vector<Error> errors; // Error (repeating). Conditional
    int version = 0;

    bool checkIfNeedResend() const;
    virtual void getLogParams(LEvntPrms& params, const std::string& country, const int status_code,
                  const int error_code, const std::string& error_text) const;
public:
    APPSAnswer() : send_attempts(ASTRA::NoExists), point_id(ASTRA::NoExists) {}
    virtual ~APPSAnswer() {}
    virtual bool init(const std::string& code, const std::string& source);
    virtual void beforeProcessAnswer() const;
    virtual std::string toString() const;
    virtual void processErrors() const = 0;
    virtual void processAnswer() const = 0;
    virtual void logAnswer(const std::string& country, const int status_code,
                  const int error_code, const std::string& error_text) const = 0;
};

class PaxReqAnswer : public APPSAnswer
{
private:
    int pax_id;
    int version;
    std::string family_name;
    // PassengerData. Applicable to "CIRS" and "CIСС". Conditional (required if ERR data group is not present)
    std::vector<AnsPaxData> passengers;
public:
    PaxReqAnswer() : APPSAnswer(), pax_id(ASTRA::NoExists), version(0) {}
    virtual ~PaxReqAnswer() {}
    virtual bool init(const std::string& code, const std::string& source);
    virtual void processErrors() const;
    virtual void processAnswer() const;
    virtual std::string toString() const;
    virtual void logAnswer(const std::string& country, const int status_code,
                  const int error_code, const std::string& error_text) const;
};

class ManifestAnswer : public APPSAnswer
{
private:
  // Manifest response. Applicable to "CIMA". Conditional (required if ERR data group is not present)
  std::string country; // 2-character IATA Country Code of the participating country.
  int resp_code; // 8700 = Request processed, 8701 = Request rejected
  int error_code; // Conditional (on error condition)
  std::string error_text; // Conditional (on error condition)

public:
  ManifestAnswer() : APPSAnswer(), resp_code(ASTRA::NoExists), error_code(ASTRA::NoExists) {}
  virtual ~ManifestAnswer() {}
  virtual bool init(const std::string& code, const std::string& source);
  virtual void processErrors() const;
  virtual void processAnswer() const;
  virtual std::string toString() const;
  virtual void logAnswer(const std::string& country, const int status_code,
                  const int error_code, const std::string& error_text) const;
};
} //namespace APPS

