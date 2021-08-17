#include "stat_pfs.h"
#include "qrys.h"
#include "astra_misc.h"
#include "passenger.h"
#include "report_common.h"
#include "stat_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace BASIC::date_time;
using namespace std;
using namespace ASTRA;
using namespace AstraLocale;

bool TPFSStatRow::operator < (const TPFSStatRow &val) const
{
    if(point_id == val.point_id)
        return pax_id < val.pax_id;
    return point_id < val.point_id;
}

void TPFSShortStat::dump()
{
    for(TPFSScdOutMap::iterator scd_out = begin();
            scd_out != end(); scd_out++) {
        for(TPFSFltMap::iterator flt = scd_out->second.begin();
                flt != scd_out->second.end(); flt++) {
            for(TPFSRouteMap::iterator route = flt->second.begin();
                    route != flt->second.end(); route++) {
                for(TPFSStatusMap::iterator status = route->second.begin();
                        status != route->second.end(); status++) {
                    LogTrace(TRACE5)
                        << "stat[" << DateTimeToStr(scd_out->first, "dd.mm.yyyy") << "]"
                        << "[" << flt->first << "]"
                        << "[" << route->first << "]"
                        << "[" << route->first << "]"
                        << "[" << status->first << "]"
                        << " = " << status->second;
                }
            }
        }
    }
}

void TPFSShortStat::add(const TPFSStatRow &row)
{
    TPFSStatusMap &status = (*this)[row.scd_out][row.flt][row.route];
    if(status.empty()) FRowCount++;
    status[row.status]++;
}

