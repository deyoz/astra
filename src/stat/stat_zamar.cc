#include "stat_zamar.h"
#include "qrys.h"
#include "report_common.h"
#include "stat/stat_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std; 
using namespace EXCEPTIONS; 
using namespace BASIC::date_time;
using namespace AstraLocale;

void set_stat_zamar(ZamarType type, const AirlineCode_t &airline, const AirportCode_t &airp, bool pr_ok)
{
    LogTrace(TRACE5) << __func__ << " start: " << airline << ", " << airp << ", " << pr_ok;
    try {
      QParams qryParams;
      qryParams << QParam("sbdo_type", otString, EncodeZamarType(type))
                << QParam("time", otDate, NowUTC())
                << QParam("airline", otString, airline.get())
                << QParam("airp", otString, airp.get())
                << QParam("amount_ok", otInteger, pr_ok)
                << QParam("amount_fault", otInteger, not pr_ok);
      std::string sqlInsert =
          "  INSERT INTO stat_zamar( "
          "     sbdo_type, "
          "     time, "
          "     airline, "
          "     airp, "
          "     amount_ok, "
          "     amount_fault "
          "  ) VALUES ( "
          "     :sbdo_type, "
          "     :time, "
          "     :airline, "
          "     :airp, "
          "     :amount_ok, "
          "     :amount_fault "
          "  ) ";

        DbCpp::Session& session = PgOra::getRWSession("STAT_ZAMAR");
        if (session.isOracle()) {
          DB::TCachedQuery insQry(
                session,
                "DECLARE "
                "  PRAGMA AUTONOMOUS_TRANSACTION; "
                "BEGIN "
                "  " + sqlInsert + "; "
                "  COMMIT; "
                "END; ",
                qryParams,
                STDLOG);
          insQry.get().Execute();
        } else {
          DbCpp::PgAutonomousSessionManager mngr = DbCpp::mainPgAutonomousSessionManager(STDLOG);
          DB::TCachedQuery insQry(mngr.session(), sqlInsert, qryParams, STDLOG);
          insQry.get().Execute();
          mngr.commit();
        }
    } catch(const Exception &E) {
        ProgError( STDLOG, "%s: %s", __func__, E.what());
    } catch(...) {
        ProgError( STDLOG, "%s: unknown error", __func__);
    }
}

void TZamarStatCounters::add(const TZamarStatRow &row)
{
    amount_ok += row.amount_ok;
    amount_fault += row.amount_fault;
}

void TZamarFullStat::add(const TZamarStatRow &row)
{
    prn_airline.check(row.airline.get());
    TZamarStatCounters &counters = (*this)[row.airline.get()][row.airp.get()];
    if(counters.empty()) FRowCount++;
    counters.add(row);
    totals.add(row);
}

void ArxRunZamarStat(
        const TStatParams &params,
        TZamarAbstractStat &ZamarStat
        )
{
    LogTrace5 << __func__;
    QParams QryParams;
    QryParams
        << QParam("FirstDate", otDate, params.FirstDate)
        << QParam("LastDate", otDate, params.LastDate)
        << QParam("sbdo_type", otString, EncodeZamarType(ZamarType::SBDO));

    string SQLText = "select arx_stat_zamar.* "
                     "from arx_stat_zamar "
                     "where ";
    params.AccessClause(SQLText, "arx_stat_zamar");
    SQLText += "   arx_stat_zamar.part_key >= :FirstDate AND arx_stat_zamar.part_key < :LastDate and \n"
               "   arx_stat_zamar.time >= :FirstDate AND arx_stat_zamar.time < :LastDate and "
               "   arx_stat_zamar.sbdo_type = :sbdo_type ";
    DB::TCachedQuery Qry(PgOra::getROSession("ARX_STAT_ZAMAR"), SQLText, QryParams, STDLOG);

    Qry.get().Execute();
    if(not Qry.get().Eof) {
        int col_airline = Qry.get().GetFieldIndex("airline");
        int col_airp = Qry.get().GetFieldIndex("airp");
        int col_amount_ok = Qry.get().GetFieldIndex("amount_ok");
        int col_amount_fault = Qry.get().GetFieldIndex("amount_fault");
        for(; not Qry.get().Eof; Qry.get().Next()) {
            TZamarStatRow row(
                    AirportCode_t(ElemIdToCodeNative(etAirp, Qry.get().FieldAsString(col_airp))),
                    AirlineCode_t(ElemIdToCodeNative(etAirline, Qry.get().FieldAsString(col_airline))),
                    Qry.get().FieldAsInteger(col_amount_ok),
                    Qry.get().FieldAsInteger(col_amount_fault)
                    );
            ZamarStat.add(row);
            params.overflow.check(ZamarStat.RowCount());
        }
    }
}

