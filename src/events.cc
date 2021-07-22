#include "events.h"
#include "astra_date_time.h"
#include "exceptions.h"
#include "oralib.h"
#include "stl_utils.h"
#include "xml_unit.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "docs/docs_common.h"
#include "aodb.h"
#include "stat/stat_main.h"
#include "flt_settings.h"
#include "qrys.h"
#include "arx_daily_pg.h"
#include "dbo.h"

#include <serverlib/algo.h>
#include <serverlib/testmode.h>

#define NICKNAME "DJEK"
#include <serverlib/slogger.h>

using namespace std;
using namespace BASIC::date_time;
using namespace AstraLocale;
using namespace EXCEPTIONS;
using namespace ASTRA;
using namespace ASTRA::date_time;

int getMoveId(TDateTime part_key, int point_id)
{
    LogTrace(TRACE5) << __FUNCTION__ << " point_id: " << point_id << " part_key: " << part_key;
    int res_move_id = NoExists;
    dbo::Session session;
    if ( part_key != NoExists ) {
        std::optional<int> move_id = session.query<int>("SELECT move_id").from("arx_points")
                .where("part_key=:part_key AND point_id=:point_id AND pr_del>=0")
                .setBind({{"point_id", point_id}, {":part_key", DateTimeToBoost(part_key)}});
        if(move_id) {
            res_move_id = *move_id;
        }
    }
    else {
        TQuery Qry(&OraSession);
        Qry.Clear();
        Qry.SQLText=
                "SELECT move_id FROM points "
                "WHERE point_id=:point_id AND pr_del>=0";
        Qry.CreateVariable( "point_id", otInteger, point_id );
        Qry.Execute();
        if ( !Qry.Eof ) res_move_id = Qry.FieldAsInteger( "move_id" );
    }
    return res_move_id;
}

void readFromArx(Dates::DateTime_t part_key, int move_id, int point_id, const vector<string>& eventTypes, xmlNodePtr logNode)
{
    LogTrace(TRACE5) << __FUNCTION__ << " move_id: " << move_id << " part_key: " << part_key;
    ostringstream sql;

    std::string msg;
    Dates::DateTime_t time;
    int read_point_id;
    int reg_no;
    int grp_id;
    std::string ev_user;
    std::string station;
    int ev_order;
    int part_num;


    if (move_id != NoExists || !eventTypes.empty())
    {
        if (move_id != NoExists)
        {
            sql << "SELECT msg, time, id2 AS point_id, \n"
                   "   (CASE when type=:evtPax THEN id2 WHEN type=:evtPay THEN id2 ELSE -1 END) AS reg_no, \n"
                   "   (CASE when type=:evtPax THEN id3 WHEN type=:evtPay THEN id3 ELSE -1 END) AS grp_id, \n"
                   "   ev_user, station, ev_order, part_num \n"
                   "FROM arx_events \n"
                   "WHERE part_key=:part_key AND (lang=:lang OR lang='ZZ') AND \n"
                   "   type=:evtDisp AND id1=:move_id \n";
        }
        if (move_id != NoExists && !eventTypes.empty())
        {
            sql << "UNION \n";
        }
        if (!eventTypes.empty())
        {
            sql << "SELECT msg, time, id1 AS point_id, \n"
                   "    (CASE when type=:evtPax THEN id2 WHEN type=:evtPay THEN id2 ELSE -1 END) AS reg_no, \n"
                   "    (CASE when type=:evtPax THEN id3 WHEN type=:evtPay THEN id3 ELSE -1 END) AS grp_id, \n"
                   "    ev_user, station, ev_order, part_num \n"
                   "FROM arx_events \n"
                   "WHERE part_key=:part_key AND (lang=:lang OR lang='ZZ') AND \n"
                   " type IN " << GetSQLEnum(eventTypes) << " AND id1=:point_id \n";
        }

        #ifdef XP_TESTING
        if(inTestMode()){
            sql << " ORDER BY ev_order \n";
        }
        #endif
    }

    auto cur  = make_db_curs(sql.str(), PgOra::getROSession("ARX_EVENTS"));
    cur.stb();

    if (move_id != NoExists)
    {
        cur.bind(":evtDisp", EncodeEventType(ASTRA::evtDisp))
           .bind(":move_id", move_id);
    }
    if (!eventTypes.empty())
    {
        cur.bind(":point_id", point_id);
    }

    cur.def(msg)
        .def(time)
        .defNull(read_point_id, ASTRA::NoExists)
        .defNull(reg_no, ASTRA::NoExists)
        .defNull(grp_id, ASTRA::NoExists)
        .defNull(ev_user, "")
        .defNull(station, "")
        .def(ev_order)
        .defNull(part_num,1)
        .bind(":lang", TReqInfo::Instance()->desk.lang)
        .bind(":part_key", part_key)
        .bind(":evtPax",EncodeEventType(ASTRA::evtPax))
        .bind(":evtPay",EncodeEventType(ASTRA::evtPay))
        .exec();
    while(!cur.fen()) {
        xmlNodePtr rowNode=NewTextChild(logNode,"row");
        NewTextChild(rowNode,"point_id",read_point_id);
        NewTextChild(rowNode,"ev_user",ev_user);
        NewTextChild(rowNode,"station",station);

        TDateTime client_time = UTCToClient(BoostToDateTime(time),TReqInfo::Instance()->desk.tz_region);

        NewTextChild(rowNode,"time",DateTimeToStr(client_time));
        NewTextChild(rowNode,"fmt_time",DateTimeToStr(client_time, "dd.mm.yy hh:nn"));
        NewTextChild(rowNode,"grp_id",grp_id,-1);
        NewTextChild(rowNode,"reg_no",reg_no,-1);
        NewTextChild(rowNode,"msg",msg);
        NewTextChild(rowNode,"ev_order",ev_order);
        NewTextChild(rowNode,"part_num",part_num,1);
    }
}

