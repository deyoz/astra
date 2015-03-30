#ifndef _PRN_TAG_STORE_H
#define _PRN_TAG_STORE_H

#include <string>
#include <map>
#include "astra_consts.h"
#include "basic.h"
#include "astra_elems.h"
#include "astra_misc.h"
#include "dev_utils.h"
#include "payment.h"
#include "remarks.h"

const std::string FT_M61 = "M61"; // form type
const std::string FT_298_401 = "298 401";
const std::string FT_298_451 = "298 451";

namespace TAG {
    const std::string BCBP_M_2 = "BCBP_M_2";
    const std::string ACT = "ACT";
    const std::string AGENT = "AGENT";
    const std::string AIRLINE = "AIRLINE";
    const std::string AIRLINE_SHORT = "AIRLINE_SHORT";
    const std::string AIRP_ARV = "AIRP_ARV";
    const std::string AIRP_ARV_NAME = "AIRP_ARV_NAME";
    const std::string AIRP_DEP = "AIRP_DEP";
    const std::string AIRP_DEP_NAME = "AIRP_DEP_NAME";
    const std::string BAG_AMOUNT = "BAG_AMOUNT"; // багаж в посадочном
    const std::string TAGS = "TAGS"; // багаж в посадочном
    const std::string BAG_WEIGHT = "BAG_WEIGHT";
    const std::string BAGGAGE = "BAGGAGE";
    const std::string BRD_FROM = "BRD_FROM";
    const std::string BRD_TO = "BRD_TO";
    const std::string CHD = "CHD";
    const std::string CITY_ARV_NAME = "CITY_ARV_NAME";
    const std::string CITY_DEP_NAME = "CITY_DEP_NAME";
    const std::string CLASS = "CLASS";
    const std::string CLASS_NAME = "CLASS_NAME";
    const std::string DESK = "DESK";
    const std::string DOCUMENT = "DOCUMENT";
    const std::string DUPLICATE = "DUPLICATE";
    const std::string EST = "EST";
    const std::string ETICKET_NO = "ETICKET_NO";
    const std::string ETKT = "ETKT";
    const std::string EXCESS = "EXCESS";
    const std::string FLT_NO = "FLT_NO";
    const std::string FQT = "FQT";
    const std::string FULL_PLACE_ARV = "FULL_PLACE_ARV";
    const std::string FULL_PLACE_DEP = "FULL_PLACE_DEP";
    const std::string FULLNAME = "FULLNAME";
    const std::string GATE = "GATE";
    const std::string GATES = "GATES";
    const std::string INF = "INF";
    const std::string LONG_ARV = "LONG_ARV";
    const std::string LONG_DEP = "LONG_DEP";
    const std::string NAME = "NAME";
    const std::string NO_SMOKE = "NO_SMOKE";
    const std::string ONE_SEAT_NO = "ONE_SEAT_NO";
    const std::string PAX_ID = "PAX_ID";
    const std::string PLACE_ARV = "PLACE_ARV";
    const std::string PLACE_DEP = "PLACE_DEP";
    const std::string REG_NO = "REG_NO";
    const std::string REM = "REM";
    const std::string RK_AMOUNT = "RK_AMOUNT";
    const std::string RK_WEIGHT = "RK_WEIGHT";
    const std::string RSTATION = "RSTATION";
    const std::string SCD = "SCD";
    const std::string SEAT_NO = "SEAT_NO";
    const std::string STR_SEAT_NO = "STR_SEAT_NO";
    const std::string SUBCLS = "SUBCLS";
    const std::string LIST_SEAT_NO = "LIST_SEAT_NO";
    const std::string SURNAME = "SURNAME";
    const std::string TEST_SERVER = "TEST_SERVER";
    const std::string TIME_PRINT = "TIME_PRINT";

