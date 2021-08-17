#include "stat_self_ckin.h"
#include "astra_misc.h"
#include "astra_date_time.h"
#include "report_common.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;
using namespace ASTRA::date_time;
using namespace BASIC::date_time;
using namespace AstraLocale;
using namespace EXCEPTIONS;

bool TSelfCkinStatRow::operator == (const TSelfCkinStatRow &item) const
{
    return pax_amount == item.pax_amount &&
        term_bp == item.term_bp &&
        term_bag == item.term_bag &&
        term_ckin_service == item.term_ckin_service &&
        adult == item.adult &&
        child == item.child &&
        baby == item.baby &&
        tckin == item.tckin &&
        flts.size() == item.flts.size();
};

void TSelfCkinStatRow::operator += (const TSelfCkinStatRow &item)
{
    pax_amount += item.pax_amount;
    term_bp += item.term_bp;
    term_bag += item.term_bag;
    term_ckin_service += item.term_ckin_service;
    adult += item.adult;
    child += item.child;
    baby += item.baby;
    tckin += item.tckin;
    flts.insert(item.flts.begin(),item.flts.end());
};

bool TKioskCmp::operator() (const TSelfCkinStatKey &lr, const TSelfCkinStatKey &rr) const
{
    if(lr.client_type == rr.client_type)
        if(lr.desk == rr.desk)
            if(lr.desk_airp == rr.desk_airp)
                if(lr.ak == rr.ak)
                    if(lr.ap == rr.ap)
                        if(lr.flt_no == rr.flt_no)
                            if(lr.scd_out == rr.scd_out)
                                if(lr.point_id == rr.point_id)
                                    return lr.places.get() < rr.places.get();
                                else
                                    return lr.point_id < rr.point_id;
                            else
                                return lr.scd_out < rr.scd_out;
                        else
                            return lr.flt_no < rr.flt_no;
                    else
                        return lr.ap < rr.ap;
                else
                    return lr.ak < rr.ak;
            else
                return lr.desk_airp < rr.desk_airp;
        else
            return lr.desk < rr.desk;
    else
        return lr.client_type < rr.client_type;
}