void EventsInterface::GetEvents(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery Qry(&OraSession);
    int point_id = NodeAsInteger("point_id",reqNode);
    TDateTime part_key = NoExists;
    if (GetNode( "part_key", reqNode )!=NULL) {
        part_key = NodeAsDateTime( "part_key", reqNode );
    }
    xmlNodePtr etNode = GetNode( "EventsTypes", reqNode );
    vector<string> eventTypes;
    bool disp_event=false;
    if (etNode!=NULL)
    {
        for(etNode=etNode->children; etNode!=NULL; etNode=etNode->next)
        {
            TEventType eventType=DecodeEventType(NodeAsString(etNode->children));
            switch(eventType)
            {
            case evtDisp:
                disp_event=true;
                break;
            case evtUnknown:
                break;
            default:
            {
                eventTypes.push_back(NodeAsString(etNode->children));
                break;
            }
            };
        };
    };

    int move_id = NoExists;
    if ( disp_event )
    {
        move_id = getMoveId(part_key, point_id);
    };

    xmlNodePtr logNode = NewTextChild(resNode, "events_log");

    if(part_key != NoExists && ARX_EVENTS_DISABLED())
    {
        throw UserException("MSG.ERR_MSG.ARX_EVENTS_DISABLED");
    }
    if (move_id != NoExists || !eventTypes.empty())
    {
        Qry.Clear();
        ostringstream sql;
        if (part_key != NoExists) {
            readFromArx(DateTimeToBoost(part_key), move_id, point_id, eventTypes, logNode);
        }
        else {
            if (move_id != NoExists)
            {
                sql << "SELECT msg, time, id2 AS point_id, \n"
                       "       DECODE(type,:evtPax,id2,:evtPay,id2,-1) AS reg_no, \n"
                       "       DECODE(type,:evtPax,id3,:evtPay,id3,-1) AS grp_id, \n"
                       "       ev_user, station, ev_order, NVL(part_num, 1) AS part_num \n"
                       "FROM events_bilingual \n"
                       "WHERE lang=:lang AND \n"
                       " type=:evtDisp AND id1=:move_id \n";
                Qry.CreateVariable("evtDisp",otString,EncodeEventType(ASTRA::evtDisp));
                Qry.CreateVariable("move_id",otInteger,move_id);
            }
            if (move_id != NoExists && !eventTypes.empty()) {
                sql << "UNION \n";
            }
            if (!eventTypes.empty())
            {
                sql << "SELECT msg, time, id1 AS point_id, \n"
                       "       DECODE(type,:evtPax,id2,:evtPay,id2,-1) AS reg_no, \n"
                       "       DECODE(type,:evtPax,id3,:evtPay,id3,-1) AS grp_id, \n"
                       "       ev_user, station, ev_order, NVL(part_num, 1) AS part_num \n"
                       "FROM events_bilingual \n"
                       "WHERE lang=:lang AND \n";
                sql << " type IN " << GetSQLEnum(eventTypes) << " AND id1=:point_id \n";
                Qry.CreateVariable("point_id",otInteger,point_id);
            };

            Qry.CreateVariable("lang", otString, TReqInfo::Instance()->desk.lang);
            Qry.CreateVariable("evtPax",otString,EncodeEventType(ASTRA::evtPax));
            Qry.CreateVariable("evtPay",otString,EncodeEventType(ASTRA::evtPay));

            #ifdef XP_TESTING
            if(inTestMode()){
                sql << " ORDER BY ev_order \n";
            }
            #endif

            //ProgTrace(TRACE5, "GetEvents: SQL=\n%s", sql.str().c_str());
            Qry.SQLText=sql.str().c_str();
            Qry.Execute();

            if (!Qry.Eof)
            {
                int col_msg=Qry.FieldIndex("msg");
                int col_time=Qry.FieldIndex("time");
                int col_point_id=Qry.FieldIndex("point_id");
                int col_reg_no=Qry.FieldIndex("reg_no");
                int col_grp_id=Qry.FieldIndex("grp_id");
                int col_ev_user=Qry.FieldIndex("ev_user");
                int col_station=Qry.FieldIndex("station");
                int col_ev_order=Qry.FieldIndex("ev_order");
                int col_part_num=Qry.FieldIndex("part_num");

                for(;!Qry.Eof;Qry.Next())
                {
                    xmlNodePtr rowNode=NewTextChild(logNode,"row");
                    NewTextChild(rowNode,"point_id",Qry.FieldAsInteger(col_point_id));
                    NewTextChild(rowNode,"ev_user",Qry.FieldAsString(col_ev_user));
                    NewTextChild(rowNode,"station",Qry.FieldAsString(col_station));

                    TDateTime time = UTCToClient(Qry.FieldAsDateTime(col_time),TReqInfo::Instance()->desk.tz_region);

                    NewTextChild(rowNode,"time",DateTimeToStr(time));
                    NewTextChild(rowNode,"fmt_time",DateTimeToStr(time, "dd.mm.yy hh:nn"));
                    NewTextChild(rowNode,"grp_id",Qry.FieldAsInteger(col_grp_id),-1);
                    NewTextChild(rowNode,"reg_no",Qry.FieldAsInteger(col_reg_no),-1);
                    NewTextChild(rowNode,"msg",Qry.FieldAsString(col_msg));
                    NewTextChild(rowNode,"ev_order",Qry.FieldAsInteger(col_ev_order));
                    NewTextChild(rowNode,"part_num",Qry.FieldAsInteger(col_part_num),1);
                };
            };
        };
    }
    logNode = NewTextChild(resNode, "form_data");
    logNode = NewTextChild(logNode, "variables");
    if ( GetNode( "seasonvars", reqNode ) ) {
        SeasonListVars( point_id, 0, logNode, reqNode );
        NewTextChild(logNode, "caption", getLocaleText("CAP.DOC.SEASON_EVENTS_LOG",
                                                       LParams() << LParam("trip", NodeAsString("trip", logNode))));
    } else {
        TRptParams rpt_params(TReqInfo::Instance()->desk.lang);
        PaxListVars(point_id, rpt_params, logNode, part_key);
        NewTextChild(logNode, "caption", getLocaleText("CAP.DOC.EVENTS_LOG",
                                                       LParams() << LParam("flight", get_flight(logNode))
                                                       << LParam("day_issue", NodeAsString("day_issue", logNode)
                                                                 )));
    }
    NewTextChild(logNode, "cap_test", getLocaleText("CAP.TEST", TReqInfo::Instance()->desk.lang));
    get_new_report_form("EventsLog", reqNode, resNode);
    NewTextChild(logNode, "short_page_number_fmt", getLocaleText("CAP.SHORT_PAGE_NUMBER_FMT"));
}

std::string TPaxToLogInfo::getBagStr() const
{
    std::ostringstream msg;
    if (bag_amount!=0 || bag_weight!=0)
    {
        if (!msg.str().empty()) msg << ", ";
        msg << "����� " << bag_amount << "/" << bag_weight;
    };
    if (rk_amount!=0 || rk_weight!=0)
    {
        if (!msg.str().empty()) msg << ", ";
        msg << "�/����� " << rk_amount << "/" << rk_weight;
    };
    if (!tags.empty())
    {
        if (!msg.str().empty()) msg << ", ";
        msg << "��ન " << tags;
    };
    return msg.str();
}

void TPaxToLogInfo::getBag(PrmEnum& param) const
{
    bool empty = true;
    if (bag_amount!=0 || bag_weight!=0)
    {
        param.prms << PrmLexema("", "EVT.LUGGAGE") << PrmSmpl<string>("", " ")
                   << PrmSmpl<int>("", bag_amount) << PrmSmpl<string>("", "/")
                   << PrmSmpl<int>("", bag_weight);
        empty = false;
    };
    if (rk_amount!=0 || rk_weight!=0)
    {
        if (!empty) param.prms << PrmSmpl<string>("", ", ");
        param.prms << PrmLexema("", "EVT.CABIN_LUGGAGE") << PrmSmpl<string>("", " ")
                   << PrmSmpl<int>("", rk_amount) << PrmSmpl<string>("", "/")
                   << PrmSmpl<int>("", rk_weight);
        empty = false;
    };

    if (!tags.empty())
    {
        if (!empty) param.prms << PrmSmpl<string>("", ", ");
        param.prms << PrmLexema("", "EVT.TAGS") << PrmSmpl<string>("", " ")
                   << PrmSmpl<string>("", tags);
    };
}

void logPaxName(const string &status, const string &surname,
                const string &name, const string &pers_type,
                LEvntPrms& params)
{
    if (pers_type.empty())
        params << PrmLexema("pax_name", "EVT.UNACCOMPANIED_LUGGAGE");
    else {
        PrmLexema lexema("pax_name", "EVT.PASSENGER");
        if (status == EncodePaxStatus(psCrew)) lexema.ChangeLexemaId("EVT.CREW_MEMBER");
        lexema.prms << PrmSmpl<std::string>("surname", surname)
                    << PrmSmpl<std::string>("name", (name.empty()?"":" ") + name)
                    << PrmElem<std::string>("pers_type", etPersType, pers_type);
        params << lexema;
    }
}

void TPaxToLogInfo::getPaxName(LEvntPrms& params) const
{
    logPaxName(status, surname, name, pers_type, params);
};

void TPaxToLogInfo::getNorm(PrmEnum& param) const
{
    if (norms_normal.empty()) {
        param.prms << PrmBool("", false);
        return;
    }
    TNormItemMap::const_iterator n=norms_normal.begin();
    for(;n!=norms_normal.end();++n)
    {
        if (n!=norms_normal.begin()) param.prms << PrmSmpl<string>("", ", ");
        if (!n->first.bag_type.empty()) {
            param.prms << PrmSmpl<string>("", n->first.bag_type+": ");
        }
        n->second.first.GetNorms(param);
        if (n->second.second)
            param.prms << PrmSmpl<string>("","(") << PrmLexema("", "EVT.MANUAL_ABBR") << PrmSmpl<string>("",")");
    };
};

TEventsBagItem& TEventsBagItem::fromDB(TQuery &Qry)
{
    CheckIn::TBagItem::fromDB(Qry);
    refused=Qry.FieldAsInteger("refused")!=0;
    pax_id=Qry.FieldIsNULL("pax_id")?ASTRA::NoExists:Qry.FieldAsInteger("pax_id");
    return *this;
};

void TPaidToLogInfo::add(const CheckIn::TBagItem &item)
{
    TEventsSumBagKey key;
    key.bag_type_view=item.key_str_compatible();
    key.is_trfer=item.is_trfer;
    std::map<TEventsSumBagKey, TEventsSumBagItem>::iterator i=bag.find(key);
    if (i==bag.end())
        i=bag.insert(make_pair(key, TEventsSumBagItem())).first;
    if (i==bag.end()) return;
    i->second.amount+=item.amount;
    i->second.weight+=item.weight;
}
void TPaidToLogInfo::add(const WeightConcept::TPaidBagItem& item)
{
    TEventsSumBagKey key;
    key.bag_type_view=item.bag_type;
    key.is_trfer=false;
    std::map<TEventsSumBagKey, TEventsSumBagItem>::iterator i=bag.find(key);
    if (i==bag.end())
        i=bag.insert(make_pair(key, TEventsSumBagItem())).first;
    if (i==bag.end()) return;
    i->second.paid+=item.weight;
    excess_wt+=TBagKilos(item.weight);
}
void TPaidToLogInfo::add(const TPaidRFISCItem& item)
{
    TEventsSumBagKey key;
    key.bag_type_view=item.RFISC;
    key.is_trfer=false;
    std::map<TEventsSumBagKey, TEventsSumBagItem>::iterator i=bag.find(key);
    if (i==bag.end())
        i=bag.insert(make_pair(key, TEventsSumBagItem())).first;
    if (i==bag.end()) return;
    if (item.paid_positive())
    {
        i->second.paid+=item.paid;
        excess_pc+=TBagPieces(item.paid);
    }
}

