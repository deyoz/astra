#ifndef _ASTRA_MISC_H_
#define _ASTRA_MISC_H_

#include <vector>
#include <string>
#include <set>
#include <tr1/memory>
#include "date_time.h"
#include "astra_consts.h"
#include "oralib.h"
#include "astra_utils.h"
#include "astra_elems.h"
#include "astra_locale.h"
#include "stages.h"
#include "xml_unit.h"
#include "astra_types.h"
#include "db_tquery.h"
#include "arx_daily_pg.h"
#include "dbostructures.h"

using BASIC::date_time::TDateTime;

class TPaxSegmentPair;
class TTripInfo;
class TAdvTripRoute;

std::optional<TTripInfo> getPointInfo(const PointId_t point_dep);
std::vector<std::string> segAirps(const TPaxSegmentPair & flight);
std::vector<int> segPoints(const TPaxSegmentPair & flight);
TAdvTripRoute getTransitRoute(const TPaxSegmentPair& flight);
std::vector<TPaxSegmentPair> transitLegs(const TAdvTripRoute& route);

class TPaxSegmentPair
{
  public:
    int point_dep;
    std::string airp_arv;

    TPaxSegmentPair(const int pointDep, const std::string& airpArv) :
      point_dep(pointDep), airp_arv(airpArv) {}

    bool operator < (const TPaxSegmentPair &seg) const
    {
      if (point_dep!=seg.point_dep)
        return point_dep < seg.point_dep;
      return airp_arv < seg.airp_arv;
    }

    PointId_t pointDep() const { return PointId_t(point_dep); }
    AirportCode_t airpArv() const { return AirportCode_t(airp_arv); }
};

class FlightProps
{
  public:
    enum Cancellation { NotCancelled,
                        WithCancelled };
    enum CheckInAbility { WithCheckIn,
                          WithOrWithoutCheckIn };
  private:
    Cancellation _cancellation;
    CheckInAbility _checkin_ability;
  public:
    FlightProps() :
      _cancellation(WithCancelled), _checkin_ability(WithOrWithoutCheckIn) {}
    FlightProps(Cancellation prop1,
                CheckInAbility prop2=WithOrWithoutCheckIn) :
      _cancellation(prop1), _checkin_ability(prop2) {}
    Cancellation cancellation() const { return _cancellation; }
    CheckInAbility checkin_ability() const { return _checkin_ability; }
};

class TSimpleMktFlight
{
  private:
    void init()
    {
      airline.clear();
      flt_no=ASTRA::NoExists;
      suffix.clear();
    }
  public:
    std::string airline;
    int flt_no;
    std::string suffix;
    explicit TSimpleMktFlight() { init(); }
    explicit TSimpleMktFlight(const std::string& _airline,
                              const int& _flt_no,
                              const std::string& _suffix) :
      airline(_airline), flt_no(_flt_no), suffix(_suffix) {}
    void clear()
    {
      init();
    }
    bool empty() const
    {
      return airline.empty() &&
             flt_no==ASTRA::NoExists &&
             suffix.empty();
    }

    bool operator == (const TSimpleMktFlight &flt) const
    {
      return airline == flt.airline &&
             flt_no == flt.flt_no &&
             suffix == flt.suffix;
    }

    void operator = (const TSimpleMktFlight &flt)
    {
        airline = flt.airline;
        flt_no = flt.flt_no;
        suffix = flt.suffix;
    }

    const TSimpleMktFlight& toXML(xmlNodePtr node,
                                  const boost::optional<AstraLocale::OutputLang>& lang) const;
    TSimpleMktFlight& fromXML(xmlNodePtr node);
    std::string flight_number(const boost::optional<AstraLocale::OutputLang>& lang = boost::none) const
    {
      std::ostringstream s;
      s << std::setw(3) << std::setfill('0') << flt_no;
      s << (lang? ElemIdToElem(etSuffix, suffix, efmtCodeNative, lang->get()): suffix);
      return s.str();
    }
    virtual ~TSimpleMktFlight() {}
};

typedef std::vector<TSimpleMktFlight> TSimpleMktFlights;

class TMktFlight : public TSimpleMktFlight
{
  private:
    void get(TQuery &Qry, int id);
    void init()
    {
      subcls.clear();
      scd_day_local = ASTRA::NoExists;
      scd_date_local = ASTRA::NoExists;
      airp_dep.clear();
      airp_arv.clear();
    }
  public:
    std::string subcls;
    int scd_day_local;
    TDateTime scd_date_local;
    std::string airp_dep;
    std::string airp_arv;

    TMktFlight()
    {
      init();
    }
    void clear()
    {
      TSimpleMktFlight::clear();
      init();
    }

    bool empty() const
    {
      return TSimpleMktFlight::empty() &&
             subcls.empty() &&
             scd_day_local == ASTRA::NoExists &&
             scd_date_local == ASTRA::NoExists &&
             airp_dep.empty() &&
             airp_arv.empty();
    }

    void getByPaxId(int pax_id);
    void getByCrsPaxId(int pax_id);
    void getByPnrId(int pnr_id);
    void dump() const;
};

class TGrpMktFlight : public TSimpleMktFlight
{
  private:
    void init()
    {
      scd_date_local = ASTRA::NoExists;
      airp_dep.clear();
      pr_mark_norms=false;
    }
  public:
    TDateTime scd_date_local;
    std::string airp_dep;
    bool pr_mark_norms;

    TGrpMktFlight()
    {
      init();
    }
    void clear()
    {
      TSimpleMktFlight::clear();
      init();
    }

