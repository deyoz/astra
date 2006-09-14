#include "stat.h"
#define NICKNAME "DENIS"
#include "test.h"
#include "oralib.h"
#include "cache.h"
#include "xml_unit.h"
#include "exceptions.h"
#include "stl_utils.h"
#include <fstream>

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;

void StatInterface::LogRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery Qry(&OraSession);        
    Qry.SQLText =
        "SELECT msg, time, id1 AS point_id, null as screen, id2 AS reg_no, id3 AS grp_id, "
        "       ev_user, station, ev_order "
        "FROM astra.events "
        "WHERE type IN (:evtPax,:evtPay) AND "
        "      id1=:trip_id AND "
        "      (id2 IS NULL OR id2=:reg_no) AND "
        "      (id3 IS NULL OR id3=:grp_id) "
        "UNION "
        "SELECT msg, time, id1 AS point_id, null as screen, id2 AS reg_no, id3 AS grp_id, "
        "       ev_user, station, ev_order "
        "FROM arx.events "
        "WHERE part_key=:part_key AND "
        "      type IN (:evtPax,:evtPay) AND "
        "      id1=:trip_id AND "
        "      (id2 IS NULL OR id2=:reg_no) AND "
        "      (id3 IS NULL OR id3=:grp_id) ";
    TParams1 SQLParams;
    SQLParams.getParams(GetNode("sqlparams", reqNode));
    SQLParams.setSQL(&Qry);
    try {
        Qry.Execute();
    } catch (EOracleError E) {
        if(E.Code == 376)
            throw UserException(376);
        else
            throw;
    }
    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    xmlNodePtr PaxLogNode = NewTextChild(dataNode, "PaxLog");
    while(!Qry.Eof) {
        xmlNodePtr rowNode = NewTextChild(PaxLogNode, "row");

        NewTextChild(rowNode, "trip_id", Qry.FieldAsInteger("point_id"));
        NewTextChild(rowNode, "idf", Qry.FieldAsString("ev_user"));
        NewTextChild(rowNode, "station", Qry.FieldAsString("station"));
        NewTextChild(rowNode, "time", DateTimeToStr(Qry.FieldAsDateTime("time")));
        NewTextChild(rowNode, "grp_id", Qry.FieldAsInteger("grp_id"));
        NewTextChild(rowNode, "n_reg", Qry.FieldAsInteger("reg_no"));
        NewTextChild(rowNode, "txt", Qry.FieldAsString("msg"));
        NewTextChild(rowNode, "ev_order", Qry.FieldAsInteger("ev_order"));
        NewTextChild(rowNode, "module", Qry.FieldAsString("screen"));

        Qry.Next();
    }
}

struct THallItem {
    int id;
    string name;
};

class THalls: public vector<THallItem> {
    public:
        void Init();
};

void THalls::Init()
{
    TQuery Qry(&OraSession);        
    Qry.SQLText = "SELECT id,name FROM astra.halls2,astra.options WHERE halls2.airp=options.cod ORDER BY id";
    Qry.Execute();
    while(!Qry.Eof) {
        THallItem hi;
        hi.id = Qry.FieldAsInteger("id");
        hi.name = Qry.FieldAsString("name");
        this->push_back(hi);
        Qry.Next();
    }
}