void TPaidToLogInfo::trace( TRACE_SIGNATURE, const std::string &descr) const
{
    ProgTrace(TRACE_PARAMS, "============ %s ============", descr.c_str());
    ProgTrace(TRACE_PARAMS, "excess_wt: %s", excess_wt.view(OutputLang(LANG_EN)).c_str());
    ProgTrace(TRACE_PARAMS, "excess_pc: %s", excess_pc.view(OutputLang(LANG_EN)).c_str());
    ProgTrace(TRACE_PARAMS, "bag:");
    for(map<TEventsSumBagKey, TEventsSumBagItem>::const_iterator b=bag.begin(); b!=bag.end(); ++b)
    {
        ostringstream str;
        str << "  ";
        if (!b->first.bag_type_view.empty())
            str << b->first.bag_type_view << ":";
        if (b->first.is_trfer)
            str << "T:";
        str << b->second.amount << "/" << b->second.weight << "/" << b->second.paid;
        ProgTrace(TRACE_PARAMS, "%s", str.str().c_str());
    };
    ProgTrace(TRACE_PARAMS, "emd:");
    for(multiset<CheckIn::TServicePaymentItem>::const_iterator p=payment.begin(); p!=payment.end(); ++p)
    {
        ostringstream str;
        str << "  ";
        if (!p->key_str(LANG_EN).empty())
            str << p->key_str(LANG_EN) << ":";
        str << p->no_str();
        str << ":" << p->doc_weight << "kg";
        ProgTrace(TRACE_PARAMS, "%s", str.str().c_str());
    }
    ProgTrace(TRACE_PARAMS, "^^^^^^^^^^^^ %s ^^^^^^^^^^^^", descr.c_str());
};

void GetBagToLogInfo(int grp_id, map<int/*id*/, TEventsBagItem> &bag)
{
    bag.clear();
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText=
            "SELECT bag2.*, "
            "       ckin.bag_pool_refused(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse) AS refused, "
            "       ckin.get_bag_pool_pax_id(bag2.grp_id,bag2.bag_pool_num) AS pax_id "
            "FROM pax_grp,bag2 "
            "WHERE pax_grp.grp_id=bag2.grp_id AND pax_grp.grp_id=:grp_id";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.Execute();
    //����� ����
    for(;!Qry.Eof;Qry.Next())
    {
        TEventsBagItem bagInfo;
        bagInfo.fromDB(Qry);
        bag[bagInfo.id]=bagInfo;
    }
}

void TGrpToLogInfo::clearExcess()
{
    paid.clearExcess();
    for(map<TPaxToLogInfoKey, TPaxToLogInfo>::iterator i=pax.begin(); i!=pax.end(); ++i)
        i->second.paid.clearExcess();
}

void TGrpToLogInfo::setExcess()
{
    if (!piece_concept)
    {
        WeightConcept::TPaidBagList paid_bag;
        WeightConcept::PaidBagFromDB(NoExists, grp_id, paid_bag);
        for(WeightConcept::TPaidBagList::const_iterator p=paid_bag.begin(); p!=paid_bag.end(); ++p)
            paid.add(*p);
    }
    else
    {
        TPaidRFISCListWithAuto paid_rfisc;
        paid_rfisc.fromDB(grp_id, true);
        for(TPaidRFISCListWithAuto::const_iterator p=paid_rfisc.begin(); p!=paid_rfisc.end(); ++p)
        {
            const TPaidRFISCItem &item=p->second;
            if (item.trfer_num!=0) continue;
            if (item.pax_id==ASTRA::NoExists)
            {
                paid.add(item);
            }
            else
            {
                map<TPaxToLogInfoKey, TPaxToLogInfo>::iterator i=findPax(item.pax_id);
                if (i!=pax.end())
                    i->second.paid.add(item);
                else
                    paid.add(item);
            };
        };
    };
}

void TGrpToLogInfo::clearEmd()
{
    paid.payment.clear();
    for(map<TPaxToLogInfoKey, TPaxToLogInfo>::iterator i=pax.begin(); i!=pax.end(); ++i)
        i->second.paid.payment.clear();
}

void TGrpToLogInfo::setEmd()
{
    CheckIn::TServicePaymentListWithAuto payment;
    payment.fromDB(grp_id);
    for(CheckIn::TServicePaymentListWithAuto::const_iterator p=payment.begin(); p!=payment.end(); ++p)
    {
        if (p->pax_id==ASTRA::NoExists)
        {
            paid.payment.insert(*p);
        }
        else
        {
            map<TPaxToLogInfoKey, TPaxToLogInfo>::iterator i=findPax(p->pax_id);
            if (i!=pax.end())
                i->second.paid.payment.insert(*p);
            else
                paid.payment.insert(*p);
        };
    };
}

void TGrpToLogInfo::setTermAgentSeatNo( const CheckIn::TPaxList &paxs )
{
    for(const auto& p : paxs) {
        if ( p.crs_seat_no == p.pax.seat_no ) { //����� ��祣� �� �롨ࠫ
            continue;
        }
        std::map<TPaxToLogInfoKey, TPaxToLogInfo>::iterator n = findPax( p.getExistingPaxIdOrSwear() );
        if ( n != pax.end() &&
             n->second.seat_no == p.pax.seat_no ) { // ����� ��ࠫ ���� � ��⥬� ��ᠤ��� �� �� ����
            n->second.agent_seat_no = p.pax.seat_no;
            ProgTrace(TRACE5, "pax_id=%d,crs_seat_no=%s,pax.seat_no=%s, currseat_no=%s",
                      p.getExistingPaxIdOrSwear(),  p.crs_seat_no.c_str(), p.pax.seat_no.c_str(), n->second.seat_no.c_str() );
        }
    }
}

void UpdGrpToLogInfo(int grp_id, TGrpToLogInfo &grpInfo)
{
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText="SELECT NVL(pax_grp.piece_concept, 0) AS piece_concept FROM pax_grp WHERE grp_id=:grp_id";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.Execute();
    if (Qry.Eof) return;
    grpInfo.piece_concept=Qry.FieldAsInteger("piece_concept")!=0;

    if (grpInfo.piece_concept)
    {
        grpInfo.clearExcess();
        grpInfo.setExcess();
        grpInfo.clearEmd();
        grpInfo.setEmd();
    };
}

