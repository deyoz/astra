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
            item++, OraSession.Commit() ) {
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
            OraSession.Rollback();
            try
            {
                if (isIgnoredEOracleError(E)) continue;
                ProgError(STDLOG,"Exception: %s (file id=%d)",E.what(),item->id);
            }
            catch(...) {};
        }
        catch(...) {
            OraSession.Rollback();
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

    TQuery PointsQry(&OraSession);
  PointsQry.Clear();
  PointsQry.SQLText =
    "SELECT points.point_id FROM points,trip_sets "
    "WHERE points.point_id=trip_sets.point_id AND "
    "      points.pr_del=0 AND points.pr_reg<>0 AND trip_sets.pr_stat=0 AND "
    "      time_out<:stat_date AND time_out>TO_DATE('01.01.0001','DD.MM.YYYY')";
  PointsQry.CreateVariable("stat_date",otDate,utcdate-2); //2 дня
  PointsQry.Execute();
  for(;!PointsQry.Eof;PointsQry.Next())
  {
    get_flight_stat(PointsQry.FieldAsInteger("point_id"), true);
    OraSession.Commit();
  };
};


void get_flight_stat(int point_id, bool final_collection)
{
   Timing::Points timing("Timing::get_flight_stat");
   timing.start("get_flight_stat", point_id);

   TFlights flightsForLock;
   flightsForLock.Get( point_id, ftTranzit );
   flightsForLock.Lock(__FUNCTION__);

   {
     QParams QryParams;
     QryParams << QParam("point_id", otInteger, point_id);
     QryParams << QParam("final_collection", otInteger, (int)final_collection);
     TCachedQuery Qry("UPDATE trip_sets SET pr_stat=:final_collection WHERE point_id=:point_id AND pr_stat=0", QryParams);
     Qry.get().Execute();
     if (Qry.get().RowsProcessed()<=0) return; //статистику не собираем
   };
   {
     QParams QryParams;
     QryParams << QParam("point_id", otInteger, point_id);
     TCachedQuery Qry("BEGIN "
                      "  statist.get_stat(:point_id); "
                      "  statist.get_trfer_stat(:point_id); "
                      "  statist.get_self_ckin_stat(:point_id); "
                      "END;",
                      QryParams);

     timing.start("statist");
     Qry.get().Execute();
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
    ProgError(STDLOG,"EOracleError %d: %s",E.Code,E.what());
    ProgError(STDLOG,"SQL: %s",E.SQLText());
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

