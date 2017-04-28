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

using BASIC::date_time::TDateTime;

class TSimpleMktFlight
{
  private:
    void init()
    {
      airline.clear();
      flt_no=ASTRA::NoExists;
      suffix.clear();
    };
  public:
    std::string airline;
    int flt_no;
    std::string suffix;
    TSimpleMktFlight() {init();};
    void clear()
    {
      init();
    };
    bool empty() const
    {
      return airline.empty() &&
             flt_no==ASTRA::NoExists &&
             suffix.empty();
    };
    virtual ~TSimpleMktFlight() {}
};

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

    bool operator == (const TSimpleMktFlight &s) const
    {
      return airline == s.airline &&
             flt_no == s.flt_no &&
             suffix == s.suffix;
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

    const TGrpMktFlight& toXML(xmlNodePtr node) const;
    TGrpMktFlight& fromXML(xmlNodePtr node);
    const TGrpMktFlight& toDB(TQuery &Qry) const;
    TGrpMktFlight& fromDB(TQuery &Qry);
    bool getByGrpId(int grp_id);
};

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
      scd_out=ASTRA::NoExists;
      real_out=ASTRA::NoExists;
      pr_del = ASTRA::NoExists;
      airline_fmt = efmtUnknown;
      suffix_fmt = efmtUnknown;
      airp_fmt = efmtUnknown;
      craft_fmt = efmtUnknown;
    };
    void init( TQuery &Qry )
    {
      init();
      airline=Qry.FieldAsString("airline");
      flt_no=Qry.FieldAsInteger("flt_no");
      suffix=Qry.FieldAsString("suffix");
      airp=Qry.FieldAsString("airp");
      if (Qry.GetFieldIndex("craft")>=0)
        craft=Qry.FieldAsString("craft");
      if (!Qry.FieldIsNULL("scd_out"))
        scd_out = Qry.FieldAsDateTime("scd_out");
      if (Qry.GetFieldIndex("real_out")>=0 && !Qry.FieldIsNULL("real_out"))
        real_out = Qry.FieldAsDateTime("real_out");
      if (Qry.GetFieldIndex("pr_del")>=0)
        pr_del = Qry.FieldAsInteger("pr_del");
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
    void init( const TGrpMktFlight &flt )
    {
      init();
      airline=flt.airline;
      flt_no=flt.flt_no;
      suffix=flt.suffix;
      scd_out=flt.scd_date_local;
      airp=flt.airp_dep;
    }
    void init( const TMktFlight &flt )
    {
      init();
      airline=flt.airline;
      flt_no=flt.flt_no;
      suffix=flt.suffix;
      scd_out=flt.scd_date_local;
      airp=flt.airp_dep;
    }
  public:
    int point_id;
    std::string airline, suffix, airp, craft;
    int flt_no, pr_del;
    TElemFmt airline_fmt, suffix_fmt, airp_fmt, craft_fmt;
    TDateTime scd_out, real_out;
    TTripInfo()
    {
      init();
    };
    TTripInfo( TQuery &Qry )
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
    };
    virtual void Init( const TGrpMktFlight &flt )
    {
      init(flt);
    };
    virtual void Init( const TMktFlight &flt )
    {
      init(flt);
    };
    virtual bool getByPointId ( const int point_id );
    virtual bool getByPointIdTlg ( const int point_id_tlg );
    virtual bool getByGrpId ( const int grp_id );
    virtual bool getByCRSPnrId ( const int pnr_id );
    virtual bool getByCRSPaxId ( const int pax_id );
    void get_client_dates(TDateTime &scd_out_client, TDateTime &real_out_client, bool trunc_time=true) const;
    static TDateTime get_scd_in(const int &point_arv);
    TDateTime get_scd_in(const std::string &airp_arv) const;
    std::string flight_view(TElemContext ctxt=ecNone, bool showScdOut=true, bool showAirp=true) const;
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
    void init( TQuery &Qry )
    {
      point_id = Qry.FieldAsInteger("point_id");
      point_num = Qry.FieldAsInteger("point_num");
      first_point = Qry.FieldIsNULL("first_point")?ASTRA::NoExists:Qry.FieldAsInteger("first_point");
      pr_tranzit = Qry.FieldAsInteger("pr_tranzit")!=0;
    };
  public:
    int point_id, point_num, first_point;
    bool pr_tranzit;
    TAdvTripInfo()
    {
      init();
    };
    TAdvTripInfo( TQuery &Qry ) : TTripInfo(Qry)
    {
      init(Qry);
    };
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
    virtual bool getByPointId ( const int point_id );
};

