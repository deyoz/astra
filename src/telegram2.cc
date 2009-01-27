#include <map>
#include <set>
#include "telegram.h"
#include "xml_unit.h"
#include "tlg/tlg.h"
#include "astra_utils.h"
#include "stl_utils.h"
#include "convert.h"
#include "serverlib/logger.h"

#define NICKNAME "DEN"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace boost::local_time;
using namespace ASTRA;

const string br = "\xa";
const size_t PART_SIZE = 2000;
const size_t LINE_SIZE = 64;
int TST_TLG_ID; // for test purposes

void get_seat_list(int pax_id, ASTRA::TCompLayerType layer, TTlgSeatList &seat_list)
{
    seat_list.Clear();
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select distinct "
        "   trip_comp_elems.xname, "
        "   trip_comp_elems.yname "
        "from "
        "   trip_comp_layers, "
        "   trip_comp_ranges, "
        "   trip_comp_elems "
        "where "
        "   trip_comp_layers.range_id = trip_comp_ranges.range_id and "
        "   trip_comp_layers.point_id = trip_comp_ranges.point_id and "
        "   trip_comp_elems.point_id = trip_comp_ranges.point_id and "
        "   trip_comp_elems.num = trip_comp_ranges.num and "
        "   trip_comp_elems.x = trip_comp_ranges.x and "
        "   trip_comp_elems.y = trip_comp_ranges.y and "
        "   trip_comp_layers.layer_type = :ckin_layer and "
        "   trip_comp_layers.pax_id = :pax_id ";
    Qry.CreateVariable("ckin_layer", otString, EncodeCompLayerType(layer));
    Qry.CreateVariable("pax_id", otInteger, pax_id);
    Qry.Execute();
    if(!Qry.Eof)
        for(; !Qry.Eof; Qry.Next())
            seat_list.add_seat(Qry.FieldAsString("xname"), Qry.FieldAsString("yname"));
}

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

  if (info.id<0) {
      info.pr_tst = false;
  }

  if(info.pr_tst) {
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
  } else
      TelegramInterface::SaveTlgOutPart(info);
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
        addr = addr.substr(len + i);
    if(not(result.empty() or result.size() == 7))
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

