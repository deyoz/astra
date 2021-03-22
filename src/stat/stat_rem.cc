#include "stat_rem.h"
#include "qrys.h"
#include "report_common.h"
#include "stat/stat_utils.h"
#include "astra_misc.h"
#include "passenger.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

template void RunRemStat(TStatParams const&, TOrderStatWriter&, TPrintAirline&);
template void RunRemStat(TStatParams const&, TRemStat&, TPrintAirline&);

using namespace std;
using namespace BASIC::date_time;
using namespace ASTRA;
using namespace AstraLocale;

void TRemStatRow::add_header(ostringstream &buf) const
{
    buf
        << "�� ॣ." << delim
        << "�⮩��" << delim
        << "�����" << delim
        << "�����" << delim
        << "���" << delim
        << "����" << delim
        << "��" << delim
        << "��" << delim
        << "��� ��" << delim
        << "�६� � ���" << delim
        << "��� ��㣨" << delim
        << "RFISC" << delim
        << "����" << delim
        << "�����"
        << endl;
}

void TRemStatRow::add_data(ostringstream &buf) const
{
    buf
        // �� ॣ
        << ElemIdToCodeNative(etAirp, airp) << delim
        // �⮩��
        << desk << delim
        // �����
        << user << delim
        // �����
        << ticket_no << delim
        // ���
        << DateTimeToStr(scd_out, "dd.mm.yyyy") << delim;
    // ����
    ostringstream flt_no_str;
    flt_no_str << setw(3) << setfill('0') << flt_no << ElemIdToCodeNative(etSuffix, suffix);
    buf
        << flt_no_str.str() << delim
        // ��
        << ElemIdToCodeNative(etAirp, airp) << delim
        // ��
        << ElemIdToCodeNative(etAirp, airp_last) << delim
        // ��� ��
        << ElemIdToCodeNative(etCraft, craft) << delim;
    // �६� � ���
    if(travel_time != NoExists)
        buf << DateTimeToStr(travel_time, "hh:nn");
    buf << delim
        // ��� ��㣨
        << rem_code << delim
        // RFISC
        << rfisc << delim
        // ����
        << rate_str() << delim
        // �����
        << ElemIdToCodeNative(etCurrency, rate_cur)
        << endl;
}

string TRemStatRow::rate_str() const
{
    ostringstream result;
    if(rate != NoExists) {
        result << fixed << setprecision(2) << setfill('0') << rate;
    }
    return result.str();
}


template <class T>
void ArxRemStat(const TStatParams &params, T &RemStat,TPrintAirline &prn_airline)
{
    tst();
        QParams QryParams;
        QryParams
            << QParam("FirstDate", otDate, params.FirstDate)
            << QParam("LastDate", otDate, params.LastDate);
        string SQLText =
            "select "
            "   arx_points.point_id, "
            "   arx_points.scd_out, "
            "   arx_points.flt_no, "
            "   arx_points.suffix, "
            "   arx_points.airp, "
            "   arx_points.airline, "
            "   arx_points.craft, "
            "   asr.airp_last, "
            "   asr.travel_time, "
            "   asr.rem_code, "
            "   asr.ticket_no, "
            "   asr.desk, "
            "   asr.rfisc, "
            "   asr.rate, "
            "   asr.rate_cur, "
            "   asr.user_id " //for joining with users2 table after split for queries for pg
            "from  arx_stat_rem asr, arx_points "
            "where asr.point_id = arx_points.point_id and ";
        params.AccessClause(SQLText, "arx_points");
        if(params.flt_no != NoExists) {
            SQLText += " arx_points.flt_no = :flt_no and ";
            QryParams << QParam("flt_no", otInteger, params.flt_no);
        }
        SQLText +=
            " arx_points.part_key >= :FirstDate and arx_points.part_key < :LastDate and "
            " asr.part_key >= :FirstDate and asr.part_key < :LastDate and ";

        UsersReader::Instance().updateUsers();

        DB::TCachedQuery Qry(PgOra::getROSession("arx_points"), SQLText, QryParams);
        Qry.get().Execute();
        if(not Qry.get().Eof) {
            int col_point_id = Qry.get().GetFieldIndex("point_id");
            int col_ticket_no = Qry.get().GetFieldIndex("ticket_no");
            int col_scd_out = Qry.get().GetFieldIndex("scd_out");
            int col_flt_no = Qry.get().GetFieldIndex("flt_no");
            int col_suffix = Qry.get().GetFieldIndex("suffix");
            int col_airp = Qry.get().GetFieldIndex("airp");
            int col_airp_last = Qry.get().GetFieldIndex("airp_last");
            int col_craft = Qry.get().GetFieldIndex("craft");
            int col_travel_time = Qry.get().GetFieldIndex("travel_time");
            int col_rem_code = Qry.get().GetFieldIndex("rem_code");
            int col_airline = Qry.get().GetFieldIndex("airline");
            int col_desk = Qry.get().GetFieldIndex("desk");
            int col_rfisc = Qry.get().GetFieldIndex("rfisc");
            int col_rate = Qry.get().GetFieldIndex("rate");
            int col_rate_cur = Qry.get().GetFieldIndex("rate_cur");
            int col_user_id = Qry.get().GetFieldIndex("user_id");
            for(; not Qry.get().Eof; Qry.get().Next()) {
                int stat_rem_user_id = Qry.get().FieldAsInteger(col_user_id);
                prn_airline.check(Qry.get().FieldAsString(col_airline));
                TRemStatRow row;
                row.point_id = Qry.get().FieldAsInteger(col_point_id);
                row.ticket_no = Qry.get().FieldAsString(col_ticket_no);
                row.scd_out = Qry.get().FieldAsDateTime(col_scd_out);
                row.flt_no = Qry.get().FieldAsInteger(col_flt_no);
                row.suffix = Qry.get().FieldAsString(col_suffix);
                row.airp = Qry.get().FieldAsString(col_airp);
                row.airp_last = Qry.get().FieldAsString(col_airp_last);
                row.craft = Qry.get().FieldAsString(col_craft);
                if(not Qry.get().FieldIsNULL(col_travel_time))
                    row.travel_time = Qry.get().FieldAsDateTime(col_travel_time);
                row.rem_code = Qry.get().FieldAsString(col_rem_code);
                row.user = UsersReader::Instance().getDescr(stat_rem_user_id);
                row.desk = Qry.get().FieldAsString(col_desk);
                row.rfisc = Qry.get().FieldAsString(col_rfisc);
                if(not Qry.get().FieldIsNULL(col_rate))
                    row.rate = Qry.get().FieldAsFloat(col_rate);
                row.rate_cur = Qry.get().FieldAsString(col_rate_cur);
                RemStat.insert(row);
                params.overflow.check(RemStat.size());
            }
        }
}

