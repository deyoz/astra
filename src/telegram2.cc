#define NICKNAME "DEN"
#define NICKTRACE SYSTEM_TRACE
#include "test.h"
#include "setup.h"
#include "logger.h"
#include "telegram.h"
#include "xml_unit.h"
#include "tlg/tlg.h"
#include "astra_utils.h"
#include "stl_utils.h"
#include <map>

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace boost::local_time;
using namespace ASTRA;

const string br = "\xa";
const size_t PART_SIZE = 2000;
const size_t LINE_SIZE = 64;

string TlgElemIdToElem(TElemType type, string id, bool pr_lat)
{
    if(!pr_lat) return id;
    string id1 = ElemIdToElem(type, id, 1);
    if(!is_lat(id1)) {
        string code_name;
        switch(type)
        {
            case etCountry:
                code_name="государства";
                break;
            case etCity:
                code_name="города";
                break;
            case etAirline:
                code_name="а/к";
                break;
            case etAirp:
                code_name="а/п";
                break;
            case etCraft:
                code_name="ВС";
                break;
            case etClass:
                code_name="класса";
                break;
            case etSubcls:
                code_name="подкласса";
                break;
            case etPersType:
                code_name="типа пассажира";
                break;
            case etGenderType:
                code_name="пола пассажира";
                break;
            case etPaxDocType:
                code_name="типа документа";
                break;
            case etPayType:
                code_name="формы оплаты";
                break;
            case etCurrency:
                code_name="валюты";
                break;
            case etSuffix:
                code_name="суффикса";
                break;
            default: 
                throw Exception("Unsupported elem type %d", type);
        };
        throw UserException("Не найден латинский код " + code_name);
    }
    return id1;
}

struct TTlgInfo {
    string tlg_type;
    //адреса получателей
    string addrs;
    //адрес отправителя
    string sender;
    //наш аэропорт
    string own_airp;
    //рейс
    int point_id;
    string airline;
    int flt_no;
    string suffix;
    string airp_dep;
    string airp_arv;
    TDateTime scd;
    TDateTime scd_local;
    TDateTime act_local;
    bool pr_summer;
    string craft;
    string bort;
    //вспомогательные чтобы вытаскивать маршрут
    int first_point;
    int point_num;
    //направление
    string airp;
    //центр бронирования
    string crs;
    //дополнительная инфа
    string extra;
    //разные настройки
    bool pr_lat;
    TTlgInfo() {
        point_id = -1;
        flt_no = -1;
        scd = 0;
        scd_local = 0;
        act_local = 0;
        pr_summer = false;
        first_point = -1;
        point_num = -1;
        pr_lat = false;
    };
};

string fetch_addr(string &addr)
{
    string result;
    // пропускаем все символы не относящиеся к слову
    size_t i = 0;
    size_t len = 0;
    for(i = 0; i < addr.size() && (u_char)addr[i] <= 0x20; i++) ;
    for(len = 0; i + len < addr.size() && (u_char)addr[i + len] > 0x20; len++) ;
    result = addr.substr(i, len);
    if(addr.size() == len)
        addr.erase();
    else
        addr = addr.substr(len + 1);
    if(result.size() > 7)
        throw UserException("Неверно указан SITA-адрес " + result);
    for(i = 0; i < result.size(); i++) {
        // c BETWEEN 'A' AND 'Z' OR c BETWEEN '0' AND '9'
        u_char c = result[i];
        if((c > 0x40 and c < 0x5b) or (c > 0x2f and c < 0x3a))
            continue;
        throw UserException("Неверно указан SITA-адрес " + result);
    }
    return result;
}

string format_addr_line(string vaddrs)
{
    string result, addr_line;
    int n = 0;
    string addr = fetch_addr(vaddrs);
    while(!addr.empty()) {
        if(result.find(addr) == string::npos && addr_line.find(addr) == string::npos) {
            n++;
            if(n > 32)
                throw UserException("Возможно указать не более 32 различных адресов получателей");
            if(addr_line.size() + addr.size() + 1 > 64) {
                result += addr_line + br;
                addr_line = addr;
            } else {
                if(addr_line.empty())
                    addr_line = addr;
                else
                    addr_line += " " + addr;
            }
        }
        addr = fetch_addr(vaddrs);
    }
    if(!addr_line.empty())
        result += addr_line + br;
    return result;
}

namespace PRL {
    struct TPNRItem {
        string airline, addr;
        void ToTlg(TTlgInfo &info, vector<string> &body);
    };

    void TPNRItem::ToTlg(TTlgInfo &info, vector<string> &body)
    {
        body.push_back(".L/" + convert_pnr_addr(addr, info.pr_lat) + ElemIdToElem(etAirline, airline, info.pr_lat));
    }

    struct TPNRList {
        vector<TPNRItem> items;
        void get(int pnr_id);
        void ToTlg(TTlgInfo &info, vector<string> &body);
    };

    void TPNRList::ToTlg(TTlgInfo &info, vector<string> &body)
    {
        for(vector<TPNRItem>::iterator iv = items.begin(); iv != items.end(); iv++)
            iv->ToTlg(info, body);
    }