typedef std::list<TAdvTripInfo> TAdvTripInfoList;

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

//����ன�� ३�
enum TTripSetType { /*�� �ਢ易��� � ३��*/
                    tsCraftInitVIP=1,               //��⮬���᪠� ࠧ��⪠ VIP-���� � ����������
                    tsEDSNoInteract=10,             //����� ���ࠪ⨢� � �����.
                    tsETSNoInteract=11,             //����� ���ࠪ⨢� � �����. ���쪮 ETL.
                    tsIgnoreTrferSet=12,            //��ଫ���� ��� �࠭��� ��� ��� ����஥�
                    tsMixedNorms=13,                //���訢���� ���, ��䮢, ᡮ஢ �� �࠭���
                    tsNoTicketCheck=15,             //�⬥�� ����஫� ����஢ ����⮢
                    tsCharterSearch=16,             //���᪮�� ����� ॣ����樨 ��� ���஢
                    tsCraftNoChangeSections=17,     //����� ��������� ����������, �����祭��� �� ३�
                    tsCheckMVTDelays=18,            //����஫� ����� ����থ�
                    tsSendMVTDelays=19,             //��ࠢ�� MVT �� ����� � ����প���
                    tsPrintSCDCloseBoarding=21,     //�⮡ࠦ���� ��������� �६��� � ��ᠤ�筮� ⠫���
                    tsMintransFile=22,              //���� ��� ����࠭�
                    tsAODBCreateFlight=26,          //�������� ३ᮢ �� AODB
                    tsSetDepTimeByMVT=27,           //���⠢����� �뫥� ३� �� ⥫��ࠬ�� MVT
                    tsSyncMeridian=28,              //����஭����� � ��ਤ�����
                    tsNoEMDAutoBinding=31,          //����� ��⮯ਢ離� EMD
                    tsCheckPayOnTCkinSegs=32,       //����஫� ������ ⮫쪮 �� ᪢����� ᥣ�����
                    tsAPISControlOnFirstSegOnly=33, //����஫� ������ APIS ⮫쪮 �� ��ࢮ� ᥣ����
                    tsAutoPTPrint=34,               //��⮬���᪠� ����� ��ᠤ����
                    tsAutoPTPrintReseat=35,         //��⮬���᪠� ����� �� �� ��������� ����
                    tsPrintFioPNL=36,               //����� � ��ᠤ�筮� ��� �� �஭�஢���� (PNL)
                    tsWeightControl=37,             //����஫� ��� ������
                    tsNoCrewCkinAlarm=40,           //�⬥�� �ॢ��� '��������� �����'
                    tsNoCtrlDocsCrew=41,            //�� ����஫�஢��� ���� ���㬥�⮢ ��� �����
                    tsNoCtrlDocsExtraCrew=42,       //�� ����஫�஢��� ���� ���㬥�⮢ ��� ���. �����

                    /*�ਢ易��� � ३�� (���� ᮮ⢥�����騥 ���� � ⠡��� trip_sets)*/
                    tsCheckLoad=2,                  //����஫� ����㧪� �� ॣ����樨
                    tsOverloadReg=3,                //����襭�� ॣ����樨 �� �ॢ�襭�� ����㧪�
                    tsExam=4,                       //��ᬮ�஢� ����஫� ��। ��ᠤ���
                    tsCheckPay=5,                   //����஫� ������ ������ �� ��ᠤ��
                    tsExamCheckPay=7,               //����஫� ������ ������ �� ��ᬮ��
                    tsRegWithTkn=8,                 //����� ॣ����樨 ��� ����஢ ����⮢
                    tsRegWithDoc=9,                 //����� ॣ����樨 ��� ����஢ ���㬥�⮢
                    tsRegWithoutTKNA=14,            //����� ॣ����樨 TKNA
                    tsAutoWeighing=20,              //����஫� ��⮬���᪮�� ����訢���� ������
                    tsFreeSeating=23,               //��������� ��ᠤ��
                    tsAPISControl=24,               //����஫� ������ APIS
                    tsAPISManualInput=25,           //��筮� ���� ������ APIS
                    tsPieceConcept=30,              //����� ������ �� ���-�� ����
                    tsLCIPersWeights=38,            //��� ���ᠦ�஢ �� �᭮����� LCI

