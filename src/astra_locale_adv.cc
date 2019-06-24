#include <string>
#include "astra_locale.h"
#include "astra_locale_adv.h"
#include "astra_utils.h"

using namespace BASIC::date_time;

template<> std::string PrmSmpl<int>::GetMsg (const std::string& lang) const {
    return IntToString(param);
}

template<> std::string PrmSmpl<double>::GetMsg (const std::string& lang) const {
    return FloatToString(param, 2);
}

template<> std::string PrmSmpl<std::string>::GetMsg (const std::string& lang) const {
    return param;
}

AstraLocale::LParam PrmDate::GetParam (const std::string& lang) const {
    return AstraLocale::LParam(name, GetMsg(lang));
}

std::string PrmDate::GetMsg (const std::string& lang) const {
    return DateTimeToStr(date, fmt, lang != AstraLocale::LANG_RU);
}

LEvntPrm* PrmDate::MakeCopy () const {
    return new PrmDate(*this);
}

AstraLocale::LParam PrmBool::GetParam (const std::string& lang) const {
    return AstraLocale::LParam(name, GetMsg(lang));
}

std::string PrmBool::GetMsg (const std::string& lang) const {
    return AstraLocale::getLocaleText(val?"да":"нет", lang);
}

LEvntPrm* PrmBool::MakeCopy () const {
    return new PrmBool(*this);
}

AstraLocale::LParam PrmEnum::GetParam (const std::string& lang) const {
    return AstraLocale::LParam(name, GetMsg(lang));
}

std::string PrmEnum::GetMsg (const std::string& lang) const {
    std::ostringstream msg;
    for (std::deque<LEvntPrm*>::const_iterator iter = prms.begin(); iter != prms.end(); iter++) {
        if (iter!=prms.begin()) msg << separator;
        msg << (*iter)->GetMsg(lang);
    }
    return msg.str();
}

LEvntPrm* PrmEnum::MakeCopy () const {
    return new PrmEnum(*this);
}

AstraLocale::LParam PrmFlight::GetParam (const std::string& lang) const {
    return AstraLocale::LParam(name, GetMsg(lang));
}

std::string PrmFlight::GetMsg (const std::string& lang) const {
    std::ostringstream msg;
    msg << PrmElem<std::string>("", etAirline, airline, efmtCodeNative).GetMsg(lang) << std::setw(3)<< std::setfill('0')
        << flt_no << PrmElem<std::string>("", etSuffix, suffix).GetMsg(lang);
    return msg.str();
}

LEvntPrm* PrmFlight::MakeCopy () const {
    return new PrmFlight(*this);
}

AstraLocale::LParam PrmLexema::GetParam (const std::string& lang) const {
    return AstraLocale::LParam(name, GetMsg(lang));
}

std::string PrmLexema::GetMsg (const std::string& lang) const {
    AstraLocale::LParams lparams;
    for (std::deque<LEvntPrm*>::const_iterator iter = prms.begin(); iter != prms.end(); iter++)
        lparams<<(*iter)->GetParam(lang);
    return AstraLocale::getLocaleText(lexema_id, lparams, lang);
}

LEvntPrm* PrmLexema::MakeCopy () const {
    return new PrmLexema(*this);
}

AstraLocale::LParam PrmStage::GetParam (const std::string& lang) const {
    return AstraLocale::LParam(name, GetMsg(lang));
}

std::string PrmStage::GetMsg (const std::string& lang) const {
    return TStagesRules::Instance()->stage_name(stage, airp, lang != AstraLocale::LANG_RU);
}

LEvntPrm* PrmStage::MakeCopy () const {
    return new PrmStage(*this);
}

LEvntPrms& LEvntPrms::operator = (const LEvntPrms& params) {
   for (std::deque<LEvntPrm*>::const_iterator iter=params.begin(); iter != params.end(); iter++)
       push_back((*iter)->MakeCopy());
   return *this;
}

LEvntPrms& LEvntPrms::operator << (const PrmElem<int>&  prmelem) {
    push_back(new PrmElem<int>(prmelem));
    return *this;
}

LEvntPrms& LEvntPrms::operator << (const PrmElem<std::string>&  prmelem) {
    push_back(new PrmElem<std::string>(prmelem));
    return *this;
}

LEvntPrms& LEvntPrms::operator << (const PrmSmpl<int>&  prmsmpl) {
    push_back(new PrmSmpl<int>(prmsmpl));
    return *this;
}

LEvntPrms& LEvntPrms::operator << (const PrmSmpl<double>&  prmsmpl) {
    push_back(new PrmSmpl<double>(prmsmpl));
    return *this;
}

LEvntPrms& LEvntPrms::operator << (const PrmSmpl<std::string>&  prmsmpl) {
    push_back(new PrmSmpl<std::string>(prmsmpl));
    return *this;
}

LEvntPrms& LEvntPrms::operator << (const PrmDate&  prmdate) {
    push_back(new PrmDate(prmdate));
    return *this;
}

LEvntPrms& LEvntPrms::operator << (const PrmBool&  prmbool) {
    push_back(new PrmBool(prmbool));
    return *this;
}

LEvntPrms& LEvntPrms::operator << (const PrmEnum&  prmenum) {
    push_back(new PrmEnum(prmenum));
    return *this;
}

LEvntPrms& LEvntPrms::operator << (const PrmLexema&  prmlexema) {
    push_back(new PrmLexema(prmlexema));
    return *this;
}