void StatInterface::PaxListRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    string qry =
        "SELECT "
        "  pax.pax_id,pax_grp.point_id, "
        "  pax_grp.grp_id, "
        "  trips.company AS airline,trips.flt_no,trips.suffix, "
        "  trips.scd, "
        "  pax.reg_no, "
        "  pax.surname,pax.name, "
        "  pax_grp.target, "
        "  astra.ckin.get_bagAmount(pax_grp.grp_id,pax.reg_no,rownum) AS bag_amount, "
        "  astra.ckin.get_bagWeight(pax_grp.grp_id,pax.reg_no,rownum) AS bag_weight, "
        "  astra.ckin.get_rkWeight(pax_grp.grp_id,pax.reg_no,rownum) AS rk_weight, "
        "  astra.ckin.get_birks(pax_grp.grp_id,pax.reg_no,0) AS tags, "
        "  astra.ckin.get_excess(pax_grp.grp_id,pax.reg_no) AS excess, "
        "  DECODE(pax.refuse,NULL,DECODE(pax_grp.pr_wl,0,DECODE(pax.pr_brd,0,'Зарег.','Посажен'),'ЛО'),'Аннул.') AS status, "
        "  pax_grp.class, "
        "  LPAD(pax.seat_no,3,'0')|| "
        "           DECODE(SIGN(1-pax.seats),-1,'+'||TO_CHAR(pax.seats-1),'') AS seat_no, "
        "  pax_grp.hall AS hall, "
        "  pax.document, "
        "  pax.ticket_no "
        "FROM astra.trips,astra.pax_grp,astra.pax "
        //--здесь дополнительная выборка grp_id по фамилии или бирке
        "WHERE trips.trip_id=pax_grp.point_id AND pax_grp.grp_id=pax.grp_id AND "
        //--дополнительное условие pax_grp.grp_id=...
        "      trips.scd>= :FirstDate AND trips.scd< :LastDate AND "
        "      (:trip IS NULL OR trips.trip= :trip) and "
        "      (:dest IS NULL OR pax_grp.target IN (SELECT cod FROM astra.place WHERE place.trip_id=trips.trip_id AND place.city=:dest)) "
        "UNION "
        "SELECT "
        "  pax.pax_id,pax_grp.point_id, "
        "  pax_grp.grp_id, "
        "  trips.company AS airline,trips.flt_no,trips.suffix, "
        "  trips.scd, "
        "  pax.reg_no, "
        "  pax.surname,pax.name, "
        "  pax_grp.target, "
        "  arx.ckin.get_bagAmount(pax_grp.part_key,pax_grp.grp_id,pax.reg_no,rownum) AS bag_amount, "
        "  arx.ckin.get_bagWeight(pax_grp.part_key,pax_grp.grp_id,pax.reg_no,rownum) AS bag_weight, "
        "  arx.ckin.get_rkWeight(pax_grp.part_key,pax_grp.grp_id,pax.reg_no,rownum) AS rk_weight, "
        "  arx.ckin.get_birks(pax_grp.part_key,pax_grp.grp_id,pax.reg_no) AS tags, "
        "  arx.ckin.get_excess(pax_grp.part_key,pax_grp.grp_id,pax.reg_no) AS excess, "
        "  DECODE(pax.refuse,NULL,DECODE(pax_grp.pr_wl,0,DECODE(pax.pr_brd,0,'Зарег.','Посажен'),'ЛО'),'Аннул.') AS status, "
        "  pax_grp.class, "
        "  LPAD(pax.seat_no,3,'0')|| "
        "           DECODE(SIGN(1-pax.seats),-1,'+'||TO_CHAR(pax.seats-1),'') AS seat_no, "
        "  pax_grp.hall AS hall, "
        "  pax.document, "
        "  pax.ticket_no "
        "FROM arx.trips,arx.pax_grp,arx.pax "
        //--здесь дополнительная выборка grp_id по фамилии или бирке
        "WHERE trips.part_key=pax_grp.part_key AND trips.trip_id=pax_grp.point_id AND pax_grp.part_key=pax.part_key AND pax_grp.grp_id=pax.grp_id AND "
        //--дополнительное условие pax_grp.grp_id=...
        "      trips.part_key>= :FirstDate AND trips.part_key< :LastDate AND "
        "      (:trip IS NULL OR trips.trip= :trip) and "
        "      (:dest IS NULL OR pax_grp.target IN (SELECT cod FROM arx.place WHERE place.part_key=trips.part_key AND place.trip_id=trips.trip_id AND place.city=:dest)) ";
    //--ORDER BY scd,trip,trip_id,n_reg
    THalls halls;
    halls.Init();
    TQuery Qry(&OraSession);        
    Qry.SQLText = qry;
    TParams1 SQLParams;
    SQLParams.getParams(GetNode("sqlparams", reqNode));
    SQLParams.setSQL(&Qry);
    try {
        Qry.Execute();
    } catch (EOracleError E) {
        if(E.Code == 376)
            throw UserException(376);
        else
            throw;
    }
    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    xmlNodePtr PaxesNode = NewTextChild(dataNode, "Paxes");
    while(!Qry.Eof) {
        xmlNodePtr rowNode = NewTextChild(PaxesNode, "row");

        string airline = Qry.FieldAsString("airline");
        int flt_no = Qry.FieldAsInteger("flt_no");
        string suffix = Qry.FieldAsString("suffix");

        NewTextChild(rowNode, "trip_id", Qry.FieldAsInteger("point_id"));
        NewTextChild(rowNode, "airline", airline);
        NewTextChild(rowNode, "flt_no", flt_no);
        NewTextChild(rowNode, "suffix", suffix);
        NewTextChild(rowNode, "trip", airline+IntToString(flt_no)+suffix);
        NewTextChild(rowNode, "scd", DateTimeToStr(Qry.FieldAsDateTime("scd")));
        NewTextChild(rowNode, "n_reg", Qry.FieldAsInteger("reg_no"));
        NewTextChild(rowNode, "family", (string)Qry.FieldAsString("surname")+" "+Qry.FieldAsString("name"));
        NewTextChild(rowNode, "bagAmount", Qry.FieldAsInteger("bag_amount"));
        NewTextChild(rowNode, "bagWeight", Qry.FieldAsInteger("bag_weight"));
        NewTextChild(rowNode, "rkWeight", Qry.FieldAsInteger("rk_weight"));
        NewTextChild(rowNode, "excess", Qry.FieldAsInteger("excess"));
        NewTextChild(rowNode, "grp_id", Qry.FieldAsInteger("grp_id"));
        NewTextChild(rowNode, "target", Qry.FieldAsString("target"));
        NewTextChild(rowNode, "tags", Qry.FieldAsString("tags"));
        NewTextChild(rowNode, "status", Qry.FieldAsString("status"));
        NewTextChild(rowNode, "class", Qry.FieldAsString("class"));
        NewTextChild(rowNode, "seat_no", Qry.FieldAsString("seat_no"));
        {
            string hall;
            if(!Qry.FieldIsNULL("hall")) {
                int hall_id = Qry.FieldAsInteger("hall");
                THalls::iterator ih = halls.begin();
                for(; ih != halls.end(); ih++) {
                    if(ih->id == hall_id) break;
                }
                if(ih == halls.end())
                    hall = IntToString(hall_id);
                else
                    hall = ih->name;
            }
            NewTextChild(rowNode, "hall", hall);
        }
        NewTextChild(rowNode, "document", Qry.FieldAsString("document"));
        NewTextChild(rowNode, "ticket_no", Qry.FieldAsString("ticket_no"));

        Qry.Next();
    }
}

