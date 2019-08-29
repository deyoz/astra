#include "stat_departed.h"
#include "astra_utils.h"
#include "dev_consts.h"
#include "stat_common.h"
#include "passenger.h"
#include "stat_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;
using namespace BASIC::date_time;

void departed_flt(TQuery &Qry, TEncodedFileStream &of)
{
    int point_id = Qry.FieldAsInteger("point_id");
    TDateTime part_key = NoExists;

    if(not Qry.FieldIsNULL("part_key"))
        part_key = Qry.FieldAsDateTime("part_key");

    TRegEvents events;
    events.fromDB(part_key, point_id);

    TQuery paxQry(&OraSession);
    paxQry.CreateVariable("point_id", otInteger, point_id);
    if(part_key != NoExists)
        paxQry.CreateVariable("part_key", otDate, part_key);

    string SQLText =
        "select \n"
        "   pax.name, \n"
        "   pax.surname, \n"
        "   pax.ticket_no, \n"
        "   pax.coupon_no, \n"
        "   pax.pax_id, \n"
        "   pax.grp_id, \n"
        "   pax.reg_no, \n"
        "   pax_grp.client_type, \n"
        "   pax_grp.airp_arv, \n";
    if(part_key == NoExists) {
        SQLText +=
            "   (SELECT 1 FROM confirm_print cnf  "
            "   WHERE " OP_TYPE_COND("op_type")" and cnf.pax_id=pax.pax_id AND voucher is null and"
            "   client_type='TERM' AND pr_print<>0 AND rownum<2) AS term_bp, "
            "   salons.get_seat_no(pax.pax_id,pax.seats,NULL,NULL,NULL,'list',NULL,0) AS seat_no, "
            "   NVL(ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) bag_amount, \n"
            "   NVL(ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) bag_weight \n";
        paxQry.CreateVariable("op_type", otString, DevOperTypes().encode(TDevOper::PrnBP));
    } else
    SQLText +=
          " NVL(arch.get_bagAmount2(pax.part_key,pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) bag_amount, \n"
          " NVL(arch.get_bagWeight2(pax.part_key,pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) bag_weight \n";
    SQLText +=
        "from \n";
    if(part_key == NoExists)
        SQLText +=
            "   pax, pax_grp \n";
    else
        SQLText +=
            "   arx_pax pax, arx_pax_grp pax_grp \n";
    SQLText +=
        "where \n"
        "   pax_grp.point_dep = :point_id and \n"
        "   pax_grp.grp_id = pax.grp_id \n";
    if(part_key != NoExists)
        SQLText +=
            " and pax.part_key = :part_key and \n"
            " pax_grp.part_key = :part_key \n";

    string delim = ";";

    paxQry.SQLText = SQLText;
    paxQry.Execute();
    TAirpArvInfo airp_arv_info;
    for(; not paxQry.Eof; paxQry.Next()) {
        int grp_id = paxQry.FieldAsInteger("grp_id");
        int reg_no = paxQry.FieldAsInteger("reg_no");
        string name = paxQry.FieldAsString("name");
        string surname = paxQry.FieldAsString("surname");

        string airline = Qry.FieldAsString("airline");
        int flt_no = Qry.FieldAsInteger("flt_no");
        string suffix = Qry.FieldAsString("suffix");
        string airp = Qry.FieldAsString("airp");
        TDateTime scd_out = Qry.FieldAsDateTime("scd_out");
        ostringstream flt_str;
        flt_str
            << airline
            << setw(3) << setfill('0') << flt_no
            << (suffix.empty() ? "" : suffix);

        string ticket_no = paxQry.FieldAsString("ticket_no");
        string coupon_no = paxQry.FieldAsString("coupon_no");
        ostringstream ticket;
        if(not ticket_no.empty())
            ticket << ticket_no << (coupon_no.empty() ? "" : "/") << coupon_no;

        string pnr_addr;
        if(part_key == NoExists)
          pnr_addr=TPnrAddrs().firstAddrByPaxId(paxQry.FieldAsInteger("pax_id"), TPnrAddrInfo::AddrAndAirline);

        CheckIn::TPaxDocItem doc;
        LoadPaxDoc(part_key, paxQry.FieldAsInteger("pax_id"), doc);
        string birth_date = (doc.birth_date!=ASTRA::NoExists?DateTimeToStr(doc.birth_date, "dd.mmm.yy"):"");
        string gender = doc.gender;

        of
            // ФИО
            << name << (name.empty() ? "" : " ") << surname << delim
            // Дата рождения
            << birth_date << delim
            // Пол
            << gender << delim
            // Тип документа
            << doc.type << delim
            // Серия и номер документа
            << doc.no << delim
            // Номер билета
            << ticket.str() << delim
            // Номер бронирования
            << pnr_addr << delim
            // Рейс
            << flt_str.str() << delim
            // Дата вылета
            << DateTimeToStr(scd_out, "ddmmm") << delim
            // От
            << airp << delim
            // До
            << airp_arv_info.get(paxQry) << delim
            // Багаж мест
            << paxQry.FieldAsInteger("bag_amount") << delim
            // Багаж вес
            << paxQry.FieldAsInteger("bag_weight") << delim;
        // Время регистрации
        TRegEvents::const_iterator evt = events.find(make_pair(grp_id, reg_no));
        if(evt != events.end())
            of << DateTimeToStr(evt->second.first, "dd.mm.yyyy hh:nn:ss");
        of << delim;
        // Номер места
        if(part_key == NoExists)
            of << paxQry.FieldAsString("seat_no");
        of << delim
            // Способ регистрации
            << paxQry.FieldAsString("client_type") << delim;
        // Печать ПТ на стойке
        if(part_key == NoExists)
            of << (paxQry.FieldAsInteger("term_bp") == 0 ? "НЕТ" : "ДА");
        of << endl;
    }

    /*
       ofstream out(IntToString(part_key == NoExists ? 0 : 1) + "pax1.sql");
       out << SQLText;
       out.close();
       */
}