void ArxRunSelfCkinStat(const TStatParams &params,
                  TSelfCkinStat &SelfCkinStat, TSelfCkinStatRow &SelfCkinStatTotal,
                  TPrintAirline &prn_airline)
{
    LogTrace5 << __func__;
    DB::TQuery Qry(PgOra::getROSession("ARX_SELF_CKIN_STAT"), STDLOG);
    for(int pass = 1; pass <= 2; pass++) {
        string SQLText =
            "select "
            "    arx_points.airline, "
            "    arx_points.airp, "
            "    arx_points.scd_out, "
            "    arx_points.flt_no, "
            "    arx_self_ckin_stat.point_id, "
            "    arx_self_ckin_stat.client_type, "
            "    desk, "
            "    desk_airp, "
            "    descr, "
            "    adult, "
            "    child, "
            "    baby, "
            "    term_bp, "
            "    term_bag, "
            "    term_ckin_service, "
            "    tckin "
            "from "
            " arx_points , "
            " arx_self_ckin_stat ";
            if(pass == 2)
                SQLText += getMoveArxQuery();

        SQLText += "where ";
        if (pass==1)
            SQLText += " arx_points.part_key >= :FirstDate AND arx_points.part_key < :arx_trip_date_range AND \n";
        if (pass==2)
            SQLText += " arx_points.part_key=arx_ext.part_key AND arx_points.move_id=arx_ext.move_id AND \n";
        SQLText += " arx_self_ckin_stat.part_key = arx_points.part_key AND \n";

        params.AccessClause(SQLText, "arx_points");

        SQLText +=
            "    arx_self_ckin_stat.point_id = arx_points.point_id and "
            "    arx_points.pr_del >= 0 and "
            "    arx_points.scd_out >= :FirstDate and "
            "    arx_points.scd_out < :LastDate ";

        if(not params.reg_type.empty()) {
            SQLText += " and arx_self_ckin_stat.client_type = :reg_type ";
            Qry.CreateVariable("reg_type", otString, params.reg_type);
        }
        if(params.flt_no != NoExists) {
            SQLText += " and arx_points.flt_no = :flt_no ";
            Qry.CreateVariable("flt_no", otInteger, params.flt_no);
        }
        if(!params.desk.empty()) {
            SQLText += "and arx_self_ckin_stat.desk = :desk ";
            Qry.CreateVariable("desk", otString, params.desk);
        }
        Qry.SQLText = SQLText;
        Qry.CreateVariable("FirstDate", otDate, params.FirstDate);
        Qry.CreateVariable("LastDate", otDate, params.LastDate);
        Qry.CreateVariable("arx_trip_date_range", otDate, params.LastDate+ARX_TRIP_DATE_RANGE());
        Qry.Execute();
        if(not Qry.Eof) {
            int col_airline = Qry.FieldIndex("airline");
            int col_airp = Qry.FieldIndex("airp");
            int col_scd_out = Qry.FieldIndex("scd_out");
            int col_flt_no = Qry.FieldIndex("flt_no");
            int col_point_id = Qry.FieldIndex("point_id");
            int col_client_type = Qry.FieldIndex("client_type");
            int col_desk = Qry.FieldIndex("desk");
            int col_desk_airp = Qry.FieldIndex("desk_airp");
            int col_descr = Qry.FieldIndex("descr");
            int col_adult = Qry.FieldIndex("adult");
            int col_child = Qry.FieldIndex("child");
            int col_term_bp = Qry.FieldIndex("term_bp");
            int col_term_bag = Qry.FieldIndex("term_bag");
            int col_term_ckin_service = Qry.FieldIndex("term_ckin_service");
            int col_baby = Qry.FieldIndex("baby");
            int col_tckin = Qry.FieldIndex("tckin");
            for(; not Qry.Eof; Qry.Next())
            {
              string airline = Qry.FieldAsString(col_airline);
              prn_airline.check(airline);

              TSelfCkinStatRow row;
              row.adult = Qry.FieldAsInteger(col_adult);
              row.child = Qry.FieldAsInteger(col_child);
              row.baby = Qry.FieldAsInteger(col_baby);
              row.tckin = Qry.FieldAsInteger(col_tckin);
              row.pax_amount = row.adult + row.child + row.baby;
              row.term_bp = Qry.FieldAsInteger(col_term_bp);
              row.term_bag = Qry.FieldAsInteger(col_term_bag);
              if (Qry.FieldIsNULL(col_term_ckin_service))
                row.term_ckin_service = row.pax_amount;
              else
                row.term_ckin_service = Qry.FieldAsInteger(col_term_ckin_service);
              int point_id=Qry.FieldAsInteger(col_point_id);
              row.flts.insert(point_id);
              if (!params.skip_rows)
              {
                string airp = Qry.FieldAsString(col_airp);
                TDateTime scd_out = Qry.FieldAsDateTime(col_scd_out);
                int flt_no = Qry.FieldAsInteger(col_flt_no);
                TSelfCkinStatKey key;
                key.client_type = Qry.FieldAsString(col_client_type);
                key.desk = Qry.FieldAsString(col_desk);
                key.desk_airp = ElemIdToCodeNative(etAirp, Qry.FieldAsString(col_desk_airp));
                key.descr = Qry.FieldAsString(col_descr);

                key.ak = ElemIdToCodeNative(etAirline, airline);
                if(
                        params.statType == statSelfCkinDetail or
                        params.statType == statSelfCkinFull
                  )
                    key.ap = airp;
                if(params.statType == statSelfCkinFull) {
                    key.flt_no = flt_no;
                    key.scd_out = scd_out;
                    key.point_id = point_id;
                    key.places.set(GetRouteAfterStr( NoExists, point_id, trtNotCurrent, trtNotCancelled), false);
                }

                AddStatRow(params.overflow, key, row, SelfCkinStat);
              }
              else
              {
                SelfCkinStatTotal+=row;
              };
            }
        }
    }
}