struct TTagQryParts {
    string
        select,
        order_by,
        astra_select,
        arx_select,
        astra_from,
        arx_from,
        where,
        astra_group_by,
        arx_group_by,
        get_birks;
    TTagQryParts(int state);
    bool TagDetailCheck(int i);
    bool checked[5];
    private:
    bool cscd, cflt, ctarget, ctarget_trfer, ccls;
    string fgroup_by, fuselect;
};

bool TTagQryParts::TagDetailCheck(int i)
{
    if(i > 4 || i == 1)
        return true;
    else if(i == 2)
        return checked[1];
    else if(i == 3)
        return checked[2] || checked[3];
    else
        return checked[i];
}

TTagQryParts::TTagQryParts(int state)
{
    cscd = state & 16;
    cflt = state & 8;
    ctarget = state & 4;
    ctarget_trfer = state & 2;
    ccls = state & 1;

    checked[0] = cscd;
    checked[1] = cflt;
    checked[2] = ctarget;
    checked[3] = ctarget_trfer;
    checked[4] = ccls;

    get_birks = "trip_company, ";
    order_by = "trip_company, ";
    fuselect = "trips.company   trip_company, ";
    fgroup_by = "trips.company, ";

    if(cflt) {
        get_birks = get_birks +
            "trip_flt_no, " +
            "nvl(trip_suffix, ' '), ";
        order_by = order_by +
            "trip_flt_no, " +
            "trip_suffix, ";
        fuselect = fuselect +
            "trips.flt_no    trip_flt_no, " +
            "trips.suffix    trip_suffix, ";
        fgroup_by = fgroup_by +
            "trips.flt_no, " +
            "trips.suffix, ";
    } else
        get_birks = get_birks +
            "NULL, " +
            "NULL, ";


    if(cflt && ctarget_trfer) {
        get_birks = get_birks +
            "nvl(transfer_company, ' '), " +
            "nvl(transfer_flt_no, -1), " +
            "nvl(transfer_suffix, ' '), ";
        order_by = order_by +
            "transfer_company, " +
            "transfer_flt_no, " +
            "transfer_suffix, ";
        fuselect = fuselect +
            "lt.airline      transfer_company, " +
            "lt.flt_no       transfer_flt_no, " +
            "lt.suffix       transfer_suffix, ";
        fgroup_by = fgroup_by +
            "lt.airline, " +
            "lt.flt_no, " +
            "lt.suffix, ";
    } else
        get_birks = get_birks +
            "NULL, " +
            "NULL, " +
            "NULL, ";

    if(ctarget) {
        get_birks = get_birks + "target, ";
        order_by = order_by + "target, ";
        fuselect = fuselect + "pax_grp.target  target, ";
        fgroup_by = fgroup_by + "pax_grp.target, ";
    } else
        get_birks = get_birks + "NULL, ";

    if(ctarget_trfer) {
        get_birks = get_birks + "nvl(transfer_target, ' '), ";
        order_by = order_by + "transfer_target, ";
        fuselect = fuselect + "lt.airp_arv     transfer_target, ";
        fgroup_by = fgroup_by + "lt.airp_arv, ";
    } else
        get_birks = get_birks + "NULL, ";

    if(ccls) {
        get_birks = get_birks + "cls, ";
        order_by = order_by + "cls, ";
        fuselect = fuselect + "pax_grp.class   cls, ";
        fgroup_by = fgroup_by + "pax_grp.class, ";
    } else
        get_birks = get_birks + "NULL, ";

    if(cscd) {
        select = "scd, " + order_by;
        astra_select =
            "to_char(trips.scd, 'dd.mm.yy')   scd, " +
            fuselect;
        arx_select =
            "to_char(trips.part_key, 'dd.mm.yy')   scd, " +
            fuselect;
        astra_group_by =
            fgroup_by;
        arx_group_by =
            fgroup_by;
    } else {
        select = order_by;
        astra_select = fuselect;
        arx_select = fuselect;
        astra_group_by = fgroup_by;
        arx_group_by = fgroup_by;
    }

    if(ctarget_trfer) {
        astra_from =
            "( "
            "  SELECT transfer.grp_id, "
            "         airline,flt_no,suffix,airp_arv,subclass "
            "  FROM astra.transfer, "
            "      (SELECT grp_id,MAX(transfer_num) AS transfer_num "
            "       FROM astra.transfer WHERE transfer_num>0 GROUP BY grp_id) last_transfer "
            "  WHERE "
            "      transfer.grp_id=last_transfer.grp_id AND "
            "      transfer.transfer_num=last_transfer.transfer_num "
            ") lt, ";
        arx_from = 
            "( "
            "  SELECT transfer.grp_id, "
            "         airline,flt_no,suffix,airp_arv,subclass "
            "  FROM arx.transfer, "
            "      (SELECT grp_id,MAX(transfer_num) AS transfer_num "
            "       FROM arx.transfer WHERE transfer_num>0 GROUP BY grp_id) last_transfer "
            "  WHERE "
            "      transfer.part_key >= :FirstDate and "
            "      transfer.part_key < :LastDate and "
            "      transfer.grp_id=last_transfer.grp_id AND "
            "      transfer.transfer_num=last_transfer.transfer_num "
            ") lt, ";
        where = "pax_grp.grp_id = lt.grp_id(+) and ";
    }
}