    bool empty() const
    {
      return TSimpleMktFlight::empty() &&
             scd_date_local == ASTRA::NoExists &&
             airp_dep.empty() &&
             pr_mark_norms==false;
    }
    bool equalFlight(const TGrpMktFlight& flt) const
    {
      return TSimpleMktFlight::operator == (flt) &&
             scd_date_local == flt.scd_date_local &&
             airp_dep == flt.airp_dep;
    }

    const TGrpMktFlight& toXML(xmlNodePtr node) const;
    TGrpMktFlight& fromXML(xmlNodePtr node);
    const TGrpMktFlight& toDB(TQuery &Qry) const;
    TGrpMktFlight& fromDB(TQuery &Qry);
    bool getByGrpId(int grp_id);
};

std::optional<PointId_t> getPointIdByPaxId(const PaxId_t pax_id);

class TTripInfo
{
  private:
    void init()
    {
      point_id = ASTRA::NoExists;
      airline.clear();
      flt_no=0;
      suffix.clear();
      airp.clear();
      craft.clear();
      bort.clear();
      trip_type.clear();
      scd_out=ASTRA::NoExists;
      est_out=boost::none;
      act_out=boost::none;
      pr_del = ASTRA::NoExists;
      pr_reg = false;
      airline_fmt = efmtUnknown;
      suffix_fmt = efmtUnknown;
      airp_fmt = efmtUnknown;
      craft_fmt = efmtUnknown;
    };

    template<typename Query>
    void init( Query &Qry )
    {
      init();
      airline=Qry.FieldAsString("airline");
      flt_no=Qry.FieldAsInteger("flt_no");
      suffix=Qry.FieldAsString("suffix");
      airp=Qry.FieldAsString("airp");
      if (Qry.GetFieldIndex("craft")>=0)
        craft=Qry.FieldAsString("craft");
      if (Qry.GetFieldIndex("bort")>=0)
        bort=Qry.FieldAsString("bort");
      if (Qry.GetFieldIndex("trip_type")>=0)
        trip_type=Qry.FieldAsString("trip_type");
      if (!Qry.FieldIsNULL("scd_out"))
        scd_out = Qry.FieldAsDateTime("scd_out");
      if (Qry.GetFieldIndex("est_out")>=0)
        est_out = Qry.FieldIsNULL("est_out")?ASTRA::NoExists:
                                             Qry.FieldAsDateTime("est_out");
      if (Qry.GetFieldIndex("act_out")>=0)
        act_out = Qry.FieldIsNULL("act_out")?ASTRA::NoExists:
                                             Qry.FieldAsDateTime("act_out");
      if (Qry.GetFieldIndex("pr_del")>=0)
        pr_del = Qry.FieldAsInteger("pr_del");
      if (Qry.GetFieldIndex("pr_reg")>=0)
        pr_reg = Qry.FieldAsInteger("pr_reg")!=0;
      if (Qry.GetFieldIndex("airline_fmt")>=0)
        airline_fmt = (TElemFmt)Qry.FieldAsInteger("airline_fmt");
      if (Qry.GetFieldIndex("suffix_fmt")>=0)
        suffix_fmt = (TElemFmt)Qry.FieldAsInteger("suffix_fmt");
      if (Qry.GetFieldIndex("airp_fmt")>=0)
        airp_fmt = (TElemFmt)Qry.FieldAsInteger("airp_fmt");
      if (Qry.GetFieldIndex("craft_fmt")>=0)
        craft_fmt = (TElemFmt)Qry.FieldAsInteger("craft_fmt");
      if (Qry.GetFieldIndex("point_id")>=0)
          point_id = Qry.FieldAsInteger("point_id");
    };
  public:
    void fromArxPoint(const dbo::Arx_Points &arx_point);
    static bool match(TQuery &Qry, const FlightProps& props)
    {
      if (props.cancellation()==FlightProps::NotCancelled && Qry.FieldAsInteger("pr_del")!=0) return false;
      if (props.checkin_ability()==FlightProps::WithCheckIn && Qry.FieldAsInteger("pr_reg")==0) return false;
      return true;
    }
    static std::string selectedFields(const std::string& table_name="")
    {
      std::string prefix=table_name+(table_name.empty()?"":".");
      std::ostringstream s;
      s << " " << prefix << "point_id,"
        << " " << prefix << "airline,"
        << " " << prefix << "flt_no,"
        << " " << prefix << "suffix,"
        << " " << prefix << "airp,"
        << " " << prefix << "craft,"
        << " " << prefix << "bort,"
        << " " << prefix << "trip_type,"
        << " " << prefix << "scd_out,"
        << " " << prefix << "est_out,"
        << " " << prefix << "act_out,"
        << " " << prefix << "pr_del,"
        << " " << prefix << "pr_reg,"
        << " " << prefix << "airline_fmt,"
        << " " << prefix << "suffix_fmt,"
        << " " << prefix << "airp_fmt,"
        << " " << prefix << "craft_fmt ";
      return s.str();
    }