void ArxRunPFSStat(
        const TStatParams &params,
        TPFSAbstractStat &PFSStat,
        TPrintAirline &prn_airline
        )
{
    tst();
    TDateTime first_date = params.FirstDate;
    TDateTime last_date = params.LastDate;
    if(params.LT) {
        first_date -= 1;
        last_date += 1;
    };
    for(int pass = 1; pass <= 2; pass++) {
        QParams QryParams;
        QryParams
            << QParam("FirstDate", otDate, first_date)
            << QParam("LastDate", otDate, last_date);
        QryParams << QParam("arx_trip_date_range", otDate, last_date + ARX_TRIP_DATE_RANGE());
        string SQLText =
            "select  "
            "   arx_points.part_key, "
            "   arx_points.point_id, "
            "   arx_points.scd_out, "
            "   arx_points.airline, "
            "   arx_points.flt_no, "
            "   arx_points.suffix, "
            "   arx_points.airp, "
            "   arx_pfs_stat.pax_id, "
            "   arx_pfs_stat.status, "
            "   arx_pfs_stat.airp_arv, "
            "   arx_pfs_stat.seats, "
            "   arx_pfs_stat.subcls, "
            "   arx_pfs_stat.pnr, "
            "   arx_pfs_stat.surname, "
            "   arx_pfs_stat.name, "
            "   arx_pfs_stat.gender, "
            "   arx_pfs_stat.birth_date "
            "from "
                "   arx_pfs_stat , "
                "   arx_points  ";
            if(pass == 2) {
                SQLText += getMoveArxQuery();
            }
        SQLText +=
            "where "
            "   arx_pfs_stat.point_id = arx_points.point_id and "
            "   arx_points.pr_del >= 0 and ";
        params.AccessClause(SQLText, "arx_points");
        if(params.flt_no != NoExists) {
            SQLText += " arx_points.flt_no = :flt_no and ";
            QryParams << QParam("flt_no", otInteger, params.flt_no);
        }
        SQLText += " arx_points.part_key = arx_pfs_stat.part_key and ";
        if(pass == 1)
            SQLText += " arx_points.part_key >= :FirstDate AND arx_points.part_key < :arx_trip_date_range AND \n";
        if(pass == 2)
            SQLText += " arx_points.part_key=arx_ext.part_key AND arx_points.move_id=arx_ext.move_id AND \n";
        SQLText += " arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate ";
        DB::TCachedQuery Qry(PgOra::getROSession("ARX_POINTS"), SQLText, QryParams, STDLOG);
        Qry.get().Execute();
        if(not Qry.get().Eof) {
            int col_part_key = Qry.get().FieldIndex("part_key");
            int col_point_id = Qry.get().FieldIndex("point_id");
            int col_scd_out = Qry.get().FieldIndex("scd_out");
            int col_airline = Qry.get().FieldIndex("airline");
            int col_flt_no = Qry.get().FieldIndex("flt_no");
            int col_suffix = Qry.get().FieldIndex("suffix");
            int col_airp = Qry.get().FieldIndex("airp");
            int col_pax_id = Qry.get().FieldIndex("pax_id");
            int col_status = Qry.get().FieldIndex("status");
            // int col_airp_arv = Qry.get().FieldIndex("airp_arv");
            int col_seats = Qry.get().FieldIndex("seats");
            int col_subcls = Qry.get().FieldIndex("subcls");
            int col_pnr = Qry.get().FieldIndex("pnr");
            int col_surname = Qry.get().FieldIndex("surname");
            int col_name = Qry.get().FieldIndex("name");
            int col_gender = Qry.get().FieldIndex("gender");
            int col_birth_date = Qry.get().FieldIndex("birth_date");
            for(; not Qry.get().Eof; Qry.get().Next()) {
                TDateTime local_scd_out = Qry.get().FieldAsDateTime(col_scd_out);
                if(params.LT) {
                    local_scd_out = UTCToLocal(local_scd_out,
                            AirpTZRegion(Qry.get().FieldAsString(col_airp)));
                    if(not(local_scd_out >= params.FirstDate and local_scd_out < params.LastDate))
                        continue;
                }
                prn_airline.check(Qry.get().FieldAsString(col_airline));
                TPFSStatRow row;
                row.point_id = Qry.get().FieldAsInteger(col_point_id);
                row.pax_id = Qry.get().FieldAsInteger(col_pax_id);
                row.status = Qry.get().FieldAsString(col_status);
                row.scd_out = local_scd_out;
                ostringstream buf;
                buf
                    << ElemIdToCodeNative(etAirline, Qry.get().FieldAsString(col_airline))
                    << setw(3) << setfill('0') << Qry.get().FieldAsInteger(col_flt_no)
                    << ElemIdToCodeNative(etSuffix, Qry.get().FieldAsString(col_suffix));
                row.flt = buf.str();
                /*
                   buf.str("");
                   buf
                   << ElemIdToCodeNative(etAirp, Qry.get().FieldAsString(col_airp)) << "-"
                   << ElemIdToCodeNative(etAirp, Qry.get().FieldAsString(col_airp_arv));
                   row.route = buf.str();
                   */


                /*
                   route.set(GetRouteAfterStr( col_part_key>=0?Qry.FieldAsDateTime(col_part_key):NoExists,
                   Qry.FieldAsInteger(col_point_id),
                   trtNotCurrent,
                   trtNotCancelled),
                   */

                TDateTime part_key = Qry.get().FieldIsNULL(col_part_key) ? NoExists : Qry.get().FieldAsDateTime(col_part_key);
                row.route = GetRouteAfterStr(part_key, row.point_id, trtWithCurrent, trtNotCancelled);


                row.seats = Qry.get().FieldAsInteger(col_seats);
                row.subcls = ElemIdToCodeNative(etSubcls, Qry.get().FieldAsString(col_subcls));
                row.pnr = Qry.get().FieldAsString(col_pnr);
                row.surname = transliter(Qry.get().FieldAsString(col_surname), TranslitFormat::V1, TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU);
                row.name = transliter(Qry.get().FieldAsString(col_name), TranslitFormat::V1, TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU);

                row.gender = Qry.get().FieldAsString(col_gender);
                switch(CheckIn::is_female(row.gender, row.name)) {
                    case NoExists:
                        row.gender.clear();
                        break;
                    case 0:
                        row.gender = getLocaleText("CAP.DOC.MALE");
                        break;
                    case 1:
                        row.gender = getLocaleText("CAP.DOC.FEMALE");
                        break;
                }

                if(Qry.get().FieldIsNULL(col_birth_date))
                    row.birth_date = NoExists;
                else
                    row.birth_date = Qry.get().FieldAsDateTime(col_birth_date);
                PFSStat.add(row);

                params.overflow.check(PFSStat.RowCount());
            }
        }
    }
}

