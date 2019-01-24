#include "self_ckin_log.h"
#include "xml_unit.h"
#include "astra_consts.h"
#include "qrys.h"
#include "date_time.h"
#include "astra_utils.h"
#include "report_common.h"
#include "stat_utils.h"
#include "docs.h"
#include "cr_lf.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace ASTRA;
using namespace std;
using namespace BASIC::date_time;
using namespace AstraLocale;

class TKioskEventTypes {
    public:
        enum Enum {
            postScreen,
            getScreen,
            statusOperation,
            statusDevices,
            Unknown
        };

        static const std::list< std::pair<Enum, std::string> >& pairsCodes()
        {
            static std::list< std::pair<Enum, std::string> > l;
            if (l.empty())
            {
                l.push_back(std::make_pair(postScreen,      "postScreen"));
                l.push_back(std::make_pair(getScreen,       "getScreen"));
                l.push_back(std::make_pair(statusOperation, "statusOperation"));
                l.push_back(std::make_pair(statusDevices,   "statusDevices"));
                l.push_back(std::make_pair(Unknown, ""));
            }
            return l;
        }
};

class TKioskEventTypesCode: public ASTRA::PairList<TKioskEventTypes::Enum, std::string>
{
    private:
        virtual std::string className() const { return "TKioskEventTypesCode"; }
    public:
        TKioskEventTypesCode() : ASTRA::PairList<TKioskEventTypes::Enum, std::string>(TKioskEventTypes::pairsCodes(),
                boost::none,
                boost::none) {}
};



namespace KIOSK_PARAM_NAME {
    const string REFERENCE = "__REFERENCE__";
    const string ERROR = "__ERROR__";
    const string FLIGHT = "flight";
    const string DATE = "selectedDate";
    const string LAST_NAME = "lastName";
    const string TICKET = "ticket";
    const string DOC = "document";
    const string SESSION = "sessionId";
}

struct TKiosksGrp {
    int kiosk_addr;
    vector<string> items;
    void fromDB(int _kiosk_addr);
    TKiosksGrp() { clear(); }
    void clear()
    {
        kiosk_addr = NoExists;
        items.clear();
    }
    bool no_data() const
    {
        return kiosk_addr != NoExists and items.empty();
    }
};

struct TParams {
    string sessionId;
    TDateTime time_from;
    TDateTime time_to;
    string app;

    string flt;
    string date;
    string pax_name;
    string tkt;
    string doc;

    string kiosk;

    TKiosksGrp kiosks_grp;

    bool dev_log;
    bool screen_log;

    TParams() { clear(); }
    void clear();
    void fromXML(xmlNodePtr reqNode);
    bool input_screen() const;
};

void TKiosksGrp::fromDB(int _kiosk_addr)
{
    clear();
    kiosk_addr = _kiosk_addr;
    if(kiosk_addr == NoExists) return;
    TCachedQuery Qry("select kiosk_id from web_clients where kiosk_addr = :kiosk_addr",
            QParams() << QParam("kiosk_addr", otInteger, kiosk_addr));
    Qry.get().Execute();
    for(; not Qry.get().Eof; Qry.get().Next())
        items.push_back(Qry.get().FieldAsString("kiosk_id"));
}

bool TParams::input_screen() const
{
    return not (
        flt.empty() and
        date.empty() and
        pax_name.empty() and
        tkt.empty() and
        doc.empty());
}

void TParams::fromXML(xmlNodePtr reqNode)
{
    clear();
    sessionId = NodeAsString("sessionId", reqNode, "");
    if(sessionId.empty()) {
        time_from = NodeAsDateTime("time_from", reqNode);
        time_to = NodeAsDateTime("time_to", reqNode);
        kiosks_grp.fromDB(NodeAsInteger("kiosk_addr", reqNode, NoExists));
        app = NodeAsString("app", reqNode, "");

        kiosk = NodeAsString("kiosk", reqNode, "");
        dev_log = NodeAsInteger("dev_log", reqNode, 0) != 0;
        screen_log = NodeAsInteger("screen_log", reqNode, 0) != 0;

        if(not dev_log) {
            flt = NodeAsString("flt", reqNode, "");
            date = NodeAsString("date", reqNode, "");
            pax_name = NodeAsString("pax_name", reqNode, "");
            tkt = NodeAsString("tkt", reqNode, "");
            doc = NodeAsString("doc", reqNode, "");
        }
    }
}