    // specific for bag tags
    const std::string AIRCODE = "AIRCODE";
    const std::string AIRLINE_NAME = "AIRLINE_NAME";
    const std::string NO = "NO";
    const std::string ISSUED = "ISSUED";
    const std::string BT_AMOUNT = "BT_AMOUNT"; // багаж в бирке
    const std::string BT_WEIGHT = "BT_WEIGHT";
    const std::string LIAB_LIMIT = "LIAB_LIMIT";
    const std::string FLT_NO1 = "FLT_NO1";
    const std::string FLT_NO2 = "FLT_NO2";
    const std::string FLT_NO3 = "FLT_NO3";
    const std::string LOCAL_DATE1 = "LOCAL_DATE1";
    const std::string LOCAL_DATE2 = "LOCAL_DATE2";
    const std::string LOCAL_DATE3 = "LOCAL_DATE3";
    const std::string AIRLINE1 = "AIRLINE1";
    const std::string AIRLINE2 = "AIRLINE2";
    const std::string AIRLINE3 = "AIRLINE3";
    const std::string AIRP_ARV1 = "AIRP_ARV1";
    const std::string AIRP_ARV2 = "AIRP_ARV2";
    const std::string AIRP_ARV3 = "AIRP_ARV3";
    const std::string FLTDATE1 = "FLTDATE1";
    const std::string FLTDATE2 = "FLTDATE2";
    const std::string FLTDATE3 = "FLTDATE3";
    const std::string AIRP_ARV_NAME1 = "AIRP_ARV_NAME1";
    const std::string AIRP_ARV_NAME2 = "AIRP_ARV_NAME2";
    const std::string AIRP_ARV_NAME3 = "AIRP_ARV_NAME3";
    const std::string PNR = "PNR";

    // specific for bag receipts
    const std::string BULKY_BT = "BULKYBT";
    const std::string BULKY_BT_LETTER = "BULKYBTLETTER";
    const std::string GOLF_BT = "GOLFBT";
    const std::string OTHER_BT = "OTHERBT";
    const std::string OTHER_BT_LETTER = "OTHERBTLETTER";
    const std::string PET_BT = "PETBT";
    const std::string SKI_BT = "SKIBT";
    const std::string VALUE_BT = "VALUEBT";
    const std::string VALUE_BT_LETTER = "VALUEBTLETTER";
    const std::string AIRLINE_CODE = "AIRLINE_CODE";
    const std::string AMOUNT_FIGURES = "AMOUNT_FIGURES";
    const std::string AMOUNT_LETTERS = "AMOUNT_LETTERS";
    const std::string BAG_NAME = "BAG_NAME";
    const std::string CHARGE = "CHARGE";
    const std::string CURRENCY = "CURRENCY";
    const std::string EQUI_AMOUNT_PAID = "EQUI_AMOUNT_PAID";
    const std::string EX_WEIGHT = "EX_WEIGHT";
    const std::string EXCHANGE_RATE = "EXCHANGE_RATE";
    const std::string ISSUE_DATE = "ISSUE_DATE";
    const std::string ISSUE_PLACE1 = "ISSUE_PLACE1";
    const std::string ISSUE_PLACE2 = "ISSUE_PLACE2";
    const std::string ISSUE_PLACE3 = "ISSUE_PLACE3";
    const std::string ISSUE_PLACE4 = "ISSUE_PLACE4";
    const std::string ISSUE_PLACE5 = "ISSUE_PLACE5";
    const std::string NDS = "NDS";
    const std::string PAX_DOC = "PAX_DOC";
    const std::string PAX_NAME = "PAX_NAME";
    const std::string PAY_FORM = "PAY_FORM";
    const std::string POINT_DEP = "POINT_DEP";
    const std::string POINT_ARV = "POINT_ARV";
    const std::string PREV_NO = "PREV_NO";
    const std::string RATE = "RATE";
    const std::string REMARKS1 = "REMARKS1";
    const std::string REMARKS2 = "REMARKS2";
    const std::string REMARKS3 = "REMARKS3";
    const std::string REMARKS4 = "REMARKS4";
    const std::string REMARKS5 = "REMARKS5";
    const std::string SERVICE_TYPE = "SERVICE_TYPE";
    const std::string TICKETS = "TICKETS";
    const std::string TO = "TO";
    const std::string TOTAL = "TOTAL";

};

