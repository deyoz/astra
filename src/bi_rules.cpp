#include "bi_rules.h"
#include "qrys.h"
#include "dev_utils.h"
#include "etick.h"
#include "term_version.h"
#include "brands.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

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
            const string &rfisc,
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
                "    decode(rem_code, null, 0, 7) + "
                "    decode(rfisc, null, 0, 8) "
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
                "    (rfisc is null or rfisc = :rfisc) and "
                "    (rem_code is null or rem_code = :rem_code) "
                "order by "
                "    priority desc ",
                QParams()
                << QParam("airline", otString, airline)
                << QParam("class", otString, cls)
                << QParam("subclass", otString, subcls)
                << QParam("brand", otString, brand)
                << QParam("fqt_airline", otString, fqt_airline)
                << QParam("tier_level", otString, tier_level)
                << QParam("aircode", otString, aircode)
                << QParam("rfisc", otString, rfisc)
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
            TDevOper::Enum op_type,
            TRule &rule
            )
    {
        if(op_type != TDevOper::PrnBI and op_type != TDevOper::PrnBP) return false;
        rule.pr_print_bi = op_type == TDevOper::PrnBI;

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
                        // �᫨ ������ ��� �� �����, ���⠥� �� ���� ������� �ନ����.
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

    bool TRule::tags_enabled(TDevOper::Enum op_type, bool first_seg) const
    {
        return
            exists() and                // �ࠢ��� �������
            (not pr_print_bi or         // �몫�祭 �ਧ��� ���. �ਣ�. � ⠡��� ���㦨����� �� � ���. �����
             op_type == TDevOper::PrnBI) and   // ��� ⥪. ������ - ����� �ਣ��襭��
            first_seg;                  // ⥪. ���� �� ��ࢮ�� ᥣ����
    }

    bool Holder::select(xmlNodePtr reqNode)
    {
        xmlNodePtr currNode = reqNode->children;
        currNode = GetNodeFast("halls", currNode);

        if(not currNode) return true;

        bool result = true;
        currNode = currNode->children;
        // �஡�� �� ��襤訬 �����
        // �᫨ ��� �� ���� �� ������ � �ࠢ����,
        // �ࠢ��� �� ���塞.
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

    int Holder::get_hall_id(TDevOper::Enum op_type, int pax_id)
    {
        TCachedQuery Qry(
                "select confirm_print.hall_id "
                "FROM confirm_print, "
                "     (SELECT MAX(time_print) AS time_print FROM confirm_print WHERE pax_id=:pax_id and voucher is null AND pr_print<>0 and " OP_TYPE_COND("op_type")") a "
                "WHERE confirm_print.time_print=a.time_print AND confirm_print.pax_id=:pax_id AND voucher is null and "
                "      " OP_TYPE_COND("confirm_print.op_type"),
                QParams() << QParam("pax_id", otInteger, pax_id) << QParam("op_type", otString, DevOperTypes().encode(op_type))
                );
        Qry.get().Execute();
        int result = NoExists;
        if(not Qry.get().Eof)
            result = Qry.get().FieldAsInteger("hall_id");
        return result;
    }

    void Holder::toXML(TDevOper::Enum op_type, xmlNodePtr resNode)
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

    void Holder::toStat(int grp_id, int pax_id, TDateTime time_print)
    {
        if(grp_id < 0 || pax_id < 0) return;
        const BIPrintRules::TRule &bi_rule = get(grp_id, pax_id);
        if(bi_rule.exists() and not bi_rule.halls.empty()) {
            TTripInfo info;
            if(info.getByGrpId(grp_id)) {
                QParams QryParams;
                QryParams
                    << QParam("point_id", otInteger, info.point_id)
                    << QParam("scd_out", otDate, info.scd_out)
                    << QParam("pax_id", otInteger, pax_id)
                    << QParam("print_type", otString, TPrintTypes().encode(bi_rule.print_type))
                    << QParam("op_type", otString, DevOperTypes().encode(op_type))
                    << QParam("hall", otInteger, bi_rule.halls.begin()->first)
                    << QParam("time_print", otDate, time_print)
                    << QParam("desk", otString, TReqInfo::Instance()->desk.code);
                if(bi_rule.halls.begin()->second != 0)
                    QryParams << QParam("terminal", otInteger, bi_rule.halls.begin()->second);
                else
                    QryParams << QParam("terminal", otInteger, FNull);
                DB::TCachedQuery Qry(
                      PgOra::getRWSession("BI_STAT"),
                      "INSERT INTO bi_stat ( "
                      "   point_id, "
                      "   scd_out, "
                      "   pax_id, "
                      "   print_type, "
                      "   terminal, "
                      "   hall, "
                      "   op_type, "
                      "   pr_print, "
                      "   time_print, "
                      "   desk "
                      ") VALUES ( "
                      "   :point_id, "
                      "   :scd_out, "
                      "   :pax_id, "
                      "   :print_type, "
                      "   :terminal, "
                      "   :hall, "
                      "   :op_type, "
                      "   0, "
                      "   :time_print, "
                      "   :desk "
                      ") ",
                      QryParams);
                try {
                    Qry.get().Execute();
                } catch(const EOracleError &E) {
                    if(E.Code != 1) throw; // ignore unique constraint violated
                }
            }
        }
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
                // �᫨ ��⮢� ����, ������ ���⮥ �ࠢ���
                if(isTestPaxId(grp_id))
                    items.insert(make_pair(pax_id, TRule()));
                else {
                    getByGrpId(grp_id);
                    grps.insert(grp_id);
                }
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

    void get_rfisc(int grp_id, int pax_id, set<string> &rfisc)
    {
        CheckIn::TServicePaymentListWithAuto payment_list;
        payment_list.fromDB(grp_id);
        payment_list.getUniqRFISCSs(pax_id, rfisc);
        // �᫨ �� ������� �� ������ RFISC, ������塞 ���⮩, �⮡� get_rule ��-⠪� ��ࠡ�⠫�
        if(rfisc.empty()) rfisc.insert("");
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
        TBrands brands; //�����, �⮡� ���஢����� ������
        for(; not paxQry.get().Eof; paxQry.get().Next()) {
            int pax_id = paxQry.get().FieldAsInteger("pax_id");
            string cls = paxQry.get().FieldAsString("class");
            string subcls = paxQry.get().FieldAsString("subclass");
            TRule bi_rule;
            // ���⠥� ����� �� ��� ���㦨����� ������������ � ��ய����
            if(bi_airline_service(t, op_type, bi_rule)) {
                // �� ������ �⠯� � bi_rule ��।�����:
                // ���᮪ �����     (bi_rule.halls)
                // ���. �ਣ�.      (bi_rule.pr_print_bi)

                // ���⠥� RFISC-�
                set<string> rfisc;
                get_rfisc(grp_id, pax_id, rfisc);

                // ���⠥� �७��
                brands.get(pax_id);
                // �᫨ �� ������� �� ������ �७��, ������塞 ���⮩, �⮡� get_rule ��-⠪� ��ࠡ�⠫�
                if(brands.empty()) brands.emplace_back();

                // ���⠥� ६�ન
                set<CheckIn::TPaxFQTItem> fqts;
                CheckIn::LoadPaxFQT(pax_id, fqts);
                // �᫨ �� ������� �� ����� ६�ન, ������塞 ������, �⮡� get_rule ��-⠪� ��ࠡ�⠫�
                if(fqts.empty()) fqts.insert(CheckIn::TPaxFQTItem());

                // ���⠥� ����� ����� � �� ���� ���. ��� (���� 3 ᨬ����)
                string aircode;
                CheckIn::TPaxTknItem tkn;
                if(CheckIn::LoadPaxTkn(pax_id, tkn)) {
                    aircode = tkn.no.substr(0, 3);
                }

                // �஡�� �� �७��� � ६�ઠ�
                // � ��� ����� ���� ��᪮�쪮 ६�ப � ࠧ�묨
                // ����ன���� ��㯯� ॣ����樨 (bi_print_rules.print_type = ALL, TWO, ONE)
                // �롨ࠥ� ᠬ�� �ਮ�����.

                BIPrintRules::TRule tmp_rule = bi_rule; // �⮡� �� ������� hall, is_business_hall, pr_print_bi
                for(set<string>::iterator iRfisc = rfisc.begin(); iRfisc != rfisc.end(); ++iRfisc)
                    for(set<CheckIn::TPaxFQTItem>::iterator iFqt = fqts.begin(); iFqt != fqts.end(); ++iFqt)
                        for(TBrands::const_iterator iBrand = brands.begin(); iBrand != brands.end(); ++iBrand) {
                            BIPrintRules::get_rule(
                                    t.airline,
                                    iFqt->tier_level,
                                    cls,
                                    subcls,
                                    iFqt->rem,
                                    iBrand->code(),
                                    iFqt->airline,
                                    aircode,
                                    *iRfisc,
                                    tmp_rule
                                    );

                            // ��᫥ ��宦����� �ࠢ��� �� ��� �ࠢ��� ���� �ਣ��襭��
                            // ����� �⮣� �ࠢ��� ��࠭����� � bi_rule:
                            // ��㯯� ॣ����樨 (bi_rule.print_type)
                            // ��ଫ���� (bi_rule.pr_issue)

                            if(tmp_rule.exists()) {
                                if(not bi_rule.exists())
                                    bi_rule = tmp_rule;
                                else {
                                    // ��� ����� �롮� �� �ਮ����
                                    if(bi_rule.print_type < tmp_rule.print_type)
                                        bi_rule = tmp_rule;
                                }
                            }
                        }
            }
            items.insert(make_pair(pax_id, bi_rule));
        }
        // ��室�� ��ࢮ�� ��� � ������ ��� �ᥩ ��㯯� (��㯯� ॣ����樨 �� �.�. bi_rule.print_type = All)
        TPaxList::iterator grpPax = items.begin();
        for(; grpPax!=items.end() and grpPax->second.print_type != BIPrintRules::TPrintType::All; ++grpPax );
        // �ਬ��塞 ��㯯���� �ࠢ��� ��� ��� ��ᮢ, �᫨ ⠪���� ��諮��
        if(grpPax != items.end()) {
            for (TPaxList::iterator iPax=items.begin(); iPax!=items.end(); ++iPax )
                iPax->second = grpPax->second;
        }
    }
}

void TPrPrint::dump(const string &file, int line)
{
    // �� ���� ���� ⮣�, �� ����稫���
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
    // ��⠫��� ���� ⥪�饣�
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
    // � ��� ⥯��� ��।������ pr_bi_print
    TCachedQuery grpQry("select pax_id from pax where grp_id = :grp_id",
            QParams() << QParam("grp_id", otInteger, grp_id));
    grpQry.get().Execute();

    Qry.SetVariable("op_type", DevOperTypes().encode(TDevOper::PrnBI));

    for(; not grpQry.get().Eof; grpQry.get().Next()) {
        int grp_pax_id = grpQry.get().FieldAsInteger("pax_id");

        map<SegNo_t, CheckIn::TCkinPaxTknItem> pax_list=CheckIn::GetTCkinTickets(PaxId_t(grp_pax_id));

        if(pax_list.empty()) {
            Qry.SetVariable("pax_id", grp_pax_id);
            Qry.Execute();
            paxs[1][grp_pax_id] = not Qry.Eof;
            grps.insert(grp_id);
        } else {
            for(map<SegNo_t, CheckIn::TCkinPaxTknItem>::iterator i = pax_list.begin(); i != pax_list.end(); i++) {
                bool pr_print;
                if(i->first.get() == 1) { // ���� ᥣ����
                    Qry.SetVariable("pax_id", i->second.paxId().get());
                    Qry.Execute();
                    pr_print = not Qry.Eof;
                } else { // ��⠫��
                    pr_print = true;
                }
                paxs[i->first.get()][i->second.paxId().get()] = pr_print;
                grps.insert(i->second.grpId().get());
            }
        }
    }

    // ��।��塞 ����� �� ��ࢮ� ᥣ����
    bool pr_bi_first = false;
    for(TPaxPrint::iterator i = paxs[1].begin(); i != paxs[1].end(); i++) {
        pr_bi_first = pr_bi_first or i->second;
    }

    // ����� ���� ���� ⮭��� ⮭�����
    // �᫨ �뫠 ��� ���� ����� �� ��ࢮ� ᥣ����,
    // ���� �஡����� �� ���⯥�⠭��
    // � ���⠢��� true ��� ���, � ������ �ਣ��襭�� ����� �⪫�祭�
    // �⮡� �ନ��� �� ������� ����� "�⯥���� �� ���⯥�⠭��?"
    if(pr_bi_first)
        for(TPaxPrint::iterator i = paxs[1].begin(); i != paxs[1].end(); i++) {
            if(not i->second) { // �� �뫮 ����
                const BIPrintRules::TRule &bi_rule = bi_rules.get(NoExists, i->first);
                i->second = not bi_rule.exists() or not bi_rule.pr_print_bi;
            }
        }

    // �⠢�� �ਧ��� ��� ��� ��⠫���
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
            QParams() << QParam("pax_id", otInteger, pax_id) << QParam("op_type", otString, DevOperTypes().encode(TDevOper::PrnBP))
            );
    Qry.get().Execute();
    pr_bp_print = not Qry.get().Eof;

    if(
            TReqInfo::Instance()->desk.compatible(OP_TYPE_VERSION) or
            TReqInfo::Instance()->isSelfCkinClientType()
      ) {
        if(paxs.empty()) { // ���� �맮�
            fromDB(grp_id, pax_id, Qry.get());
        } else if(grps.find(grp_id) == grps.end()) {
            // �� ᫥�. �맮�� ������ ���� � �ࠢ���� grp_id
            // �.�. � ⠪��, ����� �ਭ������� ��㯯� ���ᠦ�஢,
            // ��ନ஢����� � ���� �맮� (grps.size() > 1 � �. ᪢���猪)
            throw Exception("%s: unexpected grp_id: %d", __FUNCTION__, grp_id);
        }
        pr_bi_print = get_pr_print(pax_id);
    } else
        pr_bi_print = false;
}

string get_rem_txt(const string &airline, int pax_id, int tag_index)
{
    // ���⠥� RFISC-�
    // ��� �����, ⠪ � ��ᯫ���
    std::set<std::string> paxRFISCs;
    TPaidRFISCListWithAuto paid;
    paid.fromDB(pax_id, false);
    paid.getUniqRFISCSs(pax_id, paxRFISCs);
    if(paxRFISCs.empty()) paxRFISCs.insert("");

    // ���⠥� ६�ન
    set<CheckIn::TPaxFQTItem> fqts;
    CheckIn::LoadPaxFQT(pax_id, fqts);
    // �᫨ �� ������� �� ����� ६�ન, ������塞 ������
    if(fqts.empty()) fqts.insert(CheckIn::TPaxFQTItem());

    // ���⠥� �७��
    TBrands brands;
    brands.get(pax_id);
    // �᫨ �� ������� �� ������ �७��, ������塞 ���⮩
    if(brands.empty()) brands.emplace_back();

    TCachedQuery Qry(
            "select * from rem_txt_sets where "
            "   airline = :airline and "
            "   tag_index = :tag_index and "
            "   (rfisc is null or rfisc = :rfisc) and "
            "   (brand_airline is null or brand_airline = :brand_airline) and "
            "   (brand_code is null or brand_code = :brand_code) and "
            "   (fqt_airline is null or fqt_airline = :fqt_airline) and "
            "   (fqt_tier_level is null or fqt_tier_level = :fqt_tier_level) ",
            QParams()
            << QParam("airline", otString, airline)
            << QParam("tag_index", otInteger, tag_index)
            << QParam("rfisc", otString)
            << QParam("brand_airline", otString)
            << QParam("brand_code", otString)
            << QParam("fqt_airline", otString)
            << QParam("fqt_tier_level", otString));
    for(const auto &rfisc: paxRFISCs)
        for(const auto &fqt: fqts)
            for(const auto &brand: brands) {
                Qry.get().SetVariable("rfisc", rfisc);
                Qry.get().SetVariable("brand_airline", brand.oper_airline);
                Qry.get().SetVariable("brand_code", brand.code());
                Qry.get().SetVariable("fqt_airline", fqt.airline);
                Qry.get().SetVariable("fqt_tier_level", fqt.tier_level);
                Qry.get().Execute();
                if(not Qry.get().Eof)
                    return transliter(
                            string(Qry.get().FieldAsString("text")).substr(0, Qry.get().FieldAsInteger("text_length")),
                            1, Qry.get().FieldAsInteger("pr_lat"));
            }

    return "";
}
