#ifndef _PRN_TAG_STORE_H
#define _PRN_TAG_STORE_H

#include <string>
#include <map>
#include "astra_consts.h"
#include "date_time.h"
#include "astra_elems.h"
#include "astra_misc.h"
#include "dev_utils.h"
#include "payment.h"
#include "remarks.h"
#include "passenger.h"
#include "bi_rules.h"
#include "brands.h"
#include "rfisc_price.h"

using BASIC::date_time::TDateTime;

const std::string FT_M61 = "M61"; // form type
const std::string FT_298_401 = "298 401";
const std::string FT_298_451 = "298 451";
const std::string FT_823_451 = "823 451";
const std::string FT_FV_451 = "?? 451";

namespace TAG {
    const std::string BCBP_V_5 = "BCBP_V_5";
    const std::string BCBP_M_2 = "BCBP_M_2";
    const std::string ACT = "ACT";
    const std::string AGENT = "AGENT";
    const std::string AIRLINE = "AIRLINE";
    const std::string AIRLINE_SHORT = "AIRLINE_SHORT";
    const std::string AIRP_ARV = "AIRP_ARV";
    const std::string AIRP_ARV_NAME = "AIRP_ARV_NAME";
    const std::string AIRP_DEP = "AIRP_DEP";
    const std::string AIRP_DEP_NAME = "AIRP_DEP_NAME";
    const std::string BAG_AMOUNT = "BAG_AMOUNT"; // ????? ? ??ᠤ?筮?
    const std::string TAGS = "TAGS"; // ????? ? ??ᠤ?筮?
    const std::string BAG_WEIGHT = "BAG_WEIGHT";
    const std::string BAGGAGE = "BAGGAGE";
    const std::string BRAND = "BRAND";
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
    const std::string FQT_TIER_LEVEL = "FQT_TIER_LEVEL";
    const std::string FULL_PLACE_ARV = "FULL_PLACE_ARV";
    const std::string FULL_PLACE_DEP = "FULL_PLACE_DEP";
    const std::string FULLNAME = "FULLNAME";
    const std::string GATE = "GATE";
    const std::string GATES = "GATES";
    const std::string HALL = "HALL";
    const std::string INF = "INF";
    const std::string IMG = "IMG";
    const std::string LIST_SEAT_NO = "LIST_SEAT_NO";
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
    const std::string REM_TXT0 = "REM_TXT0";
    const std::string REM_TXT1 = "REM_TXT1";
    const std::string REM_TXT2 = "REM_TXT2";
    const std::string REM_TXT3 = "REM_TXT3";
    const std::string REM_TXT4 = "REM_TXT4";
    const std::string REM_TXT5 = "REM_TXT5";
    const std::string REM_TXT6 = "REM_TXT6";
    const std::string REM_TXT7 = "REM_TXT7";
    const std::string REM_TXT8 = "REM_TXT8";
    const std::string REM_TXT9 = "REM_TXT9";
    const std::string RFISC_BSN_LONGUE = "RFISC_BSN_LONGUE";
    const std::string RFISC_FAST_TRACK = "RFISC_FAST_TRACK";
    const std::string RFISC_UPGRADE = "RFISC_UPGRADE";
    const std::string RK_AMOUNT = "RK_AMOUNT";
    const std::string RK_WEIGHT = "RK_WEIGHT";
    const std::string RSTATION = "RSTATION";
    const std::string SCD = "SCD";
    const std::string SEAT_NO = "SEAT_NO";
    const std::string STR_SEAT_NO = "STR_SEAT_NO";
    const std::string SUBCLS = "SUBCLS";
    const std::string SURNAME = "SURNAME";
    const std::string TEST_SERVER = "TEST_SERVER";
    const std::string TICKET_NO = "TICKET_NO";
    const std::string TIME_PRINT = "TIME_PRINT";
    const std::string PAX_TITLE = "PAX_TITLE";
    const std::string BI_HALL = "BI_HALL";
    const std::string BI_HALL_CAPTION = "BI_HALL_CAPTION";
    const std::string BI_RULE = "BI_RULE";
    const std::string BI_RULE_GUEST = "BI_RULE_GUEST";
    const std::string BI_AIRP_TERMINAL = "BI_AIRP_TERMINAL";
    const std::string VOUCHER_CODE = "VOUCHER_CODE";
    const std::string VOUCHER_TEXT = "VOUCHER_TEXT";
    const std::string VOUCHER_TEXT1 = "VOUCHER_TEXT1";
    const std::string VOUCHER_TEXT2 = "VOUCHER_TEXT2";
    const std::string VOUCHER_TEXT3 = "VOUCHER_TEXT3";
    const std::string VOUCHER_TEXT4 = "VOUCHER_TEXT4";
    const std::string VOUCHER_TEXT5 = "VOUCHER_TEXT5";
    const std::string VOUCHER_TEXT6 = "VOUCHER_TEXT6";
    const std::string VOUCHER_TEXT7 = "VOUCHER_TEXT7";
    const std::string VOUCHER_TEXT8 = "VOUCHER_TEXT8";
    const std::string VOUCHER_TEXT9 = "VOUCHER_TEXT9";
    const std::string VOUCHER_TEXT10 = "VOUCHER_TEXT10";

