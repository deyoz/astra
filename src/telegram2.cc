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
int TST_TLG_ID; // for test purposes

void TelegramInterface::delete_tst_tlg(int tlg_id)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "DELETE FROM tst_tlg_out WHERE id=:id AND time_send_act IS NULL ";
    Qry.CreateVariable( "id", otInteger, tlg_id);
    Qry.Execute();
    if(Qry.RowsProcessed() == 0)
        ProgTrace(TRACE5, "delete_tst_tlg: not found tlg_id %d", tlg_id);
}

void SaveTlgOutPartTST( TTlgOutPartInfo &info )
{
  TQuery Qry(&OraSession);

  if (info.id<0)
      throw Exception("SaveTlgOutPartTST: tlg_id undefined");


  Qry.Clear();
  Qry.SQLText=
    "INSERT INTO tst_tlg_out(id,num,type,point_id,addr,heading,body,ending,extra, "
    "                    pr_lat,completed,time_create,time_send_scd,time_send_act) "
    "VALUES(:id,:num,:type,:point_id,:addr,:heading,:body,:ending,:extra, "
    "       :pr_lat,0,NVL(:time_create,system.UTCSYSDATE),:time_send_scd,NULL)";
  Qry.CreateVariable("id",otInteger,info.id);
  Qry.CreateVariable("num",otInteger,info.num);
  Qry.CreateVariable("type",otString,info.tlg_type);
  Qry.CreateVariable("point_id",otInteger,info.point_id);
  Qry.CreateVariable("addr",otString,info.addr);
  Qry.CreateVariable("heading",otString,info.heading);
  Qry.CreateVariable("body",otString,info.body);
  Qry.CreateVariable("ending",otString,info.ending);
  Qry.CreateVariable("extra",otString,info.extra);
  Qry.CreateVariable("pr_lat",otInteger,(int)info.pr_lat);
  if (info.time_create!=NoExists)
    Qry.CreateVariable("time_create",otDate,info.time_create);
  else
    Qry.CreateVariable("time_create",otDate,FNull);
  if (info.time_send_scd!=NoExists)
    Qry.CreateVariable("time_send_scd",otDate,info.time_send_scd);
  else
    Qry.CreateVariable("time_send_scd",otDate,FNull);
  Qry.Execute();

  info.num++;
};

string TlgElemIdToElem(TElemType type, int id, bool pr_lat)
{
    string result = ElemIdToElem(type, id, pr_lat);
    if(pr_lat && !is_lat(result)) {
        string code_name;
        switch(type)
        {
            case etClsGrp:
                code_name = "класса";
                break;
            default:
                throw Exception("Unsupported int elem type %d", type);
        throw UserException("Не найден латинский код " + code_name + " '" + result + "'");
        }
    }
    return result;
}

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
        throw UserException("Не найден латинский код " + code_name + " '" + id + "'");
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

namespace PRL_SPACE {
    struct TPNRItem {
        string airline, addr;
        void ToTlg(TTlgInfo &info, vector<string> &body);
    };

