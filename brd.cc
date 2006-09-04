#include "brd.h"
#include "xml_unit.h"
#include "exceptions.h"
#define NICKNAME "DENIS"
#include "test.h"
#include "oralib.h"
#include "cache.h"
#include "astra_utils.h"


using namespace EXCEPTIONS;
using namespace std;

int get_new_tid()
{
    TQuery *Qry = OraSession.CreateQuery();
    try {
        Qry->SQLText =
            "select "
            "    tid__seq.nextval tid "
            "from "
            "    trips "
            "where "
            "    trip_id=:trip_id "
            "for update";
        Qry->DeclareVariable("trip_id", otInteger);
        Qry->SetVariable("trip_id", JxtContext::getJxtContHandler()->currContext()->readInt("TRIP_ID"));
        Qry->Execute();
        if(Qry->Eof) throw UserException("Рейс не найден. Обновите данные");
        int result = Qry->FieldAsInteger("tid");
        OraSession.DeleteQuery(*Qry);
        return result;
    } catch(...) {
        OraSession.DeleteQuery(*Qry);
        throw;
    }
}

void BrdInterface::Deplane(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery *Qry = OraSession.CreateQuery();
    try {
        get_new_tid();
        Qry->SQLText =
            "declare "
            "   cursor cur is "
            "       select pax_id from pax_grp,pax where pax_grp.grp_id=pax.grp_id and point_id=:trip_id and pr_brd=1; "
            "   currow       cur%rowtype; "
            "begin "
            "   for currow in cur loop "
            "       update pax set pr_brd=0,tid=tid__seq.currval where pax_id=currow.pax_id and pr_brd=1; "
            "       mvd.sync_pax(currow.pax_id,:term); "
            "   end loop; "
            "end; ";
        Qry->DeclareVariable("trip_id", otInteger);
        Qry->DeclareVariable("term", otString);
        int trip_id = JxtContext::getJxtContHandler()->currContext()->readInt("TRIP_ID");
        Qry->SetVariable("trip_id", trip_id);
        Qry->SetVariable("term", JxtContext::getJxtContHandler()->currContext()->read("STATION"));
        Qry->Execute();        
        TReqInfo::Instance()->MsgToLog("Все пассажиры высажены", evtPax, trip_id);
        OraSession.DeleteQuery(*Qry);
    } catch(...) {
        OraSession.DeleteQuery(*Qry);
        throw;
    }
}

int BrdInterface::PaxUpdate(int pax_id, int &tid, int pr_brd)
{
    TQuery *Qry = OraSession.CreateQuery();
    int new_tid = get_new_tid();
    Qry->SQLText =
        "begin "
        "   select "
        "       p.name, "
        "       p.surname, "
        "       p.reg_no, "
        "       p.grp_id, "
        "       pg.point_id "
        "   into "
        "       :name, "
        "       :surname, "
        "       :regno, "
        "       :grp_id, "
        "       :point_id "
        "   from "
        "       pax p, "
        "       pax_grp pg "
        "   where "
        "       p.pax_id = :pax_id and "
        "       pg.grp_id=p.grp_id; "
        "   UPDATE pax SET "
        "       pr_brd=:pr_brd, "
        "       tid=tid__seq.currval "
        "   WHERE "
        "       pax_id= :pax_id AND "
        "       tid=:tid; "
        "   if SQL%ROWCOUNT > 0 then "
        "       mvd.sync_pax(:pax_id, :term); "
        "   else "
        "       :pr_brd := -1; "
        "   end if; "
        "end;";

    Qry->DeclareVariable("term", otString);
    Qry->DeclareVariable("pax_id", otInteger);
    Qry->DeclareVariable("tid", otInteger);
    Qry->DeclareVariable("pr_brd", otInteger);
    Qry->DeclareVariable("name", otString);
    Qry->DeclareVariable("surname", otString);
    Qry->DeclareVariable("point_id", otInteger);
    Qry->DeclareVariable("regno", otInteger);
    Qry->DeclareVariable("grp_id", otInteger);

    Qry->SetVariable("term", JxtContext::getJxtContHandler()->currContext()->read("STATION"));
    Qry->SetVariable("pax_id", pax_id);
    Qry->SetVariable("tid", tid);
    Qry->SetVariable("pr_brd", pr_brd);

    Qry->Execute();

    pr_brd = Qry->GetVariableAsInteger("pr_brd");
    tid = new_tid;

    if(pr_brd >= 0) {
        /*
        throw UserException((string)
                Qry->GetVariableAsString("point_id") + " " +
                Qry->GetVariableAsString("regno") + " " +
                Qry->GetVariableAsString("pr_brd") + " " +
                Qry->GetVariableAsString("grp_id")
                );
                */
        string msg = (string)
                "Пассажир " + Qry->GetVariableAsString("surname") + " " +
                Qry->GetVariableAsString("name") +
                (pr_brd ? " прошел посадку" : " высажен");
        TReqInfo::Instance()->MsgToLog(msg, evtPax,
                                       Qry->GetVariableAsInteger("point_id"),
                                       Qry->GetVariableAsInteger("regno"),
                                       Qry->GetVariableAsInteger("grp_id"));
    }
    OraSession.DeleteQuery(*Qry);
    return pr_brd;
}