struct TWItem {
    int bagAmount;
    int bagWeight;
    int rkWeight;
    void get(int grp_id);
    void ToTlg(vector<string> &body);
    TWItem():
        bagAmount(0),
        bagWeight(0),
        rkWeight(0)
    {};
};

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
            int col_crs_pax_id = Qry.FieldIndex("crs_pax_id");
            for(; !Qry.Eof; Qry.Next()) {
                TInfantsItem item;
                item.grp_id = Qry.FieldAsInteger(col_grp_id);
                item.surname = Qry.FieldAsString(col_surname);
                item.name = Qry.FieldAsString(col_name);
                if(!Qry.FieldIsNULL(col_crs_pax_id)) {
                    item.crs_pax_id = Qry.FieldAsInteger(col_crs_pax_id);
                    item.pax_id = item.crs_pax_id;
                }
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
                    if(k == 1 and infRow->pax_id != NoExists or
                            k > 1 and infRow->pax_id == NoExists) {
                        infRow->pax_id = NoExists;
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
        private:
            virtual void format_tag_no(ostringstream &line, const TTagItem &prev_item, const int num, TTlgInfo &info)=0;
        public:
            vector<TTagItem> items;
            void get(int grp_id);
            void ToTlg(TTlgInfo &info, vector<string> &body);
            virtual ~TTagList(){};
    };

    struct TPRLTagList:TTagList {
        private:
            void format_tag_no(ostringstream &line, const TTagItem &prev_item, const int num, TTlgInfo &info)
            {
                line
                    << ".N/" << fixed << setprecision(0) << setw(10) << setfill('0') << (prev_item.no - num + 1)
                    << setw(3) << setfill('0') << num
                    << '/' << ElemIdToElem(etAirp, prev_item.airp_arv, info.pr_lat);
            }
    };

    struct TBTMTagList:TTagList {
        private:
            void format_tag_no(ostringstream &line, const TTagItem &prev_item, const int num, TTlgInfo &info)
            {
                line
                    << ".N/" << fixed << setprecision(0) << setw(10) << setfill('0') << (prev_item.no - num + 1)
                    << setw(3) << setfill('0') << num;
            }
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
                format_tag_no(line, *prev_item, num, info);
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
        private:
            virtual string format(const TOnwardItem &item, const int i, TTlgInfo &info)=0;
        public:
            vector<TOnwardItem> items;
            void get(int grp_id);
            void ToTlg(TTlgInfo &info, vector<string> &body);
            virtual ~TOnwardList(){};
    };

    struct TBTMOnwardList:TOnwardList {
        private:
            string format(const TOnwardItem &item, const int i, TTlgInfo &info)
            {
                if(i == 1)
                    return "";
                ostringstream line;
                line
                    << ".O/"
                    << ElemIdToElem(etAirline, item.airline, info.pr_lat)
                    << setw(3) << setfill('0') << item.flt_no
                    << ElemIdToElem(etSuffix, item.suffix, info.pr_lat)
                    << '/'
                    << DateTimeToStr(item.scd, "ddmmm", info.pr_lat)
                    << '/'
                    << ElemIdToElem(etAirp, item.airp_arv, info.pr_lat);
                if(not item.subclass.empty())
                    line
                        << '/'
                        << ElemIdToElem(etSubcls, item.subclass, info.pr_lat);
                return line.str();
            }
    };

    struct TPRLOnwardList:TOnwardList {
        private:
            string format(const TOnwardItem &item, const int i, TTlgInfo &info)
            {
                ostringstream line;
                line << ".O";
                if(i > 1)
                    line << i;
                line
                    << '/'
                    << ElemIdToElem(etAirline, item.airline, info.pr_lat)
                    << setw(3) << setfill('0') << item.flt_no
                    << ElemIdToElem(etSuffix, item.suffix, info.pr_lat)
                    << ElemIdToElem(etSubcls, item.subclass, info.pr_lat)
                    << DateTimeToStr(item.scd, "dd", info.pr_lat)
                    << ElemIdToElem(etAirp, item.airp_arv, info.pr_lat);
                return line.str();
            }
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
        for(vector<TOnwardItem>::iterator iv = items.begin(); iv != items.end(); iv++, i++) {
            string line = format(*iv, i, info);
            if(not line.empty())
                body.push_back(line);
        }
    }

    struct TGRPItem {
        int pax_count;
        bool written;
        int bg;
        TWItem W;
        TPRLTagList tags;
        TPRLOnwardList onwards;
        TGRPItem() {
            pax_count = NoExists;
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
        if(not(grp_map.W.bagAmount == 0 and grp_map.W.bagWeight == 0 and grp_map.W.rkWeight == 0)) {
            ostringstream line;
            if(grp_map.pax_count > 1) {
                line.str("");
                line << ".BG/" << setw(3) << setfill('0') << grp_map.bg;
                body.push_back(line.str());
            }
            if(!grp_map.written) {
                grp_map.written = true;
                grp_map.W.ToTlg(body);
                grp_map.tags.ToTlg(info, body);
            }
        }
        grp_map.onwards.ToTlg(info, body);
    }

    void TGRPMap::get(int grp_id)
    {
        if(items.find(grp_id) != items.end()) return; // olready got
        TGRPItem item;
        item.W.get(grp_id);
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "select count(*) from pax where grp_id = :grp_id and refuse is null";
        Qry.CreateVariable("grp_id", otInteger, grp_id);
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
                rem = ".R/" + rem;
                while(rem.size() > LINE_SIZE) {
                    body.push_back(rem.substr(0, LINE_SIZE));
                    rem = ".RN/" + rem.substr(LINE_SIZE);
                }
                if(!rem.empty())
                    body.push_back(rem);
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
        TTlgSeatList seats;
        get_seat_list(pax.pax_id, cltCheckin, seats);
        string seat_list = seats.get_seat_list(info.pr_lat or info.pr_lat_seat);
        if(!seat_list.empty()) {
            TRemItem rem;
            rem.rem = "SEAT " + seat_list;
            items.push_back(rem);
        }
        Qry.Clear();
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
        Qry.CreateVariable("pax_id", otInteger, pax.pax_id);
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

void TWItem::ToTlg(vector<string> &body)
{
    ostringstream buf;
    buf << ".W/K/" << bagAmount << '/' << bagWeight;
    if(rkWeight != 0)
        buf << '/' << rkWeight;
    body.push_back(buf.str());
}

void TWItem::get(int grp_id)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT "
        "  NVL(ckin.get_bagAmount(:grp_id,NULL),0) bagAmount, "
        "  NVL(ckin.get_bagWeight(:grp_id,NULL),0) bagWeight, "
        "  NVL(ckin.get_rkWeight(:grp_id,NULL),0) rkWeight "
        "FROM dual ";
    Qry.CreateVariable("grp_id", otInteger, grp_id);
    Qry.Execute();
    bagAmount = Qry.FieldAsInteger("bagAmount");
    bagWeight = Qry.FieldAsInteger("bagWeight");
    rkWeight = Qry.FieldAsInteger("rkWeight");
}

struct TBTMGrpList;
// .F - блок информации для данного трансфера. Используется в BTM и PTM
// абстрактный класс
struct TFItem {
    int point_id_trfer;
    string airline;
    int flt_no;
    string suffix;
    TDateTime scd;
    string airp_arv;
    string subclass;
    virtual void ToTlg(TTlgInfo &info, vector<string> &body) = 0;
    virtual TBTMGrpList *get_grp_list() = 0;
    TFItem() {
        point_id_trfer = NoExists;
        flt_no = NoExists;
        scd = NoExists;
    }
    virtual ~TFItem() { };
};

struct TExtraSeatName {
    string value;
    int ord(string rem) {
        static const char *rems[] = {"STCR", "CBBG", "EXST"};
        static const int rems_size = sizeof(rems)/sizeof(rems[0]);
        int result = 0;
        for(; result < rems_size; result++)
            if(rem == rems[result])
                break;
        if(result == rems_size)
            throw Exception("TExtraSeatName:ord: rem %s not found in rems", rem.c_str());
        return result;
    }
    void get(int pax_id)
    {
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "select distinct "
            "   rem_code "
            "from "
            "   pax_rem "
            "where "
            "   pax_id = :pax_id and "
            "   rem_code in('STCR', 'CBBG', 'EXST')";
        Qry.CreateVariable("pax_id", otInteger, pax_id);
        Qry.Execute();
        if(Qry.Eof)
            value = "STCR";
        else
            for(; !Qry.Eof; Qry.Next()) {
                string tmp_value = Qry.FieldAsString("rem_code");
                if(value.empty() or ord(value) > ord(tmp_value))
                    value = tmp_value;
            }
    }
};

struct TPPax {
    public:
        int seats, grp_id;
        int pax_id;
        TPerson pers_type;
        bool unaccomp;
        string surname, name;
        TExtraSeatName exst;
        size_t name_length() const
        {
            size_t result = surname.size() + name.size();
            if(seats > 1)
                result += exst.value.size() * (seats - (name.empty() ? 0 :1));
            return result;
        }
        TPPax():
            seats(0),
            grp_id(NoExists),
            pax_id(NoExists),
            pers_type(NoPerson),
            unaccomp(false)
        {};
};

struct TPList {
    private:
        typedef vector<TPPax> TPSurname;
    public:
        map<string, TPSurname> surnames; // пассажиры сгруппированы по фамилии
        // этот оператор нужен для sort вектора TPSurname
        bool operator () (const TPPax &i, const TPPax &j)
        {
            return i.name_length() < j.name_length();
        };
        void get(TTlgInfo &info, int grp_id);
        void ToTlg(TTlgInfo &info, vector<string> &body); // used in BTM
        void ToTlg(TTlgInfo &info, vector<string> &body, TFItem &FItem); // used in PTM
};

struct TBTMGrpListItem {
    int grp_id;
    TBTMTagList NList;
    TWItem W;
    TBTMOnwardList OList;
    TPList PList;
    TBTMGrpListItem(): grp_id(NoExists) {};
};

struct TBTMGrpList {
    vector<TBTMGrpListItem> items;
    TBTMGrpListItem &get_grp_item(int grp_id)
    {
        vector<TBTMGrpListItem>::iterator iv = items.begin();
        for(; iv != items.end(); iv++) {
            if(iv->grp_id == grp_id)
                break;
        }
        if(iv == items.end())
            throw Exception("TBTMGrpList::get_grp_item: item not found, grp_id %d", grp_id);
        return *iv;
    }
    void get(TTlgInfo &info, TFItem &AFItem);
    virtual void ToTlg(TTlgInfo &info, vector<string> &body, TFItem &FItem);
    virtual ~TBTMGrpList() {};
};

// Представление списка полей .P/ как он будет в телеграмме.
// причем список этот будет представлять отдельную группу пассажиров
// объединенную по grp_id
struct TPLine {
    bool include_exst;
    bool print_bag;
    bool skip;
    int seats;
    size_t inf;
    size_t chd;
    int grp_id;
    string surname;
    vector<string> names;

    TPLine(bool ainclude_exst):
        include_exst(ainclude_exst),
        print_bag(false),
        skip(false),
        seats(0),
        inf(0),
        chd(0),
        grp_id(NoExists)
    {};
    size_t get_line_size() {
        return get_line().size() + br.size();
    }
    size_t get_line_size(TTlgInfo &info, TFItem &FItem) {
        return get_line(info, FItem).size() + br.size();
    }
    string get_line() {
        ostringstream buf;
        buf << ".P/";
        if(seats > 1)
            buf << seats;
        buf << surname;
        for(vector<string>::iterator iv = names.begin(); iv != names.end(); iv++) {
            if(!iv->empty())
                buf << "/" << *iv;
        }
        return buf.str();
    }
    string get_line(TTlgInfo &info, TFItem &FItem) {
        ostringstream result;
        result
            << ElemIdToElem(etAirline, FItem.airline, info.pr_lat)
            << setw(3) << setfill('0') << FItem.flt_no
            << ElemIdToElem(etSuffix, FItem.suffix, info.pr_lat)
            << "/"
            << DateTimeToStr(FItem.scd, "dd", info.pr_lat)
            << " "
            << ElemIdToElem(etAirp, FItem.airp_arv, info.pr_lat)
            << " "
            << seats
            << " "
            << ElemIdToElem(etSubcls, FItem.subclass, info.pr_lat)
            << " ";
        if(print_bag)
            result
                << FItem.get_grp_list()->get_grp_item(grp_id).W.bagAmount;
        else
            result
                << 0;
        result
            << "B"
            << " "
            << surname;
        for(vector<string>::iterator iv = names.begin(); iv != names.end(); iv++) {
            if(!iv->empty())
                result << "/" << *iv;
        }
        return result.str();
    }
    TPLine & operator += (const TPPax & pax)
    {
        if(grp_id == NoExists) {
            grp_id = pax.grp_id;
        } else {
            if(grp_id != pax.grp_id)
                throw Exception("TPLine operator +=: cannot add pax with different grp_id");
        }
        seats += pax.seats;
        switch(pax.pers_type) {
            case adult:
            case NoPerson:
                break;
            case child:
                chd++;
                break;
            case baby:
                inf++;
                break;
        }
        int name_count = 0;
        if(not pax.name.empty()) {
            names.push_back(pax.name);
            name_count = 1;
        }
        if(include_exst)
            for(int i = 0; i < pax.seats - name_count; i++)
                names.push_back(pax.exst.value);
        return *this;
    }
    TPLine operator + (const TPPax & pax)
    {
        TPLine result(*this);
        result += pax;
        return result;
    }
    TPLine & operator += (const int &aseats)
    {
        seats += aseats;
        return *this;
    }
    TPLine operator + (const int &aseats)
    {
        TPLine result(*this);
        result += aseats;
        return result;
    }
    bool operator == (const string &s)
    {
        return not skip and surname == s;
    }
};

void TPList::ToTlg(TTlgInfo &info, vector<string> &body, TFItem &FItem)
{
    for(map<string, TPSurname>::iterator im = surnames.begin(); im != surnames.end(); im++) {
        ProgTrace(TRACE5, "SURNAME: %s", im->first.c_str());
        vector<TPPax> one; // одно место
        vector<TPPax> many_noname; // без имени, больше одного места
        vector<TPPax> many_name; // с именем, больше одного места
        {
            TPSurname &pax_list = im->second;
            // Разложим список пассажиров на 3 подсписка
            for(vector<TPPax>::iterator iv = pax_list.begin(); iv != pax_list.end(); iv++) {
                if(iv->unaccomp) continue;
                if(iv->seats == 1) {
                    one.push_back(*iv);
                } else if(iv->name.empty()) {
                    many_noname.push_back(*iv);
                } else {
                    many_name.push_back(*iv);
                }
            }
        }
        bool print_bag = im == surnames.begin();
        if(not one.empty()) {
            // обработка one
            // Записываем в строку столько имен, сколько влезет, остальные обрубаем.
            sort(one.begin(), one.end(), *this);
            TPLine pax_line(true);
            pax_line.surname = im->first;
            for(vector<TPPax>::iterator iv = one.begin(); iv != one.end(); iv++) {
                pax_line += *iv;
            }
            if(pax_line.grp_id != NoExists) {
                pax_line.print_bag = print_bag;
                print_bag = false;
                string line = pax_line.get_line(info, FItem);
                if(line.size() + br.size() > LINE_SIZE) {
                    size_t start_pos = line.rfind(" "); // starting position of name element;
                    size_t oblique_pos = line.rfind("/", LINE_SIZE - 1);
                    if(oblique_pos < start_pos)
                        line = line.substr(0, LINE_SIZE - 1);
                    else
                        line = line.substr(0, oblique_pos);
                }
                body.push_back(line);
            }
        }
        if(not many_name.empty()) {
            // Обработка many_name. Записываем в строку сколько влезет.
            // Потом на след строку. Если и один не влезает, обрезаем имя до одного символа, затем фамилию.
            sort(many_name.begin(), many_name.end(), *this);
            TPLine pax_line(true);
            pax_line.print_bag = print_bag;
            print_bag = false;
            pax_line.surname = im->first;
            for(vector<TPPax>::iterator iv = many_name.begin(); iv != many_name.end(); iv++) {
                bool finished = false;
                while(not finished) {
                    finished = true;
                    TPLine tmp_pax_line = pax_line + *iv;
                    string line = tmp_pax_line.get_line(info, FItem);
                    if(line.size() + br.size() > LINE_SIZE) {
                        if(pax_line.grp_id == NoExists) {
                            // Один пассажир на всю строку не поместился
                            size_t diff = line.size() + br.size() - LINE_SIZE;
                            TPPax fix_pax = *iv;
                            if(fix_pax.name.size() > diff) {
                                fix_pax.name = fix_pax.name.substr(0, fix_pax.name.size() - diff);
                                body.push_back((pax_line + fix_pax).get_line(info, FItem));
                            } else {
                                diff -= fix_pax.name.size() - 1;
                                fix_pax.name = fix_pax.name.substr(0, 1);
                                if(fix_pax.surname.size() > diff) {
                                    pax_line.surname = fix_pax.surname.substr(0, fix_pax.surname.size() - diff);
                                    body.push_back((pax_line + fix_pax).get_line(info, FItem));
                                    pax_line.surname = im->first;
                                } else
                                    throw Exception("many_name item insertion failed");
                            }
                            pax_line.grp_id = NoExists;
                            pax_line.names.clear();
                            pax_line.seats = 0;
                            pax_line.print_bag = false;
                        } else {
                            // Пассажир не влез в строку к другим пассажирам
                            body.push_back((pax_line).get_line(info, FItem));
                            pax_line.grp_id = NoExists;
                            pax_line.names.clear();
                            pax_line.seats = 0;
                            pax_line.print_bag = false;
                            finished = false;
                        }
                    } else
                        pax_line += *iv;
                }
            }
            if(pax_line.grp_id != NoExists)
                body.push_back((pax_line).get_line(info, FItem));
        }
        if(not many_noname.empty()) {
            // Записываем каждого пассажира по отдельности
            // Если не влезает, обрезаем фамилию
            for(vector<TPPax>::iterator iv = many_noname.begin(); iv != many_noname.end(); iv++) {
                TPLine pax_line(true);
                pax_line.print_bag = print_bag;
                print_bag = false;
                pax_line.surname = im->first;
                string line = (pax_line + *iv).get_line(info, FItem);
                if(line.size() + br.size() > LINE_SIZE) {
                    size_t diff = line.size() + br.size() - LINE_SIZE;
                    if(pax_line.surname.size() > diff) {
                        pax_line.surname = pax_line.surname.substr(0, pax_line.surname.size() - diff);
                    } else
                        throw Exception("many_noname item insertion failed");
                    body.push_back((pax_line + *iv).get_line(info, FItem));
                } else
                    body.push_back(line);
            }
        }
    }
}

void TPList::ToTlg(TTlgInfo &info, vector<string> &body)
{
    vector<TPLine> lines;
    for(map<string, TPSurname>::iterator im = surnames.begin(); im != surnames.end(); im++) {
        TPSurname &pax_list = im->second;
        sort(pax_list.begin(), pax_list.end(), *this);
        TPLine line(false);
        line.surname = im->first;
        lines.push_back(line);
        vector<TPPax>::iterator iv = pax_list.begin();
        while(iv != pax_list.end()) {
            TPLine &curLine = lines.back();
            if((curLine + *iv).get_line_size() > LINE_SIZE) {// все, строка переполнена
                if(curLine.names.empty()) {
                    curLine += iv->seats;
                    iv++;
                } else {
                    lines.push_back(line);
                }
            } else {
                curLine += *iv;
                iv++;
            }
        }
    }

    // В полученном векторе строк, обрезаем слишком длинные
    // фамилии, объединяем между собой, если найдутся
    // совпадения обрезанных фамилий.
    for(vector<TPLine>::iterator iv = lines.begin(); iv != lines.end(); iv++) {
        TPLine &curLine = *iv;
        ProgTrace(TRACE5, "%s", curLine.surname.c_str());
        if(curLine.names.empty()) {
            size_t line_size = curLine.get_line_size();
            if(line_size > LINE_SIZE) {
                string surname = curLine.surname.substr(0, curLine.surname.size() - (line_size - LINE_SIZE));
                ProgTrace(TRACE5, "AFTER CUT: %s", surname.c_str());
                vector<TPLine>::iterator found_l = find(lines.begin(), lines.end(), surname);
                if(found_l != lines.end()) {
                    curLine.skip = true;
                    *found_l += curLine.seats;
                } else
                    curLine.surname = surname;
            }
        }
    }
    for(vector<TPLine>::iterator iv = lines.begin(); iv != lines.end(); iv++) {
        if(iv->skip) continue;
        body.push_back(iv->get_line());
    }
}

void TPList::get(TTlgInfo &info, int grp_id)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select \n"
        "   pax_id, \n"
        "   pr_brd, \n"
        "   seats, \n"
        "   surname, \n"
        "   pers_type, \n"
        "   name \n"
        "from \n"
        "   pax \n"
        "where \n"
        "  grp_id = :grp_id and \n"
        "  seats > 0 \n"
        "order by \n"
        "   surname, \n"
        "   name \n";
    Qry.CreateVariable("grp_id", otInteger, grp_id);
    Qry.Execute();
    if(Qry.Eof) {
        TPPax item;
        item.grp_id = grp_id;
        item.seats = 1;
        item.surname = "UNACCOMPANIED";
        item.unaccomp = true;
        surnames[item.surname].push_back(item);
    } else {
        int col_pax_id = Qry.FieldIndex("pax_id");
        int col_pr_brd = Qry.FieldIndex("pr_brd");
        int col_seats = Qry.FieldIndex("seats");
        int col_surname = Qry.FieldIndex("surname");
        int col_pers_type = Qry.FieldIndex("pers_type");
        int col_name = Qry.FieldIndex("name");
        for(; !Qry.Eof; Qry.Next()) {
            if(Qry.FieldAsInteger(col_pr_brd) == 0)
                continue;
            TPPax item;
            item.pax_id = Qry.FieldAsInteger(col_pax_id);
            item.grp_id = grp_id;
            item.seats = Qry.FieldAsInteger(col_seats);
            if(item.seats > 1)
                item.exst.get(Qry.FieldAsInteger(col_pax_id));
            item.surname = transliter(Qry.FieldAsString(col_surname), info.pr_lat);
            item.pers_type = DecodePerson(Qry.FieldAsString(col_pers_type));
            item.name = transliter(Qry.FieldAsString(col_name), info.pr_lat);
            surnames[item.surname].push_back(item);
        }
    }
}

struct TPTMGrpList:TBTMGrpList {
    void ToTlg(TTlgInfo &info, vector<string> &body, TFItem &FItem)
    {
        if(info.tlg_type == "PTM")
            for(vector<TBTMGrpListItem>::iterator iv = items.begin(); iv != items.end(); iv++) {
                iv->PList.ToTlg(info, body, FItem);
            }
    }
};

// в BTM структура FItem не используется
// пришлось передавать, чтобы ошибки компиляции не было
void TBTMGrpList::ToTlg(TTlgInfo &info, vector<string> &body, TFItem &AFItem)
{
    for(vector<TBTMGrpListItem>::iterator iv = items.begin(); iv != items.end(); iv++) {
        iv->NList.ToTlg(info, body);
        iv->W.ToTlg(body);
        iv->OList.ToTlg(info, body);
        iv->PList.ToTlg(info, body);
    }
}

void TBTMGrpList::get(TTlgInfo &info, TFItem &FItem)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select  \n"
        "   transfer.grp_id  \n"
        "from  \n"
        "   transfer, \n"
        "   pax_grp \n"
        "where  \n"
        "   transfer.point_id_trfer = :point_id_trfer and \n"
        "   nvl(transfer.subclass, ' ') = nvl(:subclass, ' ') and \n"
        "   transfer.grp_id = pax_grp.grp_id and \n"
        "   pax_grp.status <> 'T' and \n"
        "   pax_grp.point_dep = :point_id and \n"
        "   pax_grp.airp_arv = :airp_arv \n"
        "order by \n"
        "   grp_id \n";
    Qry.CreateVariable("point_id", otInteger, info.point_id);
    int fmt;
    Qry.CreateVariable("airp_arv", otString, ElemToElemId(etAirp, info.airp_arv, fmt));
    Qry.CreateVariable("point_id_trfer", otInteger, FItem.point_id_trfer);
    Qry.CreateVariable("subclass", otString, FItem.subclass);
    Qry.Execute();
    if(!Qry.Eof) {
        int col_grp_id = Qry.FieldIndex("grp_id");
        for(; !Qry.Eof; Qry.Next()) {
            TBTMGrpListItem item;
            item.grp_id = Qry.FieldAsInteger(col_grp_id);
            item.NList.get(item.grp_id);
            if(item.NList.items.empty())
                continue;
            item.PList.get(info, item.grp_id);
            if(item.PList.surnames.empty())
                continue;
            item.W.get(item.grp_id);
            item.OList.get(item.grp_id);
            items.push_back(item);
        }
    }
    ProgTrace(TRACE5, "items.size(): %d", items.size());
}