void GetGrpToLogInfo(int grp_id, TGrpToLogInfo &grpInfo)
{
    grpInfo.clear();
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText=
            "SELECT "
            "       pax_grp.point_dep, pax_grp.airp_arv, "
            "       pax_grp.class AS orig_class, NVL(pax.cabin_class, pax_grp.class) AS cabin_class, pax_grp.status, "
            "       pax_grp.pr_mark_norms, pax_grp.bag_refuse, "
            "       pax.pax_id, pax.reg_no, "
            "       pax.surname, pax.name, pax.pers_type, pax.refuse, pax.subclass, pax.is_female, pax.seats, "
            "       salons.get_seat_no(pax.pax_id, pax.seats, pax.is_jmp, pax_grp.status, pax_grp.point_dep, 'seats', rownum) seat_no, "
            "       pax.ticket_no, pax.coupon_no, pax.ticket_rem, 0 AS ticket_confirm, "
            "       pax.pr_brd, pax.pr_exam, "
            "       NVL(ckin.get_bagAmount2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) AS bag_amount, "
            "       NVL(ckin.get_bagWeight2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) AS bag_weight, "
            "       NVL(ckin.get_rkAmount2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) AS rk_amount, "
            "       NVL(ckin.get_rkWeight2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) AS rk_weight, "
            "       ckin.get_birks2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,:lang) AS tags, "
            "       pax_grp.trfer_confirm, NVL(pax_grp.piece_concept, 0) AS piece_concept "
            "FROM pax_grp, pax "
            "WHERE pax_grp.grp_id=pax.grp_id(+) AND pax_grp.grp_id=:grp_id";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.CreateVariable("lang",otString,AstraLocale::LANG_RU); //���� � ��� ��襬 �ᥣ�� �� ���᪮�
    Qry.Execute();
    if (!Qry.Eof)
    {
        grpInfo.grp_id=grp_id;
        grpInfo.point_dep=Qry.FieldAsInteger("point_dep");
        grpInfo.trfer_confirm=Qry.FieldAsInteger("trfer_confirm")!=0;
        grpInfo.piece_concept=Qry.FieldAsInteger("piece_concept")!=0;
        for(;!Qry.Eof;Qry.Next())
        {
            TPaxToLogInfoKey paxInfoKey;
            paxInfoKey.pax_id=Qry.FieldIsNULL("pax_id")?NoExists:Qry.FieldAsInteger("pax_id");
            paxInfoKey.reg_no=Qry.FieldIsNULL("reg_no")?NoExists:Qry.FieldAsInteger("reg_no");
            TPaxToLogInfo &paxInfo=grpInfo.pax[paxInfoKey];
            paxInfo.clear();
            paxInfo.airp_arv=Qry.FieldAsString("airp_arv");
            paxInfo.orig_cl=Qry.FieldAsString("orig_class");
            paxInfo.cabin_cl=Qry.FieldAsString("cabin_class");
            paxInfo.status=Qry.FieldAsString("status");
            paxInfo.pr_mark_norms=Qry.FieldAsInteger("pr_mark_norms")!=0;

            if (paxInfoKey.pax_id!=NoExists)
            {
                paxInfo.surname=Qry.FieldAsString("surname");
                paxInfo.name=Qry.FieldAsString("name");
                paxInfo.pers_type=Qry.FieldAsString("pers_type");
                paxInfo.refuse=Qry.FieldAsString("refuse");
                paxInfo.subcl=Qry.FieldAsString("subclass");
                paxInfo.seat_no=Qry.FieldAsString("seat_no");
                paxInfo.seats=Qry.FieldAsInteger("seats");
                if (!Qry.FieldIsNULL("is_female"))
                    paxInfo.is_female=(int)(Qry.FieldAsInteger("is_female")!=0);
                else
                    paxInfo.is_female=ASTRA::NoExists;
                paxInfo.tkn.fromDB(Qry);
                paxInfo.pr_brd=paxInfo.refuse.empty() && !Qry.FieldIsNULL("pr_brd") && Qry.FieldAsInteger("pr_brd")!=0;
                paxInfo.pr_exam=paxInfo.refuse.empty() && !Qry.FieldIsNULL("pr_exam") && Qry.FieldAsInteger("pr_exam")!=0;
                paxInfo.apis.fromDB(paxInfoKey.pax_id);
                CheckIn::LoadPaxRem(paxInfoKey.pax_id, paxInfo.rems);
                CheckIn::LoadPaxFQT(paxInfoKey.pax_id, paxInfo.fqts);
            }
            else
            {
                paxInfo.refuse=Qry.FieldAsInteger("bag_refuse")!=0?refuseAgentError:"";
            };

            paxInfo.bag_amount=Qry.FieldAsInteger("bag_amount");
            paxInfo.bag_weight=Qry.FieldAsInteger("bag_weight");
            paxInfo.rk_amount=Qry.FieldAsInteger("rk_amount");
            paxInfo.rk_weight=Qry.FieldAsInteger("rk_weight");
            paxInfo.tags=Qry.FieldAsString("tags");

            if (!grpInfo.piece_concept)
            {
                if (paxInfoKey.pax_id!=NoExists)
                    WeightConcept::PaxNormsFromDB(NoExists, paxInfoKey.pax_id, paxInfo.norms);
                else
                    WeightConcept::GrpNormsFromDB(NoExists, grp_id, paxInfo.norms);
                for(const WeightConcept::TPaxNormComplex& n : paxInfo.norms)
                {
                    if (n.normNotExists() && !n.isManuallyDeleted()) continue;
                    paxInfo.norms_normal.emplace(n, make_pair(n, n.handmade && n.handmade.get()));
                }
            };
        };

        GetBagToLogInfo(grp_id, grpInfo.bag);
        if (!grpInfo.bag.empty())
        {
            //����� ����
            for(map<int/*id*/, TEventsBagItem>::const_iterator b=grpInfo.bag.begin(); b!=grpInfo.bag.end(); ++b)
            {
                const TEventsBagItem &bagInfo=b->second;
                if (bagInfo.refused) continue;

                if (!grpInfo.piece_concept || bagInfo.pax_id==ASTRA::NoExists)
                {
                    grpInfo.paid.add(bagInfo);
                }
                else
                {
                    map<TPaxToLogInfoKey, TPaxToLogInfo>::iterator i=grpInfo.findPax(bagInfo.pax_id);
                    if (i!=grpInfo.pax.end())
                        i->second.paid.add(bagInfo);
                    else
                        grpInfo.paid.add(bagInfo);
                };
            };

            grpInfo.setExcess();
        };

        grpInfo.setEmd();
    };
};

void AddTKNLogMsgs(const CheckIn::TPaxTknItem &tknCrs,
                   const boost::optional<CheckIn::TPaxTknItem> &tknBefore,
                   const CheckIn::TPaxTknItem &tknAfter,
                   list< pair<string, LEvntPrms> > &msgs)
{
    const CheckIn::TPaxTknItem &tknPrior=tknBefore?tknBefore.get():tknCrs;

    if (tknAfter.equalAttrs(tknPrior)) return;
    if (tknAfter.empty())
        msgs.push_back(make_pair("EVT.TKN_DELETED_FOR_PASSENGER", LEvntPrms()));
    else
    {
        ostringstream s;
        s << tknAfter.rem << " " << tknAfter.no_str();

        bool equivalent_booking=tknAfter.equalAttrs(tknCrs);

        if (tknPrior.empty())
            msgs.push_back(make_pair(equivalent_booking?"EVT.TKN_ADDED_FOR_PASSENGER.EQUIVALENT_BOOKING":
                                                        "EVT.TKN_ADDED_FOR_PASSENGER",
                                     LEvntPrms() << PrmSmpl<string>("params", s.str())));
        else
            msgs.push_back(make_pair(equivalent_booking?"EVT.TKN_MODIFIED_FOR_PASSENGER.EQUIVALENT_BOOKING":
                                                        "EVT.TKN_MODIFIED_FOR_PASSENGER",
                                     LEvntPrms() << PrmSmpl<string>("params", s.str())));
    };
}

bool EquivalentBooking(const set<CheckIn::TPaxFQTItem> &fqtsCrs,
                       const CheckIn::TPaxFQTItem &fqt)
{
    for(set<CheckIn::TPaxFQTItem>::const_iterator f=fqtsCrs.begin(); f!=fqtsCrs.end(); ++f)
        if (fqt.airline==f->airline &&
                fqt.no==f->no &&
                fqt.tier_level==f->tier_level) return true;

    return false;
}

void AddFQTLogMsgs(const set<CheckIn::TPaxFQTItem> &fqtsCrs,
                   const boost::optional< set<CheckIn::TPaxFQTItem> > &fqtsBefore,
                   const set<CheckIn::TPaxFQTItem> &fqtsAfter,
                   list< pair<string, LEvntPrms> > &msgs)
{
    const set<CheckIn::TPaxFQTItem> &fqtsPrior=fqtsBefore?fqtsBefore.get():fqtsCrs;
    for(int pass=0; pass<3; pass++)
    {
        set<CheckIn::TPaxFQTItem> fqts;
        //pass==0 modified
        //pass==1 deleted
        //pass==2 added
        if (pass==0)
        {
            if (fqtsBefore)
            {
                for(set<CheckIn::TPaxFQTItem>::const_iterator f=fqtsAfter.begin(); f!=fqtsAfter.end(); ++f)
                {
                    set<CheckIn::TPaxFQTItem>::const_iterator i=fqtsPrior.find(*f);
                    if (i!=fqtsPrior.end() && !(*f==*i)) fqts.insert(*f);
                };
            }
            else
            {
                set_intersection(fqtsAfter.begin(), fqtsAfter.end(),
                                 fqtsPrior.begin(), fqtsPrior.end(),
                                 inserter(fqts, fqts.end()));
            };
        };
        if (pass==1)
            set_difference(fqtsPrior.begin(), fqtsPrior.end(),
                           fqtsAfter.begin(), fqtsAfter.end(),
                           inserter(fqts, fqts.end()));
        if (pass==2)
            set_difference(fqtsAfter.begin(), fqtsAfter.end(),
                           fqtsPrior.begin(), fqtsPrior.end(),
                           inserter(fqts, fqts.end()));

        for(set<CheckIn::TPaxFQTItem>::const_iterator f=fqts.begin(); f!=fqts.end(); ++f)
        {
            LEvntPrms params;

            ostringstream msg;
            msg << f->rem << " " << f->logStr();
            params << PrmSmpl<string>("fqt_rem", msg.str())
                   << PrmSmpl<string>("tier_level", f->tier_level);

            if (f->tier_level.empty())
                params << PrmLexema("tier_level_status", "EVT.TIER_LEVEL_STATUS.UNKNOWN");
            else if (EquivalentBooking(fqtsCrs, *f))
                params << PrmLexema("tier_level_status", "EVT.TIER_LEVEL_STATUS.EQUIVALENT_BOOKING");
            else if (f->tier_level_confirm)
                params << PrmLexema("tier_level_status", "EVT.TIER_LEVEL_STATUS.CONFIRMED");
            else
                params << PrmLexema("tier_level_status", "EVT.TIER_LEVEL_STATUS.NOT_CONFIRMED");

            switch (pass)
            {
            case 0: msgs.push_back(make_pair(fqtsBefore?"EVT.FQT_MODIFIED_FOR_PASSENGER":
                                                        "EVT.FQT_FROM_BOOKING_FOR_PASSENGER", params)); break;
            case 1: msgs.push_back(make_pair("EVT.FQT_DELETED_FOR_PASSENGER", params)); break;
            case 2: msgs.push_back(make_pair("EVT.FQT_ADDED_FOR_PASSENGER", params)); break;
            default: ;
            }
        }
    };
}

