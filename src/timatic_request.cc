#include "timatic_request.h"
#include "timatic_response.h"
#include "xml_unit.h"
#include "astra_consts.h"
#include "astra_elems.h"
#include "astra_locale_adv.h"
#include "serverlib/str_utils.h"
#include "html_pages.h"
#include "serverlib/xml_stuff.h"
#include "astra_elem_utils.h"
#include "astra_misc.h"
#include "flt_settings.h"
#include "passenger.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace ASTRA;
using namespace BASIC::date_time;

namespace Timatic {

// коммент

void fill_container(xmlNodePtr dataNode, const string &name, const auto &container)
{
    xmlNodePtr curNode = dataNode->children;
    if(not GetNodeFast(name.c_str(), curNode)) {
        xmlNodePtr contNode = NewTextChild(dataNode, name.c_str());
        xmlNodePtr CBoxItemNode = NewTextChild(contNode, "item");
        NewTextChild(CBoxItemNode, "code");
        NewTextChild(CBoxItemNode, "caption");
        for(const auto &i: container) {
            CBoxItemNode = NewTextChild(contNode, "item");
            NewTextChild(CBoxItemNode, "code", i.second.name);
            NewTextChild(CBoxItemNode, "caption", i.second.displayName);
        }
    }
}

class TCtrlType {
    public:
        enum Enum {
            label,
            cbox,
            edit,
            date,
            list,
            dateTime,
            page,
            group,
            panel,
            Unknown
        };
        static const std::list< std::pair<Enum, std::string> >& pairs()
        {
            static std::list< std::pair<Enum, std::string> > l;
            if (l.empty())
            {
                l.push_back(std::make_pair(label,       "label"));
                l.push_back(std::make_pair(cbox,        "cbox"));
                l.push_back(std::make_pair(edit,        "edit"));
                l.push_back(std::make_pair(date,        "date"));
                l.push_back(std::make_pair(list,        "list"));
                l.push_back(std::make_pair(dateTime,    "dateTime"));
                l.push_back(std::make_pair(page,        "page"));
                l.push_back(std::make_pair(group,       "group"));
                l.push_back(std::make_pair(panel,       "panel"));
            }
            return l;
        }
};

class TCtrlTypes: public ASTRA::PairList<TCtrlType::Enum, std::string>
{
    private:
        virtual std::string className() const { return "TCtrlTypes"; }
    public:
        TCtrlTypes() : ASTRA::PairList<TCtrlType::Enum, std::string>(TCtrlType::pairs(),
                TCtrlType::Unknown,
                boost::none) {}
};

const TCtrlTypes &CtrlTypes()
{
    static TCtrlTypes ctrlTypes;
    return ctrlTypes;
}

RequestType decodeRequestType(const string &name)
{
    for(const auto &i: getRequestTypeMap()) {
        if(i.second.name == name) return i.first;
    }
    throw Exception("%s: not found for '%s'", __func__, name.c_str());
}

ParameterName decodeParameterName(const string &name)
{
    for(const auto &i: getParameterNameMap()) {
        if(i.second.name == name) return i.first;
    }
    throw Exception("%s: not found for '%s'", __func__, name.c_str());
}

DocumentGroup decodeDocumentGroup(const string &name)
{
    for(const auto &i: getDocumentGroupMap()) {
        if(i.second.name == name) return i.first;
    }
    throw Exception("%s: not found for '%s'", __func__, name.c_str());
}

SecondaryDocumentType decodeSecondaryDocumentType(const string &name)
{
    for(const auto &i: getSecondaryDocumentTypeMap()) {
        if(i.second.name == name) return i.first;
    }
    throw Exception("%s: not found for '%s'", __func__, name.c_str());
}

Gender decodeGender(const string &name)
{
    for(const auto &i: getGenderMap())
        if(i.second.name == name) return i.first;
    throw Exception("%s: not found for '%s'", __func__, name.c_str());
}

Visa decodeVisa(const string &name)
{
    for(const auto &i: getVisaMap())
        if(i.second.name == name) return i.first;
    throw Exception("%s: not found for '%s'", __func__, name.c_str());
}

Ticket decodeTicket(const string &name)
{
    for(const auto &i: getTicketMap())
        if(i.second.name == name) return i.first;
    throw Exception("%s: not found for '%s'", __func__, name.c_str());
}

DocumentFeature decodeDocumentFeature(const string &name)
{
    for(const auto &i: getDocumentFeatureMap())
        if(i.second.name == name) return i.first;
    throw Exception("%s: not found for '%s'", __func__, name.c_str());
}

StayType decodeStayType(const string &name)
{
    for(const auto &i: getStayTypeMap())
        if(i.second.name == name) return i.first;
    throw Exception("%s: not found for '%s'", __func__, name.c_str());
}

ResidencyDocument decodeResidencyDocument(const string &name)
{
    for(const auto &i: getResidencyDocumentMap())
        if(i.second.name == name) return i.first;
    throw Exception("%s: not found for '%s'", __func__, name.c_str());
}

DataSection decodeSection(const string &name)
{
    for(const auto &i: getDataSectionMap())
        if(i.second.name == name) return i.first;
    throw Exception("%s: not found for '%s'", __func__, name.c_str());
}

DocumentType decodeDocumentType(const string &name)
{
    for(const auto &i: getDocumentTypeMap())
        if(i.second.name == name) return i.first;
    throw Exception("%s: not found for '%s'", __func__, name.c_str());
}

struct TLexemeListItem {
    string lat_code_not_found;
};

struct TLexemeList:public map<TElemType, TLexemeListItem> {
    void fromXML(xmlNodePtr node)
    {
        if(not node) return;
        xmlNodePtr curNode = node->children;
        curNode = GetNodeFast("lexeme_list", curNode);
        if(curNode) {
            curNode = curNode->children;
            for(; curNode; curNode = curNode->next) {
                xmlNodePtr node = curNode->children;
                auto &i = (*this)[DecodeElemType(NodeAsStringFast("ref", node))];
                i.lat_code_not_found = NodeAsStringFast("lat_code_not_found", node);
            }
        }
    }
};

struct TLayoutItem {

