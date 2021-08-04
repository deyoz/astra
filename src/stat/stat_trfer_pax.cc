#include "stat_trfer_pax.h"
#include "astra_misc.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "qrys.h"
#include "passenger.h"
#include "report_common.h"
#include "stat_utils.h"
#include "baggage_ckin.h"

#define NICKNAME "DENIS"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/slogger.h"

template void RunTrferPaxStat(TStatParams const&, TOrderStatWriter&, TPrintAirline&);
template void RunTrferPaxStat(TStatParams const&, TTrferPaxStat&, TPrintAirline&);

using namespace std;
using namespace ASTRA;
using namespace AstraLocale;
using namespace BASIC::date_time;

void TTrferPaxStatItem::clear()
{
    airline.clear();
    airp.clear();
    flt_no1 = NoExists;
    suffix1.clear();

    date1 = 0;

    trfer_airp.clear();
    flt_no2 = NoExists;
    suffix2.clear();

    date2 = 0;

    airp_arv.clear();
    seg_category = TSegCategories::Unknown;
    pax_name.clear();
    pax_doc.clear();
    pax_amount = 0;
    adult = 0;
    child = 0;
    baby = 0;

    rk_weight = 0;
    bag_amount = 0;
    bag_weight = 0;
}

void TTrferPaxStatItem::add_header(ostringstream &buf) const
{
    buf
        << getLocaleText("АК") << delim
        << getLocaleText("АПВ") << delim
        << getLocaleText("Сег.1") << delim
        << getLocaleText("Дата") << delim
        << getLocaleText("АПТ") << delim
        << getLocaleText("Сег.2") << delim
        << getLocaleText("Дата") << delim
        << getLocaleText("АПП") << delim
        << getLocaleText("Категория") << delim
        << getLocaleText("ФИО пассажира") << delim
        << getLocaleText("Документ") << delim
        << getLocaleText("CAP.DOC.PAX") << delim
        << getLocaleText("ВЗ") << delim
        << getLocaleText("РБ") << delim
        << getLocaleText("РМ") << delim
        << getLocaleText("Р/к") << delim
        << getLocaleText("БГ мест") << delim
        << getLocaleText("БГ вес") << delim
        << getLocaleText("Бирки") << endl;
}

void TTrferPaxStatItem::add_data(ostringstream &buf) const
{
    if(airline.empty()) {
        buf
            << getLocaleText("Итого:") << delim
            << delim
            << delim
            << delim
            << delim
            << delim
            << delim
            << delim
            << delim
            << delim
            << delim;
    } else {
        // АК
        buf << ElemIdToCodeNative(etAirline, airline) << delim;
        // АП
        buf << ElemIdToCodeNative(etAirp, airp) << delim;
        //Сег.1
        ostringstream tmp;
        tmp << setw(3) << setfill('0') << flt_no1 << ElemIdToCodeNative(etSuffix, suffix1);
        buf << tmp.str() << delim;
        //Дата
        buf << DateTimeToStr(date1, "dd.mm.yyyy") << delim;
        //Трансфер
        buf << ElemIdToCodeNative(etAirp, trfer_airp) << delim;
        //Сег.2
        tmp.str("");
        tmp
            << ElemIdToCodeNative(etAirline, airline2)
            << setw(3) << setfill('0') << flt_no2 << ElemIdToCodeNative(etSuffix, suffix2);
        buf << tmp.str() << delim;
        //Дата
        buf << DateTimeToStr(date2, "dd.mm.yyyy") << delim;
        //А/п прилета
        buf << ElemIdToCodeNative(etAirp, airp_arv) << delim;
        //Категория
        buf << getLocaleText(TSegCategory().encode(seg_category)) << delim;
        //ФИО
        buf << pax_name << delim;
        //Паспорт
        buf << pax_doc << delim;
    }
    //Кол-во пасс.
    buf << pax_amount << delim;
    //ВЗ
    buf << adult << delim;
    //РБ
    buf << child << delim;
    //РМ
    buf << baby << delim;
    //Р/кладь(вес)
    buf << rk_weight << delim;
    //БГ мест
    buf << bag_amount << delim;
    //БГ вес
    buf << bag_weight << delim;
    //Бирки
    buf << tags << endl;
}

