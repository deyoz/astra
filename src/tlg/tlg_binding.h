#ifndef _TLG_BINDING_H_
#define _TLG_BINDING_H_

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
};

enum TBindType {btFirstSeg=0,btAllSeg=2,btLastSeg=1};

void crs_recount(int point_id_tlg, int point_id_spp, bool check_comp); //point_id_spp ¬.¡. NoExists
bool bind_tlg(int point_id_tlg, bool check_comp);
void bind_tlg(const std::vector<TTripInfo> &flts, bool use_scd_utc, bool check_comp);
void unbind_tlg(const std::vector<TTripInfo> &flts, bool use_scd_utc);
void bind_tlg_oper(const std::vector<TTripInfo> &operFlts, bool check_comp);
void unbind_tlg_oper(const std::vector<TTripInfo> &operFlts);
void unbind_tlg(const std::vector<int> &spp_point_ids);
void trace_for_bind(const std::vector<TTripInfo> &flts, const std::string &where);
void trace_for_bind(const std::vector<int> &point_ids, const std::string &where);

#endif