void RunSelfCkinStat(const TStatParams &params,
                  TSelfCkinStat &SelfCkinStat, TSelfCkinStatRow &SelfCkinStatTotal,
                  TPrintAirline &prn_airline)
{
    DB::TQuery Qry(PgOra::getROSession({"POINTS","SELF_CKIN_STAT"}), STDLOG);
    string SQLText =
        "SELECT "
        "    points.airline, "
        "    points.airp, "
        "    points.scd_out, "
        "    points.flt_no, "
        "    self_ckin_stat.point_id, "
        "    self_ckin_stat.client_type, "
        "    desk, "
        "    desk_airp, "
        "    descr, "
        "    adult, "
        "    child, "
        "    baby, "
        "    term_bp, "
        "    term_bag, "
        "    term_ckin_service, "
        "    tckin "
        "FROM "
        "   points, "
        "   self_ckin_stat "
        "WHERE ";

    params.AccessClause(SQLText);

    SQLText +=
        "    self_ckin_stat.point_id = points.point_id AND "
        "    points.pr_del >= 0 AND "
        "    points.scd_out >= :FirstDate AND "
        "    points.scd_out < :LastDate ";

    if(not params.reg_type.empty()) {
        SQLText += " AND self_ckin_stat.client_type = :reg_type ";
        Qry.CreateVariable("reg_type", otString, params.reg_type);
    }
    if(params.flt_no != NoExists) {
        SQLText += " AND points.flt_no = :flt_no ";
        Qry.CreateVariable("flt_no", otInteger, params.flt_no);
    }
    if(!params.desk.empty()) {
        SQLText += "AND self_ckin_stat.desk = :desk ";
        Qry.CreateVariable("desk", otString, params.desk);
    }
    Qry.SQLText = SQLText;
    Qry.CreateVariable("FirstDate", otDate, params.FirstDate);
    Qry.CreateVariable("LastDate", otDate, params.LastDate);
    Qry.Execute();
    if(not Qry.Eof) {
        int col_airline = Qry.FieldIndex("airline");
        int col_airp = Qry.FieldIndex("airp");
        int col_scd_out = Qry.FieldIndex("scd_out");
        int col_flt_no = Qry.FieldIndex("flt_no");
        int col_point_id = Qry.FieldIndex("point_id");
        int col_client_type = Qry.FieldIndex("client_type");
        int col_desk = Qry.FieldIndex("desk");
        int col_desk_airp = Qry.FieldIndex("desk_airp");
        int col_descr = Qry.FieldIndex("descr");
        int col_adult = Qry.FieldIndex("adult");
        int col_child = Qry.FieldIndex("child");
        int col_term_bp = Qry.FieldIndex("term_bp");
        int col_term_bag = Qry.FieldIndex("term_bag");
        int col_term_ckin_service = Qry.FieldIndex("term_ckin_service");
        int col_baby = Qry.FieldIndex("baby");
        int col_tckin = Qry.FieldIndex("tckin");
        for(; not Qry.Eof; Qry.Next())
        {
          string airline = Qry.FieldAsString(col_airline);
          prn_airline.check(airline);

          TSelfCkinStatRow row;
          row.adult = Qry.FieldAsInteger(col_adult);
          row.child = Qry.FieldAsInteger(col_child);
          row.baby = Qry.FieldAsInteger(col_baby);
          row.tckin = Qry.FieldAsInteger(col_tckin);
          row.pax_amount = row.adult + row.child + row.baby;
          row.term_bp = Qry.FieldAsInteger(col_term_bp);
          row.term_bag = Qry.FieldAsInteger(col_term_bag);
          if (Qry.FieldIsNULL(col_term_ckin_service))
            row.term_ckin_service = row.pax_amount;
          else
            row.term_ckin_service = Qry.FieldAsInteger(col_term_ckin_service);
          int point_id=Qry.FieldAsInteger(col_point_id);
          row.flts.insert(point_id);
          if (!params.skip_rows)
          {
            string airp = Qry.FieldAsString(col_airp);
            TDateTime scd_out = Qry.FieldAsDateTime(col_scd_out);
            int flt_no = Qry.FieldAsInteger(col_flt_no);
            TSelfCkinStatKey key;
            key.client_type = Qry.FieldAsString(col_client_type);
            key.desk = Qry.FieldAsString(col_desk);
            key.desk_airp = ElemIdToCodeNative(etAirp, Qry.FieldAsString(col_desk_airp));
            key.descr = Qry.FieldAsString(col_descr);

            key.ak = ElemIdToCodeNative(etAirline, airline);
            if(
                    params.statType == statSelfCkinDetail or
                    params.statType == statSelfCkinFull
              )
                key.ap = airp;
            if(params.statType == statSelfCkinFull) {
                key.flt_no = flt_no;
                key.scd_out = scd_out;
                key.point_id = point_id;
                key.places.set(GetRouteAfterStr( NoExists, point_id, trtNotCurrent, trtNotCancelled), false);
            }

            AddStatRow(params.overflow, key, row, SelfCkinStat);
          }
          else
          {
            SelfCkinStatTotal+=row;
          };
        }
    }

    ArxRunSelfCkinStat(params, SelfCkinStat, SelfCkinStatTotal, prn_airline);
}