    // specific for EMDA
    const std::string EMD_NO = "EMD_NO";
    const std::string EMD_COUPON = "EMD_COUPON";
    const std::string EMD_RFIC = "EMD_RFIC";
    const std::string EMD_RFISC = "EMD_RFISC";
    const std::string EMD_RFISC_DESCR = "EMD_RFISC_DESCR";
    const std::string EMD_PRICE = "EMD_PRICE";
    const std::string EMD_CURRENCY = "EMD_CURRENCY";

    // specific for bag tags
    const std::string AIRCODE = "AIRCODE";
    const std::string AIRLINE_NAME = "AIRLINE_NAME";
    const std::string NO = "NO";
    const std::string ISSUED = "ISSUED";
    const std::string BT_AMOUNT = "BT_AMOUNT"; // ????? ? ??થ
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
        std::string tag_lang; // ??ࠬ??? ⥣? E - ???, R - ???
        bool pr_lat;
        std::string GetLang(TElemFmt &fmt, std::string firm_lang) const;
        bool IsInter(const TTrferRoute &aroute, std::string &country);
    public:
        std::string getRouteCountry() { return route_country; }
        bool get_pr_lat() const { return pr_lat; };
        bool english_tag() const { return tag_lang == "E"; }
        bool IsInter() const;
        std::string GetLang() const;
        std::string dup_lang() { return GetLang()==AstraLocale::LANG_EN ? AstraLocale::LANG_RU : GetLang(); }; // lang for duplicated captions
        void set_tag_lang(std::string val) { tag_lang = val; };
        std::string get_tag_lang() { return tag_lang; }
        std::string ElemIdToTagElem(TElemType type, const std::string &id, TElemFmt fmt, std::string firm_lang = "") const;
        std::string ElemIdToTagElem(TElemType type, int id, TElemFmt fmt, std::string firm_lang = "") const;
        void Init(bool apr_lat); // ???樠??????? ??? ?ᯮ?짮????? ? ???⮢?? ???⠡??.
        void Init(const TBagReceipt &arcpt, bool apr_lat); // Bag receipts
        void Init(const std::string &airp_dep, const std::string &airp_arv, bool apr_lat); // BT, BR
};

int separate_double(double d, int precision, int *iptr);

class TBPServiceTypes {
    public:
        enum Enum
        {
            UP,
            LG,
            TS_FT,    // ??㯯? TS ? ?????㯯? FT
            SA,
            Unknown
        };

