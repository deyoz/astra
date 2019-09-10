#include "stat_tlg_out.h"
#include "tlg/tlg.h"
#include "stat_utils.h"
#include "report_common.h"

using namespace std;
using namespace AstraLocale;
using namespace ASTRA;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;

bool TTlgOutStatRow::operator == (const TTlgOutStatRow &item) const
{
    return tlg_count == item.tlg_count &&
        tlg_len == item.tlg_len;
    // FIXME проверка на равенство double tlg_len (c++ double equality comparison)
    // http://en.cppreference.com/w/cpp/types/numeric_limits/epsilon
};

void TTlgOutStatRow::operator += (const TTlgOutStatRow &item)
{
    tlg_count += item.tlg_count;
    tlg_len += item.tlg_len;
};

bool TTlgOutStatCmp::operator() (const TTlgOutStatKey &key1, const TTlgOutStatKey &key2) const
{
    if (key1.sender_sita_addr!=key2.sender_sita_addr)
        return key1.sender_sita_addr < key2.sender_sita_addr;
    if (key1.receiver_descr!=key2.receiver_descr)
        return key1.receiver_descr < key2.receiver_descr;
    if (key1.receiver_sita_addr!=key2.receiver_sita_addr)
        return key1.receiver_sita_addr < key2.receiver_sita_addr;
    if (key1.receiver_country_view!=key2.receiver_country_view)
        return key1.receiver_country_view < key2.receiver_country_view;
    if (key1.time_send!=key2.time_send)
        return key1.time_send < key2.time_send;
    if (key1.airline_view!=key2.airline_view)
        return key1.airline_view < key2.airline_view;
    if (key1.flt_no!=key2.flt_no)
        return key1.flt_no < key2.flt_no;
    if (key1.suffix_view!=key2.suffix_view)
        return key1.suffix_view < key2.suffix_view;
    if (key1.airp_dep_view!=key2.airp_dep_view)
        return key1.airp_dep_view < key2.airp_dep_view;
    if (key1.scd_local_date!=key2.scd_local_date)
        return key1.scd_local_date < key2.scd_local_date;
    if (key1.tlg_type!=key2.tlg_type)
        return key1.tlg_type < key2.tlg_type;
    if(key1.airline_mark_view != key2.airline_mark_view)
        return key1.airline_mark_view < key2.airline_mark_view;
    return key1.extra < key2.extra;
}