struct TBTMFItem:TFItem {
    TBTMGrpList grp_list;
    TBTMGrpList *get_grp_list() { return &grp_list; };
    void ToTlg(TTlgInfo &info, vector<string> &body)
    {
        ostringstream line;
        line
            << ".F/"
            << ElemIdToElem(etAirline, airline, info.pr_lat)
            << setw(3) << setfill('0') << flt_no
            << ElemIdToElem(etSuffix, suffix, info.pr_lat)
            << "/"
            << DateTimeToStr(scd, "ddmmm", info.pr_lat)
            << "/"
            << ElemIdToElem(etAirp, airp_arv, info.pr_lat)
            << "/"
            << ElemIdToElem(etSubcls, subclass, info.pr_lat);
        body.push_back(line.str());
    }
};

struct TPTMFItem:TFItem {
    TPTMGrpList grp_list;
    TBTMGrpList *get_grp_list() { return &grp_list; };
    void ToTlg(TTlgInfo &info, vector<string> &body)
    {
        if(info.tlg_type == "PTMN") {
            ostringstream result;
            result
                << ElemIdToElem(etAirline, airline, info.pr_lat)
                << setw(3) << setfill('0') << flt_no
                << ElemIdToElem(etSuffix, suffix, info.pr_lat)
                << "/"
                << DateTimeToStr(scd, "dd", info.pr_lat)
                << " "
                << ElemIdToElem(etAirp, airp_arv, info.pr_lat)
                << " ";
            int seats = 0;
            int baggage = 0;
            for(vector<TBTMGrpListItem>::iterator iv = grp_list.items.begin(); iv != grp_list.items.end(); iv++) {
                baggage += iv->W.bagAmount;
                map<string, vector<TPPax> > &surnames = iv->PList.surnames;
                for(map<string, vector<TPPax> >::iterator im = surnames.begin(); im != surnames.end(); im++) {
                    vector<TPPax> &paxes = im->second;
                    for(vector<TPPax>::iterator i_paxes = paxes.begin(); i_paxes != paxes.end(); i_paxes++)
                        seats += i_paxes->seats;
                }
            }
            result
                << seats
                << " "
                << ElemIdToElem(etSubcls, subclass, info.pr_lat)
                << " "
                << baggage
                << "B";
            body.push_back(result.str());
        }
    }
};