    struct TSource {
        private:

        string name;
        boost::optional<ParameterName> val;

        public:

        TSource (const string _val)
        {
            name = _val;
            if(name != "parameterName")
                val = decodeParameterName(_val);
        }

        string descr(const string &_val)
        {
            if(val)
                switch(val.get()) {
                    case ParameterName::DocumentType:
                        return getParams(decodeDocumentType(_val)).displayName;
                    case ParameterName::DataSection:
                        return getParams(decodeSection(_val)).displayName;
                    case ParameterName::Gender:
                        return getParams(decodeGender(_val)).displayName;
                    case ParameterName::StayType:
                        return getParams(decodeStayType(_val)).displayName;
                    case ParameterName::DocumentFeature:
                        return getParams(decodeDocumentFeature(_val)).displayName;
                    case ParameterName::ResidencyDocument:
                        return getParams(decodeResidencyDocument(_val)).displayName;
                    case ParameterName::DocumentGroup:
                        return getParams(decodeDocumentGroup(_val)).displayName;
                    case ParameterName::SecondaryDocumentType:
                        return getParams(decodeSecondaryDocumentType(_val)).displayName;
                    case ParameterName::Visa:
                        return getParams(decodeVisa(_val)).displayName;
                    case ParameterName::Ticket:
                        return getParams(decodeTicket(_val)).displayName;
                    default:
                        throw Exception("descr not implemented for %s", toString(val.get()).c_str());
                }
            else
                return getParams(decodeParameterName(_val)).displayName;
        }

        void fill_container(xmlNodePtr dataNode)
        {
            if(val)
                switch(val.get()) {
                    case ParameterName::DocumentType:
                        Timatic::fill_container(dataNode, name, getDocumentTypeMap());
                        break;
                    case ParameterName::DataSection:
                        Timatic::fill_container(dataNode, name, getDataSectionMap());
                        break;
                    case ParameterName::DocumentGroup:
                        Timatic::fill_container(dataNode, name, getDocumentGroupMap());
                        break;
                    case ParameterName::SecondaryDocumentType:
                        Timatic::fill_container(dataNode, name, getSecondaryDocumentTypeMap());
                        break;
                    case ParameterName::DocumentFeature:
                        Timatic::fill_container(dataNode, name, getDocumentFeatureMap());
                        break;
                    case ParameterName::StayType:
                        Timatic::fill_container(dataNode, name, getStayTypeMap());
                        break;
                    case ParameterName::ResidencyDocument:
                        Timatic::fill_container(dataNode, name, getResidencyDocumentMap());
                        break;
                    case ParameterName::Gender:
                        Timatic::fill_container(dataNode, name, getGenderMap());
                        break;
                    case ParameterName::Visa:
                        Timatic::fill_container(dataNode, name, getVisaMap());
                        break;
                    case ParameterName::Ticket:
                        Timatic::fill_container(dataNode, name, getTicketMap());
                        break;
                    default:
                        throw Exception("%s: unknown source '%s'", __func__, name.c_str());
                }
            else
                Timatic::fill_container(dataNode, name, getParameterNameMap());
        }
    };

    size_t sort;
    string name;
    TCtrlType::Enum type;
    bool mandatory;
    boost::optional<string> caption;
    boost::optional<TElemType> elem_type;
    boost::optional<TSource> source;

    const TLexemeList &lexeme_list;

    string descr();
    string asLat(const string &val);
    virtual bool empty() = 0;
    virtual void fromXML(xmlNodePtr node);
    virtual void parseReq(xmlNodePtr node) = 0;
    virtual string str() = 0;
    virtual void set_val(const string &str) = 0;
    virtual ~TLayoutItem() = default;
    TLayoutItem(const TLexemeList &_lexeme_list): lexeme_list(_lexeme_list) {}

