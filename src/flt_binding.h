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
  private:
    void unbind_flt(int point_id, int point_id_spp);
    bool bind_flt(TQuery &Qry);
    bool bind_check(TQuery &Qry, int point_id_spp);
    bool bind_flt_or_bind_check(TQuery &Qry, int point_id_spp);
    void bind_or_unbind_flt(const std::vector<TTripInfo> &flts, bool unbind, bool use_scd_utc);
    void bind_or_unbind_flt_oper(const std::vector<TTripInfo> &operFlts, bool unbind);
    void unbind_flt_oper(const std::vector<TTripInfo> &operFlts);

    virtual void unbind_flt_virt(int point_id, int point_id_spp, bool try_bind_again)=0; //�����頥� true �᫨ ॠ�쭮 �뫠 ��離�
    virtual std::string bind_flt_sql()=0;
    virtual void bind_flt_virt(int point_id, const std::vector<int> &spp_point_ids)=0;
    virtual std::string bind_or_unbind_flt_sql(bool unbind, bool use_scd_utc)=0;
    virtual std::string unbind_flt_sql()=0;
    virtual TSearchFltInfoPtr get_search_params() { return TSearchFltInfoPtr(); }

  public:
    void bind_flt(TFltInfo &flt, TBindType bind_type, std::vector<int> &spp_point_ids);
    bool bind_flt(int point_id);
    void bind_flt_oper(const std::vector<TTripInfo> &operFlts);
    void bind_flt(const std::vector<TTripInfo> &flts, bool use_scd_utc);
    void unbind_flt(const std::vector<TTripInfo> &flts, bool use_scd_utc);
    void unbind_flt(const std::vector<int> &spp_point_ids);
    void trace_for_bind(const std::vector<TTripInfo> &flts, const std::string &where);
    void trace_for_bind(const std::vector<int> &point_ids, const std::string &where);
    virtual ~TFltBinding() {};
};

class TTlgBinding : public TFltBinding
{
  private:
    bool check_comp;
    TSearchFltInfoPtr search_params;

    void unbind_flt_virt(int point_id, int point_id_spp, bool try_bind_again);
    std::string bind_flt_sql();
    void bind_flt_virt(int point_id, const std::vector<int> &spp_point_ids);
    std::string bind_or_unbind_flt_sql(bool unbind, bool use_scd_utc);
    std::string unbind_flt_sql();
    void after_bind_or_unbind_flt(int point_id_tlg, int point_id_spp, bool unbind);
    virtual TSearchFltInfoPtr get_search_params() { return search_params; }

  public:
    TTlgBinding(bool pcheck_comp):check_comp(pcheck_comp), search_params(TSearchFltInfoPtr()) {};
    TTlgBinding(bool pcheck_comp, TSearchFltInfoPtr psearch_params):check_comp(pcheck_comp), search_params(psearch_params) {};
};

class TTrferBinding : public TFltBinding
{
  private:
    bool check_alarm;
    void unbind_flt_virt(int point_id, int point_id_spp, bool try_bind_again);
    std::string bind_flt_sql();
    void bind_flt_virt(int point_id, const std::vector<int> &spp_point_ids);
    std::string bind_or_unbind_flt_sql(bool unbind, bool use_scd_utc);
    std::string unbind_flt_sql();
    void after_bind_or_unbind_flt(int point_id_trfer, int point_id_spp, bool unbind);
  public:
    TTrferBinding():check_alarm(true) {};
    TTrferBinding(bool pcheck_alarm):check_alarm(pcheck_alarm) {};
};

namespace TypeB
{

struct TFltForBind {
    TFltInfo flt_info;
    TBindType bind_type;
    TSearchFltInfoPtr search_params;
    TFltForBind(
            TFltInfo vflt_info,
            TBindType vbind_type,
            TSearchFltInfoPtr vsearch_params
            ):
        flt_info(vflt_info),
        bind_type(vbind_type),
        search_params(vsearch_params)
    {}
};

typedef std::list<TFltForBind> TFlightsForBind;

} //namespace TypeB

#endif