    void TPNRItem::ToTlg(TTlgInfo &info, vector<string> &body)
    {
        body.push_back(".L/" + convert_pnr_addr(addr, info.pr_lat) + '/' + ElemIdToElem(etAirline, airline, info.pr_lat));
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
            if(grp_map.pax_count > 1) {
                line.str("");
                line << ".BG/" << setw(3) << setfill('0') << grp_map.bg;
                body.push_back(line.str());
            }
            if(!grp_map.written) {
                line.str("");
                grp_map.written = true;
                line << ".W/K/" << grp_map.bagAmount << '/' << grp_map.bagWeight;
                if(grp_map.rkWeight != 0)
                    line << '/' << grp_map.rkWeight;
                body.push_back(line.str());
                grp_map.tags.ToTlg(info, body);
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
        for(vector<TInfantsItem>::iterator infRow = infants.items.begin(); infRow != infants.items.end(); infRow++) {
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
            "    target, "
            "    cls, "
            "    pax.surname, "
            "    pax.name nulls first, "
            "    pax.pax_id ";
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

    struct TCOMStatsItem {
        string target;
        int f;
        int c;
        int y;
        int adult;
        int child;
        int baby;
        int f_child;
        int f_baby;
        int c_child;
        int c_baby;
        int y_child;
        int y_baby;
        int bag_amount;
        int bag_weight;
        int rk_weight;
        int f_bag_weight;
        int f_rk_weight;
        int c_bag_weight;
        int c_rk_weight;
        int y_bag_weight;
        int y_rk_weight;
        int f_add_pax;
        int c_add_pax;
        int y_add_pax;
        TCOMStatsItem()
        {
            f = 0;
            c = 0;
            y = 0;
            adult = 0;
            child = 0;
            baby = 0;
            f_child = 0;
            f_baby = 0;
            c_child = 0;
            c_baby = 0;
            y_child = 0;
            y_baby = 0;
            bag_amount = 0;
            bag_weight = 0;
            rk_weight = 0;
            f_bag_weight = 0;
            f_rk_weight = 0;
            c_bag_weight = 0;
            c_rk_weight = 0;
            y_bag_weight = 0;
            y_rk_weight = 0;
            f_add_pax = 0;
            c_add_pax = 0;
            y_add_pax = 0;
        }
    };

    struct TTotalPaxWeight {
        int weight;
        void get(TTlgInfo &info);
        TTotalPaxWeight() {
            weight = 0;
        }
    };

    void TTotalPaxWeight::get(TTlgInfo &info)
    {
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "SELECT "
            "      NVL(SUM(DECODE(:pr_summer, 0, pers_types.weight_win, pers_types.weight_sum)),0) "
            "FROM "
            "      pax_grp, "
            "      pax, "
            "      pers_types "
            "WHERE "
            "      pax_grp.grp_id = pax.grp_id AND "
            "      pax.pers_type = pers_types.code AND "
            "      pax_grp.point_dep = :point_id AND "
            "      pax.refuse IS NULL ";
        Qry.CreateVariable("point_id", otInteger, info.point_id);
        Qry.CreateVariable("pr_summer", otInteger, info.pr_summer);
        Qry.Execute();
        weight = Qry.FieldAsInteger(0);
    }

    struct TCOMStats {
        vector<TCOMStatsItem> items;
        TTotalPaxWeight total_pax_weight;
        void get(TTlgInfo &info);
        void ToTlg(ostringstream &body);
    };

    void TCOMStats::ToTlg(ostringstream &body)
    {
        TCOMStatsItem sum;
        sum.target = "TTL";
        for(vector<TCOMStatsItem>::iterator iv = items.begin(); iv != items.end(); iv++) {
            body
                << iv->target       << ' '
                << iv->adult        << '/'
                << iv->child        << '/'
                << iv->baby         << ' '
                << iv->bag_amount   << '/'
                << iv->bag_weight   << '/'
                << iv->rk_weight    << ' '
                << iv->f            << '/'
                << iv->c            << '/'
                << iv->y            << ' '
                << "0/0/0 0 0 "
                << iv->f_add_pax    << '/'
                << iv->c_add_pax    << '/'
                << iv->y_add_pax    << ' '
                << iv->f_child      << '/'
                << iv->c_child      << '/'
                << iv->y_child      << ' '
                << iv->f_baby       << '/'
                << iv->c_baby       << '/'
                << iv->y_baby       << ' '
                << iv->f_rk_weight  << '/'
                << iv->c_rk_weight  << '/'
                << iv->y_rk_weight  << ' '
                << iv->f_bag_weight << '/'
                << iv->c_bag_weight << '/'
                << iv->y_bag_weight << br;

            sum.adult += iv->adult;
            sum.child += iv->child;
            sum.baby += iv->baby;
            sum.bag_amount += iv->bag_amount;
            sum.bag_weight += iv->bag_weight;
            sum.rk_weight += iv->rk_weight;
            sum.f += iv->f;
            sum.c += iv->c;
            sum.y += iv->y;
            sum.f_add_pax += iv->f_add_pax;
            sum.c_add_pax += iv->c_add_pax;
            sum.y_add_pax += iv->y_add_pax;
            sum.f_child += iv->f_child;
            sum.c_child += iv->c_child;
            sum.y_child += iv->y_child;
            sum.f_baby += iv->f_baby;
            sum.c_baby += iv->c_baby;
            sum.y_baby += iv->y_baby;
            sum.f_rk_weight += iv->f_rk_weight;
            sum.c_rk_weight += iv->c_rk_weight;
            sum.y_rk_weight += iv->y_rk_weight;
            sum.f_bag_weight += iv->f_bag_weight;
            sum.c_bag_weight += iv->c_bag_weight;
            sum.y_bag_weight += iv->y_bag_weight;
        }
        body
            << sum.target       << ' '
            << sum.adult        << '/'
            << sum.child        << '/'
            << sum.baby         << ' '
            << sum.bag_amount   << '/'
            << sum.bag_weight   << '/'
            << sum.rk_weight    << ' '
            << sum.f            << '/'
            << sum.c            << '/'
            << sum.y            << ' '
            << "0/0/0 0 0 "
            << sum.f_add_pax    << '/'
            << sum.c_add_pax    << '/'
            << sum.y_add_pax    << ' '
            << "0 " << total_pax_weight.weight << ' '
            << sum.f_child      << '/'
            << sum.c_child      << '/'
            << sum.y_child      << ' '
            << sum.f_baby       << '/'
            << sum.c_baby       << '/'
            << sum.y_baby       << ' '
            << sum.f_rk_weight  << '/'
            << sum.c_rk_weight  << '/'
            << sum.y_rk_weight  << ' '
            << sum.f_bag_weight << '/'
            << sum.c_bag_weight << '/'
            << sum.y_bag_weight << br;
    }

    void TCOMStats::get(TTlgInfo &info)
    {
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "SELECT "
            "   points.airp target, "
            "   NVL(f, 0) f, "
            "   NVL(c, 0) c, "
            "   NVL(y, 0) y, "
            "   NVL(adult, 0) adult, "
            "   NVL(child, 0) child, "
            "   NVL(baby, 0) baby, "
            "   NVL(f_child, 0) f_child, "
            "   NVL(f_baby, 0) f_baby, "
            "   NVL(c_child, 0) c_child, "
            "   NVL(c_baby, 0) c_baby, "
            "   NVL(y_child, 0) y_child, "
            "   NVL(y_baby, 0) y_baby, "
            "   NVL(bag_amount, 0) bag_amount, "
            "   NVL(bag_weight, 0) bag_weight, "
            "   NVL(rk_weight, 0) rk_weight, "
            "   NVL(f_bag_weight, 0) f_bag_weight, "
            "   NVL(f_rk_weight, 0) f_rk_weight, "
            "   NVL(c_bag_weight, 0) c_bag_weight, "
            "   NVL(c_rk_weight, 0) c_rk_weight, "
            "   NVL(y_bag_weight, 0) y_bag_weight, "
            "   NVL(y_rk_weight, 0) y_rk_weight, "
            "   NVL(f_add_pax, 0) f_add_pax, "
            "   NVL(c_add_pax, 0) c_add_pax, "
            "   NVL(y_add_pax, 0) y_add_pax "
            "FROM "
            "   points, "
            "   ( "
            "SELECT "
            "   pax_grp.point_arv, "
            "   SUM(DECODE(pax_grp.class, 'П', DECODE(seats,0,0,1), 0)) f, "
            "   SUM(DECODE(pax_grp.class, 'Б', DECODE(seats,0,0,1), 0)) c, "
            "   SUM(DECODE(pax_grp.class, 'Э', DECODE(seats,0,0,1), 0)) y, "
            "   SUM(DECODE(pax.pers_type, 'ВЗ', 1, 0)) adult, "
            "   SUM(DECODE(pax.pers_type, 'РБ', 1, 0)) child, "
            "   SUM(DECODE(pax.pers_type, 'РМ', 1, 0)) baby, "
            "   SUM(DECODE(pax_grp.class||pax.pers_type, 'ПРБ', 1, 0)) f_child, "
            "   SUM(DECODE(pax_grp.class||pax.pers_type, 'ПРМ', 1, 0)) f_baby, "
            "   SUM(DECODE(pax_grp.class||pax.pers_type, 'БРБ', 1, 0)) c_child, "
            "   SUM(DECODE(pax_grp.class||pax.pers_type, 'БРМ', 1, 0)) c_baby, "
            "   SUM(DECODE(pax_grp.class||pax.pers_type, 'ЭРБ', 1, 0)) y_child, "
            "   SUM(DECODE(pax_grp.class||pax.pers_type, 'ЭРМ', 1, 0)) y_baby, "
            "   SUM(DECODE(pax_grp.class, 'П', decode(SIGN(1-pax.seats), -1, 1, 0), 0)) f_add_pax, "
            "   SUM(DECODE(pax_grp.class, 'Б', decode(SIGN(1-pax.seats), -1, 1, 0), 0)) c_add_pax, "
            "   SUM(DECODE(pax_grp.class, 'Э', decode(SIGN(1-pax.seats), -1, 1, 0), 0)) y_add_pax "
            "FROM "
            "   pax, pax_grp "
            "WHERE "
            "   pax_grp.point_dep = :point_id AND "
            "   pax_grp.grp_id = pax.grp_id AND "
            "   pax.refuse IS NULL "
            "GROUP BY "
            "   pax_grp.point_arv "
            "   ) a, "
            "   ( "
            "SELECT "
            "   pax_grp.point_arv, "
            "   SUM(DECODE(bag2.pr_cabin, 0, amount, 0)) bag_amount, "
            "   SUM(DECODE(bag2.pr_cabin, 0, weight, 0)) bag_weight, "
            "   SUM(DECODE(bag2.pr_cabin, 0, 0, weight)) rk_weight, "
            "   SUM(DECODE(pax_grp.class||bag2.pr_cabin, 'П0', weight, 0)) f_bag_weight, "
            "   SUM(DECODE(pax_grp.class||bag2.pr_cabin, 'П1', weight, 0)) f_rk_weight, "
            "   SUM(DECODE(pax_grp.class||bag2.pr_cabin, 'Б0', weight, 0)) c_bag_weight, "
            "   SUM(DECODE(pax_grp.class||bag2.pr_cabin, 'Б1', weight, 0)) c_rk_weight, "
            "   SUM(DECODE(pax_grp.class||bag2.pr_cabin, 'Э0', weight, 0)) y_bag_weight, "
            "   SUM(DECODE(pax_grp.class||bag2.pr_cabin, 'Э1', weight, 0)) y_rk_weight "
            "FROM "
            "   pax_grp, bag2 "
            "WHERE "
            "   pax_grp.point_dep = :point_id AND "
            "   pax_grp.grp_id = bag2.grp_id AND "
            "   pax_grp.bag_refuse = 0 "
            "GROUP BY "
            "   pax_grp.point_arv "
            "   ) b "
            "WHERE "
            "   points.point_id = a.point_arv(+) AND "
            "   points.point_id = b.point_arv(+) AND "
            "   first_point=:first_point AND point_num>:point_num AND pr_del=0 "
            "ORDER BY "
            "   point_num ";
        Qry.CreateVariable("point_id", otInteger, info.point_id);
        Qry.CreateVariable("first_point", otInteger, info.first_point);
        Qry.CreateVariable("point_num", otInteger, info.point_num);
        Qry.Execute();
        if(!Qry.Eof) {
            int col_target = Qry.FieldIndex("target");
            int col_f = Qry.FieldIndex("f");
            int col_c = Qry.FieldIndex("c");
            int col_y = Qry.FieldIndex("y");
            int col_adult = Qry.FieldIndex("adult");
            int col_child = Qry.FieldIndex("child");
            int col_baby = Qry.FieldIndex("baby");
            int col_f_child = Qry.FieldIndex("f_child");
            int col_f_baby = Qry.FieldIndex("f_baby");
            int col_c_child = Qry.FieldIndex("c_child");
            int col_c_baby = Qry.FieldIndex("c_baby");
            int col_y_child = Qry.FieldIndex("y_child");
            int col_y_baby = Qry.FieldIndex("y_baby");
            int col_bag_amount = Qry.FieldIndex("bag_amount");
            int col_bag_weight = Qry.FieldIndex("bag_weight");
            int col_rk_weight = Qry.FieldIndex("rk_weight");
            int col_f_bag_weight = Qry.FieldIndex("f_bag_weight");
            int col_f_rk_weight = Qry.FieldIndex("f_rk_weight");
            int col_c_bag_weight = Qry.FieldIndex("c_bag_weight");
            int col_c_rk_weight = Qry.FieldIndex("c_rk_weight");
            int col_y_bag_weight = Qry.FieldIndex("y_bag_weight");
            int col_y_rk_weight = Qry.FieldIndex("y_rk_weight");
            int col_f_add_pax = Qry.FieldIndex("f_add_pax");
            int col_c_add_pax = Qry.FieldIndex("c_add_pax");
            int col_y_add_pax = Qry.FieldIndex("y_add_pax");
            for(; !Qry.Eof; Qry.Next()) {
                TCOMStatsItem item;
                item.target = ElemIdToElem(etAirp, Qry.FieldAsString(col_target), info.pr_lat);
                item.f = Qry.FieldAsInteger(col_f);
                item.c = Qry.FieldAsInteger(col_c);
                item.y = Qry.FieldAsInteger(col_y);
                item.adult = Qry.FieldAsInteger(col_adult);
                item.child = Qry.FieldAsInteger(col_child);
                item.baby = Qry.FieldAsInteger(col_baby);
                item.f_child = Qry.FieldAsInteger(col_f_child);
                item.f_baby = Qry.FieldAsInteger(col_f_baby);
                item.c_child = Qry.FieldAsInteger(col_c_child);
                item.c_baby = Qry.FieldAsInteger(col_c_baby);
                item.y_child = Qry.FieldAsInteger(col_y_child);
                item.y_baby = Qry.FieldAsInteger(col_y_baby);
                item.bag_amount = Qry.FieldAsInteger(col_bag_amount);
                item.bag_weight = Qry.FieldAsInteger(col_bag_weight);
                item.rk_weight = Qry.FieldAsInteger(col_rk_weight);
                item.f_bag_weight = Qry.FieldAsInteger(col_f_bag_weight);
                item.f_rk_weight = Qry.FieldAsInteger(col_f_rk_weight);
                item.c_bag_weight = Qry.FieldAsInteger(col_c_bag_weight);
                item.c_rk_weight = Qry.FieldAsInteger(col_c_rk_weight);
                item.y_bag_weight = Qry.FieldAsInteger(col_y_bag_weight);
                item.y_rk_weight = Qry.FieldAsInteger(col_y_rk_weight);
                item.f_add_pax = Qry.FieldAsInteger(col_f_add_pax);
                item.c_add_pax = Qry.FieldAsInteger(col_c_add_pax);
                item.y_add_pax = Qry.FieldAsInteger(col_y_add_pax);
                items.push_back(item);
            }
        }
        total_pax_weight.get(info);
    }

    struct TCOMClassesItem {
        string cls;
        int cfg;
        int av;
        TCOMClassesItem() {
            cfg = NoExists;
            av = NoExists;
        }
    };

    struct TCOMClasses {
        vector<TCOMClassesItem> items;
        void get(TTlgInfo &info);
        void ToTlg(TTlgInfo &info, ostringstream &body);
    };

    void TCOMClasses::ToTlg(TTlgInfo &info, ostringstream &body)
    {
        ostringstream classes, av, padc;
        for(vector<TCOMClassesItem>::iterator iv = items.begin(); iv != items.end(); iv++) {
            classes << iv->cls << iv->cfg;
            av << iv->cls << iv->av;
            padc << iv->cls << '0';
        }
        body
            << "ARN/" << info.bort
            << " CNF/" << classes.str()
            << " CAP/" << classes.str()
            << " AV/" << av.str()
            << " PADC/" << padc.str()
            << br;
    }

    void TCOMClasses::get(TTlgInfo &info)
    {
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "SELECT "
            "   trip_classes.class, "
            "   trip_classes.cfg, "
            "   trip_classes.cfg-NVL(SUM(pax.seats),0) AS av "
            "FROM "
            "   trip_classes, "
            "   classes, "
            "   pax_grp, "
            "   pax "
            "WHERE "
            "   trip_classes.class = classes.code and "
            "   trip_classes.point_id = :point_id and "
            "   trip_classes.point_id = pax_grp.point_dep(+) and "
            "   trip_classes.class = pax_grp.class(+) and "
            "   pax_grp.grp_id = pax.grp_id(+) "
            "GROUP BY "
            "   trip_classes.class, "
            "   trip_classes.cfg, "
            "   classes.priority "
            "ORDER BY "
            "   classes.priority ";
        Qry.CreateVariable("point_id", otInteger, info.point_id);
        Qry.Execute();
        if(!Qry.Eof) {
            int col_class = Qry.FieldIndex("class");
            int col_cfg = Qry.FieldIndex("cfg");
            int col_av = Qry.FieldIndex("av");
            for(; !Qry.Eof; Qry.Next()) {
                TCOMClassesItem item;
                item.cls = ElemIdToElem(etSubcls, Qry.FieldAsString(col_class), info.pr_lat);
                item.cfg = Qry.FieldAsInteger(col_cfg);
                item.av = Qry.FieldAsInteger(col_av);
                items.push_back(item);
            }
        }
    }
}

using namespace PRL_SPACE;

int COM(TTlgInfo &info, int tst_tlg_id)
{
    TTlgOutPartInfo tlg_row;
    tlg_row.id = tst_tlg_id;
    tlg_row.num = 1;
    tlg_row.tlg_type = info.tlg_type;
    tlg_row.point_id = info.point_id;
    tlg_row.pr_lat = info.pr_lat;
    tlg_row.addr = info.addrs;
    tlg_row.time_create = NowUTC();
    ostringstream heading;
    heading
        << "." << info.sender << " " << DateTimeToStr(tlg_row.time_create, "ddhhnn") << br
        << "COM" << br;
    tlg_row.heading = heading.str();
    tlg_row.ending = "ENDCOM" + br;
    ostringstream body;
    body
        << info.airline << setw(3) << setfill('0') << info.flt_no << info.suffix << "/"
        << DateTimeToStr(info.scd_local, "ddmmm", 1) << " " << info.airp_dep
        << "/0 OP/NAM" << br;
    TCOMClasses classes;
    TCOMStats stats;
    classes.get(info);
    stats.get(info);
    classes.ToTlg(info, body);
    stats.ToTlg(body);
    tlg_row.body = body.str();
    ProgTrace(TRACE5, "COM: before save");
    SaveTlgOutPartTST(tlg_row);
    return tlg_row.id;
}

struct TPTMPaxListItem {
    string airline;
    int flt_no;
    string suffix;
    TDateTime scd;
    string airp_arv;
    string subclass;
    int seats;
    string pers_type;
    string surname;
    string name;
    int bagAmount;
    int grp_id;
    TPTMPaxListItem() {
        flt_no = NoExists;
        seats = 0;
        scd = NoExists;
        bagAmount = 0;
        grp_id = NoExists;
    }
};

struct TPTMPaxList {
    vector<TPTMPaxListItem> items;
    void get(TTlgInfo &info);
};

void TPTMPaxList::get(TTlgInfo &info)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT "
        "       trfer_trips.airline, "
        "       trfer_trips.flt_no, "
        "       trfer_trips.suffix, "
        "       trfer_trips.scd, "
        "       transfer.airp_arv, "
        "       transfer.subclass, "
        "       pax.seats, "
        "       pax.pers_type, "
        "       pax.surname, "
        "       pax.name, "
        "       NVL(ckin.get_bagAmount(pax.grp_id,pax.pax_id),0) AS bagAmount, "
        "       pax_grp.grp_id "
        "FROM pax_grp,pax,transfer,trfer_trips "
        "WHERE pax_grp.grp_id=pax.grp_id AND "
        "      pax_grp.grp_id=transfer.grp_id AND "
        "      transfer.point_id_trfer=trfer_trips.point_id AND "
        "      pax_grp.point_dep=:point_id AND "
        "      pax_grp.airp_arv=:airp AND "
        "      pax.pr_brd=1 AND "
        "      pax_grp.status<>'T' AND "
        "      transfer_num=1 "
        "ORDER BY trfer_trips.airline, "
        "         trfer_trips.flt_no, "
        "         trfer_trips.suffix, "
        "         trfer_trips.scd, "
        "         transfer.airp_arv, "
        "         transfer.subclass, "
        "         pax_grp.grp_id, "
        "         pax.surname, "
        "         pax.name ";

    Qry.CreateVariable("point_id", otInteger, info.point_id);
    Qry.CreateVariable("airp", otString, info.airp);
    Qry.Execute();
    if(!Qry.Eof) {
        int col_airline = Qry.FieldIndex("airline");
        int col_flt_no = Qry.FieldIndex("flt_no");
        int col_suffix = Qry.FieldIndex("suffix");
        int col_scd = Qry.FieldIndex("scd");
        int col_airp_arv = Qry.FieldIndex("airp_arv");
        int col_subclass = Qry.FieldIndex("subclass");
        int col_seats = Qry.FieldIndex("seats");
        int col_pers_type = Qry.FieldIndex("pers_type");
        int col_surname = Qry.FieldIndex("surname");
        int col_name = Qry.FieldIndex("name");
        int col_bagAmount = Qry.FieldIndex("bagAmount");
        int col_grp_id = Qry.FieldIndex("grp_id");
        for(; !Qry.Eof; Qry.Next()) {
            TPTMPaxListItem item;
            item.airline = Qry.FieldAsString(col_airline);
            item.flt_no = Qry.FieldAsInteger(col_flt_no);
            item.suffix = Qry.FieldAsString(col_suffix);
            item.scd = Qry.FieldAsDateTime(col_scd);
            item.airp_arv = Qry.FieldAsString(col_airp_arv);
            item.subclass = Qry.FieldAsString(col_subclass);
            item.seats = Qry.FieldAsInteger(col_seats);
            item.pers_type = Qry.FieldAsString(col_pers_type);
            item.surname = Qry.FieldAsString(col_surname);
            item.name = Qry.FieldAsString(col_name);
            item.bagAmount = Qry.FieldAsInteger(col_bagAmount);
            item.grp_id = Qry.FieldAsInteger(col_grp_id);
            items.push_back(item);
        }
    }
}

class LineOverflow: public Exception {
    public:
        LineOverflow( ):Exception( "ПЕРЕПОЛНЕНИЕ СТРОКИ ТЕЛЕГРАММЫ" ) { };
};

struct TBTMListItem {
    string airline;
    int flt_no;
    string suffix;
    TDateTime scd;
    string airp_arv;
    string subclass;
    int seats;
    string surname;
    string name;
    int grp_id;
    TBTMListItem() {
        flt_no = NoExists;
        scd = NoExists;
        seats = 0;
        grp_id = NoExists;
    }
};

struct TBTMList {
    vector<TBTMListItem> items;
    void get(TTlgInfo &info);
};

void TBTMList::get(TTlgInfo &info)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT "
        "       trfer_trips.airline, "
        "       trfer_trips.flt_no, "
        "       trfer_trips.suffix, "
        "       trfer_trips.scd, "
        "       transfer.airp_arv, "
        "       transfer.subclass, "
        "       NVL(seats,1) AS seats, "
        "       NVL(pax.surname,'UNACCOMPANIED') AS surname, "
        "       pax.name, "
        "       pax_grp.grp_id "
        "FROM pax_grp,pax,transfer,trfer_trips "
        "WHERE pax_grp.grp_id=pax.grp_id(+) AND "
        "      (pax.grp_id IS NULL AND pax_grp.class IS NULL OR  "
        "       pax.grp_id IS NOT NULL) AND  "
        "      pax_grp.bag_refuse=0 AND "
        "      pax_grp.grp_id=transfer.grp_id AND "
        "      transfer.point_id_trfer=trfer_trips.point_id AND "
        "      pax_grp.point_dep=:point_id AND "
        "      pax_grp.airp_arv=:airp AND "
        "      pax.pr_brd(+)=1 AND "
        "      pax_grp.status<>'T' AND pax.seats(+)>0 AND "
        "      transfer_num=1 "
        "ORDER BY trfer_trips.airline, "
        "         trfer_trips.flt_no, "
        "         trfer_trips.suffix, "
        "         trfer_trips.scd, "
        "         transfer.airp_arv, "
        "         transfer.subclass, "
        "         DECODE(pax.grp_id,NULL,1,0), "
        "         pax_grp.grp_id, "
        "         pax.surname, "
        "         pax.name ";
    Qry.CreateVariable("airp", otString, info.airp);
    Qry.CreateVariable("point_id", otInteger, info.point_id);
    Qry.Execute();
    if(!Qry.Eof) {
        int col_airline = Qry.FieldIndex("airline");
        int col_flt_no = Qry.FieldIndex("flt_no");
        int col_suffix = Qry.FieldIndex("suffix");
        int col_scd = Qry.FieldIndex("scd");
        int col_airp_arv = Qry.FieldIndex("airp_arv");
        int col_subclass = Qry.FieldIndex("subclass");
        int col_seats = Qry.FieldIndex("seats");
        int col_surname = Qry.FieldIndex("surname");
        int col_name = Qry.FieldIndex("name");
        int col_grp_id = Qry.FieldIndex("grp_id");
        for(; !Qry.Eof; Qry.Next()) {
            TBTMListItem item;
            item.airline = Qry.FieldAsString(col_airline);
            item.flt_no = Qry.FieldAsInteger(col_flt_no);
            item.suffix = Qry.FieldAsString(col_suffix);
            item.scd = Qry.FieldAsDateTime(col_scd);
            item.airp_arv = Qry.FieldAsString(col_airp_arv);
            item.subclass = Qry.FieldAsString(col_subclass);
            item.seats = Qry.FieldAsInteger(col_seats);
            item.surname = Qry.FieldAsString(col_surname);
            item.name = Qry.FieldAsString(col_name);
            item.grp_id = Qry.FieldAsInteger(col_grp_id);
            items.push_back(item);
        }
    }
}

struct TBTMTagsItem {
    string tag_type;
    int no_len;
    double no;
    string color;
    TBTMTagsItem() {
        no_len = NoExists;
        no = NoExists;
    }
};

struct TBTMTags {
    vector<TBTMTagsItem> items;
    void get(int grp_id);
};

void TBTMTags::get(int grp_id)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT "
        "    tag_type, "
        "    no_len, "
        "    no, "
        "    color "
        "FROM "
        "    bag_tags, "
        "    tag_types "
        "WHERE "
        "    bag_tags.tag_type=tag_types.code AND "
        "    grp_id=:grp_id "
        "ORDER BY "
        "    tag_type, "
        "    color, "
        "    no ";
    Qry.CreateVariable("grp_id", otInteger, grp_id);
    Qry.Execute();
    if(!Qry.Eof) {
        int col_tag_type = Qry.FieldIndex("tag_type");
        int col_no_len = Qry.FieldIndex("no_len");
        int col_no = Qry.FieldIndex("no");
        int col_color = Qry.FieldIndex("color");
        for(; !Qry.Eof; Qry.Next()) {
            TBTMTagsItem item;
            item.tag_type = Qry.FieldAsString(col_tag_type);
            item.no_len = Qry.FieldAsInteger(col_no_len);
            item.no = Qry.FieldAsFloat(col_no);
            item.color = Qry.FieldAsString(col_color);
            items.push_back(item);
        }
    }
}