void RunTlgOutStat(const TStatParams &params,
                   TTlgOutStat &TlgOutStat, TTlgOutStatRow &TlgOutStatTotal,
                   TPrintAirline &prn_airline)
{
    TQuery Qry(&OraSession);
    for(int pass = 0; pass <= 1; pass++) {
        string SQLText =
            "SELECT \n"
            "  tlg_stat.sender_sita_addr, \n"
            "  tlg_stat.receiver_descr, \n"
            "  tlg_stat.receiver_sita_addr, \n"
            "  tlg_stat.receiver_country, \n"
            "  tlg_stat.time_send, \n"
            "  tlg_stat.airline, \n"
            "  tlg_stat.flt_no, \n"
            "  tlg_stat.suffix, \n"
            "  tlg_stat.airp_dep, \n"
            "  tlg_stat.scd_local_date, \n"
            "  tlg_stat.tlg_type, \n"
            "  tlg_stat.airline_mark, \n"
            "  tlg_stat.extra, \n"
            "  tlg_stat.tlg_len \n"
            "FROM \n";
        if(pass != 0) {
            SQLText +=
                "   arx_tlg_stat tlg_stat \n";
        } else {
            SQLText +=
                "   tlg_stat \n";
        }
        //SQLText += "WHERE rownum < 1000\n"; //SQLText += "WHERE \n";
        SQLText += "WHERE sender_canon_name=:own_canon_name AND \n";
        params.AccessClause(SQLText, "tlg_stat", "airline", "airp_dep");
        if(!params.typeb_type.empty()) {
            SQLText += " tlg_stat.tlg_type = :tlg_type and \n";
            Qry.CreateVariable("tlg_type", otString, params.typeb_type);
        }
        if(!params.sender_addr.empty()) {
            SQLText += " tlg_stat.sender_sita_addr = :sender_sita_addr and \n";
            Qry.CreateVariable("sender_sita_addr", otString, params.sender_addr);
        }
        if(!params.receiver_descr.empty()) {
            SQLText += " tlg_stat.receiver_descr = :receiver_descr and \n";
            Qry.CreateVariable("receiver_descr", otString, params.receiver_descr);
        }
        if (pass!=0)
          SQLText +=
            "    tlg_stat.part_key >= :FirstDate AND tlg_stat.part_key < :LastDate \n";
        else
          SQLText +=
            "    tlg_stat.time_send >= :FirstDate AND tlg_stat.time_send < :LastDate \n";

        //ProgTrace(TRACE5, "RunTlgOutStat: pass=%d SQL=\n%s", pass, SQLText.c_str());
        Qry.SQLText = SQLText;
        Qry.CreateVariable("own_canon_name", otString, OWN_CANON_NAME());
        Qry.CreateVariable("FirstDate", otDate, params.FirstDate);
        Qry.CreateVariable("LastDate", otDate, params.LastDate);
        Qry.Execute();
        if(not Qry.Eof) {
            int col_sender_sita_addr = Qry.FieldIndex("sender_sita_addr");
            int col_receiver_descr = Qry.FieldIndex("receiver_descr");
            int col_receiver_sita_addr = Qry.FieldIndex("receiver_sita_addr");
            int col_receiver_country = Qry.FieldIndex("receiver_country");
            int col_time_send = Qry.FieldIndex("time_send");
            int col_airline = Qry.FieldIndex("airline");
            int col_flt_no = Qry.FieldIndex("flt_no");
            int col_suffix = Qry.FieldIndex("suffix");
            int col_airp_dep = Qry.FieldIndex("airp_dep");
            int col_scd_local_date = Qry.FieldIndex("scd_local_date");
            int col_tlg_type = Qry.FieldIndex("tlg_type");
            int col_airline_mark = Qry.FieldIndex("airline_mark");
            int col_extra = Qry.FieldIndex("extra");
            int col_tlg_len = Qry.FieldIndex("tlg_len");
            for(; not Qry.Eof; Qry.Next()) {
              string airline = Qry.FieldAsString(col_airline);
              prn_airline.check(airline);

              TTlgOutStatRow row;
              row.tlg_count = 1;
              row.tlg_len = Qry.FieldAsInteger(col_tlg_len);
              if (!params.skip_rows)
              {
                TTlgOutStatKey key;
                key.sender_sita_addr = Qry.FieldAsString(col_sender_sita_addr);
                key.receiver_descr = Qry.FieldAsString(col_receiver_descr);
                if (params.statType == statTlgOutShort)
                {
                  //для общей статистики выводим помимо кода гос-ва еще и полное название
                  key.receiver_country_view = ElemIdToNameLong(etCountry, Qry.FieldAsString(col_receiver_country));
                  if (!key.receiver_country_view.empty()) key.receiver_country_view += " / ";
                };
                key.receiver_country_view += ElemIdToCodeNative(etCountry, Qry.FieldAsString(col_receiver_country));
                key.extra = Qry.FieldAsString(col_extra);
                if (params.statType == statTlgOutDetail ||
                    params.statType == statTlgOutFull)
                {
                  key.airline_view = ElemIdToCodeNative(etAirline, airline);
                  key.airline_mark_view = ElemIdToCodeNative(etAirline, Qry.FieldAsString(col_airline_mark));
                  key.airp_dep_view = ElemIdToCodeNative(etAirp, Qry.FieldAsString(col_airp_dep));

                  if (params.statType == statTlgOutFull)
                  {
                    key.receiver_sita_addr = Qry.FieldAsString(col_receiver_sita_addr);
                    if (!Qry.FieldIsNULL(col_time_send))
                    {
                      key.time_send = Qry.FieldAsDateTime(col_time_send);
                      modf(key.time_send, &key.time_send);
                    };
                    if (!Qry.FieldIsNULL(col_flt_no))
                      key.flt_no = Qry.FieldAsInteger(col_flt_no);
                    key.suffix_view = ElemIdToCodeNative(etSuffix, Qry.FieldAsString(col_suffix));
                    if (!Qry.FieldIsNULL(col_scd_local_date))
                    {
                      key.scd_local_date = Qry.FieldAsDateTime(col_scd_local_date);
                      modf(key.scd_local_date, &key.scd_local_date);
                    };
                    key.tlg_type=Qry.FieldAsString(col_tlg_type);
                  };
                };
                AddStatRow(params.overflow, key, row, TlgOutStat);
              }
              else
              {
                TlgOutStatTotal+=row;
              };
            }
        }
    }
    return;
}

