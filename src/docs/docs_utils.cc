#include "docs_utils.h"
#include "stat/stat_utils.h"
#include "astra_utils.h"
#include "qrys.h"
#include "passenger.h"

using namespace std;
using namespace AstraLocale;

string get_test_str(int page_width, string lang)
{
    string result;
    for(int i=0;i<page_width/6;i++) result += " " + ( STAT::bad_client_img_version() and not get_test_server() ? " " : getLocaleText("CAP.TEST", lang)) + " ";
    return result;
}

bool old_cbbg()
{
    return false;
    /*
    TCachedQuery Qry("select new from test_cbbg");
    Qry.get().Execute();
    return not Qry.get().Eof and Qry.get().FieldAsInteger("new") == 0;
    */
}

namespace REPORT_PAX_REMS {
    bool getPaxRem(const string &lang, const CheckIn::TPaxRemBasic &basic, bool inf_indicator, CheckIn::TPaxRemItem &rem)
    {
        if (basic.empty()) return false;
        rem=CheckIn::TPaxRemItem(basic, inf_indicator, lang, applyLangForTranslit, CheckIn::TPaxRemBasic::outputReport);
        return true;
    }

    void get(int pax_id, TPerson pers_type, int seats,
             const CheckIn::TPaxTknItem& tkn,
             const string &lang, const map< TRemCategory, vector<string> > &filter, multiset<CheckIn::TPaxRemItem> &final_rems)
    {
        CheckIn::TPaxDocItem doc;
        CheckIn::TPaxDocoItem doco;
        CheckIn::TDocaMap doca_map;
        set<CheckIn::TPaxFQTItem> fqts;
        vector<CheckIn::TPaxASVCItem> asvc;
        multiset<CheckIn::TPaxRemItem> rems;
        map< TRemCategory, bool > cats;
        cats[remTKN]=false;
        cats[remDOC]=false;
        cats[remDOCO]=false;
        cats[remDOCA]=false;
        cats[remFQT]=false;
        cats[remASVC]=false;
        cats[remUnknown]=false;
        bool inf_indicator=pers_type==ASTRA::baby && seats==0;
        bool pr_find=true;
        if (!filter.empty())
        {
            pr_find=false;
            //䨫��� �� ������� ६�ઠ�
            map< TRemCategory, vector<string> >::const_iterator iRem=filter.begin();
            for(; iRem!=filter.end(); iRem++)
            {
                switch(iRem->first)
                {
                    case remTKN:
                        if (!tkn.empty() && !tkn.rem.empty() &&
                                find(iRem->second.begin(),iRem->second.end(),tkn.rem)!=iRem->second.end())
                            pr_find=true;
                        cats[remTKN]=true;
                        break;
                    case remDOC:
                        if (find(iRem->second.begin(),iRem->second.end(),"DOCS")!=iRem->second.end())
                        {
                            if (LoadPaxDoc(pax_id, doc)) pr_find=true;
                            cats[remDOC]=true;
                        };
                        break;
                    case remDOCO:
                        if (find(iRem->second.begin(),iRem->second.end(),"DOCO")!=iRem->second.end())
                        {
                            if (LoadPaxDoco(pax_id, doco)) pr_find=true;
                            cats[remDOCO]=true;
                        };
                        break;
                    case remDOCA:
                        if (find(iRem->second.begin(),iRem->second.end(),"DOCA")!=iRem->second.end())
                        {
                            if (LoadPaxDoca(pax_id, doca_map)) pr_find=true;
                            cats[remDOCA]=true;
                        };
                        break;
                    case remFQT:
                        LoadPaxFQT(pax_id, fqts);
                        for(set<CheckIn::TPaxFQTItem>::const_iterator f=fqts.begin();f!=fqts.end();f++)
                        {
                            if (!f->rem.empty() &&
                                    find(iRem->second.begin(),iRem->second.end(),f->rem)!=iRem->second.end())
                            {
                                pr_find=true;
                                break;
                            };
                        };
                        cats[remFQT]=true;
                        break;
                    case remASVC:
                        if (find(iRem->second.begin(),iRem->second.end(),"ASVC")!=iRem->second.end())
                        {
                            if (LoadPaxASVC(pax_id, asvc)) pr_find=true;
                            cats[remASVC]=true;
                        };
                        break;
                    default:
                        LoadPaxRem(pax_id, rems);
                        for(multiset<CheckIn::TPaxRemItem>::const_iterator r=rems.begin();r!=rems.end();r++)
                        {
                            if (!r->code.empty() &&
                                    find(iRem->second.begin(),iRem->second.end(),r->code)!=iRem->second.end())
                            {
                                pr_find=true;
                                break;
                            };
                        };
                        cats[remUnknown]=true;
                        break;
                };
                if (pr_find) break;
            };
        };

        if(pr_find) {
            CheckIn::TPaxRemItem rem;
            //����� ६�ન (��易⥫쭮 ��ࠡ��뢠�� ���묨)
            if (!cats[remUnknown]) LoadPaxRem(pax_id, rems);
            for(multiset<CheckIn::TPaxRemItem>::iterator r=rems.begin();r!=rems.end();++r)
                if (getPaxRem(lang, *r, inf_indicator, rem)) final_rems.insert(rem);

            //�����
            if (!cats[remTKN]) tkn;
            if (getPaxRem(lang, tkn, inf_indicator, rem)) final_rems.insert(rem);
            //���㬥��
            if (!cats[remDOC]) LoadPaxDoc(pax_id, doc);
            if (getPaxRem(lang, doc, inf_indicator, rem)) final_rems.insert(rem);
            //����
            if (!cats[remDOCO]) LoadPaxDoco(pax_id, doco);
            if (getPaxRem(lang, doco, inf_indicator, rem)) final_rems.insert(rem);
            //����
            if (!cats[remDOCA]) LoadPaxDoca(pax_id, doca_map);
            for(CheckIn::TDocaMap::const_iterator d = doca_map.begin(); d != doca_map.end(); ++d)
                if (getPaxRem(lang, d->second, inf_indicator, rem)) final_rems.insert(rem);
            //�����-�ணࠬ��
            if (!cats[remFQT]) LoadPaxFQT(pax_id, fqts);
            for(set<CheckIn::TPaxFQTItem>::const_iterator f=fqts.begin();f!=fqts.end();++f)
                if (getPaxRem(lang, *f, inf_indicator, rem)) final_rems.insert(rem);
            //��㣨
            if (!cats[remASVC]) LoadPaxASVC(pax_id, asvc);
            for(vector<CheckIn::TPaxASVCItem>::const_iterator a=asvc.begin();a!=asvc.end();++a)
                if (getPaxRem(lang, *a, inf_indicator, rem)) final_rems.insert(rem);
        }
    }