struct TBTMTrferItem {
    string airline;
    int flt_no;
    string suffix;
    TDateTime scd;
    string airp_arv;
    string subclass;
    TBTMTrferItem() {
        flt_no = NoExists;
        scd = NoExists;
    }
};

struct TBTMTrfer {
    vector<TBTMTrferItem> items;
    void get(int grp_id);
};

void TBTMTrfer::get(int grp_id)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT "
        "    airline, "
        "    flt_no, "
        "    suffix, "
        "    scd, "
        "    airp_arv, "
        "    subclass "
        "FROM "
        "    transfer, "
        "    trfer_trips  "
        "WHERE "
        "    transfer.point_id_trfer=trfer_trips.point_id AND "
        "    grp_id=:grp_id AND "
        "    transfer_num>=2 "
        "ORDER BY "
        "    transfer_num ";
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
            TBTMTrferItem item;
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

int BTM(TTlgInfo &info, int tst_tlg_id)
{
    const size_t NAMES_SIZE = 58; // 64-LENGTH('.P/000')
    TTlgOutPartInfo tlg_row;
    tlg_row.id = tst_tlg_id;
    tlg_row.num = 1;
    tlg_row.tlg_type = info.tlg_type;
    tlg_row.point_id = info.point_id;
    tlg_row.pr_lat = info.pr_lat;
    tlg_row.extra = info.airp;
    tlg_row.addr = info.addrs;
    tlg_row.time_create = NowUTC();
    ostringstream heading1, heading2;
    heading1
        << "." << info.sender << " " << DateTimeToStr(tlg_row.time_create,"ddhhnn") << br
        << "BTM" << br
        << ".V/1T" << info.airp_arv;
    heading2
        << ".I/"
        << info.airline << setw(3) << setfill('0') << info.flt_no << info.suffix << "/"
        << DateTimeToStr(info.scd_local, "ddmmm", 1) << "/" << info.airp_dep << br;
    tlg_row.heading = heading1.str() + "/PART" + IntToString(tlg_row.num) + br + heading2.str();
    tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + br;
    size_t part_len = tlg_row.addr.size() + tlg_row.heading.size() + tlg_row.ending.size();

    int j = 1;
    int seats = 0;
    TBTMList BTMList;
    BTMList.get(info);
    vector<string> grp;
    if(!BTMList.items.empty()) {
        TBTMListItem old1Row;
        vector<TBTMListItem>::iterator cur1Row = BTMList.items.begin();
        string names;
        while(true) {
            if(
                    cur1Row == BTMList.items.end() or
                    old1Row.grp_id == NoExists or
                    not (
                        old1Row.airline == cur1Row->airline and
                        old1Row.flt_no == cur1Row->flt_no and
                        (
                         !old1Row.suffix.empty() and !cur1Row->suffix.empty() and
                         old1Row.suffix == cur1Row->suffix or
                         old1Row.suffix.empty() and cur1Row->suffix.empty()
                        ) and
                        old1Row.scd == cur1Row->scd and
                        old1Row.airp_arv == cur1Row->airp_arv and
                        (
                         !old1Row.subclass.empty() and !cur1Row->subclass.empty() and
                         old1Row.subclass == cur1Row->subclass or
                         old1Row.subclass.empty() and cur1Row->subclass.empty()
                        ) and
                        old1Row.grp_id == cur1Row->grp_id
                        )
              ) {
                if(grp.size() > 1) { // есть багажные бирки
                    // записать grp
                    if(!names.empty()) {
                        if(seats > 1)
                            grp.push_back(".P/" + IntToString(seats) + names);
                        else
                            grp.push_back(".P/" + names);
                        seats = 0;
                        names.erase();
                    }
                    // оценим, сможем ли мы группу уместить в текущую часть телеграммы
                    size_t part_len2 = part_len;
                    for(size_t i = j - 1; i < grp.size(); i++)
                        part_len2 += grp[i].size() + br.size();
                    if(part_len2 > PART_SIZE) {
                        SaveTlgOutPartTST(tlg_row);
                        tlg_row.heading = heading1.str() + "/PART" + IntToString(tlg_row.num) + br + heading2.str();
                        tlg_row.body.erase();
                        tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + br;
                        part_len = tlg_row.addr.size() + tlg_row.heading.size() + tlg_row.ending.size();
                        for(size_t i = 0; i < grp.size(); i++) {
                            part_len += grp[i].size() + br.size();
                            tlg_row.body = tlg_row.body + grp[i] + br;
                        }
                    } else {
                        for(size_t i = j - 1; i < grp.size(); i++) {
                            part_len += grp[i].size() + br.size();
                            tlg_row.body = tlg_row.body + grp[i] + br;
                        }
                    }
                }

                grp.clear();

                if(cur1Row == BTMList.items.end()) break;

                if(
                        old1Row.grp_id == NoExists or
                        not(
                            old1Row.airline == cur1Row->airline and
                            old1Row.flt_no == cur1Row->flt_no and
                            (
                             !old1Row.suffix.empty() and !cur1Row->suffix.empty() and
                             old1Row.suffix == cur1Row->suffix or
                             old1Row.suffix.empty() and cur1Row->suffix.empty()
                            ) and
                            old1Row.scd == cur1Row->scd and
                            old1Row.airp_arv == cur1Row->airp_arv and
                            (
                             !old1Row.subclass.empty() and !cur1Row->subclass.empty() and
                             old1Row.subclass == cur1Row->subclass or
                             old1Row.subclass.empty() and cur1Row->subclass.empty()
                            )
                           )
                  )
                    // составить .F
                    j = 1;
                else
                    j = 2;

                // выведем новую групп
                // здесь cur3Row только в качестве вспомогательной переменной
                TBTMListItem cur3Row;
                cur3Row.airline = cur1Row->airline;
                cur3Row.flt_no = cur1Row->flt_no;
                cur3Row.suffix = cur1Row->suffix;
                cur3Row.scd = cur1Row->scd;
                cur3Row.airp_arv = cur1Row->airp_arv;
                cur3Row.subclass = cur1Row->subclass;

                cur3Row.airline = ElemIdToElem(etAirline, cur3Row.airline, info.pr_lat);
                // составляем первую строку .F
                if(info.pr_lat and not is_lat(cur3Row.suffix)) cur3Row.suffix.erase();
                cur3Row.airp_arv = ElemIdToElem(etAirp, cur3Row.airp_arv, info.pr_lat);
                ostringstream buf;
                buf
                    << ".F/" << cur3Row.airline
                    << setw(3) << setfill('0') << cur3Row.flt_no << cur3Row.suffix
                    << "/"
                    << DateTimeToStr(cur3Row.scd, "ddmmm", info.pr_lat)
                    << "/"
                    <<  cur3Row.airp_arv;
                if(!cur3Row.subclass.empty()) {
                    cur3Row.subclass = ElemIdToElem(etSubcls, cur3Row.subclass, info.pr_lat);
                    buf << "/" << cur3Row.subclass;
                }
                grp.push_back(buf.str());
                // получим данные по номерам бирок
                int num = 0;
                TBTMTagsItem old2Row;
                TBTMTags cur2;
                cur2.get(cur1Row->grp_id);
                vector<TBTMTagsItem>::iterator cur2Row = cur2.items.begin();
                if(cur2Row != cur2.items.end()) {
                    while(true) {
                        if (
                                cur2Row == cur2.items.end() or
                                old2Row.no != NoExists and
                                not (
                                    old2Row.tag_type == cur2Row->tag_type and
                                    (
                                     !old2Row.color.empty() and !cur2Row->color.empty() and
                                     old2Row.color == cur2Row->color or
                                     old2Row.color.empty() and cur2Row->color.empty()
                                    ) and
                                    old2Row.no + 1 == cur2Row->no and num < 999
                                    )
                           ) {
                            ostringstream buf;
                            old2Row.no = old2Row.no - num + 1;
                            buf
                                << ".N/" << fixed << setprecision(0) << setw(10) << setfill('0') << old2Row.no
                                << setw(3) << setfill('0') << num;
                            grp.push_back(buf.str());
                            if(cur2Row == cur2.items.end()) break;
                            num = 1;
                        } else {
                            num++;
                        }
                        old2Row = *cur2Row;
                        cur2Row++;
                    }
                    TQuery Qry(&OraSession);
                    Qry.SQLText =
                        "SELECT "
                        "  NVL(ckin.get_bagAmount(:grp_id,NULL),0) bagAmount, "
                        "  NVL(ckin.get_bagWeight(:grp_id,NULL),0) bagWeight, "
                        "  NVL(ckin.get_rkWeight(:grp_id,NULL),0) rkWeight "
                        "FROM dual ";
                    Qry.CreateVariable("grp_id", otInteger, cur1Row->grp_id);
                    Qry.Execute();
                    int bagAmount = Qry.FieldAsInteger("bagAmount");
                    int bagWeight = Qry.FieldAsInteger("bagWeight");
                    int rkWeight = Qry.FieldAsInteger("rkWeight");
                    ostringstream buf;
                    buf << ".W/K/" << bagAmount << '/' << bagWeight;
                    if(rkWeight != 0)
                        buf << '/' << rkWeight;
                    grp.push_back(buf.str());
                    TBTMTrfer cur3;
                    cur3.get(cur1Row->grp_id);
                    for(vector<TBTMTrferItem>::iterator cur3Row = cur3.items.begin(); cur3Row != cur3.items.end(); cur3Row++) {
                        cur3Row->airline = ElemIdToElem(etAirline, cur3Row->airline, info.pr_lat);
                        if(info.pr_lat and not is_lat(cur3Row->suffix))
                            cur3Row->suffix.erase();
                        cur3Row->airp_arv = ElemIdToElem(etAirp, cur3Row->airp_arv, info.pr_lat);
                        ostringstream buf;
                        buf
                            << ".O/"
                            << cur3Row->airline
                            << setw(3) << setfill('0') << cur3Row->flt_no
                            << cur3Row->suffix
                            << '/'
                            << DateTimeToStr(cur3Row->scd, "ddmmm", info.pr_lat)
                            << '/'
                            << cur3Row->airp_arv;
                        if(not cur3Row->subclass.empty())
                            buf
                                << '/' << ElemIdToElem(etSubcls, cur3Row->subclass, info.pr_lat);
                        grp.push_back(buf.str());
                    }
                }
            }
            if(grp.size() > 1) { // есть багажные бирки
                if(
                        not old1Row.surname.empty() and
                        old1Row.surname != cur1Row->surname and
                        seats > 0
                  ) {
                    ostringstream buf;
                    if(seats > 1)
                        buf << ".P/" << seats << names;
                    else
                        buf << ".P/" << names;
                    grp.push_back(buf.str());
                    seats = 0;
                    names.erase();
                }
                for(int k = 1; k <= 2; k++) {
                    if(names.empty())
                        names = transliter(cur1Row->surname, info.pr_lat).substr(0, 58);
                    try {
                        if(not cur1Row->name.empty()) {
                            if(
                                    cur1Row->surname == "ZZ" and
                                    not old1Row.name.empty() and old1Row.name == cur1Row->name
                              ) {
                                ;
                            } else {
                                ostringstream buf;
                                buf << '/' << transliter(cur1Row->name, info.pr_lat);
                                for(int i = 2; i <= cur1Row->seats; i++) {
                                    buf << "/EXST";
                                }
                                if(names.size() + buf.str().size() > NAMES_SIZE)
                                    throw LineOverflow();
                                names += buf.str();
                            }
                        }
                        seats += cur1Row->seats;
                        break;
                    } catch(LineOverflow E) {
                        ostringstream buf;
                        if(seats > 0) {
                            if(seats > 1)
                                buf << ".P/" << seats << names;
                            else
                                buf << ".P/" << names;
                            grp.push_back(buf.str());
                            seats = 0;
                            names.erase();
                        } else {
                            seats += cur1Row->seats;
                            break;
                        }
                    }
                }
                old1Row = *cur1Row;
            }
            cur1Row++;
        }
    }
    if(tlg_row.num == 1)
        tlg_row.heading = heading1.str() + br + heading2.str();
    tlg_row.ending = "ENDBTM" + br;
    SaveTlgOutPartTST(tlg_row);
    return tlg_row.id;
}

