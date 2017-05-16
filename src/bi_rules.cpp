#include "bi_rules.h"
#define NICKNAME "DENIS"
#include "serverlib/slogger.h"
#include "qrys.h"
#include "dev_utils.h"
#include "etick.h"
#include "term_version.h"
#include "brands.h"

using namespace std;
using namespace ASTRA;
using namespace EXCEPTIONS;

namespace BIPrintRules {

    const TPrintTypes& PrintTypes()
    {
      static TPrintTypes printTypes;
      return printTypes;
    }
    const TPrintTypesView& PrintTypesView()
    {
      static TPrintTypesView printTypesView;
      return printTypesView;
    }

    void TRule::dump(const string &file = "", int line = NoExists) const
    {
        LogTrace(TRACE5) << "-------TRule::dump(): " << file << ":" << (line == NoExists ? 0 : line) << "-------";
        LogTrace(TRACE5) << "id: " << id;
        ostringstream buf;
        for(THallsList::const_iterator i = halls.begin(); i != halls.end(); i++)
            buf << "<" << i->first << ", " << i->second << ">";
        LogTrace(TRACE5) << "halls: [" << buf.str() << "]";
        LogTrace(TRACE5) << "curr_hall: " << curr_hall.first << " -> " << curr_hall.second;
        LogTrace(TRACE5) << "pr_print_bi: " << pr_print_bi;
        LogTrace(TRACE5) << "print_type: " << print_type;
        LogTrace(TRACE5) << "---------------------------";
    }

    void get_rule(
            const string &airline,
            const string &tier_level,
            const string &cls,
            const string &subcls,
            const string &rem_code,
            const string &brand,
            const string &fqt_airline,
            const string &aircode,
            TRule &rule
            )
    {
        TCachedQuery Qry(
                "select "
                "    id, "
                "    print_type, "
                "    decode(class, null, 0, 1) + "
                "    decode(subclass, null, 0, 2) + "
                "    decode(brand_code, null, 0, 3) + "
                "    decode(fqt_airline, null, 0, 4) + "
                "    decode(fqt_tier_level, null, 0, 5) + "
                "    decode(aircode, null, 0, 6) + "
                "    decode(rem_code, null, 0, 7) "
                "    priority "
                "from "
                "    bi_print_rules "
                "where "
                "    airline = :airline and "
                "    pr_denial = 0 and "
                "    (class is null or class = :class) and "
                "    (subclass is null or subclass = :subclass) and "
                "    (brand_code is null or brand_code = :brand) and "
                "    (fqt_airline is null or fqt_airline = :fqt_airline) and "
                "    (fqt_tier_level is null or fqt_tier_level = :tier_level) and "
                "    (aircode is null or aircode = :aircode) and "
                "    (rem_code is null or rem_code = :rem_code) "
                "order by "
                "    priority desc ",
                QParams()
                << QParam("airline", otString, airline)
                << QParam("class", otString, cls)
                << QParam("subclass", otString, subcls)
                << QParam("brand", otString, subcls)
                << QParam("fqt_airline", otString, fqt_airline)
                << QParam("tier_level", otString, tier_level)
                << QParam("aircode", otString, aircode)
                << QParam("rem_code", otString, rem_code)
                );
        Qry.get().Execute();
        if(not Qry.get().Eof) {
            rule.id = Qry.get().FieldAsInteger("id");
            rule.print_type = PrintTypes().decode(Qry.get().FieldAsString("print_type"));
        }
    }