void getSegList(const string &segments, list<pair<TTripInfo, string> > &seg_list)
{
    vector<string> tokens;
    boost::split(tokens, segments, boost::is_any_of(";"));
    for(vector<string>::iterator i = tokens.begin();
            i != tokens.end(); i++) {
        vector<string> flt_info;
        boost::split(flt_info, *i, boost::is_any_of(","));
        TTripInfo flt;
        flt.airline = flt_info[0];
        flt.airp = flt_info[1];
        flt.flt_no = ToInt(flt_info[2]);
        flt.suffix = flt_info[3];
        StrToDateTime(flt_info[4].c_str(), ServerFormatDateTimeAsString, flt.scd_out);

        string airp_arv;
        if(flt_info.size() > 5)
            airp_arv = flt_info[5];
        seg_list.push_back(make_pair(flt, airp_arv));
    }
}


template <class T>
void ArxRunTrferPaxStat(
        const TStatParams &params,
        T &TrferPaxStat,
        TPrintAirline &prn_airline
        )
{
    LogTrace5 << __func__;
    QParams QryParams;
    QryParams
        << QParam("FirstDate", otDate, params.FirstDate)
        << QParam("LastDate", otDate, params.LastDate);
        //<< QParam("pr_lat", otInteger, TReqInfo::Instance()->desk.lang!=AstraLocale::LANG_RU);
    TTrferPaxStatItem totals;
    for(int pass = 1; pass <= 2; pass++) {
        QryParams << QParam("arx_trip_date_range", otDate, params.LastDate + ARX_TRIP_DATE_RANGE());
        string SQLText =
            "select arx_points.part_key, "
            "   arx_trfer_pax_stat.*, "
            "   arx_pax.surname||' '||arx_pax.name full_name, "
            "   arx_pax.pers_type, arx_pax.grp_id, arx_pax.bag_pool_num ";

        SQLText +=
            "from arx_trfer_pax_stat, arx_pax, arx_points ";
        if (pass==2) {
            SQLText += getMoveArxQuery();
        }
        SQLText +=
            "where  arx_points.part_key = arx_trfer_pax_stat.part_key and "
            "       arx_pax.part_key    = arx_trfer_pax_stat.part_key and ";
        if (pass==1) {
            SQLText += " arx_points.part_key >= :FirstDate AND arx_points.part_key < :arx_trip_date_range AND ";
        }
        if (pass==2) {
            SQLText += " arx_points.part_key=arx_ext.part_key AND arx_points.move_id=arx_ext.move_id AND ";
        }
        SQLText += "   arx_trfer_pax_stat.point_id = arx_points.point_id and ";
        params.AccessClause(SQLText, "arx_points");
        SQLText +=
            "   arx_trfer_pax_stat.scd_out >= :FirstDate AND arx_trfer_pax_stat.scd_out < :LastDate and "
            "   arx_trfer_pax_stat.pax_id = arx_pax.pax_id ";
        DB::TCachedQuery Qry(PgOra::getROSession("ARX_POINTS"), SQLText, QryParams, STDLOG);
        Qry.get().Execute();
        LogTrace(TRACE5) << __func__ << "    " << SQLText;
        using namespace CKIN;
        std::map<PointId_t, BagReader> bag_readers;
        if(not Qry.get().Eof) {
            int col_part_key = Qry.get().FieldIndex("part_key");
            int col_pax_id = Qry.get().FieldIndex("pax_id");
            int col_grp_id = Qry.get().FieldIndex("grp_id");
            int col_bag_pool_num = Qry.get().FieldIndex("bag_pool_num");
            int col_rk_weight = Qry.get().FieldIndex("rk_weight");
            int col_bag_weight = Qry.get().FieldIndex("bag_weight");
            int col_bag_amount = Qry.get().FieldIndex("bag_amount");
            int col_segments = Qry.get().FieldIndex("segments");
            int col_full_name = Qry.get().FieldIndex("full_name");
            int col_pers_type = Qry.get().FieldIndex("pers_type");
            for(; not Qry.get().Eof; Qry.get().Next()) {
                PointId_t point_id{Qry.get().FieldAsInteger("point_id")};
                std::optional<Dates::DateTime_t> part_key = std::nullopt;
                if(not Qry.get().FieldIsNULL(col_part_key))
                    part_key = DateTimeToBoost(Qry.get().FieldAsDateTime(col_part_key));
                if(!algo::contains(bag_readers, point_id)) {
                    bag_readers[point_id] = BagReader(point_id, part_key, READ::BAGS_AND_TAGS);
                }

                int pax_id = Qry.get().FieldAsInteger(col_pax_id);
                int rk_weight = Qry.get().FieldAsInteger(col_rk_weight);
                int bag_weight = Qry.get().FieldAsInteger(col_bag_weight);
                int bag_amount = Qry.get().FieldAsInteger(col_bag_amount);
                string segments = Qry.get().FieldAsString(col_segments);
                string full_name = Qry.get().FieldAsString(col_full_name);
                string pers_type = Qry.get().FieldAsString(col_pers_type);
                GrpId_t grp_id(Qry.get().FieldAsInteger(col_grp_id));
                std::optional<int> opt_bag_pool_num = std::nullopt;
                if(!Qry.get().FieldIsNULL(col_bag_pool_num)) {
                    opt_bag_pool_num = Qry.get().FieldAsInteger("bag_pool_num");
                }
                string tags = bag_readers[point_id].tags(grp_id, opt_bag_pool_num,
                     TReqInfo::Instance()->desk.lang);

                list<pair<TTripInfo, string> > seg_list;
                getSegList(segments, seg_list);

                TTrferPaxStat tmp_stat;
                TTrferPaxStatItem item;
                for(list<pair<TTripInfo, string> >::iterator flt = seg_list.begin();
                        flt != seg_list.end(); flt++) {
                    if(item.airline.empty()) {
                        prn_airline.check(flt->first.airline);
                        item.airline = flt->first.airline;
                        item.airp = flt->first.airp;
                        item.flt_no1 = flt->first.flt_no;
                        item.suffix1 = flt->first.suffix;
                        item.date1 = flt->first.scd_out;
                    } else {
                        item.trfer_airp = flt->first.airp;
                        item.airline2 = flt->first.airline;
                        item.flt_no2 = flt->first.flt_no;
                        item.suffix2 = flt->first.suffix;
                        item.date2 = flt->first.scd_out;
                        item.airp_arv = flt->second;
                        item.pax_name = transliter(full_name, 1, TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU);

                        item.pax_doc = CheckIn::GetPaxDocStr(part_key, pax_id, false);
                        typedef map<bool, TSegCategories::Enum> TSeg2Map;
                        typedef map<bool, TSeg2Map> TCategoryMap;

                        static const TCategoryMap category_map =
                        {
                            {
                                false,
                                {
                                    {false, TSegCategories::IntInt},
                                    {true, TSegCategories::IntFor}
                                }
                            },
                            {
                                true,
                                {
                                    {false, TSegCategories::ForInt},
                                    {true, TSegCategories::ForFor}
                                }
                            }
                        };

                        string country1 = get_airp_country(item.airp);
                        string country2 = get_airp_country(item.trfer_airp);
                        string country3 = get_airp_country(item.airp_arv);

                        bool is_inter1 = country1 != country2;
                        bool is_inter2 = country2 != country3;
                        item.seg_category = category_map.at(is_inter1).at(is_inter2);

                        TSegCategories::Enum seg_category = TSegCategories::Unknown;
                        if(params.seg_category != TSegCategories::Unknown) {
                            seg_category = item.seg_category;
                        }

                        if(
                                params.seg_category == seg_category and
                                (params.trfer_airp.empty() or params.trfer_airp == item.trfer_airp) and
                                (params.trfer_airline.empty() or params.trfer_airline == item.airline)
                          )
                            tmp_stat.push_back(item);

                        item.clear();
                        item.airline = flt->first.airline;
                        item.airp = flt->first.airp;
                        item.flt_no1 = flt->first.flt_no;
                        item.suffix1 = flt->first.suffix;
                        item.date1 = flt->first.scd_out;
                    }
                }

                if(tmp_stat.begin() != tmp_stat.end()) {
                    tmp_stat.begin()->pax_amount = 1;
                    tmp_stat.begin()->adult = pers_type == "ВЗ";
                    tmp_stat.begin()->child = pers_type == "РБ";
                    tmp_stat.begin()->baby = pers_type == "РМ";
                    tmp_stat.begin()->rk_weight = rk_weight;
                    tmp_stat.begin()->bag_amount = bag_amount;
                    tmp_stat.begin()->bag_weight = bag_weight;
                    tmp_stat.begin()->tags = tags;

                    totals.pax_amount++;
                    totals.adult += tmp_stat.begin()->adult;
                    totals.child += tmp_stat.begin()->child;
                    totals.baby += tmp_stat.begin()->baby;
                    totals.rk_weight += rk_weight;
                    totals.bag_amount += bag_amount;
                    totals.bag_weight += bag_weight;
                }

                for(TTrferPaxStat::iterator i = tmp_stat.begin();
                        i != tmp_stat.end(); i++) TrferPaxStat.push_back(*i);

                params.overflow.check(TrferPaxStat.size());
            }
        }
    }
    if(totals.pax_amount != 0)
        TrferPaxStat.push_back(totals);
}