void departed_month(const pair<TDateTime, TDateTime> &interval, TEncodedFileStream &of)
{
    TQuery Qry(&OraSession);
    Qry.CreateVariable("first_date", otDate, interval.first);
    Qry.CreateVariable("last_date", otDate, interval.second);
    for(int pass = 0; pass <= 2; pass++) {
        if (pass!=0)
            Qry.CreateVariable("arx_trip_date_range", otInteger, ARX_TRIP_DATE_RANGE());
        ostringstream sql;
        sql << "SELECT ";
        if(pass)
            sql << "points.part_key, ";
        else
            sql << "null part_key, ";
        sql << "point_id, airline, flt_no, suffix, airp, scd_out  FROM ";
        if (pass!=0)
        {
            sql << " arx_points points \n";
            if (pass==2)
                sql << ",(SELECT part_key, move_id FROM move_arx_ext \n"
                    "  WHERE part_key >= :last_date+:arx_trip_date_range AND part_key <= :last_date+date_range) arx_ext \n";
        }
        else
            sql << " points \n";
        sql << "WHERE \n";
        if (pass==1)
            sql << " points.part_key >= :first_date AND points.part_key < :last_date + :arx_trip_date_range AND \n";
        if (pass==2)
            sql << " points.part_key=arx_ext.part_key AND points.move_id=arx_ext.move_id AND \n";
        sql <<
            "   scd_out>=:first_date AND scd_out<:last_date AND airline='ЮТ' AND "
            "      pr_reg<>0 AND pr_del>=0";

        /*
           ofstream out(IntToString(pass) + ".sql");
           out << sql.str();
           out.close();
           */

        Qry.SQLText = sql.str().c_str();
        Qry.Execute();
        for(; not Qry.Eof; Qry.Next())
            departed_flt(Qry, of);
    }
}

int nosir_departed(int argc, char **argv)
{
    if(argc != 3) {
        cout << "usage: " << argv[0] << " yyyymmdd yyyymmdd" << endl;
        return 1;
    }
    TPerfTimer tm;
    tm.Init();
    TDateTime FirstDate, LastDate;

    if(StrToDateTime(argv[1], "yyyymmdd", FirstDate) == EOF) {
        cout << "wrong first date: " << argv[1] << endl;
        return 1;
    }

    if(StrToDateTime(argv[2], "yyyymmdd", LastDate) == EOF) {
        cout << "wrong last date: " << argv[1] << endl;
        return 1;
    }

    TPeriods period;
    period.get(FirstDate, LastDate);
    string delim = ";";
    for(TPeriods::TItems::iterator i = period.items.begin(); i != period.items.end(); i++) {
        TEncodedFileStream of("cp1251", (string)"departed." + DateTimeToStr(i->first, "yymm") + ".csv");
        of
            << "ФИО" << delim
            << "Дата рождения" << delim
            << "Пол" << delim
            << "Тип документа" << delim
            << "Серия и номер документа" << delim
            << "Номер билета" << delim
            << "Номер бронирования" << delim
            << "Рейс" << delim
            << "Дата вылета" << delim
            << "От" << delim
            << "До" << delim
            << "Багаж мест" << delim
            << "Багаж вес" << delim
            << "Время регистрации (UTC)" << delim
            << "Номер места" << delim
            << "Способ регистрации" << delim
            << "Печать ПТ на стойке" << endl;
        departed_month(*i, of);
    }
    LogTrace(TRACE5) << "time: " << tm.PrintWithMessage();
    return 1;
}

