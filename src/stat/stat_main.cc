#include "stat_main.h"
#include <boost/shared_array.hpp>
#include "stat_common.h"
#include "astra_elem_utils.h"
#include "astra_misc.h"
#include "file_queue.h"
#include "counters.h"
#include "points.h"
#include "telegram.h"
#include "stat_layout.h"
#include "stat_orders.h"
#include "stat_trfer_pax.h"
#include "stat_rfisc.h"
#include "stat_rem.h"
#include "stat_unacc.h"
#include "stat_tlg_out.h"
#include "stat_self_ckin.h"
#include "stat_annul_bt.h"
#include "stat_general.h"
#include "stat_limited_capab.h"
#include "stat_agent.h"
#include "stat_pfs.h"
#include "stat_bi.h"
#include "stat_vo.h"
#include "stat_ad.h"
#include "stat_ha.h"
#include "stat_reprint.h"
#include "stat_services.h"
#include "stat_salon.h"
#include "stat_zamar.h"
#include "baggage_ckin.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace ASTRA;
using namespace AstraLocale;
using namespace BASIC::date_time;
using namespace STAT;

void convertStatParam(xmlNodePtr paramNode)
{
    static const map<string, string> ST_to_Astra =
    {
        {"Short",       "Общая"},
        {"Detail",      "Детализированная"},
        {"Full",        "Подробная"},
        {"Total",       "Итого"},
        {"Transfer",    "Трансфер"},
        {"Pact",        "Договор"},
        {"SelfCkin",    "Саморегистрация"},
        {"Agent",       "По агентам"},
        {"Tlg",         "Отпр. телеграммы"}
    };

    string val = NodeAsString(paramNode);
    const auto idx = ST_to_Astra.find(val);
    if(idx == ST_to_Astra.end())
        throw Exception("wrong param value '%s'", val.c_str());
    NodeSetContent(paramNode, idx->second.c_str());
}

void StatInterface::Layout(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TLayout().toXML(resNode);
}

void StatInterface::stat_srv(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr curNode = reqNode->children;
    curNode = NodeAsNodeFast( "content", curNode );
    if (not curNode) return;
    curNode = curNode->children;
    curNode = NodeAsNodeFast("run_stat", curNode);
    if(not curNode)
        throw Exception("wrong format");

    TAccess access;
    access.fromXML(NodeAsNode("access", curNode));
    TAccessElems<string> norm_airlines, norm_airps;
    //нормализуем компании
    norm_airlines.set_elems_permit(access.airlines().elems_permit());
    for(set<string>::iterator i=access.airlines().elems().begin();
                              i!=access.airlines().elems().end(); ++i)
      norm_airlines.add_elem(airl_fromXML(*i, cfErrorIfEmpty, __FUNCTION__, "airline"));
    //нормализуем порты
    norm_airps.set_elems_permit(access.airps().elems_permit());
    for(set<string>::iterator i=access.airps().elems().begin();
                              i!=access.airps().elems().end(); ++i)
      norm_airps.add_elem(airp_fromXML(*i, cfErrorIfEmpty, __FUNCTION__, "airp"));

    TReqInfo &reqInfo = *(TReqInfo::Instance());
    reqInfo.user.access.merge_airlines(norm_airlines);
    reqInfo.user.access.merge_airps(norm_airps);

    xmlNodePtr statModeNode = NodeAsNode("stat_mode", curNode);
    xmlNodePtr statTypeNode = NodeAsNode("stat_type", curNode);
    if(not statModeNode or not statTypeNode)
        throw Exception("wrong param format");
    convertStatParam(statModeNode);
    convertStatParam(statTypeNode);
    RunStat(ctxt, curNode, resNode);
    xmlNodePtr formDataNode = GetNode("form_data", resNode);
    if (formDataNode!=NULL)
    {
      xmlUnlinkNode(formDataNode);
      xmlFreeNode(formDataNode);
    };
}

void orderStat(const TStatParams &params, XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TStatOrders so;
    so.get(NoExists); // list contains all orders in the system
    if(so.size() >= ORDERS_MAX_TOTAL_SIZE()) {
        NewTextChild(resNode, "collect_msg", getLocaleText("MSG.STAT_ORDERS.ORDERS_MAX_TOTAL_SIZE_EXCEEDED"));
    } else {
        so.get(); // default behaviour, orders list for current user
        if(so.is_running()) {
            NewTextChild(resNode, "collect_msg", getLocaleText("MSG.STAT_ORDERS.IS_RUNNING"));
        } else
            if(so.size() >= ORDERS_MAX_SIZE()) {
                NewTextChild(resNode, "collect_msg", getLocaleText("MSG.STAT_ORDERS.MAX_ORDERS_SIZE_EXCEEDED",
                            LParams() << LParam("max", getFileSizeStr(ORDERS_MAX_SIZE()))
                            ));
            } else {
                map<string, string> file_params;
                params.toFileParams(file_params);
                int file_id = TFileQueue::putFile(
                        OWN_POINT_ADDR(),
                        OWN_POINT_ADDR(),
                        FILE_COLLECT_TYPE,
                        file_params,
                        ""
                        );
                TStatOrder(file_id, TReqInfo::Instance()->user.user_id, osSTAT).toDB();
                NewTextChild(resNode, "collect_msg", getLocaleText("MSG.COLLECT_STAT_INFO"));
            }
    }
}

/* GRISHA */
void create_plain_files(
        const TStatParams &params,
        double &data_size,
        double &data_size_zip,
        TQueueItem &item
        )
{
    TFileParams file_params(item.params);
    // get file name
    string file_name =
        file_params.get_name() + "." +
        DateTimeToStr(item.time, "yymmddhhnn") + "." +
        DateTimeToStr(params.FirstDate, "yymm") + ".csv";

    Timing::Points timing("Timing::create_plain_files");
    timing.start(file_name, item.id);

    TPrintAirline airline;
    TOrderStatWriter order_writer(item.id, params.FirstDate, file_name);
    switch(params.statType) {
        case statTrferPax:
            RunTrferPaxStat(params, order_writer, airline);
            break;
        case statRFISC:
            RunRFISCStat(params, order_writer, airline);
            break;
        case statRem:
            RunRemStat(params, order_writer, airline);
            break;
        case statUnaccBag:
            RunUNACCFullFile(params, order_writer);
            break;
        case statTlgOutShort:
        case statTlgOutDetail:
        case statTlgOutFull:
            RunTlgOutStatFile(params, order_writer, airline);
            break;
        case statSelfCkinShort:
        case statSelfCkinDetail:
        case statSelfCkinFull:
            RunSelfCkinStatFile(params, order_writer, airline);
            break;
        case statAnnulBT:
            RunAnnulBTStatFile(params, order_writer, airline);
            break;
        case statFull:
        case statTrferFull:
            RunFullStatFile(params, order_writer, airline);
            break;
        case statLimitedCapab:
            RunLimitedCapabStatFile(params, order_writer, airline);
            break;
        case statAgentShort:
        case statAgentFull:
        case statAgentTotal:
            RunAgentStatFile(params, order_writer, airline);
            break;
        case statShort:
        case statDetail:
        case statPactShort:
            RunDetailStatFile(params, order_writer, airline);
            break;
        case statPFSFull:
            RunPFSFullFile(params, order_writer, airline);
            break;
        case statPFSShort:
            RunPFSShortFile(params, order_writer, airline);
            break;
        case statBIShort:
            RunBIShortFile(params, order_writer);
            break;
        case statBIDetail:
            RunBIDetailFile(params, order_writer);
            break;
        case statBIFull:
            RunBIFullFile(params, order_writer);
            break;
        case statVOShort:
            RunVOShortFile(params, order_writer);
            break;
        case statVOFull:
            RunVOFullFile(params, order_writer);
            break;
        case statADFull:
            RunADFullFile(params, order_writer);
            break;
        case statHAShort:
            RunHAShortFile(params, order_writer);
            break;
        case statHAFull:
            RunHAFullFile(params, order_writer);
            break;
        case statReprintShort:
            RunReprintShortFile(params, order_writer);
            break;
        case statReprintFull:
            RunReprintFullFile(params, order_writer);
            break;
        case statServicesShort:
            RunServicesShortFile(params, order_writer);
            break;
        case statServicesDetail:
            RunServicesDetailFile(params, order_writer);
            break;
        case statServicesFull:
            RunServicesFullFile(params, order_writer);
            break;
        case statSalonFull:
            RunSalonStatFile(params, order_writer);
            break;
        case statZamarFull:
            RunZamarFullFile(params, order_writer);
            break;
        default:
            throw Exception("unsupported statType %d", params.statType);
    }
    order_writer.finish();
    data_size += order_writer.data_size;
    data_size_zip += order_writer.data_size_zip;

    timing.finish(file_name, item.id);
}