class TTagLang {
    private:
        std::string route_country;
        std::string desk_lang;
        bool is_inter;
        std::string tag_lang; // параметр тега E - лат, R - рус
        bool pr_lat;
        std::string GetLang(TElemFmt &fmt, std::string firm_lang) const;
        bool IsInter(const TTrferRoute &aroute, std::string &country);
    public:
        std::string getRouteCountry() { return route_country; }
        bool get_pr_lat() { return pr_lat; };
        bool english_tag() const { return tag_lang == "E"; }
        bool IsInter() const;
        std::string GetLang();
        std::string dup_lang() { return GetLang()==AstraLocale::LANG_EN ? AstraLocale::LANG_RU : GetLang(); }; // lang for duplicated captions
        void set_tag_lang(std::string val) { tag_lang = val; };
        std::string ElemIdToTagElem(TElemType type, const std::string &id, TElemFmt fmt, std::string firm_lang = "") const;
        std::string ElemIdToTagElem(TElemType type, int id, TElemFmt fmt, std::string firm_lang = "") const;
        void Init(bool apr_lat); // Инициализация для использования в тестовых пектабах.
        void Init(const TBagReceipt &arcpt, bool apr_lat); // Bag receipts
        void Init(const std::string &airp_dep, const std::string &airp_arv, bool apr_lat); // BT, BR
};

int separate_double(double d, int precision, int *iptr);

class TPrnTagStore {
    private:


        TBagReceipt rcpt;

        struct TFieldParams {
            std::string date_format;
            boost::any TagInfo;
            size_t len;
            TFieldParams(std::string adate_format, boost::any aTagInfo, int alen):
                date_format(adate_format),
                TagInfo(aTagInfo),
                len(alen)
            {};
        };

        typedef std::string (TPrnTagStore::*TTagFunct)(TFieldParams fp);

        struct TPrnTestTagsItem {
            char type;
            std::string value, value_lat;
            bool processed;
            TPrnTestTagsItem(char vtype, std::string vvalue, std::string vvalue_lat):
                type(vtype),
                value(vvalue),
                value_lat(vvalue_lat),
                processed(false)
            {};
        };

        struct TPrnTestTags {
            std::map<std::string, TPrnTestTagsItem> items;
            void Init();
        };

        TPrnTestTags prn_test_tags;

        struct TTagPropsItem {
            size_t length;
            bool except_when_great_len;
            bool except_when_only_lat;
            bool convert_char_view;
            TTagPropsItem(int vlength, bool vexcept_when_great_len, bool vexcept_when_only_lat, bool vconvert_char_view):
                length(vlength),
                except_when_great_len(vexcept_when_great_len),
                except_when_only_lat(vexcept_when_only_lat),
                convert_char_view(vconvert_char_view)
            {};
        };

        struct TTagProps {
            ASTRA::TDevOperType op;
            std::map<std::string, TTagPropsItem> items;
            TTagProps(ASTRA::TDevOperType vop);
            void Init(ASTRA::TDevOperType vop);
        };



        struct TTagListItem {
            // Если длина тега меньше указанного порога и данные не влезли, заменяем вопросами,
            // иначе обрезаем до длины тега. Если порог = 0 и длина тега меньше длины данных, заменяем вопросами
            TTagFunct tag_funct;
            int info_type;
            bool processed;
            // Если тег встречался в образе несколько раз, то этот признак указывает, все
            // варианты были на английском или нет. Используется для записи в таблицу bp_print
            bool english_only;

            boost::any TagInfo; // данные из set_tag
            TTagListItem(TTagFunct funct, int ainfo_type = 0):
                tag_funct(funct),
                info_type(ainfo_type),
                processed(false),
                english_only(true)
            {};
            TTagListItem(): tag_funct(NULL), english_only(true) {};
        };

        int pax_id;
        int print_mode;

        std::map<const std::string, TTagListItem> tag_list;