template <class T>
struct TFList { // Список направлений трансфера
    vector<T> items;
    void get(TTlgInfo &info);
    void ToTlg(TTlgInfo &info, vector<string> &body);
};

template <class T>
void TFList<T>::get(TTlgInfo &info)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select distinct \n"
        "    transfer.point_id_trfer, \n"
        "    trfer_trips.airline, \n"
        "    trfer_trips.flt_no, \n"
        "    trfer_trips.scd, \n"
        "    transfer.airp_arv, \n"
        "    transfer.subclass \n"
        "from \n"
        "    transfer, \n"
        "    trfer_trips \n"
        "where \n"
        "    transfer.grp_id in \n"
        "    (select grp_id from pax_grp where \n"
        "       point_dep = :point_id and \n"
        "       airp_arv = :airp \n"
        "    ) and \n"
        "    transfer.transfer_num = 1 and \n"
        "    transfer.point_id_trfer = trfer_trips.point_id \n"
        "order by "
        "    trfer_trips.airline, \n"
        "    trfer_trips.flt_no, \n"
        "    trfer_trips.scd, \n"
        "    transfer.airp_arv, \n"
        "    transfer.subclass \n";
    Qry.CreateVariable("point_id", otInteger, info.point_id);
    Qry.CreateVariable("airp", otString, info.airp);
    Qry.Execute();
    if(!Qry.Eof) {
        int col_point_id_trfer = Qry.FieldIndex("point_id_trfer");
        int col_airline = Qry.FieldIndex("airline");
        int col_flt_no = Qry.FieldIndex("flt_no");
        int col_scd = Qry.FieldIndex("scd");
        int col_airp_arv = Qry.FieldIndex("airp_arv");
        int col_subclass = Qry.FieldIndex("subclass");
        for(; !Qry.Eof; Qry.Next()) {
            T item;
            item.point_id_trfer = Qry.FieldAsInteger(col_point_id_trfer);
            item.airline = Qry.FieldAsString(col_airline);
            item.flt_no = Qry.FieldAsInteger(col_flt_no);
            item.scd = Qry.FieldAsDateTime(col_scd);
            item.airp_arv = Qry.FieldAsString(col_airp_arv);
            item.subclass = Qry.FieldAsString(col_subclass);
            item.grp_list.get(info, item);
            items.push_back(item);
        }
    }
}

