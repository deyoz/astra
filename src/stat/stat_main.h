#ifndef _STAT_H_
#define _STAT_H_

#include <libxml/tree.h>
#include <set>
#include "jxtlib/JxtInterface.h"
#include "date_time.h"

using BASIC::date_time::TDateTime;


class StatInterface : public JxtInterface
{
public:
  StatInterface() : JxtInterface("123","stat")
  {
     Handler *evHandle;

     // блок обработчиков модуля Информ. по архиву
     // описаны в stat_arx.cc
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

     // модуль статистика
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

void get_flight_stat(int point_id, bool final_collection);
int nosir_stat_order(int argc,char **argv);

void stat_orders_collect(int interval);
void stat_orders_synchro(void);

void get_full_stat(TDateTime utcdate);

#endif