void processStatOrders(TQueueItem &item, TPerfTimer &tm, int interval) {
    try {
        TCachedQuery finishQry(
                "update stat_orders set "
                "   data_size = :data_size, "
                "   data_size_zip = :data_size_zip, "
                "   time_created = :tc, "
                "   status = :status "
                "where file_id = :file_id",
                QParams()
                << QParam("data_size", otFloat)
                << QParam("data_size_zip", otFloat)
                << QParam("tc", otDate)
                << QParam("file_id", otInteger, item.id)
                << QParam("status", otInteger)
                );
        TCachedQuery progressQry("update stat_orders set progress = :progress where file_id = :file_id",
                QParams()
                << QParam("file_id", otInteger, item.id)
                << QParam("progress", otInteger)
                );

        TStatParams params(TStatOverflow::ignore);
        params.fromFileParams(item.params);

        TReqInfo::Instance()->Initialize(params.desk_city);
        TReqInfo::Instance()->desk.lang = params.desk_lang;
        TReqInfo::Instance()->user.user_id = params.user_id;

        // По client_type = ctHTTP будем определять, что статистика формируется из заказа
        // В частности в статистике Саморегистрация
        TReqInfo::Instance()->client_type = ctHTTP;

        TPeriods periods;
        periods.get(params.FirstDate, params.LastDate);

        int parts = 0;
        double data_size = 0;
        double data_size_zip = 0;
        TPeriods::TItems::iterator i = periods.items.begin();

        // Возможно, в базе уже есть данные отчета (so_data не пустой)
        // тогда цикл надо начинать с последнего собранного куска
        // Иначе говоря, перематываем на текущее состояние сборки.
        TStatOrders so;
        so.get(item.id);
        if(not so.so_data_empty(item.id)) {
            const TStatOrderData &so_data = so.items.begin()->second.so_data;
            for(
                    TStatOrderData::const_iterator i_data = so_data.begin();
                    (i_data != so_data.end() and not i_data->md5_sum.empty());
                    i_data++
               ) {
                data_size += i_data->file_size;
                data_size_zip += i_data->file_size_zip;
                // отматываем периоды
                // 1. в so_data могут быть пропуски в месяцах, если в них не было данных
                // поэтому простой i++ не получится
                // 2. Дата периода не обязана быть первым днем месяца, в то время как
                // месяц отчета всегда первый день месяца
                while(true) {
                    if(DateTimeToStr(i->first, "mm.yy") == DateTimeToStr(i_data->month, "mm.yy"))
                        break;
                    i++;
                    parts++;
                }
            }
            // После перемотки итератор периода стоит на последнем успешном месяце
            // Надо его передвинуть на новый, еще не собранный месяц
            // как и счетчик parts, чтобы прогресс отображался правильно
            i++;
            parts++;
        }

        periods.dump(i);

        for(; i != periods.items.end();
                i++,
                commit_progress(progressQry.get(), ++parts, periods.items.size(), tm.Print(), interval)
           ) {
            params.FirstDate = i->first;
            params.LastDate = i->second;

            create_plain_files(
                    params,
                    data_size,
                    data_size_zip,
                    item
                    );
        }
        finishQry.get().SetVariable("data_size", data_size);
        finishQry.get().SetVariable("data_size_zip", data_size_zip);
        finishQry.get().SetVariable("tc", NowUTC());
        finishQry.get().SetVariable("status", stReady);
        finishQry.get().Execute();
    } catch(StatOverflowException &E) {
        throw;
    } catch(Exception &E) {
        TErrCommit::Instance()->exec(item.id, stError, string(E.what()).substr(0, 250).c_str());
    } catch(...) {
        TErrCommit::Instance()->exec(item.id, stError, "unknown");
    }
}

void stat_orders_collect(int interval)
{
    TPerfTimer tm;
    TFileQueue file_queue;
    file_queue.get( TFilterQueue( OWN_POINT_ADDR(), FILE_COLLECT_TYPE ) );
    for ( TFileQueue::iterator item=file_queue.begin();
            item!=file_queue.end();
            item++, ASTRA::commit() ) {
        try {
            switch(DecodeOrderSource(item->params[PARAM_ORDER_SOURCE])) {
                case osSTAT :
                    processStatOrders(*item, tm, interval);
                    break;
                default:
                    break;
            }
            TFileQueue::deleteFile(item->id);
        }
        catch(StatOverflowException &E) {
            LogTrace(TRACE5) << "stats partially collected due to timeout expired: " << tm.Print() << " ms, to be continued next time";
        }
        catch(Exception &E) {
            ASTRA::rollback();
            //ASTRA::rollback();
            try
            {
                if (isIgnoredEOracleError(E)) continue;
                ProgError(STDLOG,"Exception: %s (file id=%d)",E.what(),item->id);
            }
            catch(...) {};
        }
        catch(...) {
            ASTRA::rollback();
            //ASTRA::rollback();
            ProgError(STDLOG, "Something goes wrong");
        }
    }
}

void StatInterface::RunStat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    if (!reqInfo->user.access.rights().permitted(600))
        throw AstraLocale::UserException("MSG.INSUFFICIENT_RIGHTS.NOT_ACCESS");

    if (reqInfo->user.access.totally_not_permitted())
        throw AstraLocale::UserException("MSG.NOT_DATA");

    TStatParams params(TStatOverflow::apply);
    params.get(reqNode);