void BrdInterface::PaxUpd(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int pax_id = NodeAsInteger("pax_id", reqNode);
    int tid = NodeAsInteger("tid", reqNode);
    int pr_brd = NodeAsInteger("pr_brd", reqNode);
    pr_brd = PaxUpdate(pax_id, tid, pr_brd);

    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    if(pr_brd < 0) SetCounters(dataNode);
    NewTextChild(dataNode, "pr_brd", pr_brd);
    NewTextChild(dataNode, "new_tid", tid);
}

bool ChckSt(int pax_id, string seat_no)
{
    bool Result = true;
    TQuery *Qry = OraSession.CreateQuery();
    Qry->SQLText =
        "SELECT seat_no FROM bp_print, "
        "  (SELECT MAX(time_print) AS time_print FROM bp_print WHERE pax_id=:pax_id) a "
        "WHERE pax_id=:pax_id AND bp_print.time_print=a.time_print ";
    Qry->DeclareVariable("pax_id", otInteger);
    Qry->SetVariable("pax_id", pax_id);
    Qry->Execute();
    while(!Qry->Eof) {
        if(seat_no == Qry->FieldAsString("seat_no"))
            break;
        Qry->Next();
    }
    if(!Qry->Eof)
        Result = false;
    OraSession.DeleteQuery(*Qry);
    return Result;
}

void BrdInterface::CheckSeat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int pax_id = NodeAsInteger("sqlparams/pax_id", reqNode);
    string seat_no = NodeAsString("seat_no", reqNode);
    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    if(!ChckSt(pax_id, seat_no))
        NewTextChild(dataNode, "failed");
}


struct TPax {
    int pax_id;
    int grp_id;
    int point_id;
    int pr_brd;
    int old_pr_brd;
    int reg_no;
    string surname;
    string name;
    string pers_type;
    string seat_no;
    int seats;
    int doc_check;
    int tid;
    string seat_no_str;
    string remarks;
    int rk_amount;
    int rk_weight;
    int excess;
    int value_bag_count;
    string tags;
    int pr_payment;
};
typedef vector<TPax> TPaxList;