    void TPNRList::get(int pnr_id)
    {
        if(pnr_id == NoExists) return;
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "SELECT airline,addr FROM pnr_addrs WHERE pnr_id=:pnr_id ORDER BY addr,airline";
        Qry.CreateVariable("pnr_id", otInteger, pnr_id);
        Qry.Execute();
        for(; !Qry.Eof; Qry.Next()) {
            TPNRItem pnr;
            pnr.airline = Qry.FieldAsString("airline");
            pnr.addr = Qry.FieldAsString("addr");
            items.push_back(pnr);
        }
    }

    struct TInfantsItem {
        int grp_id;
        string surname;
        string name;
        int pax_id;
        int crs_pax_id;
        void dump();
        TInfantsItem() {
            grp_id = NoExists;
            pax_id = NoExists;
            crs_pax_id = NoExists;
        }
    };

    void TInfantsItem::dump()
    {
        ProgTrace(TRACE5, "TInfantsItem");
        ProgTrace(TRACE5, "grp_id: %d", grp_id);
        ProgTrace(TRACE5, "surname: %s", surname.c_str());
        ProgTrace(TRACE5, "name: %s", name.c_str());
        ProgTrace(TRACE5, "pax_id: %d", pax_id);
        ProgTrace(TRACE5, "crs_pax_id: %d", crs_pax_id);
        ProgTrace(TRACE5, "--------------------");
    }

    struct TAdultsItem {
        int grp_id;
        int pax_id;
        string surname;
        void dump();
        TAdultsItem() {
            grp_id = NoExists;
            pax_id = NoExists;
        }
    };

    void TAdultsItem::dump()
    {
        ProgTrace(TRACE5, "TAdultsItem");
        ProgTrace(TRACE5, "grp_id: %d", grp_id);
        ProgTrace(TRACE5, "pax_id: %d", pax_id);
        ProgTrace(TRACE5, "surname: %s", surname.c_str());
        ProgTrace(TRACE5, "--------------------");
    }

    struct TInfants {
        vector<TInfantsItem> items;
        void get(int point_id);
    };

    void TInfants::get(int point_id) {
        items.clear();
        if(point_id == NoExists) return;
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "SELECT pax.grp_id, "
            "       pax.surname, "
            "       pax.name, "
            "       NULL AS pax_id, "
            "       crs_inf.pax_id AS crs_pax_id "
            "FROM pax_grp,pax,crs_inf "
            "WHERE "
            "     pax_grp.grp_id=pax.grp_id AND "
            "     pax_grp.point_dep=:point_id AND "
            "     pax.seats=0 AND pax.pr_brd=1 AND "
            "     pax.pax_id=crs_inf.inf_id(+) ";
        Qry.CreateVariable("point_id", otInteger, point_id);
        Qry.Execute();
        if(!Qry.Eof) {
            int col_grp_id = Qry.FieldIndex("grp_id");
            int col_surname = Qry.FieldIndex("surname");
            int col_name = Qry.FieldIndex("name");
            int col_pax_id = Qry.FieldIndex("pax_id");
            int col_crs_pax_id = Qry.FieldIndex("crs_pax_id");
            for(; !Qry.Eof; Qry.Next()) {
                TInfantsItem item;
                item.grp_id = Qry.FieldAsInteger(col_grp_id);
                item.surname = Qry.FieldAsString(col_surname);
                item.name = Qry.FieldAsString(col_name);
                item.pax_id = Qry.FieldAsInteger(col_pax_id);
                if(!Qry.FieldIsNULL(col_crs_pax_id))
                    item.crs_pax_id = Qry.FieldAsInteger(col_crs_pax_id);
                item.dump();
                items.push_back(item);
            }
        }
        if(!items.empty()) {
            Qry.Clear();
            Qry.SQLText =
                "SELECT pax.grp_id, "
                "       pax.pax_id, "
                "       pax.surname "
                "FROM pax_grp,pax "
                "WHERE "
                "     pax_grp.grp_id=pax.grp_id AND "
                "     pax_grp.point_dep=:point_id AND "
                "     pax.pers_type='ВЗ' AND pax.pr_brd=1 ";
            Qry.CreateVariable("point_id", otInteger, point_id);
            Qry.Execute();
            vector<TAdultsItem> adults;
            if(!Qry.Eof) {
                int col_grp_id = Qry.FieldIndex("grp_id");
                int col_pax_id = Qry.FieldIndex("pax_id");
                int col_surname = Qry.FieldIndex("surname");
                for(; !Qry.Eof; Qry.Next()) {
                    TAdultsItem item;
                    item.grp_id = Qry.FieldAsInteger(col_grp_id);
                    item.pax_id = Qry.FieldAsInteger(col_pax_id);
                    item.surname = Qry.FieldAsString(col_surname);
                    item.dump();
                    adults.push_back(item);
                }
            }
            for(int k = 1; k <= 3; k++) {
                for(vector<TInfantsItem>::iterator infRow = items.begin(); infRow != items.end(); infRow++) {
                    if(k == 1 and infRow->crs_pax_id != NoExists or
                            k > 1 and infRow->crs_pax_id == NoExists) {
                        for(vector<TAdultsItem>::iterator adultRow = adults.begin(); adultRow != adults.end(); adultRow++) {
                            if(
                                    (infRow->grp_id == adultRow->grp_id) and
                                    (k == 1 and infRow->crs_pax_id == adultRow->pax_id or
                                     k == 2 and infRow->surname == adultRow->surname or
                                     k == 3)
                              ) {
                                infRow->pax_id = adultRow->pax_id;
                                adults.erase(adultRow);
                                break;
                            }
                        }
                    }
                }
            }
            ProgTrace(TRACE5, "TInfantsItems AFTER");
            for(vector<TInfantsItem>::iterator infRow = items.begin(); infRow != items.end(); infRow++) {
                infRow->dump();
            }
        }
    }

