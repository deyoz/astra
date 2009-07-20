#ifndef _PRINT_H
#define _PRINT_H

#include <libxml/tree.h>
#include "jxtlib/JxtInterface.h"
#include "basic.h"
#include "oralib.h"

void check_CUTE_certified(int &prn_type, std::string &dev_model, std::string &fmt_type);

struct TBagPayType
{
  std::string pay_type;
  double pay_rate_sum;
  std::string extra;
};

struct TBagReceipt
{
  bool pr_lat;
  std::string form_type;
  double no;
  std::string pax_name,pax_doc;
  int service_type,bag_type;
  std::string bag_name;
  std::string tickets,prev_no;
  std::string airline,aircode,airp_dep,airp_arv,suffix;
  int flt_no;
  int ex_amount,ex_weight;
  double value_tax,rate,exch_pay_rate;
  int exch_rate;
  std::string rate_cur,pay_rate_cur;
  std::vector<TBagPayType> pay_types;
  std::string remarks;
  BASIC::TDateTime issue_date,annul_date;
  std::string issue_desk,annul_desk,issue_place;
};

#define CASH_PAY_TYPE "çÄã"
#define NONE_PAY_TYPE "çÖí"

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

namespace to_esc {
    void convert(std::string &mso_form, TPrnType prn_type, xmlNodePtr reqNode = NULL);
    void convert_dmx(std::string &mso_form, TPrnType prn_type, xmlNodePtr reqNode = NULL);
}

//////////////////////////////// CLASS PrintDataParser ///////////////////////////////////

class PrintDataParser {
    public:
        enum TMapType {mtBTBP};
    private:
        class t_field_map {
            private:
                struct TTagValue {
                    bool null, pr_print;
                    otFieldType type;
                    std::string StringVal;
                    std::string err_msg;
                    double FloatVal;
                    int IntegerVal;
                    BASIC::TDateTime DateTimeVal;
                    bool nullable;
                    TTagValue() {
                        null = true;
                        pr_print = false;
                        type = otString;
                        FloatVal = 0;
                        IntegerVal = 0;
                        DateTimeVal = 0;
                        nullable = false;
                    }
                };

                typedef std::map<std::string, TTagValue> TData;
                TData data;
                void dump_data();
                std::string BCBP_M_2(bool pr_lat);
                std::string LONG_DEP(bool pr_lat);
                std::string LONG_ARV(bool pr_lat);


                std::string class_checked;
                int grp_id;
                int pax_id;
                int pr_lat;
                typedef std::vector<TQuery*> TQrys;
                TQrys Qrys;
                TQuery *prnQry;

                void fillBTBPMap();
                void fillMSOMap(TBagReceipt &rcpt);
                void add_mso_point(std::string name, std::string airp, bool pr_lat);
                bool printed(TData::iterator di);

            public:
                int print_mode;
                t_field_map(int grp_id, int pax_id, int pr_lat, xmlNodePtr tagsNode, TMapType map_type);
                t_field_map(TBagReceipt &rcpt);
                std::string get_field(std::string name, int len, std::string align, std::string date_format, int field_lat);
                void add_tag(std::string name, int val, bool nullable = false);
                void add_tag(std::string name, std::string val, bool nullable = false);
                void add_err_tag(std::string name, std::string val);
                void add_tag(std::string name, BASIC::TDateTime val, bool nullable = false);
                std::string GetTagAsString(std::string name);
                int GetTagAsInteger(std::string name);
                TQuery *get_prn_qry();
                ~t_field_map();
        };

        int pectab_format;
        int pr_lat;
        t_field_map field_map;
        std::string parse_field(int offset, std::string field);
        std::string parse_field0(int offset, std::string field);
        std::string parse_field1(int offset, std::string field);
        bool IsDelim(char curr_char, char &Mode);
        std::string parse_tag(int offset, std::string tag);
    public:
        PrintDataParser(TBagReceipt rcpt): field_map(rcpt)
        {
            pectab_format = 0;
            this->pr_lat = rcpt.pr_lat;
        };
        PrintDataParser(int grp_id, int pax_id, int pr_lat, xmlNodePtr tagsNode, TMapType map_type = mtBTBP):
            field_map(grp_id, pax_id, pr_lat, tagsNode, map_type)
        {
            pectab_format = 0;
            this->pr_lat = pr_lat;
        };
        std::string parse(std::string &form);
        TQuery *get_prn_qry() { return field_map.get_prn_qry(); };
        void add_tag(std::string name, int val, bool nullable = false) { return field_map.add_tag(name, val, nullable); };
        void add_tag(std::string name, std::string val, bool nullable = false) { return field_map.add_tag(name, val, nullable); };
        void add_tag(std::string name, BASIC::TDateTime val, bool nullable = false) { return field_map.add_tag(name, val, nullable); };
        std::string GetTagAsString(std::string name) { return field_map.GetTagAsString(name); };
        int GetTagAsInteger(std::string name) { return field_map.GetTagAsInteger(name); };
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
        }

        void GetPrinterList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void GetPrintDataBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void ReprintDataBTXML(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void GetPrintDataBTXML(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void ConfirmPrintBT(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void ConfirmPrintBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        static void GetPrintDataBR(std::string &form_type, PrintDataParser &parser, std::string &Print,
                xmlNodePtr reqNode
                );
        virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

#endif