void RunZamarStat(
        const TStatParams &params,
        TZamarAbstractStat &ZamarStat
        )
{
    QParams QryParams;
    QryParams
        << QParam("FirstDate", otDate, params.FirstDate)
        << QParam("LastDate", otDate, params.LastDate)
        << QParam("sbdo_type", otString, EncodeZamarType(ZamarType::SBDO));

    string SQLText = "SELECT stat_zamar.* "
                     "FROM stat_zamar "
                     "WHERE ";
    params.AccessClause(SQLText, "stat_zamar");
    SQLText +=
        "   stat_zamar.time >= :FirstDate AND stat_zamar.time < :LastDate AND "
        "   stat_zamar.sbdo_type = :sbdo_type ";
    DB::TCachedQuery Qry(PgOra::getROSession("STAT_ZAMAR"), SQLText, QryParams, STDLOG);
    Qry.get().Execute();
    if(not Qry.get().Eof) {
        int col_airline = Qry.get().GetFieldIndex("airline");
        int col_airp = Qry.get().GetFieldIndex("airp");
        int col_amount_ok = Qry.get().GetFieldIndex("amount_ok");
        int col_amount_fault = Qry.get().GetFieldIndex("amount_fault");
        for(; not Qry.get().Eof; Qry.get().Next()) {
            TZamarStatRow row(
                    AirportCode_t(ElemIdToCodeNative(etAirp, Qry.get().FieldAsString(col_airp))),
                    AirlineCode_t(ElemIdToCodeNative(etAirline, Qry.get().FieldAsString(col_airline))),
                    Qry.get().FieldAsInteger(col_amount_ok),
                    Qry.get().FieldAsInteger(col_amount_fault)
                    );
            ZamarStat.add(row);
            params.overflow.check(ZamarStat.RowCount());
        }
    }

    ArxRunZamarStat(params, ZamarStat);
}

void createXMLZamarFullStat(
        const TStatParams &params,
        const TZamarFullStat &ZamarFullStat,
        xmlNodePtr resNode)
{
    if(ZamarFullStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    NewTextChild(resNode, "airline", ZamarFullStat.prn_airline.get(), "");

    xmlNodePtr grdNode = NewTextChild(resNode, "grd");
    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("���-�� �ᯥ��� ����ᮢ"));
    SetProp(colNode, "width", 150);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("���-�� �訡���� ����ᮢ"));
    SetProp(colNode, "width", 150);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;

    for(const auto &airline: ZamarFullStat) {
        for(const auto &airp: airline.second) {
            rowNode = NewTextChild(rowsNode, "row");
            // ��
            NewTextChild(rowNode, "col", airline.first);
            // ��
            NewTextChild(rowNode, "col", airp.first);
            // ���
            NewTextChild(rowNode, "col", airp.second.amount_ok);
            // �� ���
            NewTextChild(rowNode, "col", airp.second.amount_fault);
        }
    }

    rowNode = NewTextChild(rowsNode, "row");
    NewTextChild(rowNode, "col", getLocaleText("�⮣�:"));
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col", ZamarFullStat.totals.amount_ok);
    NewTextChild(rowNode, "col", ZamarFullStat.totals.amount_fault);

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", "SBDO (Zamar)");
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("���஡���"));
}

struct TZamarFullStatCombo : public TOrderStatItem
{
    string airline;
    string airp;
    int amount_ok;
    int amount_fault;

    TZamarFullStatCombo(
            const string &_airline,
            const string &_airp,
            int _amount_ok,
            int _amount_fault):
        airline(_airline),
        airp(_airp),
        amount_ok(_amount_ok),
        amount_fault(_amount_fault)
    {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TZamarFullStatCombo::add_data(ostringstream &buf) const
{
    buf
        << airline << delim
        << airp << delim
        << amount_ok << delim
        << amount_fault << endl;
}

void TZamarFullStatCombo::add_header(ostringstream &buf) const
{
    buf
        << getLocaleText("��") << delim
        << getLocaleText("��") << delim
        << getLocaleText("���-�� �ᯥ��� ����ᮢ") << delim
        << getLocaleText("���-�� �訡���� ����ᮢ") << endl;
}

void RunZamarFullFile(const TStatParams &params, TOrderStatWriter &writer)
{
    TZamarFullStat ZamarFullStat;
    RunZamarStat(params, ZamarFullStat);
    for(const auto &airline: ZamarFullStat) {
        for(const auto &airp: airline.second) {
            writer.insert(TZamarFullStatCombo(
                        airline.first,
                        airp.first,
                        airp.second.amount_ok,
                        airp.second.amount_fault));
        }
    }
}