template <class T>
void TFList<T>::ToTlg(TTlgInfo &info, vector<string> &body)
{
//    for(vector<T>::iterator iv = items.begin(); iv != items.end(); iv++) { // почему ошибка компиляции ???
    for(size_t i = 0; i < items.size(); i++) {
        if(items[i].grp_list.items.empty()) continue;
        items[i].ToTlg(info, body);
        items[i].grp_list.ToTlg(info, body, items[i]);
    }
}

int calculate_btm_grp_len(const vector<string>::iterator &iv, const vector<string> &body)
{
    int result = 0;
    bool P_found = false;
    for(vector<string>::iterator j = iv; j != body.end(); j++) {
        if(not P_found and j->find(".P/") == 0)
            P_found = true;
        if(
                P_found and
                (j->find(".N") == 0 or j->find(".F") == 0)
          ) // Нашли новую группу
            break;
        result += j->size() + br.size();
    }
    return result;
}

int BTM(TTlgInfo &info, int tst_tlg_id)
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

    TFList<TBTMFItem> FList;
    FList.get(info);
    vector<string> body;
    FList.ToTlg(info, body);
    string part_begin;
    bool P_found = false;
    for(vector<string>::iterator iv = body.begin(); iv != body.end(); iv++) {
        if(iv->find(".F/") == 0)
            part_begin = *iv;
        if(not P_found and iv->find(".P/") == 0)
            P_found = true;
        int grp_len = 0;
        if(
                P_found and
                (iv->find(".N") == 0 or iv->find(".F") == 0)
          ) { // Нашли новую группу
            P_found = false;
            grp_len = calculate_btm_grp_len(iv, body);
        } else
              grp_len = iv->size() + br.size();
        if(part_len + grp_len <= PART_SIZE)
            grp_len = iv->size() + br.size();
        part_len += grp_len;
        if(part_len > PART_SIZE) {
            SaveTlgOutPartTST(tlg_row);
            tlg_row.heading = heading1.str() + "/PART" + IntToString(tlg_row.num) + br + heading2.str();
            tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + br;
            if(iv->find(".F") == 0)
                tlg_row.body = *iv + br;
            else
                tlg_row.body = part_begin + br + *iv + br;
            part_len = tlg_row.addr.size() + tlg_row.heading.size() +
                tlg_row.body.size() + tlg_row.ending.size();
        } else
            tlg_row.body += *iv + br;
    }

    ProgTrace(TRACE5, "body size: %d", tlg_row.body.size());
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
    TFList<TPTMFItem> FList;
    FList.get(info);
    vector<string> body;
    FList.ToTlg(info, body);
    if(body.empty())
        tlg_row.body = "NIL" + br;
    else
        for(vector<string>::iterator iv = body.begin(); iv != body.end(); iv++) {
            part_len += iv->size() + br.size();
            if(part_len > PART_SIZE) {
                SaveTlgOutPartTST(tlg_row);
                tlg_row.heading = heading.str() + "PART" + IntToString(tlg_row.num) + br;
                tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + br;
                tlg_row.body = *iv + br;
                part_len = tlg_row.addr.size() + tlg_row.heading.size() +
                    tlg_row.body.size() + tlg_row.ending.size();
            } else
                tlg_row.body += *iv + br;
        }
    tlg_row.ending = "ENDPTM" + br;
    SaveTlgOutPartTST(tlg_row);
    return tlg_row.id;
}