void GetAPISLogMsgs(const CheckIn::TAPISItem &apisBefore,
                    const CheckIn::TAPISItem &apisAfter,
                    list<pair<string, string> > &msgs)
{
    msgs.clear();

    bool manualInputBefore=(apisBefore.doc.scanned_attrs & apisBefore.doc.getNotEmptyFieldsMask()) != apisBefore.doc.getNotEmptyFieldsMask();
    bool manualInputAfter=(apisAfter.doc.scanned_attrs & apisAfter.doc.getNotEmptyFieldsMask()) != apisAfter.doc.getNotEmptyFieldsMask();
    if (!(apisAfter.doc.equalAttrs(apisBefore.doc) && manualInputBefore==manualInputAfter))
    {
        //��������� �� ���㬥���
        ostringstream msg;
        string id;
        msg << "DOCS: " << apisAfter.doc.logStr() << ".";
        id = (manualInputAfter?"EVT.APIS_LOG_MANUAL_INPUT":"EVT.APIS_LOG_SCANNING");
        msgs.push_back(make_pair(id, msg.str()));
    };

    manualInputBefore=(apisBefore.doco.scanned_attrs & apisBefore.doco.getNotEmptyFieldsMask()) != apisBefore.doco.getNotEmptyFieldsMask();
    manualInputAfter=(apisAfter.doco.scanned_attrs & apisAfter.doco.getNotEmptyFieldsMask()) != apisAfter.doco.getNotEmptyFieldsMask();
    if (!(apisAfter.doco.equalAttrs(apisBefore.doco) && manualInputBefore==manualInputAfter))
    {
        //��������� �� ����
        ostringstream msg;
        string id;
        msg << "DOCO: " << apisAfter.doco.logStr() << ".";
        id = (manualInputAfter?"EVT.APIS_LOG_MANUAL_INPUT":"EVT.APIS_LOG_SCANNING");
        msgs.push_back(make_pair(id, msg.str()));
    };

    CheckIn::TPaxDocaItem docaBefore[3];
    CheckIn::TDocaMap dm_before(apisBefore.doca_map); // to keep const qualifier
    docaBefore[0] = dm_before[apiDocaB];
    docaBefore[1] = dm_before[apiDocaR];
    docaBefore[2] = dm_before[apiDocaD];

    CheckIn::TPaxDocaItem docaAfter[3];
    CheckIn::TDocaMap dm_after(apisAfter.doca_map); // to keep const qualifier
    docaAfter[0] = dm_after[apiDocaB];
    docaAfter[1] = dm_after[apiDocaR];
    docaAfter[2] = dm_after[apiDocaD];

    for(int pass=0; pass<3; pass++)
    {
        if (docaAfter[pass]==docaBefore[pass]) continue;

        ostringstream msg;
        if (pass==0) msg << "DOCA(B): ";
        if (pass==1) msg << "DOCA(R): ";
        if (pass==2) msg << "DOCA(D): ";
        //��������� ����
        msg << docaAfter[pass].logStr();
        msgs.push_back(make_pair("EVT.APIS_LOG", msg.str()));
    };
};

static void SaveRemsToLog(const boost::optional<TRemGrp> &rem_grp,
                          const CheckIn::PaxRems &prior_rems,
                          const CheckIn::PaxRems &curr_rems,
                          const TLogLocale &msgPattern)
{
    TReqInfo* reqInfo = TReqInfo::Instance();

    CheckIn::PaxRems added, deleted;
    list<pair<CheckIn::TPaxRemItem, CheckIn::TPaxRemItem>> modified;

    CheckIn::GetPaxRemDifference(rem_grp, prior_rems, curr_rems, added, deleted, modified);

    for(const auto& item : modified)
    {
        LEvntPrms params(msgPattern.prms);
        params << PrmSmpl<string>("old_rem_text", item.first.text);
        params << PrmSmpl<string>("new_rem_text", item.second.text);

        reqInfo->LocaleToLog("EVT.REM_MODIFIED_FOR_PASSENGER",
                             params, ASTRA::evtPax, msgPattern.id1, msgPattern.id2, msgPattern.id3);
    }

    for(int pass=0; pass<2; pass++)
    {
        for(const CheckIn::TPaxRemItem& item : (pass==0?deleted:added))
        {
            LEvntPrms params(msgPattern.prms);
            params << PrmSmpl<string>("rem_text", item.text);

            reqInfo->LocaleToLog(pass==0?"EVT.REM_DELETED_FOR_PASSENGER":"EVT.REM_ADDED_FOR_PASSENGER",
                                 params, ASTRA::evtPax, msgPattern.id1, msgPattern.id2, msgPattern.id3);
        }
    }
}