void createXMLSelfCkinStat(const TStatParams &params,
                        const TSelfCkinStat &SelfCkinStat, const TSelfCkinStatRow &SelfCkinStatTotal,
                        const TPrintAirline &airline, xmlNodePtr resNode)
{
    if(SelfCkinStat.empty() && SelfCkinStatTotal==TSelfCkinStatRow())
        throw AstraLocale::UserException("MSG.NOT_DATA");

    NewTextChild(resNode, "airline", airline.get(), "");
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");
    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    TSelfCkinStatRow total;
    bool showTotal=true;
    int flts_total = 0;
    int rows = 0;
    for(TSelfCkinStat::const_iterator im = SelfCkinStat.begin(); im != SelfCkinStat.end(); ++im, rows++)
    {
        //region ��易⥫쭮 � ��砫� 横��, ���� �㤥� �ᯮ�祭 xml
        string region;
        if(params.statType == statSelfCkinFull)
        {
            try
            {
                region = AirpTZRegion(im->first.ap);
            }
            catch(AstraLocale::UserException &E)
            {
                AstraLocale::showErrorMessage("MSG.ERR_MSG.NOT_ALL_FLIGHTS_ARE_SHOWN", LParams() << LParam("msg", getLocaleText(E.getLexemaData())));
                if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //�� �㤥� �����뢠�� �⮣���� ��ப� ���� �� ����� � ����㦤����
                continue;
            };
        };

        rowNode = NewTextChild(rowsNode, "row");
        // ��� ॣ.
        NewTextChild(rowNode, "col", im->first.client_type);
        // ����
        NewTextChild(rowNode, "col", im->first.desk);
        // �/� ����
        NewTextChild(rowNode, "col", im->first.desk_airp);
        // �ਬ�砭��
        if(params.statType == statSelfCkinFull)
            NewTextChild(rowNode, "col", im->first.descr);
        // ��� �/�
        NewTextChild(rowNode, "col", im->first.ak);
        if(
                params.statType == statSelfCkinDetail or
                params.statType == statSelfCkinFull
          )
            // ��� �/�
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, im->first.ap));
        if(
                params.statType == statSelfCkinShort or
                params.statType == statSelfCkinDetail
          ) {
            // ���-�� ३ᮢ
            NewTextChild(rowNode, "col", (int)im->second.flts.size());
            flts_total += im->second.flts.size();
        }
        if(params.statType == statSelfCkinFull) {
            // ����� ३�
            ostringstream buf;
            buf << setw(3) << setfill('0') << im->first.flt_no;
            NewTextChild(rowNode, "col", buf.str().c_str());
            // ���
            NewTextChild(rowNode, "col", DateTimeToStr(
                        UTCToClient(im->first.scd_out, region), "dd.mm.yy")
                    );
            // ���ࠢ�����
            NewTextChild(rowNode, "col", im->first.places.get());
        }
        // ���-�� ����.
        NewTextChild(rowNode, "col", im->second.pax_amount);
        NewTextChild(rowNode, "col", im->second.term_bag);
        NewTextChild(rowNode, "col", im->second.term_bp);
        NewTextChild(rowNode, "col", im->second.pax_amount - im->second.term_ckin_service);

        if(params.statType == statSelfCkinFull) {
            // ��
            NewTextChild(rowNode, "col", im->second.adult);
            // ��
            NewTextChild(rowNode, "col", im->second.child);
            // ��
            NewTextChild(rowNode, "col", im->second.baby);
        }
        if(
                params.statType == statSelfCkinDetail or
                params.statType == statSelfCkinFull
          )
            // �����. ॣ.
            NewTextChild(rowNode, "col", im->second.tckin);
        if(
                params.statType == statSelfCkinShort or
                params.statType == statSelfCkinDetail
          )
            // �ਬ�砭��
            NewTextChild(rowNode, "col", im->first.descr);

        total += im->second;
    };

    rowNode = NewTextChild(rowsNode, "row");

    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("��� ॣ."));
    SetProp(colNode, "width", 75);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col", getLocaleText("�⮣�:"));
    colNode = NewTextChild(headerNode, "col", getLocaleText("����"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");
    colNode = NewTextChild(headerNode, "col", getLocaleText("�� ����"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");
    if(params.statType == statSelfCkinFull) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("�ਬ�砭��"));
        SetProp(colNode, "width", 280);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    }
    colNode = NewTextChild(headerNode, "col", getLocaleText("��� �/�"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");
    if(
            params.statType == statSelfCkinDetail or
            params.statType == statSelfCkinFull
      ) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("��� �/�"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    }
    if(
            params.statType == statSelfCkinShort or
            params.statType == statSelfCkinDetail
      ) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("���-�� ३ᮢ"));
        SetProp(colNode, "width", 85);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", flts_total);
    }
    if(params.statType == statSelfCkinFull) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("����� ३�"));
        SetProp(colNode, "width", 75);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col");
        colNode = NewTextChild(headerNode, "col", getLocaleText("���"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortDate);
        NewTextChild(rowNode, "col");
        colNode = NewTextChild(headerNode, "col", getLocaleText("���ࠢ�����"));
        SetProp(colNode, "width", 90);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    }


    // ���᪨
    colNode = NewTextChild(headerNode, "col", getLocaleText("���."));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.pax_amount);

    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.term_bag);

    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.term_bp);

    colNode = NewTextChild(headerNode, "col", getLocaleText("��� ᠬ�"));
    SetProp(colNode, "width", 45);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.pax_amount-total.term_ckin_service);

    if(params.statType == statSelfCkinFull) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.adult);
        colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.child);
        colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.baby);
    }
    if(
            params.statType == statSelfCkinDetail or
            params.statType == statSelfCkinFull
      ) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("�����."));
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.tckin);
    }
    if(
            params.statType == statSelfCkinShort or
            params.statType == statSelfCkinDetail
      ) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("�ਬ�砭��"));
        SetProp(colNode, "width", 280);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    }

    if (!showTotal)
    {
        xmlUnlinkNode(rowNode);
        xmlFreeNode(rowNode);
    };

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("����ॣ������"));
    string buf;
    switch(params.statType) {
        case statSelfCkinShort:
            buf = getLocaleText("����");
            break;
        case statSelfCkinDetail:
            buf = getLocaleText("��⠫���஢�����");
            break;
        case statSelfCkinFull:
            buf = getLocaleText("���஡���");
            break;
        default:
            throw Exception("createXMLSelfCkinStat: unexpected statType %d", params.statType);
            break;
    }
    NewTextChild(variablesNode, "stat_type_caption", buf);
}