void createXMLTlgOutStat(const TStatParams &params,
                         const TTlgOutStat &TlgOutStat, const TTlgOutStatRow &TlgOutStatTotal,
                         const TPrintAirline &airline, xmlNodePtr resNode)
{
    if(TlgOutStat.empty() && TlgOutStatTotal==TTlgOutStatRow())
      throw AstraLocale::UserException("MSG.NOT_DATA");

    NewTextChild(resNode, "airline", airline.get(), "");
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");
    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    TTlgOutStatRow total;
    ostringstream buf;
    bool showTotal=true;
    if (!params.skip_rows)
    {
      int rows = 0;
      for(TTlgOutStat::const_iterator im = TlgOutStat.begin(); im != TlgOutStat.end(); ++im, rows++)
      {
          rowNode = NewTextChild(rowsNode, "row");
          NewTextChild(rowNode, "col", im->first.sender_sita_addr);
          NewTextChild(rowNode, "col", im->first.receiver_descr);
          if (params.statType == statTlgOutFull)
            NewTextChild(rowNode, "col", im->first.receiver_sita_addr);
          NewTextChild(rowNode, "col", im->first.receiver_country_view);
          if (params.statType == statTlgOutFull)
          {
            if (im->first.time_send!=NoExists)
              NewTextChild(rowNode, "col", DateTimeToStr(im->first.time_send, "dd.mm.yy"));
            else
              NewTextChild(rowNode, "col");
          };
          if (params.statType == statTlgOutDetail ||
              params.statType == statTlgOutFull)
          {
            NewTextChild(rowNode, "col", im->first.airline_view);
            NewTextChild(rowNode, "col", im->first.airline_mark_view);
            NewTextChild(rowNode, "col", im->first.airp_dep_view);
          };
          if (params.statType == statTlgOutFull)
          {
            NewTextChild(rowNode, "col", im->first.tlg_type);
            if (im->first.scd_local_date!=NoExists)
              NewTextChild(rowNode, "col", DateTimeToStr(im->first.scd_local_date, "dd.mm.yy"));
            else
              NewTextChild(rowNode, "col");
            if (im->first.flt_no!=NoExists)
            {
              buf.str("");
              buf << setw(3) << setfill('0') << im->first.flt_no
                  << im->first.suffix_view;
              NewTextChild(rowNode, "col", buf.str());
            }
            else
              NewTextChild(rowNode, "col");
          };
          NewTextChild(rowNode, "col", im->second.tlg_count);
          buf.str("");
          buf << fixed << setprecision(0) << im->second.tlg_len;
          NewTextChild(rowNode, "col", buf.str());
          NewTextChild(rowNode, "col", im->first.extra);

          total += im->second;
      };
    }
    else total=TlgOutStatTotal;

    rowNode = NewTextChild(rowsNode, "row");

    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("Адрес отпр."));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col", getLocaleText("Итого:"));
    colNode = NewTextChild(headerNode, "col", getLocaleText("Канал"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");
    if (params.statType == statTlgOutFull)
    {
      colNode = NewTextChild(headerNode, "col", getLocaleText("Адрес получ."));
      SetProp(colNode, "width", 70);
      SetProp(colNode, "align", TAlignment::LeftJustify);
      SetProp(colNode, "sort", sortString);
      NewTextChild(rowNode, "col");
    };
    colNode = NewTextChild(headerNode, "col", getLocaleText("Гос-во"));
    if (params.statType == statTlgOutShort)
      SetProp(colNode, "width", 200);
    else
      SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");
    if (params.statType == statTlgOutFull)
    {
      colNode = NewTextChild(headerNode, "col", getLocaleText("Дата отпр."));
      SetProp(colNode, "width", 60);
      SetProp(colNode, "align", TAlignment::LeftJustify);
      SetProp(colNode, "sort", sortDate);
      NewTextChild(rowNode, "col");
    };
    if (params.statType == statTlgOutDetail ||
        params.statType == statTlgOutFull)
    {
      colNode = NewTextChild(headerNode, "col", getLocaleText("А/к факт."));
      SetProp(colNode, "width", 50);
      SetProp(colNode, "align", TAlignment::LeftJustify);
      SetProp(colNode, "sort", sortString);
      NewTextChild(rowNode, "col");
      colNode = NewTextChild(headerNode, "col", getLocaleText("А/к комм."));
      SetProp(colNode, "width", 50);
      SetProp(colNode, "align", TAlignment::LeftJustify);
      SetProp(colNode, "sort", sortString);
      NewTextChild(rowNode, "col");
      colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/п"));
      SetProp(colNode, "width", 50);
      SetProp(colNode, "align", TAlignment::LeftJustify);
      SetProp(colNode, "sort", sortString);
      NewTextChild(rowNode, "col");
    };
    if (params.statType == statTlgOutFull)
    {
      colNode = NewTextChild(headerNode, "col", getLocaleText("Тип тлг."));
      SetProp(colNode, "width", 50);
      SetProp(colNode, "align", TAlignment::LeftJustify);
      SetProp(colNode, "sort", sortString);
      NewTextChild(rowNode, "col");
      colNode = NewTextChild(headerNode, "col", getLocaleText("Дата вылета"));
      SetProp(colNode, "width", 70);
      SetProp(colNode, "align", TAlignment::LeftJustify);
      SetProp(colNode, "sort", sortDate);
      NewTextChild(rowNode, "col");
      colNode = NewTextChild(headerNode, "col", getLocaleText("Рейс"));
      SetProp(colNode, "width", 50);
      SetProp(colNode, "align", TAlignment::LeftJustify);
      SetProp(colNode, "sort", sortString);
      NewTextChild(rowNode, "col");
    };
    colNode = NewTextChild(headerNode, "col", getLocaleText("Кол-во"));
    SetProp(colNode, "width", params.statType == statTlgOutFull?40:70);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.tlg_count);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Объем (байт)"));
    SetProp(colNode, "width", params.statType == statTlgOutFull?70:100);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortFloat);
    buf.str("");
    buf << fixed << setprecision(0) << total.tlg_len;
    NewTextChild(rowNode, "col", buf.str());
    colNode = NewTextChild(headerNode, "col", getLocaleText("№ договора"));
    SetProp(colNode, "width", 150);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");

    if (!showTotal)
    {
      xmlUnlinkNode(rowNode);
      xmlFreeNode(rowNode);
    };

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("Статистика отправленных телеграмм"));
    string stat_type_caption;
    switch(params.statType) {
        case statTlgOutShort:
            stat_type_caption = getLocaleText("Общая");
            break;
        case statTlgOutDetail:
            stat_type_caption = getLocaleText("Детализированная");
            break;
        case statTlgOutFull:
            stat_type_caption = getLocaleText("Подробная");
            break;
        default:
            throw Exception("createXMLTlgOutStat: unexpected statType %d", params.statType);
            break;
    }
    NewTextChild(variablesNode, "stat_type_caption", stat_type_caption);
}