const int BagTagColsSize = 8;

void StatInterface::BagTagStatRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TTagQryParts qry_parts(NodeAsInteger("BagTagCBState", reqNode));
    string qry = (string)
        "select "
        "  tscd, " +
        qry_parts.select +
        "  color, "
        "  arx.test.get_birks(tscd, " +
        qry_parts.get_birks +
        "    nvl(color, ' ') "
        "  ) tags "
        "from ( "
        "select "
        "  trips.scd tscd, " +
        qry_parts.astra_select +
        "  bag_tags.color  color "
        "from "
        "  astra.trips, "
        "  astra.pax_grp, " +
        qry_parts.astra_from +
        "  astra.bag_tags "
        "where "
        "  trips.scd >= :FirstDate and "
        "  trips.scd < :LastDate and "
        "  (:awk is null or trips.company = :awk) and "
        "  trips.trip_id = pax_grp.point_id and " +
        qry_parts.where +
        "  pax_grp.grp_id = bag_tags.grp_id "
        "group by "
        "  trips.scd, " +
        qry_parts.astra_group_by +
        "  bag_tags.color "
        "union  "
        "select "
        "  trips.part_key tscd, " +
        qry_parts.arx_select +
        "  bag_tags.color  color "
        "from "
        "  arx.trips, "
        "  arx.pax_grp, " +
        qry_parts.arx_from +
        "  arx.bag_tags "
        "where "
        "  trips.part_key >= :FirstDate and "
        "  trips.part_key < :LastDate and "
        "  (:awk is null or trips.company = :awk) and "
        "  trips.trip_id = pax_grp.point_id and "
        "  trips.part_key = pax_grp.part_key and " +
        qry_parts.where +
        "  pax_grp.part_key = bag_tags.part_key and "
        "  pax_grp.grp_id = bag_tags.grp_id "
        "group by "
        "  trips.part_key, " +
        qry_parts.arx_group_by +
        "  bag_tags.color "
        ") "
        "order by "
        "  tscd, " +
        qry_parts.order_by +
        "color ";

    TQuery Qry(&OraSession);        
    Qry.SQLText = qry;
    TParams1 SQLParams;
    SQLParams.getParams(GetNode("sqlparams", reqNode));
    SQLParams.setSQL(&Qry);
    Qry.Execute();

    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    xmlNodePtr BagTagGrdNode = NewTextChild(dataNode, "BagTagGrd");
    while(!Qry.Eof) {
        xmlNodePtr rowNode = NewTextChild(BagTagGrdNode, "row");
        for(int i = 0; i < BagTagColsSize - 3; i++) {
            bool checked = qry_parts.TagDetailCheck(i);
            if(checked && i == 0)
                NewTextChild(rowNode, "col", Qry.FieldAsString("scd"));
            if(checked && i == 1)
                NewTextChild(rowNode, "col", Qry.FieldAsString("trip_company"));
            if(checked && i == 2) {
                string trip_company = Qry.FieldAsString("trip_company");
                string trip_flt_no = IntToString(Qry.FieldAsInteger("trip_flt_no"));
                string trip_suffix = Qry.FieldAsString("trip_suffix");
                string transfer_company;
                string transfer_flt_no;
                string transfer_suffix;

                if(qry_parts.checked[3]) {
                    transfer_company = Qry.FieldAsString("transfer_company");
                    transfer_flt_no = IntToString(Qry.FieldAsInteger("transfer_flt_no"));
                    transfer_suffix = Qry.FieldAsString("transfer_suffix");
                }
                string col2 = trip_company + trip_flt_no + trip_suffix;
                if(transfer_company.size())
                    col2 += "-" + transfer_company + transfer_flt_no + transfer_suffix;
                NewTextChild(rowNode, "col", col2);
            }
            if(checked && i == 3) {
                string target, transfer_target, col3;
                if(qry_parts.checked[2])
                    target = Qry.FieldAsString("target");
                if(qry_parts.checked[3])
                    transfer_target = Qry.FieldAsString("transfer_target");
                if(target.size()) {
                    col3 = target;
                    if(transfer_target.size())
                        col3 += "-" + transfer_target;
                } else
                    col3 = transfer_target;
                NewTextChild(rowNode, "col", col3);
            }
            if(checked && i == 4)
                NewTextChild(rowNode, "col", Qry.FieldAsString("cls"));
        }
        NewTextChild(rowNode, "col", Qry.FieldAsString("color"));
        string tags = Qry.FieldAsString("tags");
        string::size_type pos = tags.find(":");
        NewTextChild(rowNode, "col", tags.substr(pos + 1));
        NewTextChild(rowNode, "col", tags.substr(0, pos));
        Qry.Next();
    }
}