int nosir_departed_pax(int argc, char **argv)
{
    TDateTime FirstDate, LastDate;
    StrToDateTime("01.04.2016 00:00:00", "dd.mm.yyyy hh:nn:ss", FirstDate);
    LastDate = NowUTC();
    TQuery Qry(&OraSession);
    Qry.SQLText =
    "SELECT point_id, airline, flt_no, suffix, airp, scd_out  FROM points "
    "WHERE scd_out>=:FirstDate AND scd_out<:LastDate AND airline='ЮТ' AND "
    "      pr_reg<>0 AND pr_del>=0";
    Qry.CreateVariable("FirstDate", otDate, FirstDate);
    Qry.CreateVariable("LastDate", otDate, LastDate);

    TQuery paxQry(&OraSession);
    paxQry.SQLText =
        "select "
        "   pax.name, "
        "   pax.surname, "
        "   pax.ticket_no, "
        "   pax.coupon_no, "
        "   crs_pnr.pnr_id, "
        "   pax.pax_id "
        "from "
        "   pax, pax_grp, crs_pnr, crs_pax where "
        "   pax.pax_id = crs_pax.pax_id(+) and "
        "   crs_pax.pr_del(+)=0 and "
        "   crs_pax.pnr_id = crs_pnr.pnr_id(+) and "
        "   pax_grp.point_dep = :point_id and "
        "   pax_grp.grp_id = pax.grp_id ";
    paxQry.DeclareVariable("point_id", otInteger);

    Qry.Execute();
    bool pr_header = true;
    string delim = ";";
    ofstream of;
    TTripRoute route;
    for(; not Qry.Eof; Qry.Next()) {
        int point_id = Qry.FieldAsInteger("point_id");
        route.GetRouteAfter(NoExists, point_id, trtNotCurrent, trtNotCancelled);
        string airp_arv;
        if(not route.empty())
            airp_arv = route.begin()->airp;

        paxQry.SetVariable("point_id", point_id);
        paxQry.Execute();
        for(; not paxQry.Eof; paxQry.Next()) {
            if(pr_header) {
                pr_header = false;
                of.open("pax_departed.csv");
                of
                    << "ФИО" << delim
                    << "Номер билета" << delim
                    << "Номер бронирования" << delim
                    << "Рейс" << delim
                    << "Дата вылета" << delim
                    << "Направление" << delim
                    << "Дата рождения" << delim
                    << "Пол" << endl;
            }
            string name = paxQry.FieldAsString("name");
            string surname = paxQry.FieldAsString("surname");

            string airline = Qry.FieldAsString("airline");
            int flt_no = Qry.FieldAsInteger("flt_no");
            string suffix = Qry.FieldAsString("suffix");
            string airp = Qry.FieldAsString("airp");
            TDateTime scd_out = Qry.FieldAsDateTime("scd_out");
            ostringstream flt_str;
            flt_str
                << airline
                << setw(3) << setfill('0') << flt_no
                << (suffix.empty() ? "" : suffix);

            string ticket_no = paxQry.FieldAsString("ticket_no");
            string coupon_no = paxQry.FieldAsString("coupon_no");
            ostringstream ticket;
            if(not ticket_no.empty())
                ticket << ticket_no << (coupon_no.empty() ? "" : "/") << coupon_no;

            string pnr_addr=TPnrAddrs().firstAddrByPnrId(paxQry.FieldAsInteger("pnr_id"), TPnrAddrInfo::AddrAndAirline);

            CheckIn::TPaxDocItem doc;
            LoadPaxDoc(paxQry.FieldAsInteger("pax_id"), doc);
            string birth_date = (doc.birth_date!=ASTRA::NoExists?DateTimeToStr(doc.birth_date, "dd.mmm.yy"):"");
            string gender = doc.gender;

            of
                // ФИО
                << name << (name.empty() ? "" : " ") << surname << delim
                // Дата рождения
                << birth_date << delim
                // Пол
                << gender << delim
                // Тип документа
                << delim
                // Серия документа
                << delim
                // Номер документа
                << delim
                // Номер билета
                << ticket.str() << delim
                // Номер бронирования
                << pnr_addr << delim
                // Рейс
                << flt_str.str() << delim
                // Дата вылета
                << DateTimeToStr(scd_out, "ddmmm") << delim
                // От
                << delim
                // До
                << route.begin()->airp << delim;
        }
    }
    return 1;
}

int nosir_departed_sql(int argc, char **argv)
{
    TQuery Qry(&OraSession);
    Qry.SQLText = "select :part_key part_key from dual";
    Qry.CreateVariable("part_key", otDate, FNull);
    Qry.Execute();
//    departed_flt(Qry);
    Qry.SetVariable("part_key", NowUTC());
    Qry.Execute();
//    departed_flt(Qry);
    return 1;
}