    TInfants infants;

    struct TRemItem {
        string rem, cls, crs_cls, crs_subcls;
        void dump();
        string ToTlg(TTlgInfo &info);
    };

    void TRemItem::dump()
    {
        ProgTrace(TRACE5, "TRemItem");
        ProgTrace(TRACE5, "rem: %s", rem.c_str());
        ProgTrace(TRACE5, "cls: %s", cls.c_str());
        ProgTrace(TRACE5, "crs_cls: %s", crs_cls.c_str());
        ProgTrace(TRACE5, "crs_subcls: %s", crs_subcls.c_str());
        ProgTrace(TRACE5, "----------");
    }

    string TRemItem::ToTlg(TTlgInfo &info)
    {
        if(!crs_cls.empty() && cls != crs_cls) {
            if(
                    rem.substr(0, 4) == "FQTR" ||
                    rem.substr(0, 4) == "FQTU"
              ) {
                rem = "FQTV";
                if(rem.size() > 4)
                    rem += rem.substr(4);
            }
            size_t i = rem.rfind('/');
            if(i != string::npos)
                rem = rem.substr(0, i);
            rem = transliter(upperc(rem), info.pr_lat) + '/' + ElemIdToElem(etSubcls, crs_subcls, info.pr_lat);
        } else {
            rem = transliter(upperc(rem), info.pr_lat);
        }
        return rem;
    }

    struct TPRLPax;
    struct TRemList {
        vector<TRemItem> items;
        void get(TTlgInfo &info, TPRLPax &pax);
        void ToTlg(TTlgInfo &info, vector<string> &body);
    };

    struct TTagItem {
        string tag_type;
        int no_len;
        double no;
        string color;
        string airp_arv;
        TTagItem() {
            no_len = NoExists;
            no = NoExists;
        }
    };

    struct TTagList {
        vector<TTagItem> items;
        void get(int grp_id);
        void ToTlg(TTlgInfo &info, vector<string> &body);
    };

    void TTagList::ToTlg(TTlgInfo &info, vector<string> &body)
    {
        if(items.empty())
            return;
        int num = 0;
        vector<TTagItem>::iterator prev_item;
        vector<TTagItem>::iterator iv = items.begin();
        while(true) {
            if(
                    iv == items.end() or
                    iv != items.begin() and
                    not(prev_item->tag_type == iv->tag_type and
                        prev_item->color == iv->color and
                        prev_item->no + 1 == iv->no and
                        num < 999)
              ) {
                ostringstream line;
                line
                    << ".N/" << fixed << setprecision(0) << setw(10) << setfill('0') << (prev_item->no - num + 1)
                    << setw(3) << setfill('0') << num
                    << '/' << ElemIdToElem(etAirp, prev_item->airp_arv, info.pr_lat);
                body.push_back(line.str());
                if(iv == items.end())
                        break;
                num = 1;
            } else
              num++;
            prev_item = iv++;
        }
    }

    void TTagList::get(int grp_id)
    {
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "SELECT "
            "  bag_tags.tag_type, "
            "  tag_types.no_len, "
            "  bag_tags.no, "
            "  bag_tags.color, "
            "  nvl(transfer.airp_arv, pax_grp.airp_arv) airp_arv "
            "FROM "
            "  bag_tags, "
            "  tag_types, "
            "  pax_grp, "
            "  transfer "
            "WHERE "
            "  bag_tags.tag_type=tag_types.code AND "
            "  bag_tags.grp_id=:grp_id and "
            "  bag_tags.grp_id = transfer.grp_id(+) and transfer.pr_final(+)<>0 and "
            "  bag_tags.grp_id = pax_grp.grp_id "
            "ORDER BY "
            "  bag_tags.tag_type, "
            "  bag_tags.color, "
            "  bag_tags.no ";
        Qry.CreateVariable("grp_id", otInteger, grp_id);
        Qry.Execute();
        if(!Qry.Eof) {
            int col_tag_type = Qry.FieldIndex("tag_type");
            int col_no_len = Qry.FieldIndex("no_len");
            int col_no = Qry.FieldIndex("no");
            int col_color = Qry.FieldIndex("color");
            int col_airp_arv = Qry.FieldIndex("airp_arv");
            for(; !Qry.Eof; Qry.Next()) {
                TTagItem item;
                item.tag_type = Qry.FieldAsString(col_tag_type);
                item.no_len = Qry.FieldAsInteger(col_no_len);
                item.no = Qry.FieldAsFloat(col_no);
                item.color = Qry.FieldAsString(col_color);
                item.airp_arv = Qry.FieldAsString(col_airp_arv);
                items.push_back(item);
            }
        }
    }