int PTM(TTlgInfo &info, int tst_tlg_id)
{
    TTlgOutPartInfo tlg_row;
    tlg_row.id = tst_tlg_id;
    tlg_row.num = 1;
    tlg_row.tlg_type = info.tlg_type;
    tlg_row.point_id = info.point_id;
    tlg_row.pr_lat = info.pr_lat;
    tlg_row.extra = info.airp;
    tlg_row.addr = info.addrs;
    tlg_row.time_create = NowUTC();
    ostringstream heading;
    heading
        << "." << info.sender << " " << DateTimeToStr(tlg_row.time_create, "ddhhnn") << br
        << "PTM" << br
        << info.airline << setw(3) << setfill('0') << info.flt_no << info.suffix << "/"
        << DateTimeToStr(info.scd_local, "ddmmm", 1) << " " << info.airp_dep << info.airp_arv << " ";
    tlg_row.heading = heading.str() + "PART" + IntToString(tlg_row.num) + br;
    tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + br;
    size_t part_len = tlg_row.addr.size() + tlg_row.heading.size() + tlg_row.ending.size();
    TPTMPaxList pax_list;
    pax_list.get(info);
    ostringstream body;
    if(pax_list.items.empty()) {
        tlg_row.body = "NIL" + br;
    } else {
        TPTMPaxListItem old1Row;
        vector<TPTMPaxListItem>::iterator cur1Row = pax_list.items.begin();
        int seats = 0;
        int chd = 0;
        int inf = 0;
        int bagAmount = 0;
        string names;
        ostringstream grp, grph;
        while(true) {
            if(
                    cur1Row == pax_list.items.end() or
                    old1Row.grp_id != NoExists and
                    (
                     old1Row.airline != cur1Row->airline or
                     old1Row.flt_no != cur1Row->flt_no or
                     old1Row.suffix != cur1Row->suffix or
                     old1Row.scd != cur1Row->scd or
                     old1Row.airp_arv != cur1Row->airp_arv or
                     old1Row.subclass != cur1Row->subclass or
                     info.tlg_type != "PTMN" and
                     (
                      old1Row.grp_id != cur1Row->grp_id or
                      not old1Row.surname.empty() and //NULL только тогда, когда предыдущий ребенок, и он первый в группе
                      old1Row.surname != cur1Row->surname and
                      cur1Row->seats > 0
                     )
                    )
              ) {
                old1Row.airline = ElemIdToElem(etAirline, old1Row.airline, info.pr_lat);
                if(info.pr_lat and !is_lat(old1Row.suffix))
                    old1Row.suffix = "";
                old1Row.airp_arv = ElemIdToElem(etAirp, old1Row.airp_arv, info.pr_lat);
                old1Row.subclass = ElemIdToElem(etSubcls, old1Row.subclass, info.pr_lat);
                grp.str("");
                grp
                    << old1Row.airline
                    << setw(3) << setfill('0') << old1Row.flt_no << old1Row.suffix << '/'
                    << DateTimeToStr(old1Row.scd, "dd") << ' ' << old1Row.airp_arv << ' '
                    << seats << ' ' << old1Row.subclass << ' ' << bagAmount << 'B';
                grph.str("");
                if(chd > 0) grph << ".CHD" << chd;
                if(inf > 0) grph << ".INF" << inf;
                size_t pos = 64 - (grp.str().size() + grph.str().size() + 1);
                if(pos > 0 and not names.empty()) {
                    if(names.size() > pos) {
                        names = names.substr(0, pos - 1);
                        pos = names.rfind("/");
                        if(pos != string::npos)
                            names = names.substr(0, pos - 1);
                    }
                    grp << ' ' << names << grph.str();
                } else
                    grp << grph.str();
                part_len += grp.str().size() + br.size();
                if(part_len > PART_SIZE) {
                    SaveTlgOutPartTST(tlg_row);
                    tlg_row.heading = heading.str() + "PART" + IntToString(tlg_row.num) + br;
                    tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + br;
                    tlg_row.body = grp.str() + br;
                    part_len = tlg_row.addr.size() + tlg_row.heading.size() +
                        tlg_row.body.size() + tlg_row.ending.size();
                } else
                    tlg_row.body += grp.str() + br;
                seats = 0;
                chd = 0;
                inf = 0;
                bagAmount = 0;
                names.clear();
            }
            if(cur1Row == pax_list.items.end()) break;
            if(cur1Row->pers_type == "РБ") chd++;
            if(cur1Row->pers_type == "РМ") inf++;
            bagAmount += cur1Row->bagAmount;

            //формирование pnr
            if(cur1Row->seats > 0) {
                seats++;
                if(info.tlg_type != "PTMN") {
                    try {
                        string buf;
                        buf = transliter(cur1Row->surname, info.pr_lat);
                        if(buf.size() > LINE_SIZE) throw LineOverflow();
                        if(names.empty()) names = buf;
                        if(!cur1Row->name.empty()) {
                            if(cur1Row->surname == "ZZ" and !old1Row.name.empty() and old1Row.name == cur1Row->name) {
                                ;
                            } else {
                                buf = '/' + transliter(cur1Row->name, info.pr_lat);
                                if(names.size() + buf.size() > LINE_SIZE)
                                    throw LineOverflow();
                                names += buf;
                                for(int i = 2; i <= cur1Row->seats; i++) {
                                    buf = "/EXST";
                                    if(names.size() + buf.size() > LINE_SIZE)
                                        throw LineOverflow();
                                    names += buf; //впоследствии надо учитывать STCR
                                }
                            }
                        }
                    } catch(LineOverflow E) {
                    }
                }
            } else {
                if(old1Row.grp_id == NoExists or old1Row.grp_id != cur1Row->grp_id)
                    cur1Row->surname.clear();
                else
                    cur1Row->surname = old1Row.surname;
            }
            old1Row = *cur1Row;
            cur1Row++;
        }
    }
    tlg_row.ending = "ENDPTM" + br;
    SaveTlgOutPartTST(tlg_row);
    return tlg_row.id;
}