void TTlgPlace::dump()
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

void TSeatRectList::vert_pack()
{
    // Разделим список мест на список групп мест с одинаковыми горизонтальными интервалами
    vector<TSeatRectList> split;
    for(TSeatRectList::iterator iv = begin(); iv != end(); iv++) {
        vector<TSeatRectList>::iterator i_split = split.begin();
        for(; i_split != split.end(); i_split++) {
            TSeatRect &curr_split_seat = (*i_split)[0];
            if(
                    curr_split_seat.row1 == iv->row1 and
                    curr_split_seat.row2 == iv->row2
              ) {
                i_split->push_back(*iv);
                break;
            }
        }
        if(i_split == split.end()) {
            TSeatRectList sub_list;
            sub_list.push_back(*iv);
            split.push_back(sub_list);
        }
    }

    for(vector<TSeatRectList>::iterator i_split = split.begin(); i_split != split.end(); i_split++) {
        if(i_split->size() > 1) {
            TSeatRectList::iterator first = i_split->begin();
            TSeatRectList::iterator cur = i_split->begin() + 1;
            while(true) {
                TSeatRectList::iterator prev = cur - 1;
                if(cur != i_split->end() and prev_iata_line(cur->line1) == norm_iata_line(prev->line1)) {
                    if (
                            prev->row1 == cur->row1 and
                            prev->row2 == cur->row2
                       ) {
                        prev->del = true;
                    } else {
                        prev->line1 = first->line1;
                        first = cur;
                    }
                } else {
                    prev->line1 = first->line1;
                    first = cur;
                    if(cur == i_split->end())
                        break;
                }
                cur++;
            }
        }
    }

    clear();
    for(vector<TSeatRectList>::iterator i_split = split.begin(); i_split != split.end(); i_split++)
        for(TSeatRectList::iterator j = i_split->begin(); j != i_split->end(); j++) {
            if(j->del)
                continue;
            push_back(*j);
        }
}

void TSeatRectList::pack()
{
    // Разделим список мест на список групп мест с одинаковыми горизонтальными интервалами
    vector<TSeatRectList> split;
    for(TSeatRectList::iterator iv = begin(); iv != end(); iv++) {
        vector<TSeatRectList>::iterator i_split = split.begin();
        for(; i_split != split.end(); i_split++) {
            TSeatRect &curr_split_seat = (*i_split)[0];
            if(
                    curr_split_seat.line1 == iv->line1 and
                    curr_split_seat.line2 == iv->line2
              ) {
                i_split->push_back(*iv);
                break;
            }
        }
        if(i_split == split.end()) {
            TSeatRectList sub_list;
            sub_list.push_back(*iv);
            split.push_back(sub_list);
        }
    }

    for(vector<TSeatRectList>::iterator i_split = split.begin(); i_split != split.end(); i_split++) {
        if(i_split->size() > 1) {
            TSeatRectList::iterator first = i_split->begin();
            TSeatRectList::iterator cur = i_split->begin() + 1;
            while(true) {
                TSeatRectList::iterator prev = cur - 1;
                if(cur != i_split->end() and prev_iata_row(cur->row1) == norm_iata_row(prev->row1)) {
                    if (
                            prev->line1 == cur->line1 and
                            prev->line2 == cur->line2
                       ) {
                        prev->del = true;
                    } else {
                        prev->row1 = first->row1;
                        first = cur;
                    }
                } else {
                    prev->row1 = first->row1;
                    first = cur;
                    if(cur == i_split->end())
                        break;
                }
                cur++;
            }
        }
    }
    clear();
    // Записываем полученные области мест в результат.
    for(vector<TSeatRectList>::iterator i_split = split.begin(); i_split != split.end(); i_split++)
        for(TSeatRectList::iterator j = i_split->begin(); j != i_split->end(); j++) {
            if(j->del)
                continue;
            push_back(*j);
        }
}

string TSeatRectList::ToTlg()
{
    std::string result;
    // Записываем в result,
    // попутно ищем одиночные места, чтобы
    // попробовать записать их в краткой форме напр. 3ABД
    // Аккумулируем такие места в векторе alone
    vector<TSeatRectList> alone;
    for(TSeatRectList::iterator iv = begin(); iv != end(); iv++) {
        if(iv->del)
            continue;
        if(
                iv->row1 == iv->row2 and
                iv->line1 == iv->line2
          ) {
            vector<TSeatRectList>::iterator i_alone = alone.begin();
            for(; i_alone != alone.end(); i_alone++) {
                TSeatRect &curr_alone_seat = (*i_alone)[0];
                if(curr_alone_seat.row1 == iv->row1) {
                    i_alone->push_back(*iv);
                    break;
                }
            }
            if(i_alone == alone.end()) {
                TSeatRectList sub_list;
                sub_list.push_back(*iv);
                alone.push_back(sub_list);
            }
            iv->del = true;
        }
        else
        {
            if(!result.empty())
                result += " ";
            result += iv->str();
        }
    }
    for(vector<TSeatRectList>::iterator i_alone = alone.begin(); i_alone != alone.end(); i_alone++) {
        if(!result.empty())
            result += " ";
        if(i_alone->size() == 1) {
            result += (*i_alone)[0].str();
        } else {
            for(TSeatRectList::iterator sr = i_alone->begin(); sr != i_alone->end(); sr++) {
                if(sr == i_alone->begin())
                    result += sr->row1;
                result += sr->line1;
            }
        }
    }
    return result;
}