    int point_id;
    std::string airline, suffix, airp, craft, bort, trip_type;
    TDateTime scd_out;
    boost::optional<TDateTime> est_out, act_out; //��������!!! boost::none, �᫨ ����� �� �롨ࠥ��� �� ��; NoExists, �᫨ NULL � ��
    int flt_no, pr_del;
    bool pr_reg;
    TElemFmt airline_fmt, suffix_fmt, airp_fmt, craft_fmt;
    TTripInfo()
    {
      init();
    };
    TTripInfo( TQuery &Qry )
    {
      init(Qry);
    };
    TTripInfo( DB::TQuery &Qry )
    {
      init(Qry);
    };
    virtual ~TTripInfo() {};
    virtual void Clear()
    {
      init();
    };
    virtual void Init( TQuery &Qry )
    {
      init(Qry);
    }
    virtual void Init( DB::TQuery &Qry )
    {
      init(Qry);
    }
  public:
    virtual bool getByPointId (const TDateTime part_key, const int point_id,
                               const FlightProps& props = FlightProps() );
    virtual bool getByPointId ( const int point_id, const FlightProps& props = FlightProps() );
    virtual bool getByPointIdTlg ( const int point_id_tlg );
    virtual bool getByPaxId ( const int pax_id );
    virtual bool getByGrpId ( const int grp_id );
    virtual bool getByCRSPnrId ( const int pnr_id );
    virtual bool getByCRSPaxId ( const int pax_id );
    void get_client_dates(TDateTime &scd_out_client, TDateTime &real_out_client, bool trunc_time=true) const;
    static std::tuple<TDateTime, TDateTime, TDateTime> get_times_in(const int &point_arv);
    static TDateTime get_scd_in(const int &point_arv);
    static TDateTime act_est_scd_in(const int &point_arv);
    std::tuple<TDateTime, TDateTime, TDateTime> get_times_in(const std::string &airp_arv) const;
    std::string flight_view(TElemContext ctxt=ecNone, bool showScdOut=true, bool showAirp=true) const;
    TDateTime est_scd_out() const { return !est_out?ASTRA::NoExists:
                                           est_out.get()!=ASTRA::NoExists?est_out.get():
                                                                          scd_out; }
    TDateTime act_est_scd_out() const { return !act_out||!est_out?ASTRA::NoExists:
                                               act_out.get()!=ASTRA::NoExists?act_out.get():
                                               est_out.get()!=ASTRA::NoExists?est_out.get():
                                                                              scd_out; }
    bool scd_out_exists() const { return scd_out!=ASTRA::NoExists; }
    bool est_out_exists() const { return est_out && est_out.get()!=ASTRA::NoExists; }
    bool act_out_exists() const { return act_out && act_out.get()!=ASTRA::NoExists; }

    bool match(const FlightProps& props) const
    {
      if (props.cancellation()==FlightProps::NotCancelled && pr_del!=0) return false;
      if (props.checkin_ability()==FlightProps::WithCheckIn && !pr_reg) return false;
      return true;
    }

    bool match(const TAccess& access) const
    {
      return access.airlines().permitted(airline) &&
             access.airps().permitted(airp);
    }

    std::string flight_number(const boost::optional<AstraLocale::OutputLang>& lang = boost::none) const
    {
      std::ostringstream s;
      s << std::setw(3) << std::setfill('0') << flt_no;
      s << (lang? ElemIdToElem(etSuffix, suffix, efmtCodeNative, lang->get()): suffix);
      return s.str();
    }

    TGrpMktFlight grpMktFlight() const;
};

std::string flight_view(int grp_id, int seg_no); //��稭�� � 1

std::string GetTripDate( const TTripInfo &info, const std::string &separator, const bool advanced_trip_list  );
std::string GetTripName( const TTripInfo &info, TElemContext ctxt, bool showAirp=false, bool prList=false );

class TAdvTripInfo : public TTripInfo
{
  private:
    void init()
    {
      point_id=ASTRA::NoExists;
      point_num=ASTRA::NoExists;
      first_point=ASTRA::NoExists;
      pr_tranzit=false;
    };
    template<typename Query>
    void init( Query &Qry )
    {
      point_id = Qry.FieldAsInteger("point_id");
      point_num = Qry.FieldAsInteger("point_num");
      first_point = Qry.FieldIsNULL("first_point")?ASTRA::NoExists:Qry.FieldAsInteger("first_point");
      pr_tranzit = Qry.FieldAsInteger("pr_tranzit")!=0;
    }
  public:
    static std::string selectedFields(const std::string& table_name="")
    {
      std::string prefix=table_name+(table_name.empty()?"":".");
      std::ostringstream s;
      s << " " << prefix << "point_num,"
        << " " << prefix << "first_point,"
        << " " << prefix << "pr_tranzit,"
        << TTripInfo::selectedFields(table_name);
      return s.str();
    }

    int point_id, point_num, first_point;
    bool pr_tranzit;
    TAdvTripInfo()
    {
      init();
    };
    TAdvTripInfo( TQuery &Qry ) : TTripInfo(Qry)
    {
      init(Qry);
    }
    TAdvTripInfo( DB::TQuery &Qry ) : TTripInfo(Qry)
    {
      init(Qry);
    }
    TAdvTripInfo( const TTripInfo &info,
                  int p_point_id,
                  int p_point_num,
                  int p_first_point,
                  bool p_pr_tranzit) : TTripInfo(info)
    {
      point_id=p_point_id;
      point_num=p_point_num;
      first_point=p_first_point;
      pr_tranzit=p_pr_tranzit;
    };
    virtual ~TAdvTripInfo() {};
    virtual void Clear()
    {
      TTripInfo::Clear();
      init();
    };
    using TTripInfo::Init;
    virtual void Init( TQuery &Qry )
    {
      TTripInfo::Init(Qry);
      init(Qry);
    };
    virtual bool getByPointId ( const TDateTime part_key,
                                const int point_id,
                                const FlightProps& props = FlightProps() );
    virtual bool getByPointId ( const int point_id, const FlightProps& props = FlightProps() );
    static bool transitable(const PointId_t& pointId);
    bool transitable() const
    {
      if (point_id==ASTRA::NoExists) return false;
      return transitable(PointId_t(point_id));
    }
};

typedef std::list<TAdvTripInfo> TAdvTripInfoList;
typedef ASTRA::Cache<int/*pnr_id*/, TAdvTripInfoList> PnrFlightsCache;

