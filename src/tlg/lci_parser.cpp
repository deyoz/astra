#include "lci_parser.h"
#include "misc.h"
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
    eqt.dump();
    wa.dump();
    sm.dump();
    sr.dump();
    wm.dump();
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
    if(strlen(val) != 2)
        throw ETlgError("wrong action code '%s'", val);
    orig = DecodeOriginator(val[0]);
    action = DecodeAction(val[1]);
    if(orig == oUnknown)
        throw ETlgError("wrong orig %c", val[0]);
    if(action == aUnknown)
        throw ETlgError("wrong action %c", val[1]);
}

void TLCIFltInfo::dump()
{
  ProgTrace(TRACE5, "-----TLCIFltInfo::dump-----");
  flt.dump();
  ProgTrace(TRACE5, "airp: %s", airp.c_str());
  ProgTrace(TRACE5, "---------------------------");
}

void TLCIFltInfo::parse(const char *val)
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

}

TTlgPartInfo ParseLCIHeading(TTlgPartInfo heading, TLCIHeadingInfo &info)
{
    ProgTrace(TRACE5, "ParseLCIHeading: heading.p = '%s'", heading.p);
    TTlgPartInfo next;
    char *p, *line_p;
    TTlgParser tlg;
    int line;
    try
    {
        line_p=heading.p;
        line=heading.line-1;
        do
        {
            line++;
            if ((p=tlg.GetToEOLLexeme(line_p))==NULL) continue;
            info.flt_info.parse(tlg.lex);
            next.p=tlg.NextLine(line_p);
            next.line=line+1;
            return next;
        }
        while ((line_p=tlg.NextLine(line_p))!=NULL);
    }
    catch(ETlgError E)
    {
        //¢ë¢¥áâ¨ ®è¨¡ªã+­®¬¥à áâà®ª¨
        throw ETlgError("LCI, Line %d: %s",line,E.what());
    };
    next.p=line_p;
    next.line=line;
    return next;
};

void TDest::parse(const char*val)
{
}

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
    TElemFmt fmt;
    string lang;
    for(string::const_iterator is = val.begin(); is != val.end(); is++) {
        if(IsUpperLetter(*is))
        {
            if(not str_count.empty()) {
                if(str_cls.empty())
                    throw ETlgError("wrong CFG %s", val.c_str());
                string subcls = ElemToElemId(el, str_cls, fmt, lang);
                if(subcls.empty())
                    throw ETlgError("unknown %s %s", EncodeElemType(el), str_cls.c_str());
                insert(make_pair(subcls, ToInt(str_count)));
                str_count.erase();
            }
            str_cls = *is;
        } else {
            str_count.append(1, *is);
        }
    }
    if(str_cls.empty() or str_count.empty())
        throw ETlgError("wrong CFG %s", val.c_str());
    string subcls = ElemToElemId(el, str_cls, fmt, lang);
    if(subcls.empty())
        throw ETlgError("unknown %s %s", EncodeElemType(el), str_cls.c_str());
    insert(make_pair(subcls, ToInt(str_count)));
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
    if(not empty())
        throw ETlgError("SR items already exists %s", val.c_str());
    vector<string> result = split(val, '/');
    for(vector<string>::iterator iv = result.begin(); iv != result.end(); iv++)
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
    if(items.size() < 3) throw ETlgError("wrong item count within SR %s", val);
    if(items[1].size() != 1)
        throw ETlgError("SR wrong type %s", items[1].c_str());
    switch(items[1][0]) {
        case 'C':
            c.parse(items[2], etClass);
            break;
        case 'Z':
            z.parse(items[2]);
            break;
        case 'R':
            r.parse(items[2]);
            break;
        case 'S':
            s.parse(items[2]);
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
    for(map<string, TGenderWeight>::iterator im = begin(); im != end(); im++) {
        ProgTrace(TRACE5, "subcls: %s", im->first.c_str());
        im->second.dump();
    }
    ProgTrace(TRACE5, "----------------------------");
}