    struct TOnwardItem {
        string airline;
        int flt_no;
        string suffix;
        TDateTime scd;
        string airp_arv;
        string subclass;
        TOnwardItem() {
            flt_no = NoExists;
            scd = NoExists;
        }
    };

    struct TOnwardList {
        vector<TOnwardItem> items;
        void get(int grp_id);
        void ToTlg(TTlgInfo &info, vector<string> &body);
    };

    void TOnwardList::get(int grp_id)
    {
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "SELECT "
            "   airline, "
            "   flt_no, "
            "   suffix, "
            "   scd, "
            "   airp_arv, "
            "   subclass "
            "FROM transfer,trfer_trips  "
            "WHERE transfer.point_id_trfer=trfer_trips.point_id AND "
            "      grp_id=:grp_id AND transfer_num>=1 "
            "ORDER BY transfer_num ";
        Qry.CreateVariable("grp_id", otInteger, grp_id);
        Qry.Execute();
        if(!Qry.Eof) {
            int col_airline = Qry.FieldIndex("airline");
            int col_flt_no = Qry.FieldIndex("flt_no");
            int col_suffix = Qry.FieldIndex("suffix");
            int col_scd = Qry.FieldIndex("scd");
            int col_airp_arv = Qry.FieldIndex("airp_arv");
            int col_subclass = Qry.FieldIndex("subclass");
            for(; !Qry.Eof; Qry.Next()) {
                TOnwardItem item;
                item.airline = Qry.FieldAsString(col_airline);
                item.flt_no = Qry.FieldAsInteger(col_flt_no);
                item.suffix = Qry.FieldAsString(col_suffix);
                item.scd = Qry.FieldAsDateTime(col_scd);
                item.airp_arv = Qry.FieldAsString(col_airp_arv);
                item.subclass = Qry.FieldAsString(col_subclass);
                items.push_back(item);
            }
        }
    }

    void TOnwardList::ToTlg(TTlgInfo &info, vector<string> &body)
    {
        int i = 1;
        for(vector<TOnwardItem>::iterator iv = items.begin(); iv != items.end(); iv++) {
            ostringstream line;
            line << ".O";
            if(i > 1)
                line << i;
            i++;
            line
                << '/'
                << ElemIdToElem(etAirline, iv->airline, info.pr_lat)
                << setw(3) << setfill('0') << iv->flt_no
                << ElemIdToElem(etSuffix, iv->suffix, info.pr_lat)
                << ElemIdToElem(etSubcls, iv->subclass, info.pr_lat)
                << DateTimeToStr(iv->scd, "dd", info.pr_lat)
                << ElemIdToElem(etAirp, iv->airp_arv, info.pr_lat);
            body.push_back(line.str());
        }
    }

    struct TGRPItem {
        int pax_count;
        int bagAmount;
        int bagWeight;
        int rkWeight;
        bool written;
        int bg;
        TTagList tags;
        TOnwardList onwards;
        TGRPItem() {
            pax_count = NoExists;
            bagAmount = NoExists;
            bagWeight = NoExists;
            rkWeight = NoExists;
            written = false;
            bg = NoExists;
        }
    };

    struct TGRPMap {
        map<int, TGRPItem> items;
        void get(int grp_id);
        void ToTlg(TTlgInfo &info, TPRLPax &pax, vector<string> &body);
    };
    TGRPMap grp_map;

    struct TPRLPax {
        string target;
        int cls_grp_id;
        string surname;
        string name;
        int pnr_id;
        int pax_id;
        int grp_id;
        TPNRList pnrs;
        TRemList rems;
        TPRLPax() {
            cls_grp_id = NoExists;
            pnr_id = NoExists;
            pax_id = NoExists;
            grp_id = NoExists;
        }
    };

    void TGRPMap::ToTlg(TTlgInfo &info, TPRLPax &pax, vector<string> &body)
    {
        TGRPItem &grp_map = items[pax.grp_id];
        if(not(grp_map.bagAmount == 0 and grp_map.bagWeight == 0 and grp_map.rkWeight == 0)) {
            ostringstream line;
            if(!grp_map.written) {
                line.str("");
                grp_map.written = true;
                line << ".W/K/" << grp_map.bagAmount << '/' << grp_map.bagWeight;
                if(grp_map.rkWeight != 0)
                    line << '/' << grp_map.rkWeight;
                body.push_back(line.str());
                grp_map.tags.ToTlg(info, body);
            }
            if(grp_map.pax_count > 1) {
                line.str("");
                line << ".BG/" << setw(3) << setfill('0') << grp_map.bg;
                body.push_back(line.str());
            }
        }
        grp_map.onwards.ToTlg(info, body);
    }

