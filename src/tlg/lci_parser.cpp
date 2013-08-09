#include "lci_parser.h"
#include "misc.h"

#define STDLOG NICKNAME,__FILE__,__LINE__
#define NICKNAME "DEN"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;


namespace TypeB
{

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

void TCFG::parse(const string &val)
{
    string str_cls;
    string str_count;
    TElemFmt fmt;
    string lang;
    for(string::const_iterator is = val.begin(); is != val.end(); is++) {
        if(IsUpperLetter(*is))
        {
            ProgTrace(TRACE5, "IsUpperLetter *is: %c", *is);
            if(not str_count.empty()) {
                if(str_cls.empty())
                    throw ETlgError("wrong CFG %s", val.c_str());
                string subcls = ElemToElemId(etSubcls, str_cls, fmt, lang);
                if(subcls.empty())
                    throw ETlgError("unknown subclass %s", str_cls.c_str());
                insert(make_pair(subcls, ToInt(str_count)));
                str_count.erase();
            }
            str_cls = *is;
        } else {
            ProgTrace(TRACE5, "ELSE IsUpperLetter *is: %c", *is);
            str_count.append(1, *is);
        }
    }
    if(str_cls.empty() or str_count.empty())
        throw ETlgError("wrong CFG %s", val.c_str());
    string subcls = ElemToElemId(etSubcls, str_cls, fmt, lang);
    if(subcls.empty())
        throw ETlgError("unknown subclass %s", str_cls.c_str());
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
    string acraft;
    vector<string> items = split(val, '.');
    if(items.size() == 4) {
        bort = items[1];
        acraft = items[2];
        cfg.parse(items[3]);
        if(bort.empty()) throw ETlgError("bort is empty");
    } else if(items.size() == 3) {
        acraft = items[1];
        cfg.parse(items[2]);
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
    ProgTrace(TRACE5, "TWA val: %s", val);
    vector<string> items = split(val, '.');
    if(items.size() != 4)
        throw ETlgError("wrong WA %s", val);
    ProgTrace(TRACE5, "Indicator: %s", items[1].c_str());
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
            throw ETlgError("SSM: %s\n>>>>>LINE %d: %s",E.what(),line,tlg.lex);
        else
            throw ETlgError("SSM: %s\n>>>>>LINE %d",E.what(),line);
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