    template <typename T>
    bool is() const { return dynamic_cast<const T*>(this)!=NULL; };
    template<typename T> const T &as() const
    {
        if(not is<T>()) throw Exception("class not %s", typeid(T).name());
        return dynamic_cast<const T&>(*this);
    }
};

void TLayoutItem::fromXML(xmlNodePtr node)
{
    name = NodeAsStringFast("name", node);
    mandatory = NodeIsNULLFast("mandatory", node, false);
    xmlNodePtr capNode = GetNodeFast("caption", node);
    if(capNode)
        caption = NodeAsString(capNode);
    else {
        try {
            elem_type = DecodeElemType(NodeAsStringFast("ref", node));
        } catch(Exception &) {}
        try {
            source = boost::in_place(NodeAsStringFast("source", node));
        } catch(Exception &) {}
    }
    // Проверим, что с lexeme_list все в порядке
    if(elem_type and lexeme_list.find(elem_type.get()) == lexeme_list.end())
        throw Exception("lexeme not found for %s", EncodeElemType(elem_type.get()));
}

string TLayoutItem::asLat(const string &val)
{
    string result = val;
    if(not val.empty() and elem_type and not IsAscii7(val)) {
        result = ElemIdToElem(elem_type.get(), val, efmtCodeInter, AstraLocale::LANG_EN);
        if(result.empty())
            throw AstraLocale::UserException(lexeme_list.find(elem_type.get())->second.lat_code_not_found, LParams() << LParam("id", val));
    }
    return result;
}

/*
template <class T> T getLayoutItem(shared_ptr<TLayoutItem> item) {
    static_assert(std::is_base_of<TLayoutItem, T>::value, "not TLayoutItem descendant");
    return *std::static_pointer_cast<T>(item);
}
*/

struct TLayoutListItem: public TLayoutItem {
    vector<string> val;
    bool empty() { return val.empty(); }
    void set_val(const string &str) { val.push_back(asLat(str)); }
    void parseReq(xmlNodePtr node) {
        xmlNodePtr curNode = node->children;
        for(; curNode; curNode = curNode->next)
            set_val(NodeAsString(curNode));
    }
    string str() {
        ostringstream result;
        for(const auto &i: val) {
            if(not result.str().empty())
                result << ", ";
            result << i;
        }
        return result.str();
    }
    TLayoutListItem(const TLexemeList &_lexeme_list): TLayoutItem(_lexeme_list) {}
};

struct TLayoutDateItem: public TLayoutItem {
    TDateTime val;
    string DateFormat;
    virtual void fromXML(xmlNodePtr node)
    {
        TLayoutItem::fromXML(node);
        DateFormat = NodeAsStringFast("DateFormat", node, ServerFormatDateTimeAsString);
    }
    bool empty() { return val == NoExists; }
    void set_val(const string &str)
    {
        val = date_fromXML(str);
    }
    void parseReq(xmlNodePtr node) {
        set_val(NodeAsString(node));
    }
    string str() { return DateTimeToStr(val, DateFormat); }
    TLayoutDateItem(const TLexemeList &_lexeme_list): TLayoutItem(_lexeme_list)
    {
        val = NoExists;
    }
};

struct TLayoutStringItem: public TLayoutItem {
    string val;
    bool empty() { return val.empty(); }
    void set_val(const string &str)
    {
        val = asLat(str);
    }
    void parseReq(xmlNodePtr node) {
        set_val(NodeAsString(node));
    }
    string str() { return val; }
    TLayoutStringItem(const TLexemeList &_lexeme_list): TLayoutItem(_lexeme_list) {}
};

string TLayoutItem::descr()
{
    if(source) {
        return source->descr(as<TLayoutStringItem>().val);
    } else
        return str();
}


struct TLayoutPageItem;

struct TLayout: public map<string, shared_ptr<TLayoutItem>> {
    string reqTypeCaption;

    boost::optional<TLexemeList> lexeme_list;

    struct TCompareCtrl {
        bool operator() (const shared_ptr<TLayoutItem> &t1, const shared_ptr<TLayoutItem> &t2) const
        { return t1->sort < t2->sort; }
    };

    typedef set<shared_ptr<TLayoutItem>, TCompareCtrl> TSortedCtrl;

    TSortedCtrl sorted() const
    {
        TSortedCtrl result;
        for(const auto &i: *this) result.insert(i.second);
        return result;
    }

    void init(const string &req_type, xmlNodePtr ctrlNode = nullptr);

    bool empty() const {
        for(const auto &i: *this)
            if(not i.second->empty()) return false;
        return true;
    }

    void mandatory()
    {
        for(const auto &i: *this)
            if(i.second->empty() and i.second->mandatory)
                throw AstraLocale::UserException("MSG.TABLE.NOT_SET_FIELD_VALUE", LParams() << LParam("fieldname", caption(i.first)));
    }

    string caption(const string &code) const
    {
        const auto i = find(code + "Lbl");
        string result = code;
        if(i != end())
            result = i->second->caption.get();
        return result;
    }

    void parseReq(xmlNodePtr node)
    {
        xmlNodePtr curNode = node->children;
        for(; curNode; curNode = curNode->next) {
            parseReq(curNode);
            string code = (const char *)curNode->name;
            auto item = find(code);
            if(item != end())
                item->second->parseReq(curNode);
        }
    }

    void SetRequest(ParamValuesReq &req)
    {
        for(const auto i: *this) {
            if(i.second->empty()) continue;
            const string &code = i.first;
            if(i.second->is<TLayoutStringItem>()) {
                const string &val = i.second->as<TLayoutStringItem>().val;
                if(code == "ParameterName")
                    req.parameterName(decodeParameterName(val));
            }
        }
    }

    void SetRequest(ParamReq &req)
    {
        for(const auto i: *this) {
            if(i.second->empty()) continue;
            const string &code = i.first;
            if(i.second->is<TLayoutStringItem>()) {
                const string &val = i.second->as<TLayoutStringItem>().val;
                if(code == "ParamCountry")
                    req.countryCode(val);
                if(code == "ParamSection")
                    req.section(decodeSection(val));
            }
            if(i.second->is<TLayoutListItem>()) {
                const auto &val = i.second->as<TLayoutListItem>().val;
                for(const auto &i: val) {
                    if(code == "ParamTransitCountry")
                        req.transitCountry(i);
                }
            }
        }
    }

    void SetPageReq(DocumentReq &req, shared_ptr<TLayoutItem> item);

    void SetRequest(const string &prefix, StayDuration &stayDuration)
    {
        for(const auto i: *this) {
            if(i.second->empty()) continue;
            const string &code = i.first;
            if(i.second->is<TLayoutStringItem>()) {
                const string &val = i.second->as<TLayoutStringItem>().val;
                int int_val = 0;
                if(not val.empty())
                    int_val = ToInt(val);
                if(code == prefix + "Days")
                    stayDuration.days(int_val);
                if(code == prefix + "Hours")
                    stayDuration.hours(int_val);
            }
        }
    }

    void SetRequest(const string &prefix, VisaData &visaData)
    {
        for(const auto i: *this) {
            if(i.second->empty()) continue;
            const string &code = i.first;
            if(i.second->is<TLayoutStringItem>()) {
                const string &val = i.second->as<TLayoutStringItem>().val;
                if(code == prefix + "Type")
                    visaData.visaType(val);
                if(code == prefix + "IssuedBy")
                    visaData.issuedBy(val);
            }
            if(i.second->is<TLayoutDateItem>()) {
                TDateTime val = i.second->as<TLayoutDateItem>().val;
                if(code == prefix + "ExpiryDate") visaData.expiryDateGMT(DateTimeToBoost(val));
            }
        }
    }