void TParams::clear()
{
    sessionId.clear();
    time_from = NoExists;
    time_to = NoExists;
    app.clear();
    flt.clear();
    pax_name.clear();
    tkt.clear();
    doc.clear();
    kiosk.clear();
    kiosks_grp.clear();

    dev_log = false;
    screen_log = false;
}

struct TKioskEventParams {
    // items[kep.num, kep.name] = value - поля таблицы kiosk_event_params (сокр. kep)
    typedef map<string, string> TItemNameMap;
    typedef map<int, TItemNameMap> TItemsMap;
    TItemsMap items;
    void fromDB(int event_id);
    TItemsMap get_param(const string &name) const;
    string get_param_value(const string &name) const;
};

string TKioskEventParams::get_param_value(const string &name) const
{
    string result;
    for(TItemsMap::const_iterator num = items.begin();
            num != items.end(); num++) {
        TItemNameMap::const_iterator i_name = num->second.find(name);
        if(i_name != num->second.end()) {
            result = i_name->second;
            break;
        }
    }
    return result;
}

TKioskEventParams::TItemsMap TKioskEventParams::get_param(const string &name) const
{
    TItemsMap result;
    for(TItemsMap::const_iterator idx = items.begin();
            idx != items.end(); idx++) {
        for(TItemNameMap::const_iterator name_idx = idx->second.begin();
                name_idx != idx->second.end(); name_idx++) {
            if(name_idx->first == name) {
                result[idx->first][name_idx->first] = name_idx->second;
            }
        }
    }
    return result;
}

void TKioskEventParams::fromDB(int event_id)
{
    TCachedQuery Qry("select * from kiosk_event_params where event_id = :event_id",
            QParams() << QParam("event_id", otInteger, event_id));
    Qry.get().Execute();

    typedef map<int, string> TPageMap; // <page_no, value>
    typedef map<string, TPageMap> TNameMap;
    typedef map<int, TNameMap> TNumMap;

    TNumMap rows;
    if(not Qry.get().Eof) {
        int col_num = Qry.get().FieldIndex("num");
        int col_name = Qry.get().FieldIndex("name");
        int col_value = Qry.get().FieldIndex("value");
        int col_page_no = Qry.get().FieldIndex("page_no");
        for(; not Qry.get().Eof; Qry.get().Next()) {
            rows
                [Qry.get().FieldAsInteger(col_num)]
                [Qry.get().FieldAsString(col_name)]
                [Qry.get().FieldAsInteger(col_page_no)]
                =
                Qry.get().FieldAsString(col_value);
        }
    }

    for(TNumMap::iterator num_idx = rows.begin();
            num_idx != rows.end(); num_idx++) {
        for(TNameMap::iterator name_idx = num_idx->second.begin();
                name_idx != num_idx->second.end(); name_idx++) {
            string value;
            for(TPageMap::iterator page_idx = name_idx->second.begin();
                    page_idx != name_idx->second.end(); page_idx++) {
                value += page_idx->second;
            }
            items[num_idx->first][name_idx->first] = value;
        }
    }
}

struct TSelfCkinLogItem {
    int id;
    TKioskEventTypes::Enum type;
    string app;
    string screen;
    string kiosk_id;
    TDateTime time;
    int ev_order;
    TKioskEventParams evt_params;

    TSelfCkinLogItem()
    {
        clear();
    }

    void clear()
    {
        id = NoExists;
        type = TKioskEventTypes::Unknown;
        app.clear();
        screen.clear();
        kiosk_id.clear();
        time = NoExists;
        ev_order = NoExists;
    }
};

struct TSelfCkinLog {
    typedef map<int, TSelfCkinLogItem> TEVMap;
    typedef map<TDateTime, TEVMap> TTimeMap;
    typedef map<int, TTimeMap> TItemsMap;

    TItemsMap items;
    void fromDB(const TParams &params);
    void fromDB(const TParams &params, TCachedQuery &Qry);
    void toXML(xmlNodePtr resNode);
    void rowToXML(xmlNodePtr rowNodek, const TSelfCkinLogItem &log_item, const string &err = "");
    bool found(const string &value, const string &param);
};

bool TSelfCkinLog::found(const string &value, const string &param)
{
    return param.empty() or value.find(param) != string::npos;
}

