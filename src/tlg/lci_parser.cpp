#include "lci_parser.h"
#include "misc.h"
#include "salons.h"
#include "telegram.h"
#include "date_time.h"
#include "TypeBHelpMng.h"
#include "points.h" // for TFlightMaxCommerce
#include "pers_weights.h"
#include "astra_context.h"
#include "astra_context.h"
#include <sstream>
#include <serverlib/xml_stuff.h>


#define STDLOG NICKNAME,__FILE__,__LINE__
#define NICKNAME "DEN"
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace ASTRA;


namespace TypeB
{

string getElemId(const string &val, TElemType el)
{
    TElemFmt fmt;
    string lang;
    string subcls = ElemToElemId(el, val, fmt, lang);
    if(subcls.empty())
        throw ETlgError("unknown %s elem %s", EncodeElemType(el), val.c_str());
    return subcls;
}

const char *TReqTypeS[] =
{
    "SP",
    "BT",
    "SR",
    "WM",
    "WB",
    ""
};

TReqType DecodeReqType(const string &s)
{
    unsigned int i;
    for(i=0;i<sizeof(TReqTypeS)/sizeof(TReqTypeS[0]);i+=1) if (s == TReqTypeS[i]) break;
    if (i<sizeof(TReqTypeS)/sizeof(TReqTypeS[0]))
        return (TReqType)i;
    else
        return rtUnknown;
};

const char* EncodeReqType(TReqType p)
{
    return TReqTypeS[p];
};

const char *TDestInfoTypeS[] =
{
    "A",
    "C",
    "G",
    "J",
    "WB",
    ""
};

TDestInfoType DecodeDestInfoType(const string &s)
{
    unsigned int i;
    for(i=0;i<sizeof(TDestInfoTypeS)/sizeof(TDestInfoTypeS[0]);i+=1) if (s == TDestInfoTypeS[i]) break;
    if (i<sizeof(TDestInfoTypeS)/sizeof(TDestInfoTypeS[0]))
        return (TDestInfoType)i;
    else
        return dtUnknown;
};

const char* EncodeDestInfoType(TDestInfoType p)
{
    return TDestInfoTypeS[p];
};

const char *TDestInfoKeyS[] =
{
    "PT",
    "BT",
    "H",
    "SP",
    ""
};

TDestInfoKey DecodeDestInfoKey(const string &s)
{
    unsigned int i;
    for(i=0;i<sizeof(TDestInfoKeyS)/sizeof(TDestInfoKeyS[0]);i+=1) if (s == TDestInfoKeyS[i]) break;
    if (i<sizeof(TDestInfoKeyS)/sizeof(TDestInfoKeyS[0]))
        return (TDestInfoKey)i;
    else
        return dkUnknown;
};

const char* EncodeDestInfoKey(TDestInfoKey p)
{
    return TDestInfoKeyS[p];
};

const char *TGenderS[] =
{
    "M",
    "F",
    "C",
    "I",
    ""
};

TGender DecodeGender(const string &s)
{
    unsigned int i;
    for(i=0;i<sizeof(TGenderS)/sizeof(TGenderS[0]);i+=1) if (s == TGenderS[i]) break;
    if (i<sizeof(TGenderS)/sizeof(TGenderS[0]))
        return (TGender)i;
    else
        return gUnknown;
};

const char* EncodeGender(TGender p)

{
    return TGenderS[p];
};

const char *TWMDesignatorS[] =
{
    "S",
    "A",
    "WB",
    ""
};

TWMDesignator DecodeWMDesignator(const string &s)
{
    unsigned int i;
    for(i=0;i<sizeof(TWMDesignatorS)/sizeof(TWMDesignatorS[0]);i+=1) if (s == TWMDesignatorS[i]) break;
    if (i<sizeof(TWMDesignatorS)/sizeof(TWMDesignatorS[0]))
        return (TWMDesignator)i;
    else
        return wmdUnknown;
};

const char* EncodeWMDesignator(TWMDesignator p)
{
    return TWMDesignatorS[p];
};

const char *TWMTypeS[] =
{
    "P",
    "H",
    "B",

    // non standard WBW types
    "TT",
    "PT",
    "HT",
    "BT",
    "CT",
    "MT",
    ""
};

TWMType DecodeWMType(const string &s)
{
    unsigned int i;
    for(i=0;i<sizeof(TWMTypeS)/sizeof(TWMTypeS[0]);i+=1) if (s == TWMTypeS[i]) break;
    if (i<sizeof(TWMTypeS)/sizeof(TWMTypeS[0]))
        return (TWMType)i;
    else
        return wmtUnknown;
};

const char* EncodeWMType(TWMType p)
{
    return TWMTypeS[p];
};

const char *TPDTypeS[] =
{
    "C",
    "Z",
    "R",
    "J",
    ""
};

TPDType DecodePDType(const string &s)
{
    unsigned int i;
    for(i=0;i<sizeof(TPDTypeS)/sizeof(TPDTypeS[0]);i+=1) if (s == TPDTypeS[i]) break;
    if (i<sizeof(TPDTypeS)/sizeof(TPDTypeS[0]))
        return (TPDType)i;
    else
        return pdtUnknown;
};

const char* EncodePDType(TPDType p)
{
    return TPDTypeS[p];
};

const char *TWMSubTypeS[] =
{
    "G",
    "C",
    "CG",
    ""
};

TWMSubType DecodeWMSubType(const string &s)
{
    unsigned int i;
    for(i=0;i<sizeof(TWMSubTypeS)/sizeof(TWMSubTypeS[0]);i+=1) if (s == TWMSubTypeS[i]) break;
    if (i<sizeof(TWMSubTypeS)/sizeof(TWMSubTypeS[0]))
        return (TWMSubType)i;
    else
        return wmsUnknown;
};

const char* EncodeWMSubType(TWMSubType p)
{
    return TWMSubTypeS[p];
};

const char *TMeasurS[] =
{
    "KG",
    "LB",
    "N", // см. коммент к TMeasur
    ""
};

TMeasur DecodeMeasur(const char* s)
{
    unsigned int i;
    for(i=0;i<sizeof(TMeasurS)/sizeof(TMeasurS[0]);i+=1) if (strcmp(s,TMeasurS[i])==0) break;
    if (i<sizeof(TMeasurS)/sizeof(TMeasurS[0]))
        return (TMeasur)i;
    else
        return mUnknown;
};

const char* EncodeMeasur(TMeasur p)
{
    return TMeasurS[p];
};

const char TSeatingMethodS[] =
{
    'F',
    'C',
    'Z',
    'R',
    'S',
    0
};

TSeatingMethod DecodeSeatingMethod(const char s)
{
    unsigned int i;
    for(i=0;i<sizeof(TSeatingMethodS)/sizeof(TSeatingMethodS[0]);i+=1) if (s == TSeatingMethodS[i]) break;
    if (i<sizeof(TSeatingMethodS)/sizeof(TSeatingMethodS[0]))
        return (TSeatingMethod)i;
    else
        return smUnknown;
};

char EncodeSeatingMethod(TSeatingMethod p)
{
    return TSeatingMethodS[p];
};

const char TOriginatorS[] =
{
    'L',
    'C',
    0
};

TOriginator DecodeOriginator(const char s)
{
    unsigned int i;
    for(i=0;i<sizeof(TOriginatorS)/sizeof(TOriginatorS[0]);i+=1) if (s == TOriginatorS[i]) break;
    if (i<sizeof(TOriginatorS)/sizeof(TOriginatorS[0]))
        return (TOriginator)i;
    else
        return oUnknown;
};

char EncodeOriginator(TOriginator p)
{
    return TOriginatorS[p];
};

const char TActionS[] =
{
    'O',
    'S',
    'C',
    'U',
    'F',
    'E',
    'R',
    0
};

TAction DecodeAction(const char s)
{
    unsigned int i;
    for(i=0;i<sizeof(TActionS)/sizeof(TActionS[0]);i+=1) if (s == TActionS[i]) break;
    if (i<sizeof(TActionS)/sizeof(TActionS[0]))
        return (TAction)i;
    else
        return aUnknown;
};

char EncodeAction(TAction p)
{
    return TActionS[p];
};

void TLCIContent::clear()
{
    point_id_tlg = NoExists;
    time_receive = NoExists;
    sender.clear();
    typeb_in_id = NoExists;
    franchise_flt.clear();
    action_code.clear();
    req.clear();
    dst.clear();
    eqt.clear();
    wa.clear();
    sm.clear();
    sr.clear();
    wm.clear();
    pd.clear();
    sp.clear();
    si.clear();
}

int lci_data(int argc, char **argv)
{
    TLCIContent lci;

    /*
    TLCIReqInfo &reqInfo1 = lci.req[rtSP];
    reqInfo1.req_type = rtSP;
    reqInfo1.lang = "RU";
    reqInfo1.max_commerce = 100;

    reqInfo1.sr.type = TSR::srStd;
    reqInfo1.sr.format = 'F';
    reqInfo1.sr.c["П"] = 10;
    reqInfo1.sr.c["Б"] = 20;
    reqInfo1.sr.c["Э"] = 30;

    reqInfo1.sr.z["zone1"] = 15;
    reqInfo1.sr.z["zone2"] = 25;
    reqInfo1.sr.z["zone3"] = 35;

    reqInfo1.sr.r.push_back("r1");
    reqInfo1.sr.r.push_back("r2");
    reqInfo1.sr.r.push_back("r3");

    reqInfo1.sr.s.push_back("s1");
    reqInfo1.sr.s.push_back("s2");
    reqInfo1.sr.s.push_back("s3");

    reqInfo1.sr.j.amount = 660;
    reqInfo1.sr.j.seats.push_back("j_seat1");
    reqInfo1.sr.j.seats.push_back("j_seat2");
    reqInfo1.sr.j.zones["j_zone1"] = 11;

    reqInfo1.wm_type = wmtWBTotalWeight;
    reqInfo1.sp_type = spStd;

    TLCIReqInfo &reqInfo2 = lci.req[rtBT];
    reqInfo2.sr.type = TSR::srWB;
    */

    /*
    TDestInfoMap &dest_info_map = lci.dst["ДМД"];
    TDestInfo &dest_info = dest_info_map[dkPT];
    dest_info.total = 100;
    dest_info.weight = 55;
    dest_info.j = 13;
    dest_info.measur = mKG;

    dest_info.cls_total.f = 5;
    dest_info.cls_total.c = 10;
    dest_info.cls_total.y = 15;

    dest_info.actual_total.f = 20;
    dest_info.actual_total.c = 25;
    dest_info.actual_total.y = 30;

    dest_info.gender_total.m = 40;
    dest_info.gender_total.f = 50;
    dest_info.gender_total.c = 60;
    dest_info.gender_total.i = 70;
    */

    /*
    lci.eqt.bort = "BORT";
    lci.eqt.craft = "CRAFT";
    lci.eqt.cfg["П"] = 2;
    lci.eqt.cfg["Б"] = 4;
    lci.eqt.cfg["Э"] = 6;
    */

    /*
//    lci.wa.payload.amount = 120;
    lci.wa.payload.measur = mKG;
    lci.wa.underload.amount = 210;
//    lci.wa.underload.measur = mLB;
    */

    // lci.sm.value = smClass;

    /*
    lci.sr.type = TSR::srStd;
    lci.sr.format = 'F';
    lci.sr.c["П"] = 10;
    lci.sr.c["Б"] = 20;
    lci.sr.c["Э"] = 30;

    lci.sr.z["zone1"] = 15;
    lci.sr.z["zone2"] = 25;
    lci.sr.z["zone3"] = 35;

    lci.sr.r.push_back("r1");
    lci.sr.r.push_back("r2");
    lci.sr.r.push_back("r3");

    lci.sr.s.push_back("s1");
    lci.sr.s.push_back("s2");
    lci.sr.s.push_back("s3");

    lci.sr.j.amount = 660;
    lci.sr.j.seats.push_back("j_seat1");
    lci.sr.j.seats.push_back("j_seat2");
    lci.sr.j.zones["j_zone1"] = 11;
    */

    // TClsGenderWeight
    // lci.wm.parse("WM.S.P.CG.П100/50/25/20.Б80/75/50/25.Э75/70/45/20.KG");

    // TGenderCount
    // lci.wm.parse("WM.S.P.G.76/76/35/0.KG");

    // TClsWeight
    // lci.wm.parse("WM.S.P.C.85/76/70.KG");
    // lci.wm.parse("WM.S.H.C.15/10/10.KG");

    // TClsBagWeight
    // lci.wm.parse("WM.S.B.C.F45.C40.M35.LB");

    // lci.wm.parse("WM.A.B.KG");

    // lci.wm.parse("WM.S.B.14.KG");

    // lci.wm.parse("WM.WB.TT.4257/PT.2000/HT.150/BT.1000/CT.1100/MT.7.KG");

    // ---- PD - actual passenger distribution ----

    // Можно оба варианта раскоментарить

    // using standard weights
    // lci.pd.parse("PD.C.П.0/0/0/0.Б.1/0/0/0.Э.4/1/1/1\n");

    // using actual weights
    // lci.pd.parse("PD.Z.A/640.B/2050.C/17804.KG");

    // ---- PD - actual passenger distribution END OF TEST ----

    // lci.sp.parse("SP.5А/M.5Б/F.5В/M.5В/I.5Г/M.5Д/I.5Е/C.1А/M.6А/M");

    lci.dump();

    string lci_data = lci.toXML();

    LogTrace(TRACE5) << "lci_data: " << lci_data;

    TLCIContent parsed_lci;
    parsed_lci.fromXML(lci_data);

    LogTrace(TRACE5) << "parsed_lci: " << parsed_lci.toXML();

    return 1;
}

void TLCIContent::fromDB(int id)
{
    string content;
    AstraContext::GetContext("LCI", id, content);
    AstraContext::ClearContext("LCI", id);
    fromXML(content);
}

void TLCIContent::fromXML(const string &content)
{
    clear();

    // LogTrace(TRACE5) << "TLCIContent::fromXML: content: " << content;

    string converted = ConvertCodepage(content, "CP866", "UTF8");
    XMLDoc lci_data(converted);
    xmlNodePtr rootNode = lci_data.docPtr()->children;
    xml_decode_nodelist(rootNode);

    point_id_tlg = NodeAsInteger("point_id_tlg", rootNode, NoExists);
    time_receive = NodeAsDateTime("time_receive", rootNode, NoExists);
    sender = NodeAsString("sender", rootNode, "");
    typeb_in_id = NodeAsInteger("typeb_in_id", rootNode, NoExists);
    franchise_flt.fromXML(GetNode("franchise_flt", rootNode));
    action_code.fromXML(rootNode);
    req.fromXML(rootNode);
    dst.fromXML(rootNode);
    eqt.fromXML(rootNode);
    wa.fromXML(rootNode);
    sm.fromXML(rootNode);
    sr.fromXML(rootNode);
    wm.fromXML(rootNode);
    pd.fromXML(rootNode);
    sp.fromXML(rootNode);
    si.fromXML(rootNode);
}

string TLCIContent::toXML()
{
    XMLDoc doc("LCIData");
    SetXMLDocEncoding(doc.docPtr(), "cp866");
    xmlNodePtr rootNode = doc.docPtr()->children;
    NewTextChild(rootNode, "point_id_tlg", point_id_tlg, NoExists);
    if(time_receive != NoExists)
        NewTextChild(rootNode, "time_receive", BASIC::date_time::DateTimeToStr(time_receive));
    NewTextChild(rootNode, "sender", sender, "");
    NewTextChild(rootNode, "typeb_in_id", typeb_in_id, NoExists);
    if(not franchise_flt.empty())
        franchise_flt.toXML(NewTextChild(rootNode, "franchise_flt"), boost::none);
    action_code.toXML(rootNode);
    req.toXML(rootNode);
    dst.toXML(rootNode);
    eqt.toXML(rootNode);
    wa.toXML(rootNode);
    sm.toXML(rootNode);
    sr.toXML(rootNode);
    wm.toXML(rootNode);
    pd.toXML(rootNode);
    sp.toXML(rootNode);
    si.toXML(rootNode);
    return GetXMLDocText(doc.docPtr());
}

void TLCIContent::dump()
{
    ProgTrace(TRACE5, "---TLCIContent::dump---");
    action_code.dump();
    req.dump();
    dst.dump();
    eqt.dump();
    wa.dump();
    sm.dump();
    sr.dump();
    wm.dump();
    pd.dump();
    sp.dump();
    si.dump();
    ProgTrace(TRACE5, "-----------------------");
}

void TActionCode::fromXML(xmlNodePtr node)
{
    clear();
    xmlNodePtr curNode = node->children;
    curNode = GetNodeFast("ActionCode", curNode);
    if(not curNode) return;
    curNode = curNode->children;
    orig = DecodeOriginator(*NodeAsStringFast("orig", curNode));
    action = DecodeAction(*NodeAsStringFast("action", curNode));
}

void TActionCode::toXML(xmlNodePtr node)
{
    if(empty()) return;
    xmlNodePtr actionCodeNode = NewTextChild(node, "ActionCode");
    NewTextChild(actionCodeNode, "orig", string(1, EncodeOriginator(orig)));
    NewTextChild(actionCodeNode, "action", string(1, EncodeAction(action)));
}
void TActionCode::clear()
{
    orig = oUnknown;
    action = aUnknown;
}

void TActionCode::dump()
{
    ProgTrace(TRACE5, "---TActionCode::dump---");
    ProgTrace(TRACE5, "orig: %c", EncodeOriginator(orig));
    ProgTrace(TRACE5, "action: %c", EncodeAction(action));
    ProgTrace(TRACE5, "-----------------------");
}

void TActionCode::parse(const char *val)
{
    orig = DecodeOriginator(val[0]);
    action = DecodeAction(val[1]);
    if(orig == oUnknown)
        throw ETlgError("wrong orig %c", val[0]);
    if(action == aUnknown)
        throw ETlgError("wrong action %c", val[1]);
    if(action != aRequest and strlen(val) != 2)
        throw ETlgError("wrong action code '%s'", val);
}

void TLCIFltInfo::dump()
{
  ProgTrace(TRACE5, "-----TLCIFltInfo::dump-----");
  flt.dump();
  ProgTrace(TRACE5, "airp: %s", airp.c_str());
  ProgTrace(TRACE5, "---------------------------");
}

TFltInfo TLCIFltInfo::toFltInfo()
{
    TFltInfo result;
    strcpy(result.airline, flt.airline.c_str());
    result.flt_no=flt.flt_no;
    result.suffix[0] = flt.suffix;
    result.suffix[1] = 0;
    result.scd=flt.date;
    result.pr_utc=true;
    strcpy(result.airp_dep, airp.c_str());
    *result.airp_arv = 0;
    return result;
}

void TLCIFltInfo::parse(const char *val, TFlightsForBind &flts)
{
    string buf = val;
    size_t idx = buf.find('.');
    if(idx == string::npos)
        throw ETlgError("wrong flight format");
    flt.parse(buf.substr(0, idx).c_str());
    TElemFmt fmt;
    string lang;
    string aairp =  buf.substr(idx + 1);
    if(aairp.size() < 3)
        throw ETlgError("wrong airp '%s'", aairp.c_str());
    airp = ElemToElemId(etAirp, aairp, fmt, lang);
    if(airp.empty())
        throw ETlgError("airp '%s' not found", aairp.c_str());

    TTripInfo info;
    info.airline = flt.airline;
    info.airp = airp;
    info.flt_no = flt.flt_no;
    info.suffix = string(1, flt.suffix);
    info.scd_out = flt.date;
    Franchise::TProp franchise_prop;
    if(franchise_prop.get_franchisee(info, Franchise::TPropType::wb) and franchise_prop.val == Franchise::pvYes) {
        franchise_flt.airline = info.airline;
        franchise_flt.flt_no = info.flt_no;
        franchise_flt.suffix = info.suffix;

        flt.airline = franchise_prop.oper.airline;
        flt.flt_no = franchise_prop.oper.flt_no;
        if(franchise_prop.oper.suffix.empty())
            flt.suffix = 0;
        else
            flt.suffix = franchise_prop.oper.suffix[0];
    }

    // привязка к рейсы
    flts.push_back(TFltForBind(toFltInfo(),  btFirstSeg, TSearchFltInfoPtr(new TLCISearchParams())));
}

TTlgPartInfo ParseLCIHeading(TTlgPartInfo heading, TLCIHeadingInfo &info, TFlightsForBind &flts)
{
    TTlgPartInfo next;
    const char *p, *line_p;
    TTlgParser tlg;
    line_p=heading.p;
    try
    {
        do
        {
            if ((p=tlg.GetToEOLLexeme(line_p))==NULL) continue;
            info.flt_info.parse(tlg.lex,flts);
            line_p=tlg.NextLine(line_p);
            return nextPart(heading, line_p);
        }
        while ((line_p=tlg.NextLine(line_p))!=NULL);
    }
    catch(ETlgError E)
    {
        throwTlgError(E.what(), heading, line_p);
    };
    return nextPart(heading, line_p);
};

void TCFG::fromXML(xmlNodePtr node)
{
    clear();
    xmlNodePtr curNode = node->children;
    curNode = GetNodeFast("CFG", curNode);
    if(not curNode) return;
    curNode = curNode->children;
    for(; curNode; curNode = curNode->next) {
        xmlNodePtr curNode2 = curNode->children;
        insert(make_pair(
                    NodeAsStringFast("first", curNode2),
                    NodeAsIntegerFast("second", curNode2)));
    }
}

void TCFG::toXML(xmlNodePtr node) const
{
    if(empty()) return;
    xmlNodePtr cfgNode = NewTextChild(node, "CFG");
    for(const auto &i: *this) {
        xmlNodePtr itemNode = NewTextChild(cfgNode, "item");
        NewTextChild(itemNode, "first", i.first);
        NewTextChild(itemNode, "second", i.second);
    }
}

void TCFG::dump()
{
    ProgTrace(TRACE5, "---TCFG::dump---");
    for(map<string, int>::iterator im = begin(); im != end(); im++) {
        ProgTrace(TRACE5, "cfg[%s] = %d", im->first.c_str(), im->second);
    }
    ProgTrace(TRACE5, "----------------");
}

void TCFG::parse(const string &val, const TElemType el)
{
    if(not empty()) throw ETlgError("cfg already exists");
    string str_cls;
    string str_count;
    for(string::const_iterator is = val.begin(); is != val.end(); is++) {
        if(IsUpperLetter(*is))
        {
            if(not str_count.empty()) {
                if(str_cls.empty())
                    throw ETlgError("wrong CFG %s", val.c_str());
                insert(make_pair(getElemId(str_cls, el), ToInt(str_count)));
                str_count.erase();
            }
            str_cls = *is;
        } else {
            str_count.append(1, *is);
        }
    }
    if(str_cls.empty() or str_count.empty())
        throw ETlgError("wrong CFG %s", val.c_str());
    insert(make_pair(getElemId(str_cls, el), ToInt(str_count)));
}

void TEQT::clear()
{
    bort.clear();
    craft.clear();
    cfg.clear();
}

void TEQT::fromXML(xmlNodePtr node)
{
    clear();
    xmlNodePtr curNode = node->children;
    curNode = GetNodeFast("EQT", curNode);
    if(not curNode) return;
    xmlNodePtr eqtNode = curNode;
    curNode = curNode->children;
    bort = NodeAsStringFast("bort", curNode, "");
    craft = NodeAsStringFast("craft", curNode, "");
    cfg.fromXML(eqtNode);
}

void TEQT::toXML(xmlNodePtr node)
{
    if(empty()) return;
    xmlNodePtr eqtNode = NewTextChild(node, "EQT");
    NewTextChild(eqtNode, "bort", bort, "");
    NewTextChild(eqtNode, "craft", craft, "");
    cfg.toXML(eqtNode);

}

void TEQT::dump()
{
    ProgTrace(TRACE5, "---TEQT::dump---");
    ProgTrace(TRACE5, "bort: %s", bort.c_str());
    ProgTrace(TRACE5, "craft: %s", craft.c_str());
    cfg.dump();
    ProgTrace(TRACE5, "----------------");
}

void TEQT::parse(const char *val)
{
    if(not cfg.empty()) throw ETlgError("multiple EQT found");
    string acraft;
    vector<string> items;
    split(items, val, '.');
    if(items.size() == 4) {
        bort = items[1];
        acraft = items[2];
        cfg.parse(items[3], etSubcls);
        if(bort.empty()) throw ETlgError("bort is empty");
    } else if(items.size() == 3) {
        acraft = items[1];
        cfg.parse(items[2], etSubcls);
    } else
        throw ETlgError("EQT: wrong format '%s'", val);
    if(acraft.empty()) throw ETlgError("craft is empty");
    TElemFmt fmt;
    string lang;
    craft = ElemToElemId(etCraft, acraft, fmt, lang);
    if(craft.empty()) throw ETlgError("craft not found: '%s'", acraft.c_str());
}

void TWA::clear()
{
    payload.clear();
    underload.clear();
}

void TWA::dump()
{
    ProgTrace(TRACE5, "---TWA::dump---");
    if(payload.amount == NoExists)
        ProgTrace(TRACE5, "payload: NoExists");
    else
        ProgTrace(TRACE5, "payload: %d %s", payload.amount, EncodeMeasur(payload.measur));
    if(underload.amount == NoExists)
        ProgTrace(TRACE5, "underload: NoExists");
    else
        ProgTrace(TRACE5, "underload: %d %s", underload.amount, EncodeMeasur(underload.measur));
    ProgTrace(TRACE5, "---------------");
}

void TWeight::fromXML(xmlNodePtr node, const string &tag)
{
    clear();
    xmlNodePtr curNode = node->children;
    curNode = GetNodeFast(tag.c_str(), curNode);
    if(not curNode) return;
    curNode = curNode->children;
    amount = NodeAsIntegerFast("amount", curNode, NoExists);
    measur = DecodeMeasur(NodeAsStringFast("measur", curNode, ""));
}

void TWeight::toXML(xmlNodePtr node, const string &tag) const
{
    if(empty()) return;
    xmlNodePtr weightNode = NewTextChild(node, tag.c_str());
    NewTextChild(weightNode, "amount", amount, NoExists);
    NewTextChild(weightNode, "measur", EncodeMeasur(measur), "");
}

void TWA::fromXML(xmlNodePtr node)
{
    clear();
    xmlNodePtr curNode = node->children;
    curNode = GetNodeFast("WA", curNode);
    if(not curNode) return;
    payload.fromXML(curNode, "payload");
    underload.fromXML(curNode, "underload");
}

void TWA::toXML(xmlNodePtr node) const
{
    if(empty()) return;
    xmlNodePtr waNode = NewTextChild(node, "WA");
    payload.toXML(waNode, "payload");
    underload.toXML(waNode, "underload");
}

void TWA::parse(const char *val)
{
    vector<string> items;
    split(items, val, '.');
    if(items.size() != 4)
        throw ETlgError("wrong WA %s", val);
    if(items[1] == "P") {
        if(payload.amount != NoExists)
            throw ETlgError("multiple WA.P found");
        payload.amount = ToInt(items[2]);
        payload.measur = DecodeMeasur(items[3].c_str());
        if(payload.measur == mUnknown)
            throw ETlgError("unknown unit of measurment %s", val);
    } else if(items[1] == "U") {
        if(underload.amount != NoExists)
            throw ETlgError("multiple WA.U found");
        underload.amount = ToInt(items[2]);
        underload.measur = DecodeMeasur(items[3].c_str());
        if(underload.measur == mUnknown)
            throw ETlgError("unknown unit of measurment %s", val);
    } else
        throw ETlgError("WA: wrong indication payload/underload %s", items[1].c_str());
}

void TSM::fromXML(xmlNodePtr node)
{
    clear();
    xmlNodePtr curNode = node->children;
    curNode = GetNodeFast("SM", curNode);
    if(not curNode) return;
    value = DecodeSeatingMethod(string(NodeAsString(curNode))[0]);
}

void TSM::toXML(xmlNodePtr node) const
{
    if(value != smUnknown)
        NewTextChild(node, "SM", string(1, EncodeSeatingMethod(value)));
}

void TSM::dump()
{
    ProgTrace(TRACE5, "---TSM::dump---");
    ProgTrace(TRACE5, "sm.value %c", EncodeSeatingMethod(value));
    ProgTrace(TRACE5, "---------------");
}

void TSM::parse(const char *val)
{
    if(value != smUnknown) throw ETlgError("multiple SM found");
    vector<string> items;
    split(items, val, '.');
    if(items.size() != 2) throw ETlgError("wrong SM %s", val);
    if(items[1].size() != 1) throw ETlgError("wrong SM %s", val);
    value = DecodeSeatingMethod(items[1][0]);
    if(value == smUnknown) throw ETlgError("wrong SM %s", val);
}

void TSRZones::fromXML(xmlNodePtr node)
{
    clear();
    xmlNodePtr curNode = node->children;
    curNode = GetNodeFast("SRZones", curNode);
    if(not curNode) return;
    curNode = curNode->children;
    for(; curNode; curNode = curNode->next) {
        xmlNodePtr curNode2 = curNode->children;
        insert(make_pair(
                    NodeAsStringFast("first", curNode2),
                    NodeAsIntegerFast("second", curNode2)));
    }
}

void TSRZones::toXML(xmlNodePtr node) const
{
    if(empty()) return;
    xmlNodePtr SRZonesNode = NewTextChild(node, "SRZones");
    for(const auto &i: *this) {
        xmlNodePtr itemNode = NewTextChild(SRZonesNode, "item");
        NewTextChild(itemNode, "first", i.first);
        NewTextChild(itemNode, "second", i.second);
    }
}

void TSRZones::dump()
{
    ProgTrace(TRACE5, "---TSRZones::dump---");
    for(map<string, int>::iterator im = begin(); im != end(); im++)
        ProgTrace(TRACE5, "z[%s] = %d", im->first.c_str(), im->second);
    ProgTrace(TRACE5, "--------------------");
}

void TSRZones::parse(const string &val)
{
    if(not empty())
        throw ETlgError("SR zones already exists");
    vector<string> items;
    split(items, val, '/');
    for(vector<string>::iterator iv = items.begin(); iv != items.end(); iv++) {
        if(iv->size() < 3) throw ETlgError("SR wrong zone format %s", iv->c_str());
        insert(make_pair(iv->substr(0, 2), ToInt(iv->substr(2))));
    }
}

void TSR::clear()
{
    type = srUnknown;
    format = 0;
    c.clear();
    z.clear();
    r.clear();
    s.clear();
    j.clear();
}

void TSR::dump()
{
    ProgTrace(TRACE5, "---TSR::dump---");
    c.dump();
    z.dump();
    r.dump();
    s.dump();
    j.dump();
    ProgTrace(TRACE5, "---------------");
}

void TSRItems::fromXML(xmlNodePtr node, const std::string &tag)
{
    clear();
    xmlNodePtr curNode = node->children;
    curNode = GetNodeFast(tag.c_str(), curNode);
    if(not curNode) return;
    curNode = curNode->children;
    for(; curNode; curNode = curNode->next)
        push_back(NodeAsString(curNode));
}

void TSRItems::toXML(xmlNodePtr node, const std::string &tag) const
{
    if(empty()) return;
    xmlNodePtr SRItemsNode = NewTextChild(node, tag.c_str());
    for(const auto &i: *this)
        NewTextChild(SRItemsNode, "item", i);
}

void TSRItems::dump()
{
    ProgTrace(TRACE5, "---TSRItems::dump---");
    size_t count = 0;
    for(vector<string>::iterator iv = begin(); iv != end(); iv++)
        ProgTrace(TRACE5, "row %zu: %s", count++, iv->c_str());
    ProgTrace(TRACE5, "-------------------");
}

void TSRItems::parse(const string &val)
{
    vector<string> result;
    split(result, val, '/');
    for(vector<string>::iterator iv = result.begin(); iv != result.end(); iv++)
        if(not iv->empty())
            push_back(*iv);
}

void TSRJump::fromXML(xmlNodePtr node)
{
    clear();
    xmlNodePtr curNode = node->children;
    curNode = GetNodeFast("SRJump", curNode);
    if(not curNode) return;
    xmlNodePtr SRJumpNode = curNode;
    curNode = curNode->children;
    amount = NodeAsIntegerFast("amount", curNode);
    xmlNodePtr seatsNode = GetNodeFast("seats", curNode);
    if(seatsNode) {
        xmlNodePtr itemNode = seatsNode->children;
        for(; itemNode; itemNode = itemNode->next) {
            seats.push_back(NodeAsString(itemNode));
        }
    }
    zones.fromXML(SRJumpNode);
}

void TSRJump::toXML(xmlNodePtr node) const
{
    if(empty()) return;
    xmlNodePtr SRJumpNode = NewTextChild(node, "SRJump");
    NewTextChild(SRJumpNode, "amount", amount, NoExists);
    if(not seats.empty()) {
        xmlNodePtr seatsNode = NewTextChild(SRJumpNode, "seats");
        for(const auto &i: seats)
            NewTextChild(seatsNode, "item", i);
    }
    zones.toXML(SRJumpNode);
}

bool TSRJump::empty() const
{
    return
        amount == NoExists and
        seats.empty() and
        zones.empty();
}

void TSRJump::clear()
{
    amount = NoExists;
    seats.clear();
    zones.clear();
}

void TSRJump::dump()
{
    ProgTrace(TRACE5, "---TSRJump::dump---");
    ProgTrace(TRACE5, "amount: %d", amount);
    size_t count = 0;
    for(vector<string>::iterator iv = seats.begin(); iv != seats.end(); iv++)
        ProgTrace(TRACE5, "seat[%zu] = '%s'", count++, iv->c_str());
    zones.dump();
    ProgTrace(TRACE5, "-------------------");
}

void TSRJump::parse(const char *val)
{
    if(amount != NoExists)
        throw ETlgError("SR.J already exists");
    vector<string> items;
    split(items, val, '/');
    if(items.size() > 2)
        throw ETlgError("SR.J: too manu '/' %s", val);
    vector<string> buf;
    split(buf, items[0], '.');
    if(buf.size() != 3)
        throw ETlgError("Wrong SR.J %s", val);
    amount = ToInt(buf[2]);
    if(items.size() > 1) {
        split(buf, items[1], '.');
        if(buf.size() < 2)
            throw ETlgError("Wrong SR.J %s", val);
        if(buf[0].size() != 1)
            throw ETlgError("SR.J: wrong specifier %s in %s", buf[0].c_str(), val);
        switch(buf[0][0]) {
            case 'S':
                {
                    int seat_count = 0;
                    for(vector<string>::iterator iv = buf.begin() + 1; iv != buf.end(); iv++, seat_count++)
                        seats.push_back(*iv);
                    if(seat_count != amount)
                        throw ETlgError("SR.J: seat count <> total amount");
                }
                break;
            case 'Z':
                {
                    string str_zones;
                    for(vector<string>::iterator iv = buf.begin() + 1; iv != buf.end(); iv++) {
                        if(not str_zones.empty())
                            str_zones.append(1, '/');
                        str_zones += *iv;
                    }
                    zones.parse(str_zones);
                    int zone_seat_count = 0;
                    for(TSRZones::iterator iz = zones.begin(); iz != zones.end(); iz++)
                        zone_seat_count += iz->second;
                    if(zone_seat_count != amount)
                        throw ETlgError("SR.J: zone seat count <> total amount");
                }
                break;
            default:
                throw ETlgError("SR.J: unknown specifier %c in %s", buf[0][0], val);
        }
    }
}

void TSR::parse(const char *val)
{
    vector<string> items;
    split(items, val, '.');
    if(items.size() >= 2 and items[1] == "WB") {
        type = srWB;
        items.erase(items.begin() + 1);
    } else
        type = srStd;
    LogTrace(TRACE5) << "SR type: " << (type == srStd ? "srStd" : "srWB");
    string data;
    if(items.size() > 3)
        throw ETlgError("wrong item count within SR %s", val);
    if(items.size() == 3)
        data = items[2];
    if(items[1].size() != 1)
        throw ETlgError("SR wrong type %s", items[1].c_str());
    format = items[1][0];
    switch(format) {
        case 'C':
            c.parse(data, etSubcls);
            break;
        case 'Z':
            z.parse(data);
            break;
        case 'R':
            r.parse(data);
            break;
        case 'S':
            s.parse(data);
            break;
        case 'J':
            j.parse(val);
            break;
        default:
            throw ETlgError("SR unknown type %c", items[1][0]);
    }
}

bool TWM::find_item(TWMDesignator desig, TWMType type)
{
    bool result = false;
    TWMMap::iterator i_type_map = find(desig);
    if(i_type_map != end()) {
        TWMTypeMap::iterator i_sub_type_map = i_type_map->second.find(type);
        if(i_sub_type_map != i_type_map->second.end())
            result = true;
    }
    return result;
}

bool TClsGenderWeight::empty() const
{
    return
        TSubTypeHolder::empty() and
        std::map<std::string, TGenderCount>::empty();
}

void TClsGenderWeight::clear()
{
    TSubTypeHolder::clear();
    std::map<std::string, TGenderCount>::clear();
}

xmlNodePtr TClsGenderWeight::fromXML(xmlNodePtr node)
{
    clear();
    xmlNodePtr result = TSubTypeHolder::fromXML(node);
    if(result) {
        xmlNodePtr curNode = result->children;
        curNode = GetNodeFast("ClsGenderWeight", curNode);
        if(curNode) {
            curNode = curNode->children;
            for(; curNode; curNode = curNode->next) {
                xmlNodePtr curNode2 = curNode->children;
                TGenderCount gender_count;
                gender_count.fromXML(curNode);
                insert(make_pair(
                            NodeAsStringFast("first", curNode2),
                            gender_count));
            }
        }
    }
    return result;
}

xmlNodePtr TClsGenderWeight::toXML(xmlNodePtr node) const
{
    if(empty()) return NULL;

    xmlNodePtr result = TSubTypeHolder::toXML(node);
    xmlNodePtr ClsGenderWeightNode = NewTextChild(result, "ClsGenderWeight");
    for(const auto &i: *this) {
        xmlNodePtr itemNode = NewTextChild(ClsGenderWeightNode, "item");
        NewTextChild(itemNode, "first", i.first);
        i.second.toXML(itemNode);
    }
    return result;
}

void TClsGenderWeight::dump()
{
    ProgTrace(TRACE5, "---TClsGenderWeight::dump---");
    TSubTypeHolder::dump();
    for(map<string, TGenderCount>::iterator im = begin(); im != end(); im++) {
        ProgTrace(TRACE5, "subcls: %s", im->first.c_str());
        im->second.dump();
    }
    ProgTrace(TRACE5, "----------------------------");
}

void TClsGenderWeight::parse(const std::vector<std::string> &val)
{
    for(vector<string>::const_iterator iv = val.begin(); iv != val.end(); iv++) {
        if(iv->size() < 2)
            throw ETlgError("wrong cls gender item %s", iv->c_str());
        vector<string> genders(1, iv->substr(1));
        TGenderCount g;
        g.parse(genders);
        insert(make_pair(getElemId(iv->substr(0, 1), etSubcls), g));
    }
}

bool TClsWeight::empty() const
{
    return
        TSubTypeHolder::empty() and
        f == NoExists and
        c == NoExists and
        y == NoExists;
}

void TClsWeight::clear()
{
    TSubTypeHolder::clear();
    f = NoExists;
    c = NoExists;
    y = NoExists;
}

xmlNodePtr TClsWeight::fromXML(xmlNodePtr node)
{
    clear();
    xmlNodePtr result = TSubTypeHolder::fromXML(node);
    if(result) {
        xmlNodePtr curNode = result->children;
        curNode = GetNodeFast("ClsWeight", curNode);
        if(curNode) {
            curNode = curNode->children;
            f = NodeAsIntegerFast("f", curNode, NoExists);
            c = NodeAsIntegerFast("c", curNode, NoExists);
            y = NodeAsIntegerFast("y", curNode, NoExists);
        }
    }
    return result;
}

xmlNodePtr TClsWeight::toXML(xmlNodePtr node) const
{
    if(empty()) return NULL;

    xmlNodePtr result = TSubTypeHolder::toXML(node);
    xmlNodePtr itemNode = NewTextChild(result, "ClsWeight");
    NewTextChild(itemNode, "f", f, NoExists);
    NewTextChild(itemNode, "c", c, NoExists);
    NewTextChild(itemNode, "y", y, NoExists);
    return result;
}

void TClsWeight::dump()
{
    ProgTrace(TRACE5, "---TClsWeight::dump---");
    TSubTypeHolder::dump();
    ProgTrace(TRACE5, "f: %d, c: %d, y: %d", f, c, y);
    ProgTrace(TRACE5, "----------------------");
}

void TClsWeight::parse(const std::vector<std::string> &val)
{
    if(val.size() != 1) {
        ostringstream buf;
        copy(val.begin(), val.end(), ostream_iterator<string>(buf, "."));
        throw ETlgError("WM wrong cls weight %s", buf.str().c_str());
    }
    vector<string> items;
    split(items, val[0], '/');
    if(items.size() != 3)
        throw ETlgError("WM wrong cls weight amount %s", val[0].c_str());
    f = ToInt(items[0]);
    c = ToInt(items[1]);
    y = ToInt(items[2]);

}

bool TClsBagWeight::empty() const
{
    return
        TSubTypeHolder::empty() and
        std::map<std::string, int>::empty();
}

void TClsBagWeight::clear()
{
    TSubTypeHolder::clear();
    std::map<std::string, int>::clear();
}

xmlNodePtr TClsBagWeight::fromXML(xmlNodePtr node)
{
    clear();
    xmlNodePtr result = TSubTypeHolder::fromXML(node);
    if(result) {
        xmlNodePtr curNode = result->children;
        curNode = GetNodeFast("ClsBagWeight", curNode);
        if(curNode) {
            curNode = curNode->children;
            for(; curNode; curNode = curNode->next) {
                xmlNodePtr curNode2 = curNode->children;
                insert(make_pair(
                            NodeAsStringFast("first", curNode2),
                            NodeAsIntegerFast("second", curNode2)));
            }
        }
    }
    return result;
}

xmlNodePtr TClsBagWeight::toXML(xmlNodePtr node) const
{
    if(empty()) return NULL;

    xmlNodePtr result = TSubTypeHolder::toXML(node);
    xmlNodePtr ClsBagWeightNode = NewTextChild(result, "ClsBagWeight");
    for(const auto &i: *this) {
        xmlNodePtr itemNode = NewTextChild(ClsBagWeightNode, "item");
        NewTextChild(itemNode, "first", i.first);
        NewTextChild(itemNode, "second", i.second);
    }
    return result;
}

void TClsBagWeight::dump()
{
    ProgTrace(TRACE5, "---TClsBagWeight::dump---");
    TSubTypeHolder::dump();
    for(map<string, int>::iterator im = begin(); im != end(); im++)
        ProgTrace(TRACE5, "%s[%d]", im->first.c_str(), im->second);
    ProgTrace(TRACE5, "-------------------------");
}

void TClsBagWeight::parse(const std::vector<std::string> &val)
{
    for(vector<string>::const_iterator iv = val.begin(); iv != val.end(); iv++) {
        if(iv->size() < 2)
            throw ETlgError("wrong cls item %s", iv->c_str());
        insert(make_pair(getElemId(iv->substr(0, 1), etSubcls), ToInt(iv->substr(1))));
    }
}

bool TGenderCount::empty() const
{
    return
        TSubTypeHolder::empty() and
        m == NoExists and
        f == NoExists and
        c == NoExists and
        i == NoExists;
}

void TGenderCount::clear()
{
    TSubTypeHolder::clear();
    m = NoExists;
    f = NoExists;
    c = NoExists;
    i = NoExists;
}

xmlNodePtr TGenderCount::fromXML(xmlNodePtr node)
{
    clear();
    xmlNodePtr result = TSubTypeHolder::fromXML(node);
    if(result) {
        xmlNodePtr curNode = result->children;
        curNode = GetNodeFast("GenderCount", curNode);
        if(curNode) {
            curNode = curNode->children;
            m = NodeAsIntegerFast("m", curNode, NoExists);
            f = NodeAsIntegerFast("f", curNode, NoExists);
            c = NodeAsIntegerFast("c", curNode, NoExists);
            i = NodeAsIntegerFast("i", curNode, NoExists);
        }
    }
    return result;
}

xmlNodePtr TGenderCount::toXML(xmlNodePtr node) const
{
    if(empty()) return NULL;

    xmlNodePtr result = TSubTypeHolder::toXML(node);
    xmlNodePtr genderCountNode = NewTextChild(result, "GenderCount");
    NewTextChild(genderCountNode, "m", m, NoExists);
    NewTextChild(genderCountNode, "f", f, NoExists);
    NewTextChild(genderCountNode, "c", c, NoExists);
    NewTextChild(genderCountNode, "i", i, NoExists);
    return result;
}

void TGenderCount::dump()
{
    ProgTrace(TRACE5, "---TGenderCount::dump---");
    TSubTypeHolder::dump();
    ProgTrace(TRACE5, "m: %d, f: %d, c: %d, i: %d", m, f, c, i);
    ProgTrace(TRACE5, "-------------------------");
}

void TGenderCount::parse(const std::vector<std::string> &val)
{
    if(val.size() != 1) {
        ostringstream buf;
        copy(val.begin(), val.end(), ostream_iterator<string>(buf, "."));
        throw ETlgError("WM wrong gender weight %s", buf.str().c_str());
    }
    vector<string> items;
    split(items, val[0], '/');
    if(items.size() != 4)
        throw ETlgError("WM wrong gender weight items count %s", val[0].c_str());
    m = ToInt(items[0]);
    f = ToInt(items[1]);
    c = ToInt(items[2]);
    i = ToInt(items[3]);
}

bool TSimpleWeight::empty() const
{
    return
        TSubTypeHolder::empty() and
        weight == NoExists;
}

void TSimpleWeight::clear()
{
    TSubTypeHolder::clear();
    weight = NoExists;
}

xmlNodePtr TSimpleWeight::fromXML(xmlNodePtr node)
{
    clear();
    xmlNodePtr result = TSubTypeHolder::fromXML(node);
    if(result) {
        xmlNodePtr curNode = result->children;
        weight = NodeAsIntegerFast("SimpleWeight", curNode, NoExists);
    }
    return result;
}

xmlNodePtr TSimpleWeight::toXML(xmlNodePtr node) const
{
    if(empty()) return NULL;

    xmlNodePtr result = TSubTypeHolder::toXML(node);
    NewTextChild(result, "SimpleWeight", weight, NoExists);
    return result;
}

void TSimpleWeight::dump()
{
    ProgTrace(TRACE5, "---TSimpleWeight::dump---");
    TSubTypeHolder::dump();
    ProgTrace(TRACE5, "weight: %d", weight);
    ProgTrace(TRACE5, "-------------------------");
}

void TSimpleWeight::parse(const std::vector<std::string> &val)
{
    if(val.size() != 1) {
        ostringstream buf;
        copy(val.begin(), val.end(), ostream_iterator<string>(buf, "."));
        throw ETlgError("WM wrong weight %s", buf.str().c_str());
    }
    weight = ToInt(val[0]);
}

bool TSubTypeHolder::empty() const
{
    return
        sub_type == wmsUnknown and
        measur == mUnknown;
}

void TSubTypeHolder::clear()
{
    sub_type = wmsUnknown;
    measur = mUnknown;
}

xmlNodePtr TSubTypeHolder::fromXML(xmlNodePtr node)
{
    clear();
    xmlNodePtr curNode = node->children;
    xmlNodePtr result = GetNodeFast("SubTypeHolder", curNode);
    if(result) {
        curNode = result->children;
        sub_type = DecodeWMSubType(NodeAsStringFast("WMSubType", curNode, ""));
        measur = DecodeMeasur(NodeAsStringFast("Measur", curNode, ""));
    }
    return result;
}

xmlNodePtr TSubTypeHolder::toXML(xmlNodePtr node) const
{
    xmlNodePtr SubTypeHolderNode = NewTextChild(node, "SubTypeHolder");
    NewTextChild(SubTypeHolderNode, "WMSubType", EncodeWMSubType(sub_type), "");
    NewTextChild(SubTypeHolderNode, "Measur", EncodeMeasur(measur), "");
    return SubTypeHolderNode;
}

void TSubTypeHolder::dump()
{
    ProgTrace(TRACE5, "sub_type: %s", EncodeWMSubType(sub_type));
    ProgTrace(TRACE5, "measur: %s", EncodeMeasur(measur));
};

void TWM::dump()
{
    ProgTrace(TRACE5, "---TWM::dump---");
    for(TWMMap::iterator i_desig = begin(); i_desig != end(); i_desig++) {
        for(TWMTypeMap::iterator i_type = i_desig->second.begin(); i_type != i_desig->second.end(); i_type++) {
            ProgTrace(TRACE5, "TWM[%s][%s]", EncodeWMDesignator(i_desig->first), EncodeWMType(i_type->first));
            i_type->second->dump();
        }
    }
    ProgTrace(TRACE5, "---------------");
}

void TWM::fromXML(xmlNodePtr node)
{
    clear();
    xmlNodePtr curNode = node->children;
    curNode = GetNodeFast("WM", curNode);
    if(not curNode) return;
    curNode = curNode->children;
    for(; curNode; curNode = curNode->next) {
        xmlNodePtr curNode2 = curNode->children;
        TWMDesignator desig = DecodeWMDesignator(NodeAsStringFast("desig", curNode2));
        xmlNodePtr WMTypeMapNode = NodeAsNodeFast("WMTypeMap", curNode2);
        xmlNodePtr WMTypeMapItemNode = WMTypeMapNode->children;
        TWMTypeMap type_map;
        for(; WMTypeMapItemNode; WMTypeMapItemNode = WMTypeMapItemNode->next) {
            xmlNodePtr WMTypeMapItemDataNode = WMTypeMapItemNode->children;
            TWMType type = DecodeWMType(NodeAsStringFast("type", WMTypeMapItemDataNode));
            xmlNodePtr sthNode = NodeAsNodeFast("SubTypeHolder", WMTypeMapItemDataNode);
            if(sthNode) {
                xmlNodePtr sthDataNode = sthNode->children;
                TWMSubType sub_type = DecodeWMSubType(NodeAsStringFast("WMSubType", sthDataNode, ""));
                tr1::shared_ptr<TSubTypeHolder> sth;
                switch(sub_type) {
                    case wmsGender:
                        sth = tr1::shared_ptr<TSubTypeHolder>(new TGenderCount);
                        break;
                    case wmsClass:
                        if(desig == wmdStandard and type == wmtBag)
                            sth = tr1::shared_ptr<TSubTypeHolder>(new TClsBagWeight);
                        else
                            sth = tr1::shared_ptr<TSubTypeHolder>(new TClsWeight);
                        break;
                    case wmsClsGender:
                        sth = tr1::shared_ptr<TSubTypeHolder>(new TClsGenderWeight);
                        break;
                    case wmsUnknown:
                        sth = tr1::shared_ptr<TSubTypeHolder>(new TSimpleWeight);
                        break;
                }
                sth->fromXML(WMTypeMapItemNode);
                type_map.insert(make_pair(type, sth));
            }
        }
        insert(make_pair(desig, type_map));
    }
}

void TWM::toXML(xmlNodePtr node) const
{
    if(empty()) return;
    xmlNodePtr wmNode = NewTextChild(node, "WM");
    for(const auto &designator: *this) {
        xmlNodePtr itemNode = NewTextChild(wmNode, "item");
        NewTextChild(itemNode, "desig", EncodeWMDesignator(designator.first));
        if(not designator.second.empty()) {
            xmlNodePtr wmTypeMapNode = NewTextChild(itemNode, "WMTypeMap");
            for(const auto &type: designator.second) {
                xmlNodePtr typeItemNode = NewTextChild(wmTypeMapNode, "item");
                NewTextChild(typeItemNode, "type", EncodeWMType(type.first), "");
                type.second->toXML(typeItemNode);
            }
        }
    }
}

bool TWM::parse_wb(const char *val)
{
    vector<string> by_slashes;
    split(by_slashes, val, '/');
    bool result = by_slashes.size() > 1;
    if(result) {
        vector<string> by_periods;
        split(by_periods, val, '.');
        if(by_periods.size() < 2)
            throw Exception("TWM::parse_wb can't parse %s", val);
        for(vector<string>::const_iterator
                item = by_slashes.begin();
                item != by_slashes.end();
                item++) {
            string item_to_parse;
            if(item != by_slashes.begin())
                item_to_parse = by_periods[0] + "." + by_periods[1] + ".";
            item_to_parse += *item;
            if(item + 1 != by_slashes.end())
                item_to_parse += (string)"." + by_periods.back();
            parse(item_to_parse.c_str());
        }
    }
    return result;
}

void TWM::parse(const char *val)
{
    vector<string> items;
    split(items, val, '.');
    if(items.size() < 4)
        throw ETlgError("WM wrong fomrat %s", val);
    TWMDesignator desig = DecodeWMDesignator(items[1]);
    if(desig == wmdUnknown) throw ETlgError("unknown WM designator %s", items[1].c_str());

    TWMType type = DecodeWMType(items[2]);
    if(type == wmtUnknown) throw ETlgError("unknown WM type %s", items[2].c_str());

    if(find_item(wmdStandard, type) or find_item(wmdActual, type))
        throw ETlgError("duplicate WM rows found %s", val);


    tr1::shared_ptr<TSubTypeHolder> sth;
    TMeasur measur = DecodeMeasur(items.back().c_str());
    if(measur == mUnknown)
        throw ETlgError("unknown WM measur unit %s", items.back().c_str());
    items.erase(items.begin(), items.begin() + 3); // drop keyword WM + designator + type
    items.pop_back(); // remove measur info
    if(not items.empty()) {
        // В оставшихся элементах содержится инфа по sub_type
        TWMSubType sub_type = DecodeWMSubType(items[0]);
        if(sub_type == wmsUnknown) {
            // парсинг строки вида WM.WB.TT.4257/PT.2000/HT.150/BT.1000/CT.1100/MT.7.KG
            if(parse_wb(val)) return;
            sth = tr1::shared_ptr<TSubTypeHolder>(new TSimpleWeight);
        } else {
            items.erase(items.begin(), items.begin() + 1); // избавляемся от идентификатора sub_type
            if(not items.empty())
                switch(sub_type) {
                    case wmsGender:
                        sth = tr1::shared_ptr<TSubTypeHolder>(new TGenderCount);
                        break;
                    case wmsClass:
                        if(desig == wmdStandard and type == wmtBag)
                            sth = tr1::shared_ptr<TSubTypeHolder>(new TClsBagWeight);
                        else
                            sth = tr1::shared_ptr<TSubTypeHolder>(new TClsWeight);
                        break;
                    case wmsClsGender:
                        sth = tr1::shared_ptr<TSubTypeHolder>(new TClsGenderWeight);
                        break;
                    case wmsUnknown:
                        break;
                };
        }
        sth->parse(items);
        sth->sub_type = sub_type;
    }
    if(sth == NULL)
        sth = tr1::shared_ptr<TSubTypeHolder>(new TSimpleWeight);
    sth->measur = measur;
    (*this)[desig][type] = sth;
}

bool TPDItem::empty() const
{
    return actual.empty() and standard.empty();
}

void TPDItem::fromXML(xmlNodePtr node)
{
    clear();
    xmlNodePtr curNode = node->children;
    curNode = GetNodeFast("PDItem", curNode);
    if(not curNode) return;
    actual.fromXML(curNode, "actual");
    standard.fromXML(curNode);

}

void TPDItem::toXML(xmlNodePtr node) const
{
    if(empty()) return;
    xmlNodePtr pdItemNode = NewTextChild(node, "PDItem");
    actual.toXML(pdItemNode, "actual");
    standard.toXML(pdItemNode);
}

void TPDItem::dump()
{
    ProgTrace(TRACE5, "---TPDItem::dump---");
    if(actual.amount != NoExists)
        ProgTrace(TRACE5, "actual: %d, %s", actual.amount, EncodeMeasur(actual.measur));
    else
        standard.dump();
    ProgTrace(TRACE5, "-------------------");
}

string TPDParser::getKey(const string &val, TPDType type)
{
    string result;
    switch(type) {
        case pdtClass:
            result = getElemId(val, etSubcls);
            break;
        default:
            result = val;
            break;
    }
    return result;
}

void TPDParser::clear()
{
    type = pdtUnknown;
    std::map<std::string, TPDItem>::clear();
}

void TPDParser::dump()
{
    ProgTrace(TRACE5, "---TPDParser::dump---");
    ProgTrace(TRACE5, "type: %s", EncodePDType(type));
    for(map<string, TPDItem>::iterator im = begin(); im != end(); im++) {
        ProgTrace(TRACE5, "map[%s]", im->first.c_str());
        im->second.dump();
    }
    ProgTrace(TRACE5, "---------------------");
}

void TPDParser::fromXML(xmlNodePtr node)
{
    clear();
    xmlNodePtr curNode = node->children;
    curNode = GetNodeFast("PDParser", curNode);
    if(not curNode) return;
    curNode = curNode->children;
    type = DecodePDType(NodeAsStringFast("PDType", curNode));
    xmlNodePtr PDParserMapNode = NodeAsNodeFast("PDParserMap", curNode);
    if(PDParserMapNode) {
        xmlNodePtr PDParserMapItemNode = PDParserMapNode->children;
        for(; PDParserMapItemNode; PDParserMapItemNode = PDParserMapItemNode->next) {
            xmlNodePtr PDParserMapItemDataNode = PDParserMapItemNode->children;
            TPDItem PDItem;
            PDItem.fromXML(PDParserMapItemNode);
            insert(make_pair(
                        NodeAsStringFast("first", PDParserMapItemDataNode),
                        PDItem));
        }
    }
}

void TPDParser::toXML(xmlNodePtr node) const
{
    if(empty()) return;
    xmlNodePtr pdParserNode = NewTextChild(node, "PDParser");
    NewTextChild(pdParserNode, "PDType", EncodePDType(type));
    xmlNodePtr pdParserMapNode = NewTextChild(pdParserNode, "PDParserMap");
    for(const auto &i: *this) {
        xmlNodePtr itemNode = NewTextChild(pdParserMapNode, "item");
        NewTextChild(itemNode, "first", i.first);
        i.second.toXML(itemNode);
    }
}

void TPDParser::parse(TPDType atype, const vector<string> &val)
{
    type = atype;
    TMeasur measur = DecodeMeasur(val.back().c_str());
    if(measur == mUnknown) {
        // using standard weights
        vector<string>::const_iterator iv = val.begin() + 2; // skip PD.C
        while(iv != val.end()) {
            string key = getKey(*iv, type);
            iv++;
            if(iv == val.end())
                throw ETlgError("unexpected end of line");
            TPDItem item;
            item.standard.parse(vector<string>(1, *iv));
            insert(make_pair(key, item));
            iv++;
        }
    } else {
        // using actual weights
        for(vector<string>::const_iterator iv = val.begin() + 2; iv != val.end() - 1; iv++) // skip PD.C, KG
        {
            vector <string> items;
            split(items, *iv, '/');
            if(items.size() != 2)
                throw ETlgError("wrong PD actual weight format %s", iv->c_str());
            TPDItem item;
            item.actual.amount = ToInt(items[1]);
            item.actual.measur = measur;
            insert(make_pair(getKey(items[0], type), item));
        }
    }
}

void TPD::dump()
{
    ProgTrace(TRACE5, "---TPD::dump---");
    for(map<TPDType, TPDParser>::iterator im = begin(); im != end(); im++) {
        ProgTrace(TRACE5, "PD[%s]", EncodePDType(im->first));
        im->second.dump();
    }
    ProgTrace(TRACE5, "---------------");
}

void TPD::fromXML(xmlNodePtr node)
{
    clear();
    xmlNodePtr curNode = node->children;
    curNode = GetNodeFast("PD", curNode);
    if(not curNode) return;
    curNode = curNode->children;
    for(; curNode; curNode = curNode->next) {
        xmlNodePtr curNode2 = curNode->children;
        TPDParser PDParser;
        PDParser.fromXML(curNode);
        insert(make_pair(
                    DecodePDType(NodeAsStringFast("first", curNode2)),
                    PDParser));

    }
}

void TPD::toXML(xmlNodePtr node) const
{
    if(empty()) return;
    xmlNodePtr pdNode = NewTextChild(node, "PD");
    for(const auto &i: *this) {
        xmlNodePtr itemNode = NewTextChild(pdNode, "item");
        NewTextChild(itemNode, "first", EncodePDType(i.first));
        i.second.toXML(itemNode);
    }
}

void TPD::parse(const char *val)
{
    vector<string> items;
    split(items, val, '.');
    if(items.size() < 3) throw ETlgError("wrong item count within PD %s", val);
    TPDType type = DecodePDType(items[1]);
    if(type == pdtUnknown)
        throw ETlgError("PD wrong type %s", items[1].c_str());
    (*this)[type].parse(type, items);
}

void TSPItem::toXML(xmlNodePtr node) const
{
    if(empty()) return;
    xmlNodePtr spItemNode = NewTextChild(node, "SPItem");
    NewTextChild(spItemNode, "actual", actual, NoExists);
    NewTextChild(spItemNode, "gender", EncodeGender(gender), "");
}

bool TSPItem::empty() const
{
    return
        actual == NoExists and
        gender == gUnknown;
}

void TSPItem::clear()
{
    actual = NoExists;
    gender = gUnknown;
}

void TSPVector::fromXML(xmlNodePtr node)
{
    clear();
    xmlNodePtr curNode = node->children;
    curNode = GetNodeFast("spVector", curNode);
    if(not curNode) return;
    curNode = curNode->children;
    for(; curNode; curNode = curNode->next) {
        xmlNodePtr curNode2 = curNode->children;
        TSPItem item;
        item.actual = NodeAsIntegerFast("actual", curNode2, NoExists);
        item.gender = DecodeGender(NodeAsStringFast("gender", curNode2, ""));
        push_back(item);
    }
}

void TSPVector::toXML(xmlNodePtr node) const
{
    if(empty()) return;
    xmlNodePtr spVectorNode = NewTextChild(node, "spVector");
    for(const auto &i: *this) {
        if(i.empty()) continue;
        xmlNodePtr itemNode = NewTextChild(spVectorNode, "item");
        NewTextChild(itemNode, "actual", i.actual, NoExists);
        NewTextChild(itemNode, "gender", EncodeGender(i.gender), "");
    }
}

void TSP::fromXML(xmlNodePtr node)
{
    clear();
    xmlNodePtr curNode = node->children;
    curNode = GetNodeFast("SP", curNode);
    if(not curNode) return;
    curNode = curNode->children;
    pr_weight = NodeAsIntegerFast("pr_weight", curNode, NoExists);
    xmlNodePtr spMapNode = NodeAsNodeFast("SPMap", curNode);
    xmlNodePtr spMapItemNode = spMapNode->children;
    for(; spMapItemNode; spMapItemNode = spMapItemNode->next) {
        xmlNodePtr spMapItemDataNode = spMapItemNode->children;
        TSPVector spVector;
        spVector.fromXML(spMapItemNode);
        insert(make_pair(
                    NodeAsStringFast("first", spMapItemDataNode),
                    spVector));
    }
}

void TSP::toXML(xmlNodePtr node) const
{
    if(empty()) return;
    xmlNodePtr spNode = NewTextChild(node, "SP");
    NewTextChild(spNode, "pr_weight", pr_weight, NoExists);
    xmlNodePtr spMapNode = NULL;
    for(const auto &i: *this) {
        if(not spMapNode)
            spMapNode = NewTextChild(spNode, "SPMap");
        xmlNodePtr itemNode = NewTextChild(spMapNode, "item");
        NewTextChild(itemNode, "first", i.first);
        i.second.toXML(itemNode);
    }
}

void TSP::dump()
{
    ProgTrace(TRACE5, "---TSP::dump---");
    for(const auto &im: *this) {
        ostringstream buf;
        buf << "SP[" << im.first << "] = ";
        for(const auto &iv: im.second) {
            if(iv.actual != NoExists)
                buf << iv.actual;
            else
                buf << EncodeGender(iv.gender);
            ProgTrace(TRACE5, "%s", buf.str().c_str());
        }
    }
    ProgTrace(TRACE5, "---------------");
}

bool adult(TGender g)
{
    return g == gM or g == gF;
}

void TSP::parse(const char *val)
{
    /*!!!
    if(not empty())
        throw ETlgError("duplicate SP found");
        */
    vector<string> items;
    split(items, val, '.');
    for(vector<string>::iterator iv = items.begin() + 1; iv != items.end(); iv++) {
        vector<string> sp_item;
        split(sp_item, *iv, '/');
        if(sp_item.size() != 2)
            throw ETlgError("SP wrong seat format %s", iv->c_str());
        TSPItem sp_i;
        string seat = sp_item[0];
        sp_i.gender = DecodeGender(sp_item[1]);
        // Weight mode (actual weight or gender) определяется по формату первого места.
        if(pr_weight == NoExists)
            pr_weight = sp_i.gender == gUnknown;

        if(pr_weight)
            try {
                sp_i.actual = ToInt(sp_item[1]);
            } catch(EConvertError &E) {
                throw ETlgError("SP: wrong format of actual weight: %s", iv->c_str());
            }
        else if(sp_i.gender == gUnknown)
            throw ETlgError("SP: unknown gender in gender mode: %s", iv->c_str());

        vector<TSPItem> &seat_elem = (*this)[seat];
        if(seat_elem.size() > 1)
            throw ETlgError("SP: cannot add more than 2 persons on one seat");

        // Если в тлг использутеся пол пассажира и для данного места уже добавлен 1.
        if(not pr_weight and seat_elem.size() == 1) {
            if(
                    not (
                        (sp_i.gender == gI and adult(seat_elem[0].gender)) or
                        (adult(sp_i.gender) and seat_elem[0].gender == gI)
                        )
              ) {
                throw ETlgError("SP: Infant must be accompanied with adult. seat: %s, pax1: %s, pax2: %s",
                        seat.c_str(),
                        EncodeGender(sp_i.gender),
                        EncodeGender(seat_elem[0].gender)
                        );
            }
        }

        seat_elem.push_back(sp_i);
    }
}

bool TDest::find_item(const string &airp, TDestInfoKey key)
{
    bool result = false;
    map<std::string, TDestInfoMap>::iterator i_type_map = find(airp);
    if(i_type_map != end()) {
        TDestInfoMap::iterator i_sub_type_map = i_type_map->second.find(key);
        if(i_sub_type_map != i_type_map->second.end())
            result = true;
    }
    return result;
}

bool TClsTotal::empty() const
{
    return
        f == NoExists and
        c == NoExists and
        y == NoExists;
}

void TClsTotal::clear()
{
    f = NoExists;
    c = NoExists;
    y = NoExists;
}

void TClsTotal::fromXML(xmlNodePtr node, const std::string &tag)
{
    clear();
    xmlNodePtr curNode = node->children;
    curNode = GetNodeFast(tag.c_str(), curNode);
    if(not curNode) return;
    curNode = curNode->children;
    f = NodeAsIntegerFast("f", curNode, NoExists);
    c = NodeAsIntegerFast("c", curNode, NoExists);
    y = NodeAsIntegerFast("y", curNode, NoExists);
}

void TClsTotal::toXML(xmlNodePtr node, const std::string &tag) const
{
    if(empty()) return;
    xmlNodePtr tagNode = NewTextChild(node, tag.c_str());
    NewTextChild(tagNode, "f", f, NoExists);
    NewTextChild(tagNode, "c", c, NoExists);
    NewTextChild(tagNode, "y", y, NoExists);
}

void TClsTotal::dump(const string &caption)
{
    ProgTrace(TRACE5, "---TClsTotal::dump %s ---", caption.c_str());
    ProgTrace(TRACE5, "f: %d, c: %d, y: %d", f, c, y);
    ProgTrace(TRACE5, "---------------------");
}

void TClsTotal::parse(const string &val)
{
    try {
    vector<string> items;
    split(items, val, '/');
    if(items.size() == 1) // пришел извращенный WBW формат -VKO.PT.4.C.4
        ToInt(items[0]); // просто проверяем, что это число
    else if(items.size() == 3) { // пришел стандартный формат -SIN.PT.50.C.5/10/35
        f = ToInt(items[0]);
        c = ToInt(items[1]);
        y = ToInt(items[2]);
    } else
        throw ETlgError("wrong class count %zu", items.size());
    } catch(EConvertError &E) {
        throw ETlgError(E.what());
    }
}

bool TGenderTotal::empty() const
{
    return
        m == NoExists and
        f == NoExists and
        c == NoExists and
        i == NoExists;
}

void TGenderTotal::clear()
{
    m = NoExists;
    f = NoExists;
    c = NoExists;
    i = NoExists;
}

void TGenderTotal::fromXML(xmlNodePtr node)
{
    clear();
    xmlNodePtr curNode = node->children;
    curNode = GetNodeFast("GenderTotal", curNode);
    if(not curNode) return;
    curNode = curNode->children;
    m = NodeAsIntegerFast("m", curNode, NoExists);
    f = NodeAsIntegerFast("f", curNode, NoExists);
    c = NodeAsIntegerFast("c", curNode, NoExists);
    i = NodeAsIntegerFast("i", curNode, NoExists);
}

void TGenderTotal::toXML(xmlNodePtr node) const
{
    if(empty()) return;
    xmlNodePtr GenderTotalNode = NewTextChild(node, "GenderTotal");
    NewTextChild(GenderTotalNode, "m", m, NoExists);
    NewTextChild(GenderTotalNode, "f", f, NoExists);
    NewTextChild(GenderTotalNode, "c", c, NoExists);
    NewTextChild(GenderTotalNode, "i", i, NoExists);
}

void TGenderTotal::dump()
{
    ProgTrace(TRACE5, "---TGenderTotal::dump---");
    ProgTrace(TRACE5, "m: %d, f: %d, c: %d, i: %d", m, f, c, i);
    ProgTrace(TRACE5, "---------------------");
}

void TGenderTotal::parse(const string &val)
{
    vector<string> items;
    split(items, val, '/');
    if(items.size() != 4)
        throw ETlgError("wrong gender count %uz", items.size());
    m = ToInt(items[0]);
    f = ToInt(items[1]);
    c = ToInt(items[2]);
    i = ToInt(items[3]);
}

bool TDestInfo::empty() const
{
    return
        total == NoExists and
        weight == NoExists and
        j == NoExists and
        measur == mUnknown and
        cls_total.empty() and
        actual_total.empty() and
        gender_total.empty();
}

void TDestInfo::clear()
{
    total = NoExists;
    weight = NoExists;
    j = NoExists;
    measur = mUnknown;
    cls_total.clear();
    actual_total.clear();
    gender_total.clear();
}

void TDestInfo::fromXML(xmlNodePtr node)
{
    clear();
    xmlNodePtr curNode = node->children;
    curNode = GetNodeFast("DestInfo", curNode);
    if(not curNode) return;
    xmlNodePtr DestInfoNode = curNode;
    curNode = curNode->children;
    total = NodeAsIntegerFast("total", curNode, NoExists);
    weight = NodeAsIntegerFast("weight", curNode, NoExists);
    j = NodeAsIntegerFast("j", curNode, NoExists);
    measur = DecodeMeasur(NodeAsStringFast("measur", curNode));
    cls_total.fromXML(DestInfoNode, "cls_total");
    actual_total.fromXML(DestInfoNode, "actual_total");
    gender_total.fromXML(DestInfoNode);
}

void TDestInfo::toXML(xmlNodePtr node) const
{
    if(empty()) return;
    xmlNodePtr destInfoNode = NewTextChild(node, "DestInfo");
    NewTextChild(destInfoNode, "total", total, NoExists);
    NewTextChild(destInfoNode, "weight", weight, NoExists);
    NewTextChild(destInfoNode, "j", j, NoExists);
    NewTextChild(destInfoNode, "measur", EncodeMeasur(measur));
    cls_total.toXML(destInfoNode, "cls_total");
    actual_total.toXML(destInfoNode, "actual_total");
    gender_total.toXML(destInfoNode);
}

void TDestInfo::dump()
{
    ProgTrace(TRACE5, "---TDestInfo::dump---");
    ProgTrace(TRACE5, "total: %d", total);
    ProgTrace(TRACE5, "weight: %d", weight);
    ProgTrace(TRACE5, "j: %d", j);
    ProgTrace(TRACE5, "measur: %s", EncodeMeasur(measur));
    cls_total.dump("cls");
    actual_total.dump("actual");
    gender_total.dump();
    ProgTrace(TRACE5, "---------------------");
}

void TDest::fromXML(xmlNodePtr node)
{
    clear();
    xmlNodePtr curNode = node->children;
    curNode = GetNodeFast("Dest", curNode);
    if(not curNode) return;
    curNode = curNode->children;
    for(; curNode; curNode = curNode->next) {
        xmlNodePtr curNode2 = curNode->children;
        string first = NodeAsStringFast("first", curNode2);
        xmlNodePtr DestInfoMapNode = NodeAsNodeFast("DestInfoMap", curNode2);
        TDestInfoMap destInfoMap;
        if(DestInfoMapNode) {
            xmlNodePtr DestInfoMapItemNode = DestInfoMapNode->children;
            for(; DestInfoMapItemNode; DestInfoMapItemNode = DestInfoMapItemNode->next) {
                xmlNodePtr DestInfoMapItemDataNode = DestInfoMapItemNode->children;
                TDestInfoKey DIKey = DecodeDestInfoKey(NodeAsStringFast("first", DestInfoMapItemDataNode));
                TDestInfo dest_info;
                dest_info.fromXML(DestInfoMapItemNode);
                destInfoMap.insert(make_pair(DIKey, dest_info));
            }
        }
        insert(make_pair(first, destInfoMap));
    }
}

void TDest::toXML(xmlNodePtr node) const
{
    if(empty()) return;
    xmlNodePtr destNode = NewTextChild(node, "Dest");
    for(const auto &i: *this) {
        xmlNodePtr itemNode = NewTextChild(destNode, "item");
        NewTextChild(itemNode, "first", i.first);
        if(not i.second.empty()) {
            xmlNodePtr destInfoMapNode = NewTextChild(itemNode, "DestInfoMap");
            for(const auto &dest_info: i.second) {
                xmlNodePtr DestInfoMapItemNode = NewTextChild(destInfoMapNode, "item");
                NewTextChild(DestInfoMapItemNode, "first", EncodeDestInfoKey(dest_info.first));
                dest_info.second.toXML(DestInfoMapItemNode);
            }
        }
    }
}

void TDest::dump()
{
    ProgTrace(TRACE5, "---TDest::dump---");
    for(map<string, TDestInfoMap>::iterator i_dest = begin(); i_dest != end(); i_dest++) {
        for(TDestInfoMap::iterator i_key = i_dest->second.begin(); i_key != i_dest->second.end(); i_key++) {
            ProgTrace(TRACE5, "TDest[%s][%s]", i_dest->first.c_str(), EncodeDestInfoKey(i_key->first));
            i_key->second.dump();
        }
    }
    ProgTrace(TRACE5, "-----------------");
}

void TDest::parse(const char *val)
{
    vector<string> items;
    split(items, val, '.');
    if(items.size() < 3)
        throw ETlgError("Wrong TDest format");
    string airp = getElemId(items[0].substr(1), etAirp);
    if(airp.empty())
        throw ETlgError("TDest: airp not found %s", items[0].substr(1).c_str());
    TDestInfoKey key = DecodeDestInfoKey(items[1]);
    if(key == dkUnknown)
        throw ETlgError("Unknown TDest key %s", items[1].c_str());
    string id = "Dest[" + airp + "][" + EncodeDestInfoKey(key) + "]";
    TDestInfoType dst_type = DecodeDestInfoType(items[2]);
    if(dst_type != dtWB) {
        if(find_item(airp, key))
            throw ETlgError("%s already exists", id.c_str());
    }

    enum TState {
        sStart,
        sType,
        sWBSeat,
        sParseType,
        sParseJ,
        sEnd
    };

    TDestInfo dest_info;
    TState s = sStart;
    TDestInfoType type = dtUnknown;
    for(vector<string>::iterator iv = items.begin() + 2; iv != items.end(); iv++) {
        switch(s) {
            case sWBSeat:
                // nothing to do for now
                break;
            case sStart:
                {
                    type = DecodeDestInfoType(*iv);
                    if(type == dtUnknown) {
                        LogTrace(TRACE5) << "within dtUnknown: '" << *iv << "'";
                        vector<string> parts;
                        split(parts, *iv, '/');
                        if(parts.size() == 1) {
                            // указан просто тотал
                            dest_info.total = ToInt(parts[0]);
                        } else if(parts.size() == 2) {
                            // указан тотал/вес
                            dest_info.total = ToInt(parts[0]);
                            dest_info.weight = ToInt(parts[1]);
                        } else
                            throw ETlgError("%s: wrong format %s", id.c_str(), iv->c_str());
                        s = sType;
                    } else
                        s = sParseType;
                    break;
                }
            case sType:
                {
                    dest_info.measur = DecodeMeasur(iv->c_str());
                    if(dest_info.measur == mUnknown) {
                        type = DecodeDestInfoType(*iv);
                        s = sParseType;
                    } else
                        s = sEnd;
                    break;
                }
                break;
            case sParseType:
                switch(type) {
                    case dtA:
                        dest_info.actual_total.parse(*iv);
                        s = sType;
                    case dtC:
                        dest_info.cls_total.parse(*iv);
                        s = sType;
                        break;
                    case dtG:
                        dest_info.gender_total.parse(*iv);
                        s = sType;
                        break;
                    case dtJ:
                        s = sParseJ;
                        break;
                    case dtWB:
                        s = sWBSeat;
                        break;
                    case dtUnknown:
                        throw ETlgError("unknown type");
                }
                break;
            case sParseJ:
                dest_info.j = ToInt(*iv);
                s = sType;
                break;
            case sEnd:
                throw ETlgError("Wrong format");
        }
    }
    (*this)[airp][key] = dest_info;
};

bool TSR::empty() const
{
    return
        type == srUnknown and
        format == 0 and
        c.empty() and
        z.empty() and
        r.empty() and
        s.empty() and
        j.empty();
}

void TSR::fromXML(xmlNodePtr node)
{
    clear();
    xmlNodePtr curNode = node->children;
    curNode = GetNodeFast("SR", curNode);
    if(not curNode) return;
    xmlNodePtr srNode = curNode;
    curNode = curNode->children;
    type = (Type)NodeAsIntegerFast("type", curNode, srUnknown);
    string str_format = NodeAsStringFast("format", curNode, "");
    if(str_format.empty())
        format = 0;
    else
        format = str_format[0];
    c.fromXML(srNode);
    z.fromXML(srNode);
    r.fromXML(srNode, "r");
    s.fromXML(srNode, "s");
    j.fromXML(srNode);
}

void TSR::toXML(xmlNodePtr node) const
{
    if(empty()) return;
    xmlNodePtr srNode = NewTextChild(node, "SR");
    NewTextChild(srNode, "type", type, srUnknown);
    NewTextChild(srNode, "format", (format == 0 ? "" : string(1, format)), "");
    c.toXML(srNode);
    z.toXML(srNode);
    r.toXML(srNode, "r");
    s.toXML(srNode, "s");
    j.toXML(srNode);
}

bool TLCIReqInfo::empty() const
{
    return
        req_type == rtUnknown and
        lang.empty() and
        max_commerce == NoExists and
        sr.empty() and
        wm_type == wmtUnknown and
        sp_type == spUnknown;
}

void TLCIReqInfo::clear()
{
    req_type = rtUnknown;
    lang.clear();
    max_commerce = NoExists;
    sr.clear();
    wm_type = wmtUnknown;
    sp_type = spUnknown;
}

void TLCIReqInfo::fromXML(xmlNodePtr node)
{
    clear();
    xmlNodePtr curNode = node->children;
    curNode = GetNodeFast("LCIReqInfo", curNode);
    if(not curNode) return;
    xmlNodePtr reqInfoNode = curNode;
    curNode = curNode->children;
    req_type = DecodeReqType(NodeAsStringFast("req_type", curNode, ""));
    lang = NodeAsStringFast("lang", curNode, "");
    max_commerce = NodeAsIntegerFast("max_commerce", curNode, NoExists);
    sr.fromXML(reqInfoNode);
    wm_type = DecodeWMType(NodeAsStringFast("WMType", curNode, ""));
    sp_type = (TSPType)NodeAsIntegerFast("SPType", curNode, spUnknown);
}

void TLCIReqInfo::toXML(xmlNodePtr node) const
{
    if(empty()) return;
    xmlNodePtr LCIReqInfoNode = NewTextChild(node, "LCIReqInfo");
    NewTextChild(LCIReqInfoNode, "req_type", EncodeReqType(req_type), "");
    NewTextChild(LCIReqInfoNode, "lang", lang, "");
    NewTextChild(LCIReqInfoNode, "max_commerce", max_commerce, NoExists);
    sr.toXML(LCIReqInfoNode);
    NewTextChild(LCIReqInfoNode, "WMType", EncodeWMType(wm_type), "");
    NewTextChild(LCIReqInfoNode, "SPType", sp_type, spUnknown);
}

void TRequest::fromXML(xmlNodePtr node)
{
    clear();
    xmlNodePtr curNode = node->children;
    curNode = GetNodeFast("Request", curNode);
    if(not curNode) return;
    curNode = curNode->children;
    for(; curNode; curNode = curNode->next) {
        xmlNodePtr curNode2 = curNode->children;
        TLCIReqInfo req_info;
        req_info.fromXML(curNode);
        insert(make_pair(
                    DecodeReqType(NodeAsStringFast("req_type", curNode2)),
                    req_info));
    }
}

void TRequest::toXML(xmlNodePtr node)
{
    if(empty()) return;
    xmlNodePtr requestNode = NewTextChild(node, "Request");
    for(const auto &i: *this) {
        xmlNodePtr itemNode = NewTextChild(requestNode, "item");
        NewTextChild(itemNode, "req_type", EncodeReqType(i.first));
        i.second.toXML(itemNode);
    }
}

void TRequest::parse(const char *val)
{
    if(strlen(val) < strlen("LR XX"))
        throw ETlgError("Wrong request code");

    string buf = string(val).substr(3, 2);
    TLCIReqInfo req_info;
    req_info.req_type = DecodeReqType(buf);
    if(req_info.req_type == rtUnknown)
        throw ETlgError("Unknown request type '%s'", buf.c_str());
    switch(req_info.req_type) {
        case rtWB:
            {
                vector<string> items;
                split(items, val, '.');
                if(items[1] == "MAX_COMMERCE") {
                    req_info.max_commerce = ToInt(items[2]);
                    if(req_info.max_commerce >= 1000000)
                        throw ETlgError("max_commerce too big: %d", req_info.max_commerce);
                } else if(items[1] == "LANG") {
                    if(
                            items[2] != AstraLocale::LANG_RU and
                            items[2] != AstraLocale::LANG_EN
                      )
                        throw ETlgError("Unknown LANG '%s'", items[2].c_str());
                    req_info.lang = items[2];
                } else
                    throw ETlgError("Unknown lexeme: '%s'", items[1].c_str());
                break;
            }
        case rtSR:
            buf = string(val).substr(3);
            req_info.sr.parse(buf.c_str());
            break;
        case rtWM:
            req_info.wm_type = DecodeWMType(string(val).substr(6));
            break;
        case rtSP:
            {
                buf = string(val).substr(5); // get ".WB"
                if(buf.empty())
                    req_info.sp_type = spStd;
                else if(buf == ".WB")
                    req_info.sp_type = spWB;
                else
                    throw ETlgError("Unknown Seat Plan type '%s'", buf.c_str());
                break;
            }
        default:
            break;
    }
    TRequest::iterator i = find(req_info.req_type);
    if(i != end()) {
        if(req_info.req_type == rtSR) { // SR.S может встречаться несколько раз, тогда объединяем список мест
            if(req_info.sr.s.empty())
                throw ETlgError("Wrong appending SR.S format");
            if(i->second.sr.s.empty())
                throw ETlgError("Wrong existing SR.S format");
            i->second.sr.s.insert(
                    i->second.sr.s.end(),
                    req_info.sr.s.begin(),
                    req_info.sr.s.end());
        } else
            throw ETlgError("Duplicate request type '%s'", buf.c_str());
    } else
        insert(pair<TReqType, TLCIReqInfo>(req_info.req_type, req_info));
}

void TRequest::dump()
{
    ProgTrace(TRACE5, "---TRequest::dump---");
    for(TRequest::iterator i = begin(); i != end(); i++) {
        ProgTrace(TRACE5, "req_type: %s", EncodeReqType(i->first));
        switch(i->first) {
            case rtSR:
                i->second.sr.dump();
                break;
            case rtWM:
                ProgTrace(TRACE5, "WM type '%s'", EncodeWMType(i->second.wm_type));
                break;
            default:
                break;
        }
    }
    ProgTrace(TRACE5, "--------------------");
}

void TSI::fromXML(xmlNodePtr node)
{
    items.clear();
    xmlNodePtr curNode = node->children;
    curNode = GetNodeFast("SI", curNode);
    if(not curNode) return;
    curNode = curNode->children;
    for(; curNode; curNode = curNode->next)
        items.push_back(NodeAsString(curNode));
}

void TSI::toXML(xmlNodePtr node) const
{
    if(items.empty()) return;
    xmlNodePtr siNode = NewTextChild(node, "SI");
    for(const auto &i: items)
        NewTextChild(siNode, "item", i);
}

void TSI::parse(TTlgPartInfo &body, TTlgParser &tlg, TLinePtr &line_p)
{
    items.clear();
    bool start = true;
    try
    {
        do {
            tlg.GetToEOLLexeme(line_p);
            if(not *tlg.lex)
                throw ETlgError("TSI: blank line not allowed");
            if(start) {
                if(tlg.lex != (string)"SI")
                    throw ETlgError("SI not found where expected");
                start = false;
            } else {
                items.push_back(tlg.lex);
            }
            line_p=tlg.NextLine(line_p);
        } while (line_p and *line_p != 0);
    }
    catch (ETlgError E)
    {
        throwTlgError(E.what(), body, line_p);
    };
}

void TSI::dump()
{
    LogTrace(TRACE5) << "---TSI::dump---";
    for(vector<string>::iterator i = items.begin(); i != items.end(); i++)
        LogTrace(TRACE5) << *i;
    LogTrace(TRACE5) << "---------------";
}

void ParseLCIContent(TTlgPartInfo body, TLCIHeadingInfo& info, TLCIContent& con, TMemoryManager &mem)
{
    con.clear();
    TTlgParser tlg;
    TLinePtr line_p=body.p;
    TTlgElement e = ActionCode;
    try
    {
        do {
            tlg.GetToEOLLexeme(line_p);
            if(not *tlg.lex)
                throw ETlgError("blank line not allowed");
            switch(e) {
                case ActionCode:
                    con.action_code.parse(tlg.lex);
                    if(con.action_code.action == aRequest)
                        con.req.parse(tlg.lex);
                    else
                        e = LCIData;
                    break;
                case LCIData:
                    if(con.action_code.action == aOpen) // Весь контент игнорируем
                        break;
                    if(tlg.lex[0] == '-')
                        con.dst.parse(tlg.lex);
                    else if(strncmp(tlg.lex, "EQT", 3) == 0)
                        con.eqt.parse(tlg.lex);
                    else if(strncmp(tlg.lex, "WA", 2) == 0)
                        con.wa.parse(tlg.lex);
                    else if(strncmp(tlg.lex, "SM", 2) == 0)
                        con.sm.parse(tlg.lex);
                    else if(strncmp(tlg.lex, "SR", 2) == 0)
                        con.sr.parse(tlg.lex);
                    else if(strncmp(tlg.lex, "WM", 2) == 0)
                        con.wm.parse(tlg.lex);
                    else if(strncmp(tlg.lex, "PD", 2) == 0)
                        con.pd.parse(tlg.lex);
                    else if(strncmp(tlg.lex, "SP", 2) == 0)
                        con.sp.parse(tlg.lex);
                    else if(strncmp(tlg.lex, "SI", 2) == 0)
                        con.si.parse(body, tlg, line_p);
                    else
                        throw ETlgError("unknown lexeme %s", tlg.lex);
                    break;
                default:;
            }
            line_p=tlg.NextLine(line_p);
        } while (line_p and *line_p != 0);
    }
    catch (ETlgError E)
    {
        throwTlgError(E.what(), body, line_p);
    };
}

void set_seats_option(TPassSeats &seats, const TSeatRanges &seatRanges, int point_id_spp)
{
    SALONS2::TSalonList salonList;
    salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id_spp ), "", NoExists );
    for ( std::vector<SALONS2::TPlaceList*>::iterator isalonList=salonList._seats.begin();
            isalonList!=salonList._seats.end(); isalonList++ ) {
        for ( SALONS2::TPlaces::iterator iseat=(*isalonList)->places.begin(); iseat!=(*isalonList)->places.end(); iseat++ ) {
            if(not iseat->isplace) continue;
            seats.insert(TSeat(iseat->yname, iseat->xname));
        }
    }

    for(TSeatRanges::const_iterator
            seat_range = seatRanges.begin();
            seat_range != seatRanges.end();
            seat_range++) {
        TPassSeats::iterator found_seat = seats.find(seat_range->first);
        if(found_seat != seats.end())
            seats.erase(found_seat);
        else
            seats.insert(seat_range->first);
    }
    set_alarm(point_id_spp, Alarm::WBDifferSeats, not seats.empty());
    if(seats.empty()) seats.insert(TSeat());
}

