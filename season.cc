#include "season.h"
#include "xml_unit.h"
#include "exceptions.h"
#define NICKNAME "DENIS"
#include "test.h"
#include "oralib.h"
#include "cache.h"
#include "misc.h"
#include "stl_utils.h"

using namespace EXCEPTIONS;
using namespace std;
using namespace BASIC;
using namespace ASTRA;

class TDests {
    private:
        string FBC;  // тип ВС
        int FF; // кол-во мест в классе П
        int FC; // кол-во мест в классе Б
        int FY; // кол-во мест в классе Э
    public:
        string FTripType; // тип рейса
        string FCod; // код п.п.
        string FLitera;
        string FSuffix;
        string Litera;
        string FUnitrip; // объединенный рейс
        void SetCod(string value);
        string GetCod();

        void SetBC(string value);
        string GetBC();

        void SetTripType(string val);
        string GetTripType();

        int GetF();
        void SetF(int val);
        int GetC();
        void SetC(int val);
        int GetY();
        void SetY(int val);

        string FCity;
        string FCompany; // компания
        int FTrip; // номер рейса
        bool Modify;
        TDateTime FTakeoff; // Время вылета
        TDateTime FLand;  // Время посадки
        int Pr_Cancel; // Признак отмены плеча
        TDests();
        int delta_out();
};

int TDests::delta_out()
{
    int result;
    if(FTakeoff == NoExists)
        result = 0;
    else
        result = (int)FTakeoff;
    return result;
}

int TDests::GetF()
{
    return FF;
}

void TDests::SetF(int val)
{
    FF = val;    
}

int TDests::GetC()
{
    return FC;
}

void TDests::SetC(int val)
{
    FC = val;    
}

int TDests::GetY()
{
    return FY;
}

void TDests::SetY(int val)
{
    FY = val;    
}

void TDests::SetTripType(string val)
{
    FTripType = val;
}

string TDests::GetTripType()
{
    return FTripType;
}

string TDests::GetBC()
{
    return FBC;
}

void TDests::SetBC(string Value)
{
    FBC = Value;
}

void TDests::SetCod(string Value)
{
    FCod = Value;
}

string TDests::GetCod()
{
    return FCod;
}

TDests::TDests()
{
    FTrip = NoExists;
    FLand = NoExists;
    FTakeoff = NoExists;
    FF = 0;
    FC = 0;
    FY = -1;
    FTripType = 'п';
    Pr_Cancel = 0;
}

class TDestList: public vector<TDests> {
    private:
        bool Pr_Cancel;
    public:
        int CountLock;
        TDestList(): Pr_Cancel(false), CountLock(0) {};
};

class TSubRange {
    private:
        bool FNewMove_id;
        TDestList FDestList;
        string FDays; // дни выполнения
    public:
        TDateTime FFirst; // первый день выполнения
        string GetFirst();

        string GetFDays() { return FDays; };
        void SetFDays(string val) { FDays = val; };
        string GetDays();
        void SetDays();

        TDestList *GetDestList() { return &FDestList; };
        void SetDestList(TDestList *value) { FDestList = *value; FDestList.CountLock++; };

        bool Visible;
        bool Filtered;
        bool InWork;
        int Local_id;
        int Move_id;
        bool Modify;
        TDateTime FLast; // последний день выполнения
        string FTlg; // текст телеграммы
        string FReference; // полное описание телеграммы
        bool Cancel;
        TSubRange(int FMove_id, bool FNew);
};

string TSubRange::GetFirst()
{
    string result;
    if(FFirst != NoExists)
        result = DateTimeToStr(FFirst, "DD.MM");
    return result;
}

string GetPointDays(string Days)
{
    string result;
    if(!(TrimString(Days).empty() || Days == AllDays)) {
        result = NoDays;
        for(int i = 0; i < 7; i++)
            if(Days.find(AllDays[i]) != string::npos)
                result[i] = AllDays[i];
        if(result == AllDays) result = "";
    }
    return result;
}