/*
    if(
            (TReqInfo::Instance()->desk.compatible(STAT_ORDERS_VERSION) and (
                params.statType == statRFISC
                or params.statType == statRem
                or params.statType == statTlgOutFull
                )) or
            params.order
      )
        return orderStat(params, ctxt, reqNode, resNode);
*/
/*
    if (
            params.statType==statFull ||
            params.statType==statTrferFull ||
            params.statType==statSelfCkinFull ||
            params.statType==statAgentFull ||
            params.statType==statAgentShort ||
            params.statType==statAgentTotal ||
            params.statType==statTlgOutShort ||
            params.statType==statTlgOutDetail ||
            params.statType==statTlgOutFull ||
            params.statType==statRFISC ||
            params.statType==statUnaccBag
       )
    {
        if(IncMonth(params.FirstDate, 1) < params.LastDate)
            throw AstraLocale::UserException("MSG.SEARCH_PERIOD_SHOULD_NOT_EXCEED_ONE_MONTH");
    } else {
        if(IncMonth(params.FirstDate, 12) < params.LastDate)
            throw AstraLocale::UserException("MSG.SEARCH_PERIOD_SHOULD_NOT_EXCEED_ONE_YEAR");
    }
    */

    if(IncMonth(params.FirstDate, 12) < params.LastDate)
        throw AstraLocale::UserException("MSG.SEARCH_PERIOD_SHOULD_NOT_EXCEED_ONE_YEAR");

    switch(params.statType)
    {
        case statFull:
            get_compatible_report_form("FullStat", reqNode, resNode);
            break;
        case statTrferFull:
            get_compatible_report_form("TrferFullStat", reqNode, resNode);
            break;
        case statShort:
            get_compatible_report_form("ShortStat", reqNode, resNode);
            break;
        case statDetail:
            get_compatible_report_form("DetailStat", reqNode, resNode);
            break;
        case statRem:
            get_compatible_report_form("RemStat", reqNode, resNode);
            break;
        case statHAFull:
        case statHAShort:
        case statVOFull:
        case statVOShort:
        case statADFull:
        case statReprintShort:
        case statReprintFull:
        case statBIFull:
        case statBIShort:
        case statBIDetail:
        case statServicesFull:
        case statServicesShort:
        case statServicesDetail:
        case statSelfCkinFull:
        case statSelfCkinShort:
        case statSelfCkinDetail:
        case statAgentFull:
        case statAgentShort:
        case statAgentTotal:
        case statTlgOutFull:
        case statTlgOutShort:
        case statTlgOutDetail:
        case statPactShort:
        case statLimitedCapab:
        case statUnaccBag:
        case statAnnulBT:
        case statPFSFull:
        case statPFSShort:
        case statTrferPax:
        case statRFISC:
        case statSalonFull:
        case statZamarFull:
            get_compatible_report_form("stat", reqNode, resNode);
            break;
        default:
            throw Exception("unexpected stat type %d", params.statType);
    };


    try
    {
        if (params.statType==statShort || params.statType==statDetail || params.statType == statPactShort)
        {
            params.pr_pacts =
                reqInfo->user.access.rights().permitted(605) and params.seance == seanceAll;

            if(not params.pr_pacts and params.statType == statPactShort)
                throw UserException("MSG.INSUFFICIENT_RIGHTS.NOT_ACCESS");

            TDetailStat DetailStat;
            TDetailStatRow DetailStatTotal;
            TPrintAirline airline;

            if (params.pr_pacts)
                RunPactDetailStat(params, DetailStat, DetailStatTotal, airline);
            else
                RunDetailStat(params, DetailStat, DetailStatTotal, airline);

            createXMLDetailStat(params, params.pr_pacts, DetailStat, DetailStatTotal, airline, resNode);
        };
        if (params.statType==statFull || params.statType==statTrferFull)
        {
            TFullStat FullStat;
            TFullStatRow FullStatTotal;
            TPrintAirline airline;

            RunFullStat(params, FullStat, FullStatTotal, airline);

            createXMLFullStat(params, FullStat, FullStatTotal, airline, resNode);
        };
        if(
                params.statType == statSelfCkinShort or
                params.statType == statSelfCkinDetail or
                params.statType == statSelfCkinFull
          ) {
            TSelfCkinStat SelfCkinStat;
            TSelfCkinStatRow SelfCkinStatTotal;
            TPrintAirline airline;
            RunSelfCkinStat(params, SelfCkinStat, SelfCkinStatTotal, airline);
            createXMLSelfCkinStat(params, SelfCkinStat, SelfCkinStatTotal, airline, resNode);
        }
        if(
                params.statType == statAgentShort or
                params.statType == statAgentFull or
                params.statType == statAgentTotal
          ) {
            TAgentStat AgentStat;
            TAgentStatRow AgentStatTotal;
            TPrintAirline airline;
            RunAgentStat(params, AgentStat, AgentStatTotal, airline);
            createXMLAgentStat(params, AgentStat, AgentStatTotal, airline, resNode);
        }
        if(
                params.statType == statTlgOutShort or
                params.statType == statTlgOutDetail or
                params.statType == statTlgOutFull
          ) {
            TTlgOutStat TlgOutStat;
            TTlgOutStatRow TlgOutStatTotal;
            TPrintAirline airline;
            RunTlgOutStat(params, TlgOutStat, TlgOutStatTotal, airline);
            createXMLTlgOutStat(params, TlgOutStat, TlgOutStatTotal, airline, resNode);
        }
        if(params.statType == statRFISC)
        {
            TPrintAirline airline;
            TRFISCStat RFISCStat;
            RunRFISCStat(params, RFISCStat, airline);
            createXMLRFISCStat(params,RFISCStat, airline, resNode);
        }
        if(params.statType == statSalonFull)
        {
            TSalonStat SalonStat;
            RunSalonStat(params, SalonStat);
            createXMLSalonStat(params,SalonStat, resNode);
        }
        if(params.statType == statRem)
        {
            TPrintAirline airline;
            TRemStat RemStat;
            RunRemStat(params, RemStat, airline);
            createXMLRemStat(params,RemStat, airline, resNode);
        }
        if(params.statType == statLimitedCapab)
        {
            TPrintAirline airline;
            TLimitedCapabStat LimitedCapabStat;
            RunLimitedCapabStat(params, LimitedCapabStat, airline);
            createXMLLimitedCapabStat(params, LimitedCapabStat, airline, resNode);
        }
        if(params.statType == statUnaccBag)
        {
            TUNACCFullStat UNACCFullStat;
            RunUNACCStat(params, UNACCFullStat);
            createXMLUNACCFullStat(params, UNACCFullStat, resNode);
        }
        if(params.statType == statAnnulBT)
        {
            TPrintAirline airline;
            TAnnulBTStat AnnulBTStat;
            RunAnnulBTStat(params, AnnulBTStat, airline);
            createXMLAnnulBTStat(params, AnnulBTStat, airline, resNode);
        }
        if(params.statType == statPFSFull)
        {
            TPrintAirline airline;
            TPFSStat PFSStat;
            RunPFSStat(params, PFSStat, airline);
            createXMLPFSStat(params, PFSStat, airline, resNode);
        }
        if(params.statType == statPFSShort)
        {
            TPrintAirline airline;
            TPFSShortStat PFSShortStat;
            RunPFSStat(params, PFSShortStat, airline);
            createXMLPFSShortStat(params, PFSShortStat, airline, resNode);
        }
        if(params.statType == statTrferPax)
        {
            TPrintAirline airline;
            TTrferPaxStat TrferPaxStat;
            RunTrferPaxStat(params, TrferPaxStat, airline);
            createXMLTrferPaxStat(params, TrferPaxStat, airline, resNode);
        }
        if(params.statType == statBIShort)
        {
            TBIShortStat BIShortStat;
            RunBIStat(params, BIShortStat);
            createXMLBIShortStat(params, BIShortStat, resNode);
        }
        if(params.statType == statBIDetail)
        {
            TBIDetailStat BIDetailStat;
            RunBIStat(params, BIDetailStat);
            createXMLBIDetailStat(params, BIDetailStat, resNode);
        }
        if(params.statType == statBIFull)
        {
            TBIFullStat BIFullStat;
            RunBIStat(params, BIFullStat);
            createXMLBIFullStat(params, BIFullStat, resNode);
        }
        if(params.statType == statServicesShort)
        {
            TServicesShortStat ServicesShortStat;
            RunServicesStat(params, ServicesShortStat);
            createXMLServicesShortStat(params, ServicesShortStat, resNode);
        }
        if(params.statType == statServicesFull)
        {
            TServicesFullStat ServicesFullStat;
            RunServicesStat(params, ServicesFullStat);
            createXMLServicesFullStat(params, ServicesFullStat, resNode);
        }
        if(params.statType == statServicesDetail)
        {
            TServicesDetailStat ServicesDetailStat;
            RunServicesStat(params, ServicesDetailStat);
            createXMLServicesDetailStat(params, ServicesDetailStat, resNode);
        }
        if(params.statType == statReprintShort)
        {
            TReprintShortStat ReprintShortStat;
            RunReprintStat(params, ReprintShortStat);
            createXMLReprintShortStat(params, ReprintShortStat, resNode);
        }
        if(params.statType == statReprintFull)
        {
            TReprintFullStat ReprintFullStat;
            RunReprintStat(params, ReprintFullStat);
            createXMLReprintFullStat(params, ReprintFullStat, resNode);
        }
        if(params.statType == statHAShort)
        {
            THAShortStat HAShortStat;
            RunHAStat(params, HAShortStat);
            createXMLHAShortStat(params, HAShortStat, resNode);
        }
        if(params.statType == statHAFull)
        {
            THAFullStat HAFullStat;
            RunHAStat(params, HAFullStat);
            createXMLHAFullStat(params, HAFullStat, resNode);
        }
        if(params.statType == statVOShort)
        {
            TVOShortStat VOShortStat;
            RunVOStat(params, VOShortStat);
            createXMLVOShortStat(params, VOShortStat, resNode);
        }
        if(params.statType == statVOFull)
        {
            TVOFullStat VOFullStat;
            RunVOStat(params, VOFullStat);
            createXMLVOFullStat(params, VOFullStat, resNode);
        }
        if(params.statType == statADFull)
        {
            TADFullStat ADFullStat;
            RunADStat(params, ADFullStat);
            createXMLADFullStat(params, ADFullStat, resNode);
        }
        if(params.statType == statZamarFull)
        {
            TZamarFullStat ZamarFullStat;
            RunZamarStat(params, ZamarFullStat);
            createXMLZamarFullStat(params, ZamarFullStat, resNode);
        }
    }
    /* GRISHA */
    catch (StatOverflowException &E)
    {
        if(TReqInfo::Instance()->desk.compatible(STAT_ORDERS_VERSION))
        {
            LogTrace(TRACE5) << E.getLexemaData().lexema_id << " thrown. Move to orders";
            RemoveChildNodes(resNode);
            return orderStat(params, ctxt, reqNode, resNode);
        } else {
            throw;
        }
    }
    catch (EOracleError &E)
    {
        if(E.Code == 376)
            throw AstraLocale::UserException("MSG.ONE_OF_DB_FILES_UNAVAILABLE.CALL_ADMIN");
        else
            throw;
    }
}