    void TGRPMap::get(int grp_id)
    {
        if(items.find(grp_id) != items.end()) return; // olready got
        TQuery Qry(&OraSession);
        Qry.SQLText =
          "SELECT "
          "  NVL(ckin.get_bagAmount(:grp_id,NULL),0), "
          "  NVL(ckin.get_bagWeight(:grp_id,NULL),0), "
          "  NVL(ckin.get_rkWeight(:grp_id,NULL),0) "
          "FROM dual ";
        Qry.CreateVariable("grp_id", otInteger, grp_id);
        Qry.Execute();
        TGRPItem item;
        item.bagAmount = Qry.FieldAsInteger(0);
        item.bagWeight = Qry.FieldAsInteger(1);
        item.rkWeight = Qry.FieldAsInteger(2);
        Qry.SQLText =
            "select count(*) from pax where grp_id = :grp_id and refuse is null";
        Qry.Execute();
        item.pax_count = Qry.FieldAsInteger(0);
        item.tags.get(grp_id);
        item.onwards.get(grp_id);
        item.bg = items.size() + 1;
        ProgTrace(TRACE5, "item.bg: %d", items.size());
        ProgTrace(TRACE5, "TGRPMap::get: grp_id %d", grp_id);
        items[grp_id] = item;
    }

    void TRemList::ToTlg(TTlgInfo &info, vector<string> &body)
    {
        string rem_code;
        for(vector<TRemItem>::iterator iv = items.begin(); iv != items.end(); iv++) {
            iv->dump();
            string rem = iv->ToTlg(info);
            if(rem_code != rem.substr(0, 4)) {
                rem_code = rem.substr(0, 4);
                body.push_back(".R/" + rem);
            }
        }
    }

    void TRemList::get(TTlgInfo &info, TPRLPax &pax)
    {
        items.clear();
        if(pax.pax_id == NoExists) return;
        // rems must be push_backed exactly in this order. Don't swap!
        ProgTrace(TRACE5, "in TRemList::get: pax_id %d; grp_id %d", pax.pax_id, pax.grp_id);
        for(vector<TInfantsItem>::iterator infRow = infants.items.begin(); infRow != infants.items.end(); infRow++) {
            infRow->dump();
            if(infRow->grp_id == pax.grp_id and infRow->pax_id == pax.pax_id) {
                TRemItem rem;
                rem.rem = "1INF " + infRow->surname;
                if(!infRow->name.empty()) {
                    rem.rem += "/" + infRow->name;
                }
                items.push_back(rem);
            }
        }
        TQuery Qry(&OraSession);
        Qry.CreateVariable("pax_id", otInteger, pax.pax_id);
        Qry.SQLText = "select * from pax where pax.pax_id = :pax_id and pax.pers_type in ('РБ', 'РМ') and pax.seats>0 ";
        Qry.Execute();
        if(!Qry.Eof) {
            TRemItem rem;
            rem.rem = "1CHD";
            items.push_back(rem);
        }
        Qry.SQLText =
            "select seat_no from pax where pax_id = :pax_id and pax.seat_no is not null ";
        Qry.Execute();
        if(!Qry.Eof) {
            TRemItem rem;
            rem.rem = "SEAT " + convert_seat_no(Qry.FieldAsString(0), info.pr_lat);
            items.push_back(rem);
        }
        Qry.SQLText =
            "select "
            "    pax_rem.rem, "
            "    pax_grp.class, "
            "    crs_pnr.class crs_class, "
            "    crs_pnr.subclass crs_subclass, "
            "    2 ord "
            "from "
            "    pax, "
            "    pax_grp, "
            "    crs_pax, "
            "    crs_pnr, "
            "    pax_rem "
            "where "
            "    pax.pax_id = :pax_id and "
            "    pax.grp_id = pax_grp.grp_id and "
            "    pax.pax_id = pax_rem.pax_id and "
            "    pax_rem.rem_code not in (/*'PSPT',*/ 'OTHS', 'DOCS', 'CHD', 'CHLD', 'INF', 'INFT') and "
            "/*        pax_rem.rem_code not like 'TKN%' and */ "
            "    pax.pax_id=crs_pax.pax_id(+) AND "
            "    crs_pax.pnr_id=crs_pnr.pnr_id(+) ";
        Qry.Execute();
        if(!Qry.Eof) {
            int col_rem = Qry.FieldIndex("rem");
            int col_class = Qry.FieldIndex("class");
            int col_crs_class = Qry.FieldIndex("crs_class");
            int col_crs_subclass = Qry.FieldIndex("crs_subclass");
            for(; !Qry.Eof; Qry.Next()) {
                TRemItem rem;
                rem.rem = Qry.FieldAsString(col_rem);
                rem.cls = Qry.FieldAsString(col_class);
                rem.crs_cls = Qry.FieldAsString(col_crs_class);
                rem.crs_subcls = Qry.FieldAsString(col_crs_subclass);
                items.push_back(rem);
            }
        }
    }

    struct TPRLDest {
        int point_num;
        string airp;
        string cls;
        vector<TPRLPax> PaxList;
        TPRLDest() {
            point_num = NoExists;
        }
        void GetPaxList(TTlgInfo &info);
        void PaxListToTlg(TTlgInfo &info, vector<string> &body);
    };

    void TPRLDest::PaxListToTlg(TTlgInfo &info, vector<string> &body)
    {
        for(vector<TPRLPax>::iterator iv = PaxList.begin(); iv != PaxList.end(); iv++) {
            string line = "1" + transliter(iv->surname, info.pr_lat).substr(0, 63);
            if(!iv->name.empty()) {
                string name = transliter(iv->name, info.pr_lat);
                if(iv->name.size() + line.size() + 1 <= LINE_SIZE)
                    line += "/" + name;
            }
            body.push_back(line);
            iv->pnrs.ToTlg(info, body);
            ProgTrace(TRACE5, "before iv->rems.ToTlg: name %s; surname %s", iv->name.c_str(), iv->surname.c_str());
            iv->rems.ToTlg(info, body);
            grp_map.ToTlg(info, *iv, body);
        }
    }