void StatInterface::DepStatRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery Qry(&OraSession);        
    Qry.SQLText =
        "SELECT "
        "  0 AS priority, "
        "  halls2.name AS hall, "
        "  NVL(SUM(trips),0) AS trips, "
        "  NVL(SUM(pax),0) AS pax, "
        "  NVL(SUM(weight),0) AS weight, "
        "  NVL(SUM(unchecked),0) AS unchecked, "
        "  NVL(SUM(excess),0) AS excess "
        "FROM "
        " (SELECT     "
        "    0,stat.hall, "
        "    COUNT(DISTINCT trips.trip_id) AS trips, "
        "    SUM(f+c+y) AS pax, "
        "    SUM(weight) AS weight, "
        "    SUM(unchecked) AS unchecked, "
        "    SUM(stat.excess) AS excess "
        "  FROM astra.trips,astra.stat "
        "  WHERE trips.trip_id=stat.trip_id AND "
        "        trips.scd>= :FirstDate AND trips.scd< :LastDate AND "
        "        (:flt IS NULL OR trips.trip= :flt) AND "
        "        (:dest IS NULL OR stat.target IN (SELECT cod FROM astra.place WHERE place.trip_id=trips.trip_id AND place.city=:dest)) and "
        "        (:awk IS NULL OR trips.company = :awk) and "
        "        exists ( "
        "          select * from astra.pax_grp where "
        "            trips.trip_id = pax_grp.point_id(+) and "
        "            (:class is null or pax_grp.class = :class) "
        "        ) "
        "  GROUP BY stat.hall "
        "  UNION "
        "  SELECT "
        "    1,stat.hall, "
        "    COUNT(DISTINCT trips.trip_id) AS trips, "
        "    SUM(f+c+y) AS pax, "
        "    SUM(weight) AS weight, "
        "    SUM(unchecked) AS unchecked, "
        "    SUM(stat.excess) AS excess "
        "  FROM arx.trips,arx.stat "
        "  WHERE trips.part_key=stat.part_key AND trips.trip_id=stat.trip_id AND "
        "        stat.part_key>= :FirstDate AND stat.part_key< :LastDate AND "
        "        (:flt IS NULL OR trips.trip= :flt) AND "
        "        (:dest IS NULL OR stat.target IN (SELECT cod FROM arx.place WHERE place.part_key=trips.part_key AND place.trip_id=trips.trip_id AND place.city=:dest)) and "
        "        (:awk IS NULL OR trips.company = :awk) and "
        "        exists ( "
        "          select * from arx.pax_grp where "
        "            trips.trip_id = pax_grp.point_id(+) and "
        "            (:class is null or pax_grp.class = :class) "
        "        ) "
        "  GROUP BY stat.hall) stat,astra.halls2 "
        "WHERE stat.hall(+)=halls2.id "
        "GROUP BY halls2.name "
        "UNION "
        "SELECT "
        "  1 AS priority, "
        "  'Всего' AS hall, "
        "  NVL(SUM(trips),0) AS trips, "
        "  NVL(SUM(pax),0) AS pax, "
        "  NVL(SUM(weight),0) AS weight, "
        "  NVL(SUM(unchecked),0) AS unchecked, "
        "  NVL(SUM(excess),0) AS excess "
        "FROM "
        " (SELECT         "
        "    COUNT(DISTINCT trips.trip_id) AS trips, "
        "    SUM(f+c+y) AS pax, "
        "    SUM(weight) AS weight, "
        "    SUM(unchecked) AS unchecked, "
        "    SUM(stat.excess) AS excess "
        "  FROM astra.trips,astra.stat "
        "  WHERE trips.trip_id=stat.trip_id AND "
        "        trips.scd>= :FirstDate AND trips.scd< :LastDate AND "
        "        (:flt IS NULL OR trips.trip= :flt) AND "
        "        (:dest IS NULL OR stat.target IN (SELECT cod FROM astra.place WHERE place.trip_id=trips.trip_id AND place.city=:dest)) and "
        "        (:awk IS NULL OR trips.company = :awk) and "
        "        exists ( "
        "          select * from astra.pax_grp where "
        "            trips.trip_id = pax_grp.point_id(+) and "
        "            (:class is null or pax_grp.class = :class) "
        "        ) "
        "union "
        "  SELECT "
        "    COUNT(DISTINCT trips.trip_id) AS trips, "
        "    SUM(f+c+y) AS pax, "
        "    SUM(weight) AS weight, "
        "    SUM(unchecked) AS unchecked, "
        "    SUM(stat.excess) AS excess "
        "  FROM arx.trips,arx.stat "
        "  WHERE trips.part_key=stat.part_key AND trips.trip_id=stat.trip_id AND "
        "        stat.part_key>= :FirstDate AND stat.part_key< :LastDate AND "
        "        (:flt IS NULL OR trips.trip= :flt) AND "
        "        (:dest IS NULL OR stat.target IN (SELECT cod FROM arx.place WHERE place.part_key=trips.part_key AND place.trip_id=trips.trip_id AND place.city=:dest)) and "
        "        (:awk IS NULL OR trips.company = :awk) and "
        "        exists ( "
        "          select * from arx.pax_grp where "
        "            trips.trip_id = pax_grp.point_id(+) and "
        "            (:class is null or pax_grp.class = :class) "
        "        ) "
        ") stat "
        "ORDER BY priority,hall ";

    TParams1 SQLParams;

    xmlNodePtr paramsNode = GetNode("sqlparams", reqNode);
    TDateTime FirstDate = NodeAsDateTime("FirstDate", paramsNode);
    TDateTime LastDate = NodeAsDateTime("LastDate", paramsNode);
    SQLParams.getParams(paramsNode);

    SQLParams.setSQL(&Qry);
    try {
        Qry.Execute();
    } catch (EOracleError E) {
        if(E.Code == 376)
            throw UserException(376);
        else
            throw;
    }

    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    xmlNodePtr DepStatNode = NewTextChild(dataNode, "DepStat");
    while(!Qry.Eof) {
        int pax = Qry.FieldAsInteger("pax");
        xmlNodePtr rowNode = NewTextChild(DepStatNode, "row");
        NewTextChild(rowNode, "hall", Qry.FieldAsString("hall"));
        NewTextChild(rowNode, "trips", Qry.FieldAsInteger("trips"));
        NewTextChild(rowNode, "pax", pax);
        NewTextChild(rowNode, "weight", Qry.FieldAsInteger("weight"));
        NewTextChild(rowNode, "unchecked", Qry.FieldAsInteger("unchecked"));
        NewTextChild(rowNode, "excess", Qry.FieldAsInteger("excess"));
        char avg[50];
        sprintf(avg, "%.2f", pax/(LastDate - FirstDate));
        NewTextChild(rowNode, "avg", avg);
        Qry.Next();
    }
}

void StatInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

}
