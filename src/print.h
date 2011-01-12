#ifndef _PRINT_H
#define _PRINT_H

#include <libxml/tree.h>
#include "jxtlib/JxtInterface.h"
#include "basic.h"
#include "oralib.h"
#include <set>
#include "prn_tag_store.h"

void check_CUTE_certified(int &prn_type, std::string &dev_model, std::string &fmt_type);

typedef enum {
    ptIER506A = 1,
    ptIER508A,
    ptIER506B,
    ptIER508B,
    ptIER557A,
    ptIER567A,
    ptGenicom,
    ptDRV,
    ptIER508BR,
    ptOKIML390,
    ptOKIML3310,
    ptOLIVETTI,
    ptZEBRA,
    ptOLIVETTICOM,
    ptDATAMAX,
    ptDATAMAXCOM
} TPrnType;

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
        PrintDataParser(int pr_lat = 0): pectab_format(0), pts(pr_lat != 0) {};
        PrintDataParser(TBagReceipt rcpt): pectab_format(0), pts(rcpt) {};
        PrintDataParser(int grp_id, int pax_id, bool pr_lat, xmlNodePtr tagsNode, TBTRoute *route = NULL):
            pectab_format(0), pts(grp_id, pax_id, pr_lat, tagsNode, route) {};
        std::string parse(std::string &form);
        std::string GetTagAsString(std::string name) { return ""; }; //!!! zatychka
};

// !!! Next generation
void GetTripBPPectabs(int point_id, std::string dev_model, std::string fmt_type, xmlNodePtr node);
void GetTripBTPectabs(int point_id, std::string dev_model, std::string fmt_type, xmlNodePtr node);

void GetTripBPPectabs(int point_id, int prn_type, xmlNodePtr node);
void GetTripBTPectabs(int point_id, int prn_type, xmlNodePtr node);
void GetPrintDataBT(xmlNodePtr dataNode, int grp_id, int pr_lat);
std::string get_validator(TBagReceipt &rcpt);
double CalcPayRate(const TBagReceipt &rcpt);
double CalcRateSum(const TBagReceipt &rcpt);
double CalcPayRateSum(const TBagReceipt &rcpt);

class PrintInterface: public JxtInterface
{
    public:
        PrintInterface(): JxtInterface("123", "print")
        {
            Handler *evHandle;
            evHandle=JxtHandler<PrintInterface>::CreateHandler(&PrintInterface::GetPrintDataBP);
            AddEvent("GetPrintDataBP",evHandle);
            AddEvent("GetGRPPrintDataBP",evHandle);
            evHandle=JxtHandler<PrintInterface>::CreateHandler(&PrintInterface::ReprintDataBTXML);
            AddEvent("ReprintDataBT",evHandle);
            evHandle=JxtHandler<PrintInterface>::CreateHandler(&PrintInterface::GetPrintDataBTXML);
            AddEvent("GetPrintDataBT",evHandle);
            evHandle=JxtHandler<PrintInterface>::CreateHandler(&PrintInterface::ConfirmPrintBT);
            AddEvent("ConfirmPrintBT",evHandle);
            evHandle=JxtHandler<PrintInterface>::CreateHandler(&PrintInterface::ConfirmPrintBP);
            AddEvent("ConfirmPrintBP",evHandle);
            evHandle=JxtHandler<PrintInterface>::CreateHandler(&PrintInterface::GetPrinterList);
            AddEvent("GetPrinterList",evHandle);
            evHandle=JxtHandler<PrintInterface>::CreateHandler(&PrintInterface::RefreshPrnTests);
            AddEvent("refresh_prn_tests",evHandle);
        }

        void RefreshPrnTests(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void GetPrinterList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void GetPrintDataBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void ReprintDataBTXML(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void GetPrintDataBTXML(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void ConfirmPrintBT(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void ConfirmPrintBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        static void GetPrintDataBR(std::string &form_type, PrintDataParser &parser,
                std::string &Print, bool &hex, xmlNodePtr reqNode
                );
        virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

#endif