    void TPRLDest::GetPaxList(TTlgInfo &info)
    {
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "select "
            "    pax_grp.airp_arv target, "
            "    cls_grp.id cls, "
            "    pax.surname, "
            "    pax.name, "
            "    crs_pnr.pnr_id, "
            "    pax.pax_id, "
            "    pax.grp_id "
            "from "
            "    pax, "
            "    pax_grp, "
            "    cls_grp, "
            "    crs_pax, "
            "    crs_pnr "
            "WHERE "
            "    pax_grp.point_dep = :point_id and "
            "    pax_grp.airp_arv = :airp and "
            "    pax_grp.grp_id=pax.grp_id AND "
            "    pax_grp.class_grp = cls_grp.id(+) AND "
            "    cls_grp.code = :class and "
            "    pax.pr_brd = 1 and "
            "    pax.seats>0 and "
            "    pax.pax_id = crs_pax.pax_id(+) and "
            "    crs_pax.pnr_id = crs_pnr.pnr_id(+) "
            "order by "
            "    pax.surname ";
        Qry.CreateVariable("point_id", otInteger, info.point_id);
        Qry.CreateVariable("airp", otString, airp);
        Qry.CreateVariable("class", otString, cls);
        Qry.Execute();
        if(!Qry.Eof) {
            int col_target = Qry.FieldIndex("target");
            int col_cls = Qry.FieldIndex("cls");
            int col_surname = Qry.FieldIndex("surname");
            int col_name = Qry.FieldIndex("name");
            int col_pnr_id = Qry.FieldIndex("pnr_id");
            int col_pax_id = Qry.FieldIndex("pax_id");
            int col_grp_id = Qry.FieldIndex("grp_id");
            for(; !Qry.Eof; Qry.Next()) {
                TPRLPax pax;
                pax.target = Qry.FieldAsString(col_target);
                if(!Qry.FieldIsNULL(col_cls))
                    pax.cls_grp_id = Qry.FieldAsInteger(col_cls);
                pax.surname = Qry.FieldAsString(col_surname);
                pax.name = Qry.FieldAsString(col_name);
                if(!Qry.FieldIsNULL(col_pnr_id))
                    pax.pnr_id = Qry.FieldAsInteger(col_pnr_id);
                pax.pax_id = Qry.FieldAsInteger(col_pax_id);
                pax.grp_id = Qry.FieldAsInteger(col_grp_id);
                pax.pnrs.get(pax.pnr_id);
                pax.rems.get(info, pax);
                grp_map.get(pax.grp_id);
                PaxList.push_back(pax);
            }
        }
    }
}

using namespace PRL;