void TSelfCkinLog::rowToXML(xmlNodePtr rowNode, const TSelfCkinLogItem &log_item, const string &err)
{
    SetProp(rowNode, "id", log_item.id);
    // Время
    NewTextChild(rowNode, "col", DateTimeToStr(log_item.time));
    // Киоск
    NewTextChild(rowNode, "col", log_item.kiosk_id);
    // Ошибка
    NewTextChild(rowNode, "col", err);
    // Экран
    NewTextChild(rowNode, "col", place_CR_LF(log_item.evt_params.get_param(KIOSK_PARAM_NAME::REFERENCE).begin()->second.begin()->second));
    // Приложение
    NewTextChild(rowNode, "col", log_item.app);
    // Тип
    NewTextChild(rowNode, "col", TKioskEventTypesCode().encode(log_item.type));
    // №
    NewTextChild(rowNode, "col", log_item.ev_order);
    // Сессия
    NewTextChild(rowNode, "col", log_item.evt_params.get_param_value(KIOSK_PARAM_NAME::SESSION));
}

void TSelfCkinLog::toXML(xmlNodePtr resNode)
{
    NewTextChild(resNode, "screenCol", 3);
    NewTextChild(resNode, "sessionCol", 7);
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");

    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("Время"));
    SetProp(colNode, "width", 110);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Киоск"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Ошибка"));
    SetProp(colNode, "width", 200);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Экран"));
    SetProp(colNode, "width", 400);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Приложение"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Тип"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("№"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Сессия"));
    SetProp(colNode, "width", 210);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;

    for(TItemsMap::iterator id = items.begin();
            id != items.end(); id++) {
        for(TTimeMap::iterator time_idx = id->second.begin();
                time_idx != id->second.end(); time_idx++) {
            for(TEVMap::iterator ev_idx = time_idx->second.begin();
                    ev_idx != time_idx->second.end(); ev_idx++) {
                rowNode = NewTextChild(rowsNode, "row");
                TKioskEventParams::TItemsMap errors = ev_idx->second.evt_params.get_param(KIOSK_PARAM_NAME::ERROR);
                if(errors.empty())
                    rowToXML(rowNode, ev_idx->second);
                else
                    for(TKioskEventParams::TItemsMap::iterator idx = errors.begin();
                            idx != errors.end(); idx++) {
                        for(TKioskEventParams::TItemNameMap::iterator name_idx = idx->second.begin();
                                name_idx != idx->second.end(); name_idx++)
                        {
                            rowToXML(rowNode, ev_idx->second, name_idx->second);
                        }
                    }
            }
        }
    }
    LogTrace(TRACE5) << GetXMLDocText(resNode->doc);
}

void TSelfCkinLog::fromDB(const TParams &params, TCachedQuery &Qry)
{
    Qry.get().Execute();
    if(not Qry.get().Eof) {
        int col_id = Qry.get().FieldIndex("id");
        int col_type = Qry.get().FieldIndex("type");
        int col_app = Qry.get().FieldIndex("application");
        int col_screen = Qry.get().FieldIndex("screen");
        int col_kiosk_id = Qry.get().FieldIndex("kioskid");
        int col_time = Qry.get().FieldIndex("time");
        int col_ev_order = Qry.get().FieldIndex("ev_order");
        for(; not Qry.get().Eof; Qry.get().Next()) {
            TSelfCkinLogItem logItem;
            logItem.id = Qry.get().FieldAsInteger(col_id);
            logItem.type = TKioskEventTypesCode().decode(Qry.get().FieldAsString(col_type));
            logItem.app = Qry.get().FieldAsString(col_app);
            logItem.screen = Qry.get().FieldAsString(col_screen);
            logItem.kiosk_id = Qry.get().FieldAsString(col_kiosk_id);
            logItem.time = Qry.get().FieldAsDateTime(col_time);
            logItem.ev_order = Qry.get().FieldAsInteger(col_ev_order);
            logItem.evt_params.fromDB(logItem.id);

            if(
                    not params.sessionId.empty() or
                    logItem.type == TKioskEventTypes::statusOperation or
                    logItem.type == TKioskEventTypes::statusDevices or
                    not params.input_screen() or
                    (
                     logItem.screen == "ParamsInputScreen" and
                     logItem.type == TKioskEventTypes::postScreen and
                     (
                      (params.flt.empty() or
                       logItem.evt_params.get_param_value(KIOSK_PARAM_NAME::FLIGHT) == params.flt) and
                      (params.date.empty() or
                       logItem.evt_params.get_param_value(KIOSK_PARAM_NAME::DATE) == params.date) and
                      (params.pax_name.empty() or
                       upperc(logItem.evt_params.get_param_value(KIOSK_PARAM_NAME::LAST_NAME)).find(params.pax_name) != string::npos) and
                      (params.tkt.empty() or
                       upperc(logItem.evt_params.get_param_value(KIOSK_PARAM_NAME::TICKET)).find(params.tkt) != string::npos) and
                      (params.doc.empty() or
                       upperc(logItem.evt_params.get_param_value(KIOSK_PARAM_NAME::DOC)).find(params.doc) != string::npos)
                     )
                    )
                        )
                        items[logItem.id][logItem.time][logItem.ev_order] = logItem;

            /*
               const string &value = upperc(logItem.evt_params.get_param(KIOSK_PARAM_NAME::REFERENCE).begin()->second.begin()->second);
               if(
               found(value, params.flt) and
               found(value, params.pax_name) and
               found(value, params.tkt) and
               found(value, params.doc)
               )
               items[logItem.time][logItem.ev_order] = logItem;
               */
        }
    }
}

