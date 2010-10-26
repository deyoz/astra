#ifndef _BRD_H_
#define _BRD_H_

#include <libxml/tree.h>
#include "oralib.h"
#include "docs.h"
#include "jxtlib/JxtInterface.h"

class BrdInterface : public JxtInterface
{
private:
    static bool PaxUpdate(int point_id, int pax_id, int &tid, bool mark, bool pr_exam_with_brd);
public:
  BrdInterface() : JxtInterface("123","brd")
  {
     Handler *evHandle;
     //vlad
     evHandle=JxtHandler<BrdInterface>::CreateHandler(&BrdInterface::PaxList);
     AddEvent("PaxList",evHandle);
     AddEvent("PaxByPaxId",evHandle);
     AddEvent("PaxByRegNo",evHandle);
     AddEvent("PaxByScanData",evHandle);

     evHandle=JxtHandler<BrdInterface>::CreateHandler(&BrdInterface::DeplaneAll);
     AddEvent("DeplaneAll",evHandle);
  };

  void PaxList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void DeplaneAll(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};

  static void readTripData( int point_id, xmlNodePtr dataNode );
  static void readTripCounters( const int point_id,
                                const TRptParams &rpt_params,
                                xmlNodePtr dataNode,
                                const bool used_for_web_rpt,
                                const std::string &client_type );

  static void GetPaxQuery(TQuery &Qry, const int point_id,
                                       const int reg_no,
                                       const std::string &lang,
                                       const bool used_for_web_rpt,
                                       const std::string &client_type);

  static void GetPax(xmlNodePtr reqNode, xmlNodePtr resNode);
};

#endif