    bool bi_airline_service(
            const TTripInfo &info,
            TDevOperType op_type,
            TRule &rule
            )
    {
        if(op_type != dotPrnBI and op_type != dotPrnBP) return false;
        rule.pr_print_bi = op_type == dotPrnBI;

        TCachedQuery Qry(
                "select "
                "   terminal, "
                "   hall, "
                "   pr_print_bi "
                "from "
                "   bi_airline_service "
                "where "
                "   airline = :airline and "
                "   airp = :airp and "
                "   pr_denial = 0 "
                "order by "
                "   hall nulls first "
                ,
                QParams()
                << QParam("airline", otString, info.airline)
                << QParam("airp", otString, info.airp)
                );
        Qry.get().Execute();
        bool result = not Qry.get().Eof;
        if(result) {
            for(; not Qry.get().Eof; Qry.get().Next()) {
                int terminal = Qry.get().FieldAsInteger("terminal");
                int hall = NoExists;
                if(not Qry.get().FieldIsNULL("hall"))
                    hall = Qry.get().FieldAsInteger("hall");
                bool pr_print_bi = Qry.get().FieldAsInteger("pr_print_bi") != 0;

                LogTrace(TRACE5) << "terminal: " << terminal;
                LogTrace(TRACE5) << "hall: " << hall;
                LogTrace(TRACE5) << "pr_print_bi: " << pr_print_bi;
                LogTrace(TRACE5) << "rule.pr_print_bi: " << rule.pr_print_bi;

                if(hall == NoExists) {
                    if(pr_print_bi == rule.pr_print_bi) {
                        // Если бизнес зал не задан, достаем все залы данного терминала.
                        TCachedQuery hallsQry("select id from bi_halls where terminal = :terminal",
                                QParams() << QParam("terminal", otInteger, Qry.get().FieldAsInteger("terminal")));
                        hallsQry.get().Execute();
                        for(; not hallsQry.get().Eof; hallsQry.get().Next())
                            rule.halls[hallsQry.get().FieldAsInteger("id")] = terminal;
                    }
                } else if(pr_print_bi != rule.pr_print_bi) {
                    rule.halls.erase(hall);
                } else
                    rule.halls[hall] = terminal;
            }
            result = not rule.halls.empty();
        }
        return result;
    }

    bool TRule::tags_enabled(TDevOperType op_type, bool first_seg) const
    {
        return
            exists() and                // правило существует
            (not pr_print_bi or         // Выключен признак Биз. пригл. в таблице Обслуживание АК в биз. залах
             op_type == dotPrnBI) and   // или тек. операция - печать приглашений
            first_seg;                  // тек. пакс из первого сегмента
    }

    bool Holder::select(xmlNodePtr reqNode)
    {
        xmlNodePtr currNode = reqNode->children;
        currNode = GetNodeFast("halls", currNode);

        if(not currNode) return true;

        bool result = true;
        currNode = currNode->children;
        // Пробег по пришедшим залам
        // Если хотя бы один не найден в правилах,
        // Правила не меняем.
        for(; currNode; currNode = currNode->next) {
            xmlNodePtr dataNode = currNode->children;
            int pax_id = NodeAsIntegerFast("pax_id", dataNode);
            int hall_id = NodeAsIntegerFast("hall_id", dataNode);

            TPaxList::iterator rule = items.find(pax_id);
            if(rule == items.end()) {
                result = false;
                break;
            }

            TRule::THallsList::iterator iHall = rule->second.halls.find(hall_id);
            if(iHall != rule->second.halls.end())
                rule->second.curr_hall = *iHall;
            else {
                result = false;
                break;
            }

        }
        if(result) {
            for(TPaxList::iterator i = items.begin(); i != items.end(); i++) {
                if(i->second.curr_hall.first != NoExists) {
                    i->second.halls.clear();
                    i->second.halls.insert(i->second.curr_hall);
                }
            }
        }
        return result;
    }

    int Holder::get_hall_id(TDevOperType op_type, int pax_id)
    {
        TCachedQuery Qry(
                "select confirm_print.hall_id "
                "FROM confirm_print, "
                "     (SELECT MAX(time_print) AS time_print FROM confirm_print WHERE pax_id=:pax_id and voucher is null AND pr_print<>0 and " OP_TYPE_COND("op_type")") a "
                "WHERE confirm_print.time_print=a.time_print AND confirm_print.pax_id=:pax_id AND voucher is null and "
                "      " OP_TYPE_COND("confirm_print.op_type"),
                QParams() << QParam("pax_id", otInteger, pax_id) << QParam("op_type", otString, EncodeDevOperType(op_type))
                );
        Qry.get().Execute();
        int result = NoExists;
        if(not Qry.get().Eof)
            result = Qry.get().FieldAsInteger("hall_id");
        return result;
    }