void BrdInterface::BrdList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    enum {search, list} ListType;

    ProgTrace(TRACE5, "Query: %s", reqNode->name);
    TQuery *Qry = OraSession.CreateQuery();

    string condition;
    if(strcmp((char *)reqNode->name, "brd_list") == 0) {
        ListType = list;
        condition = " AND point_id= :point_id AND pr_brd= :pr_brd ";
    } else if(strcmp((char *)reqNode->name, "search_reg") == 0) {
        ListType = search;
        condition = " AND point_id= :point_id AND reg_no= :reg_no AND pr_brd IS NOT NULL ";
    } else if(strcmp((char *)reqNode->name, "search_bar") == 0) {
        ListType = search;
        condition = " AND pax_id= :pax_id AND pr_brd IS NOT NULL ";
    } else throw Exception("BrdInterface::BrdList: Unknown command tag %s", reqNode->name);

    string sqlText = (string)
        "SELECT "
        "    pax_id, "
        "    pax.grp_id, "
        "    point_id, "
        "    pr_brd, "
        "    reg_no, "
        "    surname, "
        "    name, "
        "    pers_type, "
        "    seat_no, "
        "    seats, "
        "    doc_check, "
        "    pax.tid, "
        "    LPAD(seat_no,3,'0')||DECODE(SIGN(1-seats),-1,'+'||TO_CHAR(seats-1),'') AS seat_no_str, "
        "    ckin.get_remarks(pax_id,', ',0) AS remarks, "
        "    NVL(ckin.get_rkAmount(pax_grp.grp_id,NULL,rownum),0) AS rk_amount, "
        "    NVL(ckin.get_rkWeight(pax_grp.grp_id,NULL,rownum),0) AS rk_weight, "
        "    /*  NVL(ckin.get_excess(pax_grp.grp_id,NULL),0) AS excess,*/ NVL(pax_grp.excess,0) AS excess, "
        "    NVL(value_bag.count,0) AS value_bag_count, "
        "    ckin.get_birks(pax_grp.grp_id,NULL) AS tags, "
        "    kassa.pr_payment(pax_grp.grp_id) AS pr_payment  "
        "FROM "
        "    pax_grp, "
        "    pax, "
        "    ( "
        "     SELECT "
        "        grp_id, "
        "        COUNT(*) AS count "
        "     FROM "
        "        value_bag "
        "     GROUP BY "
        "        grp_id "
        "    ) value_bag "
        "WHERE "
        "    pax_grp.grp_id=pax.grp_id AND "
        "    pax_grp.grp_id=value_bag.grp_id(+)  " +
        condition +
        "ORDER BY reg_no ";
    Qry->SQLText = sqlText;
    TParams1 SQLParams;
    SQLParams.getParams(GetNode("sqlparams", reqNode));
    SQLParams.setSQL(Qry);
    Qry->Execute();
    if(ListType == search && Qry->Eof)
        throw UserException(1, "Пассажир не зарегистрирован");
    TPaxList PaxList;
    while(!Qry->Eof) {
        TPax Pax;
        Pax.pax_id          = Qry->FieldAsInteger("pax_id");
        Pax.grp_id          = Qry->FieldAsInteger("grp_id");
        Pax.point_id        = Qry->FieldAsInteger("point_id");
        Pax.pr_brd          = Qry->FieldAsInteger("pr_brd");
        Pax.old_pr_brd      = Pax.pr_brd;
        Pax.reg_no          = Qry->FieldAsInteger("reg_no");
        Pax.surname         = Qry->FieldAsString("surname");
        Pax.name            = Qry->FieldAsString("name");
        Pax.pers_type       = Qry->FieldAsString("pers_type");
        Pax.seat_no         = Qry->FieldAsString("seat_no");
        Pax.seats           = Qry->FieldAsInteger("seats");
        Pax.doc_check       = Qry->FieldAsInteger("doc_check");
        Pax.tid             = Qry->FieldAsInteger("tid");
        Pax.seat_no_str     = Qry->FieldAsString("seat_no_str");
        Pax.remarks         = Qry->FieldAsString("remarks");
        Pax.rk_amount       = Qry->FieldAsInteger("rk_amount");
        Pax.rk_weight       = Qry->FieldAsInteger("rk_weight");
        Pax.excess          = Qry->FieldAsInteger("excess");
        Pax.value_bag_count = Qry->FieldAsInteger("value_bag_count");
        Pax.tags            = Qry->FieldAsString("tags");
        Pax.pr_payment      = Qry->FieldAsInteger("pr_payment");
        PaxList.push_back(Pax);
        Qry->Next();
    }
    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    switch(ListType) {
        case search:
            ProgTrace(TRACE5, "ListType: search");
            break;
        case list:
            ProgTrace(TRACE5, "ListType: list");
            break;
    }
    if(ListType == search) {
        TPaxList::iterator Pax = PaxList.begin();
        if(Pax->point_id != JxtContext::getJxtContHandler()->currContext()->readInt("TRIP_ID"))
            throw UserException(1, "Пассажир относится к другому рейсу");
        int boarding = NodeAsInteger("boarding", reqNode);
        if(!boarding && !Pax->pr_brd || boarding && Pax->pr_brd) {
            if(Pax->pr_brd)
                throw UserException(1, "Пассажир с указанным номером уже прошел посадку");
            else
                throw UserException(1, "Пассажир с указанным номером не прошел посадку");
        }
        if(!Pax->pr_brd && !ChckSt(Pax->pax_id, Pax->seat_no))
            NewTextChild(dataNode, "failed");
        else {
            // update
            Pax->pr_brd = !Pax->pr_brd;
            int Result = PaxUpdate(Pax->pax_id, Pax->tid, Pax->pr_brd);
            NewTextChild(dataNode, "pr_brd", Result);
        }
    }
    OraSession.DeleteQuery(*Qry);

    xmlNodePtr listNode = NewTextChild(dataNode, "brd_list");
    for(TPaxList::iterator iv = PaxList.begin(); iv != PaxList.end(); iv++) {
        xmlNodePtr paxNode = NewTextChild(listNode, "pax");
        NewTextChild(paxNode, "pax_id", iv->pax_id);
        NewTextChild(paxNode, "grp_id", iv->grp_id);
        NewTextChild(paxNode, "point_id", iv->point_id);
        NewTextChild(paxNode, "pr_brd", iv->pr_brd);
        NewTextChild(paxNode, "old_pr_brd", iv->old_pr_brd);
        NewTextChild(paxNode, "reg_no", iv->reg_no);
        NewTextChild(paxNode, "surname", iv->surname);
        NewTextChild(paxNode, "name", iv->name);
        NewTextChild(paxNode, "pers_type", iv->pers_type);
        NewTextChild(paxNode, "seat_no", iv->seat_no);
        NewTextChild(paxNode, "seats", iv->seats);
        NewTextChild(paxNode, "doc_check", iv->doc_check);
        NewTextChild(paxNode, "tid", iv->tid);
        NewTextChild(paxNode, "seat_no_str", iv->seat_no_str);
        NewTextChild(paxNode, "remarks", iv->remarks);
        NewTextChild(paxNode, "rk_amount", iv->rk_amount);
        NewTextChild(paxNode, "rk_weight", iv->rk_weight);
        NewTextChild(paxNode, "excess", iv->excess);
        NewTextChild(paxNode, "value_bag_count", iv->value_bag_count);
        NewTextChild(paxNode, "tags", iv->tags);
        NewTextChild(paxNode, "pr_payment", iv->pr_payment);
    }
}

