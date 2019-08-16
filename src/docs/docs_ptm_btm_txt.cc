#include "docs_ptm_btm_txt.h"
#include "docs_ptm.h"
#include "docs_btm.h"
#include "stat/stat_utils.h"
#include "docs_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace ASTRA;
using namespace std;
using namespace AstraLocale;

void PTMBTMTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  if (rpt_params.rpt_type==rtPTMTXT)
    REPORTS::PTM(rpt_params, reqNode, resNode);
  else
    BTM(rpt_params, reqNode, resNode);

  xmlNodePtr variablesNode=NodeAsNode("form_data/variables",resNode);
  xmlNodePtr dataSetsNode=NodeAsNode("form_data/datasets",resNode);

  string str;
  ostringstream s;
  //текстовый формат
  int page_width=75;
  //специально вводим для кириллических символов, так как в терминале при экспорте проблемы
  //максимальная длина строки при экспорте в байтах! не должна превышать ~147 (65 рус + 15 лат)
  int max_symb_count= rpt_params.IsInter() ? page_width : 60;
  NewTextChild(variablesNode, "page_width", page_width);
  NewTextChild(variablesNode, "test_server", STAT::bad_client_img_version() ? 2 : get_test_server());
  if(STAT::bad_client_img_version())
      NewTextChild(variablesNode, "doc_cap_test", " ");

  s.str("");
  s << get_test_str(page_width, rpt_params.GetLang());
  NewTextChild(variablesNode, "test_str", s.str());


  s.str("");
  if (rpt_params.rpt_type==rtPTMTXT)
  {
      ProgTrace(TRACE5, "'%s'", NodeAsString((rpt_params.IsInter() ? "ptm_lat" : "ptm"), variablesNode)); //!!!
      str.assign(NodeAsString((rpt_params.IsInter() ? "ptm_lat" : "ptm"), variablesNode));
  }
  else
    str.assign(getLocaleText("БАГАЖНАЯ ВЕДОМОСТЬ", rpt_params.GetLang()));

  s << setfill(' ')
    << str
    << right << setw(page_width-str.size())
    << string(NodeAsString((rpt_params.IsInter() ? "own_airp_name_lat" : "own_airp_name"),variablesNode)).substr(0,max_symb_count-str.size());
  NewTextChild(variablesNode, "page_header_top", s.str());


  s.str("");
  str.assign(getLocaleText("Владелец или Оператор: ", rpt_params.GetLang()));
  s << left
    << str
    << string(NodeAsString("airline_name",variablesNode)).substr(0,max_symb_count-str.size()) << endl
    << setw(10) << getLocaleText("№ рейса", rpt_params.GetLang());
  if (rpt_params.IsInter())
    s << setw(19) << "Aircraft";
  else
    s << setw(9)  << "№ ВС"
      << setw(10) << "ТипВС Ст. ";

  if (!NodeIsNULL("airp_arv_name",variablesNode))
    s << setw(15) << getLocaleText("А/п вылета", rpt_params.GetLang())
      << setw(20) << getLocaleText("А/п назначения", rpt_params.GetLang());
  else
    s << setw(35) << getLocaleText("А/п вылета", rpt_params.GetLang());
  s << setw(6)  << getLocaleText("Дата", rpt_params.GetLang())
    << setw(5)  << getLocaleText("Время", rpt_params.GetLang()) << endl;

  s << setw(10) << NodeAsString("flt",variablesNode)
    << setw(11) << NodeAsString("bort",variablesNode)
    << setw(4)  << NodeAsString("craft",variablesNode)
    << setw(4)  << NodeAsString("park",variablesNode);

  if (!NodeIsNULL("airp_arv_name",variablesNode))
    s << setw(15) << string(NodeAsString("airp_dep_name",variablesNode)).substr(0,20-1)
      << setw(20) << string(NodeAsString("airp_arv_name",variablesNode)).substr(0,20-1);
  else
    s << setw(35) << string(NodeAsString("airp_dep_name",variablesNode)).substr(0,40-1);

  s << setw(6) << NodeAsString("scd_date",variablesNode)
    << setw(5) << NodeAsString("scd_time",variablesNode);
  string departure = NodeAsString("takeoff", variablesNode);
  if(not departure.empty())
      s << endl << getLocaleText("Вылет", rpt_params.GetLang()) << ": " << departure;
  NewTextChild(variablesNode, "page_header_center", s.str() );

  s.str("");
  str.assign(NodeAsString((rpt_params.IsInter()?"pr_brd_pax_lat":"pr_brd_pax"),variablesNode));
  if (!NodeIsNULL("zone",variablesNode))
  {
    unsigned int zone_len=max_symb_count-str.size()-1;
    string zone;
    zone.assign(getLocaleText("CAP.DOC.ZONE", rpt_params.GetLang()) + ": ");
    zone.append(NodeAsString("zone",variablesNode));
    if (zone_len<zone.size())
      s << str << right << setw(page_width-str.size()) << zone.substr(0,zone_len-3).append("...") << endl;
    else
      s << str << right << setw(page_width-str.size()) << zone << endl;
  }
  else
    s << str << endl;

  if (rpt_params.rpt_type==rtPTMTXT)
    s << left
      << setw(4)  << (getLocaleText("CAP.DOC.REG", rpt_params.GetLang()))
      << setw(rpt_params.IsInter()?13:14) << (getLocaleText("Фамилия", rpt_params.GetLang()))
      << setw(4)  << (getLocaleText("Пол", rpt_params.GetLang()))
      << setw(rpt_params.IsInter()?4:3)   << (getLocaleText("Кл", rpt_params.GetLang()))
      << setw(5)  << (getLocaleText("CAP.DOC.SEAT_NO", rpt_params.GetLang()))
      << setw(4)  << (getLocaleText("РБ", rpt_params.GetLang()))
      << setw(4)  << (getLocaleText("РМ", rpt_params.GetLang()))
      << setw(7)  << (getLocaleText("CAP.DOC.BAG", rpt_params.GetLang()))
      << setw(6)  << (getLocaleText("Р/кл", rpt_params.GetLang()))
      << setw(15) << (getLocaleText("CAP.DOC.BAG_TAG_NOS", rpt_params.GetLang()))
      << setw(9)  << (getLocaleText("Ремарки", rpt_params.GetLang()));
  else
    s << left
      << setw(29) << (getLocaleText("Номера багажных бирок", rpt_params.GetLang()))
      << setw(10) << (getLocaleText("Цвет", rpt_params.GetLang()))
      << setw(5)  << (getLocaleText("Мест", rpt_params.GetLang()))
      << setw(7)  << (getLocaleText("Вес", rpt_params.GetLang()))
      << setw(8)  << (getLocaleText("№ Конт.", rpt_params.GetLang()))
      << setw(10) << (getLocaleText("CAP.DOC.HOLD", rpt_params.GetLang()))
      << setw(11) << (getLocaleText("Отсек", rpt_params.GetLang()));

  NewTextChild(variablesNode, "page_header_bottom", s.str() );

  if (rpt_params.rpt_type==rtPTMTXT)
  {
    s.str("");
    s
        << setw(17)
        << (getLocaleText("Всего в классе", rpt_params.GetLang()))
        << setw(9)
        << "M/F"
        << setw(4)
        << getLocaleText("Крс", rpt_params.GetLang())
        << right
        << setw(3)
        << getLocaleText("РБ", rpt_params.GetLang()) << " "
        << setw(3)
        << getLocaleText("РМ", rpt_params.GetLang()) << " "
        << left
        << setw(7)
        << getLocaleText("Баг.", rpt_params.GetLang())
        << right
        << setw(5)
        << getLocaleText("Р/кл", rpt_params.GetLang())
        << setw(7) << " " // заполнение 7 пробелов (обязат. д.б. вкл. флаг right, см. выше)
        << "XCR DHC MOS JMP"
        << endl
        // Здесь видно, что Багаж и р/кл (%2u/%-4u%5u) расположены вплотную, что не есть хорошо.
        << "%-16s %-7s  %3u %3u %3u %2u/%-4u%5u       %3u %3u %3u %3u";
    NewTextChild(variablesNode, "total_in_class_fmt", s.str());

    s.str("");
    if (!NodeIsNULL("airp_arv_name",variablesNode))
    {
      str.assign(NodeAsString("airp_dep_name",variablesNode)).append("-");
      str.append(NodeAsString("airp_arv_name",variablesNode));

      s << left
        << setw(6) << (getLocaleText("Всего", rpt_params.GetLang()))
        << setw(50) << str.substr(0,50-1)
        << (getLocaleText("Подпись", rpt_params.GetLang())) << endl;
    }
    else
      s << left
        << (getLocaleText("Всего", rpt_params.GetLang())) << endl;

    s << setw(7) << (getLocaleText("Кресел", rpt_params.GetLang()))
      << setw(8) << (getLocaleText("ВЗ/Ж", rpt_params.GetLang()))
      << setw(7) << (getLocaleText("РБ", rpt_params.GetLang()))
      << setw(7) << (getLocaleText("РМ", rpt_params.GetLang()))
      << setw(7) << (getLocaleText("Мест", rpt_params.GetLang()))
      << setw(7) << (getLocaleText("Вес", rpt_params.GetLang()))
      << setw(7) << (getLocaleText("Р/кл", rpt_params.GetLang()))
      << setw(8) << (getLocaleText("CAP.DOC.EX_BAG", rpt_params.GetLang()))
      << setw(16) << "XCR DHC MOS JMP" << endl
      << "%-6u %-7s %-6u %-6u %-6u %-6u %-6u %-6s  %-3u %-3u %-3u %-3u" << endl
      << (getLocaleText("Подпись", rpt_params.GetLang())) << endl
      << setw(30) << string(NodeAsString("pts_agent", variablesNode)).substr(0, 30) << endl
      << (getLocaleText("CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)), rpt_params.GetLang()));

    NewTextChild(variablesNode, "page_footer_top", s.str() );


    xmlNodePtr dataSetNode = NodeAsNode("v_pm_trfer", dataSetsNode);
    vector<string> rows;
    map< string, vector<string> > fields;
    int row;
    xmlNodePtr rowNode=dataSetNode->children;
    for(;rowNode!=NULL;rowNode=rowNode->next)
    {
      str=NodeAsString("airp_arv_name",rowNode);
      ReplaceTextChild(rowNode,"airp_arv_name",str.substr(0,max_symb_count));
      if (!NodeIsNULL("last_target",rowNode))
      {
        str.assign(getLocaleText("ДО", rpt_params.GetLang()) + ": ").append(NodeAsString("last_target",rowNode));
        ReplaceTextChild(rowNode,"last_target",str.substr(0,max_symb_count));
      };

      //рабиваем фамилию, бирки, ремарки
      SeparateString(NodeAsString("full_name",rowNode),13,rows);
      fields["full_name"]=rows;
      SeparateString(NodeAsString("tags",rowNode),15,rows);
      fields["tags"]=rows;
      SeparateString(NodeAsString("remarks",rowNode),9,rows);
      fields["remarks"]=rows;

      string gender = NodeAsString("gender",rowNode);

      row=0;
      string pers_type=NodeAsString("pers_type",rowNode);
      s.str("");
      do
      {
        if (row!=0) s << endl;
        s << right << setw(3) << (row==0?NodeAsString("reg_no",rowNode):"") << " "
          << left << setw(13) << (!fields["full_name"].empty()?*(fields["full_name"].begin()):"") << " "
          << left <<  setw(4) << (row==0?gender:"")
          << left <<  setw(3) << (row==0?NodeAsString("class",rowNode):"")
          << right << setw(4) << (row==0?NodeAsString("seat_no",rowNode):"") << " "
          << left <<  setw(4) << (row==0&&pers_type=="CHD"?" X ":"")
          << left <<  setw(4) << (row==0&&pers_type=="INF"?" X ":"");
        if (row!=0 ||
            (NodeAsInteger("bag_amount",rowNode)==0 &&
            NodeAsInteger("bag_weight",rowNode)==0))
          s << setw(7) << "";
        else
          s << right << setw(2) << NodeAsInteger("bag_amount",rowNode) << "/"
            << left << setw(4) << NodeAsInteger("bag_weight",rowNode);
        if (row!=0 ||
            NodeAsInteger("rk_weight",rowNode)==0)
          s << setw(5) << "";
        else
          s << right << setw(4) << NodeAsInteger("rk_weight",rowNode) << " ";
        s << left << setw(15) << (!fields["tags"].empty()?*(fields["tags"].begin()):"") << " "
          << left << setw(9) << (!fields["remarks"].empty()?*(fields["remarks"].begin()):"") << " ";

        for(map< string, vector<string> >::iterator f=fields.begin();f!=fields.end();f++)
          if (!f->second.empty()) f->second.erase(f->second.begin());
        row++;
      }
      while(!fields["full_name"].empty() ||
            !fields["tags"].empty() ||
            !fields["remarks"].empty());

      NewTextChild(rowNode,"str",s.str());
    };

    for(int k=(int)rpt_params.pr_trfer;k>=0;k--)
    {
      s.str("");
      if (!rpt_params.pr_trfer)
        s << setw(15) << (getLocaleText("Всего багажа", rpt_params.GetLang()));
      else
      {
        if (k==0)
          s << setw(19) << (getLocaleText("Всего нетр. баг.", rpt_params.GetLang()));
        else
          s << setw(19) << (getLocaleText("Всего тр. баг.", rpt_params.GetLang()));
      };

      s << setw(7) << (getLocaleText("Кресел", rpt_params.GetLang()))
        << setw(8) << (getLocaleText("ВЗ/Ж", rpt_params.GetLang()))
        << setw(7) << (getLocaleText("РБ", rpt_params.GetLang()))
        << setw(7) << (getLocaleText("РМ", rpt_params.GetLang()))
        << setw(7) << (getLocaleText("Мест", rpt_params.GetLang()))
        << setw(7) << (getLocaleText("Вес", rpt_params.GetLang()))
        << setw(7) << (getLocaleText("Р/кл", rpt_params.GetLang()))
        << setw(7) << (getLocaleText("CAP.DOC.EX_BAG", rpt_params.GetLang()));

      if (!rpt_params.pr_trfer)
        NewTextChild(variablesNode, "subreport_header", s.str() );
      else
      {
        if (k==0)
          NewTextChild(variablesNode, "subreport_header", s.str() );
        else
          NewTextChild(variablesNode, "subreport_header_trfer", s.str() );
      };
    };

    dataSetNode = NodeAsNode(rpt_params.pr_trfer ? "v_pm_trfer_total" : "v_pm_total", dataSetsNode);

    rowNode=dataSetNode->children;
    for(;rowNode!=NULL;rowNode=rowNode->next)
    {
      ostringstream adl_fem;
      adl_fem << NodeAsInteger("adl", rowNode) << '/' << NodeAsInteger("adl_f", rowNode);

      s.str("");
      s << setw(rpt_params.pr_trfer?19:15) << NodeAsString("class_name",rowNode)
        << setw(7) << NodeAsInteger("seats",rowNode)
        << setw(8) << adl_fem.str()
        << setw(7) << NodeAsInteger("chd",rowNode)
        << setw(7) << NodeAsInteger("inf",rowNode)
        << setw(7) << NodeAsInteger("bag_amount",rowNode)
        << setw(7) << NodeAsInteger("bag_weight",rowNode)
        << setw(7) << NodeAsInteger("rk_weight",rowNode)
        << setw(7) << NodeAsString("excess",rowNode) << endl
        << "XCR/DHC/MOS/JMP: "
        << NodeAsInteger("xcr",rowNode) << "/"
        << NodeAsInteger("dhc",rowNode) << "/"
        << NodeAsInteger("mos",rowNode) << "/"
        << NodeAsInteger("jmp",rowNode);

      NewTextChild(rowNode,"str",s.str());
    };
  }
  else
  {
    s.str("");
    s << "%-39s%4u %6u";
    NewTextChild(variablesNode, "total_in_class_fmt", s.str());

    xmlNodePtr dataSetNode = NodeAsNode(rpt_params.pr_trfer ? "v_bm_trfer" : "v_bm", dataSetsNode);
    vector<string> rows;
    map< string, vector<string> > fields;
    int row;
    xmlNodePtr rowNode=dataSetNode->children;
    for(;rowNode!=NULL;rowNode=rowNode->next)
    {
      str=NodeAsString("airp_arv_name",rowNode);
      ReplaceTextChild(rowNode,"airp_arv_name",str.substr(0,max_symb_count));
      if (!NodeIsNULL("last_target",rowNode))
      {
        str.assign(getLocaleText("ДО", rpt_params.GetLang()) + ": ").append(NodeAsString("last_target",rowNode));
        ReplaceTextChild(rowNode,"last_target",str.substr(0,max_symb_count));
      };

      //разбиваем диапазоны бирок, цвет
      int offset = 2;
      if(not NodeIsNULL("bag_name", rowNode))
          offset += 2;
      if(not NodeIsNULL("to_ramp", rowNode))
          offset += 2;

      SeparateString(NodeAsString("birk_range",rowNode),28 - offset,rows);
      fields["birk_range"]=rows;
      SeparateString(NodeAsString("color",rowNode),9,rows);
      fields["color"]=rows;

      row=0;
      s.str("");
      do
      {
        if (row!=0) s << endl;
        s << setw(offset) << "" //отступ
          << left << setw(28 - offset) << (!fields["birk_range"].empty()?*(fields["birk_range"].begin()):"") << " "
          << left << setw(9) << (!fields["color"].empty()?*(fields["color"].begin()):"") << " "
          << right << setw(4) << (row==0?NodeAsString("num",rowNode):"") << " ";

        for(map< string, vector<string> >::iterator f=fields.begin();f!=fields.end();f++)
          if (!f->second.empty()) f->second.erase(f->second.begin());
        row++;
      }
      while(!fields["birk_range"].empty() ||
            !fields["color"].empty());

      NewTextChild(rowNode,"str",s.str());
    };

    if (rpt_params.pr_trfer)
    {
      for(int k=(int)rpt_params.pr_trfer;k>=0;k--)
      {
        s.str("");
        s << left;
        if (k==0)
          s << setw(39) << (getLocaleText("Всего багажа, исключая трансферный", rpt_params.GetLang()));
        else
          s << setw(39) << (getLocaleText("Всего трансферного багажа", rpt_params.GetLang()));
        s << "%4u %6u";
        if (k==0)
          NewTextChild(variablesNode, "total_not_trfer_fmt", s.str() );
        else
          NewTextChild(variablesNode, "total_trfer_fmt", s.str() );
      };

      s.str("");
      s << setw(39) << (getLocaleText("Всего багажа", rpt_params.GetLang()))
        << "%4u %6u" << endl
        << setw(39) << (getLocaleText("Трансферного багажа", rpt_params.GetLang()))
        << "%4u %6u";
      NewTextChild(variablesNode, "report_footer", s.str() );
    };

    s.str("");
    s << (getLocaleText("CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)), rpt_params.GetLang()));
    NewTextChild(variablesNode, "page_footer_top", s.str() );



    s.str("");
    if (!NodeIsNULL("airp_arv_name",variablesNode))
    {
      str.assign(NodeAsString("airp_dep_name",variablesNode)).append("-");
      str.append(NodeAsString("airp_arv_name",variablesNode));

      s << left
        << setw(6) << (getLocaleText("Всего", rpt_params.GetLang()))
        << setw(50) << str.substr(0,50-1)
        << (getLocaleText("Подпись агента СОПП", rpt_params.GetLang())) << endl;
    }
    else
      s << left
        << setw(56) << (getLocaleText("Всего", rpt_params.GetLang()))
        << (getLocaleText("Подпись агента СОПП", rpt_params.GetLang())) << endl;

    s << setw(6)  << (getLocaleText("Мест", rpt_params.GetLang()))
      << setw(7)  << (getLocaleText("Вес", rpt_params.GetLang()))
      << setw(43) << (getLocaleText("Количество мест прописью", rpt_params.GetLang()))
      << setw(24) << string(NodeAsString("pts_agent", variablesNode)).substr(0, 24) << endl;

    SeparateString(NodeAsString("Tot",variablesNode),42,rows);
    row=0;
    do
    {
      if (row!=0) s << endl;
      s << setw(6)  << (row==0?NodeAsString("TotPcs",variablesNode):"")
        << setw(7)  << (row==0?NodeAsString("TotWeight",variablesNode):"")
        << setw(42) << (!rows.empty()?*(rows.begin()):"");
      if (!rows.empty()) rows.erase(rows.begin());
      row++;
    }
    while(!rows.empty());
    NewTextChild(variablesNode,"report_summary",s.str());
  };
};