struct TSOMPlace {
    int x, y, num, point_arv;
    string xname, yname;
    void dump();
    TSOMPlace() {
        num = NoExists;
        x = NoExists;
        y = NoExists;
        point_arv = NoExists;
    }
};

void TSOMPlace::dump()
{
    ostringstream buf;
    buf
        << "num: " << num << "; "
        << "y: " << y << "; "
        << "x: " << x << "; ";
    if(point_arv != NoExists)
        buf << "point_arv: " << point_arv << "; ";
    if(!xname.empty())
        buf << yname << xname;
    ProgTrace(TRACE5, buf.str().c_str());
}

typedef map<int, TSOMPlace> t_som_row;
typedef map<int, t_som_row> t_som_block;
typedef map<int, t_som_block> t_som_comp;

struct TSOMList {
    private:
        t_som_comp comp;
        void init_comp(int point_id);
        void apply_comp(int point_id);
        void get_places(int point_dep, int point_arv, int pax_id, string seat_no, int seats);
        void get_place(int point_dep, TSOMPlace &place, string seat_no);
        void dump_comp();
        void dump_list(map<int, string> &list);
        void seat_to_str(string &list, t_som_row::iterator &first_place,  t_som_row::iterator &last_place);
        void get_seat_list(map<int, string> &list);
    public:
        vector<string> items;
        void get(TTlgInfo &info);
};