    void Holder::toXML(TDevOperType op_type, xmlNodePtr resNode)
    {
        xmlNodePtr paxListNode = NULL;
        for(TPaxList::const_iterator i = items.begin(); i != items.end(); i++) {
            if(not i->second.pr_get) continue;
            if(i->second.exists() and i->second.halls.size() > 1) {
                if(not paxListNode) {
                    xmlNodePtr printNode = NewTextChild(NewTextChild(resNode, "data"), "print");
                    SetProp(printNode, "halls");
                    paxListNode = NewTextChild(printNode, "passengers");
                }
                xmlNodePtr itemNode = NewTextChild(paxListNode, "passenger");
                NewTextChild(itemNode, "pax_id", i->first);
                NewTextChild(itemNode, "hall_id", get_hall_id(op_type, i->first), NoExists); // last printed hall
                xmlNodePtr hallsNode = NewTextChild(itemNode, "halls");
                for(TRule::THallsList::const_iterator iHall = i->second.halls.begin(); iHall != i->second.halls.end(); iHall++) {
                    xmlNodePtr iHallNode = NewTextChild(hallsNode, "item");
                    NewTextChild(iHallNode, "id", iHall->first);
                    NewTextChild(iHallNode, "name", (string)
                            "[" + ElemIdToNameLong(etAirpTerminal, iHall->second) + "] " +
                            ElemIdToNameLong(etBIHall, iHall->first));
                }
            }
        }
    }

    bool Holder::complete() const
    {
        bool result = true;
        for(TPaxList::const_iterator i = items.begin(); i != items.end(); i++) {
            if(not i->second.pr_get) continue;
            if(not i->second.exists()) continue;
            result = i->second.halls.size() == 1;
            if(not result) break;
        }
        return result;
    }

    void Holder::dump(const string &file, int line) const
    {
        LogTrace(TRACE5) << "-----BIPrintRules::Holder::dump: " << file << ":" << (line == NoExists ? 0 : line) << "-----";
        LogTrace(TRACE5) << "items.size(): " << items.size();
        for(TPaxList::const_iterator i = items.begin(); i != items.end(); i++) {
            LogTrace(TRACE5) << "pax_id: " << i->first;
            i->second.dump();
        }
        LogTrace(TRACE5) << "------------------------------------";
    }

    const TRule &Holder::get(int grp_id, int pax_id)
    {
        TPaxList::iterator iPax = items.find(pax_id);
        if(iPax == items.end()) {
            if(grp_id == NoExists) {
                TCachedQuery grpQry("select grp_id from pax where pax_id = :pax_id",
                        QParams() << QParam("pax_id", otInteger, pax_id));
                grpQry.get().Execute();
                if(grpQry.get().Eof)
                    throw Exception("bi_rules: grp_id not found for pax_id: %d", pax_id);
                grp_id = grpQry.get().FieldAsInteger("grp_id");
            }
            if(grps.find(grp_id) == grps.end()) { // Rules for this group not queried yet
                getByGrpId(grp_id);
                grps.insert(grp_id);
            }
            iPax = items.find(pax_id);
            if(iPax == items.end())
                throw Exception("BIPrintRules::Holder: rule not defined for pax_id %d", pax_id);
        }
        iPax->second.pr_get = true;
        LogTrace(TRACE5) << "rule to return";
        iPax->second.dump();
        return iPax->second;
    }

