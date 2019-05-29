#ifndef _STAT_H_
#define _STAT_H_

#include <libxml/tree.h>
#include <set>
#include "jxtlib/JxtInterface.h"
#include "date_time.h"

using BASIC::date_time::TDateTime;

namespace STAT {

    struct agent_stat_t {
        int inc, dec;
        agent_stat_t(int ainc, int adec): inc(ainc), dec(adec) {};
        bool operator == (const agent_stat_t &item) const
        {
            return inc == item.inc &&
                   dec == item.dec;
        };
        void operator += (const agent_stat_t &item)
        {
            inc += item.inc;
            dec += item.dec;
        };
    };
    void agent_stat_delta(
            int point_id,
            int user_id,
            const std::string &desk,
            TDateTime ondate,
            int pax_time,
            int pax_amount,
            agent_stat_t dpax_amount, // d prefix stands for delta
            agent_stat_t dtckin_amount,
            agent_stat_t dbag_amount,
            agent_stat_t dbag_weight,
            agent_stat_t drk_amount,
            agent_stat_t drk_weight
            );
    int agent_stat_delta(int argc,char **argv);
    int ovb(int argc,char **argv);

    class TMoveIds : public std::set< std::pair<TDateTime, int> >
    {
      public:
        void get_for_airp(TDateTime first_date, TDateTime last_date, const std::string& airp);
    };

}


class StatInterface : public JxtInterface
{
public:
  StatInterface() : JxtInterface("123","stat")
  {
     Handler *evHandle;
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::PaxListRun);
     AddEvent("PaxListRun",evHandle);
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::PaxSrcRun);
     AddEvent("PaxSrcRun",evHandle);
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::FltLogRun);
     AddEvent("FltLogRun",evHandle);
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::FltTaskLogRun);
     AddEvent("FltTaskLogRun",evHandle);
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::LogRun);
     AddEvent("LogRun",evHandle);
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::SystemLogRun);
     AddEvent("SystemLogRun",evHandle);
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::FltCBoxDropDown);
     AddEvent("FltCBoxDropDown",evHandle);
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::CommonCBoxDropDown);
     AddEvent("CommonCBoxDropDown",evHandle);
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::RunStat);
     AddEvent("run_stat",evHandle);
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::stat_srv);
     AddEvent("stat_srv",evHandle);
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::Layout);
     AddEvent("layout",evHandle);

     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::StatOrders);
     AddEvent("stat_orders",evHandle);
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::StatOrderDel);
     AddEvent("stat_order_del",evHandle);
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::DownloadOrder);
     AddEvent("download_order",evHandle);
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::FileList);
     AddEvent("get_file_list",evHandle);
  };

  void FltCBoxDropDown(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void CommonCBoxDropDown(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void FltLogRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void FltTaskLogRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void LogRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SystemLogRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void PaxListRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void PaxSrcRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void RunStat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void stat_srv(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Layout(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void TestRunStat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  // Работа с заказами
  void StatOrders(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void StatOrderDel(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void DownloadOrder(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void FileList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};
};

void get_flight_stat(std::map<std::string, long> &stat_times, int point_id, bool final_collection);
int nosir_rfisc_stat(int argc,char **argv);
int nosir_lim_capab_stat(int argc,char **argv);
int nosir_rfisc_all(int argc,char **argv);
int nosir_self_ckin(int argc,char **argv);
int nosir_stat_order(int argc,char **argv);
int nosir_departed_pax(int argc, char **argv);
int nosir_departed(int argc, char **argv);
int nosir_departed_sql(int argc, char **argv);
int nosir_seDCSAddReport(int argc, char **argv);


void stat_orders_collect(void);
void stat_orders_synchro(void);

void get_full_stat(TDateTime utcdate);

#endif
