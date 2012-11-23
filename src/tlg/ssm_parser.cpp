#include "ssm_parser.h"
#include "misc.h"

#define STDLOG NICKNAME,__FILE__,__LINE__
#define NICKNAME "DEN"
#include "serverlib/test.h"
#include "serverlib/posthooks.h"
#include "tlg.h"

using namespace std;
using namespace BASIC;
using namespace ASTRA;
using namespace EXCEPTIONS;

namespace TypeB
{

const string SUB_SEPARATOR = "//";
const string SI_INDICATOR = "SI";

bool isLatUpperString(string val)
{
//!!! non standard    if(not is_lat(val)) return false;
    string::iterator is = val.begin();
    for(; is != val.end(); is++) {
        if(not IsLetter(*is)) continue;
        if(not IsUpperLetter(*is)) break;
    }
    return is == val.end();
}

const char *TActionIdentifierS[] =
{
    "NEW",
    "CNL",
    "RIN",
    "RPL",
    "SKD",
    "ACK",
    "ADM",
    "CON",
    "EQT",
    "FLT",
    "NAC",
    "REV",
    "RSD",
    "RRT",
    "TIM",
    "DEL", //!!! not found in standard
    ""
};

TActionIdentifier DecodeActionIdentifier(const char* s)
{
    unsigned int i;
    for(i=0;i<sizeof(TActionIdentifierS)/sizeof(TActionIdentifierS[0]);i+=1) if (strcmp(s,TActionIdentifierS[i])==0) break;
    if (i<sizeof(TActionIdentifierS)/sizeof(TActionIdentifierS[0]))
        return (TActionIdentifier)i;
    else
        return aiUnknown;
};

const char* EncodeActionIdentifier(TActionIdentifier p)
{
    return TActionIdentifierS[p];
};

void TDEI_8::dump()
{
    ProgTrace(TRACE5, "-----DEI 8-----");
    ProgTrace(TRACE5, "TRC: %c", TRC);
    if(DEI != NoExists)
        ProgTrace(TRACE5, "DEI: %d", DEI);
    ProgTrace(TRACE5, "data: '%s'", data.c_str());
    ProgTrace(TRACE5, "----------------");
}

void TDEI_8::parse(const char *val)
{
    char TRC[2]; // Traffic Restriction Code
    char DEI[4];
    int fmt = 1;
    char c;
    int res;
    while(fmt) {
        *TRC = 0;
        *DEI = 0;
        switch(fmt) {
            case 1:
                fmt = 0; c = 0;
                res = sscanf(val, "8/%1[A-ZА-ЯЁ]%c", TRC, &c);
                if(c != 0 or res != 1) fmt = 2;
                break;
            case 2:
                fmt = 0; c = 0;
                res = sscanf(val, "8/%1[A-ZА-ЯЁ]/%3[0-9]%c", TRC, DEI, &c);
                if(c != 0 or res != 2) fmt = 3;
                break;
            case 3:
                fmt = 0; c = 0;
                res = sscanf(val, "8/%1[A-ZА-ЯЁ]/%3[0-9]/%s", TRC, DEI, lexh);
                if(res != 3)
                    throw ETlgError("wrong format");
                break;
        }
    }
    if(strlen(DEI) != 0 and strlen(DEI) != 3)
        throw ETlgError("wrong format");
    this->TRC = *TRC;
    if(*DEI) StrToInt(DEI, this->DEI);
    data = lexh;
    if(this->DEI != NoExists) {
        if(
                (this->DEI < 170 or
                 this->DEI > 173) and
                (this->DEI < 710 or
                 this->DEI > 799)
          )
            throw ETlgError("TDEI_8::parse: wrong DEI %d", this->DEI);
        if(
                this->TRC == 'Z' and 
                this->DEI >= 170 and
                this->DEI <= 173 and
                data.empty()
          )
            throw ETlgError("TDEI_8::parse: data must be stated for TRC = Z; DEI = %d", this->DEI);
        if(
                not data.empty() and
                this->DEI >= 710 and
                this->DEI <= 712
          )
            throw ETlgError("TDEI_8::parse: data must not be stated for DEI = %d", this->DEI);
        if(
                data.empty() and
                this->DEI >= 713 and
                this->DEI <= 779
          )
            throw ETlgError("TDEI_8::parse: data must be stated for DEI = %d", this->DEI);
    }
}

void TDEI_7::dump()
{
    ProgTrace(TRACE5, "-----DEI 7-----");
    int idx = 0;
    for(vector<TMealItem>::iterator iv = meal_service.begin(); iv != meal_service.end(); iv++, idx++)
        ProgTrace(TRACE5, "meal %d: class: '%s', meal: '%s'", idx, iv->cls.c_str(), iv->meal.c_str());
    ProgTrace(TRACE5, "----------------");
}

bool isLetterString(const string &val)
{
    for(string::const_iterator is = val.begin(); is != val.end(); is++)
        if(not IsLetter(*is))
            return false;
    return true;
}

void TDEI_7::insert(bool default_meal, string &meal)
{
    if(
            not isLetterString(meal) or
            not default_meal and (meal.size() > 3 or meal.size() < 2) or
            default_meal and (meal.size() > 2 or meal.empty())
      )
        throw Exception("wrong meal format '%s'", meal.c_str());
    TMealItem meal_item;
    if(default_meal) {
        // Для всех классов в PRBD, которые правее
        // последнего не пустого в этом списке
        // присваивается meal по умолчанию
        // Напр. PRBD = FCYML
        // DEI_7 = 7/FDC/CD/YS/MS/LS - для всех классов определены meal
        // DEI_7 = 7/FDC/CD//S - для всех классов правее С из PRBD (т.е. Y, M, L) определен meal S
        meal_item.meal = meal;
    } else {
        meal_item.cls.assign(1, meal[0]);
        TElemFmt fmt;
        string lang;
        meal_item.cls = ElemToElemId(etSubcls, meal_item.cls, fmt, lang);
        meal_item.meal = meal.substr(1);
    }
    meal_service.push_back(meal_item);
}

void TDEI_7::parse(const char *val)
{
    string buf = val;
    if(buf.substr(0, 2) != "7/")
        throw ETlgError("wrong DEI 7 identifier: '%s'", buf.substr(0, 2).c_str());
    buf.erase(0, 2); // '7/' get off
    bool default_meal = false;
    while(true) {
        size_t idx = buf.find('/');
        if(idx == string::npos) {
            if(buf.empty())
                throw ETlgError("wrong meal format");
            insert(default_meal, buf);
            break;
        } else if(default_meal)
            throw ETlgError("wrong format");
        string meal = buf.substr(0, idx);
        if(meal.empty()) { // 2 слеша подряд
            default_meal = true;
        } else {
            insert(default_meal, meal);
        }

        buf.erase(0, idx + 1);
    }
}

void TDEI_6::dump()
{
    ProgTrace(TRACE5, "-----DEI 6-----");
    ProgTrace(TRACE5, "airline: '%s'", airline.c_str());
    ProgTrace(TRACE5, "flt_no: %d", flt_no);
    if(suffix)
        ProgTrace(TRACE5, "suffix: %c", suffix);
    if(layover != NoExists)
        ProgTrace(TRACE5, "layover: %d", layover);
    if(date != NoExists)
        ProgTrace(TRACE5, "date: %s", DateTimeToStr(date, "dd.mm.yyyy").c_str());
    ProgTrace(TRACE5, "----------------");
}

void TDEI_6::parse(const char *val)
{
    string buf = val;
    buf.erase(0, 2); // '6/' ripped off
    size_t idx = buf.find('/');
    TSSMFltInfo flt_info;
    flt_info.parse(buf.substr(0, idx).c_str());
    airline = flt_info.airline;
    flt_no = flt_info.flt_no;
    suffix = *flt_info.suffix;
    buf.erase(0, idx + 1);
    if(not buf.empty()) {
        if(tlg_type == tcSSM)
            StrToInt(buf.c_str(), layover);
        else if(tlg_type == tcASM)
            date = ParseDate(buf.c_str());
        else
            throw ETlgError("unexpected tlg category");
    }
}

void TDEI_airline::dump()
{
    ProgTrace(TRACE5, "-----DEI %d-----", id);
    ProgTrace(TRACE5, "airline: %s", airline);
    ProgTrace(TRACE5, "----------------");
}

void TDEI_airline::parse(const char *val)
{
    string format = IntToString(id) + "/%3[A-ZА-ЯЁ0-9]%c";
    char c = 0;
    int res = sscanf(val, format.c_str(), airline, &c);
    if(c != 0 or res != 1)
        throw ETlgError("wrong DEI %d format", id);
    if(not(strlen(airline) == 1 and *airline == 'X'))
        GetAirline(airline, true);
}

void TDEI_1::dump()
{
    ProgTrace(TRACE5, "-----DEI 1-----");
    int i = 1;
    for(vector<string>::iterator iv = airlines.begin(); iv != airlines.end(); iv++, i++)
        ProgTrace(TRACE5, "airline %d: '%s'", i, iv->c_str());
    ProgTrace(TRACE5, "----------------");
}

void TDEI_1::parse(const char *val)
{
    char airline1[4];
    char airline2[4];
    char airline3[4];
    *airline3 = 0;
    char c = 0;
    int res = sscanf(val, "1/%3[A-ZА-ЯЁ0-9]/%3[A-ZА-ЯЁ0-9]/%3[A-ZА-ЯЁ0-9]%c", airline1, airline2, airline3, &c);
    if(c != 0 or res != 3) {
        c = 0;
        *airline3 = 0;
        res = sscanf(val, "1/%3[A-ZА-ЯЁ0-9]/%3[A-ZА-ЯЁ0-9]%c", airline1, airline2, &c);
        if(c != 0 or res != 2)
            throw ETlgError("wrong DEI 1");
    }
    airlines.push_back(GetAirline(airline1));
    airlines.push_back(GetAirline(airline2));
    if(*airline3)
        airlines.push_back(GetAirline(airline3));
}

void TActionInfo::parse(char *val)
{
    int res = sscanf(val, "%3[A-Z]%[ XASM]", lexh, val);
    if(res < 1 or strlen(lexh) != 3)
        throw ETlgError("wrong Action Identifier");
    id = DecodeActionIdentifier(lexh);
    if(id == aiUnknown)
        throw ETlgError("Unknown Action Identifier '%s'", lexh);
    if(res == 2) {
        if(strcmp(val, " XASM") != 0)
            throw ETlgError("wrong XASM");
        xasm = true;
        if( not (
                    id == aiNEW or
                    id == aiCNL or
                    id == aiRPL or
                    id == aiSKD
                )
          )
            throw ETlgError("XASM not applicable for Action Identifier '%s'", EncodeActionIdentifier(id));
    }
}

void TSSMFltInfo::parse(const char *val)
{
    char flt[9];
    char c = 0;
    suffix[1] = 0;
    int res=sscanf(val,"%8[A-ZА-ЯЁ0-9]%c",flt,&c);
    if(c !=0 or res != 1) throw ETlgError("wrong flight");
    if (IsDigit(flt[2]))
        res=sscanf(flt,"%2[A-ZА-ЯЁ0-9]%5lu%c%c",
                airline,&flt_no,&(suffix[0]),&c);
    else
        res=sscanf(flt,"%3[A-ZА-ЯЁ0-9]%5lu%c%c",
                airline,&flt_no,&(suffix[0]),&c);
    if (c!=0||res<2||flt_no<0) throw ETlgError("Wrong flight");
    if (res==3&&
            !IsUpperLetter(suffix[0])) throw ETlgError("Wrong flight");
    GetAirline(airline);
    GetSuffix(suffix[0]);
}

void TPeriodOfOper::parse(char *&ph, TTlgParser &tlg)
{
    // Parse Existing period of Operation
    parse(true, tlg.lex);
    ph = tlg.GetLexeme(ph);
    parse(false, tlg.lex);
    // Day(s) of Operation
    if((ph = tlg.GetLexeme(ph)) == NULL)
        throw ETlgError("Day(s) of Operation not found");
    char c = 0;
    int res = sscanf(tlg.lex, "%7[0-9]%c", oper_days, &c);
    if(c != 0 or res != 1) {
        c = 0;
        char buf[3];
        res = sscanf(tlg.lex, "%7[0-9]/%2[W2]%c", oper_days, buf, &c);
        if(c != 0 or res != 2 or strcmp(buf, "W2") != 0)
            throw ETlgError("wrong Day(s) of Operation / Frequency Rate format");
        rate = frW2;
    }
}

void TPeriodOfOper::parse(bool pr_from, const char *val)
{
    try {
        if(*val == 0) throw Exception("not found");
        if(pr_from)
            from.parse(val);
        else
            to.parse(val);
    } catch(Exception &E) {
        throw ETlgError("period of operation, %s: %s", (pr_from ? "From Date" : "To Date"), E.what());
    }

}

void TSSMDate::parse(const char *val)
{
    char sday[3];
    char smonth[4];
    char syear[3];
    *syear = 0;
    char c = 0;
    int res;
    res = sscanf(val, "%2[0-9]%3[A-ZА-ЯЁ]%2[0-9]%c", sday, smonth, syear, &c);
    if(c != 0 or res != 3) {
        *syear = 0;
        c = 0;
        res=sscanf(val,"%2[0-9]%3[A-ZА-ЯЁ]%c", sday, smonth, &c);
        if(c != 0 or res != 2)
            throw Exception("wrong format");
    }
    if(
            strlen(sday) != 2 or
            strlen(smonth) != 3 or
            res == 3 and strlen(syear) != 2
      )
        throw Exception("wrong format");

    TDateTime today=NowUTC();
    int year,mon,currday, day;
    StrToInt(sday, day);

    if(*syear != 0)
        StrToInt(syear, year);
    else
        DecodeDate(today,year,mon,currday);

    try
    {
        for(mon=1;mon<=12;mon++)
            if (strcmp(smonth,Months[mon-1].lat)==0||
                    strcmp(smonth,Months[mon-1].rus)==0) break;
        EncodeDate(year,mon,day,date);
    }
    catch(EConvertError)
    {
        throw ETlgError("Can't convert UTC date");
    };
}

void TDEIHolder::dump()
{
    ProgTrace(TRACE5, "----DEI list");
    if(not dei1.empty()) dei1.dump();
    if(not dei2.empty()) dei2.dump();
    if(not dei3.empty()) dei3.dump();
    if(not dei4.empty()) dei4.dump();
    if(not dei5.empty()) dei5.dump();
    if(not dei6.empty()) dei6.dump();
    if(not dei7.empty()) dei7.dump();
    if(not dei9.empty()) dei9.dump();
}

void TDEIHolder::parse(TTlgElement e, char *&ph, TTlgParser &tlg)
{
    int last_id = NoExists;
    while(true) {
        if(last_id == 9) break;
        ph = tlg.GetLexeme(ph);
        if(not ph) break;
        char idx[2];
        int res = sscanf(tlg.lex, "%1[0-9]/", idx);
        if(res != 1)
            throw ETlgError("wrong DEI format");
        int id;
        StrToInt(idx, id);
        TDEI *dei = NULL;
        if(last_id != NoExists and id <= last_id)
            throw ETlgError("DEI %d wrong order", id);
        if(
                tlg_type == tcSSM and (
                    e == FlightElement and (id == 6 or id == 7) or
                    e == PeriodFrequency and id == 7 or
                    e == Equipment and (id == 1 or id == 7)
                    ) or
                tlg_type == tcASM and (
                    e == FlightElement and (id == 7 or id == 9) or
                    e == Equipment and (id == 1 or id == 7)
                    )
          )
            throw ETlgError("unapplicable DEI %d for %s", id, GetTlgElementName(e));
        switch(id) {
            case 1:
                dei = &dei1;
                break;
            case 2:
                dei = &dei2;
                break;
            case 3:
                dei = &dei3;
                break;
            case 4:
                dei = &dei4;
                break;
            case 5:
                dei = &dei5;
                break;
            case 6:
                dei = &dei6;
                break;
            case 7:
                dei = &dei7;
                break;
            case 9:
                dei = &dei9;
                break;
            default:
                throw ETlgError("unknown DEI id %d", id);
        }
        if(not dei->empty())
            throw ETlgError("DEI %d was already parsed", id);
        dei->parse(tlg.lex);
        last_id = id;
    }
}

void TRouting::parse_leg_airps(string buf)
{
    try {
        TElemFmt fmt;
        string lang;
        for(size_t idx = 0; idx != string::npos; idx = buf.find('/')) {
            if(not idx) continue;
            string airp = buf.substr(0, idx);
            if(airp.size() != 3)
                throw Exception("wrong airp");
            leg_airps.push_back(ElemToElemId(etAirp, airp, fmt, lang));
            buf.erase(0, idx + 1);
        }
        if(buf.size() != 3)
            throw Exception("wrong airp");
        leg_airps.push_back(ElemToElemId(etAirp, buf, fmt, lang));
        if(leg_airps.size() > 12 or
                leg_airps.size() < 2)
            throw Exception("wrong airp");
    } catch(Exception &E) {
        leg_airps.clear();
    }
}

void TRouteStation::dump()
{
    if(empty()) return;
    ProgTrace(TRACE5, "----TRouteStation----");
    ProgTrace(TRACE5, "airp: %s", airp);
    ProgTrace(TRACE5, "scd: %s", (scd == NoExists ? "NoExists" : DateTimeToStr(scd, "hh:nn").c_str()));
    ProgTrace(TRACE5, "pax_scd: %s", (pax_scd == NoExists ? "NoExists" : DateTimeToStr(pax_scd, "hh:nn").c_str()));
    ProgTrace(TRACE5, "date_variation: %d", date_variation);
    ProgTrace(TRACE5, "---------------------");
}

void TRouteStation::parse(const char *val)
{
    char scd[5], day[3], pax_scd[5], c = 0;
    int res;
    int fmt = 1;
    while(fmt) {
        *airp = 0;
        *scd = 0;
        *day = 0;
        *pax_scd = 0;
        switch(fmt) {
            case 1:
                fmt = 0; c = 0;
                res = sscanf(val, "%3[A-ZА-ЯЁ]%4[0-9]/%2[M0-9]/%4[0-9]%c", airp, scd, day, pax_scd, &c);
                if(c != 0 or res != 4) fmt = 2;
                break;
            case 2:
                fmt = 0; c = 0;
                res = sscanf(val, "%3[A-ZА-ЯЁ]%4[0-9]/%4[0-9]%c", airp, scd, pax_scd, &c);
                if(c != 0 or res != 3 or strlen(pax_scd) != 4) fmt = 3;
                break;
            case 3:
                fmt = 0; c = 0;
                res = sscanf(val, "%3[A-ZА-ЯЁ]%4[0-9]/%2[M0-9]%c", airp, scd, day, &c);
                if(c != 0 or res != 3) fmt = 4;
                break;
            case 4:
                fmt = 0; c = 0;
                res = sscanf(val, "%3[A-ZА-ЯЁ]%4[0-9]%c", airp, scd, &c);
                if(c != 0 or res != 2)
                    throw ETlgError("wrong format");
                break;
        }
    }
    if(
            strlen(airp) != 3 or
            strlen(scd) != 4 or
            *pax_scd != 0 and strlen(pax_scd) != 4
      )
        throw ETlgError("wrong format");
    TlgElemToElemId(etAirp, airp, airp);
    StrToDateTime(scd, "hhnn", this->scd);
    if(*pax_scd)
        StrToDateTime(pax_scd, "hhnn", this->pax_scd);
    if(*day == 'M') {
        if(strlen(day) != 2)
            throw ETlgError("wrong route station day '%s'", day);
        else {
            StrToInt(day + 1, date_variation);
            date_variation = -date_variation;
        }
    } else
        StrToInt(day, date_variation);
}

void TSegment::parse(const char *val)
{
    raw_data = val;
    string buf = val;
    if(buf.size() < 8) throw ETlgError("wrong format");
    airp_dep = buf.substr(0, 3);
    airp_arv = buf.substr(3, 3);
    if(buf[6] != ' ') throw ETlgError("wrong format");
    buf.erase(0, 7);
    TElemFmt fmt;
    string lang;
    airp_dep = ElemToElemId(etAirp, airp_dep, fmt, lang);
    airp_arv = ElemToElemId(etAirp, airp_arv, fmt, lang);
    try {
        dei8.parse(buf.c_str());
    } catch(...) {
        other.parse(buf.c_str());
    }
}

void TOther::parse(const char *val)
{
    char DEI[4];
    int fmt = 1;
    char c;
    int res;
    *lexh = 0;
    while(fmt) {
        *DEI = 0;
        switch(fmt) {
            case 1:
                fmt = 0; c = 0;
                res = sscanf(val, "%3[0-9]%c", DEI, &c);
                if(c != 0 or res != 1) fmt = 2;
                break;
            case 2:
                fmt = 0; c = 0;
                res = sscanf(val, "%3[0-9]/%s", DEI, lexh);
                if(res != 2)
                    throw ETlgError("wrong format");
                break;
        }
    }
    if(strlen(DEI) != 0 and (strlen(DEI) < 2 or strlen(DEI) > 3))
        throw ETlgError("TOther::parse: wrong DEI format");
    StrToInt(DEI, this->DEI);
    data = lexh;
}

void TASMContent::dump()
{
    int idx = 0;
    for(vector<TASMSubMessage>::iterator iv = msgs.begin(); iv != msgs.end(); iv++, idx++) {
        ProgTrace(TRACE5, "ASM submessage %d", idx);
        iv->dump();
    }
    if(not si.empty()) {
        ProgTrace(TRACE5, "Whole Message SI");
        for(vector<string>::iterator iv = si.begin(); iv != si.end(); iv++)
            ProgTrace(TRACE5, "    %s", iv->c_str());
    }
}

void TSSMContent::dump()
{
    int idx = 0;
    for(vector<TSSMSubMessage>::iterator iv = msgs.begin(); iv != msgs.end(); iv++, idx++) {
        ProgTrace(TRACE5, "SSM submessage %d", idx);
        iv->dump();
    }
    if(not si.empty()) {
        ProgTrace(TRACE5, "Whole Message SI");
        for(vector<string>::iterator iv = si.begin(); iv != si.end(); iv++)
            ProgTrace(TRACE5, "    %s", iv->c_str());
    }
}

void TReject::dump()
{
    if(errors.empty()) return;
    ProgTrace(TRACE5, "------REJECT INFORMATION-------");
    for(vector<TError>::iterator iv = errors.begin(); iv != errors.end(); iv++)
        ProgTrace(TRACE5, "line: %d; err: %s", iv->line, iv->err.c_str());
    if(not rejected_msg.empty()) {
        ProgTrace(TRACE5, "------REJECTED MSG-------");
        for(vector<string>::iterator iv = rejected_msg.begin(); iv != rejected_msg.end(); iv++)
            ProgTrace(TRACE5, "%s", iv->c_str());
    }
}

void TEqtRouting::dump()
{
    ProgTrace(TRACE5, "Equipment information:");
    ProgTrace(TRACE5, "    service_type: %c", eqt.service_type);
    ProgTrace(TRACE5, "    aircraft: '%s'", eqt.aircraft);
    ProgTrace(TRACE5, "    PRBD: '%s'", eqt.PRBD.c_str());
    ProgTrace(TRACE5, "    PRBM: '%s'", eqt.PRBM.c_str());
    ProgTrace(TRACE5, "    craft_cfg: '%s'", eqt.craft_cfg.c_str());
    eqt.dei_holder.dump();

    ProgTrace(TRACE5, "Routing or leg information:");
    if(not routing.empty()) {
        int idx = 0;
        for(vector<TRouting>::iterator iv = routing.begin(); iv != routing.end(); iv++, idx++) {
            ProgTrace(TRACE5, "    leg #%d", idx);
            if(not iv->leg_airps.empty()) {
                ProgTrace(TRACE5, "----leg_airps list");
                for(vector<string>::iterator iv1 = iv->leg_airps.begin(); iv1 != iv->leg_airps.end(); iv1++)
                    ProgTrace(TRACE5, "%s", iv1->c_str());
            }
            ProgTrace(TRACE5, "    routing.station_dep:");
            iv->station_dep.dump();
            ProgTrace(TRACE5, "    routing.station_arv:");
            iv->station_arv.dump();
            iv->dei_holder.dump();
        }
    }
}

void TSSMSubMessage::dump()
{
    ProgTrace(TRACE5, "-------TSSMContent::dump()-----------");
    ProgTrace(TRACE5, "action_identifier: %s", EncodeActionIdentifier(ainfo.id));
    ProgTrace(TRACE5, "xasm: %s", (ainfo.xasm ? "true" : "false"));
    ProgTrace(TRACE5, "    Flight Information:");

    ostringstream buf;
    for(vector<TSchedule>::iterator i_scd = scd.begin(); i_scd != scd.end(); i_scd++) {
        ProgTrace(TRACE5, "schedule#");
        for(vector<TFlightInformation>::iterator fi = i_scd->flights.begin(); fi != i_scd->flights.end(); fi++)
            buf << " " << fi->flt.airline << fi->flt.flt_no;
        ProgTrace(TRACE5, "  flights: %s", buf.str().c_str());

        for(vector<TRoute>::iterator ir = i_scd->routes.begin(); ir != i_scd->routes.end(); ir++) {
            ProgTrace(TRACE5, "  route#");
            for(vector<TPeriods>::iterator ip = ir->periods.begin(); ip != ir->periods.end(); ip++) {
                ProgTrace(TRACE5, "    periods:");
                int idx = 0;
                ProgTrace(TRACE5, "      period_frequency:");
                for(vector<TPeriodFrequency>::iterator iv = ip->period_frequency.begin(); iv != ip->period_frequency.end(); iv++, idx++) {
                    ProgTrace(TRACE5, "        %s %s", 
                            DateTimeToStr(iv->oper_period.from.date, "dd.mm.yy").c_str(),
                            DateTimeToStr(iv->oper_period.to.date, "dd.mm.yy").c_str()
                            );
                }
                ProgTrace(TRACE5, "      eqt_routing:");
                for(vector<TEqtRouting>::iterator i = ip->eqt_routing.begin(); i != ip->eqt_routing.end(); i++) {
                    ProgTrace(TRACE5, "        eqt: %c %s %s", i->eqt.service_type, i->eqt.aircraft, i->eqt.craft_cfg.c_str());
                    ProgTrace(TRACE5, "        routing");
                    for(vector<TRouting>::iterator ir = i->routing.begin(); ir != i->routing.end(); ir++)
                        ProgTrace(TRACE5, "          %s %s", ir->station_dep.airp, ir->station_arv.airp);
                }
            }
            ProgTrace(TRACE5, "    common routing:");
            for(vector<TRouting>::iterator ip = ir->routing.begin(); ip != ir->routing.end(); ip++)
                ProgTrace(TRACE5, "      %s %s", ip->station_dep.airp, ip->station_arv.airp);

            ProgTrace(TRACE5, "    segments:");
            for(vector<TSegment>::iterator is = ir->segments.begin(); is != ir->segments.end(); is++)
                ProgTrace(TRACE5, "      %s %s", is->airp_dep.c_str(), is->airp_arv.c_str());
        }
    }

    if(*new_flt.airline) {
        ProgTrace(TRACE5, "New Flight Information:");
        ProgTrace(TRACE5, "    airline: %s", new_flt.airline);
        ProgTrace(TRACE5, "    flt_no: %lu", new_flt.flt_no);
        ProgTrace(TRACE5, "    suffix: %s", new_flt.suffix);
    }

    if(not si.empty()) {
        ProgTrace(TRACE5, "Sub-Message SI");
        for(vector<string>::iterator iv = si.begin(); iv != si.end(); iv++)
            ProgTrace(TRACE5, "    %s", iv->c_str());
    }
    reject_info.dump();
    ProgTrace(TRACE5, "-------------------------------------");
}

int ssm(int argc,char **argv)
{
    if(argc != 1) {
        cout << "Usage: ssm" << endl;
        return 1;
    }

    int processed = 0;
    int id = NoExists;
    TPerfTimer tm;
    tm.Init();
    try {
        TQuery Qry(&OraSession);
        Qry.SQLText = "select id, data from ssm_in where type = :type and nvl2(:id, id, 0) = nvl(:id, 0)";
        Qry.CreateVariable("type", otString, "SSM");
        //Qry.CreateVariable("id", otInteger, 200000);
        Qry.CreateVariable("id", otInteger, FNull);
        Qry.Execute();
        char buf[5000];

        TMemoryManager mem(STDLOG);
        string addr = (string)
            "ADDRESS\xa" +
            ".ADDRESS 010101\n";

        for(; not Qry.Eof; Qry.Next()) {
            id = Qry.FieldAsInteger("id");
            THeadingInfo *HeadingInfo=NULL;
            TSSMContent con;

            strcpy(buf, (addr + Qry.FieldAsString("data")).c_str());
            TTlgParts parts = GetParts(buf, mem);

            ParseHeading(parts.heading,HeadingInfo,mem);  //может вернуть NULL
            if(HeadingInfo == NULL)
                throw Exception("HeadingInfo is null, id: %d", id);
            if(HeadingInfo->tlg_cat == tcUnknown) {
                cout << "unknown heading, id: " << id << endl;
                continue;
            }
            TSSMHeadingInfo &info = *(dynamic_cast<TSSMHeadingInfo*>(HeadingInfo));
            if(info.msr.num > 1) {
                cout << id << ": " << info.msr.grp << " " << info.msr.num << endl;
            }


            ParseSSMContent(parts.body, info, con, mem);
            // con.dump();
            /*
            // Несколько периодов
            for(std::vector<TSSMSubMessage>::iterator iv = con.msgs.begin(); iv != con.msgs.end(); iv++)
            if(iv->period_frequency.size() > 1)
            cout << id << endl;
             */

            /*
               if(con.msgs.size() == 1 and con.msgs[0].eqt_routing.size() > 1)
               cout << id << endl;
             */

            /*
               for(std::vector<TSSMSubMessage>::iterator iv = con.msgs.begin(); iv != con.msgs.end(); iv++)
               if(iv->eqt_routing.size() > 1)
               throw Exception("eqt_routing found %d", id);
             */

            /*
               for(std::vector<TSSMSubMessage>::iterator iv = con.msgs.begin(); iv != con.msgs.end(); iv++)
               for(std::vector<TEqtRouting>::iterator eqt_item = iv->eqt_routing.begin(); eqt_item != iv->eqt_routing.end(); eqt_item++)
               if(eqt_item->routing.size() > 1)
               throw Exception("routing found %d", id);
             */

            processed++;
            if(processed % 1000 == 0)
                cout << "processed: " << processed << endl;
        }
    } catch(Exception &E) {
        cout << "PROCESS SSM FAILED: " << E.what() << "; processed: " << processed << endl;
        return 1;
    } catch(...) {
        cout << "UNEXPECTED ERROR id = " << id << endl;
        return 1;
    }

    cout << "total processed: " << processed << "; " << tm.PrintWithMessage() << endl;


    return 0;
}

bool IsUpperLetterString(const string &val)
{
    string::const_iterator is = val.begin();
    for(is = val.begin(); is != val.end(); is++)
        if(not IsUpperLetter(*is)) break;
    return is == val.end();
}

void TEquipment::parse(TTlgElement e, const char *val)
{
    raw_data = val;
    TTlgParser tlg;
    strcpy(lexh, val);
    char *ph = tlg.GetLexeme(lexh);
    if(
            strlen(tlg.lex) != 1 and
            not IsUpperLetter(tlg.lex[0])
      )
        throw ETlgError("wrong Service Type format");
    service_type = tlg.lex[0];
    ph = tlg.GetLexeme(ph);
    if(not ph or strlen(tlg.lex) != 3)
        throw ETlgError("wrong aircraft format");
    strcpy(aircraft, tlg.lex);
    ph = tlg.GetLexeme(ph);
    if(not ph /*or not IsUpperLetter(tlg.lex[0]) !!! non standard '.F0C0Y144'*/)
        throw ETlgError("wrong PRBD format");
    PRBD = tlg.lex;
    size_t idx = PRBD.find("/");
    if(idx != string::npos) {
        size_t idx2 = PRBD.find(".");
        if(idx2 < idx)
            throw ETlgError("/ must be earlier than .");
        PRBM = PRBD.substr(idx + 1, idx2 - idx - 1);
        PRBD.erase(idx, idx2 - idx);
    }
    idx = PRBD.find(".");
    if(idx != string::npos) {
        craft_cfg = PRBD.substr(idx + 1);
        PRBD.erase(idx);
    }
    if(not PRBD.empty() and not IsUpperLetter(PRBD[0]))
        throw ETlgError("first char in PRBD element must be letter");
    if(not PRBM.empty()) {
        if(not IsUpperLetterString(PRBM))
            throw ETlgError("PRBM must consists of letters only");
        if(PRBM.size() % 2 != 0)
            throw ETlgError("PRBM must be sequence of pairs");
    }
    dei_holder.parse(e, ph, tlg);
}

void TRouting::parse(TTlgElement e, TActionIdentifier aid, const char *val)
{
    TTlgParser tlg;
    strcpy(lexh, val);
    char *ph = tlg.GetLexeme(lexh);
    switch(aid) {
        case aiADM:
        case aiCON:
        case aiEQT:
            parse_leg_airps(tlg.lex);
            break;
        default:
            break;
    }
    switch(aid) {
        case aiCON:
        case aiEQT:
            ph = tlg.GetLexeme(ph);
            if(ph) throw ETlgError("Unknown lexeme");
            break;
        case aiNEW:
        case aiRPL:
        case aiTIM:
            {
                station_dep.parse(tlg.lex);
                ph = tlg.GetLexeme(ph);
                station_arv.parse(tlg.lex);
                dei_holder.parse(e, ph, tlg);
            }
            break;
        case aiADM:
            dei_holder.parse(e, ph, tlg);
            break;
        default:
            break;
    }
}

void ParseSSMContent(TTlgPartInfo body, TSSMHeadingInfo& info, TSSMContent& con, TMemoryManager &mem)
{
    con.Clear();
    TTlgParser tlg;
    char *line_p=body.p, *ph;
    int line=body.line;
    TTlgElement e = ActionIdentifier;
    TSSMSubMessage *ssm_msg = NULL;
    try
    {
        do {
            tlg.GetToEOLLexeme(line_p);
            if(not *tlg.lex and e != RejectBody and e != Reject)
                throw ETlgError("blank line not allowed here %s", GetTlgElementName(e));
            if(not isLatUpperString(tlg.lex))
                throw ETlgError("isLatUpperString failed");
            switch(e) {
                case ActionIdentifier:
                    con.msgs.push_back(TSSMSubMessage());
                    ssm_msg = &con.msgs.back();
                    ssm_msg->ainfo.parse(tlg.lex);

                    switch(ssm_msg->ainfo.id) {
                        case aiACK:
                            e = EndOfMessage;
                            break;
                        case aiNAC:
                            e = Reject;
                            break;
                        default:
                            e = FlightElement;
                            break;
                    }
                    break;
                case FlightElement:
                    {
                        try {
                            strcpy(lexh, tlg.lex);
                            ph = tlg.GetLexeme(lexh);
                            TFlightInformation flt_info;
                            flt_info.raw_data = tlg.lex;
                            flt_info.flt.parse(tlg.lex);
                            if(*flt_info.flt.suffix != 0 and
                                    (ssm_msg->ainfo.id == aiSKD or
                                     ssm_msg->ainfo.id == aiRSD
                                    )
                              )
                                throw ETlgError("flt suffix not allowed for Action Identifier '%s'", EncodeActionIdentifier(ssm_msg->ainfo.id));
                            if(ssm_msg->ainfo.id == aiREV) {
                                ph = tlg.GetLexeme(ph);
                                if(ph == NULL)
                                    throw ETlgError("Existing period of Operation not found");
                                flt_info.oper_period.parse(ph, tlg);
                                if((ph = tlg.GetLexeme(ph)) != NULL) throw ETlgError("Unknown lexeme");
                            }
                            if(
                                    ssm_msg->ainfo.id == aiSKD or 
                                    ssm_msg->ainfo.id == aiFLT or 
                                    ssm_msg->ainfo.id == aiRSD or 
                                    ssm_msg->ainfo.id == aiTIM
                              )
                                if((ph = tlg.GetLexeme(ph)) != NULL) throw ETlgError("Unknown lexeme");
                            flt_info.dei_holder.parse(e, ph, tlg);
                            ssm_msg->scd.add(flt_info);
                            break;
                        } catch(ETlgError &E) {
                            e = PeriodFrequency;
                            continue;
                        }
                    }
                case PeriodFrequency:
                    {
                        try {
                            TPeriodFrequency period_frequency;
                            period_frequency.raw_data = tlg.lex;
                            strcpy(lexh, tlg.lex);
                            if(
                                    ssm_msg->ainfo.id == aiSKD or
                                    ssm_msg->ainfo.id == aiRSD
                              ) {
                                ph = tlg.GetLexeme(lexh);
                                period_frequency.effective_date.parse(tlg.lex);
                                if((ph = tlg.GetLexeme(ph)))
                                    period_frequency.discontinue_date.parse(tlg.lex);
                                if((ph = tlg.GetLexeme(ph)))
                                    throw ETlgError("Unknown lexeme");
                            } else {
                                ph = tlg.GetLexeme(lexh);
                                if(ph == NULL)
                                    throw ETlgError("Period of Operation not found");
                                period_frequency.oper_period.parse(ph, tlg);
                            }
                            period_frequency.dei_holder.parse(e, ph, tlg);
                            ssm_msg->scd.add(period_frequency);
                            break;
                        } catch (ETlgError &E) {
                            switch(ssm_msg->ainfo.id) {
                                case aiFLT:
                                    e = NewFlight;
                                    break;
                                case aiCNL:
                                case aiSKD:
                                case aiREV:
                                case aiRSD:
                                    e = SubSI;
                                    break;
                                case aiNEW:
                                case aiRPL:
                                case aiCON:
                                case aiEQT:
                                    e = Equipment;
                                    break;
                                case aiADM:
                                case aiTIM:
                                    e = Routing;
                                    break;
                                default:
                                    e = EndOfMessage;
                                    break;
                            }
                            continue;
                        }
                    }
                case NewFlight:
                    {
                        ssm_msg->new_flt.parse(tlg.lex);
                        e = Segment;
                        break;
                    }
                case Equipment:
                    {
                        TEquipment eqt(tcSSM);
                        eqt.parse(Equipment, tlg.lex);
                        ssm_msg->scd.add(eqt);
                        e = Routing;
                        break;
                    }
                case Routing:
                    {
                        try {
                            TRouting routing(tcSSM);
                            routing.parse(Routing, ssm_msg->ainfo.id, tlg.lex);
                            ssm_msg->scd.add(routing);
                            break;
                        } catch(ETlgError &E) {
                            e = Segment;
                            continue;
                        }
                    }
                case Segment:
                    {
                        try {
                            TSegment seg;
                            seg.parse(tlg.lex);
                            ssm_msg->scd.add(seg);
                            break;
                        } catch(ETlgError &E) {
                            e = SubSI;
                        }
                    }
                case SubSI:
                    {
                        string buf = tlg.lex;
                        if(buf.substr(0, 2) == SI_INDICATOR) {
                            ssm_msg->si.push_back(buf.substr(3));
                            e = SubSIMore;
                            break;
                        } else {
                            e = SubSeparator;
                            continue;
                        }
                    }
                case SubSIMore:
                    if(SUB_SEPARATOR == tlg.lex) {
                        e = SubSeparator;
                        continue;
                    } else {
                        if(ssm_msg->si.size() == 3)
                            throw ETlgError("wrong format, sub msg SI must contain max of 3 lines");
                        ssm_msg->si.push_back(tlg.lex);
                        e = SubSIMore;
                        break;
                    }
                case SubSeparator:
                    {
                        if(SUB_SEPARATOR == tlg.lex) {
                            e = SI;
                            break;
                        } else {
                            e = FlightElement;
                            continue;
                        }
                    }
                case SI:
                    {
                        string buf = tlg.lex;
                        if(buf.substr(0, 2) == SI_INDICATOR) {
                            con.si.push_back(buf.substr(3));
                            e = SIMore;
                            break;
                        } else {
                            e = ActionIdentifier;
                            continue;
                        }
                    }
                case SIMore:
                    {
                        if(con.si.size() == 3)
                            throw ETlgError("wrong format, whole msg SI must contain max of 3 lines");
                        con.si.push_back(tlg.lex);
                        e = SIMore;
                        break;
                    }
                case Reject:
                    {
                        if(*tlg.lex) throw ETlgError("wrong format. blank line expected");
                        e = RejectBody;
                        break;
                    }
                case RejectBody:
                    {
                        if(*tlg.lex) {
                            string buf = tlg.lex;
                            if(buf.size() < 5) throw ETlgError("reject info line too short");
                            TError err;
                            StrToInt(buf.substr(0, 3).c_str(), err.line);
                            if(buf[3] != ' ') throw ETlgError("reject info line wrong format");
                            err.err = buf.substr(4);
                            ssm_msg->reject_info.errors.push_back(err);
                        } else {
                            if(ssm_msg->reject_info.errors.empty())
                                throw ETlgError("wrong format. Reject reason line expected");
                            e = RepeatOfRejected;
                        }
                        break;
                    }
                case RepeatOfRejected:
                    {
                        if(not *tlg.lex)
                            throw ETlgError("wrong format. Rejected msg line expected");
                        ssm_msg->reject_info.rejected_msg.push_back(tlg.lex);
                        break;
                    }
                case EndOfMessage:
                    throw ETlgError("unknown lexeme");
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
    return;
}

void SaveNEW(TSSMHeadingInfo &info, TSSMContent &con, TSSMSubMessage &msg)
{
}

void SaveSSMContent(int tlg_id, TSSMHeadingInfo& info, TSSMContent& con)
{
    for(vector<TSSMSubMessage>::iterator iv = con.msgs.begin(); iv != con.msgs.end(); iv++) {
        switch(iv->ainfo.id) {
            case aiNEW:
                SaveNEW(info, con, *iv);
                break;
            case aiCNL:
                break;
            case aiRPL:
                break;
            case aiSKD:
                break;
            case aiACK:
                break;
            case aiADM:
                break;
            case aiCON:
                break;
            case aiEQT:
                break;
            case aiFLT:
                break;
            case aiNAC:
                break;
            case aiREV:
                break;
            case aiRSD:
                break;
            case aiTIM:
                break;
            default:
                throw ETlgError("unknown id %s", EncodeActionIdentifier(iv->ainfo.id));
        }
    }
}

void TASMActionInfo::parse(const char *val)
{
    string buf = val;
    if(buf.size() < 3)
        throw ETlgError("wrong Action Identifier: %s", buf.c_str());
    id = DecodeActionIdentifier(buf.substr(0, 3).c_str());
    buf.erase(0, 3);
    if(buf.empty()) return;
    if(
            not buf.empty() and (
                id == aiRPL or
                id == aiCON or
                id == aiEQT or
                id == aiRRT or
                id == aiTIM
                )
      ) { // fill secondary acion identifiers
        while(not buf.empty() and buf[0] == '/') {
            if(buf.size() < 4)
                throw ETlgError("wrong secondary action id: %s", buf.c_str());
            secondary_ids.push_back(DecodeActionIdentifier(buf.substr(1, 3).c_str()));
            buf.erase(0, 4);
        }
        if(secondary_ids.size() > 5)
            throw ETlgError("too many secondary action ids: %d", secondary_ids.size());
    }
    if(not buf.empty() and id != aiACK and id != aiNAC) {
        if(buf[0] != ' ')
            throw ETlgError("wrong format. Expected space before reasons");
        buf[0] = '/';
        while(not buf.empty() and buf[0] == '/') {
            reasons.push_back(buf.substr(1, 4));
            buf.erase(0, 5);
            if(reasons.back().size() != 4 or not IsUpperLetterString(reasons.back()))
                throw ETlgError("wrong reason format: %s", reasons.back().c_str());
        }
        if(reasons.size() > 9)
            throw ETlgError("too many Change Reasons: %d", reasons.size());
    }
    if(not buf.empty())
        throw ETlgError("unexpected data in Action Info");
};

void TASMActionInfo::dump()
{
    ProgTrace(TRACE5, "Action Identifier: %s", EncodeActionIdentifier(id));
    for(vector<TActionIdentifier>::iterator iv = secondary_ids.begin(); iv != secondary_ids.end(); iv++)
        ProgTrace(TRACE5, "Secondary Action ID: %s", EncodeActionIdentifier(*iv));
    for(vector<string>::iterator iv = reasons.begin(); iv != reasons.end(); iv++)
        ProgTrace(TRACE5, "Reason: %s", iv->c_str());
};

void TASMSubMessage::dump()
{
    ProgTrace(TRACE5, "-------TASMContent::dump()-----------");
    ainfo.dump();
    if(not flts.empty()) {
        int idx = 0;
        for(vector<TASMFlightInfo>::iterator iv = flts.begin(); iv != flts.end(); iv++, idx++) {
            ProgTrace(TRACE5, "------Flight #%d", idx);
            iv->dump();
        }
    }
    if(not eqt_routing.empty()) {
        int idx = 0;
        for(vector<TEqtRouting>::iterator iv = eqt_routing.begin(); iv != eqt_routing.end(); iv++, idx++) {
            ProgTrace(TRACE5, "-------TEqtRouting item %d---------", idx);
            iv->dump();
        }
    }
    if(not segs.empty()) {
        ProgTrace(TRACE5, "Segment information:");
        int idx = 0;
        for(vector<TSegment>::iterator iv = segs.begin(); iv != segs.end(); iv++, idx++) {
            ProgTrace(TRACE5, "    seg #%d", idx);
            ProgTrace(TRACE5, "    airp_dep: %s", iv->airp_dep.c_str());
            ProgTrace(TRACE5, "    airp_arv: %s", iv->airp_arv.c_str());
            if(not iv->dei8.empty())
                iv->dei8.dump();
            if(iv->other.DEI != NoExists) {
                ProgTrace(TRACE5, "    other.DEI: %d", iv->other.DEI);
                ProgTrace(TRACE5, "    other.data: %s", iv->other.data.c_str());
            }
        }
    }
    if(not si.empty()) {
        ProgTrace(TRACE5, "Sub-Message SI");
        for(vector<string>::iterator iv = si.begin(); iv != si.end(); iv++)
            ProgTrace(TRACE5, "    %s", iv->c_str());
    }
    reject_info.dump();
    ProgTrace(TRACE5, "-------------------------------------");
}

void TFlightIdentifier::dump()
{
    ProgTrace(TRACE5, "TFlightIdentifier::dump");
    ProgTrace(TRACE5, "airline: %s", airline.c_str());
    ProgTrace(TRACE5, "flt_no: %d", flt_no);
    ProgTrace(TRACE5, "suffix: %c", suffix);
    ProgTrace(TRACE5, "date: %s", DateTimeToStr(date, "dd.mm.yyyy").c_str());

}

// на входе строка формата nn(aaa(nn))
TDateTime ParseDate(const string &buf)
{
    char sday[3];
    char smonth[4];
    char syear[3];
    char c;
    int res;
    int fmt = 1;
    while(fmt) {
        *sday = 0;
        *smonth = 0;
        *syear = 0;
        c = 0;
        switch(fmt) {
            case 1:
                fmt = 0;
                res = sscanf(buf.c_str(), "%2[0-9]%3[A-ZА-ЯЁ]%2[0-9]%c", sday, smonth, syear, &c);
                if(c != 0 or res != 3) fmt = 2;
                break;
            case 2:
                fmt = 0;
                res = sscanf(buf.c_str(), "%2[0-9]%3[A-ZА-ЯЁ]%c", sday, smonth, &c);
                if(c != 0 or res != 2) fmt = 3;
                break;
            case 3:
                fmt = 0;
                res = sscanf(buf.c_str(), "%2[0-9]%c", sday, &c);
                if(c != 0 or res != 1) 
                    throw ETlgError("Flight Identifier: wrong date format: %s", buf.c_str());
                break;
        }
    }
    if(strlen(sday) != 2)
        throw ETlgError("Flight Identifier: wrond day %s", sday);
    if(*smonth and strlen(smonth) != 3)
        throw ETlgError("Flight Identifier: wrond month %s", smonth);
    if(*syear and strlen(syear) != 2)
        throw ETlgError("Flight Identifier: wrond year %s", syear);

    TDateTime today=NowUTC();
    int year,mon,currday, day;
    StrToInt(sday, day);

    if(*syear != 0)
        StrToInt(syear, year);
    else
        DecodeDate(today,year,mon,currday);

    TDateTime date;
    try
    {
        if(*smonth)
            for(mon=1;mon<=12;mon++)
                if (strcmp(smonth,Months[mon-1].lat)==0||
                        strcmp(smonth,Months[mon-1].rus)==0) break;
        EncodeDate(year,mon,day,date);
    }
    catch(EConvertError)
    {
        throw ETlgError("Can't convert UTC date");
    };
    return date;
}

void TFlightIdentifier::parse(const char *val)
{
    string buf = val;
    size_t idx = buf.find('/');
    if(idx == string::npos)
        throw ETlgError("Flight Identifier: wrong format: %s", val);
    TSSMFltInfo flt_info;
    flt_info.parse(buf.substr(0, idx).c_str());
    airline = flt_info.airline;
    flt_no = flt_info.flt_no;
    suffix = *flt_info.suffix;
    buf.erase(0, idx + 1);
    date = ParseDate(buf);

}

void TASMFlightInfo::dump()
{
    flt.dump();
    if(not legs.empty()) {
        ProgTrace(TRACE5, "----legs----");
        for(vector<string>::iterator iv = legs.begin(); iv != legs.end(); iv++)
            ProgTrace(TRACE5, "%s", iv->c_str());
        ProgTrace(TRACE5, "----end of legs----");
    }
    if(not new_flt.airline.empty()) {
        ProgTrace(TRACE5, "----new flt----");
        new_flt.dump();
    }
    if(not dei_holder.empty())
        dei_holder.dump();
}

void TASMFlightInfo::parse(TTlgElement e, TActionIdentifier aid, const char *val)
{
    TTlgParser tlg;
    strcpy(lexh, val);
    char *p;
    p = tlg.GetLexeme(lexh);
    flt.parse(tlg.lex);
    switch(aid) {
        case aiCNL:
        case aiRIN:
        case aiADM:
        case aiCON:
        case aiEQT:
        case aiFLT:
        case aiRRT:
            {
                p = tlg.GetLexeme(p);
                TRouting routing(tcASM);
                routing.parse_leg_airps(tlg.lex);
                legs = routing.leg_airps;
                if(not legs.empty())
                    p = tlg.GetLexeme(p);
                break;
            }
        default:
            break;
    }
    if(aid == aiFLT)
        new_flt.parse(tlg.lex);
    else {
        dei_holder.parse(e, p, tlg);
        if(
                not dei_holder.dei1.empty() and
                (
                 aid != aiNEW and
                 aid != aiRPL and
                 aid != aiADM
                )
          )
            throw ETlgError("dei1 not applicable in %s", EncodeActionIdentifier(aid));
        if(
                (
                 not dei_holder.dei2.empty() or
                 not dei_holder.dei3.empty() or
                 not dei_holder.dei4.empty() or
                 not dei_holder.dei5.empty() or
                 not dei_holder.dei6.empty() or
                 not dei_holder.dei9.empty()
                ) and (
                    aid != aiNEW and
                    aid != aiRPL and
                    aid != aiADM and
                    aid != aiEQT
                    )
          )
            throw ETlgError("dei2-6,9 not applicable in %s", EncodeActionIdentifier(aid));
        if(not dei_holder.dei7.empty() and aid != aiADM)
            throw ETlgError("dei7 not applicable in %s", EncodeActionIdentifier(aid));
    }
    p = tlg.GetLexeme(p);
    if(p != NULL)
        throw ETlgError("Flight Information: unknown lexeme");
}

void ParseASMContent(TTlgPartInfo body, TSSMHeadingInfo& info, TASMContent& con, TMemoryManager &mem)
{
    con.Clear();
    TTlgParser tlg;
    char *line_p=body.p, *ph;
    int line=body.line;
    TTlgElement e = ActionIdentifier;
    TASMSubMessage *ssm_msg = NULL;
    try
    {
        do {
            tlg.GetToEOLLexeme(line_p);
            if(not *tlg.lex and e != RejectBody and e != Reject)
                throw ETlgError("blank line not allowed here %s", GetTlgElementName(e));
            if(not isLatUpperString(tlg.lex))
                throw ETlgError("isLatUpperString failed");
            switch(e) {
                case ActionIdentifier:
                    con.msgs.push_back(TASMSubMessage());
                    ssm_msg = &con.msgs.back();
                    ssm_msg->ainfo.parse(tlg.lex);
                    switch(ssm_msg->ainfo.id) {
                        case aiACK:
                            e = EndOfMessage;
                            break;
                        case aiNAC:
                            e = Reject;
                            break;
                        default:
                            e = FlightElement;
                            break;
                    }
                    break;
                case FlightElement:
                    try {
                        TASMFlightInfo flt;
                        flt.parse(e, ssm_msg->ainfo.id, tlg.lex);
                        ssm_msg->flts.push_back(flt);
                        break;
                    } catch(ETlgError &E) {
                        if(ssm_msg->flts.empty())
                            throw ETlgError("Flight Info not found");
                        switch(ssm_msg->ainfo.id) {
                            case aiNEW:
                            case aiRPL:
                            case aiCON:
                            case aiEQT:
                            case aiRRT:
                                e = Equipment;
                                break;
                            case aiCNL:
                            case aiRIN:
                            case aiFLT:
                                e = SubSI;
                                break;
                            case aiADM:
                                e = Segment;
                                break;
                            case aiTIM:
                                e = Routing;
                                break;
                            default:
                                throw ETlgError("%s not allowed in FlightElement", EncodeActionIdentifier(ssm_msg->ainfo.id));
                        }
                        continue;
                    }
                case Equipment:
                case Routing:
                    {
                        try {
                            TEqtRouting eqt_routing(tcASM);
                            eqt_routing.eqt.parse(Equipment, tlg.lex);
                            ssm_msg->eqt_routing.push_back(eqt_routing);
                        } catch (ETlgError &E) {
                            try {
                                TRouting routing(tcASM);
                                routing.parse(Routing, ssm_msg->ainfo.id, tlg.lex);
                                if(ssm_msg->eqt_routing.empty())
                                    ssm_msg->eqt_routing.push_back(TEqtRouting(tcASM));
                                ssm_msg->eqt_routing.back().routing.push_back(routing);
                            } catch(ETlgError &E) {
                                e = Segment;
                                continue;
                            }
                        }
                        break;
                    }
                case Segment:
                    {
                        try {
                            TSegment seg;
                            seg.parse(tlg.lex);
                            ssm_msg->segs.push_back(seg);
                            e = Segment;
                            break;
                        } catch(ETlgError &E) {
                            e = SubSI;
                        }
                    }
                case SubSI:
                    {
                        string buf = tlg.lex;
                        if(buf.substr(0, 2) == SI_INDICATOR) {
                            ssm_msg->si.push_back(buf.substr(3));
                            e = SubSIMore;
                            break;
                        } else {
                            e = SubSeparator;
                            continue;
                        }
                    }
                case SubSIMore:
                    if(SUB_SEPARATOR == tlg.lex) {
                        e = SubSeparator;
                        continue;
                    } else {
                        if(ssm_msg->si.size() == 3)
                            throw ETlgError("wrong format, sub msg SI must contain max of 3 lines");
                        ssm_msg->si.push_back(tlg.lex);
                        e = SubSIMore;
                        break;
                    }
                case SubSeparator:
                    {
                        if(SUB_SEPARATOR == tlg.lex)
                            e = SI;
                        else
                            throw ETlgError("Unknown lexeme, expected sub msg separator");
                        break;
                    }
                case SI:
                    {
                        string buf = tlg.lex;
                        if(buf.substr(0, 2) == SI_INDICATOR) {
                            con.si.push_back(buf.substr(3));
                            e = SIMore;
                            break;
                        } else {
                            e = ActionIdentifier;
                            continue;
                        }
                    }
                case SIMore:
                    {
                        if(con.si.size() == 3)
                            throw ETlgError("wrong format, whole msg SI must contain max of 3 lines");
                        con.si.push_back(tlg.lex);
                        e = SIMore;
                        break;
                    }
                case Reject:
                    {
                        if(*tlg.lex) throw ETlgError("wrong format. blank line expected");
                        e = RejectBody;
                        break;
                    }
                case RejectBody:
                    {
                        if(*tlg.lex) {
                            string buf = tlg.lex;
                            if(buf.size() < 5) throw ETlgError("reject info line too short");
                            TError err;
                            StrToInt(buf.substr(0, 3).c_str(), err.line);
                            if(buf[3] != ' ') throw ETlgError("reject info line wrong format");
                            err.err = buf.substr(4);
                            ssm_msg->reject_info.errors.push_back(err);
                        } else {
                            if(ssm_msg->reject_info.errors.empty())
                                throw ETlgError("wrong format. Reject reason line expected");
                            e = RepeatOfRejected;
                        }
                        break;
                    }
                case RepeatOfRejected:
                    {
                        if(not *tlg.lex)
                            throw ETlgError("wrong format. Rejected msg line expected");
                        ssm_msg->reject_info.rejected_msg.push_back(tlg.lex);
                        break;
                    }
                case EndOfMessage:
                    throw ETlgError("unknown lexeme");
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
            throw ETlgError("ASM: %s\n>>>>>LINE %d: %s",E.what(),line,tlg.lex);
        else
            throw ETlgError("ASM: %s\n>>>>>LINE %d",E.what(),line);
    };
    con.dump();
    return;
}

void SaveASMContent(int tlg_id, TSSMHeadingInfo& info, TASMContent& con)
{
}

bool TRoute::route_exists()
{
    bool result;
    vector<TPeriods>::iterator iv = periods.begin();
    for(; iv != periods.end(); iv++) {
        vector<TEqtRouting>::iterator i_eqt = iv->eqt_routing.begin();
        for(; i_eqt != iv->eqt_routing.end(); i_eqt++)
            if(not i_eqt->routing.empty()) break;
        if(i_eqt != iv->eqt_routing.end()) break;
    }
    result = iv != periods.end();
    return result or not routing.empty();
}

void TSchedules::add(const TFlightInformation &c)
{
    if(empty()) push_back(TSchedule());
    if(not curr_scd().routes.empty())
        push_back(TSchedule());
    curr_scd().flights.push_back(c);
}

void TSchedules::add(const TPeriodFrequency &c)
{
    if(curr_scd().flights.empty())
        throw ETlgError("flight not found");
    if(curr_scd().routes.empty() or curr_route().route_exists())
        curr_scd().routes.push_back(TRoute());
    // Если в текущем блоке периодов уже есть equipment, то добавляем новый блок
    if(curr_route().periods.empty() or not curr_periods().eqt_routing.empty())
        curr_route().periods.push_back(TPeriods());
    curr_periods().period_frequency.push_back(c);
}

void TSchedules::add(const TSegment &c)
{
    if(not curr_route().route_exists())
        throw ETlgError("segment not allowed: route not exists");
    curr_route().segments.push_back(c);
}

void TSchedules::add(const TRouting &c)
{
    vector<TEqtRouting> &eqt_routing = curr_periods().eqt_routing;
    /*
    if(eqt_routing.empty()) // К этому моменту список экуипментов не может быт пустым
        throw ETlgError("no equipment found for route");
        Оказывается, в TIM может
        */
    if(eqt_routing.empty())
        eqt_routing.push_back(TEqtRouting(tcSSM));

    // Если в последнем встреченном экуипменте не было маршрута (PEPER)
    // или общий маршрут уже не пустой, относим маршрут к общему, иначе в текущий экуипмент
    if(
            not curr_route().routing.empty() or 
            curr_route().periods.size() > 1 and curr_route().periods[0].eqt_routing.back().routing.empty()
      )
        curr_route().routing.push_back(c);
    else
        eqt_routing.back().routing.push_back(c);
}

void TSchedules::add(const TEquipment &c)
{
    // Если есть общий маршрут, то equipment быть уже не может
    if(not curr_route().routing.empty())
        throw ETlgError("Equipment not allowed here, common routing already exists");
    // Если внутри блока периодов несколько equipment, то все они должны быть с маршрутом
    vector<TEqtRouting> &eqt = curr_periods().eqt_routing;
    if(eqt.size() > 0) {
        vector<TEqtRouting>::iterator i = eqt.begin();
        for(; i != eqt.end(); i++)
            if(i->routing.empty()) break;
        if(i != eqt.end()) throw ETlgError("empty route for equipment found");
    }
    eqt.push_back(TEqtRouting(tcSSM));
    eqt.back().eqt = c;
}

TSchedule &TSchedules::curr_scd()
{
    if(empty())
        throw ETlgError("curr_scd not found");
    return back();
};

TRoute &TSchedules::curr_route()
{
    if(empty() or curr_scd().routes.empty())
        throw ETlgError("curr_route not found");
    return curr_scd().routes.back();
}

TPeriods &TSchedules::curr_periods()
{
    if(empty() or curr_scd().routes.empty() or curr_route().periods.empty())
        throw ETlgError("curr_periods not found");
    return curr_route().periods.back();
}

TTlgPartInfo ParseSSMHeading(TTlgPartInfo heading, TSSMHeadingInfo &info)
{
    int line,res;
    char c, *p,*ph,*line_p;
    TTlgParser tlg;
    TTlgElement e;
    TTlgPartInfo next;

    try
    {
        line_p=heading.p;
        line=heading.line-1;
        e=TimeModeElement;
        do
        {
            line++;
            if ((p=tlg.GetToEOLLexeme(line_p))==NULL) continue;
            switch (e)
            {
                case TimeModeElement:
                    {
                        if(strcmp(tlg.lex, "LT") == 0) {
                            info.time_mode = tmLT;
                        } else if(strcmp(tlg.lex, "UTC") == 0) {
                            info.time_mode = tmUTC;
                        }
                        e = MessageSequenceReference;
                        if(info.time_mode == tmUnknown)
                            info.time_mode = tmUTC; // по умолчанию, согласно стандарту
                        else break;
                    }
                case MessageSequenceReference:
                    {
                        strcpy(lexh, tlg.lex);
                        ph = tlg.GetSlashedLexeme(lexh);
                        bool success = true;
                        char day[3], grp[8], num[4];
                        *day = 0;
                        *grp = 0;
                        *num = 0;
                        c = 0;
                        res = sscanf(tlg.lex, "%2[0-9]%3[A-ZА-ЯЁ]%7[0-9]%c%3[0-9]%c",
                                day, info.msr.month, grp, &info.msr.type, num, &c);
                        if(
                                res != 5 or
                                c != 0 or
                                strlen(day) != 2 or
                                strlen(info.msr.month) != 3 or
                                strlen(grp) != 7 or //!!! non standard must be 5
                                info.msr.type != 'C' and info.msr.type != 'E' or
                                strlen(num) != 3
                          )
                            success = false;
                        StrToInt(day, info.msr.day);
                        StrToInt(grp, info.msr.grp);
                        StrToInt(num, info.msr.num);
                        ph = tlg.GetToEOLLexeme(ph);
                        if(strlen(tlg.lex) > 35)
                            throw ETlgError("Creator Reference too long: %s", tlg.lex);
                        info.msr.creator = tlg.lex;

                        if(success) {
                            next.p=tlg.NextLine(line_p);
                            next.line=line+1;
                        } else {
                            next.p=line_p;
                            next.line=line;
                        }
                        return next;
                    }
                default:;
            };
        }
        while ((line_p=tlg.NextLine(line_p))!=NULL);
    }
    catch(ETlgError E)
    {
        //вывести ошибку+номер строки
        throw ETlgError("SSM, Line %d: %s",line,E.what());
    };
    next.p=line_p;
    next.line=line;
    return next;
};

void TSSMHeadingInfo::dump()
{
    ProgTrace(TRACE5, "-----TSSMHeadingInfo::dump-------");
    switch(time_mode) {
        case tmLT:
            ProgTrace(TRACE5, "time_mode: LT");
            break;
        case tmUTC:
            ProgTrace(TRACE5, "time_mode: UTC");
            break;
        case tmUnknown:
            ProgTrace(TRACE5, "time_mode: Unknown");
            break;
    }
    ProgTrace(TRACE5, "msr.day: %d", msr.day);
    ProgTrace(TRACE5, "msr.month: %s", msr.month);
    ProgTrace(TRACE5, "msr.grp: %d", msr.grp);
    ProgTrace(TRACE5, "msr.type: %c", msr.type);
    ProgTrace(TRACE5, "msr.num: %d", msr.num);
    ProgTrace(TRACE5, "msr.creator: '%s'", msr.creator.c_str());
    ProgTrace(TRACE5, "---------------------------------");
}

}