template <class T>
void RunRemStat(
        const TStatParams &params,
        T &RemStat,
        TPrintAirline &prn_airline
        )
{
        QParams QryParams;
        QryParams
            << QParam("FirstDate", otDate, params.FirstDate)
            << QParam("LastDate", otDate, params.LastDate);
        string SQLText =
            "select "
            "   points.point_id, "
            "   cs.ticket_no, "
            "   points.scd_out, "
            "   points.flt_no, "
            "   points.suffix, "
            "   points.airp, "
            "   cs.airp_last, "
            "   points.craft, "
            "   cs.travel_time, "
            "   cs.rem_code, "
            "   points.airline, "
            "   users2.descr, "
            "   cs.desk, "
            "   cs.rfisc, "
            "   cs.rate, "
            "   cs.rate_cur "
            "from ";
        SQLText +=
            "   stat_rem cs, "
            "   points, ";
        SQLText +=
            "   users2 "
            "where "
            "   cs.point_id = points.point_id and ";
        params.AccessClause(SQLText);
        if(params.flt_no != NoExists) {
            SQLText += " points.flt_no = :flt_no and ";
            QryParams << QParam("flt_no", otInteger, params.flt_no);
        }
        SQLText +=
            "    points.scd_out >= :FirstDate AND points.scd_out < :LastDate and ";
        SQLText +=
            "    cs.user_id = users2.user_id ";
        TCachedQuery Qry(SQLText, QryParams);
        Qry.get().Execute();
        if(not Qry.get().Eof) {
            int col_point_id = Qry.get().GetFieldIndex("point_id");
            int col_ticket_no = Qry.get().GetFieldIndex("ticket_no");
            int col_scd_out = Qry.get().GetFieldIndex("scd_out");
            int col_flt_no = Qry.get().GetFieldIndex("flt_no");
            int col_suffix = Qry.get().GetFieldIndex("suffix");
            int col_airp = Qry.get().GetFieldIndex("airp");
            int col_airp_last = Qry.get().GetFieldIndex("airp_last");
            int col_craft = Qry.get().GetFieldIndex("craft");
            int col_travel_time = Qry.get().GetFieldIndex("travel_time");
            int col_rem_code = Qry.get().GetFieldIndex("rem_code");
            int col_airline = Qry.get().GetFieldIndex("airline");
            int col_descr = Qry.get().GetFieldIndex("descr");
            int col_desk = Qry.get().GetFieldIndex("desk");
            int col_rfisc = Qry.get().GetFieldIndex("rfisc");
            int col_rate = Qry.get().GetFieldIndex("rate");
            int col_rate_cur = Qry.get().GetFieldIndex("rate_cur");
            for(; not Qry.get().Eof; Qry.get().Next()) {
                prn_airline.check(Qry.get().FieldAsString(col_airline));
                TRemStatRow row;
                row.point_id = Qry.get().FieldAsInteger(col_point_id);
                row.ticket_no = Qry.get().FieldAsString(col_ticket_no);
                row.scd_out = Qry.get().FieldAsDateTime(col_scd_out);
                row.flt_no = Qry.get().FieldAsInteger(col_flt_no);
                row.suffix = Qry.get().FieldAsString(col_suffix);
                row.airp = Qry.get().FieldAsString(col_airp);
                row.airp_last = Qry.get().FieldAsString(col_airp_last);
                row.craft = Qry.get().FieldAsString(col_craft);
                if(not Qry.get().FieldIsNULL(col_travel_time))
                    row.travel_time = Qry.get().FieldAsDateTime(col_travel_time);
                row.rem_code = Qry.get().FieldAsString(col_rem_code);
                row.user = Qry.get().FieldAsString(col_descr);
                row.desk = Qry.get().FieldAsString(col_desk);
                row.rfisc = Qry.get().FieldAsString(col_rfisc);
                if(not Qry.get().FieldIsNULL(col_rate))
                    row.rate = Qry.get().FieldAsFloat(col_rate);
                row.rate_cur = Qry.get().FieldAsString(col_rate_cur);
                RemStat.insert(row);
                params.overflow.check(RemStat.size());
            }
        }
        ArxRemStat(params, RemStat, prn_airline);
}