        static const std::list< std::pair<Enum, std::string> >& pairsCodes()
        {
            static std::list< std::pair<Enum, std::string> > l;
            if (l.empty())
            {
                l.push_back(std::make_pair(UP,      "UP"));
                l.push_back(std::make_pair(LG,      "LG"));
                l.push_back(std::make_pair(TS_FT,   "TS FT"));
                l.push_back(std::make_pair(SA,      "SA"));
                l.push_back(std::make_pair(Unknown, ""));
            }
            return l;
        }

        static const std::list< std::pair<Enum, std::string> >& pairsDescr()
        {
            static std::list< std::pair<Enum, std::string> > l;
            if (l.empty())
            {
                l.push_back(std::make_pair(UP,      "UPGRADE"));
                l.push_back(std::make_pair(LG,      "BUSINESS LOUNGE"));
                l.push_back(std::make_pair(TS_FT,    "FAST TRACK"));
                l.push_back(std::make_pair(Unknown, ""));
            }
            return l;
        }
};

class TBPServiceTypesCode : public ASTRA::PairList<TBPServiceTypes::Enum, std::string>
{
  private:
    virtual std::string className() const { return "TBPServiceTypesCode"; }
  public:
    TBPServiceTypesCode() : ASTRA::PairList<TBPServiceTypes::Enum, std::string>(TBPServiceTypes::pairsCodes(),
                                                                            boost::none,
                                                                            boost::none) {}
};

class TBPServiceTypesDescr : public ASTRA::PairList<TBPServiceTypes::Enum, std::string>
{
  private:
    virtual std::string className() const { return "TBPServiceTypesDescr"; }
  public:
    TBPServiceTypesDescr() : ASTRA::PairList<TBPServiceTypes::Enum, std::string>(TBPServiceTypes::pairsDescr(),
                                                                            boost::none,
                                                                            boost::none) {}
};

struct TBCBPData {
    std::string surname;
    std::string name;
    bool etkt;
    std::string pnr;
    std::string airp_dep;
    std::string airp_arv;
    std::string airline;
    int flt_no;
    std::string suffix;
    TDateTime scd;

    //   ?᫨ class_grp = NoExists, ? ??મ? ???????? cls
    std::string cls;
    int class_grp;

    std::string seat_no;
    int reg_no;
    std::string pers_type; // ??, ??, ??
    bool is_boarding_pass;
    int pax_id;
    bool is_rem_txt; // ? ???⠡? ?? ???????????? ⥣? rem_txt ? ???? ?? ???? ?? ??? ?? ???⮩

    void clear()
    {
        surname.clear();
        name.clear();
        etkt = false;
        pnr.clear();
        airp_dep.clear();
        airp_arv.clear();
        airline.clear();
        flt_no = ASTRA::NoExists;
        suffix.clear();
        scd = ASTRA::NoExists;
        cls.clear();
        class_grp = ASTRA::NoExists;
        seat_no.clear();
        reg_no = ASTRA::NoExists;
        pers_type.clear();
        is_boarding_pass = true;
        pax_id = ASTRA::NoExists;
        is_rem_txt = false;
    }

    TBCBPData() { clear(); }
    std::string toString(const TTagLang &tag_lang);
};

class TSpaceIfEmpty {
    private:
        bool val;
    public:
        TSpaceIfEmpty(bool _val): val(_val) {}
        void set(bool _val) { val = _val; }
        bool get() { return val; }
};

class TPrnTagStore {
    private:

        friend class TBCBPData;


        struct TImgMng {
            static TImgMng *Instance()
            {
                static boost::shared_ptr<TImgMng> instance_ = NULL;
                if ( !instance_ ) {
                    instance_ = boost::shared_ptr<TImgMng>(new TImgMng);
                }
                return instance_.get();
            }
        };

        boost::shared_ptr<BCBPSections> scan_data;
        const std::string scan; // ?????? 2D ??મ??
        boost::optional<const std::list<AstraLocale::LexemaData> &> errors;
        bool from_scan_code;
        TBagReceipt rcpt;

