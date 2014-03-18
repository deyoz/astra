#include "lci_parser.h"
#include "misc.h"
#include "salons.h"
#include "telegram.h"
#include <sstream>


#define STDLOG NICKNAME,__FILE__,__LINE__
#define NICKNAME "DEN"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;


namespace TypeB
{

string getElemId(const string &val, TElemType el)
{
    TElemFmt fmt;
    string lang;
    string subcls = ElemToElemId(el, val, fmt, lang);
    if(subcls.empty())
        throw ETlgError("unknown subcls %s", val.c_str());
    return subcls;
}

const char *TReqTypeS[] =
{
    "SP",
    "BT",
    "SR",
    "WM",
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

const char EncodeSeatingMethod(TSeatingMethod p)
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

const char EncodeOriginator(TOriginator p)
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

const char EncodeAction(TAction p)
{
    return TActionS[p];
};

void TLCIContent::dump()
{
    ProgTrace(TRACE5, "---TLCIContent::dump---");
    action_code.dump();
    req.dump();
    eqt.dump();
    wa.dump();
    sm.dump();
    sr.dump();
    wm.dump();
    pd.dump();
    sp.dump();
    dst.dump();
    ProgTrace(TRACE5, "-----------------------");
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
    // привязка к рейсы
    flts.push_back(make_pair(toFltInfo(), btFirstSeg));
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

vector<string> split(string val, char c)
{
    vector<string> result;
    size_t idx = val.find(c);
    while(idx != string::npos) {
        result.push_back(val.substr(0, idx));
        val.erase(0, idx + 1);
        idx = val.find(c);
    }
    result.push_back(val);
    return result;
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
    vector<string> items = split(val, '.');
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

void TWA::parse(const char *val)
{
    vector<string> items = split(val, '.');
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

void TSM::dump()
{
    ProgTrace(TRACE5, "---TSM::dump---");
    ProgTrace(TRACE5, "sm.value %c", EncodeSeatingMethod(value));
    ProgTrace(TRACE5, "---------------");
}

void TSM::parse(const char *val)
{
    if(value != smUnknown) throw ETlgError("multiple SM found");
    vector<string> items = split(val, '.');
    if(items.size() != 2) throw ETlgError("wrong SM %s", val);
    if(items[1].size() != 1) throw ETlgError("wrong SM %s", val);
    value = DecodeSeatingMethod(items[1][0]);
    if(value == smUnknown) throw ETlgError("wrong SM %s", val);
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
    vector<string> items = split(val, '/');
    for(vector<string>::iterator iv = items.begin(); iv != items.end(); iv++) {
        if(iv->size() < 3) throw ETlgError("SR wrong zone format %s", iv->c_str());
        insert(make_pair(iv->substr(0, 2), ToInt(iv->substr(2))));
    }
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
    vector<string> result = split(val, '/');
    for(vector<string>::iterator iv = result.begin(); iv != result.end(); iv++)
        if(not iv->empty())
            push_back(*iv);
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
    vector<string> items = split(val, '/');
    if(items.size() > 2)
        throw ETlgError("SR.J: too manu '/' %s", val);
    vector<string> buf = split(items[0], '.');
    if(buf.size() != 3)
        throw ETlgError("Wrong SR.J %s", val);
    amount = ToInt(buf[2]);
    if(items.size() > 1) {
        buf = split(items[1], '.');
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
    vector<string> items = split(val, '.');
    string data;
    if(items.size() > 3)
        throw ETlgError("wrong item count within SR %s", val);
    if(items.size() == 3)
        data = items[2];
    if(items[1].size() != 1)
        throw ETlgError("SR wrong type %s", items[1].c_str());
    switch(items[1][0]) {
        case 'C':
            c.parse(data, etClass);
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
    vector<string> items = split(val[0], '/');
    if(items.size() != 3)
        throw ETlgError("WM wrong cls weight amount %s", val[0].c_str());
    f = ToInt(items[0]);
    c = ToInt(items[1]);
    y = ToInt(items[2]);

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
    vector<string> items = split(val[0], '/');
    if(items.size() != 4)
        throw ETlgError("WM wrong gender weight items count %s", val[0].c_str());
    m = ToInt(items[0]);
    f = ToInt(items[1]);
    c = ToInt(items[2]);
    i = ToInt(items[3]);
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

void TWM::parse(const char *val)
{
    vector<string> items = split(val, '.');
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
            vector <string> items = split(*iv, '/');
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

void TPD::parse(const char *val)
{
    vector<string> items = split(val, '.');
    if(items.size() < 3) throw ETlgError("wrong item count within PD %s", val);
    TPDType type = DecodePDType(items[1]);
    if(type == pdtUnknown)
        throw ETlgError("PD wrong type %s", items[1].c_str());
    (*this)[type].parse(type, items);
}

void TSP::dump()
{
    ProgTrace(TRACE5, "---TSP::dump---");
    for(map<string, TSPItem>::iterator im = begin(); im != end(); im++) {
        ostringstream buf;
        buf << "SP[" << im->first << "] = ";
        if(im->second.actual != NoExists)
            buf << im->second.actual;
        else
            buf << EncodeGender(im->second.gender);
        ProgTrace(TRACE5, "%s", buf.str().c_str());
    }
    ProgTrace(TRACE5, "---------------");
}

void TSP::parse(const char *val)
{
    /*!!!
    if(not empty())
        throw ETlgError("duplicate SP found");
        */
    vector<string> items = split(val, '.');
    for(vector<string>::iterator iv = items.begin() + 1; iv != items.end(); iv++) {
        vector<string> sp_item = split(*iv, '/');
        if(sp_item.size() != 2)
            throw ETlgError("SP wrong seat format %s", iv->c_str());
        TSPItem sp_i;
        string seat = sp_item[0];
        if(find(seat) != end())
            throw ETlgError("SP seat already exists %s", seat.c_str());
        sp_i.gender = DecodeGender(sp_item[1]);
        if(sp_i.gender == gUnknown)
            sp_i.actual = ToInt(sp_item[1]);
        insert(make_pair(seat, sp_i));
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

void TClsTotal::dump(const string &caption)
{
    ProgTrace(TRACE5, "---TClsTotal::dump %s ---", caption.c_str());
    ProgTrace(TRACE5, "f: %d, c: %d, y: %d", f, c, y);
    ProgTrace(TRACE5, "---------------------");
}

void TClsTotal::parse(const string &val)
{
    vector<string> items = split(val, '/');
    if(items.size() != 3)
        throw ETlgError("wrong class count %uz", items.size());
    f = ToInt(items[0]);
    c = ToInt(items[1]);
    y = ToInt(items[2]);
}

void TGenderTotal::dump()
{
    ProgTrace(TRACE5, "---TGenderTotal::dump---");
    ProgTrace(TRACE5, "m: %d, f: %d, c: %d, i: %d", m, f, c, i);
    ProgTrace(TRACE5, "---------------------");
}

void TGenderTotal::parse(const string &val)
{
    vector<string> items = split(val, '/');
    if(items.size() != 4)
        throw ETlgError("wrong gender count %uz", items.size());
    m = ToInt(items[0]);
    f = ToInt(items[1]);
    c = ToInt(items[2]);
    i = ToInt(items[3]);
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
    vector<string> items = split(val, '.');
    if(items.size() < 3)
        throw ETlgError("Wrong TDest format");
    string airp = getElemId(items[0].substr(1), etAirp);
    if(airp.empty())
        throw ETlgError("TDest: airp not found %s", items[0].substr(1).c_str());
    TDestInfoKey key = DecodeDestInfoKey(items[1]);
    if(key == dkUnknown)
        throw ETlgError("Unknown TDest key %s", items[1].c_str());
    string id = "Dest[" + airp + "][" + EncodeDestInfoKey(key) + "]";
    if(find_item(airp, key))
        throw ETlgError("%s already exists", id.c_str());

    enum TState {
        sStart,
        sType,
        sParseType,
        sParseJ,
        sEnd
    };

    TDestInfo dest_info;
    TState s = sStart;
    TDestInfoType type = dtUnknown;
    for(vector<string>::iterator iv = items.begin() + 2; iv != items.end(); iv++) {
        switch(s) {
            case sStart:
                {
                    type = DecodeDestInfoType(*iv);
                    if(type == dtUnknown) {
                        vector<string> parts = split(*iv, '/');
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
                    case dtC:
                        dest_info.cls_total.parse(*iv);
                        break;
                    case dtG:
                        dest_info.gender_total.parse(*iv);
                        break;
                    case dtJ:
                        s = sParseJ;
                        break;
                    case dtUnknown:
                        throw ETlgError("unknown type");
                }
                s = sType;
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
        case rtSR:
            buf = string(val).substr(3);
            req_info.sr.parse(buf.c_str());
            break;
        case rtWM:
            req_info.wm_type = DecodeWMType(string(val).substr(6));
            break;
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

void ParseLCIContent(TTlgPartInfo body, TLCIHeadingInfo& info, TLCIContent& con, TMemoryManager &mem)
{
    con.Clear();
    TTlgParser tlg;
    const char *line_p=body.p;
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

void SaveLCIContent(int tlg_id, TLCIHeadingInfo& info, TLCIContent& con)
{
    int point_id_tlg=SaveFlt(tlg_id,info.flt_info.toFltInfo(),btFirstSeg);
    TQuery Qry(&OraSession);
    Qry.SQLText =
      "SELECT point_id_spp FROM tlg_binding WHERE point_id_tlg=:point_id";
    Qry.CreateVariable("point_id", otInteger, point_id_tlg);
    Qry.Execute();
    if ( Qry.Eof ) {
      throw Exception( "Flight not found, point_id_tlg=%d", point_id_tlg );
    }
    int point_id_spp = Qry.FieldAsInteger( "point_id_spp" );

    vector<TSeatRange> ranges_tmp, seatRanges;
    if(con.action_code.action == aRequest) {
        TCreateInfo createInfo("LCI", TCreatePoint());
        // !!! приведение константной ссылки к неконстантной. Не хорошо.
        TypeB::TLCIOptions &options = (TypeB::TLCIOptions&)(*createInfo.optionsAs<TypeB::TLCIOptions>());

        options.is_lat = true;
        options.equipment=false;
        options.weight_avail="N";
        options.seating=false;
        options.weight_mode=false;
        options.seat_restrict="S";
        options.pas_totals = false;
        options.bag_totals = false;
        options.pas_distrib = false;
        options.seat_plan = false;

        for(TRequest::iterator i = con.req.begin(); i != con.req.end(); i++) {
            switch(i->first) {
                case rtSR:
                    if ( !seatRanges.empty() ) {
                        throw Exception( "SaveLCIContent second rtSR" );
                    }
                    for(TSRItems::iterator sr_i = i->second.sr.s.begin(); sr_i != i->second.sr.s.end(); sr_i++) {
                        ParseSeatRange(*sr_i, ranges_tmp, false);
                        seatRanges.insert( seatRanges.end(), ranges_tmp.begin(), ranges_tmp.end() );
                    }
                    SALONS2::resetLayers( point_id_spp, cltProtect, seatRanges, string("разметка мест по телеграмме LCI: ") );
                    break;
                case rtSP:
                    options.seat_plan = true;
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
        createInfo.set_addrs(info.sender);
        TelegramInterface::SendTlg(vector<TCreateInfo>(1, createInfo));
    } else {
        //
    }
}

int lci(int argc, char **argv)
{
    TMemoryManager mem(STDLOG);

    std::ifstream ifs ("lci", std::ifstream::binary);

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
        GetParts(buffer,parts,HeadingInfo,bind_flts,mem);

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

    delete[] buffer;

    return 0;
}

}