int LCI_ACT_TIMEOUT()
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("LCI_ACT_TIMEOUT",NoExists,NoExists,2);
  return VAR;
};

int LCI_EST_TIMEOUT()
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("LCI_EST_TIMEOUT",NoExists,NoExists,48);
  return VAR;
};

string TLCIContent::answer()
{
    string result;
    try {
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "SELECT point_id_spp, nvl(points.est_out, points.scd_out) scd_out FROM tlg_binding, points WHERE point_id_tlg=:point_id and point_id_spp = point_id";
        Qry.CreateVariable("point_id", otInteger, point_id_tlg);
        Qry.Execute();
        if ( Qry.Eof )
            throw ETlgError(tlgeNotMonitorYesAlarm, "Flight not found");

        TNearestDate nd(time_receive);
        for(; not Qry.Eof; Qry.Next()) {
            nd.sorted_points[Qry.FieldAsDateTime( "scd_out" )] =
                Qry.FieldAsInteger( "point_id_spp" );
        }
        int point_id_spp = nd.get();

        TTripInfo flt;
        flt.getByPointId(point_id_spp);
        TDateTime dep_time;
        int timeout;
        if(flt.act_out_exists()) {
            dep_time = flt.act_est_scd_out();
            timeout = LCI_ACT_TIMEOUT();
        } else {
            dep_time = flt.est_scd_out();
            timeout = LCI_EST_TIMEOUT();
        }
        if((time_receive - dep_time) > timeout / 24.)
            throw ETlgError(tlgeNotMonitorYesAlarm, "Flight has departed");

        TSeatRanges ranges_tmp, seatRanges;

        TCreateInfo createInfo("LCI", TCreatePoint());
        // !!! приведение константной ссылки к неконстантной. Не хорошо.
        TypeB::TLCIOptions &options = (TypeB::TLCIOptions&)(*createInfo.optionsAs<TypeB::TLCIOptions>());

        options.is_lat = false;
        options.equipment=false;
        options.weight_avail="N";
        options.seating=false;
        options.weight_mode=false;
        options.seat_restrict="S";
        options.pas_totals = false;
        options.bag_totals = false;
        options.pas_distrib = false;
        options.seat_plan = true;
        options.version = "WB";
        options.franchise_info = franchise_flt;

        if(action_code.action == aRequest) {

            for(TRequest::iterator i = req.begin(); i != req.end(); i++) {
                switch(i->first) {
                    case rtSR:
                        if ( !seatRanges.empty() ) {
                            throw Exception( "SaveLCIContent second rtSR" );
                        }
                        for(TSRItems::iterator sr_i = i->second.sr.s.begin(); sr_i != i->second.sr.s.end(); sr_i++) {
                            ParseSeatRange(*sr_i, ranges_tmp, false);
                            seatRanges.insert( seatRanges.end(), ranges_tmp.begin(), ranges_tmp.end() );
                        }
                        switch(i->second.sr.type) {
                            case TSR::srWB:
                                {
                                    if(not seatRanges.empty()) { // Пришел запрос вида LR SR.WB.S.1A/1B....
                                        set_seats_option(options.seats, seatRanges, point_id_spp);
                                        options.seat_plan = false;
                                    }
                                    if(not i->second.sr.c.empty()) {  // Пришел запрос вида LR SR.WB.C.C12Y116
                                        for(TCFG::iterator
                                                cfg_item = i->second.sr.c.begin();
                                                cfg_item != i->second.sr.c.end();
                                                ++cfg_item) {
                                            options.cfg.push_back(TCFGItem());
                                            // options.cfg.back().cls = cfg_item->first; // игнорим классы
                                            options.cfg.back().cfg = cfg_item->second;
                                        }
                                        options.seat_restrict.clear();
                                        options.seat_plan = false;
                                    }
                                }
                                break;
                            case TSR::srStd:
                                SALONS2::resetLayers( point_id_spp, cltProtect, seatRanges, "EVT.LAYOUT_MODIFIED_LCI.SEAT_PLAN" );
                                break;
                            default:
                                break;
                        }
                        break;
                    case rtWB:
                        options.is_lat = i->second.lang == AstraLocale::LANG_EN;
                        if(i->second.max_commerce != NoExists)
                            LogTrace(TRACE5) << "max_commerce: " << i->second.max_commerce;
                        break;
                    case rtBT:
                        options.bag_totals = true;
                        break;
                    case rtWM:
                        options.weight_mode = true;
                        break;
                    default:
                        break;
                }
            }
            createInfo.point_id = point_id_spp;
            createInfo.set_addrs(sender);
            result = TelegramInterface::SendTlg(vector<TCreateInfo>(1, createInfo), typeb_in_id);
        } else if(action_code.action == aOpen) {
            options.seat_plan = true;
            options.version = "WB";
            createInfo.point_id = point_id_spp;
            createInfo.set_addrs(sender);
            result = TelegramInterface::SendTlg(vector<TCreateInfo>(1, createInfo));
        } else if(action_code.action == aUpdate) {
            std::tr1::shared_ptr<TSubTypeHolder> sth = wm[wmdWB][wmtWBTotalWeight];
            if(sth) {
                const TSimpleWeight &mc = *dynamic_cast<TSimpleWeight *>(sth.get());
                if(mc.measur != mN) {
                    TFlightMaxCommerce maxCommerce(true);
                    if ( mc.weight == 0 )
                        maxCommerce.SetValue( ASTRA::NoExists );
                    else
                        maxCommerce.SetValue( mc.weight );
                    maxCommerce.Save( point_id_spp );
                }
            }
            sth = wm[wmdStandard][wmtPax];
            if(sth and sth->sub_type == wmsGender) {
                const TGenderCount &gc = *dynamic_cast<TGenderCount *>(sth.get());
                ClassesPersWeight cpw;
                cpw.male = gc.m;
                cpw.female = gc.f;
                cpw.child = gc.c;
                cpw.infant = gc.i;
                PersWeightRules lci_pwr(true);
                lci_pwr.Add(cpw);

                PersWeightRules db_pwr;
                db_pwr.read(point_id_spp);
                if(not lci_pwr.equal(&db_pwr))
                    lci_pwr.write(point_id_spp);
            }
        }
    } catch(Exception &E) {
        result = E.what();
    }
    return result;
}