std::set<PointIdTlg_t> getPointIdTlgByPointIdsSpp(const PointId_t point_id_spp);
std::optional<PointIdTlg_t> getPointIdTlgByPaxId(const PaxId_t pax_id, bool with_deleted);
void getPointIdsSppByPointIdTlg(const PointIdTlg_t& point_id_tlg,
                                std::set<PointId_t>& point_ids_spp,
                                bool clear = true);
std::set<PointId_t> getPointIdsSppByPointIdTlg(const PointIdTlg_t& point_id_tlg);
void getTripsByPointIdTlg(const int point_id_tlg, TAdvTripInfoList &trips);
void getTripsByCRSPnrId(const int pnr_id, TAdvTripInfoList &trips);
void getTripsByCRSPaxId(const int pax_id, TAdvTripInfoList &trips);

class TLastTrferInfo
{
  public:
    std::string airline, suffix, airp_arv;
    int flt_no;
    TLastTrferInfo()
    {
      Clear();
    };
    TLastTrferInfo( TQuery &Qry )
    {
      Init(Qry);
    };
    void Clear()
    {
      airline.clear();
      flt_no=ASTRA::NoExists;
      suffix.clear();
      airp_arv.clear();
    };
    virtual void Init( TQuery &Qry )
    {
      airline=Qry.FieldAsString("trfer_airline");
      if (!Qry.FieldIsNULL("trfer_flt_no"))
        flt_no=Qry.FieldAsInteger("trfer_flt_no");
      else
        flt_no=ASTRA::NoExists;
      suffix=Qry.FieldAsString("trfer_suffix");
      airp_arv=Qry.FieldAsString("trfer_airp_arv");
    };
    bool IsNULL()
    {
      return (airline.empty() &&
              flt_no==ASTRA::NoExists &&
              suffix.empty() &&
              airp_arv.empty());
    };
    std::string str();
    virtual ~TLastTrferInfo() {};
};

class TLastTCkinSegInfo : public TLastTrferInfo
{
  public:
    TLastTCkinSegInfo():TLastTrferInfo() {};
    TLastTCkinSegInfo( TQuery &Qry ):TLastTrferInfo()
    {
      Init(Qry);
    };
    virtual void Init( TQuery &Qry )
    {
      airline=Qry.FieldAsString("tckin_seg_airline");
      if (!Qry.FieldIsNULL("tckin_seg_flt_no"))
        flt_no=Qry.FieldAsInteger("tckin_seg_flt_no");
      else
        flt_no=ASTRA::NoExists;
      suffix=Qry.FieldAsString("tckin_seg_suffix");
      airp_arv=Qry.FieldAsString("tckin_seg_airp_arv");
    };
};

//��楤�� ��ॢ��� �⤥�쭮�� ��� (��� ����� � ����) � �����業�� TDateTime
//��� ��������� ��� ᮢ�������� ���� �� �⭮襭�� � base_date
//��ࠬ��� back - ���ࠢ����� ���᪠ (true - � ��諮� �� base_date, false - � ���饥)
TDateTime DayToDate(int day, TDateTime base_date, bool back);

enum TDateDirection { dateBefore, dateAfter, dateEverywhere };
TDateTime DayMonthToDate(int day, int month, TDateTime base_date, TDateDirection dir);

struct TTripRouteItem
{
  TDateTime part_key;
  int point_id;
  int point_num;
  std::string airp;
  bool pr_cancel;
  TTripRouteItem()
  {
    Clear();
  }
  void Clear()
  {
    part_key = ASTRA::NoExists;
    point_id = ASTRA::NoExists;
    point_num = ASTRA::NoExists;
    airp.clear();
    pr_cancel = true;
  }
  bool suitable(const PointId_t& pointId) const
  {
    return pointId.get()==point_id;
  }
  bool suitable(const AirportCode_t& airpCode) const
  {
    return airpCode.get()==airp;
  }
};

struct TAdvTripRouteItem : TTripRouteItem
{
  TDateTime scd_in, scd_out, act_out;
  std::string airline_out, suffix_out;
  int flt_num_out;

  TAdvTripRouteItem() : TTripRouteItem()
  {
    Clear();
  }
  void Clear()
  {
    scd_in = ASTRA::NoExists;
    scd_out = ASTRA::NoExists;
    act_out = ASTRA::NoExists;
    airline_out.clear();
    suffix_out.clear();
    flt_num_out = ASTRA::NoExists;
  }
  TDateTime scd_in_local() const
  {
    if (scd_in!=ASTRA::NoExists)
      return BASIC::date_time::UTCToLocal(scd_in, AirpTZRegion(airp));
    return ASTRA::NoExists;
  }
  std::string scd_in_local(const std::string& fmt) const
  {
    if (scd_in==ASTRA::NoExists) return "";
    return BASIC::date_time::DateTimeToStr(scd_in_local(), fmt);
  }
  TDateTime scd_out_local() const
  {
    if (scd_out!=ASTRA::NoExists)
      return BASIC::date_time::UTCToLocal(scd_out, AirpTZRegion(airp));
    return ASTRA::NoExists;
  }
  std::string scd_out_local(const std::string& fmt) const
  {
    if (scd_out==ASTRA::NoExists) return "";
    return BASIC::date_time::DateTimeToStr(scd_out_local(), fmt);
  }
  std::string flight_number(const boost::optional<AstraLocale::OutputLang>& lang) const
  {
    if (flt_num_out==ASTRA::NoExists) return "";
    std::ostringstream s;
    s << std::setw(3) << std::setfill('0') << flt_num_out
      << (lang? ElemIdToPrefferedElem(etSuffix, suffix_out, efmtCodeNative, lang->get()): suffix_out);
    return s.str();
  }
  bool match(const TAccess& access) const
  {
    return access.airlines().permitted(airline_out) &&
           access.airps().permitted(airp);
  }

};