int RPL(TTlgInfo &info)
{
    TTlgOutPartInfo tlg_row;
    tlg_row.num = 1;
    tlg_row.tlg_type = info.tlg_type;
    tlg_row.point_id = info.point_id;
    tlg_row.pr_lat = info.pr_lat;
    tlg_row.addr = info.addrs;
    tlg_row.time_create = NowUTC();
    ostringstream heading;
    heading
        << "." << info.sender << " " << DateTimeToStr(tlg_row.time_create, "ddhhnn") << br
        << "PRL" << br
        << info.airline << setw(3) << setfill('0') << info.flt_no << info.suffix << "/"
        << DateTimeToStr(info.scd_local, "ddmmm", 1) << " " << info.airp_dep << " ";
    tlg_row.heading = heading.str() + "PART" + IntToString(tlg_row.num) + br;
    tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + br;
    size_t part_len = tlg_row.addr.size() + tlg_row.heading.size() + tlg_row.ending.size();
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   point_num, "
        "   DECODE(pr_tranzit,0,point_id,first_point) first_point "
        "from "
        "   points "
        "where "
        "   point_id = :point_id";
    Qry.CreateVariable("point_id", otInteger, info.point_id);
    Qry.Execute();
    if(Qry.Eof)
        throw UserException("Рейс не найден");
    int vpoint_num = Qry.FieldAsInteger("point_num");
    int vfirst_point = Qry.FieldAsInteger("first_point");
    infants.get(vfirst_point);
    grp_map.items.clear();
    Qry.Clear();
    Qry.SQLText =
        "select point_num, airp, class from "
        "( "
        "  SELECT airp, point_num, point_id FROM points "
        "  WHERE first_point = :vfirst_point AND point_num > :vpoint_num AND pr_del=0 "
        ") a, "
        "( "
        "  SELECT DISTINCT cls_grp.code AS class "
        "  FROM pax_grp,cls_grp "
        "  WHERE pax_grp.class_grp=cls_grp.id AND "
        "        pax_grp.point_dep = :vpoint_id AND pax_grp.bag_refuse=0 "
        "  UNION "
        "  SELECT class FROM trip_classes WHERE point_id = :vpoint_id "
        ") b  "
        "ORDER by "
        "  point_num, airp, class ";
    Qry.CreateVariable("vpoint_id", otInteger, info.point_id);
    Qry.CreateVariable("vfirst_point", otInteger, vfirst_point);
    Qry.CreateVariable("vpoint_num", otInteger, vpoint_num);
    Qry.Execute();
    vector<TPRLDest> dests;
    for(; !Qry.Eof; Qry.Next()) {
        TPRLDest dest;
        dest.point_num = Qry.FieldAsInteger("point_num");
        dest.airp = Qry.FieldAsString("airp");
        dest.cls = Qry.FieldAsString("class");
        dest.GetPaxList(info);
        dests.push_back(dest);
    }
    vector<string> body;
    ostringstream line;
    bool pr_empty = true;
    for(vector<TPRLDest>::iterator iv = dests.begin(); iv != dests.end(); iv++) {
        if(iv->PaxList.empty()) {
            line.str("");
            line
                << "-" << TlgElemIdToElem(etAirp, iv->airp, info.pr_lat)
                << "00" << TlgElemIdToElem(etClass, iv->cls, info.pr_lat);
            body.push_back(line.str());
        } else {
            pr_empty = false;
            line.str("");
            line
                << "-" << TlgElemIdToElem(etAirp, iv->airp, info.pr_lat)
                << setw(2) << setfill('0') << iv->PaxList.size()
                << TlgElemIdToElem(etClass, iv->cls, info.pr_lat);
            body.push_back(line.str());
            iv->PaxListToTlg(info, body);
        }
    }

    if(pr_empty) {
        body.clear();
        body.push_back("NIL");
    }
    string part_begin;
    for(vector<string>::iterator iv = body.begin(); iv != body.end(); iv++) {
        if(iv->find('-') == 0)
            part_begin = *iv;
        int pax_len = 0;
        if(iv->find('1') == 0) {
            for(vector<string>::iterator j = iv; j != body.end() and j->find('1') != 0; j++) {
                pax_len += j->size() + br.size();
            }
        } else
            pax_len = iv->size() + br.size();
        if(part_len + pax_len <= PART_SIZE)
            pax_len = iv->size() + br.size();
        part_len += pax_len;
        if(part_len > PART_SIZE) {
            TelegramInterface::SaveTlgOutPart(tlg_row);
            tlg_row.heading = heading.str() + "PART" + IntToString(tlg_row.num) + br;
            tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + br;
            if(iv->find('-') == 0)
                tlg_row.body = *iv + br;
            else
                tlg_row.body = part_begin + br + *iv + br;
            part_len = tlg_row.addr.size() + tlg_row.heading.size() +
                tlg_row.body.size() + tlg_row.ending.size();
        } else
            tlg_row.body += *iv + br;
    }
    tlg_row.ending = "ENDPRL" + br;
    TelegramInterface::SaveTlgOutPart(tlg_row);
    return tlg_row.id;
}

int Unknown(TTlgInfo &info, bool &vcompleted)
{
    TTlgOutPartInfo tlg_row;
    vcompleted = false;
    tlg_row.num = 1;
    tlg_row.tlg_type = info.tlg_type;
    tlg_row.point_id = info.point_id;
    tlg_row.pr_lat = info.pr_lat;
    tlg_row.addr = info.addrs;
    tlg_row.time_create = NowUTC();
    tlg_row.heading = '.' + info.sender + ' ' + DateTimeToStr(tlg_row.time_create,"ddhhnn") + br;
    tlg_row.extra = info.extra;
    TelegramInterface::SaveTlgOutPart(tlg_row);
    return tlg_row.id;
}

