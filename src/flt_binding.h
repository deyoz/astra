#ifndef _FLT_BINDING_H_
#define _FLT_BINDING_H_

#include <vector>
#include "basic.h"
#include "astra_misc.h"

class TFltInfo
{
  public:
    char airline[4];
    long flt_no;
    char suffix[2];
    BASIC::TDateTime scd;
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
};

void crs_recount(int point_id_tlg, int point_id_spp, bool check_comp);

enum TBindType {btFirstSeg=0,btAllSeg=2,btLastSeg=1};

class TFltBinding
{
  private:
    void unbind_flt(int point_id, int point_id_spp);
    bool bind_flt(TQuery &Qry);
    bool bind_check(TQuery &Qry, int point_id_spp);
    void bind_flt(TFltInfo &flt, TBindType bind_type, std::vector<int> &spp_point_ids);
    bool bind_flt_or_bind_check(TQuery &Qry, int point_id_spp);
    void bind_or_unbind_flt(const std::vector<TTripInfo> &flts, bool unbind, bool use_scd_utc);
    void bind_or_unbind_flt_oper(const std::vector<TTripInfo> &operFlts, bool unbind);
    void unbind_flt_oper(const std::vector<TTripInfo> &operFlts);

    virtual void unbind_flt_virt(int point_id, int point_id_spp, bool try_bind_again)=0; //возвращает true если реально была отвязка
    virtual std::string bind_flt_sql()=0;
    virtual void bind_flt_virt(int point_id, const std::vector<int> &spp_point_ids)=0;
    virtual std::string bind_or_unbind_flt_sql(bool unbind, bool use_scd_utc)=0;
    virtual std::string unbind_flt_sql()=0;

  public:
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
    void unbind_flt_virt(int point_id, int point_id_spp, bool try_bind_again);
    std::string bind_flt_sql();
    void bind_flt_virt(int point_id, const std::vector<int> &spp_point_ids);
    std::string bind_or_unbind_flt_sql(bool unbind, bool use_scd_utc);
    std::string unbind_flt_sql();
    void after_bind_or_unbind_flt(int point_id_tlg, int point_id_spp, bool unbind);
  public:
    TTlgBinding(bool pcheck_comp):check_comp(pcheck_comp) {};
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

#endif
