#ifndef _PRINT_H
#define _PRINT_H

#include <libxml/tree.h>
#include "jxtlib/JxtInterface.h"
#include "date_time.h"
#include "oralib.h"
#include <set>
#include "prn_tag_store.h"
#include "checkin.h"

using BASIC::date_time::TDateTime;

struct TPrnParams {
    std::string encoding;
    int offset, top;
    bool pr_lat;
    void get_prn_params(xmlNodePtr prnParamsNode);
    TPrnParams(): encoding("CP866"), offset(20), top(0), pr_lat(false) {};
    TPrnParams(xmlNodePtr prnParamsNode): encoding("CP866"), offset(20), top(0), pr_lat(false) {get_prn_params(prnParamsNode);};
};

//////////////////////////////// CLASS PrintDataParser ///////////////////////////////////

class PrintDataParser {
    private:
        int pectab_format;
        std::string parse_field(int offset, std::string field);
        std::string parse_field0(int offset, std::string field);
        std::string parse_field1(int offset, std::string field);
        bool IsDelim(char curr_char, char &Mode);
        std::string parse_tag(int offset, std::string tag);
    public:
        TPrnTagStore pts;
        PrintDataParser(const std::string &scan, bool pr_lat = false): pectab_format(0), pts(scan, pr_lat) {};
        PrintDataParser(bool pr_lat = false): pectab_format(0), pts(pr_lat) {};
        PrintDataParser(const TBagReceipt &rcpt, bool pr_lat): pectab_format(0), pts(rcpt, pr_lat) {};
        PrintDataParser(int grp_id, int pax_id, bool pr_lat, xmlNodePtr tagsNode, const TTrferRoute &route = TTrferRoute()):
            pectab_format(0), pts(grp_id, pax_id, pr_lat, tagsNode, route) {};
        std::string parse(std::string &form);
};


void GetTripBPPectabs(int point_id, ASTRA::TDevOperType op_type, const std::string &dev_model, const std::string &fmt_type, xmlNodePtr node);
void GetTripBTPectabs(int point_id, const std::string &dev_model, const std::string &fmt_type, xmlNodePtr node);

std::string get_validator(const TBagReceipt &rcpt, bool pr_lat);
double CalcPayRate(const TBagReceipt &rcpt);
double CalcRateSum(const TBagReceipt &rcpt);
double CalcPayRateSum(const TBagReceipt &rcpt);

class PrintInterface: public JxtInterface
{
    public:
        struct BPPax {
          int point_dep;
          int grp_id;
          int pax_id;
          int reg_no;
          std::string full_name;
          std::pair<std::string, bool> gate; //bool=true, �᫨ ������ set_tag, ���� � gate ��祣� �� ������
          TDateTime time_print;
          std::string prn_form;
          std::string scan;

          bool hex;
          BPPax()
          {
            clear();
          };
          BPPax( int vgrp_id, int vpax_id, int vreg_no )
          {
            clear();
            grp_id=vgrp_id;
            pax_id=vpax_id;
            reg_no=vreg_no;
          };
          void clear()
          {
            point_dep=ASTRA::NoExists;
            grp_id=ASTRA::NoExists;
            pax_id=ASTRA::NoExists;
            reg_no=ASTRA::NoExists;
            full_name.clear();
            gate=std::make_pair("", false);
            time_print=ASTRA::NoExists;
            prn_form.clear();
            scan.clear();
            hex=false;
          };
          bool fromDB(int vpax_id, int test_point_dep);
        };

        struct BPParams {
          std::string dev_model;
          std::string fmt_type;
          std::string form_type;
          TPrnParams prnParams;
          xmlNodePtr clientDataNode;
        };

        PrintInterface(): JxtInterface("123", "print")
        {
            Handler *evHandle;
            evHandle=JxtHandler<PrintInterface>::CreateHandler(&PrintInterface::GetPrintDataBP);
            AddEvent("GetPrintDataBP",evHandle);
            AddEvent("GetGRPPrintDataBP",evHandle);
            AddEvent("GetGRPPrintData",evHandle);
            AddEvent("GetPrintData",evHandle);
            evHandle=JxtHandler<PrintInterface>::CreateHandler(&PrintInterface::ReprintDataBTXML);
            AddEvent("ReprintDataBT",evHandle);
            evHandle=JxtHandler<PrintInterface>::CreateHandler(&PrintInterface::GetPrintDataBTXML);
            AddEvent("GetPrintDataBT",evHandle);
            evHandle=JxtHandler<PrintInterface>::CreateHandler(&PrintInterface::ConfirmPrintBT);
            AddEvent("ConfirmPrintBT",evHandle);
            evHandle=JxtHandler<PrintInterface>::CreateHandler(&PrintInterface::ConfirmPrintBP);
            AddEvent("ConfirmPrintBP",evHandle);
            AddEvent("ConfirmPrintData",evHandle);
            evHandle=JxtHandler<PrintInterface>::CreateHandler(&PrintInterface::RefreshPrnTests);
            AddEvent("refresh_prn_tests",evHandle);
            evHandle=JxtHandler<PrintInterface>::CreateHandler(&PrintInterface::GetImg);
            AddEvent("GetImg",evHandle);
        }

        void GetImg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void RefreshPrnTests(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void GetPrintDataBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void ReprintDataBTXML(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void GetPrintDataBTXML(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void ConfirmPrintBT(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void ConfirmPrintBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        static void GetPrintDataBR(std::string &form_type, PrintDataParser &parser,
                std::string &Print, bool &hex, xmlNodePtr reqNode
                );
        virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

        static void GetPrintDataBP(
                                   ASTRA::TDevOperType op_type,
                                   BPParams &params,
                                   std::string &data,
                                   std::string &pectab,
                                   BIPrintRules::Holder &bi_rules,
                                   std::vector<BPPax> &paxs);
        static void ConfirmPrintBP(ASTRA::TDevOperType op_type,
                                   const std::vector<BPPax> &paxs,
                                   CheckIn::UserException &ue);
        
        static void check_pectab_availability(BPParams &params, int grp_id, ASTRA::TDevOperType op_type);

        static void get_pectab(
                ASTRA::TDevOperType op_type,
                BPParams &params,
                std::string &data,
                std::string &pectab
                );
};

#endif