void StatInterface::FileList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int file_id = NodeAsInteger("file_id", reqNode);
    TStatOrders so;
    so.get(file_id);
    so.items.begin()->second.check_integrity(NoExists);
    if(so.items.begin()->second.status != stReady)
        throw UserException("MSG.STAT_ORDERS.FILE_TRANSFER_ERROR",
                LParams() << LParam("status", EncodeOrderStatus(so.items.begin()->second.status)));
    if(so.so_data_empty(file_id))
        throw UserException("MSG.STAT_ORDERS.ORDER_IS_EMPTY");
    xmlNodePtr filesNode = NewTextChild(resNode, "files");
    for(TStatOrderMap::iterator curr_order_idx = so.items.begin(); curr_order_idx != so.items.end(); curr_order_idx++) {
        const TStatOrder &so = curr_order_idx->second;
        for(TStatOrderData::const_iterator so_data_idx = so.so_data.begin(); so_data_idx != so.so_data.end(); so_data_idx++) {
            xmlNodePtr itemNode = NewTextChild(filesNode, "item");
            NewTextChild(itemNode, "month", DateTimeToStr(so_data_idx->month, ServerFormatDateTimeAsString));
            NewTextChild(itemNode, "file_name", so_data_idx->file_name);
            NewTextChild(itemNode, "file_size", so_data_idx->file_size);
            NewTextChild(itemNode, "file_size_zip", so_data_idx->file_size_zip);
        }
    }
}

void StatInterface::DownloadOrder(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int file_id = NodeAsInteger("file_id", reqNode);
    TDateTime month = NodeAsDateTime("month", reqNode, NoExists);
    int pos = NodeAsInteger("pos", reqNode, 0);
    TDateTime finished_month = NodeAsDateTime("finished_month", reqNode, NoExists);
    TStatOrders so;
    if(finished_month != NoExists)
        so.get_part(file_id, finished_month)->complete();

    if(month == NoExists) return;

    so.get_part(file_id, month);

    ifstream in(get_part_file_name(file_id, month).c_str(), ios::binary);
    if(in.is_open()) {
        boost::shared_array<char> data (new char[ORDERS_BLOCK_SIZE()]);
        in.seekg(pos);
        in.read(data.get(), ORDERS_BLOCK_SIZE());

        NewTextChild(resNode, "data", StrUtils::b64_encode(data.get(), in.gcount()));
    } else
        throw UserException("file open error");
}

void StatInterface::StatOrderDel(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int file_id = NodeAsInteger("file_id", reqNode);
    TStatOrders so;
    so.get(file_id);

    if(so.items.size() != 1) return; // must be exactly one item

    TStatOrder &order = so.items.begin()->second;
    if(order.status == stRunning)
        throw UserException("MSG.STAT_ORDERS.CANT_DEL_RUNNING");
    order.del();
}

void StatInterface::StatOrders(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TPerfTimer tm;
    string source = NodeAsString("source", reqNode);
    int file_id = NodeAsInteger("file_id", reqNode, NoExists);
    TStatOrders so;
    so.get(TReqInfo::Instance()->user.user_id, file_id, source);
    so.check_integrity();
    if(file_id != NoExists) { // был запрошен ровно один отчет, для него проверка целостности
        so.items.begin()->second.check_integrity(NoExists);
    }
    so.toXML(resNode);
    LogTrace(TRACE5) << "StatOrders handler time: " << tm.PrintWithMessage();
}

void get_full_stat(TDateTime utcdate)
{
    //соберем статистику по истечении двух дней от вылета,
    //если не проставлен признак окончательного сбора статистики pr_stat

    DB::TQuery PointsQry(PgOra::getROSession({"POINTS", "TRIP_SETS"}), STDLOG);
    PointsQry.SQLText =
       "SELECT points.point_id FROM points "
        "INNER JOIN trip_sets "
           "ON points.point_id = trip_sets.point_id "
        "WHERE points.pr_del = 0 "
          "AND points.pr_reg <> 0 "
          "AND trip_sets.pr_stat = 0 "
          "AND time_out > TO_DATE('01.01.1900','DD.MM.YYYY') "
          "AND time_out < :stat_date";
    PointsQry.CreateVariable("stat_date", otDate, utcdate - 2); // 2 дня

    for (PointsQry.Execute(); !PointsQry.Eof; PointsQry.Next()) {
        get_flight_stat(PointsQry.FieldAsInteger("point_id"), true);
        ASTRA::commit();
    }
}

namespace {

struct StatKey
{
  PointId_t point_id;
  std::string airp_arv;
  int hall;
  std::string status;
  std::string client_type;

  bool operator <(const StatKey& key) const;
};

bool StatKey::operator <(const StatKey& key) const
{
  if (point_id != key.point_id)
    return point_id < key.point_id;
  if (airp_arv != key.airp_arv)
    return airp_arv < key.airp_arv;
  if (hall != key.hall)
    return hall < key.hall;
  if (status != key.status)
    return status < key.status;
  return client_type < key.client_type;
}

struct StatData
{
  int f;
  int c;
  int y;
  int adult;
  int child;
  int baby;
  int child_wop;
  int baby_wop;
  int term_bp;
  int term_bag;
  int term_ckin_service;