void RunPFSStat(
        const TStatParams &params,
        TPFSAbstractStat &PFSStat,
        TPrintAirline &prn_airline
        )
{
    TDateTime first_date = params.FirstDate;
    TDateTime last_date = params.LastDate;
    if(params.LT) {
        first_date -= 1;
        last_date += 1;
    };

    QParams QryParams;
    QryParams
        << QParam("FirstDate", otDate, first_date)
        << QParam("LastDate", otDate, last_date);
    string SQLText =
        "SELECT "
        " NULL part_key, "
        " points.point_id, "
        " points.scd_out, "
        " points.airline, "
        " points.flt_no, "
        " points.suffix, "
        " points.airp, "
        " pfs_stat.pax_id, "
        " pfs_stat.status, "
        " pfs_stat.airp_arv, "
        " pfs_stat.seats, "
        " pfs_stat.subcls, "
        " pfs_stat.pnr, "
        " pfs_stat.surname, "
        " pfs_stat.name, "
        " pfs_stat.gender, "
        " pfs_stat.birth_date "
        "FROM "
        "   pfs_stat, "
        "   points "
        "WHERE "
        "   pfs_stat.point_id = points.point_id AND "
        "   points.pr_del >= 0 AND ";
    params.AccessClause(SQLText);
    if(params.flt_no != NoExists) {
        SQLText += " points.flt_no = :flt_no AND ";
        QryParams << QParam("flt_no", otInteger, params.flt_no);
    }
    SQLText += " points.scd_out >= :FirstDate AND points.scd_out < :LastDate ";
    DB::TCachedQuery Qry(PgOra::getROSession({"PFS_STAT","POINTS"}), SQLText, QryParams, STDLOG);
    Qry.get().Execute();
    if(not Qry.get().Eof) {
        int col_part_key = Qry.get().FieldIndex("part_key");
        int col_point_id = Qry.get().FieldIndex("point_id");
        int col_scd_out = Qry.get().FieldIndex("scd_out");
        int col_airline = Qry.get().FieldIndex("airline");
        int col_flt_no = Qry.get().FieldIndex("flt_no");
        int col_suffix = Qry.get().FieldIndex("suffix");
        int col_airp = Qry.get().FieldIndex("airp");
        int col_pax_id = Qry.get().FieldIndex("pax_id");
        int col_status = Qry.get().FieldIndex("status");
        // int col_airp_arv = Qry.get().FieldIndex("airp_arv");
        int col_seats = Qry.get().FieldIndex("seats");
        int col_subcls = Qry.get().FieldIndex("subcls");
        int col_pnr = Qry.get().FieldIndex("pnr");
        int col_surname = Qry.get().FieldIndex("surname");
        int col_name = Qry.get().FieldIndex("name");
        int col_gender = Qry.get().FieldIndex("gender");
        int col_birth_date = Qry.get().FieldIndex("birth_date");
        for(; not Qry.get().Eof; Qry.get().Next()) {
            TDateTime local_scd_out = Qry.get().FieldAsDateTime(col_scd_out);
            if(params.LT) {
                local_scd_out = UTCToLocal(local_scd_out,
                        AirpTZRegion(Qry.get().FieldAsString(col_airp)));
                if(not(local_scd_out >= params.FirstDate and local_scd_out < params.LastDate))
                    continue;
            }
            prn_airline.check(Qry.get().FieldAsString(col_airline));
            TPFSStatRow row;
            row.point_id = Qry.get().FieldAsInteger(col_point_id);
            row.pax_id = Qry.get().FieldAsInteger(col_pax_id);
            row.status = Qry.get().FieldAsString(col_status);
            row.scd_out = local_scd_out;
            ostringstream buf;
            buf
                << ElemIdToCodeNative(etAirline, Qry.get().FieldAsString(col_airline))
                << setw(3) << setfill('0') << Qry.get().FieldAsInteger(col_flt_no)
                << ElemIdToCodeNative(etSuffix, Qry.get().FieldAsString(col_suffix));
            row.flt = buf.str();
            /*
               buf.str("");
               buf
               << ElemIdToCodeNative(etAirp, Qry.get().FieldAsString(col_airp)) << "-"
               << ElemIdToCodeNative(etAirp, Qry.get().FieldAsString(col_airp_arv));
               row.route = buf.str();
               */


            /*
               route.set(GetRouteAfterStr( col_part_key>=0?Qry.FieldAsDateTime(col_part_key):NoExists,
               Qry.FieldAsInteger(col_point_id),
               trtNotCurrent,
               trtNotCancelled),
               */

            TDateTime part_key = Qry.get().FieldIsNULL(col_part_key) ? NoExists : Qry.get().FieldAsDateTime(col_part_key);
            row.route = GetRouteAfterStr(part_key, row.point_id, trtWithCurrent, trtNotCancelled);


            row.seats = Qry.get().FieldAsInteger(col_seats);
            row.subcls = ElemIdToCodeNative(etSubcls, Qry.get().FieldAsString(col_subcls));
            row.pnr = Qry.get().FieldAsString(col_pnr);
            row.surname = transliter(Qry.get().FieldAsString(col_surname), TranslitFormat::V1, TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU);
            row.name = transliter(Qry.get().FieldAsString(col_name), TranslitFormat::V1, TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU);

            row.gender = Qry.get().FieldAsString(col_gender);
            switch(CheckIn::is_female(row.gender, row.name)) {
                case NoExists:
                    row.gender.clear();
                    break;
                case 0:
                    row.gender = getLocaleText("CAP.DOC.MALE");
                    break;
                case 1:
                    row.gender = getLocaleText("CAP.DOC.FEMALE");
                    break;
            }

            if(Qry.get().FieldIsNULL(col_birth_date))
                row.birth_date = NoExists;
            else
                row.birth_date = Qry.get().FieldAsDateTime(col_birth_date);
            PFSStat.add(row);

            params.overflow.check(PFSStat.RowCount());
        }
    }

    ArxRunPFSStat(params, PFSStat, prn_airline);
}