void SaveGrpToLog(const TGrpToLogInfo &grpInfoBefore,
                  const TGrpToLogInfo &grpInfoAfter,
                  const CheckIn::TGrpEMDProps &handmadeEMDDiff,
                  TAgentStatInfo &agentStat)
{
    int point_id=       grpInfoAfter.grp_id==NoExists?grpInfoBefore.point_dep:grpInfoAfter.point_dep;
    int grp_id=         grpInfoAfter.grp_id==NoExists?grpInfoBefore.grp_id:grpInfoAfter.grp_id;
    bool piece_concept= grpInfoAfter.grp_id==NoExists?grpInfoBefore.piece_concept:grpInfoAfter.piece_concept;
    TTripInfo operFlt;
    operFlt.getByPointId(point_id);
    TGrpMktFlight markFlt;
    markFlt.getByGrpId(grp_id);

    bool SyncPaxs=is_sync_paxs(point_id);

    bool auto_weighing=GetAutoWeighing(point_id, "�");
    bool apis_control=GetAPISControl(point_id);

    TRemGrp service_stat_rem_grp;
    service_stat_rem_grp.Load(retREM_STAT, operFlt.airline);
    TRemGrp events_log_rem_grp({"MSG"});

    agentStat.clear();

    TReqInfo* reqInfo = TReqInfo::Instance();
    map< TPaxToLogInfoKey, TPaxToLogInfo>::const_iterator a=grpInfoAfter.pax.begin();
    map< TPaxToLogInfoKey, TPaxToLogInfo>::const_iterator b=grpInfoBefore.pax.begin();
    bool allGrpAgentError=true;
    for(;a!=grpInfoAfter.pax.end() || b!=grpInfoBefore.pax.end();)
    {
        map< TPaxToLogInfoKey, TPaxToLogInfo>::const_iterator aPax=grpInfoAfter.pax.end();
        map< TPaxToLogInfoKey, TPaxToLogInfo>::const_iterator bPax=grpInfoBefore.pax.end();

        if (a==grpInfoAfter.pax.end() ||
                (a!=grpInfoAfter.pax.end() && b!=grpInfoBefore.pax.end() && b->first < a->first))
        {
            bPax=b;
            ++b;
        } else
            if (b==grpInfoBefore.pax.end() ||
                    (a!=grpInfoAfter.pax.end() && b!=grpInfoBefore.pax.end() && a->first < b->first))
            {
                aPax=a;
                ++a;
            } else
                if (a!=grpInfoAfter.pax.end() && b!=grpInfoBefore.pax.end() && a->first==b->first)
                {
                    aPax=a;
                    bPax=b;
                    ++a;
                    ++b;
                };

        if (aPax!=grpInfoAfter.pax.end() && aPax->second.refuse!=refuseAgentError) allGrpAgentError=false;

        bool changed=false;
        if (aPax!=grpInfoAfter.pax.end())
        {
            bool is_crew=aPax->second.status==EncodePaxStatus(psCrew);
            bool is_unaccomp=aPax->second.orig_cl.empty() && !is_crew;
            if (aPax->second.refuse.empty())
            {
                //���ᠦ�� �� ࠧॣ����஢��
                if (bPax!=grpInfoBefore.pax.end() && bPax->second.refuse.empty())
                {
                    if (!is_unaccomp &&
                            !(aPax->second.surname==bPax->second.surname &&
                              aPax->second.name==bPax->second.name &&
                              aPax->second.pers_type==bPax->second.pers_type &&
                              aPax->second.subcl==bPax->second.subcl &&
                              aPax->second.seat_no==bPax->second.seat_no &&
                              (apis_control ||
                               (aPax->second.apis.doc.equalAttrs(bPax->second.apis.doc) &&
                                aPax->second.apis.doco.equalAttrs(bPax->second.apis.doco) &&
                                aPax->second.apis.doca_map==bPax->second.apis.doca_map)) &&
                              aPax->second.rems==bPax->second.rems))
                    {
                        //���ᠦ�� �������
                        string lexema_id = is_crew?"EVT.CHANGED_CREW_MEMBER_DATA":"EVT.CHANGED_PASSENGER_DATA";
                        LEvntPrms params;
                        aPax->second.getPaxName(params);
                        reqInfo->LocaleToLog(lexema_id, params, ASTRA::evtPax, point_id, aPax->first.reg_no, grp_id);
                        changed=true;
                    };
                    if (!(aPax->second.norms_normal==bPax->second.norms_normal))
                    {
                        if (!piece_concept)
                        {
                            LEvntPrms params;
                            aPax->second.getPaxName(params);
                            if (aPax->second.pr_mark_norms && operFlt.airline!=markFlt.airline) {
                                PrmEnum prmenum("airline", "");
                                prmenum.prms << PrmSmpl<string>("", " (") << PrmElem<string>("", etAirline, markFlt.airline)
                                             << PrmSmpl<string>("", ") ");
                                params << prmenum;
                            }
                            else
                                params << PrmSmpl<string>("airline", "");
                            PrmEnum norms("norm", "");
                            aPax->second.getNorm(norms);
                            params << norms;
                            reqInfo->LocaleToLog("EVT.BAGG_NORMS", params, ASTRA::evtPax, point_id, aPax->first.reg_no, grp_id);
                        };
                        changed=true;
                    };
                    if (!is_unaccomp && apis_control)
                    {
                        list<pair<string, string> > msgs;
                        GetAPISLogMsgs(bPax->second.apis, aPax->second.apis, msgs);
                        for(list<pair<string, string> >::const_iterator m=msgs.begin(); m!=msgs.end(); ++m)
                        {
                            LEvntPrms params;
                            aPax->second.getPaxName(params);
                            reqInfo->LocaleToLog(m->first, params << PrmSmpl<string>("params", m->second),
                                                 ASTRA::evtPax, point_id, aPax->first.reg_no, grp_id);
                            changed=true;
                        };
                    };
                }
                else
                {
                    //���ᠦ�� ��������
                    LEvntPrms params;
                    aPax->second.getPaxName(params);
                    params << PrmElem<string>("airp", etAirp, aPax->second.airp_arv);
                    if (!is_unaccomp)
                    {
                        if (aPax->second.pr_exam)
                            params << PrmLexema("exam", "EVT.EXAMED");
                        else
                            params << PrmSmpl<string>("exam", "");
                        if (aPax->second.pr_brd)
                            params << PrmLexema("board", "EVT.BOARDED");
                        else
                            params << PrmSmpl<string>("board", "");
                        if (!is_crew) {
                            PrmLexema lexema("params" ,"EVT.CLASS_STATUS_SEAT_NO");
                            lexema.prms << PrmElem<string>("cls", etClass, aPax->second.cabin_cl )
                                        << PrmElem<string>("status", etGrpStatusType, aPax->second.status, efmtNameLong);
                            if (!aPax->second.seat_no.empty())
                                lexema.prms << PrmSmpl<string>("seat_no", aPax->second.seat_no);
                            else
                                lexema.prms << PrmBool("seat_no", false);
                            if (!aPax->second.agent_seat_no.empty()) {
                                lexema.prms << PrmLexema("agent_seat_no", "EVT.AGENT_SEAT_NO");
                            }
                            else {
                                lexema.prms << PrmSmpl<string>("agent_seat_no", "");
                            }

                            params << lexema;
                        }
                        else
                            params << PrmSmpl<string>("params", "");
                    }
                    else
                    {
                        params << PrmSmpl<string>("exam", "") << PrmLexema("board", "EVT.BOARDED");
                        PrmLexema lexema("params" ,"EVT.STATUS");
                        lexema.prms << PrmElem<string>("status", etGrpStatusType, aPax->second.status, efmtNameLong);
                        params << lexema;
                    };
                    if (!piece_concept)
                    {
                        if (aPax->second.pr_mark_norms && operFlt.airline!=markFlt.airline) {
                            PrmEnum prmenum("airline", "");
                            prmenum.prms << PrmSmpl<string>("", " (") << PrmElem<string>("", etAirline, markFlt.airline)
                                         << PrmSmpl<string>("", ") ");
                            params << prmenum;
                        }
                        else
                            params << PrmSmpl<string>("airline", "");
                        PrmEnum norms("norm", "");
                        aPax->second.getNorm(norms);
                        params << norms;
                    };
                    reqInfo->LocaleToLog(piece_concept?"EVT.PASSENGER.CHECKEDIN_PC":"EVT.PASSENGER.CHECKEDIN",
                                         params, ASTRA::evtPax, point_id, aPax->first.reg_no, grp_id);

                    if (!is_unaccomp && apis_control)
                    {
                        list<pair<string, string> > msgs;
                        GetAPISLogMsgs(CheckIn::TAPISItem(), aPax->second.apis, msgs);
                        for(list<pair<string, string> >::const_iterator m=msgs.begin(); m!=msgs.end(); ++m)
                        {
                            LEvntPrms params;
                            aPax->second.getPaxName(params);
                            reqInfo->LocaleToLog(m->first, params << PrmSmpl<string>("params", m->second),
                                                 ASTRA::evtPax, point_id, aPax->first.reg_no, grp_id);
                        };
                    };

                    changed=true;
                };

                if (!is_unaccomp)
                {
                    //��������� �� ������
                    list< pair<string, LEvntPrms> > msgs;
                    if (!(bPax!=grpInfoBefore.pax.end() &&
                          bPax->second.refuse.empty() &&
                          aPax->second.tkn==bPax->second.tkn))
                    {
                        CheckIn::TPaxTknItem tknCrs;
                        LoadCrsPaxTkn(aPax->first.pax_id, tknCrs);
                        if (bPax!=grpInfoBefore.pax.end() && bPax->second.refuse.empty())
                            AddTKNLogMsgs(tknCrs, bPax->second.tkn, aPax->second.tkn, msgs);
                        else
                            AddTKNLogMsgs(tknCrs, boost::none, aPax->second.tkn, msgs);
                        changed=true;
                    }
                    if (!(bPax!=grpInfoBefore.pax.end() &&
                          bPax->second.refuse.empty() &&
                          aPax->second.fqts==bPax->second.fqts))
                    {
                        set<CheckIn::TPaxFQTItem> fqtsCrs;
                        LoadCrsPaxFQT(aPax->first.pax_id, fqtsCrs);
                        if (bPax!=grpInfoBefore.pax.end() && bPax->second.refuse.empty())
                            AddFQTLogMsgs(fqtsCrs, bPax->second.fqts, aPax->second.fqts, msgs);
                        else
                            AddFQTLogMsgs(fqtsCrs, boost::none, aPax->second.fqts, msgs);
                        changed=true;
                    }
                    for(list< pair<string, LEvntPrms> >::iterator m=msgs.begin(); m!=msgs.end(); ++m)
                    {
                        aPax->second.getPaxName(m->second);
                        reqInfo->LocaleToLog(m->first, m->second, ASTRA::evtPax, point_id, aPax->first.reg_no, grp_id);
                    }
                };
            }
            else
            {
                int id2=(aPax->second.refuse!=refuseAgentError?aPax->first.reg_no:NoExists);

                //���ᠦ�� ࠧॣ����஢��
                if (!is_unaccomp)
                {
                    if (bPax==grpInfoBefore.pax.end() || bPax->second.refuse.empty())
                    {
                        //࠭�� �� �뫨 ࠧॣ����஢���
                        std::string lexema_id;
                        LEvntPrms params;
                        aPax->second.getPaxName(params);
                        if (reqInfo->client_type!=ctTerm && aPax->second.refuse==refuseAgentError)
                            lexema_id = "EVT.UNREGISTRATION_CANCEL";
                        else {
                            lexema_id = "EVT.UNREGISTRATION_REFUSE";
                            params << PrmElem<std::string>("refuse", etRefusalType, aPax->second.refuse, efmtNameLong);
                        }
                        reqInfo->LocaleToLog(lexema_id, params, ASTRA::evtPax, point_id, id2, grp_id);
                        changed=true;
                    }
                    else
                    {
                        //࠭�� �뫨 ࠧॣ����஢���
                        if (aPax->second.refuse!=bPax->second.refuse)
                        {
                            std::string lexema_id;
                            LEvntPrms params;
                            aPax->second.getPaxName(params);
                            if (reqInfo->client_type!=ctTerm && aPax->second.refuse==refuseAgentError)
                                lexema_id = "EVT.REGISTRATION_CANCEL";
                            else {
                                lexema_id = "EVT.REGISTRATION_REFUSE";
                                params << PrmElem<std::string>("refuse", etRefusalType, aPax->second.refuse, efmtNameLong);
                            }
                            reqInfo->LocaleToLog(lexema_id, params, ASTRA::evtPax, point_id, id2, grp_id);
                            changed=true;
                        };
                    };
                }
                else
                {
                    LEvntPrms params;
                    aPax->second.getPaxName(params);
                    reqInfo->LocaleToLog("EVT.PASSENGER_DELETED", params, ASTRA::evtPax, point_id, id2, grp_id);
                    changed=true;
                };
            };
        }
        else
        {
            if (bPax==grpInfoBefore.pax.end()) continue;
            //���ᠦ�� 㤠���
            LEvntPrms params;
            bPax->second.getPaxName(params);
            reqInfo->LocaleToLog("EVT.PASSENGER_DELETED", params, ASTRA::evtPax, point_id, NoExists, grp_id);
            changed=true;
        };

        //��襬 pax_rem_origin
        if (aPax!=grpInfoAfter.pax.end() && aPax->second.refuse!=refuseAgentError)
        {
            if (bPax!=grpInfoBefore.pax.end() && bPax->second.refuse!=refuseAgentError)
            {
                CheckIn::SyncPaxRemOrigin(service_stat_rem_grp,
                                          aPax->first.pax_id,
                                          bPax->second.rems,
                                          aPax->second.rems,
                                          reqInfo->user.user_id,
                                          reqInfo->desk.code);
            }
            else
            {
                //��ࢮ��砫쭠� ॣ������
                CheckIn::PaxRems crs_rems;
                CheckIn::LoadCrsPaxRem(aPax->first.pax_id, crs_rems);
                CheckIn::SyncPaxRemOrigin(service_stat_rem_grp,
                                          aPax->first.pax_id,
                                          crs_rems,
                                          aPax->second.rems,
                                          reqInfo->user.user_id,
                                          reqInfo->desk.code);
            }
        }

        TLogLocale msgPattern;
        (aPax!=grpInfoAfter.pax.end()?aPax->second.getPaxName(msgPattern.prms):bPax->second.getPaxName(msgPattern.prms));
        msgPattern.id1=point_id;
        msgPattern.id2=((aPax!=grpInfoAfter.pax.end() && aPax->second.refuse!=refuseAgentError)?aPax->first.reg_no:NoExists);
        msgPattern.id3=grp_id;
        //��������� � ६�ઠ� ���ᠦ�஢
        const CheckIn::PaxRems& remsBefore=(bPax!=grpInfoBefore.pax.end() && bPax->second.refuse!=refuseAgentError)?bPax->second.rems:CheckIn::PaxRems();
        const CheckIn::PaxRems& remsAfter=(aPax!=grpInfoAfter.pax.end() && aPax->second.refuse!=refuseAgentError)?aPax->second.rems:CheckIn::PaxRems();
        SaveRemsToLog(events_log_rem_grp, remsBefore, remsAfter, msgPattern);

        string bagStrAfter, bagStrBefore;
        int d=0;
        if (aPax!=grpInfoAfter.pax.end() && aPax->second.refuse!=refuseAgentError)
        {
            bagStrAfter=aPax->second.getBagStr();
            d++;
        };
        if (bPax!=grpInfoBefore.pax.end() && bPax->second.refuse!=refuseAgentError)
        {
            bagStrBefore=bPax->second.getBagStr();
            d--;
        };
        if (d>0) agentStat.dpax_amount.inc++;
        if (d<0) agentStat.dpax_amount.dec++;
        if (changed) agentStat.pax_amount++;
        if (bagStrAfter!=bagStrBefore)
        {
            //����� ���������
            string lexema_id;
            LEvntPrms params;
            PrmEnum prmenum("bag", "");
            (aPax!=grpInfoAfter.pax.end()?aPax->second.getPaxName(params):bPax->second.getPaxName(params));

            if (bagStrAfter.empty()) {
                lexema_id = "EVT.LUGGAGE_DELETED";
                bPax->second.getBag(prmenum);
            }
            else if (bagStrBefore.empty()) {
                lexema_id = "EVT.LUGGAGE_ADDED";
                aPax->second.getBag(prmenum);
            }
            else if (!bagStrAfter.empty() && !bagStrBefore.empty()) {
                lexema_id = "EVT.LUGGAGE_MODIFIED";
                aPax->second.getBag(prmenum);
            }
            params << prmenum;
            reqInfo->LocaleToLog(lexema_id, params, ASTRA::evtPax, point_id,
                                 (aPax!=grpInfoAfter.pax.end() && aPax->second.refuse!=refuseAgentError)?aPax->first.reg_no:NoExists, grp_id);
            changed=true;
        };

        const TPaidToLogInfo &paidBefore=(bPax!=grpInfoBefore.pax.end() && bPax->second.refuse!=refuseAgentError)?bPax->second.paid:TPaidToLogInfo();
        const TPaidToLogInfo &paidAfter=(aPax!=grpInfoAfter.pax.end() && aPax->second.refuse!=refuseAgentError)?aPax->second.paid:TPaidToLogInfo();
        SavePaidToLog(paidBefore, paidAfter, msgPattern, piece_concept,
                      !(aPax!=grpInfoAfter.pax.end() && aPax->second.refuse!=refuseAgentError),
                      handmadeEMDDiff);

        if (SyncPaxs)
        {
            int aodb_pax_id=NoExists;
            int aodb_reg_no=NoExists;
            if (aPax!=grpInfoAfter.pax.end())
            {
                aodb_pax_id=aPax->first.pax_id;
                aodb_reg_no=aPax->first.reg_no;
            }
            else
            {
                aodb_pax_id=bPax->first.pax_id;
                aodb_reg_no=bPax->first.reg_no;
            };
            if (aodb_pax_id!=NoExists && aodb_reg_no!=NoExists)
            {
                if (changed) //�뫨 ��������� �� ॣ����樨
                    updatePaxChange( operFlt, PaxId_t(aodb_pax_id), RegNo_t(aodb_reg_no), TermWorkingMode::CheckIn );

                bool boardedAfter=false, boardedBefore=false;
                if (aPax!=grpInfoAfter.pax.end())
                    boardedAfter=aPax->second.pr_brd;
                if (bPax!=grpInfoBefore.pax.end())
                    boardedBefore=bPax->second.pr_brd;
                if (boardedAfter!=boardedBefore) //�뫨 ��������� � ��ᠤ���/��ᠤ���
                    updatePaxChange( operFlt, PaxId_t(aodb_pax_id), RegNo_t(aodb_reg_no), TermWorkingMode::Boarding );
            };
        };
    };
    //����� �������� ������ (using_scales=false):
    map<bool/*pr_cabin*/, map< int/*id*/, TEventsBagItem> > man_weighing_bag_inc, man_weighing_bag_dec;
    //�����᪠� ����⨪� �� ��������� ������
    {
        std::map< int/*id*/, TEventsBagItem>::const_iterator a=grpInfoAfter.bag.begin();
        std::map< int/*id*/, TEventsBagItem>::const_iterator b=grpInfoBefore.bag.begin();
        for(;a!=grpInfoAfter.bag.end() || b!=grpInfoBefore.bag.end();)
        {
            std::map< int/*id*/, TEventsBagItem>::const_iterator aBag=grpInfoAfter.bag.end();
            std::map< int/*id*/, TEventsBagItem>::const_iterator bBag=grpInfoBefore.bag.end();

            if (a==grpInfoAfter.bag.end() ||
                    (a!=grpInfoAfter.bag.end() && b!=grpInfoBefore.bag.end() && b->first < a->first))
            {
                bBag=b;
                ++b;
            } else
                if (b==grpInfoBefore.bag.end() ||
                        (a!=grpInfoAfter.bag.end() && b!=grpInfoBefore.bag.end() && a->first < b->first))
                {
                    aBag=a;
                    ++a;
                } else
                    if (a!=grpInfoAfter.bag.end() && b!=grpInfoBefore.bag.end() && a->first==b->first)
                    {
                        if (!(a->second==b->second))
                        {
                            aBag=a;
                            bBag=b;
                        };
                        ++a;
                        ++b;
                    };

            if (aBag!=grpInfoAfter.bag.end())
            {
                if (aBag->second.pr_cabin)
                {
                    agentStat.drk_amount.inc+=aBag->second.amount;
                    agentStat.drk_weight.inc+=aBag->second.weight;
                }
                else
                {
                    agentStat.dbag_amount.inc+=aBag->second.amount;
                    agentStat.dbag_weight.inc+=aBag->second.weight;
                };
                if (auto_weighing && !aBag->second.using_scales)
                    man_weighing_bag_inc[aBag->second.pr_cabin].insert(*aBag);
            };

            if (bBag!=grpInfoBefore.bag.end())
            {
                if (bBag->second.pr_cabin)
                {
                    agentStat.drk_amount.dec+=bBag->second.amount;
                    agentStat.drk_weight.dec+=bBag->second.weight;
                }
                else
                {
                    agentStat.dbag_amount.dec+=bBag->second.amount;
                    agentStat.dbag_weight.dec+=bBag->second.weight;
                };
                if (auto_weighing && !bBag->second.using_scales)
                    man_weighing_bag_dec[bBag->second.pr_cabin].insert(*bBag);
            };

        };
    };

    for(int pass=0; pass<2; pass++)
    {
        map<bool/*pr_cabin*/, map< int/*id*/, TEventsBagItem> >::const_iterator i=
                (pass==0)?man_weighing_bag_dec.begin():man_weighing_bag_inc.begin();
        for(;i!=(pass==0?man_weighing_bag_dec.end():man_weighing_bag_inc.end());++i)
        {
            if (i->second.empty()) continue;

            PrmEnum prmenum("bag", ", ");
            for(map< int/*id*/, TEventsBagItem>::const_iterator j=i->second.begin();
                j!=i->second.end(); ++j)
            {
                ostringstream msg;
                if (!j->second.key_str(LANG_EN).empty())
                    msg << j->second.key_str(LANG_EN) << ":";
                if (j->second.is_trfer)
                    msg << "T:";
                msg << j->second.amount << "/" << j->second.weight;
                prmenum.prms << PrmSmpl<std::string>("", msg.str());
            };

            if (pass==0)
            {
                if (i->first) //pr_cabin
                    reqInfo->LocaleToLog("EVT.REMOVE_CABIN_LUGGAGE_MAN_ENTERED_WEIGHT", LEvntPrms() << prmenum,
                                         ASTRA::evtPax, point_id, ASTRA::NoExists, grp_id);
                else
                    reqInfo->LocaleToLog("EVT.REMOVE_LUGGAGE_MAN_ENTERED_WEIGHT", LEvntPrms() << prmenum,
                                         ASTRA::evtPax, point_id, ASTRA::NoExists, grp_id);
            }
            else
            {
                if (i->first) //pr_cabin
                    reqInfo->LocaleToLog("EVT.ENTER_CABIN_LUGGAGE_MAN_ENTERED_WEIGHT", LEvntPrms() << prmenum,
                                         ASTRA::evtPax, point_id, ASTRA::NoExists, grp_id);
                else
                    reqInfo->LocaleToLog("EVT.ENTER_LUGGAGE_MAN_ENTERED_WEIGHT", LEvntPrms() << prmenum,
                                         ASTRA::evtPax, point_id, ASTRA::NoExists, grp_id);
            };
        };
    };

    TLogLocale msg;
    msg.id1=point_id;
    msg.id2=ASTRA::NoExists;
    msg.id3=grp_id;
    SavePaidToLog(grpInfoBefore.paid, grpInfoAfter.paid, msg, piece_concept, allGrpAgentError, handmadeEMDDiff);
};