  StatData operator +(const StatData& data);
};

StatData StatData::operator +(const StatData& data)
{
  f += data.f;
  c += data.c;
  y += data.y;
  adult += data.adult;
  child += data.child;
  baby += data.baby;
  child_wop += data.child_wop;
  baby_wop += data.baby_wop;
  term_bp += data.term_bp;
  term_bag += data.term_bag;
  term_ckin_service += data.term_ckin_service;
  return *this;
}

struct SelfCkinStatKey
{
  PointId_t point_id;
  std::string client_type;
  std::string desk;
  std::string airp;
  std::string descr;

  bool operator <(const SelfCkinStatKey& key) const;
};

bool SelfCkinStatKey::operator <(const SelfCkinStatKey& key) const
{
  if (point_id != key.point_id)
    return point_id < key.point_id;
  if (client_type != key.client_type)
    return client_type < key.client_type;
  if (desk != key.desk)
    return desk < key.desk;
  if (airp != key.airp)
    return airp < key.airp;
  return descr < key.descr;
}

struct SelfCkinStatData
{
  int adult;
  int child;
  int baby;
  int tckin;
  int term_bp;
  int term_bag;
  int term_ckin_service;

  SelfCkinStatData operator +(const SelfCkinStatData& data);
};

SelfCkinStatData SelfCkinStatData::operator +(const SelfCkinStatData& data)
{
  adult += data.adult;
  child += data.child;
  baby += data.baby;
  tckin += data.tckin;
  term_bp += data.term_bp;
  term_bag += data.term_bag;
  term_ckin_service += data.term_ckin_service;
  return *this;
}

} // namespace

bool existsConfirmPrint(TDevOper::Enum op_type, int pax_id);