                    //���, ����, �� �������� � ��� ᥪ�� ����ன��, ����� �� � ⠡��� trip_sets

                    /*�ਢ易��� � ३�� �� �����*/
                    tsBrdWithReg=101,               //��ᠤ�� �� ॣ����樨
                    tsExamWithBrd=102,              //��ᬮ�� �� ��ᠤ��

                    /*�� �ਢ易��� � ३�� ��� ᠬ�ॣ����樨*/
                    tsRegWithSeatChoice=201,        //����� ॣ����樨 ��� �롮� ����
                    tsRegRUSNationOnly=203,         //����� ॣ����樨 ��१����⮢ ��
                    tsSelfCkinCharterSearch=204,    //���᪮�� ����� ᠬ�ॣ����樨 ��� ���஢
                    tsNoRepeatedSelfCkin=205        //����� ����୮� ᠬ�ॣ����樨
                  };

bool DefaultTripSets( const TTripSetType setType );
bool GetTripSets( const TTripSetType setType,
                  const TTripInfo &info );
bool GetSelfCkinSets( const TTripSetType setType,
                      const int point_id,
                      const ASTRA::TClientType client_type );
bool GetSelfCkinSets( const TTripSetType setType,
                      const TTripInfo &info,
                      const ASTRA::TClientType client_type );

class TPnrAddrItem
{
  public:
    char airline[4];
    char addr[21];
    TPnrAddrItem()
    {
      *airline=0;
      *addr=0;
    };
};

std::string GetPnrAddr(int pnr_id, std::vector<TPnrAddrItem> &pnrs);
std::string GetPnrAddr(int pnr_id, std::vector<TPnrAddrItem> &pnrs, std::string &airline);
std::string GetPaxPnrAddr(int pax_id, std::vector<TPnrAddrItem> &pnrs);
std::string GetPaxPnrAddr(int pax_id, std::vector<TPnrAddrItem> &pnrs, std::string &airline);

//��楤�� ��ॢ��� �⤥�쭮�� ��� (��� ����� � ����) � �����業�� TDateTime
//��� ��������� ��� ᮢ�������� ���� �� �⭮襭�� � base_date
//��ࠬ��� back - ���ࠢ����� ���᪠ (true - � ��諮� �� base_date, false - � ���饥)
TDateTime DayToDate(int day, TDateTime base_date, bool back);

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
  };
  void Clear()
  {
    part_key = ASTRA::NoExists;
    point_id = ASTRA::NoExists;
    point_num = ASTRA::NoExists;
    airp.clear();
    pr_cancel = true;
  }
};

struct TAdvTripRouteItem : TTripRouteItem
{
  TDateTime scd_in, scd_out, act_out;
  std::string airline, suffix;
  int flt_num;