        struct TPointInfo {
            BASIC::TDateTime scd, est, act;
            int point_id;
            std::string craft, bort;
            std::string airline, suffix;
            std::vector<std::string> gates;
            int flt_no;
            TPointInfo():
                scd(ASTRA::NoExists),
                est(ASTRA::NoExists),
                act(ASTRA::NoExists),
                point_id(ASTRA::NoExists)
            {};
            void Init(int apoint_id, int grp_id);
        };
        TPointInfo pointInfo;

        struct TGrpInfo {
            int grp_id;
            std::string airp_dep;
            std::string airp_arv;
            int point_dep, point_arv;
            int class_grp;
            int excess;
            TGrpInfo():
                grp_id(ASTRA::NoExists),
                class_grp(ASTRA::NoExists)
            {};
            void Init(int agrp_id, int apax_id);
        };
        TGrpInfo grpInfo;

        struct TFqtInfo {
            bool pr_init;
            std::string airline, no, extra;
            TFqtInfo(): pr_init(false) {};
            void Init(int apax_id);
        };
        TFqtInfo fqtInfo;

        struct TPaxInfo {
            int pax_id;
            std::string name, surname, document;
            std::string name_2d;
            std::string surname_2d;
            std::string ticket_rem;
            std::string ticket_no;
            int coupon_no;
            int reg_no;
            bool pr_smoke;
            int seats;
            std::string pers_type;
            int bag_amount, bag_weight;
            int rk_amount, rk_weight;
            std::string tags;
            std::string subcls;
            bool pr_bp_print;
            TPaxInfo():
                pax_id(ASTRA::NoExists),
                coupon_no(ASTRA::NoExists),
                reg_no(ASTRA::NoExists),
                pr_smoke(false),
                seats(ASTRA::NoExists),
                bag_amount(ASTRA::NoExists),
                bag_weight(ASTRA::NoExists),
                rk_amount(ASTRA::NoExists),
                rk_weight(ASTRA::NoExists),
                pr_bp_print(false)
            {};
            void Init(int apax_id, TTagLang &tag_lang);
        };
        TPaxInfo paxInfo;

        struct TBrdInfo {
            BASIC::TDateTime brd_from, brd_to, brd_to_est, brd_to_scd;
            TBrdInfo():
                brd_from(ASTRA::NoExists),
                brd_to(ASTRA::NoExists),
                brd_to_est(ASTRA::NoExists),
                brd_to_scd(ASTRA::NoExists)
            {};
            void Init(int point_id);
        };
        TBrdInfo brdInfo;

        struct TRemInfo {
            bool pr_init;
            TRemGrp rem;
            TRemInfo(): pr_init(false) {}
            void Init(int point_id);
        };
        TRemInfo remInfo;

        struct TPnrInfo {
            bool pr_init;
            std::string airline;
            std::vector<TPnrAddrItem> pnrs;
            TPnrInfo(): pr_init(false) {};
            void Init(int pax_id);
        };
        TPnrInfo pnrInfo;

        struct TRemarksInfo:public std::vector<std::string> {
            private:
                bool fexists;
            public:
                TRemarksInfo(): fexists(false) {}
                bool exists() { return fexists; };
                void Init(TPrnTagStore &pts);
                void add(const std::string &val);
                std::string rem_at(size_t idx);
        };
        TRemarksInfo remarksInfo;


        struct TTimePrint {
            BASIC::TDateTime val;
            TTimePrint(BASIC::TDateTime aval): val(aval) {};
        };

        TTimePrint time_print;

        struct TRStationInfo {
            bool pr_init;
            std::string name;
            TRStationInfo(): pr_init(false) {};
            void Init();
        };
        TRStationInfo rstationInfo;

        std::string get_fmt_seat(std::string fmt, bool english_tag);