void get_stat(const PointId_t& point_id)
{
  DB::TQuery QryDel(PgOra::getRWSession("STAT"), STDLOG);
  QryDel.SQLText = "DELETE FROM stat WHERE point_id=:point_id";
  QryDel.CreateVariable("point_id", otInteger, point_id.get());
  QryDel.Execute();

  std::map<StatKey,StatData> stat_map;
  DB::TQuery Qry(PgOra::getROSession("ORACLE"), STDLOG);
  Qry.SQLText =
      "SELECT "
      "    pax.pax_id, "
      "    airp_arv, "
      "    hall, "
      "    status, "
      "    client_type, "
      "    class, "
      "    pers_type, "
      "    seats, "
      "    (SELECT 1 FROM bag2 "
      "     WHERE bag2.grp_id=pax.grp_id AND pax.bag_pool_num IS NOT NULL AND "
      "           ckin.get_bag_pool_pax_id(bag2.grp_id,bag2.bag_pool_num)=pax.pax_id AND "
      "           (bag2.is_trfer=0 OR NVL(bag2.handmade,0)<>0) AND "
      "           bag2.hall IS NOT NULL AND rownum<2) AS term_bag, "
      "    (SELECT 1 FROM events_bilingual, stations "
      "     WHERE events_bilingual.station=stations.desk AND "
      "           stations.work_mode='Р' AND "
      "           lang='RU' AND type IN (:evt_pax, :evt_pay) AND "
      "           id1=pax_grp.point_dep AND id2=pax.reg_no AND rownum<2) AS term_ckin_service "
      "FROM pax_grp,pax,tckin_pax_grp "
      "WHERE pax_grp.grp_id=pax.grp_id AND point_dep=:point_id AND pax_grp.status NOT IN ('E') and "
      "      tckin_pax_grp.grp_id(+)=pax_grp.grp_id AND "
      "      tckin_pax_grp.transit_num(+)<>0 AND "
      "      not (pax_grp.status IN ('T') AND "
      "      tckin_pax_grp.grp_id is not null) AND "
      "      pax.refuse is null ";
  Qry.CreateVariable("point_id", otInteger, point_id.get());
  Qry.CreateVariable("evt_pax", otString, EncodeEventType(evtPax));
  Qry.CreateVariable("evt_pay", otString, EncodeEventType(evtPay));
  Qry.Execute();

  for (;!Qry.Eof;Qry.Next()) {
    const int term_bp = existsConfirmPrint(TDevOper::PrnBP, Qry.FieldAsInteger("pax_id")) ? 1 : 0;
    std::string status = Qry.FieldAsString("status");
    if (status != "T") {
      status  = "N";
    }
    const TClass cls = DecodeClass(Qry.FieldAsString("class").c_str());
    const StatKey key = {
      point_id,
      Qry.FieldAsString("airp_arv"),
      Qry.FieldAsInteger("hall"),
      status,
      Qry.FieldAsString("client_type")
    };
    const int seats = Qry.FieldAsInteger("seats");
    const TPerson pers_type = DecodePerson(Qry.FieldAsString("pers_type").c_str());
    const StatData data = {
      cls == F ? 1 : 0,
      cls == C ? 1 : 0,
      cls == Y ? 1 : 0,
      pers_type == adult ? 1 : 0,
      pers_type == child ? 1 : 0,
      pers_type == baby  ? 1 : 0,
      seats ? 0 : (pers_type == child ? 1 : 0),
      seats ? 0 : (pers_type == baby ? 1 : 0),
      term_bp,
      Qry.FieldAsInteger("term_bag"),
      Qry.FieldAsInteger("term_ckin_service")
    };
    auto res = stat_map.emplace(key, data);
    const bool item_exists = !res.second;
    if (item_exists) {
      StatData& exists_data = res.first->second;
      exists_data = exists_data + data;
    }
  }

  for (const auto& item: stat_map) {
    const StatKey& key = item.first;
    const StatData& data = item.second;

    DB::TQuery QryIns(PgOra::getRWSession("STAT"), STDLOG);
    QryIns.SQLText =
        "INSERT INTO stat ( "
        "point_id,airp_arv,hall,status,client_type, "
        "f,c,y,adult,child,baby,child_wop,baby_wop, "
        "pcs,weight,unchecked,excess_wt,excess_pc, "
        "term_bp, term_bag, term_ckin_service "
        ") VALUES ("
        ":point_id,:airp_arv,:hall,:status,:client_type, "
        ":f,:c,:y,:adult,:child,:baby,:child_wop,:baby_wop, "
        ":pcs,:weight,:unchecked,:excess_wt,:excess_pc, "
        ":term_bp, :term_bag, :term_ckin_service "
        ")";
    QryIns.CreateVariable("point_id", otInteger, point_id.get());
    QryIns.CreateVariable("airp_arv", otString, key.airp_arv);
    QryIns.CreateVariable("hall", otInteger, key.hall);
    QryIns.CreateVariable("status", otString, key.status);
    QryIns.CreateVariable("client_type", otString, key.client_type);
    QryIns.CreateVariable("f", otInteger, data.f);
    QryIns.CreateVariable("c", otInteger, data.c);
    QryIns.CreateVariable("y", otInteger, data.y);
    QryIns.CreateVariable("adult", otInteger, data.adult);
    QryIns.CreateVariable("child", otInteger, data.child);
    QryIns.CreateVariable("baby", otInteger, data.baby);
    QryIns.CreateVariable("child_wop", otInteger, data.child_wop);
    QryIns.CreateVariable("baby_wop", otInteger, data.baby_wop);
    QryIns.CreateVariable("pcs", otInteger, 0);
    QryIns.CreateVariable("weight", otInteger, 0);
    QryIns.CreateVariable("unchecked", otInteger, 0);
    QryIns.CreateVariable("excess_wt", otInteger, 0);
    QryIns.CreateVariable("excess_pc", otInteger, 0);
    QryIns.CreateVariable("term_bp", otInteger, data.term_bp);
    QryIns.CreateVariable("term_bag", otInteger, data.term_bag);
    QryIns.CreateVariable("term_ckin_service", otInteger, data.term_ckin_service);
    QryIns.Execute();
  }

  DB::TQuery cur1(PgOra::getROSession("PAX_GRP-BAG2"), STDLOG);
  cur1.SQLText =
      "SELECT "
      "  airp_arv, "
      "  bag2.hall, "
      "  DECODE(status,'T','T','N') AS status, "
      "  client_type, "
      "  SUM(DECODE(pr_cabin,0,amount,0)) AS pcs, "
      "  SUM(DECODE(pr_cabin,0,weight,0)) AS weight, "
      "  SUM(DECODE(pr_cabin,1,weight,0)) AS unchecked "
      "FROM pax_grp,bag2 "
      "WHERE pax_grp.grp_id=bag2.grp_id "
      "AND pax_grp.point_dep=:point_id "
      "AND pax_grp.status NOT IN ('E') "
      "AND ckin.bag_pool_refused(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse)=0 "
      "GROUP BY airp_arv,bag2.hall,DECODE(status,'T','T','N'),client_type ";
  cur1.CreateVariable("point_id", otInteger, point_id.get());
  cur1.Execute();

  for (;!cur1.Eof;cur1.Next()) {
    if (cur1.FieldAsInteger("pcs") == 0
        && cur1.FieldAsInteger("weight") == 0
        && cur1.FieldAsInteger("unchecked") == 0)
    {
      continue;
    }
    QParams params;
    params << QParam("point_id", otInteger, point_id.get())
           << QParam("airp_arv", otString, cur1.FieldAsString("airp_arv"))
           << QParam("hall", otInteger, cur1.FieldAsInteger("hall"))
           << QParam("status", otString, cur1.FieldAsString("status"))
           << QParam("client_type", otString, cur1.FieldAsString("client_type"))
           << QParam("pcs", otInteger, cur1.FieldAsInteger("pcs"))
           << QParam("weight", otInteger, cur1.FieldAsInteger("weight"))
           << QParam("unchecked", otInteger, cur1.FieldAsInteger("unchecked"));
    DB::TCachedQuery QryUpd(
          PgOra::getRWSession("STAT"),
          "UPDATE stat "
          "SET pcs=pcs+:pcs, "
          "    weight=weight+:weight, "
          "    unchecked=unchecked+:unchecked "
          "WHERE point_id=:point_id AND "
          "      airp_arv=:airp_arv AND "
          "      (hall IS NULL AND :hall IS NULL OR hall=:hall) AND "
          "      status=:status AND "
          "      client_type=:client_type ",
          params,
          STDLOG);
    QryUpd.get().Execute();
    if (QryUpd.get().RowsProcessed() == 0) {
      DB::TCachedQuery QryIns(
            PgOra::getRWSession("STAT"),
            "INSERT INTO stat ("
            "point_id,airp_arv,hall,status,client_type, "
            "f,c,y,adult,child,baby,child_wop,baby_wop, "
            "pcs,weight,unchecked,excess_wt,excess_pc "
            ") VALUES ( "
            ":point_id,:airp_arv,:hall,:status,:client_type, "
            "0,0,0,0,0,0,0,0, "
            ":pcs,:weight,:unchecked,0,0 "
            ") ",
            params,
            STDLOG);
      QryIns.get().Execute();
    }
  }

  DB::TQuery cur2(PgOra::getROSession("PAX_GRP-BAG2"), STDLOG);
  cur2.SQLText =
      "SELECT "
      "  airp_arv, "
      "  NVL(bag2.hall,pax_grp.hall) AS hall, "
      "  DECODE(status,'T','T','N') AS status, "
      "  client_type, "
      "  SUM(DECODE(bag_refuse,0,excess_wt,0)) AS excess_wt, "
      "  SUM(DECODE(bag_refuse,0,excess_pc,0)) AS excess_pc "
      "FROM pax_grp, "
      "     (SELECT bag2.grp_id,bag2.hall "
      "      FROM bag2, "
      "           (SELECT bag2.grp_id,MAX(bag2.num) AS num "
      "            FROM pax_grp,bag2 "
      "            WHERE pax_grp.grp_id=bag2.grp_id "
      "            AND pax_grp.point_dep=:point_id "
      "            AND ckin.bag_pool_refused(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse)=0 "
      "            GROUP BY bag2.grp_id) last_bag "
      "      WHERE bag2.grp_id=last_bag.grp_id AND bag2.num=last_bag.num) bag2 "
      "WHERE pax_grp.grp_id=bag2.grp_id(+) AND point_dep=:point_id AND pax_grp.status NOT IN ('E') "
      "GROUP BY airp_arv,NVL(bag2.hall,pax_grp.hall),DECODE(status,'T','T','N'),client_type ";
  cur2.CreateVariable("point_id", otInteger, point_id.get());
  cur2.Execute();

  for (;!cur2.Eof;cur2.Next()) {
    if (cur2.FieldAsInteger("excess_wt") == 0
        && cur2.FieldAsInteger("excess_pc") == 0)
    {
      continue;
    }

    QParams params;
    params << QParam("point_id", otInteger, point_id.get())
           << QParam("airp_arv", otString, cur2.FieldAsString("airp_arv"))
           << QParam("hall", otInteger, cur2.FieldAsInteger("hall"))
           << QParam("status", otString, cur2.FieldAsString("status"))
           << QParam("client_type", otString, cur2.FieldAsString("client_type"))
           << QParam("excess_wt", otInteger, cur2.FieldAsInteger("excess_wt"))
           << QParam("excess_pc", otInteger, cur2.FieldAsInteger("excess_pc"));
    DB::TCachedQuery QryUpd(
          PgOra::getRWSession("STAT"),
          "UPDATE stat "
          "SET excess_wt=excess_wt+:excess_wt, excess_pc=excess_pc + :excess_pc "
          "WHERE point_id=:point_id AND "
          "      airp_arv=:airp_arv AND "
          "      (hall IS NULL AND :hall IS NULL OR hall=:hall) AND "
          "      status=:status AND "
          "      client_type=:client_type ",
          params,
          STDLOG);
    QryUpd.get().Execute();
    if (QryUpd.get().RowsProcessed() == 0) {
      DB::TCachedQuery QryIns(
            PgOra::getRWSession("STAT"),
            "INSERT INTO stat ( "
            "point_id,airp_arv,hall,status,client_type, "
            "f,c,y,adult,child,baby,child_wop,baby_wop, "
            "pcs,weight,unchecked,excess_wt,excess_pc "
            ") VALUES ( "
            ":point_id,:airp_arv,:hall,:status,:client_type, "
            "0,0,0,0,0,0,0,0, "
            "0,0,0,:excess_wt,:excess_pc "
            ") ",
            params,
            STDLOG);
      QryIns.get().Execute();
    }
  }
}

namespace {

struct TrferGrpData
{
  CheckIn::TSimplePaxGrpItem grp;
  std::optional<CheckIn::TSimplePaxItem> pax;
  std::string trfer_route;
  int pcs = 0;
  int weight = 0;
  int unchecked = 0;