struct TSelfCkinStatCombo : public TOrderStatItem
{
    std::pair<TSelfCkinStatKey, TSelfCkinStatRow> data;
    TStatParams params;
    TSelfCkinStatCombo(const std::pair<TSelfCkinStatKey, TSelfCkinStatRow> &aData,
        const TStatParams &aParams): data(aData), params(aParams) {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TSelfCkinStatCombo::add_header(ostringstream &buf) const
{
    buf << getLocaleText("��� ॣ.") << delim;
    buf << getLocaleText("����") << delim;
    buf << getLocaleText("�� ����") << delim;
    if (params.statType == statSelfCkinFull)
        buf << getLocaleText("�ਬ�砭��") << delim;
    buf << getLocaleText("��� �/�") << delim;
    if (params.statType == statSelfCkinDetail or params.statType == statSelfCkinFull)
        buf << getLocaleText("��� �/�") << delim;
    if (params.statType == statSelfCkinShort or params.statType == statSelfCkinDetail)
        buf << getLocaleText("���-�� ३ᮢ") << delim;
    if (params.statType == statSelfCkinFull)
    {
        buf << getLocaleText("����� ३�") << delim;
        buf << getLocaleText("���") << delim;
        buf << getLocaleText("���ࠢ�����") << delim;
    }
    buf << getLocaleText("���.") << delim;
    buf << getLocaleText("��") << delim;
    buf << getLocaleText("��") << delim;
    buf << getLocaleText("��� ᠬ�") << delim;
    if (params.statType == statSelfCkinFull)
    {
        buf << getLocaleText("��") << delim;
        buf << getLocaleText("��") << delim;
        buf << getLocaleText("��") << delim;
    }
    if (params.statType == statSelfCkinDetail or params.statType == statSelfCkinFull)
        buf << getLocaleText("�����.") << delim;
    if (params.statType == statSelfCkinShort or params.statType == statSelfCkinDetail)
        buf << getLocaleText("�ਬ�砭��") << delim;
    buf << endl;
}

void TSelfCkinStatCombo::add_data(ostringstream &buf) const
{
    string region;
    if (params.statType == statSelfCkinFull)
    {
        try { region = AirpTZRegion(data.first.ap); }
        catch(AstraLocale::UserException &E) { return; };
    }
    buf << data.first.client_type << delim; // ��� ॣ.
    buf << data.first.desk << delim; // ����
    buf << data.first.desk_airp << delim; // �/� ����
    if (params.statType == statSelfCkinFull)
        buf << data.first.descr << delim; // �ਬ�砭��
    buf << data.first.ak << delim; // ��� �/�
    if (params.statType == statSelfCkinDetail or params.statType == statSelfCkinFull)
        buf << ElemIdToCodeNative(etAirp, data.first.ap) << delim; // ��� �/�
    if (params.statType == statSelfCkinShort or params.statType == statSelfCkinDetail)
        buf << (int)data.second.flts.size() << delim; // ���-�� ३ᮢ
    if (params.statType == statSelfCkinFull)
    {
        ostringstream oss1;
        oss1 << setw(3) << setfill('0') << data.first.flt_no;
        buf << oss1.str() << delim; // ����� ३�
        buf << DateTimeToStr(UTCToClient(data.first.scd_out, region), "dd.mm.yy")
         << delim; // ���
        buf << data.first.places.get() << delim; // ���ࠢ�����
    }
    buf << data.second.pax_amount << delim; // ���-�� ����.
    buf << data.second.term_bag << delim; // ��
    buf << data.second.term_bp << delim; // ��
    buf << (data.second.pax_amount - data.second.term_ckin_service)
     << delim; // ��� ᠬ�
    if (params.statType == statSelfCkinFull)
    {
        buf << data.second.adult << delim; // ��
        buf << data.second.child << delim; // ��
        buf << data.second.baby << delim; // ��
    }
    if (params.statType == statSelfCkinDetail or params.statType == statSelfCkinFull)
        buf << data.second.tckin << delim; // �����. ॣ.
    if (params.statType == statSelfCkinShort or params.statType == statSelfCkinDetail)
        buf << data.first.descr << delim; // �ਬ�砭��
    buf << endl;
}

void RunSelfCkinStatFile(const TStatParams &params, TOrderStatWriter &writer, TPrintAirline &prn_airline)
{
    TSelfCkinStat SelfCkinStat;
    TSelfCkinStatRow SelfCkinStatTotal;
    RunSelfCkinStat(params, SelfCkinStat, SelfCkinStatTotal, prn_airline);
    for (TSelfCkinStat::const_iterator i = SelfCkinStat.begin(); i != SelfCkinStat.end(); ++i)
        writer.insert(TSelfCkinStatCombo(*i, params));
}