        std::string BCBP_M_2(TFieldParams fp);
        std::string ACT(TFieldParams fp);
        std::string AGENT(TFieldParams fp);
        std::string AIRLINE(TFieldParams fp);
        std::string AIRLINE_SHORT(TFieldParams fp);
        std::string AIRP_ARV(TFieldParams fp);
        std::string AIRP_ARV_NAME(TFieldParams fp);
        std::string AIRP_DEP(TFieldParams fp);
        std::string AIRP_DEP_NAME(TFieldParams fp);
        std::string BAG_AMOUNT(TFieldParams fp);
        std::string TAGS(TFieldParams fp);
        std::string BAG_WEIGHT(TFieldParams fp);
        std::string BAGGAGE(TFieldParams fp);
        std::string BRD_FROM(TFieldParams fp);
        std::string BRD_TO(TFieldParams fp);
        std::string CHD(TFieldParams fp);
        std::string CITY_ARV_NAME(TFieldParams fp);
        std::string CITY_DEP_NAME(TFieldParams fp);
        std::string CLASS(TFieldParams fp);
        std::string CLASS_NAME(TFieldParams fp);
        std::string DESK(TFieldParams fp);
        std::string DOCUMENT(TFieldParams fp);
        std::string DUPLICATE(TFieldParams fp);
        std::string EST(TFieldParams fp);
        std::string ETICKET_NO(TFieldParams fp);
        std::string ETKT(TFieldParams fp);
        std::string EXCESS(TFieldParams fp);
        std::string FLT_NO(TFieldParams fp);
        std::string FQT(TFieldParams fp);
        std::string FULL_PLACE_ARV(TFieldParams fp);
        std::string FULL_PLACE_DEP(TFieldParams fp);
        std::string FULLNAME(TFieldParams fp);
        std::string GATE(TFieldParams fp);
        std::string GATES(TFieldParams fp);
        std::string INF(TFieldParams fp);
        std::string LONG_ARV(TFieldParams fp);
        std::string LONG_DEP(TFieldParams fp);
        std::string NAME(TFieldParams fp);
        std::string NO_SMOKE(TFieldParams fp);
        std::string ONE_SEAT_NO(TFieldParams fp);
        std::string PAX_ID(TFieldParams fp);
        std::string PLACE_ARV(TFieldParams fp);
        std::string PLACE_DEP(TFieldParams fp);
        std::string REG_NO(TFieldParams fp);
        std::string REM(TFieldParams fp);
        std::string RK_AMOUNT(TFieldParams fp);
        std::string RK_WEIGHT(TFieldParams fp);
        std::string RSTATION(TFieldParams fp);
        std::string SCD(TFieldParams fp);
        std::string SEAT_NO(TFieldParams fp);
        std::string STR_SEAT_NO(TFieldParams fp);
        std::string SUBCLS(TFieldParams fp);
        std::string LIST_SEAT_NO(TFieldParams fp);
        std::string SURNAME(TFieldParams fp);
        std::string TEST_SERVER(TFieldParams fp);
        std::string TIME_PRINT(TFieldParams fp);

        // specific for bag tags
        std::string AIRCODE(TFieldParams fp);
        std::string AIRLINE_NAME(TFieldParams fp);
        std::string NO(TFieldParams fp);
        std::string ISSUED(TFieldParams fp);
        std::string BT_AMOUNT(TFieldParams fp);
        std::string BT_WEIGHT(TFieldParams fp);
        std::string LIAB_LIMIT(TFieldParams fp);
        std::string FLT_NO1(TFieldParams fp);
        std::string FLT_NO2(TFieldParams fp);
        std::string FLT_NO3(TFieldParams fp);
        std::string LOCAL_DATE1(TFieldParams fp);
        std::string LOCAL_DATE2(TFieldParams fp);
        std::string LOCAL_DATE3(TFieldParams fp);
        std::string AIRLINE1(TFieldParams fp);
        std::string AIRLINE2(TFieldParams fp);
        std::string AIRLINE3(TFieldParams fp);
        std::string AIRP_ARV1(TFieldParams fp);
        std::string AIRP_ARV2(TFieldParams fp);
        std::string AIRP_ARV3(TFieldParams fp);
        std::string FLTDATE1(TFieldParams fp);
        std::string FLTDATE2(TFieldParams fp);
        std::string FLTDATE3(TFieldParams fp);
        std::string AIRP_ARV_NAME1(TFieldParams fp);
        std::string AIRP_ARV_NAME2(TFieldParams fp);
        std::string AIRP_ARV_NAME3(TFieldParams fp);
        std::string PNR(TFieldParams fp);