LEvntPrms& LEvntPrms::operator << (const PrmFlight&  prmflight) {
    push_back(new PrmFlight(prmflight));
    return *this;
}

LEvntPrms& LEvntPrms::operator << (const PrmStage&  prmstage) {
    push_back(new PrmStage(prmstage));
    return *this;
}

AstraLocale::LParams LEvntPrms::GetParams (const std::string& lang) const {
    AstraLocale::LParams lparams;
    for (std::deque<LEvntPrm*>::const_iterator iter=begin(); iter != end(); iter++)
        lparams<<(*iter)->GetParam(lang);
    return lparams;
}

void LEvntPrms::toXML(xmlNodePtr eventNode) const {
    xmlNodePtr paramsNode = NewTextChild(eventNode,"params");
    for (std::deque<LEvntPrm*>::const_iterator iter=begin(); iter != end(); iter++) {
        xmlNodePtr paramNode = NewTextChild(paramsNode,"param");
        (*iter)->ParamToXML(paramNode);
    }
}

void LEvntPrms::fromXML(xmlNodePtr paramsNode) {
    for(xmlNodePtr paramNode = paramsNode->children; paramNode != NULL; paramNode = paramNode->next) {
        std::string type = NodeAsString("type", paramNode);
        if(type == (std::string("PrmSmpl<") + typeid(int).name() + std::string(">"))) {
            std::string name = NodeAsString("name", paramNode);
            int value = NodeAsInteger("value", paramNode);
            push_back(new PrmSmpl<int>(name, value));
        }
        else if(type == (std::string("PrmSmpl<") + typeid(std::string).name() + std::string(">"))) {
            std::string name = NodeAsString("name", paramNode);
            std::string value = NodeAsString("value", paramNode);
            push_back(new PrmSmpl<std::string>(name, value));
        }
        else if(type == (std::string("PrmSmpl<") + typeid(double).name() + std::string(">"))) {
            std::string name = NodeAsString("name", paramNode);
            double value = NodeAsFloat("value", paramNode);
            push_back(new PrmSmpl<double>(name, value));
        }
        else if(type == (std::string("PrmElem<") + typeid(int).name() + std::string(">"))) {
            std::string name = NodeAsString("name", paramNode);
            int elem_type = NodeAsInteger("elem_type", paramNode);
            int id = NodeAsInteger("id", paramNode);
            int fmt = NodeAsInteger("fmt", paramNode);
            push_back(new PrmElem<int>(name, (TElemType)elem_type, id, (TElemFmt)fmt));
        }
        else if(type == (std::string("PrmElem<") + typeid(std::string).name() + std::string(">"))) {
            std::string name = NodeAsString("name", paramNode);
            int elem_type = NodeAsInteger("elem_type", paramNode);
            std::string id = NodeAsString("id", paramNode);
            int fmt = NodeAsInteger("fmt", paramNode);
            push_back(new PrmElem<std::string>(name, (TElemType)elem_type, id, (TElemFmt)fmt));
        }
        else if(type == "PrmLexema") {
            std::string name = NodeAsString("name", paramNode);
            std::string id = NodeAsString("lexema_id", paramNode);
            xmlNodePtr paramsNode = NodeAsNode("params", paramNode);
            PrmLexema lexema(name, id);
            lexema.prms.fromXML(paramsNode);
            push_back(new PrmLexema(lexema));
        }
        else if(type == "PrmEnum") {
            std::string name = NodeAsString("name", paramNode);
            std::string separator = NodeAsString("separator", paramNode);
            xmlNodePtr paramsNode = NodeAsNode("params", paramNode);
            PrmEnum prmenum(name, separator);
            prmenum.prms.fromXML(paramsNode);
            push_back(new PrmEnum(prmenum));
        }
        else if(type == "PrmBool") {
            std::string name = NodeAsString("name", paramNode);
            int val = NodeAsInteger("val", paramNode);
            push_back(new PrmBool(name, val));
        }
        else if(type == "PrmStage") {
            std::string name = NodeAsString("name", paramNode);
            int stage = NodeAsInteger("stage", paramNode);
            std::string airp = NodeAsString("airp", paramNode);
            push_back(new PrmStage(name, (TStage)stage, airp));
        }
        else if(type == "PrmFlight") {
            std::string name = NodeAsString("name", paramNode);
            std::string airline = NodeAsString("airline", paramNode);
            int flt_no = NodeAsInteger("flt_no", paramNode);
            std::string suffix = NodeAsString("suffix", paramNode);
            push_back(new PrmFlight(name, airline, flt_no, suffix));
        }
        else if(type == "PrmDate") {
            std::string name = NodeAsString("name", paramNode);
            double date = NodeAsFloat("date", paramNode);
            std::string  fmt = NodeAsString("fmt", paramNode);
            push_back(new PrmDate(name, date, fmt));
        }
    }
}

void LocaleToXML (const xmlNodePtr node, const std::string& lexema_id, const LEvntPrms& params)
{
    if (node==NULL) return;
    NewTextChild(node,"lexema_id", lexema_id);
    params.toXML(node);
}

void LocaleFromXML (const xmlNodePtr node, std::string& lexema_id, LEvntPrms& params)
{
    if (node==NULL) return;
    lexema_id = NodeAsString("lexema_id", node);
    xmlNodePtr paramsNode = NodeAsNode("params", node);
    params.fromXML(paramsNode);
}
