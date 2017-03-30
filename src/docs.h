#ifndef _DOCS_H_
#define _DOCS_H_

#include <libxml/tree.h>
#include "jxtlib/JxtInterface.h"
#include "astra_consts.h"
#include "date_time.h"
#include "remarks.h"
#include "print.h"

enum TSortType {stRegNo, stSurname, stSeatNo};

struct TRptParams {
    private:
        bool route_inter;
        std::string route_country_lang; //язык страны, где выполняется внутренний рейс
        std::string req_lang; // Язык, затребованный с клиента, если пустой, то вычисляем язык на основе маршрута
        std::string GetLang(TElemFmt &fmt, std::string firm_lang) const;
    public:
        TPrnParams prn_params;
        TSortType sort;
        int point_id;
        ASTRA::TRptType rpt_type;
        std::string airp_arv;
        std::string ckin_zone;
        bool pr_et;
        bool pr_trfer;
        bool pr_brd;
        TSimpleMktFlight mkt_flt;
        std::string client_type;
        std::map< TRemCategory, std::vector<std::string> > rems;
        int text;
        std::string ElemIdToReportElem(TElemType type, const std::string &id, TElemFmt fmt, std::string firm_lang = "") const;
        std::string ElemIdToReportElem(TElemType type, int id, TElemFmt fmt, std::string firm_lang = "") const;
        bool IsInter() const;
        std::string GetLang() const;
        std::string dup_lang() { return GetLang()==AstraLocale::LANG_EN ? AstraLocale::LANG_RU : GetLang(); }; // lang for duplicated captions
        void Init(xmlNodePtr node);
        TRptParams(std::string lang) {
            req_lang = lang;
        };
        TRptParams():
            route_inter(true),
            point_id(ASTRA::NoExists),
            pr_et(false),
            pr_trfer(false),
            pr_brd(false),
            text(ASTRA::NoExists)
    {};
};

bool bad_client_img_version();
void get_new_report_form(const std::string name, xmlNodePtr reqNode, xmlNodePtr resNode);
void get_compatible_report_form(const std::string name, xmlNodePtr reqNode, xmlNodePtr resNode);
void PaxListVars(int point_id, TRptParams &rpt_params, xmlNodePtr variablesNode,
                 BASIC::date_time::TDateTime part_key = ASTRA::NoExists);
void SeasonListVars(int trip_id, int pr_lat, xmlNodePtr variablesNode, xmlNodePtr reqNode);
std::string get_flight(xmlNodePtr variablesNode);
std::vector<std::string> get_grp_zone_list(int point_id);

std::string vs_number(int number, bool pr_lat = false);

class DocsInterface : public JxtInterface
{
public:
  DocsInterface() : JxtInterface("","docs")
  {
     Handler *evHandle;
     evHandle=JxtHandler<DocsInterface>::CreateHandler(&DocsInterface::SaveReport);
     AddEvent("save_report",evHandle);
     evHandle=JxtHandler<DocsInterface>::CreateHandler(&DocsInterface::RunReport2);
     AddEvent("run_report2",evHandle);
     evHandle=JxtHandler<DocsInterface>::CreateHandler(&DocsInterface::GetFonts);
     AddEvent("GetFonts",evHandle);

     evHandle=JxtHandler<DocsInterface>::CreateHandler(&DocsInterface::GetSegList);
     AddEvent("GetSegList2",evHandle);
     AddEvent("GetSegList",evHandle);
     evHandle=JxtHandler<DocsInterface>::CreateHandler(&DocsInterface::RunReport);
     AddEvent("run_report",evHandle);
     evHandle=JxtHandler<DocsInterface>::CreateHandler(&DocsInterface::RunSPP);
     AddEvent("run_spp",evHandle);
     evHandle=JxtHandler<DocsInterface>::CreateHandler(&DocsInterface::LogPrintEvent);
     AddEvent("LogPrintEvent",evHandle);
     evHandle=JxtHandler<DocsInterface>::CreateHandler(&DocsInterface::LogExportEvent);
     AddEvent("LogExportEvent",evHandle);
  };

  void SaveReport(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void RunReport2(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetFonts(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  void GetSegList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void RunReport(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void RunSPP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void LogExportEvent(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void LogPrintEvent(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  static void GetZoneList(int point_id, xmlNodePtr dataNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};
};

struct t_tag_nos_row {
    int pr_liab_limit;
    double no;
};

std::string get_tag_range(std::vector<t_tag_nos_row> tag_nos, std::string lang);

int testbm(int argc,char **argv);

void populate_doc_cap(xmlNodePtr variablesNode, std::string lang);
void ANNUL_TAGS(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode);
void VOUCHERS(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode);
void SERVICES(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode);

TDateTime getReportSCDOut(int point_id);
std::string getJMPSeatNo(int pax_id);

#endif