        struct TFieldParams {
            std::string tag_name;
            std::string date_format;
            boost::any TagInfo;
            size_t len;
            std::string text;
            TFieldParams(
                    const std::string &atag_name,
                    const std::string &adate_format,
                    boost::any aTagInfo,
                    int alen,
                    const std::string &atext):
                tag_name(atag_name),
                date_format(adate_format),
                TagInfo(aTagInfo),
                len(alen),
                text(atext)
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
            ASTRA::TDevOper::Enum op;
            std::map<std::string, TTagPropsItem> items;
            TTagProps(ASTRA::TDevOper::Enum vop);
            void Init(ASTRA::TDevOper::Enum vop);
        };



        struct TTagListItem {
            // ?᫨ ????? ⥣? ?????? 㪠??????? ??ண? ? ?????? ?? ??????, ?????塞 ?????ᠬ?,
            // ????? ??१??? ?? ????? ⥣?. ?᫨ ??ண = 0 ? ????? ⥣? ?????? ????? ??????, ?????塞 ?????ᠬ?
            TTagFunct tag_funct;
            int info_type;
            bool processed;
            // ?᫨ ⥣ ?????砫?? ? ??ࠧ? ??᪮?쪮 ࠧ, ?? ???? ?ਧ??? 㪠?뢠??, ???
            // ??ਠ??? ?뫨 ?? ??????᪮? ??? ???. ?ᯮ???????? ??? ?????? ? ⠡???? bp_print
            bool english_only;

            boost::any TagInfo; // ?????? ?? set_tag
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
            TTripInfo operFlt;
            // ????? ३?? (᫥?. ??? ????) ????? ?⫨?????? ?? operFlt (?࠭砩? ??? ??થ⨭?)
            std::string airline, suffix;
            int flt_no;
            std::vector<std::string> gates;
            std::vector<TInfantAdults> infants;
            TPointInfo() { clear(); }
            void clear()
            {
                operFlt.Clear();
                airline.clear();
                suffix.clear();
                flt_no = ASTRA::NoExists;
                gates.clear();
                infants.clear();
            }
            void Init(ASTRA::TDevOper::Enum op, int apoint_id, int grp_id);
        };
        TPointInfo pointInfo;

        struct TBIHallInfo {
            int hall_id;
            int airp_terminal_id;
            TBIHallInfo(): hall_id(ASTRA::NoExists), airp_terminal_id(ASTRA::NoExists) {}
        };
        TBIHallInfo BIHallInfo;

        struct TGrpInfo {
            int grp_id;
            std::string airp_dep;
            std::string airp_arv;
            int point_dep, point_arv;
            int hall;
            std::string status;
            TPrPrint prPrintInfo;
            bool pr_print_fio_pnl;
            TGrpInfo():
                grp_id(ASTRA::NoExists),
                point_dep(ASTRA::NoExists),
                point_arv(ASTRA::NoExists),
                hall(ASTRA::NoExists),
                pr_print_fio_pnl(false)
            {}
            void Init(int agrp_id, int apax_id);
        };
        TGrpInfo grpInfo;

        struct TFqtInfo {
            bool pr_init;
            std::string airline, no, extra, tier_level;
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
            bool is_jmp;
            int reg_no;
            bool pr_smoke;
            int seats;
            std::string pers_type;
            int bag_amount, bag_weight;
            int rk_amount, rk_weight;
            TBagKilos excess_wt;
            TBagPieces excess_pc;
            std::string tags;
            std::string subcls;
            std::string crs_cls;
            std::string cls;
            int class_grp;
            bool pr_bp_print;
            bool pr_bi_print;
            CheckIn::TPaxDocItem doc;
            TBrands brands;
            TPaxInfo():
                pax_id(ASTRA::NoExists),
                coupon_no(ASTRA::NoExists),
                is_jmp(false),
                reg_no(ASTRA::NoExists),
                pr_smoke(false),
                seats(ASTRA::NoExists),
                bag_amount(ASTRA::NoExists),
                bag_weight(ASTRA::NoExists),
                rk_amount(ASTRA::NoExists),
                rk_weight(ASTRA::NoExists),
                excess_wt(ASTRA::NoExists),
                excess_pc(ASTRA::NoExists),
                class_grp(ASTRA::NoExists),
                pr_bp_print(false),
                pr_bi_print(false)
            {}
            void Init(const TGrpInfo &grp_info, int apax_id, TTagLang &tag_lang);
        };
        TPaxInfo paxInfo;