//��᪮�쪮 ���� �����⮢ ��� ���짮����� �㭪権 ࠡ��� � ������⮬:
//point_id = points.point_id
//first_point = points.first_point
//point_num = points.point_num
//pr_tranzit = points.pr_tranzit
//TTripRouteType1 = ������� � ������� �㭪� � point_id
//TTripRouteType2 = ������� � ������� �⬥����� �㭪��

//�㭪樨 �������� false, �᫨ � ⠡��� points �� ������ point_id

enum TTripRouteType1 { trtNotCurrent,
                       trtWithCurrent };
enum TTripRouteType2 { trtNotCancelled,
                       trtWithCancelled };

class TTripBase
{
private:
  virtual void GetRoute(TDateTime part_key,
                int point_id,
                int point_num,
                int first_point,
                bool pr_tranzit,
                bool after_current,
                TTripRouteType1 route_type1,
                TTripRouteType2 route_type2,
                TQuery& Qry) = 0;
  virtual bool GetRoute(TDateTime part_key,
                int point_id,
                bool after_current,
                TTripRouteType1 route_type1,
                TTripRouteType2 route_type2) = 0;
public:
  //������� ��᫥ �㭪� point_id
  bool GetRouteAfter(TDateTime part_key,
                     int point_id,
                     TTripRouteType1 route_type1,
                     TTripRouteType2 route_type2);
  void GetRouteAfter(TDateTime part_key,
                     int point_id,
                     int point_num,
                     int first_point,
                     bool pr_tranzit,
                     TTripRouteType1 route_type1,
                     TTripRouteType2 route_type2);
  void GetRouteAfter(const TAdvTripInfo& fltInfo,
                     TTripRouteType1 route_type1,
                     TTripRouteType2 route_type2);
  //������� �� �㭪� point_id
  bool GetRouteBefore(TDateTime part_key,
                      int point_id,
                      TTripRouteType1 route_type1,
                      TTripRouteType2 route_type2);
  void GetRouteBefore(TDateTime part_key,
                      int point_id,
                      int point_num,
                      int first_point,
                      bool pr_tranzit,
                      TTripRouteType1 route_type1,
                      TTripRouteType2 route_type2);
  void GetRouteBefore(const TAdvTripInfo& fltInfo,
                      TTripRouteType1 route_type1,
                      TTripRouteType2 route_type2);
  virtual ~TTripBase() {}
};

class TTripRoute : public TTripBase, public std::vector<TTripRouteItem>

{
  private:
    virtual void GetRoute(TDateTime part_key,
                  int point_id,
                  int point_num,
                  int first_point,
                  bool pr_tranzit,
                  bool after_current,
                  TTripRouteType1 route_type1,
                  TTripRouteType2 route_type2,
                  TQuery& Qry);
    void GetArxRoute(TDateTime part_key,
                     int point_id,
                     int point_num,
                     int first_point,
                     bool pr_tranzit,
                     bool after_current,
                     TTripRouteType1 route_type1,
                     TTripRouteType2 route_type2);

    virtual bool GetRoute(TDateTime part_key,
                int point_id,
                bool after_current,
                TTripRouteType1 route_type1,
                TTripRouteType2 route_type2);
    bool GetArxRoute(TDateTime part_key,
                     int point_id,
                     bool after_current,
                     TTripRouteType1 route_type1,
                     TTripRouteType2 route_type2);
  public:
    //�����頥� ᫥���騩 �㭪� �������
    void GetNextAirp(TDateTime part_key,
                     int point_id,
                     int point_num,
                     int first_point,
                     bool pr_tranzit,
                     TTripRouteType2 route_type2,
                     TTripRouteItem& item);
    bool GetNextAirp(TDateTime part_key,
                     int point_id,
                     TTripRouteType2 route_type2,
                     TTripRouteItem& item);

    //�����頥� �।��騩 �㭪� �������
    void GetPriorAirp(TDateTime part_key,
                      int point_id,
                      int point_num,
                      int first_point,
                      bool pr_tranzit,
                      TTripRouteType2 route_type2,
                      TTripRouteItem& item);
    bool GetPriorAirp(TDateTime part_key,
                      int point_id,
                      TTripRouteType2 route_type2,
                      TTripRouteItem& item);
    boost::optional<TTripRouteItem> findFirstAirp(const std::string& airp) const
    {
      for(const TTripRouteItem& item : *this)
        if (item.airp==airp) return item;
      return boost::none;
    }
    boost::optional<TTripRouteItem> getFirstAirp() const
    {
      if (!empty()) return front();
      return boost::none;
    }

    std::string GetStr() const;
    virtual ~TTripRoute() {}
};

class TAdvTripRoute : public TTripBase, public std::vector<TAdvTripRouteItem>
{
  private:
    virtual void GetRoute(TDateTime part_key,
                  int point_id,
                  int point_num,
                  int first_point,
                  bool pr_tranzit,
                  bool after_current,
                  TTripRouteType1 route_type1,
                  TTripRouteType2 route_type2,
                  TQuery& Qry);

    void GetArxRoute(TDateTime part_key,
                        int point_id,
                        int point_num,
                        int first_point,
                        bool pr_tranzit,
                        bool after_current,
                        TTripRouteType1 route_type1,
                        TTripRouteType2 route_type2);

    virtual bool GetRoute(TDateTime part_key,
                int point_id,
                bool after_current,
                TTripRouteType1 route_type1,
                TTripRouteType2 route_type2);
    bool GetArxRoute(TDateTime part_key,
                     int point_id,
                     bool after_current,
                     TTripRouteType1 route_type1,
                     TTripRouteType2 route_type2);

