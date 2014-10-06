#include <string>
#include "astra_locale.h"
#include "astra_locale_adv.h"
#include "astra_utils.h"

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
    return BASIC::DateTimeToStr(date, fmt, lang != AstraLocale::LANG_RU);
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

#include "jxtlib/xml_stuff.h"
#include "transfer.h"
#include <set>

int test_astra_locale_adv(int argc,char **argv)
{
    TLogLocale tloglocale;
    tloglocale.lexema_id = "MSG.BAGGAGE.FROM_FLIGHT"; //Baggage from flight [flt%s]
    tloglocale.prms << PrmFlight("flt", "ЮТ", 23, "Д");

    printf("%s\n", AstraLocale::getLocaleText(tloglocale.lexema_id, tloglocale.prms.GetParams(AstraLocale::LANG_RU), AstraLocale::LANG_RU).c_str());
    printf("%s\n", AstraLocale::getLocaleText(tloglocale.lexema_id, tloglocale.prms.GetParams(AstraLocale::LANG_EN), AstraLocale::LANG_EN).c_str());

    TQuery Qry(&OraSession);

    Qry.Clear();
    Qry.SQLText="SELECT system.UTCSYSDATE AS now FROM dual";
    Qry.Execute();
    if (Qry.Eof) throw EXCEPTIONS::Exception("strange situation");
    BASIC::TDateTime date=Qry.FieldAsDateTime("now");

    TLogLocale tloglocale2;
    tloglocale2.lexema_id = "MSG.ARV_TIME_FOR_POINT_NOT_EXISTS"; //Время прилета рейса в пункте [airp%s] не существует [time%s]
    tloglocale2.prms << PrmElem<std::string>("airp", etAirp, "ДМД") << PrmDate("time", date, "hh:nn dd.mm.yy (UTC)");

    printf("%s\n", AstraLocale::getLocaleText(tloglocale2.lexema_id, tloglocale2.prms.GetParams(AstraLocale::LANG_RU), AstraLocale::LANG_RU).c_str());
    printf("%s\n", AstraLocale::getLocaleText(tloglocale2.lexema_id, tloglocale2.prms.GetParams(AstraLocale::LANG_EN), AstraLocale::LANG_EN).c_str());

    TLogLocale tloglocale3;
    tloglocale3.lexema_id = "MSG.ERR_MSG.NOT_ALL_FLIGHTS_ARE_SHOWN"; // [msg%s] Некоторые рейсы не отображаются.
    PrmEnum prmenum("msg", "--");
    prmenum.prms << PrmElem<std::string>("", etAirp, "ДМД") << PrmSmpl<std::string>("", "Внуково") << PrmSmpl<std::string>("", "Пулково") << PrmSmpl<std::string>("", "Шереметьево");
    tloglocale3.prms << prmenum;
    printf("%s\n", AstraLocale::getLocaleText(tloglocale3.lexema_id, tloglocale3.prms.GetParams(AstraLocale::LANG_RU), AstraLocale::LANG_RU).c_str());
    printf("%s\n", AstraLocale::getLocaleText(tloglocale3.lexema_id, tloglocale3.prms.GetParams(AstraLocale::LANG_EN), AstraLocale::LANG_EN).c_str());

    TLogLocale tloglocale4;
//    tloglocale4.lexema_id = "MSG.FLIGHT.CANCELED.REFRESH_DATA"; // Идет отмена вылета рейса
    tloglocale4.lexema_id = "MSG.COMMERCIAL_FLIGHT.AIRLINE.UNKNOWN_CODE"; // Неизвестный код а/к [airline%s] коммерческого рейса [flight%s]
    PrmLexema prmlexema("flight", "MSG.FLIGHT.CHANGED_NAME.REFRESH_DATA");  //   Рейс [flight%s] изменен. Обновите данные
    prmlexema.prms << PrmFlight("flight", "ЮТ", 23, "Д");
    tloglocale4.prms << prmlexema  << PrmSmpl<std::string>("airline", "Трансаэро") /*<< PrmSmpl<std::string>("flight", "")*/;
    printf("%s\n", AstraLocale::getLocaleText(tloglocale4.lexema_id, tloglocale4.prms.GetParams(AstraLocale::LANG_RU), AstraLocale::LANG_RU).c_str());
    printf("%s\n", AstraLocale::getLocaleText(tloglocale4.lexema_id, tloglocale4.prms.GetParams(AstraLocale::LANG_EN), AstraLocale::LANG_EN).c_str());

    TLogLocale tloglocale5;
    tloglocale5.lexema_id = "MSG.STAGE.ACT_TIME_NOT_EXACTLY_DEFINED_FOR_AIRP"; //Фактическое время выполнения шага '[stage%s]' в пункте [airp%s] не определено однозначно
    tloglocale5.prms << PrmElem<std::string>("airp", etAirp, "ДМД") << PrmStage("stage", sOpenCheckIn, "ДМД");
    printf("%s\n", AstraLocale::getLocaleText(tloglocale5.lexema_id, tloglocale5.prms.GetParams(AstraLocale::LANG_RU), AstraLocale::LANG_RU).c_str());
    printf("%s\n", AstraLocale::getLocaleText(tloglocale5.lexema_id, tloglocale5.prms.GetParams(AstraLocale::LANG_EN), AstraLocale::LANG_EN).c_str());

    printf("%s\n", (ElemIdToPrefferedElem(etRight, 810, efmtNameLong, AstraLocale::LANG_EN)).c_str());
    printf("%s\n", (ElemIdToPrefferedElem(etRight, 390, efmtNameLong, AstraLocale::LANG_RU)).c_str());

    std::ifstream fin;
    fin.open("/home/user/work/xml", std::ios::in);
    std::string text, tmp, lexema_id;
    while(getline(fin, tmp))
        text += tmp + "\n";
    XMLDoc doc(text);
    LEvntPrms params;
    xml_decode_nodelist(doc.docPtr()->children);
    xmlNodePtr eventNode = NodeAsNode("/event", doc.docPtr());
    XMLToLocale(eventNode, lexema_id, params);
    printf("%s\n", AstraLocale::getLocaleText(lexema_id, params.GetParams(AstraLocale::LANG_RU), AstraLocale::LANG_RU).c_str());
    printf("%s\n", AstraLocale::getLocaleText(lexema_id, params.GetParams(AstraLocale::LANG_EN), AstraLocale::LANG_EN).c_str());

    std::set<InboundTrfer::TConflictReason> conflicts;
    for (int i = 0; i < 7; i++)
        conflicts.insert(InboundTrfer::TConflictReason(i));
    TLogLocale tlocale;
    ConflictReasonsToLog(conflicts, tlocale);
    return 0;
}