        struct TBrdInfo {
            TDateTime brd_from, brd_to, brd_to_est, brd_to_scd;
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
            TPnrAddrs pnrs;
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
            TDateTime val;
            TTimePrint(TDateTime aval): val(aval) {};
        };

        TTimePrint time_print;

        struct TRStationInfo {
            bool pr_init;
            std::string name;
            TRStationInfo(): pr_init(false) {};
            void Init();
        };
        TRStationInfo rstationInfo;

        struct TEMDAInfo {
            private:
                boost::optional<TPriceRFISCList> prices;

                struct TResult {
                    std::string RFIC;
                    std::string RFISC;
                    std::string rfisc_descr;
                    std::string currency;
                    float price;
                    bool pr_print;
                    void clear()
                    {
                        RFIC.clear();
                        RFISC.clear();
                        rfisc_descr.clear();
                        currency.clear();
                        price = ASTRA::NoExists;
                        pr_print = false;
                    }
                };
                TResult res;
                bool find(int pax_id, boost::any &emd_no, boost::any &emd_coupon, const std::string &lang);
            public:
                void Init(int grp_id, int pax_id, boost::any &emd_no, boost::any &emd_coupon, const std::string &lang);
                float get_price() { return res.price; }
                std::string get_currency() { return res.currency; }
                std::string get_rfic() { return res.RFIC; }
                std::string get_rfisc() { return res.RFISC; }
                std::string get_rfisc_descr() { return res.rfisc_descr; }
                bool get_pr_print() { return res.pr_print; }
        };

        TEMDAInfo emdaInfo;

        std::string get_fmt_seat(std::string fmt, bool english_tag);
        std::string BCBP_M_2(TFieldParams fp);
        std::string BCBP_V_5(TFieldParams fp);
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
        std::string BRAND(TFieldParams fp);
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
        std::string FQT_TIER_LEVEL(TFieldParams fp);
        std::string FQT(TFieldParams fp);
        std::string FULL_PLACE_ARV(TFieldParams fp);
        std::string FULL_PLACE_DEP(TFieldParams fp);
        std::string FULLNAME(TFieldParams fp);
        std::string GATE(TFieldParams fp);
        std::string GATES(TFieldParams fp);
        std::string HALL(TFieldParams fp);
        std::string IMG(TFieldParams fp);
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
        std::string REM_TXT(TFieldParams fp);
        std::string RFISC_BSN_LONGUE(TFieldParams fp);
        std::string RFISC_FAST_TRACK(TFieldParams fp);
        std::string RFISC_UPGRADE(TFieldParams fp);
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
        std::string TICKET_NO(TFieldParams fp);
        std::string TIME_PRINT(TFieldParams fp);
        std::string PAX_TITLE(TFieldParams fp);
        std::string BI_HALL(TFieldParams fp);
        std::string BI_HALL_CAPTION(TFieldParams fp);
        std::string BI_RULE(TFieldParams fp);
        std::string BI_RULE_GUEST(TFieldParams fp);
        std::string BI_AIRP_TERMINAL(TFieldParams fp);
        std::string VOUCHER_CODE(TFieldParams fp);
        std::string VOUCHER_TEXT(TFieldParams fp);
        std::string VOUCHER_TEXT_FREE(TFieldParams fp);