template <class T>
void RunTrferPaxStat(
        const TStatParams &params,
        T &TrferPaxStat,
        TPrintAirline &prn_airline
        )
{
    LogTrace5 << __func__;
    QParams QryParams;
    QryParams
        << QParam("FirstDate", otDate, params.FirstDate)
        << QParam("LastDate", otDate, params.LastDate);
    TTrferPaxStatItem totals;

    DB::TCachedQuery Qry(
          PgOra::getROSession("TRFER_PAX_STAT"),
          "SELECT * FROM trfer_pax_stat "
          "WHERE scd_out >= :FirstDate "
          "AND scd_out < :LastDate ",
          QryParams,
          STDLOG);
    Qry.get().Execute();
    using namespace CKIN;
    std::map<PointId_t, BagReader> bag_readers;
    if(not Qry.get().Eof) {
        int col_point_id = Qry.get().FieldIndex("point_id");
        int col_pax_id = Qry.get().FieldIndex("pax_id");
        int col_rk_weight = Qry.get().FieldIndex("rk_weight");
        int col_bag_weight = Qry.get().FieldIndex("bag_weight");
        int col_bag_amount = Qry.get().FieldIndex("bag_amount");
        int col_segments = Qry.get().FieldIndex("segments");
        for(; not Qry.get().Eof; Qry.get().Next()) {
            PointId_t point_id{Qry.get().FieldAsInteger(col_point_id)};
            if(!algo::contains(bag_readers, point_id)) {
                bag_readers[point_id] = BagReader(point_id, std::nullopt, READ::BAGS_AND_TAGS);
            }
            TTripInfo flt;
            flt.getByPointId(point_id.get());
            if(not params.ap.empty() && flt.airp != params.ap) {
                continue;
            }

            if(not params.ak.empty() && flt.airline != params.ak) {
                continue;
            }

            if(params.flt_no != NoExists && flt.flt_no != params.flt_no) {
                continue;
            }

            int pax_id = Qry.get().FieldAsInteger(col_pax_id);
            int rk_weight = Qry.get().FieldAsInteger(col_rk_weight);
            int bag_weight = Qry.get().FieldAsInteger(col_bag_weight);
            int bag_amount = Qry.get().FieldAsInteger(col_bag_amount);
            string segments = Qry.get().FieldAsString(col_segments);
            CheckIn::TSimplePaxItem pax;
            if (!pax.getByPaxId(pax_id)) {
                continue;
            }
            DB::TQuery QryBricks2(PgOra::getROSession("PAX"), STDLOG);
            QryBricks2.SQLText =
                "SELECT grp_id, bag_pool_num "
                "FROM pax "
                "WHERE pax_id = :pax_id ";
            QryBricks2.CreateVariable("pax_id", otInteger, pax_id);
            QryBricks2.Execute();
            if (QryBricks2.Eof) {
              continue;
            }
            GrpId_t grp_id(QryBricks2.FieldAsInteger("grp_id"));
            std::optional<int> bag_pool_num = std::nullopt;
            if(!QryBricks2.FieldIsNULL("bag_pool_num")) {
                bag_pool_num = QryBricks2.FieldAsInteger("bag_pool_num");
            }

            string tags = bag_readers[point_id].tags(grp_id, bag_pool_num, TReqInfo::Instance()->desk.lang);

            list<pair<TTripInfo, string> > seg_list;
            getSegList(segments, seg_list);

            TTrferPaxStat tmp_stat;
            TTrferPaxStatItem item;
            for(list<pair<TTripInfo, string> >::iterator flt = seg_list.begin();
                    flt != seg_list.end(); flt++) {
                if(item.airline.empty()) {
                    prn_airline.check(flt->first.airline);
                    item.airline = flt->first.airline;
                    item.airp = flt->first.airp;
                    item.flt_no1 = flt->first.flt_no;
                    item.suffix1 = flt->first.suffix;
                    item.date1 = flt->first.scd_out;
                } else {
                    item.trfer_airp = flt->first.airp;
                    item.airline2 = flt->first.airline;
                    item.flt_no2 = flt->first.flt_no;
                    item.suffix2 = flt->first.suffix;
                    item.date2 = flt->first.scd_out;
                    item.airp_arv = flt->second;
                    item.pax_name = transliter(pax.full_name(), 1, TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU);

                    item.pax_doc = CheckIn::GetPaxDocStr(std::nullopt, pax_id, false);
                    typedef map<bool, TSegCategories::Enum> TSeg2Map;
                    typedef map<bool, TSeg2Map> TCategoryMap;

                    static const TCategoryMap category_map =
                    {
                        {
                            false,
                            {
                                {false, TSegCategories::IntInt},
                                {true, TSegCategories::IntFor}
                            }
                        },
                        {
                            true,
                            {
                                {false, TSegCategories::ForInt},
                                {true, TSegCategories::ForFor}
                            }
                        }
                    };

                    string country1 = get_airp_country(item.airp);
                    string country2 = get_airp_country(item.trfer_airp);
                    string country3 = get_airp_country(item.airp_arv);

                    bool is_inter1 = country1 != country2;
                    bool is_inter2 = country2 != country3;
                    item.seg_category = category_map.at(is_inter1).at(is_inter2);

                    TSegCategories::Enum seg_category = TSegCategories::Unknown;
                    if(params.seg_category != TSegCategories::Unknown) {
                        seg_category = item.seg_category;
                    }

                    if(
                            params.seg_category == seg_category and
                            (params.trfer_airp.empty() or params.trfer_airp == item.trfer_airp) and
                            (params.trfer_airline.empty() or params.trfer_airline == item.airline)
                      )
                        tmp_stat.push_back(item);

                    item.clear();
                    item.airline = flt->first.airline;
                    item.airp = flt->first.airp;
                    item.flt_no1 = flt->first.flt_no;
                    item.suffix1 = flt->first.suffix;
                    item.date1 = flt->first.scd_out;
                }
            }

            if(tmp_stat.begin() != tmp_stat.end()) {
                tmp_stat.begin()->pax_amount = 1;
                tmp_stat.begin()->adult = pax.pers_type == adult;
                tmp_stat.begin()->child = pax.pers_type == child;
                tmp_stat.begin()->baby = pax.pers_type == baby;
                tmp_stat.begin()->rk_weight = rk_weight;
                tmp_stat.begin()->bag_amount = bag_amount;
                tmp_stat.begin()->bag_weight = bag_weight;
                tmp_stat.begin()->tags = tags;

                totals.pax_amount++;
                totals.adult += tmp_stat.begin()->adult;
                totals.child += tmp_stat.begin()->child;
                totals.baby += tmp_stat.begin()->baby;
                totals.rk_weight += rk_weight;
                totals.bag_amount += bag_amount;
                totals.bag_weight += bag_weight;
            }

            for(TTrferPaxStat::iterator i = tmp_stat.begin();
                    i != tmp_stat.end(); i++) TrferPaxStat.push_back(*i);

            params.overflow.check(TrferPaxStat.size());
        }
    }

    ArxRunTrferPaxStat(params, TrferPaxStat, prn_airline);

    if(totals.pax_amount != 0)
        TrferPaxStat.push_back(totals);
}

