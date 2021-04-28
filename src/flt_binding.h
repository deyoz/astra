#ifndef _FLT_BINDING_H_
#define _FLT_BINDING_H_

#include <vector>
#include "date_time.h"
#include "astra_misc.h"

using BASIC::date_time::TDateTime;
using BASIC::date_time::DateTimeToStr;

enum TBindType {btFirstSeg=0,btAllSeg=2,btLastSeg=1,btNone=3};

class TFltInfo
{
  public:
    char airline[4];
    long flt_no;
    char suffix[2];
    TDateTime scd;
    bool pr_utc;
    char airp_dep[4],airp_arv[4];
    TFltInfo()
    {
      Clear();
    };
    TFltInfo(const TTripInfo &flt);
    TFltInfo(const TFltInfo &flt)
    {
      *this=flt;
    };
    void Clear()
    {
      *airline=0;
      flt_no=0;
      *suffix=0;
      scd=0;
      pr_utc=false;
      *airp_dep=0;
      *airp_arv=0;
    };
    bool Empty()
    {
      return *airline==0;
    };
    void operator = (const TFltInfo &flt)
    {
      strcpy(airline, flt.airline);
      flt_no=flt.flt_no;
      strcpy(suffix, flt.suffix);
      scd=flt.scd;
      pr_utc=flt.pr_utc;
      strcpy(airp_dep, flt.airp_dep);
      strcpy(airp_arv, flt.airp_arv);
    };
    bool operator == (const TFltInfo &flt) const
    {
      return strcmp(airline, flt.airline)==0 &&
             flt_no==flt.flt_no &&
             strcmp(suffix, flt.suffix)==0 &&
             scd==flt.scd &&
             pr_utc==flt.pr_utc &&
             strcmp(airp_dep, flt.airp_dep)==0 &&
             strcmp(airp_arv, flt.airp_arv)==0;
    };
    void dump() const;
    void parse(const char *val);
    std::pair<int, bool> getPointId(TBindType bind_type) const;
    std::string toString() const
    {
      std::ostringstream result;
      result << airline
             << std::setw(3) << std::setfill('0') << flt_no << suffix;
      if (scd != 0)
        result << "/" << DateTimeToStr(scd, "ddmmm");
      result << " " << airp_dep << airp_arv;
      return result.str();
    }
    void set_airline(const std::string &_airline);
    void set_airp_dep(const std::string &_airp_dep);
    void set_airp_arv(const std::string &_airp_arv);
    void set_suffix(const std::string &_suffix);
};

class TlgSource
{
  public:
    int point_id;
    int tlg_id;
    bool has_errors;
    bool has_alarm_errors;

    TlgSource(int _point_id, int _tlg_id, bool _has_errors, bool _has_alarm_errors) :
      point_id(_point_id), tlg_id(_tlg_id), has_errors(_has_errors), has_alarm_errors(_has_alarm_errors) {}

    const TlgSource& toDB() const;
};

void crs_recount(int point_id_tlg, int point_id_spp, bool check_comp);

class TFltBinding
{
  protected:
    void unbind_flt(int point_id, int point_id_spp);
    bool bind_check(DB::TQuery &Qry, int point_id_spp);
    bool bind_flt_inner(DB::TQuery &Qry);
    bool bind_flt_or_bind_check(DB::TQuery &Qry, int point_id_spp);
    virtual void bind_or_unbind_flt(const std::vector<TTripInfo> &flts, bool unbind, bool use_scd_utc)=0;
    void bind_or_unbind_flt_oper(const std::vector<TTripInfo> &operFlts, bool unbind);
    void unbind_flt_oper(const std::vector<TTripInfo> &operFlts);

    virtual void unbind_flt_virt(int point_id, int point_id_spp, bool try_bind_again)=0; //возвращает true если реально была отвязка
    virtual void bind_flt_virt(int point_id, const std::vector<int> &spp_point_ids)=0;
    virtual FltOperFilter::DateFlags dateFlags() const { return {}; }

  public:
    virtual bool bind_flt_by_point_id(int point_id)=0;
    void bind_flt(const TFltInfo &flt, TBindType bind_type, std::vector<int> &spp_point_ids);
    void bind_flt_oper(const std::vector<TTripInfo> &operFlts);
    void bind_flt(const std::vector<TTripInfo> &flts, bool use_scd_utc);
    void unbind_flt(const std::vector<TTripInfo> &flts, bool use_scd_utc);
    virtual void unbind_flt_by_point_ids(const std::vector<int> &spp_point_ids)=0;
    void trace_for_bind(const std::vector<TTripInfo> &flts, const std::string &where);
    void trace_for_bind(const std::vector<int> &point_ids, const std::string &where);
    virtual ~TFltBinding() {};
};

class TTlgBinding : public TFltBinding
{
  private:
    bool check_comp;
    FltOperFilter::DateFlags dateFlags_;

    void unbind_flt_virt(int point_id, int point_id_spp, bool try_bind_again);
    void bind_flt_virt(int point_id, const std::vector<int> &spp_point_ids);
    void bind_or_unbind_flt(const std::vector<TTripInfo> &flts, bool unbind, bool use_scd_utc);
    void after_bind_or_unbind_flt(int point_id_tlg, int point_id_spp, bool unbind);
    virtual FltOperFilter::DateFlags dateFlags() const { return dateFlags_; }

  public:
    virtual bool bind_flt_by_point_id(int point_id);
    virtual void unbind_flt_by_point_ids(const std::vector<int> &spp_point_ids);

    TTlgBinding(bool pcheck_comp) :
      check_comp(pcheck_comp) {}
    TTlgBinding(bool pcheck_comp, const FltOperFilter::DateFlags& dateFlags) :
      check_comp(pcheck_comp),
      dateFlags_(dateFlags) {}
};

class TTrferBinding : public TFltBinding
{
  private:
    bool check_alarm;
    void unbind_flt_virt(int point_id, int point_id_spp, bool try_bind_again);
    void bind_flt_virt(int point_id, const std::vector<int> &spp_point_ids);
    void bind_or_unbind_flt(const std::vector<TTripInfo> &flts, bool unbind, bool use_scd_utc);
    void after_bind_or_unbind_flt(int point_id_trfer, int point_id_spp, bool unbind);
  public:
    virtual bool bind_flt_by_point_id(int point_id);
    virtual void unbind_flt_by_point_ids(const std::vector<int> &spp_point_ids);

    TTrferBinding():check_alarm(true) {};
    TTrferBinding(bool pcheck_alarm):check_alarm(pcheck_alarm) {};
};

namespace TypeB
{

struct TFltForBind {
    TFltInfo flt_info;
    TBindType bind_type;
    FltOperFilter::DateFlags dateFlags;
    TFltForBind(
            TFltInfo vflt_info,
            TBindType vbind_type,
            const FltOperFilter::DateFlags& vdateFlags={}
            ):
        flt_info(vflt_info),
        bind_type(vbind_type),
        dateFlags(vdateFlags)
    {}
};

typedef std::list<TFltForBind> TFlightsForBind;

} //namespace TypeB

void get_wb_franchise_flts(const TTripInfo &trip_info, std::vector<TTripInfo> &franchise_flts);

#endif
