#include "docs_annul_tags.h"
#include "stat/stat_annul_bt.h"

using namespace ASTRA;
using namespace AstraLocale;
using namespace std;

void ANNUL_TAGS(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    get_compatible_report_form("annul_tags", reqNode, resNode);
    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_annul");
    // переменные отчёта
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    // заголовок отчёта
    NewTextChild(variablesNode, "caption",
            getLocaleText("CAP.DOC.ANNUL_TAGS", LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());

    NewTextChild(variablesNode, "doc_cap_annul_reg_no", getLocaleText("№"));
    NewTextChild(variablesNode, "doc_cap_annul_fio", getLocaleText("Ф.И.О."));
    NewTextChild(variablesNode, "doc_cap_annul_no", getLocaleText("№№ баг. бирок"));
    NewTextChild(variablesNode, "doc_cap_annul_weight", getLocaleText("БГ вес"));
    NewTextChild(variablesNode, "doc_cap_annul_bag_type", getLocaleText("Тип багажа/RFISC"));
    NewTextChild(variablesNode, "doc_cap_annul_trfer", getLocaleText("Трфр"));
    NewTextChild(variablesNode, "doc_cap_annul_trfer_dir", getLocaleText("До трфр"));

    TAnnulBTStat AnnulBTStat;
    RunAnnulBTStat(AnnulBTStat, rpt_params.point_id);

    TCachedQuery paxQry("select reg_no, name, surname from pax where pax_id = :pax_id",
            QParams() << QParam("pax_id", otInteger));

    struct TPaxInfo {
        int reg_no;
        string surname;
        string name;
        TPaxInfo(): reg_no(NoExists) {}
    };

    map<int, TPaxInfo> pax_map;

    for(list<TAnnulBTStatRow>::const_iterator i = AnnulBTStat.rows.begin(); i != AnnulBTStat.rows.end(); i++) {
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");

        map<int, TPaxInfo>::iterator iPax = pax_map.find(i->pax_id);

        if(iPax == pax_map.end()) {
            TPaxInfo pax;
            if(i->pax_id != NoExists) {
                paxQry.get().SetVariable("pax_id", i->pax_id);
                paxQry.get().Execute();
                if(not paxQry.get().Eof) {
                    pax.reg_no = paxQry.get().FieldAsInteger("reg_no");
                    pax.name = paxQry.get().FieldAsString("name");
                    pax.surname = paxQry.get().FieldAsString("surname");
                }
            }
            pair<map<int, TPaxInfo>::iterator, bool> ret =
                pax_map.insert(make_pair(i->pax_id, pax));
            iPax = ret.first;
        }

        //  Рег№
        if(iPax->second.reg_no == NoExists)
            NewTextChild(rowNode, "reg_no");
        else
            NewTextChild(rowNode, "reg_no", iPax->second.reg_no);
        //  пассажира ФИО
        ostringstream buf;
        if(iPax->second.reg_no != NoExists)
            buf
                << transliter(iPax->second.surname, 1, rpt_params.GetLang() != AstraLocale::LANG_RU) << " "
                << transliter(iPax->second.name, 1, rpt_params.GetLang() != AstraLocale::LANG_RU);
        NewTextChild(rowNode, "fio", buf.str());
        //  номер бирки
        NewTextChild(rowNode, "no", get_tag_range(i->tags, LANG_EN));
        //  значение по весу
        if (i->weight != NoExists)
            NewTextChild(rowNode, "weight", i->weight);
        else
            NewTextChild(rowNode, "weight");

        //  тип багажа
        buf.str("");
        if(not i->rfisc.empty())
            buf << i->rfisc;
        else if(i->bag_type != NoExists)
            buf << ElemIdToNameLong(etBagType, i->bag_type);
        NewTextChild(rowNode, "bag_type", buf.str());

        if(i->trfer_airline.empty()) {
            //  призн.трансфера
            NewTextChild(rowNode, "pr_trfer", getLocaleText("НЕТ"));
            //  направление трфр
            NewTextChild(rowNode, "trfer_airp_arv");
        } else {
            NewTextChild(rowNode, "pr_trfer", getLocaleText("ДА"));
            NewTextChild(rowNode, "trfer_airp_arv", rpt_params.ElemIdToReportElem(etAirp, i->trfer_airp_arv, efmtCodeNative));
        }
    }
}


