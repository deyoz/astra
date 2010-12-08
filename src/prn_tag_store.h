#ifndef _PRN_TAG_STORE_H
#define _PRN_TAG_STORE_H

#include <string>
#include <map>
#include "astra_consts.h"
#include "basic.h"
#include "astra_elems.h"
#include "astra_misc.h"

namespace TAG {
    const std::string BCBP_M_2 = "BCBP_M_2";
    const std::string ACT = "ACT";
    const std::string AIRLINE = "AIRLINE";
    const std::string AIRLINE_SHORT = "AIRLINE_SHORT";
    const std::string AIRP_ARV = "AIRP_ARV";
    const std::string AIRP_ARV_NAME = "AIRP_ARV_NAME";
    const std::string AIRP_DEP = "AIRP_DEP";
    const std::string AIRP_DEP_NAME = "AIRP_DEP_NAME";
    const std::string BAG_AMOUNT = "BAG_AMOUNT"; // багаж в посадочном
    const std::string BAG_WEIGHT = "BAG_WEIGHT";
    const std::string BRD_FROM = "BRD_FROM";
    const std::string BRD_TO = "BRD_TO";
    const std::string BT_AMOUNT = "BT_AMOUNT"; // багаж в бирке
    const std::string BT_WEIGHT = "BT_WEIGHT";
    const std::string CITY_ARV_NAME = "CITY_ARV_NAME";
    const std::string CITY_DEP_NAME = "CITY_DEP_NAME";
    const std::string CLASS = "CLASS";
    const std::string CLASS_NAME = "CLASS_NAME";
    const std::string DOCUMENT = "DOCUMENT";
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
    const std::string LONG_ARV = "LONG_ARV";
    const std::string LONG_DEP = "LONG_DEP";
    const std::string NAME = "NAME";
    const std::string NO_SMOKE = "NO_SMOKE";
    const std::string ONE_SEAT_NO = "ONE_SEAT_NO";
    const std::string PAX_ID = "PAX_ID";
    const std::string PLACE_ARV = "PLACE_ARV";
    const std::string PLACE_DEP = "PLACE_DEP";
    const std::string REG_NO = "REG_NO";
    const std::string SCD = "SCD";
    const std::string SEAT_NO = "SEAT_NO";
    const std::string STR_SEAT_NO = "STR_SEAT_NO";
    const std::string SURNAME = "SURNAME";
    const std::string TEST_SERVER = "TEST_SERVER";
};

class TPrnTagStore {
    private:

        class TTagLang {
            private:
                bool route_inter;
                std::string tag_lang; // параметр тега E - лат, R - рус
                std::string route_country_lang; //язык страны, где выполняется внутренний рейс
                bool pr_lat;
                std::string GetLang(TElemFmt &fmt, std::string firm_lang) const;
            public:
                bool IsInter() const;
                std::string GetLang();
                std::string dup_lang() { return GetLang()==AstraLocale::LANG_EN ? AstraLocale::LANG_RU : GetLang(); }; // lang for duplicated captions
                void set_tag_lang(std::string val) { tag_lang = val; };
                std::string ElemIdToTagElem(TElemType type, const std::string &id, TElemFmt fmt, std::string firm_lang = "") const;
                std::string ElemIdToTagElem(TElemType type, int id, TElemFmt fmt, std::string firm_lang = "") const;
                void Init(int point_dep, int point_arv, bool apr_lat);
        };

        TTagLang tag_lang;

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

        struct TTagListItem {
            TTagFunct tag_funct;
            int info_type;
            boost::any TagInfo; // данные из set_tag
            TTagListItem(TTagFunct funct, int ainfo_type): tag_funct(funct), info_type(ainfo_type) {};
            TTagListItem(TTagFunct funct): tag_funct(funct), info_type(0) {};
            TTagListItem(): tag_funct(NULL) {};
        };

        int pax_id, pr_lat;

        std::map<const std::string, TTagListItem> tag_list;

        struct TPointInfo {
            BASIC::TDateTime scd, est, act;
            int point_id;
            std::string craft, bort;
            std::string airline, suffix;
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
            void Init(int agrp_id);
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
            std::string ticket_rem;
            float ticket_no;
            int coupon_no;
            int reg_no;
            std::string no_smoke;
            int seats;
            std::string pers_type;
            TPaxInfo():
                pax_id(ASTRA::NoExists),
                ticket_no(ASTRA::NoExists),
                coupon_no(ASTRA::NoExists)
            {};
            void Init(int apax_id);
        };
        TPaxInfo paxInfo;

        struct TBagInfo {
            int bag_amount, bag_weight;
            TBagInfo(): bag_amount(ASTRA::NoExists), bag_weight(ASTRA::NoExists) {};
            void Init(int grp_id);
        };
        TBagInfo bagInfo;

        struct TBrdInfo {
            BASIC::TDateTime brd_from, brd_to;
            TBrdInfo(): brd_from(ASTRA::NoExists), brd_to(ASTRA::NoExists) {};
            void Init(int point_id);
        };
        TBrdInfo brdInfo;

        struct TPnrInfo {
            bool pr_init;
            std::vector<TPnrAddrItem> pnrs;
            TPnrInfo(): pr_init(false) {};
            void Init(int pax_id);
        };
        TPnrInfo pnrInfo;

        std::string BCBP_M_2(TFieldParams fp);
        std::string ACT(TFieldParams fp);
        std::string AIRLINE(TFieldParams fp);
        std::string AIRLINE_SHORT(TFieldParams fp);
        std::string AIRP_ARV(TFieldParams fp);
        std::string AIRP_ARV_NAME(TFieldParams fp);
        std::string AIRP_DEP(TFieldParams fp);
        std::string AIRP_DEP_NAME(TFieldParams fp);
        std::string BAG_AMOUNT(TFieldParams fp);
        std::string BAG_WEIGHT(TFieldParams fp);
        std::string BRD_FROM(TFieldParams fp);
        std::string BRD_TO(TFieldParams fp);
        std::string BT_AMOUNT(TFieldParams fp);
        std::string BT_WEIGHT(TFieldParams fp);
        std::string CITY_ARV_NAME(TFieldParams fp);
        std::string CITY_DEP_NAME(TFieldParams fp);
        std::string CLASS(TFieldParams fp);
        std::string CLASS_NAME(TFieldParams fp);
        std::string DOCUMENT(TFieldParams fp);
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
        std::string LONG_ARV(TFieldParams fp);
        std::string LONG_DEP(TFieldParams fp);
        std::string NAME(TFieldParams fp);
        std::string NO_SMOKE(TFieldParams fp);
        std::string ONE_SEAT_NO(TFieldParams fp);
        std::string PAX_ID(TFieldParams fp);
        std::string PLACE_ARV(TFieldParams fp);
        std::string PLACE_DEP(TFieldParams fp);
        std::string REG_NO(TFieldParams fp);
        std::string SCD(TFieldParams fp);
        std::string SEAT_NO(TFieldParams fp);
        std::string STR_SEAT_NO(TFieldParams fp);
        std::string SURNAME(TFieldParams fp);
        std::string TEST_SERVER(TFieldParams fp);

    public:
        TPrnTagStore(int agrp_id, int apax_id, int apr_lat);
        TPrnTagStore() {};
        void set_tag(std::string name, std::string value);
        std::string get_field(std::string name, int len, std::string align, std::string date_format, std::string tag_lang);

        void tst_get_tag_list(std::vector<std::string> &tag_list);
};

#endif
