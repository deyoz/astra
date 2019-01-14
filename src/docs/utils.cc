#include "utils.h"
#include "stat_utils.h"
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
    TCachedQuery Qry("select new from test_cbbg");
    Qry.get().Execute();
    return not Qry.get().Eof and Qry.get().FieldAsInteger("new") == 0;
}

namespace REPORT_PAX_REMS {
    bool getPaxRem(const string &lang, const CheckIn::TPaxRemBasic &basic, bool inf_indicator, CheckIn::TPaxRemItem &rem)
    {
        if (basic.empty()) return false;
        rem=CheckIn::TPaxRemItem(basic, inf_indicator, lang, applyLangForTranslit, CheckIn::TPaxRemBasic::outputReport);
        return true;
    }

    void get(TQuery &Qry, const string &lang, const map< TRemCategory, vector<string> > &filter, multiset<CheckIn::TPaxRemItem> &final_rems)
    {
        CheckIn::TPaxTknItem tkn;
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
        int pax_id=Qry.FieldAsInteger("pax_id");
        bool inf_indicator=DecodePerson(Qry.FieldAsString("pers_type"))==ASTRA::baby && Qry.FieldAsInteger("seats")==0;
        bool pr_find=true;
        if (!filter.empty())
        {
            pr_find=false;
            //фильтр по конкретным ремаркам
            map< TRemCategory, vector<string> >::const_iterator iRem=filter.begin();
            for(; iRem!=filter.end(); iRem++)
            {
                switch(iRem->first)
                {
                    case remTKN:
                        tkn.fromDB(Qry);
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
            //обычные ремарки (обязательно обрабатываем первыми)
            if (!cats[remUnknown]) LoadPaxRem(pax_id, rems);
            for(multiset<CheckIn::TPaxRemItem>::iterator r=rems.begin();r!=rems.end();++r)
                if (getPaxRem(lang, *r, inf_indicator, rem)) final_rems.insert(rem);

            //билет
            if (!cats[remTKN]) tkn.fromDB(Qry);
            if (getPaxRem(lang, tkn, inf_indicator, rem)) final_rems.insert(rem);
            //документ
            if (!cats[remDOC]) LoadPaxDoc(pax_id, doc);
            if (getPaxRem(lang, doc, inf_indicator, rem)) final_rems.insert(rem);
            //виза
            if (!cats[remDOCO]) LoadPaxDoco(pax_id, doco);
            if (getPaxRem(lang, doco, inf_indicator, rem)) final_rems.insert(rem);
            //адреса
            if (!cats[remDOCA]) LoadPaxDoca(pax_id, doca_map);
            for(CheckIn::TDocaMap::const_iterator d = doca_map.begin(); d != doca_map.end(); ++d)
                if (getPaxRem(lang, d->second, inf_indicator, rem)) final_rems.insert(rem);
            //бонус-программа
            if (!cats[remFQT]) LoadPaxFQT(pax_id, fqts);
            for(set<CheckIn::TPaxFQTItem>::const_iterator f=fqts.begin();f!=fqts.end();++f)
                if (getPaxRem(lang, *f, inf_indicator, rem)) final_rems.insert(rem);
            //услуги
            if (!cats[remASVC]) LoadPaxASVC(pax_id, asvc);
            for(vector<CheckIn::TPaxASVCItem>::const_iterator a=asvc.begin();a!=asvc.end();++a)
                if (getPaxRem(lang, *a, inf_indicator, rem)) final_rems.insert(rem);
        }
    }

    void get(TQuery &Qry, const string &lang, multiset<CheckIn::TPaxRemItem> &final_rems)
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
}