    void get(DB::TQuery &Qry, const string &lang, const map< TRemCategory, vector<string> > &filter, multiset<CheckIn::TPaxRemItem> &final_rems)
    {
        int pax_id=Qry.FieldAsInteger("pax_id");
        TPerson pers_type = DecodePerson(Qry.FieldAsString("pers_type").c_str());
        int seats = Qry.FieldAsInteger("seats");
        CheckIn::TPaxTknItem tkn;
        if (Qry.GetFieldIndex("ticket_no")>=0
            && Qry.GetFieldIndex("coupon_no")>=0
            && Qry.GetFieldIndex("ticket_rem")>=0
            && Qry.GetFieldIndex("ticket_confirm")>=0)
        {
          tkn.fromDB(Qry);
        }
        get(pax_id, pers_type, seats, tkn, lang, filter, final_rems);
    }

    void get(TQuery &Qry, const string &lang, const map< TRemCategory, vector<string> > &filter, multiset<CheckIn::TPaxRemItem> &final_rems)
    {
        int pax_id=Qry.FieldAsInteger("pax_id");
        TPerson pers_type = DecodePerson(Qry.FieldAsString("pers_type"));
        int seats = Qry.FieldAsInteger("seats");
        CheckIn::TPaxTknItem tkn;
        if (Qry.GetFieldIndex("ticket_no")>=0
            && Qry.GetFieldIndex("coupon_no")>=0
            && Qry.GetFieldIndex("ticket_rem")>=0
            && Qry.GetFieldIndex("ticket_confirm")>=0)
        {
          tkn.fromDB(Qry);
        }
        get(pax_id, pers_type, seats, tkn, lang, filter, final_rems);
    }