int create_tlg(
        const string      vtype,
        const int         vpoint_id,
        const string      vairp_trfer,
        const string      vcrs,
        const string      vextra,
        const bool        vpr_lat,
        const string      vaddrs
        )
{
    if(vtype.empty())
        throw UserException("Не указан тип телеграммы");
    TQuery Qry(&OraSession);
    Qry.SQLText = "select basic_type, editable from typeb_types where code = :vtype";
    Qry.CreateVariable("vtype", otString, vtype);
    Qry.Execute();
    if(Qry.Eof)
        throw UserException("Неверно указан тип телеграммы");
    string vbasic_type = Qry.FieldAsString("basic_type");
    bool veditable = Qry.FieldAsInteger("editable") != 0;
    TTlgInfo info;
    info.tlg_type = vtype;
    if((vbasic_type == "PTM" || vbasic_type == "BTM") && vairp_trfer.empty())
        throw UserException("Не указан аэропорт назначения");
    if(vbasic_type == "PFS" && vcrs.empty())
        throw UserException("Не указан центр бронирования");
    info.point_id = vpoint_id;
    info.pr_lat = vpr_lat;
    string vsender = OWN_SITA_ADDR();
    if(vsender.empty())
        throw UserException("Не указан адрес отправителя");
    if(vsender.size() != 7 || !is_lat(vsender))
        throw UserException("Неверно указан адрес отправителя");
    info.sender = vsender;
    if(vpoint_id != -1) {
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "SELECT "
            "   airline, "
            "   flt_no, "
            "   suffix, "
            "   scd_out scd, "
            "   act_out, "
            "   craft, "
            "   bort, "
            "   airp, "
            "   point_num, "
            "   DECODE(pr_tranzit,0,point_id,first_point) first_point "
            "from "
            "   points "
            "where "
            "   point_id = :vpoint_id ";
        Qry.CreateVariable("vpoint_id", otInteger, vpoint_id);
        Qry.Execute();
        if(Qry.Eof)
            throw UserException("Рейс не найден");
        info.airline = Qry.FieldAsString("airline");
        info.flt_no = Qry.FieldAsInteger("flt_no");
        info.suffix = Qry.FieldAsString("suffix");
        info.scd = Qry.FieldAsDateTime("scd");
        info.craft = Qry.FieldAsString("craft");
        info.bort = Qry.FieldAsString("bort");
        info.own_airp = Qry.FieldAsString("airp");
        info.point_num = Qry.FieldAsInteger("point_num");
        info.first_point = Qry.FieldAsInteger("first_point");
        info.airline = TlgElemIdToElem(etAirline, info.airline, info.pr_lat);
        info.suffix = TlgElemIdToElem(etSuffix, info.suffix, info.pr_lat);
        info.airp_dep = TlgElemIdToElem(etAirp, info.own_airp, info.pr_lat);

        string tz_region=AirpTZRegion(info.own_airp);
        info.scd_local = UTCToLocal( info.scd, tz_region );
        info.act_local = UTCToLocal( Qry.FieldAsDateTime("act_out"), tz_region );
        //вычисляем признак летней/зимней навигации
        tz_database &tz_db = get_tz_database();
        time_zone_ptr tz = tz_db.time_zone_from_region( tz_region );
        if (tz==NULL) throw Exception("Region '%s' not found",tz_region.c_str());
        if (tz->has_dst())
        {
            local_date_time ld(DateTimeToBoost(info.scd),tz);
            info.pr_summer=ld.is_dst();
        };
    }
    if(vbasic_type == "PTM" || vbasic_type == "BTM") {
        info.airp = vairp_trfer;
        info.airp_arv = TlgElemIdToElem(etAirp, vairp_trfer, info.pr_lat);
    }
    if(vbasic_type == "PFS") {
        info.crs = vcrs;
    }
    if(vbasic_type == "???") {
        info.extra = vextra;
    }
    info.addrs = format_addr_line(vaddrs);
    if(info.addrs.empty())
        throw UserException("Не указаны адреса получателей телеграммы");
    bool vcompleted = !veditable;
    int vid = NoExists;

    if(vbasic_type == "PRL") vid = RPL(info);
    else vid = Unknown(info, vcompleted);

    Qry.Clear();
    Qry.SQLText = "update tlg_out set completed = :vcompleted where id = :vid";
    Qry.CreateVariable("vcompleted", otInteger, vcompleted);
    Qry.CreateVariable("vid", otInteger, vid);
    Qry.Execute();

    return vid;
}

void TelegramInterface::CreateTlg2(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery Qry(&OraSession);
    int point_id = NodeAsInteger( "point_id", reqNode );
    xmlNodePtr node=reqNode->children;
    string tlg_type=NodeAsStringFast( "tlg_type", node);
    //!!!потом удалить (17.03.08)
    if (GetNodeFast("pr_numeric",node)!=NULL)
    {
        if (NodeAsIntegerFast("pr_numeric",node)!=0)
        {
            if (tlg_type=="PFS") tlg_type="PFSN";
            if (tlg_type=="PTM") tlg_type="PTMN";
        };
    };
    if (tlg_type=="MVT") tlg_type="MVTA";
    //!!!потом удалить (17.03.08)
    string airp_trfer = NodeAsStringFast( "airp_arv", node, "");
    string crs = NodeAsStringFast( "crs", node, "");
    string extra = NodeAsStringFast( "extra", node, "");
    bool pr_lat = NodeAsIntegerFast( "pr_lat", node)!=0;
    string addrs = NodeAsStringFast( "addrs", node);
    Qry.Clear();
    Qry.SQLText="SELECT short_name FROM typeb_types WHERE code=:tlg_type";
    Qry.CreateVariable("tlg_type",otString,tlg_type);
    Qry.Execute();
    if (Qry.Eof) throw Exception("CreateTlg: Unknown telegram type %s",tlg_type.c_str());
    string short_name=Qry.FieldAsString("short_name");

    int tlg_id = NoExists;
    try {
        tlg_id = create_tlg(
                tlg_type,
                point_id,
                airp_trfer,
                crs,
                extra,
                pr_lat,
                addrs
                );
    } catch(UserException E) {
        throw UserException( "Ошибка формирования. %s", E.what());
    }

    if (tlg_id == NoExists) throw Exception("create_tlg without result");
    ostringstream msg;
    msg << "Телеграмма " << short_name
        << " (ид=" << tlg_id << ") сформирована: ";
    msg << "адреса: " << addrs << ", ";
    if (!airp_trfer.empty())
        msg << "а/п: " << airp_trfer << ", ";
    if (!crs.empty())
        msg << "центр: " << crs << ", ";
    if (!extra.empty())
        msg << "доп.: " << extra << ", ";
    msg << "лат.: " << (pr_lat ? "да" : "нет");
    if (point_id==-1) point_id=0;
    TReqInfo::Instance()->MsgToLog(msg.str(),evtTlg,point_id,tlg_id);
    NewTextChild( resNode, "tlg_id", tlg_id);
};