string TSubRange::GetDays()
{
    return GetPointDays(FDays);
}

TSubRange::TSubRange(int FMove_id, bool FNew = false)
{
    Filtered = true;
    Cancel = false;
    FNewMove_id = FNew;
    if(FNew) {
        Local_id = FMove_id;
        Move_id = NoExists;
    } else {
        Local_id = FMove_id;
        Move_id = FMove_id;
    }
    InWork = true;
    FFirst = NoExists;
    FLast = NoExists;
    FDays = "1234567";
    Visible = true;
}

class TSubRangeList;

class TViewTrip {
    public:
        TSubRangeList *SubRangeList;
};

typedef vector<TViewTrip> TViewTrips;

class TSubRangeList: public vector<TSubRange> {
    public:
        int Trip_id;
        TSubRangeList(): Trip_id(NoExists) {};
        void FilterRanges(TViewTrips &ViewTrips);
};

class TRange {
    public:
    bool Summer;
    BASIC::TDateTime First, Last;
    std::string Days;
    void Init(xmlNodePtr rangeNode);
};

void TRange::Init(xmlNodePtr rangeNode)
{
    xmlNodePtr dateNode = NodeAsNode("First", rangeNode);
    if(NodeIsNULL(dateNode))
        First = NoExists;
    else
        First = NodeAsDateTime(dateNode);
    dateNode = NodeAsNode("Last", rangeNode);
    if(NodeIsNULL(dateNode))
        Last = NoExists;
    else
        Last = NodeAsDateTime(dateNode);
    Summer = NodeAsInteger("Summer", rangeNode) == 1;
    Days = NodeAsInteger("Days", rangeNode);
}

class TFilter {
    public:
        std::string Caption;
        int ItemIndex;
        TRange Range;
        BASIC::TDateTime FirstTime, LastTime;
        std::string Company, Dest, Airp, TripType;
        void Init(xmlNodePtr node);
};

class TSeason: public vector<TRange> {
    public:
        void Init(xmlNodePtr seasonNode);
};

void TSeason::Init(xmlNodePtr seasonNode)
{
    clear();
    xmlNodePtr rangeNode = seasonNode->children;
    while(rangeNode) {
        TRange Range;
        tst();
        Range.Init(rangeNode);
        tst();
        this->push_back(Range);
        rangeNode = rangeNode->next;
    }
}

void TFilter::Init(xmlNodePtr node)
{
    xmlNodePtr curNode = NodeAsNode("Filter", node);
    Caption = NodeAsString("Caption", curNode);
    ItemIndex = NodeAsInteger("ItemIndex", curNode);
    Company = NodeAsString("Company", curNode);
    Dest = NodeAsString("Dest", curNode);
    Airp = NodeAsString("Airp", curNode);
    TripType = NodeAsString("TripType", curNode);
    xmlNodePtr dateNode = NodeAsNode("FirstTime", curNode);
    if(NodeIsNULL(dateNode))
        FirstTime = NoExists;
    else
        FirstTime = NodeAsDateTime(dateNode);
    dateNode = NodeAsNode("LastTime", curNode);
    if(NodeIsNULL(dateNode))
        LastTime = NoExists;
    else
        LastTime = NodeAsDateTime(dateNode);

    curNode = NodeAsNode("Range", curNode);
    Range.Init(curNode);
}

TSeason Season;
TFilter Filter;

class TRangeList: public vector<TSubRangeList> {
    public:
        TViewTrips ViewTrips;
        void Read();
        void CreateViewer(TSubRangeList *ASubRangeList);
};

string GetCommonDay(string a, string b)
{
    string result = NoDays;
    for(int i = 0; i < 7; i++) {
        if(a[i] == b[i])
            result[i] = a[i];
        else
            result[i] = '.';
    }
    return result;
}