void SaveLCIContent(int tlg_id, TDateTime time_receive, TLCIHeadingInfo& info, TLCIContent& con)
{
    int point_id_tlg=SaveFlt(tlg_id,info.flt_info.toFltInfo(),btFirstSeg,TSearchFltInfoPtr(new TLCISearchParams()));

    con.point_id_tlg = point_id_tlg;
    con.time_receive = time_receive;
    con.sender = info.sender;
    con.typeb_in_id = tlg_id;
    con.franchise_flt = info.flt_info.franchise_flt;

    TypeBHelpMng::notify_ok(tlg_id, AstraContext::SetContext("LCI", con.toXML())); // Отвешиваем процесс, если есть.
}

int lci(int argc, char **argv)
{
    TMemoryManager mem(STDLOG);

    std::ifstream ifs ("lci", std::ifstream::binary);

    /*
    // get pointer to associated buffer object
    std::filebuf* pbuf = ifs.rdbuf();

    // get file size using buffer's members
    std::size_t size = pbuf->pubseekoff (0,ifs.end,ifs.in);
    pbuf->pubseekpos (0,ifs.in);

    // allocate memory to contain file data
    char* buffer=new char[size];

    // get file data
    pbuf->sgetn (buffer,size);

    ifs.close();
    */
    string buffer = string( std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>() );

    LogTrace(TRACE5) << "buffer: '" << buffer << "'";

    /*
       char c;
       streambuf *sb;
       sb = in.rdbuf();
       while(sb->sgetc() != EOF) {
       c = sb->sbumpc();
       buf << c;
       }
       */

    /*
       char *buf = (char *)
       "MOWKB1H\xa"
       ".DMBRUUH 060849\n"
       "LCI\n"
       "ЮТ001/29JUL.ДМД\n"
       "CF\n"
       "EQT.ASDF.ТУ5.Б012Э122\n"
       "WA.P.1485.KG\n"
       "WA.U.0.KG\n"
       "SM.S\n"
       "SR.C.Б11Э115\n"
       "SR.Z.0F22/0Z104\n"
       "WM.S.P.CG.П100/50/25/20.Б80/75/50/25.Э75/70/45/20.KG\n"
       "WM.A.B.KG\n"
       "-СОЧ.PT.0.C.0/0/0\n"
       "-СОЧ.BT.0/0.C.0/0/0.A.0/0/0.KG\n"
       "-СОЧ.H.0.KG\n"
       "-ВНК.PT.2.C.0/0/2\n"
       "-ВНК.BT.1/500.C.0/0/1.A.0/0/500.KG\n"
       "-ВНК.H.0.KG\n"
       "-ПЛК.PT.1.C.0/0/1\n"
       "-ПЛК.BT.1/500.C.0/0/1.A.0/0/500.KG\n"
       "-ПЛК.H.0.KG\n"
       "-ВРН.PT.3.C.0/0/3\n"
       "-ВРН.BT.0/0.C.0/0/0.A.0/0/0.KG\n"
       "-ВРН.H.0.KG\n"
       "-АЯТ.PT.2.C.0/1/1\n"
       "-АЯТ.BT.0/0.C.0/0/0.A.0/0/0.KG\n"
       "-АЯТ.H.45.KG\n"
       "PD.C.П.0/0/0/0.Б.1/0/0/0.Э.4/1/1/1\n"
       "SP.5А/M.5Б/F.5В/M.5Г/M.5Д/I.5Е/C.1А/M.6А/M\n";
       */

    THeadingInfo *HeadingInfo=NULL;
    TLCIContent con;

    TFlightsForBind bind_flts;
    TTlgPartsText parts;
    try {
        GetParts(buffer.c_str(),parts,HeadingInfo,bind_flts,mem);

        TTlgPartInfo part;
        part.p=parts.heading.c_str();
        part.EOL_count=CalcEOLCount(parts.addr.c_str());
        part.offset=parts.addr.size();
        ParseHeading(part,HeadingInfo,bind_flts,mem);  //может вернуть NULL
        if(HeadingInfo == NULL)
            throw Exception("HeadingInfo is null");

        TLCIHeadingInfo &info = *(dynamic_cast<TLCIHeadingInfo*>(HeadingInfo));
        info.flt_info.dump();

        part.p=parts.body.c_str();
        part.EOL_count+=CalcEOLCount(parts.heading.c_str());
        part.offset+=parts.heading.size();
        ParseLCIContent(part, info, con, mem);

        string lci_data = con.toXML();

        LogTrace(TRACE5) << "lci_data: " << lci_data;

        TLCIContent parsed_lci;
        parsed_lci.fromXML(lci_data);

        LogTrace(TRACE5) << "parsed_lci: " << parsed_lci.toXML();

//        con.dump();

        mem.destroy(HeadingInfo, STDLOG);
        if (HeadingInfo!=NULL) delete HeadingInfo;
    }
    catch(...)
    {
        mem.destroy(HeadingInfo, STDLOG);
        if (HeadingInfo!=NULL) delete HeadingInfo;
        throw;
    };

//    delete[] buffer;

    return 0;
}

}