  static std::vector<TrferGrpData> load(const PointId_t& point_id);
};

std::vector<TrferGrpData> TrferGrpData::load(const PointId_t& point_id)
{
  CKIN::BagReader bag_reader(PointId_t(point_id), std::nullopt, CKIN::READ::BAGS);
  std::map<GrpId_t,std::string> trfer_route_map;
  std::vector<TrferGrpData> result;
  DB::TCachedQuery Qry(
        PgOra::getROSession({"PAX","PAX_GRP"}),
        "SELECT "
        "  pax.*, "
        "  airp_arv, airp_dep, bag_refuse, class, class_grp, client_type, "
        "  desk, excess_pc, excess_wt, hall, inbound_confirm, piece_concept, "
        "  point_arv, point_dep, point_id_mark, pr_mark_norms, rollback_guaranteed, "
        "  status, time_create, trfer_confirm, trfer_conflict, user_id "
        "FROM pax_grp "
        "LEFT OUTER JOIN pax ON (pax_grp.grp_id = pax.grp_id AND pax.refuse IS NULL) "
        "WHERE point_dep = :point_id "
        "AND pax_grp.status NOT IN ('E') ",
        QParams() << QParam("point_id", otInteger, point_id.get()),
        STDLOG);
  Qry.get().Execute();
  for(; !Qry.get().Eof; Qry.get().Next()) {
    CheckIn::TSimplePaxGrpItem grp;
    grp.fromDB(Qry.get());
    TrferGrpData trfer_grp_data = { grp, std::nullopt, std::string() };
    if (!Qry.get().FieldIsNULL("pax_id")) {
      CheckIn::TSimplePaxItem pax;
      pax.fromDB(Qry.get());
      trfer_grp_data.pax = pax;
    }
    const GrpId_t grp_id(grp.id);
    auto pos = trfer_route_map.find(grp_id);
    if (pos == trfer_route_map.end()) {
      TTrferRoute trfer_route;
      if (!trfer_route.GetRoute(grp_id.get(), trtWithFirstSeg)) {
        continue;
      }
      const std::string trfer_route_line = trfer_route.makeLine();
      if (trfer_route_line.empty()) {
        continue;
      }
      trfer_route_map.emplace(grp_id, trfer_route_line);
      trfer_grp_data.trfer_route = trfer_route_line;
    } else {
      trfer_grp_data.trfer_route = pos->second;
    }
    trfer_grp_data.pcs = bag_reader.bagAmount(grp_id, std::nullopt);
    trfer_grp_data.weight = bag_reader.bagWeight(grp_id, std::nullopt);
    trfer_grp_data.unchecked = bag_reader.rkWeight(grp_id, std::nullopt);
    result.push_back(trfer_grp_data);
  }
  return result;
}

struct TrferStatKey
{
  std::string trfer_route;
  TClientType client_type;

  bool operator < (const TrferStatKey& key) const
  {
    if (trfer_route != key.trfer_route) {
      return trfer_route < key.trfer_route;
    }
    return client_type < key.client_type;
  }
};

struct TrferStatData
{
  int f = 0;
  int c = 0;
  int y = 0;
  int adult = 0;
  int child = 0;
  int baby = 0;
  int child_wop = 0;
  int baby_wop = 0;
  int pcs = 0;
  int weight = 0;
  int unchecked = 0;
  int excess_wt = 0;
  int excess_pc = 0;

  static void deleteByPointId(const PointId_t& point_id);
  void save(const PointId_t& point_id, const TrferStatKey& key) const;
};

void TrferStatData::deleteByPointId(const PointId_t& point_id)
{
  DB::TCachedQuery Qry(
        PgOra::getRWSession("TRFER_STAT"),
        "DELETE FROM trfer_stat "
        "WHERE point_id=:point_id",
        QParams() << QParam("point_id", otInteger, point_id.get()),
        STDLOG);
  Qry.get().Execute();
}

void TrferStatData::save(const PointId_t& point_id, const TrferStatKey& key) const
{
  DB::TCachedQuery Qry(
        PgOra::getRWSession("TRFER_STAT"),
        "INSERT INTO trfer_stat ( "
        "  point_id,trfer_route,client_type, "
        "  f,c,y,adult,child,baby,child_wop,baby_wop, "
        "  pcs,weight,unchecked,excess_wt,excess_pc "
        ") VALUES ( "
        "  :point_id,:trfer_route,:client_type, "
        "  :f,:c,:y,:adult,:child,:baby,:child_wop,:baby_wop, "
        "  :pcs,:weight,:unchecked,:excess_wt,:excess_pc "
        ")",
        QParams() << QParam("point_id", otInteger, point_id.get())
                  << QParam("trfer_route", otString, key.trfer_route)
                  << QParam("client_type", otString, EncodeClientType(key.client_type))
                  << QParam("f", otInteger, f)
                  << QParam("c", otInteger, c)
                  << QParam("y", otInteger, y)
                  << QParam("adult", otInteger, adult)
                  << QParam("child", otInteger, child)
                  << QParam("baby", otInteger, baby)
                  << QParam("child_wop", otInteger, child_wop)
                  << QParam("baby_wop", otInteger, baby_wop)
                  << QParam("pcs", otInteger, pcs)
                  << QParam("weight", otInteger, weight)
                  << QParam("unchecked", otInteger, unchecked)
                  << QParam("excess_wt", otInteger, excess_wt)
                  << QParam("excess_pc", otInteger, excess_pc),
        STDLOG);
  Qry.get().Execute();
}

} // namespace

void get_trfer_stat(const PointId_t& point_id)
{
  TrferStatData::deleteByPointId(point_id);

  std::map<TrferStatKey,TrferStatData> trfer_stat_map;
  const std::vector<TrferGrpData> trfer_items = TrferGrpData::load(point_id);
  for (const TrferGrpData& trfer_item: trfer_items) {
    const TrferStatKey key = { trfer_item.trfer_route, trfer_item.grp.client_type };
    auto res = trfer_stat_map.emplace(key, TrferStatData{});
    TrferStatData& data = res.first->second;
    if (trfer_item.pax || (!trfer_item.pax && trfer_item.grp.cl.empty())) {
      data.f += (trfer_item.grp.cl == EncodeClass(ASTRA::F) ? 1 : 0);
      data.c += (trfer_item.grp.cl == EncodeClass(ASTRA::C) ? 1 : 0);
      data.y += (trfer_item.grp.cl == EncodeClass(ASTRA::Y) ? 1 : 0);
      if (trfer_item.pax) {
        const CheckIn::TSimplePaxItem& pax = *trfer_item.pax;
        data.adult += (pax.pers_type == ASTRA::adult ? 1 : 0);
        data.child += (pax.pers_type == ASTRA::child ? 1 : 0);
        data.baby  += (pax.pers_type == ASTRA::baby  ? 1 : 0);
        data.child_wop += (pax.seats == 0 && pax.pers_type == ASTRA::child ? 1 : 0);
        data.baby_wop  += (pax.seats == 0 && pax.pers_type == ASTRA::baby  ? 1 : 0);
      }
    }
    if (trfer_item.grp.bag_refuse.empty()) {
      data.excess_pc += (trfer_item.grp.excess_pc != ASTRA::NoExists ? trfer_item.grp.excess_pc : 0);
      data.excess_wt += (trfer_item.grp.excess_wt != ASTRA::NoExists ? trfer_item.grp.excess_wt : 0);
    }
    data.pcs += trfer_item.pcs;
    data.weight += trfer_item.weight;
    data.unchecked += trfer_item.unchecked;
  }

  for (const auto& trfer_item: trfer_stat_map) {
    const TrferStatKey& key = trfer_item.first;
    const TrferStatData& data = trfer_item.second;
    data.save(point_id, key);
  }
}