void TSOMList::dump_list(map<int, string> &list)
{
    for(map<int, string>::iterator im = list.begin(); im != list.end(); im++) {
        ProgTrace(TRACE5, "point_arv: %d; seats: %s", im->first, (convert_seat_no(im->second, 1)).c_str());
    }
}
void TSOMList::get_seat_list(map<int, string> &list)
{
    map<int, t_som_row::iterator> first_place_map;
    map<int, t_som_row::iterator> last_place_map;
    t_som_row::iterator *first_place = NULL;
    t_som_row::iterator *last_place = NULL;
    for(t_som_comp::iterator anum = comp.begin(); anum != comp.end(); anum++)
        for(t_som_block::iterator ay = anum->second.begin(); ay != anum->second.end(); ay++)
            for(t_som_row::iterator ax = ay->second.begin(); ax != ay->second.end(); ax++) {
                if(ax->second.point_arv != NoExists) {
                    if(last_place == NULL or (*last_place)->second.point_arv != ax->second.point_arv) {
                        if(last_place != NULL)
                            seat_to_str(list[(*first_place)->second.point_arv], *first_place, *last_place);
                        if(first_place_map.find(ax->second.point_arv) == first_place_map.end()) {
                            first_place_map[ax->second.point_arv] = NULL;
                            last_place_map[ax->second.point_arv] = NULL;
                        }
                        first_place = &first_place_map[ax->second.point_arv];
                        last_place = &last_place_map[ax->second.point_arv];
                    }
                    if(*first_place == NULL) {
                        *first_place = ax;
                        *last_place = *first_place;
                    } else {
                        t_som_row::iterator prev_place;
                        if(ax == ay->second.begin()) {
                            t_som_block::iterator prev_row = ay;
                            prev_row--;
                            prev_place = prev_row->second.end();
                            prev_place--;
                        } else {
                            prev_place = ax;
                            prev_place--;
                        }
                        if(*last_place == prev_place)
                            *last_place = ax;
                        else {
                            seat_to_str(list[(*first_place)->second.point_arv], *first_place, *last_place);
                            *first_place = ax;
                            *last_place = *first_place;
                        }
                    }
                }
            }
    seat_to_str(list[(*first_place)->second.point_arv], *first_place, *last_place);
}