    void SetRequest(const string &prefix, TransitCountry &country);

    void SetRequest(DocumentReq &req)
    {
        for(const auto i: *this) {
            if(i.second->empty()) continue;
            const string &code = i.first;
            if(i.second->is<TLayoutPageItem>()) {
                SetPageReq(req, i.second);
            }
            if(i.second->is<TLayoutStringItem>()) {
                const string &val = i.second->as<TLayoutStringItem>().val;
                if(code == "Section")
                    req.section(decodeSection(val));
                if(code == "Destination")
                    req.destinationCode(val);
                if(code == "Nationality")
                    req.nationalityCode(val);
                if(code == "DocumentType")
                    req.documentType(decodeDocumentType(val));
                if(code == "secondaryDoc")
                    req.secondaryDocumentType(decodeSecondaryDocumentType(val));
                if(code == "DocumentGroup")
                    req.documentGroup(decodeDocumentGroup(val));
                if(code == "IssueCountry")
                    req.issueCountryCode(val);
                if(code == "ResidentCountry")
                    req.residentCountryCode(val);
                if(code == "DepartAirp")
                    req.departAirportCode(val);
                if(code == "CarrierCode")
                    req.carrierCode(val);
                if(code == "StayType")
                    req.stayType(decodeStayType(val));
                if(code == "BirthCountry")
                    req.birthCountryCode(val);
                if(code == "Gender")
                    req.gender(decodeGender(val));
                if(code == "PassportSeries")
                    req.passportSeries(val);
                if(code == "Visa")
                    req.visa(decodeVisa(val));
                if(code == "ResidencyDocument")
                    req.residencyDocument(decodeResidencyDocument(val));
                if(code == "DocumentFeature")
                    req.documentFeature(decodeDocumentFeature(val));
                if(code == "Ticket")
                    req.ticket(decodeTicket(val));
            }
            if(i.second->is<TLayoutListItem>()) {
                const auto &val = i.second->as<TLayoutListItem>().val;
                for(const auto &i: val) {
                    if(code == "CountriesVisited")
                        req.countriesVisited(i);
                }
            }
            if(i.second->is<TLayoutDateItem>()) {
                TDateTime val = i.second->as<TLayoutDateItem>().val;
                if(code == "IssueDate")     req.issueDateGMT(DateTimeToBoost(val));
                if(code == "ExpiryDate")    req.expiryDateGMT(DateTimeToBoost(val));
                if(code == "ArrivalDate")   req.arrivalDateGMT(DateTimeToBoost(val));
                if(code == "DepartDate")    req.departDateGMT(DateTimeToBoost(val));
                if(code == "BirthDate")     req.birthDateGMT(DateTimeToBoost(val));
            }
        }
    }

    void set_val(const string &name, const string &val);

};

struct TLayoutPageItem: public TLayoutItem {

    struct TTabItem {
        string caption;
        string tabType;
        TLayout layout;
    };

    vector<TTabItem> tabs;

    virtual void fromXML(xmlNodePtr node)
    {
        TLayoutItem::fromXML(node);
        if(type == TCtrlType::page) {
            node = NodeAsNodeFast("tab_list", node)->children;
            for(; node; node = node->next) {
                xmlNodePtr curNode = node->children;
                tabs.emplace_back();
                tabs.back().layout.lexeme_list = lexeme_list;
                tabs.back().caption = NodeAsStringFast("caption", curNode);
                tabs.back().tabType = NodeAsStringFast("tab_type", curNode);
                tabs.back().layout.init({}, NodeAsNodeFast("ctrl_list", curNode));
            }
        } else {
            tabs.emplace_back();
            tabs.back().layout.lexeme_list = lexeme_list;
            if(caption)
                tabs.back().caption = caption.get();
            tabs.back().tabType = name;
            tabs.back().layout.init({}, NodeAsNodeFast("ctrl_list", node));
        }
    }

    bool empty() {
        for(const auto &tab: tabs)
            if(not tab.layout.empty()) return false;
        return true;
    }

    void set_val(const string &str) {}

    virtual void parseReq(xmlNodePtr node)
    {
        if(type == TCtrlType::page) {
            xmlNodePtr curNode = node->children;
            for(; curNode; curNode = curNode->next)
                for(auto &tab: tabs)
                    if(tab.tabType == (const char *)curNode->name)
                        tab.layout.parseReq(curNode);
        } else
            tabs.back().layout.parseReq(node);
    }

    string str() { return {}; };