void createXMLTrferPaxStat(
        const TStatParams &params,
        TTrferPaxStat &TrferPaxStat,
        const TPrintAirline &prn_airline,
        xmlNodePtr resNode)
{
    if(TrferPaxStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    NewTextChild(resNode, "airline", prn_airline.get(), "");

    xmlNodePtr grdNode = NewTextChild(resNode, "grd");

    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("АК"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("АПВ"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Сег.1"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Дата"));
    SetProp(colNode, "width", 55);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("АПТ"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Сег.2"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Дата"));
    SetProp(colNode, "width", 55);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("АПП"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Категория"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("ФИО пассажира"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Документ"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("CAP.DOC.PAX"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("ВЗ"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("РБ"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("РМ"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Р/к"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("БГ мест"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("БГ вес"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Бирки"));
    SetProp(colNode, "width", 90);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;

    for(TTrferPaxStat::iterator stat = TrferPaxStat.begin();
            stat != TrferPaxStat.end(); stat++) {
        rowNode = NewTextChild(rowsNode, "row");
        if(stat->airline.empty()) {
            NewTextChild(rowNode, "col", getLocaleText("Итого:"));
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
        } else {
            // АК
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirline, stat->airline));
            // АП
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, stat->airp));
            //Сег.1
            ostringstream buf;
            buf << setw(3) << setfill('0') << stat->flt_no1 << ElemIdToCodeNative(etSuffix, stat->suffix1);
            NewTextChild(rowNode, "col", buf.str());
            //Дата
            NewTextChild(rowNode, "col", DateTimeToStr(stat->date1, "dd.mm.yy"));
            //Трансфер
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, stat->trfer_airp));
            //Сег.2
            buf.str("");
            buf
                << ElemIdToCodeNative(etAirline, stat->airline2)
                << setw(3) << setfill('0') << stat->flt_no2 << ElemIdToCodeNative(etSuffix, stat->suffix2);
            NewTextChild(rowNode, "col", buf.str());
            //Дата
            NewTextChild(rowNode, "col", DateTimeToStr(stat->date2, "dd.mm.yy"));
            //А/п прилета
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, stat->airp_arv));
            //Категория
            NewTextChild(rowNode, "col", getLocaleText(TSegCategory().encode(stat->seg_category)));
            //ФИО
            NewTextChild(rowNode, "col", stat->pax_name);
            //Паспорт
            NewTextChild(rowNode, "col", stat->pax_doc);
        }
        //Кол-во пасс.
        NewTextChild(rowNode, "col", stat->pax_amount);
        //ВЗ
        NewTextChild(rowNode, "col", stat->adult);
        //РБ
        NewTextChild(rowNode, "col", stat->child);
        //РМ
        NewTextChild(rowNode, "col", stat->baby);
        //Р/кладь(вес)
        NewTextChild(rowNode, "col", stat->rk_weight);
        //БГ мест
        NewTextChild(rowNode, "col", stat->bag_amount);
        //БГ вес
        NewTextChild(rowNode, "col", stat->bag_weight);
        //Бирки
        NewTextChild(rowNode, "col", stat->tags);
    }

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("Трансфер"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("Подробная"));
}