    template <class T>
    void trimRoute(const T& criterion)
    {
      TAdvTripRoute::iterator i=std::find_if(begin(), end(),
                                             [&criterion](const auto& i) { return i.suitable(criterion); });
      if (i!=end()) {
          erase(++i, end());
      }
      else {
          clear();
      }
    }
  public:
    template <class T>
    void getRouteBetween(const TAdvTripInfo& fltInfo, const T& criterion)
    {
      clear();
      GetRouteAfter(fltInfo, trtWithCurrent, trtNotCancelled);
      trimRoute(criterion);
    }

    void getRouteBetween(const TPaxSegmentPair& segmentPair)
    {
      clear();
      GetRouteAfter(ASTRA::NoExists, segmentPair.point_dep, trtWithCurrent, trtNotCancelled);
      trimRoute(segmentPair.airpArv());
    }

    virtual ~TAdvTripRoute() {}
};

class TPaxSeats {
    private:
      int pr_lat_seat;
      TQuery *Qry;
    public:
        TPaxSeats( int point_id );
        std::string getSeats( int pax_id, const std::string format );
    ~TPaxSeats();
};

struct TTrferRouteItem
{
  TTripInfo operFlt;
  std::string airp_arv;
  TElemFmt airp_arv_fmt;
  boost::optional<bool> piece_concept;
  TTrferRouteItem()
  {
    Clear();
  };
  void Clear()
  {
    operFlt.Clear();
    airp_arv.clear();
    airp_arv_fmt=efmtUnknown;
    piece_concept=boost::none;
  };
};

enum TTrferRouteType { trtNotFirstSeg,
                       trtWithFirstSeg };

class TTrferRoute : public std::vector<TTrferRouteItem>
{
  public:
    bool GetRoute(int grp_id,
                  TTrferRouteType route_type);
};

struct TCkinRouteItem
{
  int grp_num, seg_no, transit_num;
  bool pr_depend;

  int grp_id;
  int point_dep, point_arv;
  std::string airp_dep, airp_arv;
  ASTRA::TPaxStatus status;
  TTripInfo operFlt;

  TCkinRouteItem(TQuery &Qry);
};

struct GrpRouteItem
{
  GrpId_t grp_id;
  PointId_t point_id;
  int tckin_id;
  int seg_no;
};

struct GrpRoute
{
  GrpRouteItem src;
  GrpRouteItem dest;

  static std::vector<GrpRoute> load(int transfer_num,
                                    const GrpId_t& grp_id_src,
                                    const GrpId_t& grp_id_dest);
};

struct PaxGrpRouteItem: public GrpRouteItem
{
  PaxId_t pax_id;
  int distance;
};

struct PaxGrpRoute
{
  PaxGrpRouteItem src;
  PaxGrpRouteItem dest;

  static std::vector<PaxGrpRoute> load(const PaxId_t& pax_id,
                                       int transfer_num,
                                       const GrpId_t& grp_id_src,
                                       const GrpId_t& grp_id_dest);
};

using TCkinGrpIds = std::list<GrpId_t>;

class TCkinRouteInsertItem
{
  public:
    GrpId_t grpId;
    boost::optional<RegNo_t> firstRegNo; //��� ��ᮯ஢��������� ������ none
    ASTRA::TPaxStatus status;

    TCkinRouteInsertItem(const GrpId_t& grpId_,
                         const boost::optional<RegNo_t>& firstRegNo_,
                         const ASTRA::TPaxStatus status_) :
      grpId(grpId_), firstRegNo(firstRegNo_), status(status_) {}
};

class TCkinRoute : public std::vector<TCkinRouteItem>
{
  public:
    enum Currentity  { NotCurrent,
                       WithCurrent };
    enum Dependence  { OnlyDependent,
                       IgnoreDependence };
    enum GroupStatus { WithoutTransit,
                       WithTransit };
  private:
    enum Direction   { Full, Before, After };

    static std::string getSelectSQL(const Direction direction,
                                    const std::string& subselect);

    boost::optional<int> getRoute(const int tckin_id,
                                  const int grp_num,
                                  const Direction direction);
    boost::optional<int> getRoute(const GrpId_t& grpId,
                                  const Direction direction);
    boost::optional<int> getRoute(const PaxId_t& paxId,
                                  const Direction direction);

    void applyFilter(const boost::optional<int>& current_grp_num,
                     const Currentity currentity,
                     const Dependence dependence,
                     const GroupStatus groupStatus);
  public:
    bool getRoute(const PaxId_t& paxId);                 //१����=false ⮫쪮 �᫨ ��� grp_id �� �ந��������� ᪢����� ॣ������!

    bool getRoute(const GrpId_t& grpId,
                  const Currentity currentity,
                  const Dependence dependence,
                  const GroupStatus groupStatus);   //१����=false ⮫쪮 �᫨ ��� grp_id �� �ந��������� ᪢����� ॣ������!

    bool getRouteAfter(const GrpId_t& grpId,
                       const Currentity currentity,
                       const Dependence dependence,
                       const GroupStatus groupStatus);   //१����=false ⮫쪮 �᫨ ��� grp_id �� �ந��������� ᪢����� ॣ������!

    //᪢����� ������� �� ��몮��� grp_id
    bool getRouteBefore(const GrpId_t& grpId,
                        const Currentity currentity,
                        const Dependence dependence,
                        const GroupStatus groupStatus);  //१����=false ⮫쪮 �᫨ ��� grp_id �� �ந��������� ᪢����� ॣ������!