bool ARange_In_BRange(TRange &ARange, TRange &BRange)
{
    bool result;
    if(ARange.First >= BRange.First && ARange.First <= BRange.Last)
        result = true;
    else
        if(ARange.Last >= BRange.First && ARange.Last <= BRange.Last)
            result = true;
        else
            if(BRange.First >= ARange.First && BRange.Last <= ARange.Last)
                result = true;
            else
                result = false;
    return result && GetCommonDay(ARange.Days, BRange.Days) != NoDays;
}

string GetWOPointDays(string Days)
{
//    return Days.erase(remove(Days.begin(), Days.end(), '.'), Days.end());
    return "";
}

string GetNextDays(TSubRange &S)
{
    string result;
    TDestList &DestList = *S.GetDestList();
    TDestList::iterator iv;
    for(iv = DestList.begin(); iv != DestList.end(); iv++)
        if(iv->FCod == TReqInfo::Instance()->desk.airp) break;
    if(iv != DestList.end() && iv->delta_out() > 0) {
        result = " (";
        string FDays = S.GetFDays();
        for(string::iterator iv = FDays.begin(); iv != FDays.end(); iv++)
            if(*iv != '.') {
                int Day = StrToInt(*iv);
            }
    }
}

void PutNextExec(string &Str, TSubRange &S)
{
    if(S.GetDays().size())
        Str += "; " + GetWOPointDays(S.GetDays()) + GetNextDays(S) + " ";
    else
        Str += " ";
//    if(S.FFirst != S.FLast)
//        Str += S.First + "-" + S.Last;
//    else
//        Str += S.First;
//    if(S.Tlg.size()) Str += " " + S.Tlg;
}

void TSubRangeList::FilterRanges(TViewTrips &ViewTrips)
{
    string Exec, NoExec;
    for(TSubRangeList::iterator iv = this->begin(); iv != this->end(); iv++) {
        TSubRange &SubRange = *iv;
        SubRange.Filtered = false;
        bool DataKey = false;
        TRange ARange;
        ARange.Days = SubRange.GetFDays();
        bool CompKey = Filter.Company.empty();
        bool DestKey = Filter.Dest.empty();
        bool AirpKey = Filter.Airp.empty();
        bool TripTypeKey = Filter.TripType.empty();
        bool TimeKey = Filter.FirstTime == NoExists;
        TDestList &DestList = *SubRange.GetDestList();
        for(TDestList::iterator jv = DestList.begin(); jv != DestList.end(); jv++) {
            TDests &Dest = *jv;
            DestKey = DestKey || Dest.FCity == Filter.Dest;
            AirpKey = AirpKey || Dest.FCod == Filter.Airp;
            CompKey = CompKey || Dest.FCompany == Filter.Company;
            TripTypeKey = TripTypeKey || Dest.FTripType == Filter.TripType;
            if(Dest.FCod == TReqInfo::Instance()->desk.airp) {
                TimeKey =
                    TimeKey ||
                    Dest.FLand >= Filter.FirstTime && Dest.FLand <= Filter.LastTime ||
                    Dest.FTakeoff >= Filter.FirstTime && Dest.FTakeoff <= Filter.LastTime;
                if(Dest.FLand > NoExists) {
                    ARange.First = SubRange.FFirst + Dest.FLand;
                    ARange.Last = SubRange.FLast + Dest.FLand;
                    if(!DataKey)
                        DataKey = ARange_In_BRange(ARange, Filter.Range);
                }
            }
            SubRange.Filtered = DataKey && DestKey && AirpKey && CompKey && TripTypeKey && TimeKey;
            if(SubRange.Filtered) {
                if(SubRange.Cancel)
                    PutNextExec(NoExec, SubRange);
                else
                    PutNextExec(Exec, SubRange);
                break;
            }
        }
    }
}