string segListFromDB(int tckin_id)
{
    DB::TCachedQuery Qry(
          PgOra::getROSession("TCKIN_PAX_GRP"),
          "SELECT grp_id FROM tckin_pax_grp "
          "WHERE tckin_id = :tckin_id "
          "AND tckin_pax_grp.transit_num=0 "
          "ORDER BY seg_no",
          QParams() << QParam("tckin_id", otInteger, tckin_id),
          STDLOG);
    Qry.get().Execute();
    ostringstream result;
    for(; not Qry.get().Eof; Qry.get().Next()) {
        int grp_id = Qry.get().FieldAsInteger("grp_id");

        if(not result.str().empty()) result << ";";
        TTripInfo info;
        if(info.getByGrpId(grp_id)) {
            result
                << info.airline << ","
                << info.airp << ","
                << info.flt_no << ","
                << info.suffix << ","
                << DateTimeToStr(info.scd_out) << ",";
            CheckIn::TSimplePaxGrpItem grp;
            if(grp.getByGrpId(grp_id))
                result << grp.airp_arv;
        }
    }
    return result.str();
}

void get_trfer_pax_stat(int point_id)
{
    DB::TCachedQuery delQry(
          PgOra::getRWSession("TRFER_PAX_STAT"),
          "DELETE FROM trfer_pax_stat "
          "WHERE point_id = :point_id",
          QParams() << QParam("point_id", otInteger, point_id),
          STDLOG);
    delQry.get().Execute();

    TTripInfo info;
    info.getByPointId(point_id);

    DB::TCachedQuery insQry(
          PgOra::getRWSession("TRFER_PAX_STAT"),
          "INSERT INTO trfer_pax_stat( "
          "   point_id, "
          "   scd_out, "
          "   pax_id, "
          "   rk_weight, "
          "   bag_weight, "
          "   bag_amount, "
          "   segments "
          ") VALUES( "
          "   :point_id, "
          "   :scd_out, "
          "   :pax_id, "
          "   :rk_weight, "
          "   :bag_weight, "
          "   :bag_amount, "
          "   :segments "
          ")",
          QParams()
          << QParam("point_id", otInteger, point_id)
          << QParam("scd_out", otDate, info.scd_out)
          << QParam("pax_id", otInteger)
          << QParam("rk_weight", otInteger)
          << QParam("bag_weight", otInteger)
          << QParam("bag_amount", otInteger)
          << QParam("segments", otString),
          STDLOG
          );

    DB::TCachedQuery selQry(
          PgOra::getROSession({"PAX","PAX_GRP","TCKIN_PAX_GRP"}),
          "SELECT "
          "    pax.pax_id, "
          "    tckin_pax_grp.tckin_id, "
          "    pax.grp_id, "
          "    pax.bag_pool_num "
          "FROM "
          "    pax_grp, "
          "    tckin_pax_grp, "
          "    pax "
          "WHERE "
          "    pax_grp.point_dep = :point_id AND "
          "    pax.refuse IS NULL AND "
          "    pax_grp.grp_id = tckin_pax_grp.grp_id AND "
          "    tckin_pax_grp.seg_no = 1 AND "
          "    tckin_pax_grp.transit_num = 0 AND "
          "    pax_grp.grp_id = pax.grp_id ",
          QParams() << QParam("point_id", otInteger, point_id),
          STDLOG);
    selQry.get().Execute();
    using namespace CKIN;
    BagReader bag_reader(PointId_t(point_id), std::nullopt, READ::BAGS);
    if(not selQry.get().Eof) {
        int col_pax_id = selQry.get().FieldIndex("pax_id");
        int col_tckin_id = selQry.get().FieldIndex("tckin_id");
        GrpId_t grp_id(selQry.get().FieldAsInteger("grp_id"));
        std::optional<int> bag_pool_num = std::nullopt;
        if(!selQry.get().FieldIsNULL("bag_pool_num")) {
            bag_pool_num = selQry.get().FieldAsInteger("bag_pool_num");
        }
        for(; not selQry.get().Eof; selQry.get().Next()) {

            insQry.get().SetVariable("pax_id", selQry.get().FieldAsInteger(col_pax_id));
            insQry.get().SetVariable("rk_weight", bag_reader.rkWeight(grp_id, bag_pool_num));
            insQry.get().SetVariable("bag_weight", bag_reader.bagWeight(grp_id, bag_pool_num));
            insQry.get().SetVariable("bag_amount", bag_reader.bagAmount(grp_id, bag_pool_num));
            insQry.get().SetVariable("segments", segListFromDB(selQry.get().FieldAsInteger(col_tckin_id)));

            insQry.get().Execute();
        }
    }
}