    void Holder::getByGrpId(int grp_id)
    {
        TCachedQuery fltQry("select points.* from points, pax_grp where pax_grp.grp_id = :grp_id and pax_grp.point_dep = points.point_id",
                QParams() << QParam("grp_id", otInteger, grp_id));
        fltQry.get().Execute();
        TTripInfo t(fltQry.get());

        TCachedQuery paxQry(
                "select "
                "   pax.pax_id, "
                "   pax_grp.class, "
                "   pax.subclass "
                "from "
                "   pax, "
                "   pax_grp "
                "where "
                "   pax_grp.grp_id = :grp_id and "
                "   pax.grp_id = pax_grp.grp_id ",
                QParams() << QParam("grp_id", otInteger, grp_id));
        paxQry.get().Execute();
        for(; not paxQry.get().Eof; paxQry.get().Next()) {
            int pax_id = paxQry.get().FieldAsInteger("pax_id");
            string cls = paxQry.get().FieldAsString("class");
            string subcls = paxQry.get().FieldAsString("subclass");
            TRule bi_rule;
            // Достаем данные из кеша Обслуживание авиакомпаний в аэропортах
            if(bi_airline_service(t, op_type, bi_rule)) {
                // на данном этапе в bi_rule определены:
                // Список залов     (bi_rule.halls)
                // Биз. пригл.      (bi_rule.pr_print_bi)

                // Достаем бренды
                TBrands brands;
                brands.get(pax_id);
                // Если не найдено ни одного бренда, добавляем пустой, чтобы get_rule все-таки отработала
                if(brands.items.empty()) brands.items.push_back(NoExists);

                // Достаем ремарки
                set<CheckIn::TPaxFQTItem> fqts;
                CheckIn::LoadPaxFQT(pax_id, fqts);
                // Если не найдено ни одной ремарки, добавляем пустую, чтобы get_rule все-таки отработала
                if(fqts.empty()) fqts.insert(CheckIn::TPaxFQTItem());

                // Достаем номер билета и из него расч. код (первые 3 символа)
                string aircode;
                CheckIn::TPaxTknItem tkn;
                if(CheckIn::LoadPaxTkn(pax_id, tkn)) {
                    aircode = tkn.no.substr(0, 3);
                }

                // Пробег по брендам и ремаркам
                // у паса может быть несколько ремарок с разными
                // настройками группы регистрации (bi_print_rules.print_type = ALL, TWO, ONE)
                // выбираем самую приоритетную.

                BIPrintRules::TRule tmp_rule = bi_rule; // чтобы не потерять hall, is_business_hall, pr_print_bi
                for(set<CheckIn::TPaxFQTItem>::iterator iFqt = fqts.begin(); iFqt != fqts.end(); ++iFqt)
                    for(TBrands::TItems::iterator iBrand = brands.items.begin(); iBrand != brands.items.end(); ++iBrand) {
                        BIPrintRules::get_rule(
                                t.airline,
                                iFqt->tier_level,
                                cls,
                                subcls,
                                iFqt->rem,
                                ElemIdToCodeNative(etBrand, *iBrand),
                                iFqt->airline,
                                aircode,
                                tmp_rule
                                );

                        // После нахождения правила из кеша Правила печати приглашений
                        // данные этого правила сохраняются в bi_rule:
                        // Группа регистрации (bi_rule.print_type)
                        // Оформление (bi_rule.pr_issue)

                        if(tmp_rule.exists()) {
                            if(not bi_rule.exists())
                                bi_rule = tmp_rule;
                            else {
                                // вот здесь выбор по приоритету
                                if(bi_rule.print_type < tmp_rule.print_type)
                                    bi_rule = tmp_rule;
                            }
                        }
                    }
            }
            items.insert(make_pair(pax_id, bi_rule));
        }
        // Находим первого паса с печатью для всей группы (Группа регистрации ДА т.е. bi_rule.print_type = All)
        TPaxList::iterator grpPax = items.begin();
        for(; grpPax!=items.end() and grpPax->second.print_type != BIPrintRules::TPrintType::All; ++grpPax );
        // Применяем групповое правило для всех пасов, если таковое нашлось
        if(grpPax != items.end()) {
            for (TPaxList::iterator iPax=items.begin(); iPax!=items.end(); ++iPax )
                iPax->second = grpPax->second;
        }
    }
}

void TPrPrint::dump(const string &file, int line)
{
    // Это просто дамп того, что получилось
    LogTrace(TRACE5) << "--------matrix: " << file << ":" << line << " --------";
    for(TPaxMap::iterator iPax = paxs.begin(); iPax != paxs.end(); iPax++) {
        for(TPaxPrint::iterator iPrn = iPax->second.begin(); iPrn != iPax->second.end(); iPrn++) {
            LogTrace(TRACE5) << "matrix[" << iPax->first << "][" << iPrn->first << "] = " << iPrn->second;
        }
    }
}

bool TPrPrint::get_pr_print(int pax_id)
{
    bool result = false;
    // Осталось найти текущего
    for(TPaxMap::iterator iPax = paxs.begin(); iPax != paxs.end(); iPax++) {
        TPaxPrint::iterator iPrn = iPax->second.begin();
        for(; iPrn != iPax->second.end(); iPrn++) {
            if(iPrn->first == pax_id)
                break;
        }
        if(iPrn != iPax->second.end()) {
            result = iPrn->second;
            break;
        }
    }
    return result;
}