void createXMLRemStat(const TStatParams &params, const TRemStat &RemStat, const TPrintAirline &prn_airline, xmlNodePtr resNode)
{
    if(RemStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    NewTextChild(resNode, "airline", prn_airline.get(), "");
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");

    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("�� ॣ."));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�⮩��"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�����"));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�����"));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("���"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("����"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��� ��"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�६� � ���"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��� ��㣨"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("RFISC"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("����"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortFloat);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�����"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    for(TRemStat::iterator i = RemStat.begin(); i != RemStat.end(); i++) {
        rowNode = NewTextChild(rowsNode, "row");
        // �� ॣ
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp));
        // �⮩��
        NewTextChild(rowNode, "col", i->desk);
        // �����
        NewTextChild(rowNode, "col", i->user);
        // �����
        NewTextChild(rowNode, "col", i->ticket_no);
        // ���
        NewTextChild(rowNode, "col", DateTimeToStr(i->scd_out, "dd.mm.yyyy"));
        // ����
        ostringstream buf;
        buf << setw(3) << setfill('0') << i->flt_no << ElemIdToCodeNative(etSuffix, i->suffix);
        NewTextChild(rowNode, "col", buf.str());
        // ��
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp));
        // ��
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp_last));
        // ��� ��
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etCraft, i->craft));
        // �६� � ���
        if(i->travel_time == NoExists)
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col", DateTimeToStr(i->travel_time, "hh:nn"));
        // ��� ��㣨
        NewTextChild(rowNode, "col", i->rem_code);
        // RFISC
        NewTextChild(rowNode, "col", i->rfisc);
        // ����
        NewTextChild(rowNode, "col", i->rate_str());
        // �����
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etCurrency, i->rate_cur));
    }

    STAT::set_variables(resNode);
}

namespace RemStat {
    struct TFltInfo {
        struct THelperFltInfo {
            string airp_last;
            TDateTime travel_time;
            THelperFltInfo(): travel_time(NoExists) {}
        };
        typedef map<int, THelperFltInfo> TItems;
        TItems items;
        TItems::iterator get(const int &point_id, const string &craft, const string &airp)
        {
            TItems::iterator result = items.find(point_id);
            if(result == items.end()) {
                THelperFltInfo hfi;
                TTripRoute route;
                route.GetRouteAfter(NoExists, point_id, trtNotCurrent, trtNotCancelled);
                hfi.airp_last = route.back().airp;

                hfi.travel_time = getTimeTravel(craft, airp, hfi.airp_last);
                pair<TItems::iterator, bool> res = items.insert(make_pair(point_id, hfi));
                result = res.first;
            }
            return result;
        }
    };
}

struct TRemInfo {
    string rfisc;
    double rate;
    string rate_cur;
    TRemInfo(const string &rem_code, const string &rem) {
        parse(rem_code, rem);
    }
    void parse(const string &rem_code, const string &rem);
};