        // specific for EMDA
        std::string EMD_NO(TFieldParams fp);
        std::string EMD_COUPON(TFieldParams fp);
        std::string EMD_RFIC(TFieldParams fp);
        std::string EMD_RFISC(TFieldParams fp);
        std::string EMD_RFISC_DESCR(TFieldParams fp);
        std::string EMD_PRICE(TFieldParams fp);
        std::string EMD_CURRENCY(TFieldParams fp);

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

        std::string get_test_field(const std::string &name, size_t len, const std::string &text, const std::string &date_format);
        std::string get_real_field(const std::string &name, size_t len, const std::string &text, const std::string &date_format);
        std::string get_field_from_bcbp(const std::string &name, size_t len, const std::string &text, const std::string &date_format);

        void init_bp_tags();

        bool isBoardingPass();

        struct TRfiscDescr {
            std::set<TBPServiceTypes::Enum> found_services;

            void fromDB(int grp_id, int pax_id);
            std::string get(const std::string &crs_cls, TBPServiceTypes::Enum code, TPrnTagStore &pts);

            TRfiscDescr()
            {
                clear();
            }

            void clear()
            {
                found_services.clear();
            }
            void dump() const;
        };
        TRfiscDescr rfisc_descr;

        TSpaceIfEmpty space_if_empty;

        // ᯨ᮪ ???? ⥣?? ? ???⠡?
        std::vector<std::string> pectab_tags;
        bool rem_txt_exists();

    public:
        TTagProps prn_tag_props;
        TTagLang tag_lang;
        ASTRA::TDevOper::Enum get_op_type() { return prn_tag_props.op; }
        TPrnTagStore(ASTRA::TDevOper::Enum _op_type, int agrp_id, int apax_id, bool afrom_scan_code, int apr_lat, xmlNodePtr tagsNode, const TTrferRoute &aroute = TTrferRoute());
        TPrnTagStore(ASTRA::TDevOper::Enum _op_type, const std::string &ascan, boost::optional<const std::list<AstraLocale::LexemaData> &> _errors, bool apr_lat);
        TPrnTagStore(bool apr_lat);
        TPrnTagStore(const TBagReceipt &arcpt, bool apr_lat);
        TPrnTagStore(ASTRA::TDevOper::Enum _op_type, const std::string& airp_dep, const std::string& airp_arv, bool apr_lat);

        void set_tag(std::string name, const TBCBPData &value);
        void set_tag(std::string name, const BIPrintRules::TRule &value);
        void set_tag(std::string name, std::string value);
        void set_tag(std::string name, int value);
        void set_tag(std::string name, TDateTime value);
        std::string get_field(std::string name, size_t len, const std::string &text, std::string align, std::string date_format, std::string tag_lang, bool pr_user_except = true);
        void save_foreign_scan();
        void confirm_print(bool pr_print, ASTRA::TDevOper::Enum op_type);
        std::string get_tag_no_err( // ?????? get_tag, ??????? ?????????? ?訡?? "?????? ?????? ?? ??⨭᪨?"
                std::string name,
                std::string date_format = BASIC::date_time::ServerFormatDateTimeAsString,
                std::string tag_lang = "R"); // R - russian; E - english
        std::string get_tag(
                std::string name,
                std::string date_format = BASIC::date_time::ServerFormatDateTimeAsString,
                std::string tag_lang = "R"); // R - russian; E - english
        bool tag_processed(std::string name);
        void set_print_mode(int val);
        void clear();
        TDateTime get_time_print() { return time_print.val; };
        void tst_get_tag_list(std::vector<std::string> &tag_list);
        void tagsFromXML(xmlNodePtr tagsNode);
        void set_space_if_empty(bool val) { space_if_empty.set(val); };
        std::string errorsToString();
        void get_pectab_tags(const std::string &form);
        void get_pectab_tags(const std::vector<std::string> &tags);
};

bool get_pr_print_emda(int pax_id, const std::string &emd_no, int emd_coupon);

#endif