string TSeatRect::str()
{
    string result;
    if(row1 == row2) {
        if(line1 == line2)
            result = row1 + line1;
        else {
            if(line1 == prev_iata_line(line2))
                result = row1 + line1 + line2;
            else
                result = row1 + line1 + "-" + line2;
        }
    } else {
        if(line1 == line2)
            result = row1 + "-" + row2 + line1;
        else {
            result = row1 + "-" + row2 + line1 + "-" + line2;
        }
    }
    return result;
}

struct TSeatListContext {
    string first_xname, last_xname;
    void seat_to_str(TSeatRectList &SeatRectList, string yname, string first_place,  string last_place, bool pr_lat);
    void vert_seat_to_str(TSeatRectList &SeatRectList, string yname, string first_place,  string last_place, bool pr_lat);
};

void TTlgSeatList::add_seat(int point_id, string xname, string yname)
{
    if(is_iata_row(yname) && is_iata_line(xname)) {
        TTlgPlace place;
        place.xname = norm_iata_line(xname);
        place.yname = norm_iata_row(yname);
        place.point_arv = point_id;
        comp[place.yname][place.xname] = place;
    }
}

void TTlgSeatList::dump_list(std::map<int, TSeatRectList> &list)
{
    for(std::map<int, TSeatRectList>::iterator im = list.begin(); im != list.end(); im++) {
        string result;
        TSeatRectList &SeatRectList = im->second;
        for(TSeatRectList::iterator iv = SeatRectList.begin(); iv != SeatRectList.end(); iv++) {
            if(iv->del) continue;
            result += iv->str() + " ";
        }
        ProgTrace(TRACE5, "point_arv: %d; SeatRectList: %s", im->first, result.c_str());
    }
}

void TTlgSeatList::dump_list(map<int, string> &list)
{
    for(map<int, string>::iterator im = list.begin(); im != list.end(); im++) {
        ProgTrace(TRACE5, "point_arv: %d; seats: %s", im->first, (/*convert_seat_no(*/im->second/*, 1)*/).c_str());
    }
}

string  TTlgSeatList::get_seat_one(bool pr_lat)
{
    string result;
    if(!comp.empty()) {
        t_tlg_comp::iterator ay = comp.begin();
        t_tlg_row::iterator ax = ay->second.begin();
        result = denorm_iata_row(ay->first) + denorm_iata_line(ax->first, pr_lat);
    }
    return result;
}

string  TTlgSeatList::get_seat_list(bool pr_lat)
{
    map<int, string> list;
    get_seat_list(list, pr_lat);
    if(list.size() > 1)
        throw Exception("TTlgSeatList::get_seat_list(): wrong map size %d", list.size());
    return list[0];
}

int TTlgSeatList::get_list_size(std::map<int, std::string> &list)
{
    int result = 0;
    for(map<int, std::string>::iterator im = list.begin(); im != list.end(); im++)
        result += im->second.size();
    return result;
}

void TTlgSeatList::get_seat_list(map<int, string> &list, bool pr_lat)
{
    list.clear();
    map<int, TSeatRectList> hrz_list, vert_list;
    // Пробег карты мест по горизонтали
    // определение минимальной и максимальной координаты линии в которых есть
    // занятые места (используются для последующего вертикального пробега)
    string min_col, max_col;
    dump_comp();
    for(t_tlg_comp::iterator ay = comp.begin(); ay != comp.end(); ay++) {
        map<int, TSeatListContext> ctxt;
        string *first_xname = NULL;
        string *last_xname = NULL;
        TSeatRectList *SeatRectList = NULL;
        TSeatListContext *cur_ctxt = NULL;
        t_tlg_row &row = ay->second;
        if(min_col.empty() or less_iata_line(row.begin()->first, min_col))
                min_col = row.begin()->first;
        if(max_col.empty() or not less_iata_line(row.rbegin()->first, max_col))
                max_col = row.rbegin()->first;
        for(t_tlg_row::iterator ax = row.begin(); ax != row.end(); ax++) {
            ProgTrace(TRACE5, "ax->second.point_arv: %d", ax->second.point_arv);
            cur_ctxt = &ctxt[ax->second.point_arv];
            first_xname = &cur_ctxt->first_xname;
            last_xname = &cur_ctxt->last_xname;
            ProgTrace(TRACE5, "first_xname: %s", first_xname->c_str());
            ProgTrace(TRACE5, "last_xname: %s", last_xname->c_str());
            SeatRectList = &hrz_list[ax->second.point_arv];
            if(first_xname->empty()) {
                *first_xname = ax->first;
                *last_xname = *first_xname;
            } else {
                if(prev_iata_line(ax->first) == *last_xname)
                    *last_xname = ax->first;
                else {
                    cur_ctxt->seat_to_str(*SeatRectList, ax->second.yname, *first_xname, *last_xname, pr_lat);
                    *first_xname = ax->first;
                    *last_xname = *first_xname;
                }
            }
        }
        // Дописываем последние оставшиеся места в ряду для каждого направления
        // Put last row seats for each dest hrz_list
        for(map<int, TSeatRectList>::iterator im = hrz_list.begin(); im != hrz_list.end(); im++) {
            cur_ctxt = &ctxt[im->first];
            first_xname = &cur_ctxt->first_xname;
            last_xname = &cur_ctxt->last_xname;
            SeatRectList = &im->second;
            if(first_xname != NULL and !first_xname->empty())
                cur_ctxt->seat_to_str(*SeatRectList, ay->first, *first_xname, *last_xname, pr_lat);
        }
    }

    // Пробег карты мест по вертикали
    string i_col = min_col;
    while(true) {
        map<int, TSeatListContext> ctxt;
        string *first_xname = NULL;
        string *last_xname = NULL;
        TSeatRectList *SeatRectList = NULL;
        TSeatListContext *cur_ctxt = NULL;
        for(t_tlg_comp::iterator i_comp = comp.begin(); i_comp != comp.end(); i_comp++) {
            t_tlg_row &row = i_comp->second;
            t_tlg_row::iterator col_pos = row.find(i_col);
            if(col_pos != row.end()) {
                cur_ctxt = &ctxt[col_pos->second.point_arv];
                first_xname = &cur_ctxt->first_xname;
                last_xname = &cur_ctxt->last_xname;
                SeatRectList = &vert_list[col_pos->second.point_arv];
                if(first_xname->empty()) {
                    *first_xname = col_pos->second.yname;
                    *last_xname = *first_xname;
                } else {
                    if(prev_iata_row(col_pos->second.yname) == *last_xname)
                        *last_xname = col_pos->second.yname;
                    else {
                        cur_ctxt->vert_seat_to_str(*SeatRectList, col_pos->first, *first_xname, *last_xname, pr_lat);
                        *first_xname = col_pos->second.yname;
                        *last_xname = *first_xname;
                    }
                }
            }
        }
        // Дописываем последние оставшиеся места в ряду для каждого направления
        // Put last row seats for each dest vert_list
        for(map<int, TSeatRectList>::iterator im = vert_list.begin(); im != vert_list.end(); im++) {
            cur_ctxt = &ctxt[im->first];
            first_xname = &cur_ctxt->first_xname;
            last_xname = &cur_ctxt->last_xname;
            SeatRectList = &im->second;
            if(first_xname != NULL and !first_xname->empty())
                cur_ctxt->vert_seat_to_str(*SeatRectList, i_col, *first_xname, *last_xname, pr_lat);
        }
        if(i_col == max_col)
            break;
        i_col = next_iata_line(i_col);
    }
    map<int, TSeatRectList>::iterator i_hrz = hrz_list.begin();
    map<int, TSeatRectList>::iterator i_vert = vert_list.begin();
    while(true) {
        if(i_hrz == hrz_list.end())
            break;
        i_hrz->second.pack();
        i_vert->second.vert_pack();
        string hrz_result = i_hrz->second.ToTlg();
        string vert_result = i_vert->second.ToTlg();
        if(hrz_result.size() > vert_result.size())
            list[i_vert->first] = vert_result;
        else
            list[i_hrz->first] = hrz_result;
        i_hrz++;
        i_vert++;
    }
}