        // specific for bag receipts
        std::string BULKY_BT(TFieldParams fp);
        std::string BULKY_BT_LETTER(TFieldParams fp);
        std::string GOLF_BT(TFieldParams fp);
        std::string OTHER_BT(TFieldParams fp);
        std::string OTHER_BT_LETTER(TFieldParams fp);
        std::string PET_BT(TFieldParams fp);
        std::string SKI_BT(TFieldParams fp);
        std::string VALUE_BT(TFieldParams fp);
        std::string VALUE_BT_LETTER(TFieldParams fp);
        std::string BR_AIRCODE(TFieldParams fp);
        std::string BR_AIRLINE(TFieldParams fp);
        std::string AIRLINE_CODE(TFieldParams fp);
        std::string AMOUNT_FIGURES(TFieldParams fp);
        std::string AMOUNT_LETTERS(TFieldParams fp);
        std::string BAG_NAME(TFieldParams fp);
        std::string CHARGE(TFieldParams fp);
        std::string CURRENCY(TFieldParams fp);
        std::string EQUI_AMOUNT_PAID(TFieldParams fp);
        std::string EX_WEIGHT(TFieldParams fp);
        std::string EXCHANGE_RATE(TFieldParams fp);
        std::string ISSUE_DATE(TFieldParams fp);
        std::string ISSUE_PLACE1(TFieldParams fp);
        std::string ISSUE_PLACE2(TFieldParams fp);
        std::string ISSUE_PLACE3(TFieldParams fp);
        std::string ISSUE_PLACE4(TFieldParams fp);
        std::string ISSUE_PLACE5(TFieldParams fp);
        std::string NDS(TFieldParams fp);
        std::string PAX_DOC(TFieldParams fp);
        std::string PAX_NAME(TFieldParams fp);
        std::string PAY_FORM(TFieldParams fp);
        std::string POINT_DEP(TFieldParams fp);
        std::string POINT_ARV(TFieldParams fp);
        std::string PREV_NO(TFieldParams fp);
        std::string RATE(TFieldParams fp);
        std::string REMARKS1(TFieldParams fp);
        std::string REMARKS2(TFieldParams fp);
        std::string REMARKS3(TFieldParams fp);
        std::string REMARKS4(TFieldParams fp);
        std::string REMARKS5(TFieldParams fp);
        std::string SERVICE_TYPE(TFieldParams fp);
        std::string TICKETS(TFieldParams fp);
        std::string TO(TFieldParams fp);
        std::string TOTAL(TFieldParams fp);

        std::string get_test_field(std::string name, size_t len, std::string date_format);
        std::string get_real_field(std::string name, size_t len, std::string date_format);

    public:
        TTagProps prn_tag_props;
        TTagLang tag_lang;
        TPrnTagStore(int agrp_id, int apax_id, int apr_lat, xmlNodePtr tagsNode, const TTrferRoute &aroute = TTrferRoute());
        TPrnTagStore(bool apr_lat);
        TPrnTagStore(const TBagReceipt &arcpt, bool apr_lat);
        void set_tag(std::string name, std::string value);
        void set_tag(std::string name, int value);
        void set_tag(std::string name, BASIC::TDateTime value);
        std::string get_field(std::string name, size_t len, std::string align, std::string date_format, std::string tag_lang, bool pr_user_except = true);
        void save_bp_print(bool pr_print = false);
        std::string get_tag_no_err( // Версия get_tag, которая игнорирует ошибку "Данные печати не латинские"
                std::string name,
                std::string date_format = BASIC::ServerFormatDateTimeAsString,
                std::string tag_lang = "R"); // R - russian; E - english
        std::string get_tag(
                std::string name,
                std::string date_format = BASIC::ServerFormatDateTimeAsString,
                std::string tag_lang = "R"); // R - russian; E - english
        bool tag_processed(std::string name);
        void set_print_mode(int val);
        void clear();
        BASIC::TDateTime get_time_print() { return time_print.val; };

        void tst_get_tag_list(std::vector<std::string> &tag_list);
};

#endif