void TClsGenderWeight::parse(const std::vector<std::string> &val)
{
    TElemFmt fmt;
    string lang;
    for(vector<string>::const_iterator iv = val.begin(); iv != val.end(); iv++) {
        if(iv->size() < 2)
            throw ETlgError("wrong cls gender item %s", iv->c_str());
        string asubcls = iv->substr(0, 1);
        string subcls = ElemToElemId(etSubcls, asubcls, fmt, lang);
        if(subcls.empty())
            throw ETlgError("unknown subcls %s", iv->c_str());
        vector<string> genders(1, iv->substr(1));
        TGenderWeight g;
        g.parse(genders);
        insert(make_pair(subcls, g));
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
    TElemFmt fmt;
    string lang;
    for(vector<string>::const_iterator iv = val.begin(); iv != val.end(); iv++) {
        if(iv->size() < 2)
            throw ETlgError("wrong cls item %s", iv->c_str());
        string asubcls = iv->substr(0, 1);
        string subcls = ElemToElemId(etSubcls, asubcls, fmt, lang);
        if(subcls.empty())
            throw ETlgError("unknown subcls %s", iv->c_str());
        insert(make_pair(subcls, ToInt(iv->substr(1))));
    }
}

void TGenderWeight::dump()
{
    ProgTrace(TRACE5, "---TGenderWeight::dump---");
    TSubTypeHolder::dump();
    ProgTrace(TRACE5, "m: %d, f: %d, c: %d, i: %d", m, f, c, i);
    ProgTrace(TRACE5, "-------------------------");
}

void TGenderWeight::parse(const std::vector<std::string> &val)
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
        // ‚ ®áâ ¢è¨åáï í«¥¬¥­â å á®¤¥à¦¨âáï ¨­ä  ¯® sub_type
        TWMSubType sub_type = DecodeWMSubType(items[0]);
        if(sub_type == wmsUnknown) {
            sth = tr1::shared_ptr<TSubTypeHolder>(new TSimpleWeight);
        } else {
            items.erase(items.begin(), items.begin() + 1); // ¨§¡ ¢«ï¥¬áï ®â ¨¤¥­â¨ä¨ª â®à  sub_type
            if(not items.empty())
                switch(sub_type) {
                    case wmsGender:
                        sth = tr1::shared_ptr<TSubTypeHolder>(new TGenderWeight);
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

void ParseLCIContent(TTlgPartInfo body, TLCIHeadingInfo& info, TLCIContent& con, TMemoryManager &mem)
{
    con.Clear();
    TTlgParser tlg;
    char *line_p=body.p;
    int line=body.line;
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
            line++;
        } while (line_p and *line_p != 0);
    }
    catch (ETlgError E)
    {
        if (tlg.GetToEOLLexeme(line_p)!=NULL)
            throw ETlgError("LCI: %s\n>>>>>LINE %d: %s",E.what(),line,tlg.lex);
        else
            throw ETlgError("LCI: %s\n>>>>>LINE %d",E.what(),line);
    };
}

void SaveLCIContent(int tlg_id, TLCIHeadingInfo& info, TLCIContent& con)
{
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
       "ž’001/29JUL.„Œ„\n"
       "CF\n"
       "EQT.ASDF.’“5.012122\n"
       "WA.P.1485.KG\n"
       "WA.U.0.KG\n"
       "SM.S\n"
       "SR.C.11115\n"
       "SR.Z.0F22/0Z104\n"
       "WM.S.P.CG.100/50/25/20.80/75/50/25.75/70/45/20.KG\n"
       "WM.A.B.KG\n"
       "-‘Ž—.PT.0.C.0/0/0\n"
       "-‘Ž—.BT.0/0.C.0/0/0.A.0/0/0.KG\n"
       "-‘Ž—.H.0.KG\n"
       "-‚Š.PT.2.C.0/0/2\n"
       "-‚Š.BT.1/500.C.0/0/1.A.0/0/500.KG\n"
       "-‚Š.H.0.KG\n"
       "-‹Š.PT.1.C.0/0/1\n"
       "-‹Š.BT.1/500.C.0/0/1.A.0/0/500.KG\n"
       "-‹Š.H.0.KG\n"
       "-‚.PT.3.C.0/0/3\n"
       "-‚.BT.0/0.C.0/0/0.A.0/0/0.KG\n"
       "-‚.H.0.KG\n"
       "-€Ÿ’.PT.2.C.0/1/1\n"
       "-€Ÿ’.BT.0/0.C.0/0/0.A.0/0/0.KG\n"
       "-€Ÿ’.H.45.KG\n"
       "PD.C..0/0/0/0..1/0/0/0..4/1/1/1\n"
       "SP.5€/M.5/F.5‚/M.5ƒ/M.5„/I.5…/C.1€/M.6€/M\n";
       */

    THeadingInfo *HeadingInfo=NULL;
    TLCIContent con;

    try {
        TTlgParts parts = GetParts(buffer,mem);

        ParseHeading(parts.heading,HeadingInfo,mem);  //¬®¦¥â ¢¥à­ãâì NULL
        if(HeadingInfo == NULL)
            throw Exception("HeadingInfo is null");

        TLCIHeadingInfo &info = *(dynamic_cast<TLCIHeadingInfo*>(HeadingInfo));
        info.flt_info.dump();

        ParseLCIContent(parts.body, info, con, mem);
        con.dump();

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