    //�����頥� �।��騩 ᥣ���� ��몮��� (�.�. �஬������ �࠭���� ᥣ����)
    static boost::optional<TCkinRouteItem> getPriorGrp(const int tckin_id,
                                                       const int grp_num,
                                                       const Dependence dependence,
                                                       const GroupStatus groupStatus);

    static boost::optional<TCkinRouteItem> getPriorGrp(const GrpId_t& grpId,
                                                       const Dependence dependence,
                                                       const GroupStatus groupStatus);
    //�����頥� ᫥���騩 ᥣ���� ��몮��� (�.�. �஬������ �࠭���� ᥣ����)
    static boost::optional<TCkinRouteItem> getNextGrp(const GrpId_t& grpId,
                                                      const Dependence dependence,
                                                      const GroupStatus groupStatus);

    TCkinGrpIds getTCkinGrpIds() const
    {
      return algo::transform<TCkinGrpIds>(*this, [](const auto& i) { return GrpId_t(i.grp_id); });
    }

    static boost::optional<GrpId_t> toDB(const std::list<TCkinRouteInsertItem>& tckinGroups);

    static std::string copySubselectSQL(const std::string& mainTable,
                                        const std::initializer_list<std::string>& otherTables,
                                        const bool forEachPassenger);
};

enum TCkinSegmentSet { cssNone,
                       cssAllPrev,
                       cssAllPrevCurr,
                       cssAllPrevCurrNext,
                       cssCurr };

//�����頥� tckin_id
int SeparateTCkin(int grp_id,
                  TCkinSegmentSet upd_depend,
                  TCkinSegmentSet upd_tid,
                  int tid);

void CheckTCkinIntegrity(const std::set<int> &tckin_ids, int tid);

struct TCodeShareSets {
  private:
    TQuery *Qry;
  public:
    //����ன��
    bool pr_mark_norms;
    bool pr_mark_bp;
    bool pr_mark_rpt;
    TCodeShareSets();
    ~TCodeShareSets();
    void clear()
    {
      pr_mark_norms=false;
      pr_mark_bp=false;
      pr_mark_rpt=false;
    }
    void get(const TTripInfo &operFlt,
             const TTripInfo &markFlt,
             bool is_local_scd_out=false);
};

void GetMktFlights(const TTripInfo &operFltInfo, TSimpleMktFlights &simpleMktFlights);
//�����! �६� �뫥� scd_out � operFlt ������ ���� � UTC
//       return_scd_utc=false: �६� �뫥� � markFltInfo �����頥��� �����쭮� �⭮�⥫쭮 airp
//       return_scd_utc=true: �६� �뫥� � markFltInfo �����頥��� � UTC
void GetMktFlights(const TTripInfo &operFltInfo, std::vector<TTripInfo> &markFltInfo, bool return_scd_utc=false);

//�����! �६� �뫥� scd_out � operFlt ������ ���� � UTC
//       �६� �뫥� � markFltInfo ��।����� �����쭮� �⭮�⥫쭮 airp
std::string GetMktFlightStr( const TTripInfo &operFlt, const TTripInfo &markFlt, bool &equal);  //!!!vlad ��।�����

std::set<std::string> GetCrsList(const PointId_t& point_id);
bool IsRouteInter(int point_dep, int point_arv, std::string &country);
bool IsTrferInter(std::string airp_dep, std::string airp_arv, std::string &country);

std::string GetRouteAfterStr(TDateTime part_key,  //NoExists �᫨ � ����⨢��� ����, ���� � ��娢���
                             int point_id,
                             TTripRouteType1 route_type1,
                             TTripRouteType2 route_type2,
                             const std::string &lang="",
                             bool show_city_name=false,
                             const std::string &separator="-");

struct TInfantAdults {
  int grp_id;
  int pax_id;
  int reg_no;
  std::string surname;
  int parent_pax_id;
  int temp_parent_id;
  void clear();
  void fromDB(TQuery &Qry);
  TInfantAdults(TQuery &Qry);
  TInfantAdults() { clear(); };
};

template <class T1>
bool ComparePass( const T1 &item1, const T1 &item2 )
{
  return item1.reg_no < item2.reg_no;
};

/* ������ ���� ��������� ���� � ⨯� T1:
  grp_id, pax_id, reg_no, surname, parent_pax_id, temp_parent_id - �� ⠡���� crs_inf,
  � ⨯� T2:
  grp_id, pax_id, reg_no, surname */
template <class T1, class T2>
void SetInfantsToAdults( std::vector<T1> &InfItems, std::vector<T2> AdultItems )
{
  sort( InfItems.begin(), InfItems.end(), ComparePass<T1> );
  sort( AdultItems.begin(), AdultItems.end(), ComparePass<T2> );
  for ( int k = 1; k <= 4; k++ ) {
    for(typename std::vector<T1>::iterator infRow = InfItems.begin(); infRow != InfItems.end(); infRow++) {
      if ( k != 1 && infRow->temp_parent_id != ASTRA::NoExists ) {
        continue;
      }
      infRow->temp_parent_id = ASTRA::NoExists;
      for(typename std::vector<T2>::iterator adultRow = AdultItems.begin(); adultRow != AdultItems.end(); adultRow++) {
        if(
           (infRow->grp_id == adultRow->grp_id) and
           ((k == 1 and infRow->reg_no == adultRow->reg_no) or
            (k == 2 and infRow->parent_pax_id == adultRow->pax_id) or
            (k == 3 and infRow->surname == adultRow->surname) or
            (k == 4))
          ) {
              infRow->temp_parent_id = adultRow->pax_id;
              infRow->parent_pax_id = adultRow->pax_id;
              AdultItems.erase(adultRow);
              break;
            }
      }
    }
  }
}