struct TTlgOutStatCombo : public TOrderStatItem
{
    std::pair<TTlgOutStatKey, TTlgOutStatRow> data;
    TStatParams params;
    TTlgOutStatCombo(const std::pair<TTlgOutStatKey, TTlgOutStatRow> &aData,
        const TStatParams &aParams): data(aData), params(aParams) {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TTlgOutStatCombo::add_header(ostringstream &buf) const
{
    buf << "Адрес отпр." << delim;
    buf << "Канал" << delim;
    if (params.statType == statTlgOutFull)
        buf << "Адрес получ." << delim;
    buf << "Гос-во" << delim;
    if (params.statType == statTlgOutFull)
        buf << "Дата отпр." << delim;
    if (params.statType == statTlgOutDetail || params.statType == statTlgOutFull)
    {
        buf << "А/к факт." << delim;
        buf << "А/к комм." << delim;
        buf << "Код а/п" << delim;
    }
    if (params.statType == statTlgOutFull)
    {
        buf << "Тип тлг." << delim;
        buf << "Дата вылета" << delim;
        buf << "Рейс" << delim;
    }
    buf << "Кол-во" << delim;
    buf << "Объем (байт)" << delim;
    buf << "№ договора" << endl;
}

void TTlgOutStatCombo::add_data(ostringstream &buf) const
{
    buf << data.first.sender_sita_addr << delim; // Адрес отпр.
    buf << data.first.receiver_descr << delim; // Канал
    if (params.statType == statTlgOutFull)
        buf << data.first.receiver_sita_addr << delim; // Адрес получ.
    buf << data.first.receiver_country_view << delim; // Гос-во
    if (params.statType == statTlgOutFull)
    {
        if (data.first.time_send != NoExists)
            buf << DateTimeToStr(data.first.time_send, "dd.mm.yy"); // Дата отпр.
        buf << delim;
    }
    if (params.statType == statTlgOutDetail || params.statType == statTlgOutFull)
    {
        buf << data.first.airline_view << delim; // А/к факт.
        buf << data.first.airline_mark_view << delim; // А/к комм.
        buf << data.first.airp_dep_view << delim; // Код а/п
    }
    if (params.statType == statTlgOutFull)
    {
        buf << data.first.tlg_type << delim; // Тип тлг.
        if (data.first.scd_local_date != NoExists)
            buf << DateTimeToStr(data.first.scd_local_date, "dd.mm.yy"); // Дата вылета
        buf << delim;
        if (data.first.flt_no != NoExists)
        {
            ostringstream oss1;
            oss1 << setw(3) << setfill('0')
             << data.first.flt_no << data.first.suffix_view;
            buf << oss1.str(); // Рейс
        }
        buf << delim;
    }
    buf << data.second.tlg_count << delim; // Кол-во
    ostringstream oss2;
    oss2 << fixed << setprecision(0) << data.second.tlg_len;
    buf << oss2.str() << delim; // Объем (байт)
    buf << data.first.extra << endl; // № договора
}


void RunTlgOutStatFile(const TStatParams &params, TOrderStatWriter &writer, TPrintAirline &prn_airline)
{
    TTlgOutStat TlgOutStat;
    TTlgOutStatRow TlgOutStatTotal;
    RunTlgOutStat(params, TlgOutStat, TlgOutStatTotal, prn_airline);
    for (TTlgOutStat::const_iterator i = TlgOutStat.begin(); i != TlgOutStat.end(); ++i)
        writer.insert(TTlgOutStatCombo(*i, params));
}