void SavePaidToLog(const TPaidToLogInfo &paidBefore,
                   const TPaidToLogInfo &paidAfter,
                   const TLogLocale &msgPattern,
                   bool piece_concept,
                   bool onlyEMD,
                   const CheckIn::TGrpEMDProps &handmadeEMDDiff)
{
    //  paidBefore.trace(TRACE5, "SavePaidToLog: paidBefore");
    //  paidAfter.trace(TRACE5, "SavePaidToLog: paidAfter");

    TReqInfo* reqInfo = TReqInfo::Instance();
    for(int pass=0; pass<2; pass++)
    {
        multiset<CheckIn::TServicePaymentItem> payment;
        if (pass==0)
            set_difference(paidBefore.payment.begin(), paidBefore.payment.end(),
                           paidAfter.payment.begin(), paidAfter.payment.end(),
                           inserter(payment, payment.end()));
        else
            set_difference(paidAfter.payment.begin(), paidAfter.payment.end(),
                           paidBefore.payment.begin(), paidBefore.payment.end(),
                           inserter(payment, payment.end()));
        multiset<CheckIn::TServicePaymentItem> paymentByType[3];
        for(multiset<CheckIn::TServicePaymentItem>::const_iterator p=payment.begin(); p!=payment.end(); ++p)
            if (p->isEMD())
            {
                if (handmadeEMDDiff.find(CheckIn::TGrpEMDPropsItem(*p))==handmadeEMDDiff.end())
                    paymentByType[0].insert(*p); //��⮬���᪨� EMD
                else
                    paymentByType[1].insert(*p);
            }
            else paymentByType[2].insert(*p);

        for(int i=0; i<3; i++)
        {
            const multiset<CheckIn::TServicePaymentItem> &paymentRef=paymentByType[i];
            if (!paymentRef.empty())
            {
                PrmEnum prmenum((i<2?"emd":"mco"), "");
                for(multiset<CheckIn::TServicePaymentItem>::const_iterator p=paymentRef.begin(); p!=paymentRef.end(); ++p)
                {
                    std::ostringstream msg;
                    if (p!=paymentRef.begin()) prmenum.prms << PrmSmpl<string>("", ", ");
                    if (!p->key_str(LANG_EN).empty())
                        msg << p->key_str(LANG_EN) << ":";
                    msg << p->no_str();
                    prmenum.prms << PrmSmpl<string>("", msg.str());
                    if (p->doc_weight!=ASTRA::NoExists)
                        prmenum.prms << PrmSmpl<string>("", ":") << PrmSmpl<int>("", p->doc_weight) << PrmLexema("", "EVT.KG");
                };
                LEvntPrms params(msgPattern.prms);
                params << prmenum;
                if (i==0)
                    reqInfo->LocaleToLog(msgPattern.prms.empty()?(pass==0?"EVT.EMD_DELETED_AUTOMATICALLY":"EVT.EMD_ADDED_AUTOMATICALLY"):
                                                                 (pass==0?"EVT.EMD_DELETED_AUTOMATICALLY_FOR_PASSENGER":"EVT.EMD_ADDED_AUTOMATICALLY_FOR_PASSENGER"),
                                         params, ASTRA::evtPay, msgPattern.id1, msgPattern.id2, msgPattern.id3);
                if (i==1)
                    reqInfo->LocaleToLog(msgPattern.prms.empty()?(pass==0?"EVT.EMD_DELETED":"EVT.EMD_ADDED"):
                                                                 (pass==0?"EVT.EMD_DELETED_FOR_PASSENGER":"EVT.EMD_ADDED_FOR_PASSENGER"),
                                         params, ASTRA::evtPay, msgPattern.id1, msgPattern.id2, msgPattern.id3);
                if (i==2)
                    reqInfo->LocaleToLog(msgPattern.prms.empty()?(pass==0?"EVT.MCO_DELETED":"EVT.MCO_ADDED"):
                                                                 (pass==0?"EVT.MCO_DELETED_FOR_PASSENGER":"EVT.MCO_ADDED_FOR_PASSENGER"),
                                         params, ASTRA::evtPay, msgPattern.id1, msgPattern.id2, msgPattern.id3);
            };
        };
    };

    if (onlyEMD) return;

    if (paidBefore.bag==paidAfter.bag &&
            paidBefore.excess_wt==paidAfter.excess_wt &&
            paidBefore.excess_pc==paidAfter.excess_pc) return;

    LEvntPrms lprms(msgPattern.prms);
    lprms << PrmSmpl<int>(piece_concept?"pieces":"weight", piece_concept?paidAfter.excess_pc.getQuantity():
                                                                         paidAfter.excess_wt.getQuantity());

    if (!paidAfter.bag.empty())
    {
        PrmEnum prmenum("bag", ", ");
        for(map<TEventsSumBagKey, TEventsSumBagItem>::const_iterator b=paidAfter.bag.begin(); b!=paidAfter.bag.end(); ++b)
        {
            if (b->second.empty()) continue;
            ostringstream msg;
            if (!b->first.bag_type_view.empty())
                msg << b->first.bag_type_view << ":";
            if (b->first.is_trfer)
                msg << "T:";
            msg << b->second.amount << "/" << b->second.weight << "/" << b->second.paid;
            prmenum.prms << PrmSmpl<std::string>("", msg.str());
        };
        lprms << prmenum;
    }
    else lprms << PrmBool("bag", false);

    reqInfo->LocaleToLog(msgPattern.prms.empty()?(piece_concept?"EVT.LUGGAGE.PAID_PIECES":"EVT.LUGGAGE.PAID_WEIGHT"):
                                                 (piece_concept?"EVT.LUGGAGE.PAID_PIECES_FOR_PASSENGER":"EVT.LUGGAGE.PAID_WEIGHT_FOR_PASSENGER"),
                         lprms, ASTRA::evtPax, msgPattern.id1, msgPattern.id2, msgPattern.id3);
}