void TSelfCkinLog::fromDB(const TParams &params)
{
    if(params.kiosks_grp.no_data()) return;
    QParams QryParams;
    string SQLText = "select * from kiosk_events where kioskid is not null and ";
    if(params.sessionId.empty()) {
        SQLText += "time > :time_from and time <= :time_to";
        QryParams
            << QParam("time_from", otDate, params.time_from)
            << QParam("time_to", otDate, params.time_to);
        /*
        if(params.input_screen()) {
            SQLText +=
                " and screen = 'ParamsInputScreen' "
                " and type = :postScreen ";
            QryParams << QParam("postScreen", otString, TKioskEventTypesCode().encode(TKioskEventTypes::postScreen));
        }
        */
        if(params.kiosks_grp.kiosk_addr != NoExists)
            SQLText += " and kioskid in " + GetSQLEnum(params.kiosks_grp.items);
        else if(not params.kiosk.empty()) {
            SQLText += " and kioskid = :kiosk_id ";
            QryParams << QParam("kiosk_id", otString, params.kiosk);
        }
        if(not params.app.empty()) {
            SQLText += " and application = :app ";
            QryParams << QParam("app", otString, params.app);
        }
        if(params.dev_log or params.screen_log) {
            QryParams
                << QParam("statusDevices", otString, TKioskEventTypesCode().encode(TKioskEventTypes::statusDevices))
                << QParam("statusOperation", otString, TKioskEventTypesCode().encode(TKioskEventTypes::statusOperation));
            if(params.dev_log)
                SQLText += " and type in(:statusDevices, :statusOperation) ";
            else if(params.screen_log)
                SQLText += " and type not in(:statusDevices, :statusOperation) ";
        }
    } else {
        SQLText += " session_id = :sessionId ";
        QryParams << QParam("sessionId", otString, params.sessionId);
    }
    TCachedQuery Qry(SQLText, QryParams);
    fromDB(params, Qry);
    if(not params.sessionId.empty() and not items.empty()) {
        TCachedQuery devQry(
                "select * from kiosk_events where "
                "   kioskid = :kiosk_id and "
                "   type in(:statusDevices, :statusOperation) and "
                "   time > :time_from - 5/1440 and time <= :time_to + 5/1440 ",
                QParams()
                << QParam("kiosk_id", otString, items.begin()->second.begin()->second.begin()->second.kiosk_id)
                << QParam("time_from", otDate, items.begin()->second.begin()->first)
                << QParam("time_to", otDate, items.rbegin()->second.begin()->first)
                << QParam("statusDevices", otString, TKioskEventTypesCode().encode(TKioskEventTypes::statusDevices))
                << QParam("statusOperation", otString, TKioskEventTypesCode().encode(TKioskEventTypes::statusOperation)));

        fromDB(params, devQry);
    }
}

void SelfCkinLogInterface::Run(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    get_compatible_report_form("SelfCkinLog", reqNode, resNode);
    xmlNodePtr formDataNode = STAT::set_variables(resNode);

    TParams params;
    params.fromXML(reqNode);
    TSelfCkinLog log;
    log.fromDB(params);
    if(log.items.empty())
        throw UserException("MSG.NOT_DATA");
    log.toXML(resNode);
}