void get_self_ckin_stat(const PointId_t& point_id)
{
  LogTrace(TRACE6) << __func__ << ": point_id=" << point_id;
  DB::TQuery QryDel(PgOra::getRWSession("SELF_CKIN_STAT"), STDLOG);
  QryDel.SQLText = "DELETE FROM self_ckin_stat "
                   "WHERE point_id = :point_id ";
  QryDel.CreateVariable("point_id", otInteger, point_id.get());
  QryDel.Execute();

  std::map<SelfCkinStatKey,SelfCkinStatData> self_ckin_stat_map;
  DB::TQuery Qry(PgOra::getRWSession("ORACLE"), STDLOG);
  Qry.SQLText =
      "SELECT "
      "  pax.pax_id, "
      "  point_dep, "
      "  web_clients.client_type, "
      "  web_clients.desk, "
      "  desk_grp.airp, "
      "  web_clients.descr, "
      "  pax.pers_type, "
      "  tckin_pax_grp.grp_id AS tckin_grp_id, "
      "  (SELECT 1 FROM bag2 "
      "   WHERE bag2.grp_id=pax.grp_id AND pax.bag_pool_num IS NOT NULL AND "
      "         ckin.get_bag_pool_pax_id(bag2.grp_id,bag2.bag_pool_num)=pax.pax_id AND "
      "         (bag2.is_trfer=0 OR NVL(bag2.handmade,0)<>0) AND "
      "         bag2.hall IS NOT NULL AND rownum<2) AS term_bag, "
      "  (SELECT 1 FROM events_bilingual, stations "
      "   WHERE events_bilingual.station=stations.desk AND "
      "         stations.work_mode='Р' AND "
      "         lang='RU' AND type IN (:evt_pax, :evt_pay) AND "
      "         id1=pax_grp.point_dep AND id2=pax.reg_no AND rownum<2) AS term_ckin_service "
      "FROM "
      "  pax_grp, "
      "  pax, "
      "  tckin_pax_grp, "
      "  web_clients, "
      "  desks, "
      "  desk_grp "
      "WHERE "
      "  pax_grp.point_dep = :point_id and "
      "  pax_grp.status NOT IN ('E') and "
      "  pax_grp.grp_id = pax.grp_id and "
      "  pax_grp.user_id = web_clients.user_id and "
      "  web_clients.client_type in ('KIOSK', 'WEB', 'MOBIL') and "
      "  web_clients.desk = desks.code and "
      "  desks.grp_id = desk_grp.grp_id and "
      "  pax_grp.grp_id = tckin_pax_grp.grp_id(+) and "
      "  tckin_pax_grp.seg_no(+) = 1 and "
      "  tckin_pax_grp.transit_num(+) = 0 ";
  Qry.CreateVariable("point_id", otInteger, point_id.get());
  Qry.CreateVariable("evt_pax", otString, EncodeEventType(evtPax));
  Qry.CreateVariable("evt_pay", otString, EncodeEventType(evtPay));
  Qry.Execute();
  for (;!Qry.Eof;Qry.Next()) {
    const int term_bp = existsConfirmPrint(TDevOper::PrnBP, Qry.FieldAsInteger("pax_id")) ? 1 : 0;

    const SelfCkinStatKey key = {
      point_id,
      Qry.FieldAsString("client_type"),
      Qry.FieldAsString("desk"),
      Qry.FieldAsString("airp"),
      Qry.FieldAsString("descr")
    };
    const TPerson pers_type = DecodePerson(Qry.FieldAsString("pers_type").c_str());
    const SelfCkinStatData data = {
      pers_type == adult ? 1 : 0,
      pers_type == child ? 1 : 0,
      pers_type == baby  ? 1 : 0,
      !Qry.FieldIsNULL("tckin_grp_id") ? 1 : 0,
      term_bp,
      Qry.FieldAsInteger("term_bag"),
      Qry.FieldAsInteger("term_ckin_service")
    };
    auto res = self_ckin_stat_map.emplace(key, data);
    const bool item_exists = !res.second;
    if (item_exists) {
      SelfCkinStatData& exists_data = res.first->second;
      exists_data = exists_data + data;
    }
  }

  for (const auto& item: self_ckin_stat_map) {
    const SelfCkinStatKey& key = item.first;
    const SelfCkinStatData& data = item.second;

    DB::TQuery QryIns(PgOra::getRWSession("SELF_CKIN_STAT"), STDLOG);
    QryIns.SQLText =
        "INSERT INTO self_ckin_stat ( "
        "  point_id, "
        "  client_type, "
        "  desk, "
        "  desk_airp, "
        "  descr, "
        "  adult, "
        "  child, "
        "  baby, "
        "  tckin, "
        "  term_bp, "
        "  term_bag, "
        "  term_ckin_service "
        ") VALUES ( "
        "  :point_id, "
        "  :client_type, "
        "  :desk, "
        "  :desk_airp, "
        "  :descr, "
        "  :adult, "
        "  :child, "
        "  :baby, "
        "  :tckin, "
        "  :term_bp, "
        "  :term_bag, "
        "  :term_ckin_service "
        ")";
    QryIns.CreateVariable("point_id", otInteger, point_id.get());
    QryIns.CreateVariable("client_type", otString, key.client_type);
    QryIns.CreateVariable("desk", otString, key.desk);
    QryIns.CreateVariable("desk_airp", otString, key.airp);
    QryIns.CreateVariable("descr", otString, key.descr);
    QryIns.CreateVariable("adult", otInteger, data.adult);
    QryIns.CreateVariable("child", otInteger, data.child);
    QryIns.CreateVariable("baby", otInteger, data.baby);
    QryIns.CreateVariable("tckin", otInteger, data.tckin);
    QryIns.CreateVariable("term_bp", otInteger, data.term_bp);
    QryIns.CreateVariable("term_bag", otInteger, data.term_bag);
    QryIns.CreateVariable("term_ckin_service", otInteger, data.term_ckin_service);
    QryIns.Execute();
  }
}

void get_flight_stat(int point_id, bool final_collection)
{
   LogTrace5 << " point_id: " << point_id;
   Timing::Points timing("Timing::get_flight_stat");
   timing.start("get_flight_stat", point_id);

   TFlights flightsForLock;
   flightsForLock.Get( point_id, ftTranzit );
   flightsForLock.Lock(__FUNCTION__);
   {
     QParams QryParams;
     QryParams << QParam("point_id", otInteger, point_id);
     QryParams << QParam("final_collection", otInteger, (int)final_collection);

     DB::TCachedQuery Qry(PgOra::getRWSession("TRIP_SETS"),
        "UPDATE trip_sets SET pr_stat = :final_collection "
         "WHERE point_id = :point_id AND pr_stat = 0", QryParams, STDLOG);
     Qry.get().Execute();
     if (Qry.get().RowsProcessed()<=0) return; //статистику не собираем
   };
   {
     timing.start("statist");
     get_stat(PointId_t(point_id));
     get_trfer_stat(PointId_t(point_id));
     get_self_ckin_stat(PointId_t(point_id));
     timing.finish("statist");
     typedef void  (*TStatFunction)(int);
     static const map<string, TStatFunction> m =
     {
         {"rfisc_stat",             get_rfisc_stat},
         {"rem_stat",               get_rem_stat},
         {"limited_capability_stat",get_limited_capability_stat},
         {"kuf_stat",               get_kuf_stat},
         {"pfs_stat",               get_pfs_stat},
         {"trfer_pax_stat",         get_trfer_pax_stat},
         {"stat_vo",                get_stat_vo},
         {"stat_ha",                get_stat_ha},
         {"stat_ad",                get_stat_ad},
         {"stat_reprint",           get_stat_reprint},
         {"stat_services",          get_stat_services}
     };
     for(const auto &i: m) {
         LogTrace(TRACE5) << i.first;
         timing.start(i.first);
         (*i.second)(point_id);
         timing.finish(i.first);
     }
   };

   TReqInfo::Instance()->LocaleToLog("EVT.COLLECT_STATISTIC", evtFlt, point_id);

   timing.finish("get_flight_stat", point_id);
};

void collectStatTask(const TTripTaskKey &task)
{
  LogTrace(TRACE5) << __FUNCTION__ << ": " << task;

  time_t time_start=time(NULL);
  try
  {
    get_flight_stat(task.point_id, false);
  }
  catch(const EOracleError &E) {
    E.showProgError();
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"%s (point_id=%d): %s", __FUNCTION__, task.point_id, E.what());
  };
  time_t time_end=time(NULL);
  if (time_end-time_start>1)
    ProgTrace(TRACE5,"Attention! %s execute time: %ld secs, point_id=%d",
                     __FUNCTION__, time_end-time_start, task.point_id);
}