void TSeatListContext::vert_seat_to_str(TSeatRectList &SeatRectList, string yname, string first_xname, string last_xname, bool pr_lat)
{
    yname = denorm_iata_line(yname, pr_lat);
    first_xname = denorm_iata_row(first_xname);
    last_xname = denorm_iata_row(last_xname);
    TSeatRect rect;
    rect.row1 = first_xname;
    rect.line1 = yname;
    rect.line2 = yname;
    if(first_xname == last_xname)
        rect.row2 = first_xname;
    else
        rect.row2 = last_xname;
    SeatRectList.push_back(rect);
}

void TSeatListContext::seat_to_str(TSeatRectList &SeatRectList, string yname, string first_xname, string last_xname, bool pr_lat)
{
    yname = denorm_iata_row(yname);
    first_xname = denorm_iata_line(first_xname, pr_lat);
    last_xname = denorm_iata_line(last_xname, pr_lat);
    TSeatRect rect;
    rect.row1 = yname;
    rect.row2 = yname;
    rect.line1 = first_xname;
    if(first_xname == last_xname)
        rect.line2 = first_xname;
    else
        rect.line2 = last_xname;
    SeatRectList.push_back(rect);
}

void TTlgSeatList::dump_comp()
{
    for(t_tlg_comp::iterator ay = comp.begin(); ay != comp.end(); ay++)
        for(t_tlg_row::iterator ax = ay->second.begin(); ax != ay->second.end(); ax++) {
            ostringstream buf;
            buf
                << "yname: " << ay->first << "; "
                << "xname: " << ax->first << "; ";
            if(!ax->second.yname.empty())
                buf << ax->second.yname << ax->second.xname << " " << ax->second.point_arv;
            ProgTrace(TRACE5, "%s", buf.str().c_str());
        }
}

void TTlgSeatList::apply_comp(TTlgInfo &info)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select distinct "
        "   trip_comp_layers.point_arv, "
        "   trip_comp_elems.xname, "
        "   trip_comp_elems.yname "
        "from "
        "   trip_comp_layers, "
        "   trip_comp_ranges, "
        "   trip_comp_elems "
        "where "
        "   trip_comp_layers.range_id = trip_comp_ranges.range_id and "
        "   trip_comp_layers.point_id = trip_comp_ranges.point_id and "
        "   trip_comp_elems.point_id = trip_comp_ranges.point_id and "
        "   trip_comp_elems.num = trip_comp_ranges.num and "
        "   trip_comp_elems.x = trip_comp_ranges.x and "
        "   trip_comp_elems.y = trip_comp_ranges.y and "
        "   trip_comp_layers.layer_type = :ckin_layer and "
        "   trip_comp_layers.point_id = :point_id ";
    Qry.CreateVariable("ckin_layer", otString, EncodeCompLayerType(cltCheckin));
    Qry.CreateVariable("point_id", otInteger, info.point_id);
    Qry.Execute();
    if(!Qry.Eof) {
        int col_point_arv = Qry.FieldIndex("point_arv");
        int col_xname = Qry.FieldIndex("xname");
        int col_yname = Qry.FieldIndex("yname");
        for(; !Qry.Eof; Qry.Next()) {
            add_seat(
                    Qry.FieldAsInteger(col_point_arv),
                    Qry.FieldAsString(col_xname),
                    Qry.FieldAsString(col_yname)
                    );
        }
    }
}

void TTlgSeatList::get(TTlgInfo &info)
{
    apply_comp(info);
    map<int, string> list;
    get_seat_list(list, (info.pr_lat or info.pr_lat_seat));
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
        else {
            item += /*convert_seat_no(*/list[point_id]/*, info.pr_lat)*/;
            while(item.size() + 1 > LINE_SIZE) {
                size_t pos = item.rfind(' ', LINE_SIZE - 2);
                items.push_back(item.substr(0, pos));
                item = item.substr(pos + 1);
            }
        }
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
    TTlgSeatList SOMList;
    SOMList.get(info);
    for(vector<string>::iterator iv = SOMList.items.begin(); iv != SOMList.items.end(); iv++) {
        part_len += iv->size() + br.size();
        if(part_len > PART_SIZE) {
            SaveTlgOutPartTST(tlg_row);
            tlg_row.heading = heading.str() + "PART" + IntToString(tlg_row.num) + br;
            tlg_row.ending = "ENDPART" + IntToString(tlg_row.num) + br;
            tlg_row.body = *iv + br;
            part_len = tlg_row.addr.size() + tlg_row.heading.size() +
                tlg_row.body.size() + tlg_row.ending.size();
        } else
            tlg_row.body += *iv + br;
    }
    tlg_row.ending = "ENDSOM" + br;
    SaveTlgOutPartTST(tlg_row);
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
                << "00" << TlgElemIdToElem(etSubcls, iv->cls, true); //всегда на латинице - так надо
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
    ProgTrace(TRACE5, "create_tlg entrance: %s", (tst_tlg_id < 0 ? "real_mode" : "test_mode"));
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
            "   points.airline, "
            "   points.flt_no, "
            "   points.suffix, "
            "   points.scd_out scd, "
            "   points.act_out, "
            "   points.craft, "
            "   points.bort, "
            "   points.airp, "
            "   points.point_num, "
            "   DECODE(points.pr_tranzit,0,points.point_id,points.first_point) first_point, "
            "   nvl(trip_sets.pr_lat_seat, 1) pr_lat_seat "
            "from "
            "   points, "
            "   trip_sets "
            "where "
            "   points.point_id = :vpoint_id AND points.pr_del>=0 and "
            "   points.point_id = trip_sets.point_id(+) ";
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
        info.pr_lat_seat = Qry.FieldAsInteger("pr_lat_seat") != 0;

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
    if(tst_tlg_id < 0) // real_mode
        Qry.SQLText = "update tlg_out set completed = :vcompleted where id = :vid";
    else
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