    void get(TQuery &Qry, const string &lang, multiset<CheckIn::TPaxRemItem> &final_rems)
    {
        return get(Qry, lang, map< TRemCategory, vector<string> >(), final_rems);
    }

    void get(DB::TQuery &Qry, const std::string &lang, std::multiset<CheckIn::TPaxRemItem> &final_rems)
    {
        return get(Qry, lang, map< TRemCategory, vector<string> >(), final_rems);
    }

    void get_rem_codes(TQuery &Qry, const string &lang, set<string> &rem_codes)
    {
        multiset<CheckIn::TPaxRemItem> final_rems;
        get(Qry, lang, map< TRemCategory, vector<string> >(), final_rems);
        for(multiset<CheckIn::TPaxRemItem>::iterator i = final_rems.begin(); i != final_rems.end(); i++)
            rem_codes.insert(i->code);
    }

    void get_rem_codes(DB::TQuery &Qry, const string &lang, set<string> &rem_codes)
    {
        multiset<CheckIn::TPaxRemItem> final_rems;
        get(Qry, lang, map< TRemCategory, vector<string> >(), final_rems);
        for(multiset<CheckIn::TPaxRemItem>::iterator i = final_rems.begin(); i != final_rems.end(); i++)
            rem_codes.insert(i->code);
    }

    void get_rem_codes(int pax_id, TPerson pers_type, int seats,
                       const CheckIn::TPaxTknItem& tkn,
                       const string &lang, set<string> &rem_codes)
    {
      multiset<CheckIn::TPaxRemItem> final_rems;
      get(pax_id, pers_type, seats, tkn, lang,
          map< TRemCategory, vector<string> >(), final_rems);
      for(multiset<CheckIn::TPaxRemItem>::iterator i = final_rems.begin(); i != final_rems.end(); i++)
        rem_codes.insert(i->code);
    }
}

string get_last_target(TQuery &Qry, TRptParams &rpt_params)
{
    string result;
    if(rpt_params.pr_trfer) {
        string airline = Qry.FieldAsString("trfer_airline");
        if(!airline.empty()) {
            ostringstream buf;
            buf
                << rpt_params.ElemIdToReportElem(etAirp, Qry.FieldAsString("trfer_airp_arv"), efmtNameLong).substr(0, 50)
                << "("
                << rpt_params.ElemIdToReportElem(etAirline, airline, efmtCodeNative)
                << setw(3) << setfill('0') << Qry.FieldAsInteger("trfer_flt_no")
                << rpt_params.ElemIdToReportElem(etSuffix, Qry.FieldAsString("trfer_suffix"), efmtCodeNative)
                << ")/" << DateTimeToStr(Qry.FieldAsDateTime("trfer_scd"), "dd");
            result = buf.str();
        }
    }
    return result;
}

std::string get_last_target(DB::TQuery &Qry, TRptParams &rpt_params)
{
    string result;
    if(rpt_params.pr_trfer) {
        string airline = Qry.FieldAsString("trfer_airline");
        if(!airline.empty()) {
            ostringstream buf;
            buf
                << rpt_params.ElemIdToReportElem(etAirp, Qry.FieldAsString("trfer_airp_arv"), efmtNameLong).substr(0, 50)
                << "("
                << rpt_params.ElemIdToReportElem(etAirline, airline, efmtCodeNative)
                << setw(3) << setfill('0') << Qry.FieldAsInteger("trfer_flt_no")
                << rpt_params.ElemIdToReportElem(etSuffix, Qry.FieldAsString("trfer_suffix"), efmtCodeNative)
                << ")/" << DateTimeToStr(Qry.FieldAsDateTime("trfer_scd"), "dd");
            result = buf.str();
        }
    }
    return result;
}