template <class T1>
bool AdultsWithBaby( int adult_id, const std::vector<T1> &InfItems )
{
  for(typename std::vector<T1>::const_iterator infRow = InfItems.begin(); infRow != InfItems.end(); infRow++) {
    if ( infRow->parent_pax_id == adult_id ) {
      return true;
    }
  }
  return false;
}

bool is_sync_FileParamSets( const TTripInfo &tripInfo, const std::string& syncType );
bool is_sync_paxs( int point_id );
bool is_sync_flights( int point_id );
void update_pax_change( int point_id, int pax_id, int reg_no, const std::string &work_mode );
void update_pax_change( const TTripInfo &fltInfo, int pax_id, int reg_no, const std::string &work_mode );
void update_flights_change( int point_id );

class TPaxNameTitle
{
  public:
    std::string title;
    bool is_female;
    TPaxNameTitle() { clear(); };
    void clear() { title.clear(); is_female=false; };
    bool empty() const { return title.empty(); };
};

const std::map<std::string, TPaxNameTitle>& pax_name_titles();
bool GetPaxNameTitle(std::string &name, bool truncate, TPaxNameTitle &info);
std::string TruncNameTitles(const std::string &name);

std::string SeparateNames(std::string &names);

int CalcWeightInKilos(int weight, std::string weight_unit);

struct TCFGItem {
    int priority;
    std::string cls;
    int cfg, block, prot;
    TCFGItem():
        priority(ASTRA::NoExists),
        cfg(ASTRA::NoExists),
        block(ASTRA::NoExists),
        prot(ASTRA::NoExists)
    {};
    bool operator < (const TCFGItem &i) const
    {
        if(priority != i.priority)
            return priority < i.priority;
        if(cls != i.cls)
            return cls < i.cls;
        if(cfg != i.cfg)
            return cfg < i.cfg;
        if(block != i.block)
            return block < i.block;
        return prot < i.prot;
    }

    bool operator == (const TCFGItem &i) const
    {
        return
            priority == i.priority and
            cls == i.cls and
            cfg == i.cfg and
            block == i.block and
            prot == i.prot;
    }

};

struct TCFG:public std::vector<TCFGItem> {
    void get(int point_id, TDateTime part_key = ASTRA::NoExists); //NoExists �᫨ � ����⨢��� ����, ���� � ��娢���
    void get_arx(int point_id, TDateTime part_key);
    std::string str(const std::string &lang="", const std::string &separator=" ");
    void param(LEvntPrms& params);
    TCFG(int point_id, TDateTime part_key = ASTRA::NoExists) { get(point_id, part_key); };
    TCFG() {};
};

class FltMarkFilter
{
  public:
    enum DateType { Local, UTC };

  private:
    AirlineCode_t airline_;
    FlightNumber_t fltNumber_;
    FlightSuffix_t suffix_;
    AirportCode_t airpDep_;
    TDateTime scdDateDepLocalTruncated_;

  public:
    FltMarkFilter(const AirlineCode_t& airline,
                  const FlightNumber_t& fltNumber,
                  const FlightSuffix_t& suffix,
                  const AirportCode_t& airpDep,
                  const TDateTime scdDateDep,
                  const DateType scdDateDepInUTC);

    std::set<PointIdMkt_t> search() const;
};

class FltOperFilter
{
  public:
    enum DateFlag { Est, Act };
    enum DateType { Local, UTC };

    using DateFlags = std::set<DateFlag>;

  private:
    AirlineCode_t airline_;
    FlightNumber_t fltNumber_;
    FlightSuffix_t suffix_;
    std::optional<AirportCode_t> airpDepOpt_;
    TDateTime dateDepTruncated_;
    bool dateDepInUTC_;
    DateFlags dateDepFlags_;
    FlightProps fltProps_;
    std::string additionalWhere_;

    bool suitable(const AirportCode_t& airpDep,
                  const TDateTime timeDepInUTC) const;

  public:
    FltOperFilter(const AirlineCode_t& airline,
                  const FlightNumber_t& fltNumber,
                  const FlightSuffix_t& suffix,
                  const std::optional<AirportCode_t>& airpDep,
                  const TDateTime dateDep,
                  const DateType dateDepType,
                  const std::set<DateFlag>& dateDepFlags={},
                  const FlightProps& fltProps=FlightProps());

    void setAdditionalWhere(const std::string& additionalWhere) { additionalWhere_=additionalWhere; }

    AirlineCode_t airline() const { return airline_; }
    std::optional<AirportCode_t> airpDep() const { return airpDepOpt_; }
    TDateTime dateDep() const { return dateDepTruncated_; }

    std::list<TAdvTripInfo> search() const;
};

// �㭪樨 �ࠢ������ �������� ��㯮�冷祭���� list ��� vector
template <class T>
bool compareVectors(std::vector<T> a, std::vector<T> b)
{
    sort(a.begin(), a.end());
    sort(b.begin(), b.end());
    return a == b;
}

template <class T>
bool compareLists(const std::list<T> &a, const std::list<T> &b)
{
    return compareVectors(std::vector<T>(a.begin(), a.end()), std::vector<T>(b.begin(), b.end()));
}

TDateTime getTimeTravel(const std::string &craft, const std::string &airp, const std::string &airp_last);

double getFileSizeDouble(const std::string &str);
std::string getFileSizeStr(double size);
AstraLocale::LexemaData GetLexemeDataWithFlight(const AstraLocale::LexemaData &data, const TTripInfo &fltInfo);
AstraLocale::LexemaData GetLexemeDataWithRegNo(const AstraLocale::LexemaData &data, int reg_no);

#endif /*_ASTRA_MISC_H_*/