void TSOMList::seat_to_str(string &list, t_som_row::iterator &first_place,  t_som_row::iterator &last_place)
{
    if(!list.empty())
        list += ", ";
    if(first_place == last_place)
        list += first_place->second.yname + first_place->second.xname;
    else {
        list +=
            first_place->second.yname + first_place->second.xname
            + "-"
            + last_place->second.yname + last_place->second.xname;
    }
}

void TSOMList::get_place(int point_dep, TSOMPlace &place, string seat_no)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT num, x, y, xname, yname FROM trip_comp_elems WHERE "
        "   yname||xname = DECODE( INSTR( :seat_no, '0' ), 1, SUBSTR( :seat_no, 2 ), :seat_no ) AND "
        "   point_id=:point_id";
    Qry.CreateVariable("point_id", otInteger, point_dep);
    Qry.CreateVariable("seat_no", otString, seat_no);
    Qry.Execute();
    if(!Qry.Eof) {
        place.num = Qry.FieldAsInteger("num");
        place.x = Qry.FieldAsInteger("x");
        place.y = Qry.FieldAsInteger("y");
        place.xname = Qry.FieldAsString("xname");
        place.yname = Qry.FieldAsString("yname");
    }
}

void TSOMList::get_places(int point_dep, int point_arv, int pax_id, string seat_no, int seats)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   decode(count(*), 0, 0, 1) "
        "from "
        "   pax_rem "
        "where "
        "   pax_id = :pax_id and "
        "   rem_code = 'STCR'";
    Qry.CreateVariable("pax_id", otInteger, pax_id);
    Qry.Execute();
    if(Qry.Eof)
        throw Exception("TSOMList::get_places: pr_vert fetch failed for pax_id: %d", pax_id);
    bool pr_vert = Qry.FieldAsInteger(0) != 0;
    TSOMPlace place;
    get_place(point_dep, place, seat_no);
    place.point_arv = point_arv;
    comp[place.num][place.y][place.x] = place;
    Qry.Clear();
    Qry.SQLText =
        "select yname, xname from trip_comp_elems where "
        "   point_id = :point_id and "
        "   x = :x and "
        "   y = :y and "
        "   num = :num ";
    Qry.CreateVariable("point_id", otInteger, point_dep);
    Qry.CreateVariable("num", otInteger, place.num);
    Qry.DeclareVariable("x", otInteger);
    Qry.DeclareVariable("y", otInteger);
    for(int i = 1; i < seats; i++) {
        if(pr_vert)
            place.y++;
        else
            place.x++;
        Qry.SetVariable("x", place.x);
        Qry.SetVariable("y", place.y);
        Qry.Execute();
        if(Qry.Eof)
            throw Exception("TSOMList::get_places: next seat fetch failed. seqat_no: %s, x: %d, y: %d", seat_no.c_str(), place.x, place.y);
        place.xname = Qry.FieldAsString("xname");
        place.yname = Qry.FieldAsString("yname");
        place.point_arv = point_arv;
        comp[place.num][place.y][place.x] = place;
    }
}

void TSOMList::dump_comp()
{
    for(t_som_comp::iterator anum = comp.begin(); anum != comp.end(); anum++)
        for(t_som_block::iterator ay = anum->second.begin(); ay != anum->second.end(); ay++)
            for(t_som_row::iterator ax = ay->second.begin(); ax != ay->second.end(); ax++) {
                ostringstream buf;
                buf
                    << "num: " << anum->first << "; "
                    << "y: " << ay->first << "; "
                    << "x: " << ax->first << "; ";
                if(!ax->second.yname.empty())
                    buf << ax->second.yname << ax->second.xname << " " << ax->second.point_arv;
                ProgTrace(TRACE5, "%s", buf.str().c_str());
            }
}