int insert_locales(int argc,char **argv)
{
    std::ifstream fin_loc;
    std::ifstream fin_ru;
    std::ifstream fin_en;
    if (argc != 4)
        throw EXCEPTIONS::Exception("Not enouth arguments");
    fin_loc.open(argv[1], std::ios::in);
    fin_ru.open(argv[2], std::ios::in);
    fin_en.open(argv[3], std::ios::in);
    if(fin_loc.fail() || fin_ru.fail() || fin_en.fail())
        throw EXCEPTIONS::Exception("Can't open file");
    TQuery Qry(&OraSession);
    Qry.SQLText = "SELECT tid__seq.nextval tid from dual";
    Qry.Execute();
    int tid = Qry.FieldAsInteger("tid");
    Qry.Clear();
    Qry.SQLText =
        "begin "
        "  DELETE FROM locale_messages WHERE id=:lexema_id; "
        "  INSERT INTO locale_messages(id,lang,text,pr_del,tid,pr_term) VALUES(:lexema_id,:lang_ru,:msg_ru,:pr_del,:tid,:pr_term); "
        "  INSERT INTO locale_messages(id,lang,text,pr_del,tid,pr_term) VALUES(:lexema_id,:lang_en,:msg_en,:pr_del,:tid,:pr_term); "
        "end; ";
    Qry.CreateVariable("lang_ru", otString, AstraLocale::LANG_RU);
    Qry.CreateVariable("lang_en", otString, AstraLocale::LANG_EN);
    Qry.CreateVariable("tid", otInteger, tid);
    Qry.CreateVariable("pr_del", otInteger, 0);
    Qry.CreateVariable("pr_term", otInteger, 0);
    Qry.DeclareVariable("lexema_id", otString);
    Qry.DeclareVariable("msg_ru", otString);
    Qry.DeclareVariable("msg_en", otString);
    try {
        while(true) {
            char msg_ru[250];
            char msg_en[250];
            std::string lexema_id;
            fin_loc >> lexema_id;
            fin_ru.getline(msg_ru, 250);
            fin_en.getline(msg_en, 250);
            if (fin_loc.eof() || fin_ru.eof() || fin_en.eof()) break;
            Qry.SetVariable("lexema_id", lexema_id);
            Qry.SetVariable("msg_ru", msg_ru);
            Qry.SetVariable("msg_en", msg_en);
            Qry.Execute();
        }
        Qry.Clear();
        Qry.CreateVariable("lexema_id", otString, "м");
        Qry.CreateVariable("lang_en", otString, "EN");
        Qry.CreateVariable("msg_en", otString, "pc");
        Qry.CreateVariable("tid", otInteger, tid);
        Qry.CreateVariable("pr_del", otInteger, 0);
        Qry.CreateVariable("pr_term", otInteger, 0);
        Qry.SQLText =
          "begin "
          "  DELETE FROM locale_messages WHERE id=:lexema_id; "
          "  INSERT INTO locale_messages(id,lang,text,pr_del,tid,pr_term) VALUES(:lexema_id,:lang_en,:msg_en,:pr_del,:tid,:pr_term); "
          "end; ";
        Qry.Execute();
    }
    catch(...) {
        OraSession.Rollback();
        throw;
    }
    OraSession.Commit();
    return 0;
}

void LocaleToXML (xmlNodePtr parent, const std::string& lexema_id, const LEvntPrms& params)
{
    xmlNodePtr eventNode = NewTextChild(parent,"event");
    NewTextChild(eventNode,"lexema_id", lexema_id);
    params.toXML(eventNode);
}

void XMLToLocale (const xmlNodePtr eventNode, std::string& lexema_id, LEvntPrms& params)
{
    lexema_id = NodeAsString("lexema_id", eventNode);
    xmlNodePtr paramsNode = NodeAsNode("params", eventNode);
    params.fromXML(paramsNode);
}