void TPrPrint::fromDB(int grp_id, int pax_id, TQuery &Qry)
{
    // А вот теперь определение pr_bi_print
    TCachedQuery grpQry("select pax_id from pax where grp_id = :grp_id",
            QParams() << QParam("grp_id", otInteger, grp_id));
    grpQry.get().Execute();

    Qry.SetVariable("op_type", EncodeDevOperType(dotPrnBI));

    for(; not grpQry.get().Eof; grpQry.get().Next()) {
        int grp_pax_id = grpQry.get().FieldAsInteger("pax_id");

        map<int, CheckIn::TCkinPaxTknItem> pax_list;
        CheckIn::GetTCkinTickets(grp_pax_id, pax_list);

        if(pax_list.empty()) {
            Qry.SetVariable("pax_id", grp_pax_id);
            Qry.Execute();
            paxs[1][grp_pax_id] = not Qry.Eof;
            grps.insert(grp_id);
        } else {
            for(map<int, CheckIn::TCkinPaxTknItem>::iterator i = pax_list.begin(); i != pax_list.end(); i++) {
                bool pr_print;
                if(i->first == 1) { // первый сегмент
                    Qry.SetVariable("pax_id", i->second.pax_id);
                    Qry.Execute();
                    pr_print = not Qry.Eof;
                } else { // остальные
                    pr_print = true;
                }
                paxs[i->first][i->second.pax_id] = pr_print;
                grps.insert(i->second.grp_id);
            }
        }
    }

    // определяем печать на первом сегменте
    bool pr_bi_first = false;
    for(TPaxPrint::iterator i = paxs[1].begin(); i != paxs[1].end(); i++) {
        pr_bi_first = pr_bi_first or i->second;
    }

    // Здесь есть одна тонкая тонкость
    // Если была хоть одна печать на первом сегменте,
    // надо пробежать по неотпечатанным
    // и выставить true для всех, у которых приглашения вообще отключены
    // чтобы терминал не задавал вопрос "Отпечатать все неотпечатанные?"
    if(pr_bi_first)
        for(TPaxPrint::iterator i = paxs[1].begin(); i != paxs[1].end(); i++) {
            if(not i->second) { // Не было печати
                const BIPrintRules::TRule &bi_rule = bi_rules.get(NoExists, i->first);
                i->second = not bi_rule.exists() or not bi_rule.pr_print_bi;
            }
        }

    // ставим признак для всех остальных
    for(TPaxMap::iterator iPax = paxs.begin(); iPax != paxs.end(); iPax++) {
        if(iPax->first == 1) continue;
        for(TPaxPrint::iterator iPrn = iPax->second.begin(); iPrn != iPax->second.end(); iPrn++) {
            iPrn->second = pr_bi_first;
        }
    }
}

void TPrPrint::get_pr_print(int grp_id, int pax_id, bool &pr_bp_print, bool &pr_bi_print)
{
    TCachedQuery Qry(
            "SELECT pax_id FROM confirm_print WHERE pax_id=:pax_id and voucher is null AND pr_print<>0 AND rownum=1 and " OP_TYPE_COND("op_type"),
            QParams() << QParam("pax_id", otInteger, pax_id) << QParam("op_type", otString, EncodeDevOperType(dotPrnBP))
            );
    Qry.get().Execute();
    pr_bp_print = not Qry.get().Eof;

    if(TReqInfo::Instance()->desk.compatible(OP_TYPE_VERSION)) {
        if(paxs.empty()) { // первый вызов
            fromDB(grp_id, pax_id, Qry.get());
        } else if(grps.find(grp_id) == grps.end()) {
            // Все след. вызовы должны быть с правильным grp_id
            // т.е. с таким, который принадлежит группе пассажиров,
            // сформированной в первый вызов (grps.size() > 1 в сл. сквозняка)
            throw Exception("%s: unexpected grp_id: %d", __FUNCTION__, grp_id);
        }
        pr_bi_print = get_pr_print(pax_id);
    } else
        pr_bi_print = false;
}

