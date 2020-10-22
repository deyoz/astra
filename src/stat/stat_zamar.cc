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
        TCachedQuery Qry(
                "declare "
                "  pragma autonomous_transaction; "
                "begin "
                "  insert into stat_zamar( "
                "     sbdo_type, "
                "     time, "
                "     airline, "
                "     airp, "
                "     amount_ok, "
                "     amount_fault "
                "  ) values ( "
                "     :sbdo_type, "
                "     :time, "
                "     :airline, "
                "     :airp, "
                "     :amount_ok, "
                "     :amount_fault "
                "  ); "
                "  commit; "
                "end; ",
                QParams()
                << QParam("sbdo_type", otString, EncodeZamarType(type))
                << QParam("time", otDate, NowUTC())
                << QParam("airline", otString, airline.get())
                << QParam("airp", otString, airp.get())
                << QParam("amount_ok", otInteger, pr_ok)
                << QParam("amount_fault", otInteger, not pr_ok));
        Qry.get().Execute();
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

void RunZamarStat(
        const TStatParams &params,
        TZamarAbstractStat &ZamarStat
        )
{
    for(int pass = 0; pass <= 1; pass++) {
        QParams QryParams;
        QryParams
            << QParam("FirstDate", otDate, params.FirstDate)
            << QParam("LastDate", otDate, params.LastDate)
            << QParam("sbdo_type", otString, EncodeZamarType(ZamarType::SBDO));

        string SQLText = "select stat_zamar.* from ";
        if(pass != 0) {
            SQLText +=
                "   arx_stat_zamar stat_zamar ";
        } else {
            SQLText +=
                "   stat_zamar ";
        }
        SQLText +=
            "where ";
        params.AccessClause(SQLText, "stat_zamar");

        if(pass != 0)
            SQLText +=
                "    stat_zamar.part_key >= :FirstDate AND stat_zamar.part_key < :LastDate and \n";
        SQLText +=
            "   stat_zamar.time >= :FirstDate AND stat_zamar.time < :LastDate and "
            "   stat_zamar.sbdo_type = :sbdo_type ";
        TCachedQuery Qry(SQLText, QryParams);

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
    colNode = NewTextChild(headerNode, "col", getLocaleText("АК"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("АП"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Кол-во успешных запросов"));
    SetProp(colNode, "width", 150);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Кол-во ошибочных запросов"));
    SetProp(colNode, "width", 150);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;

    for(const auto &airline: ZamarFullStat) {
        for(const auto &airp: airline.second) {
            rowNode = NewTextChild(rowsNode, "row");
            // АК
            NewTextChild(rowNode, "col", airline.first);
            // АП
            NewTextChild(rowNode, "col", airp.first);
            // Норм
            NewTextChild(rowNode, "col", airp.second.amount_ok);
            // Не норм
            NewTextChild(rowNode, "col", airp.second.amount_fault);
        }
    }

    rowNode = NewTextChild(rowsNode, "row");
    NewTextChild(rowNode, "col", getLocaleText("Итого:"));
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col", ZamarFullStat.totals.amount_ok);
    NewTextChild(rowNode, "col", ZamarFullStat.totals.amount_fault);

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", "SBDO (Zamar)");
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("Подробная"));
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
        << getLocaleText("АК") << delim
        << getLocaleText("АП") << delim
        << getLocaleText("Кол-во успешных запросов") << delim
        << getLocaleText("Кол-во ошибочных запросов") << endl;
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