void TRemInfo::parse(const string &rem_code, const string &rem)
{
    rfisc.clear();
    rate_cur.clear();
    rate = NoExists;

    if(rem_code != "PRSA") return;

    vector<string> tokens;
    boost::split(tokens, rem, boost::is_any_of("/"));
    if(tokens.size() == 2) { // PRSA/0B7
        rfisc = tokens[1];
    } else if(tokens.size() == 4) { // PRSA/0B7/1500/RUB
        rfisc = tokens[1];
        if ( StrToFloat( tokens[2].c_str(), rate ) == EOF )
            rate = NoExists;
        TElemFmt fmt;
        rate_cur = ElemToElemId(etCurrency, tokens[3], fmt);
    }
}

void get_rem_stat(int point_id)
{
    TCachedQuery delQry("delete from stat_rem where point_id = :point_id", QParams() << QParam("point_id", otInteger, point_id));
    delQry.get().Execute();
    TCachedQuery Qry(
            "select "
            "    pax.pax_id, "
            "    points.craft, "
            "    points.airp, "
            "    pro.rem_code, "
            "    pro.rem, "
            "    pro.user_id, "
            "    pro.desk "
            "from "
            "    points, "
            "    pax_grp, "
            "    pax, "
            "    pax_rem_origin pro "
            "where "
            "    points.point_id = :point_id and "
            "    pax_grp.point_dep = points.point_id and "
            "    pax_grp.grp_id = pax.grp_id and"
            "    pax.pax_id = pro.pax_id ",
            QParams() << QParam("point_id", otInteger, point_id)
            );
    TCachedQuery insQry(
            "insert into stat_rem( "
            "   point_id, "
            "   travel_time, "
            "   rem_code, "
            "   ticket_no, "
            "   airp_last, "
            "   user_id, "
            "   desk, "
            "   rfisc, "
            "   rate, "
            "   rate_cur "
            ") values ( "
            "   :point_id, "
            "   :travel_time, "
            "   :rem_code, "
            "   :ticket_no, "
            "   :airp_last, "
            "   :user_id, "
            "   :desk, "
            "   :rfisc, "
            "   :rate, "
            "   :rate_cur "
            ") ",
            QParams()
            << QParam("point_id", otInteger, point_id)
            << QParam("travel_time", otDate)
            << QParam("rem_code", otString)
            << QParam("ticket_no", otString)
            << QParam("airp_last", otString)
            << QParam("user_id", otInteger)
            << QParam("desk", otString)
            << QParam("rfisc", otString)
            << QParam("rate", otFloat)
            << QParam("rate_cur", otString)
            );

    Qry.get().Execute();


    if(not Qry.get().Eof) {
        int col_pax_id = Qry.get().FieldIndex("pax_id");
        int col_craft = Qry.get().FieldIndex("craft");
        int col_airp = Qry.get().FieldIndex("airp");
        int col_rem_code = Qry.get().FieldIndex("rem_code");
        int col_rem = Qry.get().FieldIndex("rem");
        int col_user_id = Qry.get().FieldIndex("user_id");
        int col_desk = Qry.get().FieldIndex("desk");
        RemStat::TFltInfo flt_info;
        for(; not Qry.get().Eof; Qry.get().Next()) {
            int pax_id = Qry.get().FieldAsInteger(col_pax_id);
            string craft = Qry.get().FieldAsString(col_craft);
            string airp = Qry.get().FieldAsString(col_airp);

            RemStat::TFltInfo::TItems::iterator flt_info_idx = flt_info.get(point_id, craft, airp);

            CheckIn::TPaxTknItem tkn;
            LoadPaxTkn(pax_id, tkn);
            ostringstream ticket_no;
            if(not tkn.empty()) {
                ticket_no << tkn.no;
                if (tkn.coupon!=ASTRA::NoExists)
                    ticket_no << "/" << tkn.coupon;
            }

            if(flt_info_idx->second.travel_time == NoExists)
                insQry.get().SetVariable("travel_time", FNull);
            else
                insQry.get().SetVariable("travel_time", flt_info_idx->second.travel_time);
            insQry.get().SetVariable("rem_code", Qry.get().FieldAsString(col_rem_code));
            insQry.get().SetVariable("ticket_no", ticket_no.str());
            insQry.get().SetVariable("airp_last", flt_info_idx->second.airp_last);
            insQry.get().SetVariable("user_id", Qry.get().FieldAsInteger(col_user_id));
            insQry.get().SetVariable("desk", Qry.get().FieldAsString(col_desk));

            TRemInfo remInfo(
                    Qry.get().FieldAsString(col_rem_code),
                    Qry.get().FieldAsString(col_rem));

            insQry.get().SetVariable("rfisc", remInfo.rfisc);
            if(remInfo.rate == NoExists)
                insQry.get().SetVariable("rate", FNull);
            else
                insQry.get().SetVariable("rate", remInfo.rate);
            insQry.get().SetVariable("rate_cur", remInfo.rate_cur);

            insQry.get().Execute();
        }
    }
}