void TSOMList::apply_comp(int point_id)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   pax.pax_id, "
        "   pax.seat_no, "
        "   pax.seats, "
        "   pax_grp.point_arv "
        "from "
        "   pax_grp, "
        "   pax "
        "where "
        "   pax_grp.point_dep = :point_dep and "
        "   pax_grp.grp_id = pax.grp_id ";
    Qry.CreateVariable("point_dep", otInteger, point_id);
    Qry.Execute();
    if(!Qry.Eof) {
        int col_pax_id = Qry.FieldIndex("pax_id");
        int col_seat_no = Qry.FieldIndex("seat_no");
        int col_seats = Qry.FieldIndex("seats");
        int col_point_arv = Qry.FieldIndex("point_arv");
        for(; !Qry.Eof; Qry.Next()) {
            if(Qry.FieldIsNULL(col_seat_no))
                continue;
            get_places(
                    point_id,
                    Qry.FieldAsInteger(col_point_arv),
                    Qry.FieldAsInteger(col_pax_id),
                    Qry.FieldAsString(col_seat_no),
                    Qry.FieldAsInteger(col_seats)
                    );
        }
    }
}

void TSOMList::init_comp(int point_id)
{
    TQuery Qry(&OraSession);
    Qry.SQLText = "select num, x, y from trip_comp_elems where point_id = :point_id order by num, y, x";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    for(; !Qry.Eof; Qry.Next()) {
        TSOMPlace place;
        int num = Qry.FieldAsInteger("num");
        int y = Qry.FieldAsInteger("y");
        int x = Qry.FieldAsInteger("x");
        comp[num][y][x] = place;
    }
}

void TSOMList::get(TTlgInfo &info)
{
    init_comp(info.point_id);
    apply_comp(info.point_id);
    map<int, string> list;
    get_seat_list(list);
    dump_list(list);
    // finally we got map with key - point_arv, data - string represents seat list for given point_arv

    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   point_num, "
        "   DECODE(pr_tranzit,0,point_id,first_point) first_point "
        "from "
        "   points "
        "where "
        "   point_id = :point_id AND pr_del=0 AND pr_reg<>0";
    Qry.CreateVariable("point_id", otInteger, info.point_id);
    Qry.Execute();
    if(Qry.Eof)
        throw UserException("Рейс не найден");
    int vpoint_num = Qry.FieldAsInteger("point_num");
    int vfirst_point = Qry.FieldAsInteger("first_point");
    Qry.Clear();
    Qry.SQLText =
        "  SELECT point_id, airp FROM points "
        "  WHERE first_point = :vfirst_point AND point_num > :vpoint_num AND pr_del=0 "
        "ORDER by "
        "  point_num ";
    Qry.CreateVariable("vfirst_point", otInteger, vfirst_point);
    Qry.CreateVariable("vpoint_num", otInteger, vpoint_num);
    Qry.Execute();
    for(; !Qry.Eof; Qry.Next()) {
        string item;
        int point_id = Qry.FieldAsInteger("point_id");
        string airp = Qry.FieldAsString("airp");
        item = "-" + TlgElemIdToElem(etAirp, airp, info.pr_lat) + ".";
        if(list[point_id].empty())
            item += "NIL";
        else
            item += convert_seat_no(list[point_id], info.pr_lat);
        items.push_back(item);
    }
}

int SOM(TTlgInfo &info, int tst_tlg_id)
{
    ProgTrace(TRACE5, "SOM started");
    TTlgOutPartInfo tlg_row;
    tlg_row.id = tst_tlg_id;
    tlg_row.num = 1;
    tlg_row.tlg_type = info.tlg_type;
    tlg_row.point_id = info.point_id;
    tlg_row.pr_lat = info.pr_lat;
    tlg_row.addr = info.addrs;
    tlg_row.time_create = NowUTC();
    ostringstream heading;
    heading
        << "." << info.sender << " " << DateTimeToStr(tlg_row.time_create, "ddhhnn") << br
        << "SOM" << br
        << info.airline << setw(3) << setfill('0') << info.flt_no << info.suffix << "/"
        << DateTimeToStr(info.scd_local, "ddmmm", 1) << " " << info.airp_dep << " ";
    tlg_row.heading = heading.str() + "PART" + IntToString(tlg_row.num) + br;
    tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + br;
    size_t part_len = tlg_row.addr.size() + tlg_row.heading.size() + tlg_row.ending.size();
    TSOMList SOMList;
    SOMList.get(info);
    for(vector<string>::iterator iv = SOMList.items.begin(); iv != SOMList.items.end(); iv++) {
    }
    return tlg_row.id;
}

int PRL(TTlgInfo &info, int tst_tlg_id)
{
    TTlgOutPartInfo tlg_row;
    tlg_row.id = tst_tlg_id;
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
        "   point_id = :point_id AND pr_del=0 AND pr_reg<>0";
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
                << "00" << TlgElemIdToElem(etClass, iv->cls, true); //всегда на латинице - так надо
            body.push_back(line.str());
        } else {
            pr_empty = false;
            line.str("");
            line
                << "-" << TlgElemIdToElem(etAirp, iv->airp, info.pr_lat)
                << setw(2) << setfill('0') << iv->PaxList.size()
                << TlgElemIdToElem(etClsGrp, iv->PaxList[0].cls_grp_id, true); //всегда на латинице - так надо
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
            pax_len = iv->size() + br.size();
            for(vector<string>::iterator j = iv + 1; j != body.end() and j->find('1') != 0; j++) {
                pax_len += j->size() + br.size();
            }
        } else
            pax_len = iv->size() + br.size();
        if(part_len + pax_len <= PART_SIZE)
            pax_len = iv->size() + br.size();
        part_len += pax_len;
        if(part_len > PART_SIZE) {
            SaveTlgOutPartTST(tlg_row);
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
    SaveTlgOutPartTST(tlg_row);
    return tlg_row.id;
}

int Unknown(TTlgInfo &info, bool &vcompleted, int tst_tlg_id)
{
    TTlgOutPartInfo tlg_row;
    vcompleted = false;
    tlg_row.id = tst_tlg_id;
    tlg_row.num = 1;
    tlg_row.tlg_type = info.tlg_type;
    tlg_row.point_id = info.point_id;
    tlg_row.pr_lat = info.pr_lat;
    tlg_row.addr = info.addrs;
    tlg_row.time_create = NowUTC();
    tlg_row.heading = '.' + info.sender + ' ' + DateTimeToStr(tlg_row.time_create,"ddhhnn") + br;
    tlg_row.extra = info.extra;
    SaveTlgOutPartTST(tlg_row);
    return tlg_row.id;
}

int TelegramInterface::create_tlg(
        const string      vtype,
        const int         vpoint_id,
        const string      vairp_trfer,
        const string      vcrs,
        const string      vextra,
        const bool        vpr_lat,
        const string      vaddrs,
        const int         tst_tlg_id
        )
{
    ProgTrace(TRACE5, "create_tlg entrance");
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
            "   point_id = :vpoint_id AND pr_del>=0";
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

    if(vbasic_type == "PTM") vid = PTM(info, tst_tlg_id);
    else if(vbasic_type == "BTM") vid = BTM(info, tst_tlg_id);
    else if(vbasic_type == "PRL") vid = PRL(info, tst_tlg_id);
    else if(vbasic_type == "COM") vid = COM(info, tst_tlg_id);
    else if(vbasic_type == "SOM") vid = SOM(info, tst_tlg_id);
    else vid = Unknown(info, vcompleted, tst_tlg_id);

    Qry.Clear();
    Qry.SQLText = "update tst_tlg_out set completed = :vcompleted where id = :vid";
    Qry.CreateVariable("vcompleted", otInteger, vcompleted);
    Qry.CreateVariable("vid", otInteger, vid);
    Qry.Execute();

    return vid;
}

void TelegramInterface::CreateTlg2(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode, int tst_tlg_id)
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
                addrs,
                tst_tlg_id //!!!
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