void TRangeList::CreateViewer(TSubRangeList *ASubRangeList = NULL)
{
    {
        TViewTrips::iterator iv = ViewTrips.begin();
        while(1) {
            if( 
                    ASubRangeList == NULL ||
                    (ASubRangeList != NULL && iv->SubRangeList->Trip_id == ASubRangeList->Trip_id)
              )
                iv = ViewTrips.erase(iv);
            else
                iv++;
            if(iv == ViewTrips.end()) break;
        }
    }
    if(ASubRangeList != NULL)
        ; //ASubRangeList->FilterRanges(ViewTrips)
    else {
        for(TRangeList::iterator iv = this->begin(); iv != this->end(); iv++)
            iv->FilterRanges(ViewTrips);
    }
}

void TRangeList::Read()
{
    TQuery *Qry = OraSession.CreateQuery();
    try {
        Qry->SQLText =
            "SELECT a.trip_id,a.move_id,a.num as num,b.first_day,b.last_day,b.days,b.pr_cancel as cancel, "
            "b.tlg,b.reference,r.num as rnum,r.cod,airps.city,r.pr_cancel,r.land+r.delta_in as land, "
            "r.company,r.trip,r.bc,r.takeoff+r.delta_out as takeoff,r.litera,r.triptype,r.f,r.c,r.y,r.unitrip, "
            "r.suffix  "
            "FROM routes r, sched_days b, airps, "
            "(SELECT trip_id,move_id as move_id,MIN(num) as num  "
            "FROM sched_days GROUP BY trip_id, move_id) a  "
            "WHERE a.trip_id=b.trip_id AND r.move_id=b.move_id AND  "
            "a.move_id=b.move_id AND r.cod=airps.cod AND a.num=b.num  "
            "UNION  "
            "SELECT a.trip_id,a.move_id,a.num,a.first_day,a.last_day,a.days,a.pr_cancel, "
            "a.tlg,a.reference,0,NULL,NULL,0,TO_DATE(NULL),NULL,0,NULL,TO_DATE(NULL), "
            "NULL,' ',0,0,0,NULL,NULL  "
            "FROM sched_days a, "
            "(SELECT trip_id,move_id,MIN(num) as num FROM sched_days  "
            "GROUP BY trip_id,move_id) b  "
            "WHERE a.trip_id=b.trip_id AND a.move_id=b.move_id AND a.num!=b.num  "
            "ORDER BY trip_id, move_id,num";
        Qry->Execute();

        if(!Qry->Eof) {
            int idx_trip_id     = Qry->FieldIndex("trip_id");
            int idx_move_id     = Qry->FieldIndex("move_id");
            int idx_num         = Qry->FieldIndex("num");
            int idx_first_day   = Qry->FieldIndex("first_day");
            int idx_last_day    = Qry->FieldIndex("last_day");
            int idx_days        = Qry->FieldIndex("days");
            int idx_cancel      = Qry->FieldIndex("cancel");
            int idx_tlg         = Qry->FieldIndex("tlg");
            int idx_reference   = Qry->FieldIndex("reference");
            int idx_rnum        = Qry->FieldIndex("rnum");
            int idx_cod         = Qry->FieldIndex("cod");
            int idx_city        = Qry->FieldIndex("city");
            int idx_pr_cancel   = Qry->FieldIndex("pr_cancel");
            int idx_land        = Qry->FieldIndex("land");
            int idx_company     = Qry->FieldIndex("company");
            int idx_trip        = Qry->FieldIndex("trip");
            int idx_bc          = Qry->FieldIndex("bc");
            int idx_takeoff     = Qry->FieldIndex("takeoff");
            int idx_litera      = Qry->FieldIndex("litera");
            int idx_triptype    = Qry->FieldIndex("triptype");
            int idx_f           = Qry->FieldIndex("f");
            int idx_c           = Qry->FieldIndex("c");
            int idx_y           = Qry->FieldIndex("y");
            int idx_unitrip     = Qry->FieldIndex("unitrip");
            int idx_suffix      = Qry->FieldIndex("suffix");

            while(!Qry->Eof) {
                int Trip_id = Qry->FieldAsInteger(idx_trip_id);
                TSubRangeList SubRangeList;
                SubRangeList.Trip_id = Trip_id;
                while(!Qry->Eof && Trip_id == Qry->FieldAsInteger(idx_trip_id)) {
                    int Move_id = Qry->FieldAsInteger(idx_move_id);
                    TSubRange SubRange(Move_id);
                    SubRange.FFirst = Qry->FieldAsDateTime(idx_first_day);
                    SubRange.FLast = Qry->FieldAsDateTime(idx_last_day);
                    SubRange.SetFDays(Qry->FieldAsString(idx_days));
                    SubRange.FTlg = Qry->FieldAsString(idx_tlg);
                    SubRange.FReference = Qry->FieldAsString(idx_reference);
                    SubRange.Cancel = Qry->FieldAsInteger(idx_cancel) > 0;
                    if(Qry->FieldIsNULL(idx_cod)) {
                        SubRange.SetDestList(SubRangeList.back().GetDestList());
                        Qry->Next();
                    } else {
                        while(
                                !Qry->Eof &&
                                Trip_id == Qry->FieldAsInteger(idx_trip_id) &&
                                Move_id == Qry->FieldAsInteger(idx_move_id) &&
                                !Qry->FieldIsNULL(idx_cod)
                             ) {
                            TDests Dest;
                            Dest.SetCod(Qry->FieldAsString( idx_cod ));
                            Dest.FCity = Qry->FieldAsString( idx_city );
                            Dest.SetBC(Qry->FieldAsString( idx_bc ));
                            if(Qry->FieldIsNULL( idx_land ))
                                Dest.FLand = NoExists;
                            else
                                Dest.FLand = Qry->FieldAsDateTime(idx_land);
                            if(Qry->FieldIsNULL(idx_takeoff))
                                Dest.FTakeoff = NoExists;
                            else
                                Dest.FTakeoff = Qry->FieldAsDateTime(idx_takeoff);
                            Dest.SetTripType(Qry->FieldAsString( idx_triptype ));
                            Dest.SetF(Qry->FieldAsInteger(idx_f ));
                            Dest.SetC(Qry->FieldAsInteger(idx_c ));
                            Dest.SetY(Qry->FieldAsInteger(idx_y ));
                            Dest.FCompany = Qry->FieldAsString(idx_company);
                            if(Qry->FieldIsNULL(idx_trip))
                                Dest.FTrip = NoExists;
                            else
                                Dest.FTrip = Qry->FieldAsInteger(idx_trip);
                            Dest.FUnitrip = Qry->FieldAsString(idx_unitrip);
                            Dest.Pr_Cancel = Qry->FieldAsInteger(idx_pr_cancel);
                            Dest.Litera = Qry->FieldAsString( idx_litera );
                            Dest.FSuffix = Qry->FieldAsString(idx_suffix);
                            SubRange.GetDestList()->push_back( Dest );
                            Qry->Next();
                        }
                    }
                    SubRangeList.push_back(SubRange);
                }
                push_back(SubRangeList);
            }
        }
    } catch(...) {
        OraSession.DeleteQuery(*Qry);
        throw;
    }
    OraSession.DeleteQuery(*Qry);
}

void SeasonInterface::Read(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    string buf = "123456";
    throw UserException("%d", StrToInt(buf));
    Season.Init(NodeAsNode("Season", reqNode));
    Filter.Init(reqNode);

    TRangeList RangeList;
    TPerfTimer tm;
    RangeList.Read();
    ProgTrace(TRACE5, "Read(): %d", tm.Print());
//    RangeList.CreateViewer();
    ProgTrace(TRACE5, "RangeList.size(): %d", RangeList.size());
}

void SeasonInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

}
