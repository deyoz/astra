#include <string>
#include <fstream>
#include "astra_locale.h"
#include "astra_elems.h"
#include "astra_utils.h"
#include "event_utils.h"

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
    return BASIC::DateTimeToStr(date, fmt, (lang == AstraLocale::LANG_EN)?true:false);
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
    return TStagesRules::Instance()->stage_name(stage, airp, (lang == AstraLocale::LANG_EN)?true:false);
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

AstraLocale::LParams LEvntPrms::GetParams (const std::string& lang) {
    AstraLocale::LParams lparams;
    for (std::deque<LEvntPrm*>::iterator iter=begin(); iter != end(); iter++)
        lparams<<(*iter)->GetParam(lang);
    return lparams;
}

int test_event_utils(int argc,char **argv)
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
        "  INSERT INTO locale_messages(id,lang,text,pr_del,tid,pr_term) VALUES(:lexema_id,:lang_ru,:msg_ru,:pr_del,:tid,:pr_term); "
        "  INSERT INTO locale_messages(id,lang,text,pr_del,tid,pr_term) VALUES(:lexema_id,:lang_en,:msg_en,:pr_del,:tid,:pr_term); "
        "end; ";
    Qry.CreateVariable("lang_ru", otString, "RU");
    Qry.CreateVariable("lang_en", otString, "EN");
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
        Qry.SQLText = "INSERT INTO locale_messages(id,lang,text,pr_del,tid,pr_term) VALUES(:lexema_id,:lang_en,:msg_en,:pr_del,:tid,:pr_term)";
        Qry.Execute();
    }
    catch(...) {
        OraSession.Rollback();
        throw;
    }
    OraSession.Commit();
    return 0;
}