//�㭪�� �� ⮫쪮 �����頥� auto_weighing ��� ����,
//�� � ���� � ���, �᫨ ��� ������� ���� ���������� ����ன��
bool GetAutoWeighing(int point_id, const string &work_mode)
{
    TReqInfo* reqInfo = TReqInfo::Instance();
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText=
            "SELECT stations.using_scales "
            "FROM stations "
            "WHERE stations.desk=:desk AND stations.work_mode=:work_mode";
    Qry.CreateVariable("desk", otString, reqInfo->desk.code);
    Qry.CreateVariable("work_mode", otString, work_mode);
    Qry.Execute();
    bool auto_weighing=false;
    if (!Qry.Eof)
        auto_weighing=TTripSetList().fromDB(point_id).value(tsAutoWeighing, false) &&
                Qry.FieldAsInteger("using_scales")!=0;

    Qry.Clear();
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("desk", otString, reqInfo->desk.code);
    if (auto_weighing)
    {
        Qry.SQLText=
                "INSERT INTO trip_auto_weighing(point_id, desk) VALUES(:point_id, :desk)";
        try
        {
            Qry.Execute();
            if (Qry.RowsProcessed()>0)
                reqInfo->LocaleToLog("EVT.SET_LUGGAGE_AUTO_WEIGHTING_CONTROL",
                                     LEvntPrms() << PrmSmpl<std::string>("desk", reqInfo->desk.code), ASTRA::evtFlt, point_id);
        }
        catch(const EOracleError& E)
        {
            if (E.Code!=1) throw;
        };
    }
    else
    {
        Qry.SQLText=
                "DELETE FROM trip_auto_weighing WHERE point_id=:point_id AND desk=:desk";
        Qry.Execute();
        if (Qry.RowsProcessed()>0)
            reqInfo->LocaleToLog("EVT.CANCEL_LUGGAGE_AUTO_WEIGHTING_CONTROL",
                                 LEvntPrms() << PrmSmpl<std::string>("desk", reqInfo->desk.code), ASTRA::evtFlt, point_id);
    };

    return auto_weighing;
};

bool GetAPISControl(int point_id)
{
    return TTripSetList().fromDB(point_id).value(tsAPISControl, false);
};