void BrdInterface::SetCounters(xmlNodePtr dataNode)
{
    TQuery Qry(&OraSession);
    int trip_id = JxtContext::getJxtContHandler()->currContext()->readInt("TRIP_ID");
    Qry.SQLText =
        "SELECT "
        "    COUNT(*) AS reg, "
        "    NVL(SUM(DECODE(pr_brd,0,0,1)),0) AS brd "
        "FROM "
        "    pax_grp, "
        "    pax "
        "WHERE "
        "    pax_grp.grp_id=pax.grp_id AND "
        "    point_id=:trip_id AND "
        "    pr_brd IS NOT NULL ";
    Qry.DeclareVariable("trip_id", otInteger);

    Qry.SetVariable("trip_id", trip_id);
    Qry.Execute();
    if(!Qry.Eof) {
        xmlNodePtr countersNode = NewTextChild(dataNode, "counters");
        NewTextChild(countersNode, "reg", Qry.FieldAsInteger("reg"));
        NewTextChild(countersNode, "brd", Qry.FieldAsInteger("brd"));
    }
}

void BrdInterface::Trip(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery *Qry = OraSession.CreateQuery();
    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    if(GetNode("info", reqNode)) {
        Qry->SQLText =
            "SELECT "
            "    trips.trip_id, "
            "    trips.bc, "
            "    SUBSTR(ckin.get_classes(trips.trip_id),1,255) AS classes, "
            "    SUBSTR(ckin.get_places(trips.trip_id),1,255) AS places, "
            "    TO_CHAR(gtimer.get_stage_time(trips.trip_id,:brd_close_stage_id),'HH24:MI') AS brd_to, "
            "    TO_CHAR(NVL(NVL(trips.act,trips.est),trips.scd),'HH24:MI') AS takeoff, "
            "    trips.triptype, "
            "    trips.litera, "
            "    stages.brd_stage, "
            "    trips.remark "
            "FROM "
            "    trips, "
            "    trip_stations, "
            "    ( "
            "     SELECT "
            "        gtimer.get_stage( :trip_id, :brd_stage_type ) as brd_stage "
            "     FROM dual "
            "    ) stages "
            "WHERE "
            "    trip_stations.trip_id= :trip_id AND "
            "    trips.act IS NULL AND "
            "    trip_stations.name= :station AND "
            "    trip_stations.work_mode='П' AND "
            "    trips.trip_id= :trip_id AND "
            "    trips.status=0 AND  "
            "    stages.brd_stage = :brd_open_stage_id ";
        TParams1 SQLParams;
        SQLParams.getParams(GetNode("sqlparams", reqNode));
        SQLParams.setSQL(Qry);
        Qry->Execute();
        if(!Qry->Eof) {
            xmlNodePtr fltNode = NewTextChild(dataNode, "flt");
            NewTextChild(fltNode, "trip_id", Qry->FieldAsInteger("trip_id"));
            NewTextChild(fltNode, "bc", Qry->FieldAsString("bc"));
            NewTextChild(fltNode, "classes", Qry->FieldAsString("classes"));
            NewTextChild(fltNode, "places", Qry->FieldAsString("places"));
            NewTextChild(fltNode, "brd_to", Qry->FieldAsString("brd_to"));
            NewTextChild(fltNode, "takeoff", Qry->FieldAsString("takeoff"));
            NewTextChild(fltNode, "triptype", Qry->FieldAsString("triptype"));
            NewTextChild(fltNode, "litera", Qry->FieldAsString("litera"));
            NewTextChild(fltNode, "brd_stage", Qry->FieldAsInteger("brd_stage"));
            NewTextChild(fltNode, "remark", Qry->FieldAsString("remark"));
        }
    }
    Qry->Clear();
    if(GetNode("counters", reqNode)) {
        int trip_id = NodeAsInteger("sqlparams/trip_id", reqNode);
        JxtContext::getJxtContHandler()->currContext()->write("TRIP_ID", trip_id);
        Qry->SQLText =
            "SELECT "
            "    COUNT(*) AS reg, "
            "    NVL(SUM(DECODE(pr_brd,0,0,1)),0) AS brd "
            "FROM "
            "    pax_grp, "
            "    pax "
            "WHERE "
            "    pax_grp.grp_id=pax.grp_id AND "
            "    point_id=:trip_id AND "
            "    pr_brd IS NOT NULL ";
        Qry->DeclareVariable("trip_id", otInteger);

        Qry->SetVariable("trip_id", trip_id);
        Qry->Execute();
        if(!Qry->Eof) {
            xmlNodePtr countersNode = NewTextChild(dataNode, "counters");
            NewTextChild(countersNode, "reg", Qry->FieldAsInteger("reg"));
            NewTextChild(countersNode, "brd", Qry->FieldAsInteger("brd"));
        }
    }
    OraSession.DeleteQuery(*Qry);
}

void BrdInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

}