    TLayoutPageItem(const TLexemeList &_lexeme_list): TLayoutItem(_lexeme_list) {}
};

shared_ptr<string> params_to_str(const TLayout &layout, shared_ptr<string> str = {})
{
    if(not str) str = make_shared<string>();
    for(const auto &i: layout.sorted()) {
        if(i->empty()) continue;
        *str += i->name + ": " + i->descr() + "; ";
        if(i->is<TLayoutPageItem>()) {
            const auto &tabs = i->as<TLayoutPageItem>().tabs;
            for(const auto &tab: tabs) {
                params_to_str(tab.layout, str);
            }
        }
    }
    return str;
}

shared_ptr<TLayoutItem> find_layout_item(const TLayout &layout, const string &name)
{
    auto res = layout.find(name);
    if(res != layout.end())
        return res->second;
    else {
        for(const auto &i: layout) {
            if(i.second->is<TLayoutPageItem>()) {
                const auto &tabs = i.second->as<TLayoutPageItem>().tabs;
                for(const auto &tab: tabs) {
                    auto res = find_layout_item(tab.layout, name);
                    if(res) return res;
                }
            }
        }
        return nullptr;
    }
}

void TLayout::set_val(const string &name, const string &val)
{
    auto item = find_layout_item(*this, name);
    if(item)
        item->set_val(val);
    else
        throw Exception("TLayout::set_val: %s not found", name.c_str());
}

void TLayout::SetRequest(const string &prefix, TransitCountry &country)
{
    for(const auto i: *this) {
        if(i.second->empty()) continue;
        const string &code = i.first;
        if(i.second->is<TLayoutPageItem>()) {
            if(code == prefix + "VisaData") {
                TLayoutPageItem &page = dynamic_cast<TLayoutPageItem &>(*(i.second));
                VisaData visaData;
                page.tabs.back().layout.SetRequest(code, visaData);
                country.visaData(visaData);
            }
        }
        if(i.second->is<TLayoutStringItem>()) {
            const string &val = i.second->as<TLayoutStringItem>().val;
            if(code == prefix + "Airp")
                country.airportCode(val);
            if(code == prefix + "Visa")
                country.visa(decodeVisa(val));
            if(code == prefix + "Ticket")
                country.ticket(decodeTicket(val));
        }
        if(i.second->is<TLayoutDateItem>()) {
            TDateTime val = i.second->as<TLayoutDateItem>().val;
            if(code == prefix + "ArrivalTime") country.arrivalTimestamp(DateTimeToBoost(val));
            if(code == prefix + "DepartureTime") country.departTimestamp(DateTimeToBoost(val));
        }
    }
}

void TLayout::SetPageReq(DocumentReq &req, shared_ptr<TLayoutItem> item)
{
    TLayoutPageItem &page = dynamic_cast<TLayoutPageItem &>(*item);
    for(auto &tab: page.tabs) {
        if(tab.layout.empty()) continue;
        if(page.name == "TCountry") {
            TransitCountry country;
            tab.layout.SetRequest(tab.tabType, country);
            req.transitCountry(country);
        }
        if(page.name == "StayDuration") {
            StayDuration stayDuration;
            tab.layout.SetRequest(tab.tabType, stayDuration);
            req.stayDuration(stayDuration);
        }
        if(page.name == "VisaData") {
            VisaData visaData;
            tab.layout.SetRequest(tab.tabType, visaData);
            req.visaData(visaData);
        }
    }
}


void TLayout::init(const string &req_type, xmlNodePtr ctrlNode)
{
    Optional<XMLDoc> layout; // инит в корневом вызове только.
    static size_t sort;
    if(not ctrlNode) {
        sort = 0;
        layout = boost::in_place(StrUtils::b64_decode(getResource(TIMATIC_XML_RESOURCE)));
        xml_decode_nodelist(layout->docPtr()->children);
        xmlNodePtr curNode = layout->docPtr()->children;
        curNode = NodeAsNodeFast("layout", curNode)->children;
        lexeme_list = boost::in_place();
        lexeme_list->fromXML(GetNodeFast("data", curNode));
        curNode = NodeAsNodeFast("ctrl_list", curNode)->children;
        curNode = NodeAsNodeFast("ctrl", curNode)->children;
        curNode = NodeAsNodeFast("tab_list", curNode)->children;
        while(curNode) {
            xmlNodePtr node = curNode->children;
            if(NodeAsStringFast("tab_type", node) == req_type) {
                reqTypeCaption = NodeAsStringFast("caption", node);
                ctrlNode = NodeAsNodeFast("ctrl_list", node);
                break;
            }
            curNode = curNode->next;
        }
        if(not ctrlNode)
            throw Exception("layout: req_type not found %s", req_type.c_str());
    }

    ctrlNode = ctrlNode->children;

    for(; ctrlNode; ctrlNode = ctrlNode->next) {
        xmlNodePtr curNode = ctrlNode->children;
        TCtrlType::Enum type = CtrlTypes().decode(NodeAsStringFast("type", curNode));
        xmlNodePtr ctrlListNode = GetNodeFast("ctrl_list", curNode);
        if(type != TCtrlType::Unknown) {
            shared_ptr<TLayoutItem> item;
            switch(type) {
                case TCtrlType::list:
                    item = make_shared<TLayoutListItem>(lexeme_list.get());
                    break;
                case TCtrlType::date:
                case TCtrlType::dateTime:
                    item = make_shared<TLayoutDateItem>(lexeme_list.get());
                    break;
                case TCtrlType::page:
                case TCtrlType::group:
                case TCtrlType::panel:
                    // Если это был page, то не надо парсить чайлды, т.к. они уже в page fromXML далее
                    ctrlListNode = nullptr;
                    item = make_shared<TLayoutPageItem>(lexeme_list.get());
                    break;
                default:
                    item = make_shared<TLayoutStringItem>(lexeme_list.get());
            }
            item->type = type;
            item->sort = sort++;
            item->fromXML(curNode);
            insert(make_pair(item->name, item));
        }
        if(ctrlListNode)
            init({}, ctrlListNode);
    }
}

struct TReqParamTab {
    private:
        struct TRow {
            const int &FMaxColSpan;
            string param;
            string val;
            vector<TRow> rows;
            int rowspan() const;
            void toXML(xmlNodePtr trNode, xmlNodePtr tdNode, int lvl) const;
            TRow(int &_FMaxColSpan): FMaxColSpan(_FMaxColSpan) {}
        };
        vector<TRow> rows;
        int FMaxColSpan;
        void fromLayout(const TLayout &layout, boost::optional<vector<TRow> &> table = boost::none, int lvl = 0);
        void IncLvl(int &lvl);
    public:
        void toXML(const TLayout &layout, xmlNodePtr tableNode);
        void trace(boost::optional<const vector<TRow> &> table = boost::none);
        void add(const string &param, const string &val);
};

void TReqParamTab::add(const string &param, const string &val)
{
    rows.emplace_back(FMaxColSpan);
    rows.back().param = param;
    rows.back().val = val;
}

void TReqParamTab::TRow::toXML(xmlNodePtr trNode, xmlNodePtr tdNode, int lvl) const
{
    lvl++;
    if(rows.empty()) {
        SetProp(tdNode, "colspan", FMaxColSpan - lvl + 1, 1);
        NewTextChild(trNode, "td", val);
    } else {
        vector<TRow>::const_iterator i = rows.begin();
        for(; i != rows.end(); i++) {
            if(i == rows.begin()) {
                tdNode = NewTextChild(trNode, "td", i->param);
            } else {
                xmlNodePtr tableNode = trNode->parent;
                trNode = NewTextChild(tableNode, "tr");
                tdNode = NewTextChild(trNode, "td", i->param);
            }
            SetProp(tdNode, "rowspan", i->rowspan(), 1);
            i->toXML(trNode, tdNode, lvl);
        }
    }
}

void TReqParamTab::toXML(const TLayout &layout, xmlNodePtr tableNode)
{
    FMaxColSpan = 0;
    fromLayout(layout, rows);
    int lvl = 0;
    for(const auto &i: rows) {
        xmlNodePtr trNode = NewTextChild(tableNode, "tr");
        xmlNodePtr tdNode = NewTextChild(trNode, "td", i.param);
        SetProp(tdNode, "rowspan", i.rowspan(), 1);
        i.toXML(trNode, tdNode, lvl);
    }
}

int TReqParamTab::TRow::rowspan() const
{
    int result = 1;
    for(const auto &i: rows)
        result += i.rowspan();
    if(not rows.empty()) result--;
    return result;
}

void TReqParamTab::trace(boost::optional<const vector<TReqParamTab::TRow> &> table)
{
    if(not table)
        table = rows;
    for(const auto &i: table.get()) {
        LogTrace(TRACE5)
            << "param: " << i.param
            << "; rowspan: " << i.rowspan()
            << "; size: " << i.rows.size()
            << "; val: '" + i.val + "'";
        trace(i.rows);
    }
}

void TReqParamTab::IncLvl(int &lvl)
{
    lvl++;
    if(lvl > FMaxColSpan)
        FMaxColSpan = lvl;
}

void TReqParamTab::fromLayout(const TLayout &layout, boost::optional<vector<TReqParamTab::TRow> &> table, int lvl)
{
    if(not table) {
        rows.clear();
        table = rows;
    }
    IncLvl(lvl);
    for(const auto &i: layout.sorted()) {
        if(i->empty()) continue;
        table->emplace_back(FMaxColSpan);
        auto &row = table->back();
        row.param = layout.caption(i->name);
        row.val = i->descr();

        if(i->is<TLayoutPageItem>()) {
            const auto &tabs = i->as<TLayoutPageItem>().tabs;
            for(const auto &tab: tabs) {
                if(tab.layout.empty()) continue;
                if(tab.caption.empty())
                    fromLayout(tab.layout, row.rows, lvl);
                else {
                    if(
                            row.param == i->name and // Для тек контейнера не найден соов. Lbl
                            tabs.size() == 1 // только одна страница
                      ) {
                        // Тогда в кач-ве Lbl юзаем заголовок страницы (tab.caption)
                        row.param = tab.caption;
                        fromLayout(tab.layout, row.rows, lvl);
                    } else {
                        IncLvl(lvl);
                        row.rows.emplace_back(FMaxColSpan);
                        row.rows.back().param = tab.caption;
                        fromLayout(tab.layout, row.rows.back().rows, lvl);
                    }
                }
            }
        }
    }
}

void TTimaticAccess::fromXML(xmlNodePtr node)
{
    clear();
    if(not node) return;
    xmlNodePtr curNode = node->children;
    curNode = NodeAsNodeFast("config", curNode)->children;
    airline = NodeAsStringFast("airline", curNode);
    airp = NodeAsStringFast("airp", curNode);
    host = NodeAsStringFast("host", curNode);
    port = NodeAsIntegerFast("port", curNode);
    username = NodeAsStringFast("username", curNode);
    sub_username = NodeAsStringFast("sub_username", curNode);
    pwd = NodeAsStringFast("pwd", curNode);
    pr_denial = NodeAsIntegerFast("pr_denial", curNode) != 0;
}

void TTimaticAccess::trace() const
{
    LogTrace(TRACE5) << "---TTimaticAccess---";
    LogTrace(TRACE5) << "airline: " << airline;
    LogTrace(TRACE5) << "airp: " << airp;
    LogTrace(TRACE5) << "host: " << host;
    LogTrace(TRACE5) << "port: " << port;
    LogTrace(TRACE5) << "username: " << username;
    LogTrace(TRACE5) << "sub_username: " << sub_username;
    LogTrace(TRACE5) << "pwd: " << pwd;
    LogTrace(TRACE5) << "pr_denial: " << pr_denial;
    LogTrace(TRACE5) << "--------------------";
}

void TTimaticAccess::toXML(xmlNodePtr node)
{
    if(GetNode("config", node)) return;

    xmlNodePtr configNode = NewTextChild(node, "config");
    NewTextChild(configNode, "airline", airline);
    NewTextChild(configNode, "airp", airp);
    NewTextChild(configNode, "host", host);
    NewTextChild(configNode, "port", port);
    NewTextChild(configNode, "username", username);
    NewTextChild(configNode, "sub_username", sub_username);
    NewTextChild(configNode, "pwd", pwd);
    NewTextChild(configNode, "pr_denial", pr_denial);
}

void TTimaticAccess::clear()
{
    airline.clear();
    airp.clear();
    host.clear();
    port = NoExists;
    username.clear();
    sub_username.clear();
    pwd.clear();
    pr_denial = true;
}

const TTimaticAccess &TTimaticAccess::fromDB(TQuery &Qry)
{
    clear();
    if(not Qry.Eof) {
        airline = Qry.FieldAsString("airline");
        airp = Qry.FieldAsString("airp");
        host = Qry.FieldAsString("host");
        port = Qry.FieldAsInteger("port");
        username = Qry.FieldAsString("username");
        sub_username = Qry.FieldAsString("sub_username");
        pwd = Qry.FieldAsString("pwd");
        pr_denial = Qry.FieldAsInteger("pr_denial") != 0;
    }
    return *this;
}

vector<TTimaticAccess> GetTimaticAccessSets()
{
    vector<TTimaticAccess> result;
    TCachedQuery Qry(
            "select timatic_sets.*, "
            "   DECODE( airline, NULL, 0, 8 )+ "
            "   DECODE( airp, NULL, 0, 4 ) priority "
            "from timatic_sets "
            "order by priority desc");
    Qry.get().Execute();
    for(; not Qry.get().Eof; Qry.get().Next()) {
        result.push_back(TTimaticAccess().fromDB(Qry.get()));
    }
    return result;
}

boost::optional<TTimaticAccess> GetTimaticUserAccess()
{
    boost::optional<TTimaticAccess> result;
    auto access_sets = GetTimaticAccessSets();
    for(const auto &i: access_sets) {
        if(
                (i.airline.empty() or TReqInfo::Instance()->user.access.airlines().permitted(i.airline)) and
                (i.airp.empty() or TReqInfo::Instance()->user.access.airps().permitted(i.airp)) and
                not i.pr_denial
          ) {
            result = i;
            break;
        }
    }
    return result;
}

boost::optional<TTimaticAccess> GetTimaticAccess(const string &airline, const string &airp)
{
    boost::optional<TTimaticAccess> result;
    TCachedQuery Qry(
            "select timatic_sets.*, "
            "   DECODE( airline, NULL, 0, 8 )+ "
            "   DECODE( airp, NULL, 0, 4 ) priority "
            "from timatic_sets where "
            "   (airline IS NULL OR airline=:airline) AND "
            "   (airp IS NULL OR airp = :airp) "
            "order by priority desc",
            QParams()
            << QParam("airline", otString, airline)
            << QParam("airp", otString, airp)
            );
    Qry.get().Execute();
    if(not Qry.get().Eof and Qry.get().FieldAsInteger("pr_denial") == 0) {
        result = boost::in_place();
        result->fromDB(Qry.get());
    }
    return result;
}

RequestType initRequestLayout(xmlNodePtr reqNode, TLayout &layout)
{
    xmlNodePtr curNode = reqNode->children;
    curNode = NodeAsNodeFast("pageControl", curNode)->children;
    if(not curNode) throw Exception("tab node not found");
    string tab_type = (const char *)curNode->name;
    layout.init(tab_type);
    layout.parseReq(curNode);
    return decodeRequestType(tab_type);
}

shared_ptr<Request> parseRequest(xmlNodePtr reqNode)
{
    TLayout layout;
    RequestType request_type = initRequestLayout(reqNode, layout);
    layout.mandatory();
    shared_ptr<Request> result;
    switch(request_type) {
        case RequestType::Document:
            result = make_shared<DocumentReq>();
            layout.SetRequest(dynamic_cast<DocumentReq&>(*result));
            break;
        case RequestType::Param:
            result = make_shared<ParamReq>();
            layout.SetRequest(dynamic_cast<ParamReq&>(*result));
            break;
        case RequestType::ParamValues:
            result = make_shared<ParamValuesReq>();
            layout.SetRequest(dynamic_cast<ParamValuesReq&>(*result));
            break;
        default:
            throw Exception("%s: not implemented for %s", __func__, toString(request_type).c_str());
    }
    return result;
}

void layout_add_data(xmlNodePtr node, xmlNodePtr dataNode)
{
    xmlNodePtr curNode = node->children;
    while(curNode) {
        if((string)"source" == (const char *)curNode->name)
            TLayoutItem::TSource(NodeAsString(curNode)).fill_container(dataNode);
        layout_add_data(curNode, dataNode);
        curNode = curNode->next;
    }
}

int initDocRequestLayout(xmlNodePtr reqNode, TLayout &layout, shared_ptr<TTripInfo> flt = {})
{
    if(not flt) flt = make_shared<TTripInfo>();
    int pax_id = NodeAsInteger("pax_id", reqNode);

    string airp_dep, airp_arv;
    if(flt->getByPaxId(pax_id)) {
        CheckIn::TSimplePaxGrpItem grp;
        grp.getByPaxId(pax_id);
        airp_dep = grp.airp_dep;
        airp_arv = grp.airp_arv;
    } else if(flt->getByCRSPaxId(pax_id)) {
        /*
           r231069 не перенесен из stand !!!
        auto crs_segment = CheckIn::crsSegment(PaxId_t(pax_id));
        if(crs_segment) {
            airp_dep = flt->airp;
            airp_arv = crs_segment->airp_arv;
        }
        */
    }

    xmlNodePtr docNode = NodeAsNode("document", reqNode);
    xmlNodePtr curNode = docNode->children;
    string doc_type = NodeAsStringFast("type", curNode, "");
    string issue_country = NodeAsStringFast("issue_country", curNode, "");
    string no = NodeAsStringFast("no", curNode, "");
    string nationality = NodeAsStringFast("nationality", curNode, "");
    string birth_date = NodeAsStringFast("birth_date", curNode, "");
    string gender = NodeAsStringFast("gender", curNode, "");
    string expiry_date = NodeAsStringFast("expiry_date", curNode, "");
    
    layout.init(toString(RequestType::Document));

    layout.set_val("Section", toString(DataSection::PassportVisaHealth));
    layout.set_val("Destination", getCountryByAirp(airp_arv).AsString("code"));
    if(not nationality.empty())
        layout.set_val("Nationality", getBaseTable(etPaxDocCountry).get_row("code",nationality).AsString("country"));

    if(doc_type == "P") { // Пассажирский паспорт
        layout.set_val("DocumentType", toString(DocumentType::Passport));
        if(not issue_country.empty())
            layout.set_val("IssueCountry", getBaseTable(etPaxDocCountry).get_row("code",issue_country).AsString("country"));
        if(not expiry_date.empty())
            layout.set_val("ExpiryDate", expiry_date);
        if(not no.empty())
            layout.set_val("PassportSeries", no);
    } else
        throw UserException("Timatic: 'Document type' field value must be 'P'");

    layout.set_val("DocumentGroup", toString(DocumentGroup::AlienResidents));
    if(not birth_date.empty())
        layout.set_val("BirthDate", birth_date);
    if(not gender.empty())
            layout.set_val("Gender", toString(CheckIn::is_female(gender, "") ? Gender::Female : Gender::Male));
    layout.set_val("DepartAirp", airp_dep);
    layout.set_val("CarrierCode", flt->airline);
    return pax_id;
}

shared_ptr<Request> parseDocRequest(xmlNodePtr reqNode)
{
    TLayout layout;
    initDocRequestLayout(reqNode, layout);
    layout.mandatory();
    shared_ptr<Request> result = make_shared<DocumentReq>();
    layout.SetRequest(dynamic_cast<DocumentReq&>(*result));
    return result;
}

void processReq(xmlNodePtr reqNode, xmlNodePtr resNode, const HttpData &httpData)
{
    XMLDoc html = XMLDoc("html");
    xmlNodePtr htmlNode = html.docPtr()->children;
    xmlNodePtr headNode = NewTextChild(htmlNode, "head");
    xmlNodePtr metaNode = NewTextChild(headNode, "meta");
    SetProp(metaNode, "http-equiv", "Content-Type");
    SetProp(metaNode, "content", "text/html; charset=utf-8");

    // ECS = 27,
    // Alt-X = 88,
    // Ctrl-O = 79 - открыть url - не дай боже
    // Backspace = 8
    NewTextChild(headNode, "script",
            "function keypresed() {"
            "  var tecla=window.event.keyCode;"
            "  if(tecla == 27 || tecla == 88 || tecla == 79 || tecla == 8) {"  
            "    document.title = 'Command'+tecla;"
            "    event.keyCode=0;"
            "    event.returnValue=false;"
            "  }"
            "}"
            "document.onkeydown=keypresed;");

    xmlNodePtr styleNode = NewTextChild(headNode, "style", ".default_color{color:#00305B} .red_color{color:red}");

    //    xmlNodePtr styleNode = NewTextChild(headNode, "style",
    //            "*{ "
    //            "    margin:0; "
    //            "    padding:0; "
    //            "    color:#00305B; "
    //            "} "
    //            "body{ "
    //            "    text-align:center; /*For IE6 Shenanigans*/ "
    //            "} "
    //            "#wrapper{ "
    //            "    width:960px; "
    //            "    margin:0 auto; "
    //            "    text-align:left; "
    //            "}");

    //"body {"
    //"        background: black }"
    //"    section {"
    //"        background: white;"
    //"        color: #00305B;"
    //"        border-radius: 1em;"
    //"        padding: 1em;"
    //"        position: absolute;"
    //"        top: 50%;"
    //"        left: 50%;"
    //"        margin-right: -50%;"
    //"        transform: translate(-50%, -50%) }");


    xmlNodePtr bodyNode = NewTextChild(htmlNode, "body");
    //    bodyNode = NewTextChild(bodyNode, "section"); // Для стиля см выше
    xmlNodePtr tableNode = NewTextChild(bodyNode, "table");
    SetProp(tableNode, "class", "default_color");
    SetProp(tableNode, "cellspacing", 2);
    SetProp(tableNode, "border", 1);
    SetProp(tableNode, "cellpadding", 5);
    SetProp(tableNode, "width", 600);

    TLayout layout;
    if(string("run") == (const char *)reqNode->name) {
        initRequestLayout(reqNode, layout);
        // Строковой параметр м.б. максимум 500, поэтому лог обрезается (
        LEvntPrms params;
        params << PrmSmpl<string>("log", *params_to_str(layout));
        TReqInfo::Instance()->LocaleToLog("EVT.TIMATIC.INFO_MODE", params, ASTRA::evtTimatic);
    } else {
        shared_ptr<TTripInfo> flt = make_shared<TTripInfo>();
        int pax_id = initDocRequestLayout(reqNode, layout, flt);
        CheckIn::TSimplePaxItem pax;
        pax.getByPaxId(pax_id);

        LEvntPrms params;
        params << PrmSmpl<string>("fullname", pax.name + " " + pax.surname);
        params << PrmSmpl<string>("log", *params_to_str(layout));
        TReqInfo::Instance()->LocaleToLog("EVT.TIMATIC.MANUAL_MODE", params, ASTRA::evtPax, flt->point_id,
                pax.reg_no, pax.grp_id);
    }


    TReqParamTab param_tab;
    param_tab.add("Request", layout.reqTypeCaption);
    param_tab.toXML(layout, tableNode);

    TimaticResponseToXML(bodyNode, httpData);

    xml_encode_nodelist(html.docPtr()->children);

    NewTextChild(resNode, "html", StrUtils::b64_encode(GetXMLDocTextOptions(html.docPtr(), xmlSaveOption(XML_SAVE_FORMAT + XML_SAVE_NO_DECL))));
}

}