void createXMLPFSStat(
        const TStatParams &params,
        const TPFSStat &PFSStat,
        const TPrintAirline &prn_airline,
        xmlNodePtr resNode)
{
    if(PFSStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    NewTextChild(resNode, "airline", prn_airline.get(), "");
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");

    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("���"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("����"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�������"));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�����"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("����"));
    SetProp(colNode, "width", 65);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("RBD"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("PNR"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�������"));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("���"));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("���"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��������"));
    SetProp(colNode, "width", 85);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    for(TPFSStat::iterator i = PFSStat.begin(); i != PFSStat.end(); i++) {
        rowNode = NewTextChild(rowsNode, "row");
        // ���
        NewTextChild(rowNode, "col", DateTimeToStr(i->scd_out, "dd.mm.yyyy"));
        // ����
        NewTextChild(rowNode, "col", i->flt);
        // �������
        NewTextChild(rowNode, "col", i->route);
        // �����
        NewTextChild(rowNode, "col", i->status);
        // ���-�� ����
        NewTextChild(rowNode, "col", i->seats);
        // RBD
        NewTextChild(rowNode, "col", i->subcls);
        // PNR
        NewTextChild(rowNode, "col", i->pnr);
        // �������
        NewTextChild(rowNode, "col", i->surname);
        // ���
        NewTextChild(rowNode, "col", i->name);
        // ���
        NewTextChild(rowNode, "col", i->gender);
        // ��� ஦�����
        if(i->birth_date == NoExists)
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col", DateTimeToStr(i->birth_date, "dd.mm.yyyy"));
    }

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", "PFS");
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("���஡���"));
}

void createXMLPFSShortStat(
        const TStatParams &params,
        TPFSShortStat &PFSShortStat,
        const TPrintAirline &prn_airline,
        xmlNodePtr resNode)
{
    if(PFSShortStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    NewTextChild(resNode, "airline", prn_airline.get(), "");
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");

    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("���"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("����"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�������"));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("NOREC"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("NOSHO"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("GOSHO"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("OFFLK"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    for(TPFSShortStat::iterator
            stat = PFSShortStat.begin();
            stat != PFSShortStat.end(); stat++)
        for(TPFSFltMap::iterator
                flt = stat->second.begin();
                flt != stat->second.end(); flt++)
            for(TPFSRouteMap::iterator
                    route = flt->second.begin();
                    route != flt->second.end(); route++)
            {
                rowNode = NewTextChild(rowsNode, "row");
                // ���
                NewTextChild(rowNode, "col", DateTimeToStr(stat->first, "dd.mm.yyyy"));
                // ����
                NewTextChild(rowNode, "col", flt->first);
                // �������
                NewTextChild(rowNode, "col", route->first);
                // NOREC
                NewTextChild(rowNode, "col", route->second["NOREC"]);
                // NOSHO
                NewTextChild(rowNode, "col", route->second["NOSHO"]);
                // GOSHO
                NewTextChild(rowNode, "col", route->second["GOSHO"]);
                // OFFLK
                NewTextChild(rowNode, "col", route->second["OFFLK"]);
            }


    /*
    for(TPFSShortStat::iterator i = PFSShortStat.begin(); i != PFSShortStat.end(); i++) {
        rowNode = NewTextChild(rowsNode, "row");
        // ���
        NewTextChild(rowNode, "col", DateTimeToStr(i->scd_out, "dd.mm.yyyy"));
        // ����
        NewTextChild(rowNode, "col", i->flt);
        // �������
        NewTextChild(rowNode, "col", i->route);
        // �����
        NewTextChild(rowNode, "col", i->status);
        // ���-�� ����
        NewTextChild(rowNode, "col", i->seats);
        // RBD
        NewTextChild(rowNode, "col", i->subcls);
        // PNR
        NewTextChild(rowNode, "col", i->pnr);
        // �������
        NewTextChild(rowNode, "col", i->surname);
        // ���
        NewTextChild(rowNode, "col", i->name);
        // ���
        NewTextChild(rowNode, "col", i->gender);
        // ��� ஦�����
        if(i->birth_date == NoExists)
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col", DateTimeToStr(i->birth_date, "dd.mm.yyyy"));
    }
    */

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", "PFS");
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("����"));
}

struct TPFSShortStatCombo : public TOrderStatItem
{
    TDateTime scd_out;
    string flt;
    string route;
    int norec;
    int nosho;
    int gosho;
    int offlk;

    TPFSShortStatCombo(
            TDateTime _scd_out,
            const string &_flt,
            const string &_route,
            int _norec,
            int _nosho,
            int _gosho,
            int _offlk):
        scd_out(_scd_out),
        flt(_flt),
        route(_route),
        norec(_norec),
        nosho(_nosho),
        gosho(_gosho),
        offlk(_offlk)
    {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

struct TPFSFullStatCombo : public TOrderStatItem
{
    const TPFSStatRow &row;
    TPFSFullStatCombo(const TPFSStatRow &_row): row(_row) {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TPFSShortStatCombo::add_data(ostringstream &buf) const
{
    buf << DateTimeToStr(scd_out, "dd.mm.yyyy") << delim;
    buf << flt << delim;
    buf << route << delim;
    buf << norec << delim;
    buf << nosho << delim;
    buf << gosho << delim;
    buf << offlk << endl;
}

void TPFSShortStatCombo::add_header(ostringstream &buf) const
{
    buf
        << getLocaleText("���") << delim
        << getLocaleText("����") << delim
        << getLocaleText("�������") << delim
        << "NOREC" << delim
        << "NOSHO" << delim
        << "GOSHO" << delim
        << "OFFLK" << endl;
}

void TPFSFullStatCombo::add_data(ostringstream &buf) const
{
        // ���
        buf << DateTimeToStr(row.scd_out, "dd.mm.yyyy") << delim;
        // ����
        buf << row.flt << delim;
        // �������
        buf << row.route << delim;
        // �����
        buf << row.status << delim;
        // ���-�� ����
        buf << row.seats << delim;
        // RBD
        buf << row.subcls << delim;
        // PNR
        buf << row.pnr << delim;
        // �������
        buf << row.surname << delim;
        // ���
        buf << row.name << delim;
        // ���
        buf << row.gender << delim;
        // ��� ஦�����
        buf << (row.birth_date == NoExists ? "" : DateTimeToStr(row.birth_date, "dd.mm.yyyy"));
        buf << endl;
}

void TPFSFullStatCombo::add_header(ostringstream &buf) const
{
    buf
        << getLocaleText("���") << delim
        << getLocaleText("����") << delim
        << getLocaleText("�������") << delim
        << getLocaleText("�����") << delim
        << getLocaleText("����") << delim
        << "RBD" << delim
        << "PNR" << delim
        << getLocaleText("�������") << delim
        << getLocaleText("���") << delim
        << getLocaleText("���") << delim
        << getLocaleText("��������")
        << endl;
}

void RunPFSShortFile(const TStatParams &params, TOrderStatWriter &writer, TPrintAirline &prn_airline)
{
    TPFSShortStat PFSShortStat;
    RunPFSStat(params, PFSShortStat, prn_airline);
    for(TPFSShortStat::iterator
            stat = PFSShortStat.begin();
            stat != PFSShortStat.end(); stat++)
        for(TPFSFltMap::iterator
                flt = stat->second.begin();
                flt != stat->second.end(); flt++)
            for(TPFSRouteMap::iterator
                    route = flt->second.begin();
                    route != flt->second.end(); route++)
            {
                writer.insert(TPFSShortStatCombo(
                            stat->first,
                            flt->first,
                            route->first,
                            route->second["NOREC"],
                            route->second["NOSHO"],
                            route->second["GOSHO"],
                            route->second["OFFLK"]
                            ));
            }
}

void RunPFSFullFile(const TStatParams &params, TOrderStatWriter &writer, TPrintAirline &prn_airline)
{
    TPFSStat PFSStat;
    RunPFSStat(params, PFSStat, prn_airline);
    for(TPFSStat::iterator i = PFSStat.begin(); i != PFSStat.end(); i++)
        writer.insert(TPFSFullStatCombo(*i));
}