  TAdvTripRouteItem() : TTripRouteItem()
  {
    Clear();
  }
  void Clear()
  {
    scd_in = ASTRA::NoExists;
    scd_out = ASTRA::NoExists;
    act_out = ASTRA::NoExists;
    airline.clear();
    suffix.clear();
    flt_num = ASTRA::NoExists;
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
    virtual bool GetRoute(TDateTime part_key,
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
    virtual bool GetRoute(TDateTime part_key,
                int point_id,
                bool after_current,
                TTripRouteType1 route_type1,
                TTripRouteType2 route_type2);
  public:
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
  int grp_id;
  int point_dep, point_arv;
  std::string airp_dep, airp_arv;
  int seg_no;
  TTripInfo operFlt;
  TCkinRouteItem()
  {
    Clear();
  };
  void Clear()
  {
    grp_id = ASTRA::NoExists;
    point_dep = ASTRA::NoExists;
    point_arv = ASTRA::NoExists;
    seg_no = ASTRA::NoExists;
    operFlt.Clear();
  };
};

enum TCkinRouteType1 { crtNotCurrent,
                       crtWithCurrent };
enum TCkinRouteType2 { crtOnlyDependent,
                       crtIgnoreDependent };

class TCkinGrpIds : public std::list<int> {};

class TCkinRoute : public std::vector<TCkinRouteItem>
{
  private:
    void GetRoute(int tckin_id,
                  int seg_no,
                  bool after_current,
                  TCkinRouteType1 route_type1,
                  TCkinRouteType2 route_type2,
                  TQuery& Qry);
    bool GetRoute(int grp_id,
                  bool after_current,
                  TCkinRouteType1 route_type1,
                  TCkinRouteType2 route_type2);

  public:
    //᪢����� ������� ��᫥ ��몮��� grp_id
    bool GetRouteAfter(int grp_id,
                       TCkinRouteType1 route_type1,
                       TCkinRouteType2 route_type2);   //१����=false ⮫쪮 �᫨ ��� grp_id �� �ந��������� ᪢����� ॣ������!
    void GetRouteAfter(int tckin_id,
                       int seg_no,
                       TCkinRouteType1 route_type1,
                       TCkinRouteType2 route_type2);
    //᪢����� ������� �� ��몮��� grp_id
    bool GetRouteBefore(int grp_id,
                        TCkinRouteType1 route_type1,
                        TCkinRouteType2 route_type2);  //१����=false ⮫쪮 �᫨ ��� grp_id �� �ந��������� ᪢����� ॣ������!
    void GetRouteBefore(int tckin_id,
                        int seg_no,
                        TCkinRouteType1 route_type1,
                        TCkinRouteType2 route_type2);

    //�����頥� ᫥���騩 ᥣ���� ��몮���
    void GetNextSeg(int tckin_id,
                    int seg_no,
                    TCkinRouteType2 route_type2,
                    TCkinRouteItem& item);
    bool GetNextSeg(int grp_id,
                    TCkinRouteType2 route_type2,
                    TCkinRouteItem& item);     //१����=false ⮫쪮 �᫨ ��� grp_id �� �ந��������� ᪢����� ॣ������!
                                               //������⢨� ᫥���饣� ᥣ���� �ᥣ�� ���� �஢����� �� �����饭���� item

    //�����頥� �।��騩 ᥣ���� ��몮���
    void GetPriorSeg(int tckin_id,
                     int seg_no,
                     TCkinRouteType2 route_type2,
                     TCkinRouteItem& item);
    bool GetPriorSeg(int grp_id,
                     TCkinRouteType2 route_type2,
                     TCkinRouteItem& item);    //१����=false ⮫쪮 �᫨ ��� grp_id �� �ந��������� ᪢����� ॣ������!
                                               //������⢨� �।��饣� ᥣ���� �ᥣ�� ���� �஢����� �� �����饭���� item
    void get(TCkinGrpIds &tckin_grp_ids) const
    {
      tckin_grp_ids.clear();
      for(TCkinRoute::const_iterator i=begin(); i!=end(); ++i)
        tckin_grp_ids.push_back(i->grp_id);
    }
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

//�����! �६� �뫥� scd_out � operFlt ������ ���� � UTC
//       return_scd_utc=false: �६� �뫥� � markFltInfo �����頥��� �����쭮� �⭮�⥫쭮 airp
//       return_scd_utc=true: �६� �뫥� � markFltInfo �����頥��� � UTC
void GetMktFlights(const TTripInfo &operFltInfo, std::vector<TTripInfo> &markFltInfo, bool return_scd_utc=false);

//�����! �६� �뫥� scd_out � operFlt ������ ���� � UTC
//       �६� �뫥� � markFltInfo ��।����� �����쭮� �⭮�⥫쭮 airp
std::string GetMktFlightStr( const TTripInfo &operFlt, const TTripInfo &markFlt, bool &equal);
bool IsMarkEqualOper( const TTripInfo &operFlt, const TTripInfo &markFlt );

void GetCrsList(int point_id, std::vector<std::string> &crs);
bool IsRouteInter(int point_dep, int point_arv, std::string &country);
bool IsTrferInter(std::string airp_dep, std::string airp_arv, std::string &country);

std::string GetRouteAfterStr(TDateTime part_key,  //NoExists �᫨ � ����⨢��� ����, ���� � ��娢���
                             int point_id,
                             TTripRouteType1 route_type1,
                             TTripRouteType2 route_type2,
                             const std::string &lang="",
                             bool show_city_name=false,
                             const std::string &separator="-");

class TBagTagNumber
{
  public:
    std::string alpha_part;
    double numeric_part;
    TBagTagNumber(const std::string &apart, double npart):alpha_part(apart),numeric_part(npart) {};
    bool operator < (const TBagTagNumber &no) const
    {
      if (alpha_part!=no.alpha_part)
        return alpha_part<no.alpha_part;
      return numeric_part<no.numeric_part;
    };
    bool operator == (const TBagTagNumber &no) const
    {
      return alpha_part == no.alpha_part &&
             numeric_part == no.numeric_part;
    };
};

void GetTagRanges(const std::multiset<TBagTagNumber> &tags,
                  std::vector<std::string> &ranges);   //ranges ���஢��

std::string GetTagRangesStr(const std::multiset<TBagTagNumber> &tags);
std::string GetTagRangesStr(const TBagTagNumber &tag);

std::string GetBagRcptStr(const std::vector<std::string> &rcpts);

const int TEST_ID_BASE = 1000000000;
const int TEST_ID_LAST = TEST_ID_BASE+999999999;
const int EMPTY_ID = TEST_ID_LAST+1;
bool isTestPaxId(int id);
int getEmptyPaxId();
bool isEmptyPaxId(int id);

struct TInfantAdults {
  int grp_id;
  int pax_id;
  int reg_no;
  std::string surname;
  int parent_pax_id;
  int temp_parent_id;
  TInfantAdults() {
   grp_id = ASTRA::NoExists;
   pax_id = ASTRA::NoExists;
   reg_no = ASTRA::NoExists;
   parent_pax_id = ASTRA::NoExists;
  };
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
};

bool is_sync_paxs( int point_id );
void update_pax_change( int point_id, int pax_id, int reg_no, const std::string &work_mode );

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
    std::string str(const std::string &lang="", const std::string &separator=" ");
    void param(LEvntPrms& params);
    TCFG(int point_id, TDateTime part_key = ASTRA::NoExists) { get(point_id, part_key); };
    TCFG() {};
};

enum TDepDateType {ddtEST, ddtACT}; // departure date type
class TDepDateFlags: public BitSet<TDepDateType> {};

class TSearchFltInfo
{
  public:
    std::string airline,suffix,airp_dep;
    int flt_no;
    TDateTime scd_out;
    bool scd_out_in_utc;
    bool only_with_reg;
    std::string additional_where;

    TDepDateFlags dep_date_flags;
    virtual bool before_add(const TAdvTripInfo &flt) const { return true; };
    virtual void before_exit(std::list<TAdvTripInfo> &flts) const {};

    TSearchFltInfo()
    {
      flt_no=ASTRA::NoExists;
      scd_out=ASTRA::NoExists;;
      scd_out_in_utc=false;
      only_with_reg=false;
    };

    virtual ~TSearchFltInfo() {}
};

typedef std::tr1::shared_ptr<TSearchFltInfo> TSearchFltInfoPtr;

struct TLCISearchParams:public TSearchFltInfo {
    TLCISearchParams() { dep_date_flags.setFlag(ddtEST); }
    virtual bool before_add(const TAdvTripInfo &flt) const { return true; };
    virtual void before_exit(std::list<TAdvTripInfo> &flts) const {};
};

void SearchFlt(const TSearchFltInfo &filter, std::list<TAdvTripInfo> &flts);
void SearchMktFlt(const TSearchFltInfo &filter, std::set<int> &point_ids);

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

#endif /*_ASTRA_MISC_H_*/


