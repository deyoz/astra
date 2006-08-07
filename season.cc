#include "season.h"
#include "xml_unit.h"
#include "exceptions.h"
#define NICKNAME "DENIS"
#include "test.h"
#include "oralib.h"
#include "cache.h"
#include "misc.h"

using namespace EXCEPTIONS;
using namespace std;
using namespace BASIC;
using namespace ASTRA;

class TDests {
    private:
        string FCod; // код п.п.
        string FBC;  // тип ВС
        string FTripType; // тип рейса
        int FF; // кол-во мест в классе П
        int FC; // кол-во мест в классе Б
        int FY; // кол-во мест в классе Э
    public:
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
};

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
        int CountLock;
    public:
        TDestList(): Pr_Cancel(false), CountLock(0) {};
};

class TSubRange {
    private:
        bool FNewMove_id;
        TDestList FDestList;
    public:
        bool Visible;
        bool Filtered;
        string FDays; // дни выполнения
        bool InWork;
        int Local_id;
        int Move_id;
        bool Modify;
        TDateTime FFirst; // первый день выполнения
        TDateTime FLast; // последний день выполнения
        string FTlg; // текст телеграммы
        string FReference; // полное описание телеграммы
        bool Cancel;
        TSubRange(int FMove_id, bool FNew);
};

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

class TSubRangeList: public vector<TSubRange> {
    public:
        int Trip_id;
        TSubRangeList(): Trip_id(NoExists) {};
};

class TRangeList: public vector<TSubRangeList> {
    public:
        void Read();
};

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
                            SubRange.FDays = Qry->FieldAsString(idx_days);
                            SubRange.FTlg = Qry->FieldAsString(idx_tlg);
                            SubRange.FReference = Qry->FieldAsString(idx_reference);
                            SubRange.Cancel = Qry->FieldAsInteger(idx_cancel) > 0;
                            if(Qry->FieldIsNULL(idx_cod)) {
                                //!!! SubRange.DestList
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
                                    //                            Subrange.DestList.push_back( Dest );
                                    Qry->Next();
                                }
                            }
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
    TRangeList RangeList;
    RangeList.Read();
    TPerfTimer tm;
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
        ProgTrace(TRACE5, "Executing qry %d", tm.Print());

        if(!Qry->Eof) {
            xmlNodePtr dataNode = NewTextChild(resNode, "data");
            xmlNodePtr rangeListNode = NewTextChild(dataNode, "rangeList");

            tm.Init();
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
            ProgTrace(TRACE5, "Building idx %d", tm.Print());

            tm.Init();

            int i = 0;
            while(!Qry->Eof) {
                xmlNodePtr rangeNode = NewTextChild(rangeListNode, "range");

                /*
                for(int i = 0; i < Qry->FieldsCount(); i++)
                    NewTextChild(rangeNode, Qry->FieldName(i), Qry->FieldAsString(i));
                */

                NewTextChild(rangeNode, "trip_id",      Qry->FieldAsString(idx_trip_id));
                NewTextChild(rangeNode, "move_id",      Qry->FieldAsString(idx_move_id));
                NewTextChild(rangeNode, "num",          Qry->FieldAsString(idx_num));
                NewTextChild(rangeNode, "first_day",    Qry->FieldAsString(idx_first_day));
                NewTextChild(rangeNode, "last_day",     Qry->FieldAsString(idx_last_day));
                NewTextChild(rangeNode, "days",         Qry->FieldAsString(idx_days));
                NewTextChild(rangeNode, "cancel",       Qry->FieldAsString(idx_cancel));
                NewTextChild(rangeNode, "tlg",          Qry->FieldAsString(idx_tlg));
                NewTextChild(rangeNode, "reference",    Qry->FieldAsString(idx_reference));
                NewTextChild(rangeNode, "rnum",         Qry->FieldAsString(idx_rnum));
                NewTextChild(rangeNode, "cod",          Qry->FieldAsString(idx_cod));
                NewTextChild(rangeNode, "city",         Qry->FieldAsString(idx_city));
                NewTextChild(rangeNode, "pr_cancel",    Qry->FieldAsString(idx_pr_cancel));
                NewTextChild(rangeNode, "land",         Qry->FieldAsString(idx_land));
                NewTextChild(rangeNode, "company",      Qry->FieldAsString(idx_company));
                NewTextChild(rangeNode, "trip",         Qry->FieldAsString(idx_trip));
                NewTextChild(rangeNode, "bc",           Qry->FieldAsString(idx_bc));
                NewTextChild(rangeNode, "takeoff",      Qry->FieldAsString(idx_takeoff));
                NewTextChild(rangeNode, "litera",       Qry->FieldAsString(idx_litera));
                NewTextChild(rangeNode, "triptype",     Qry->FieldAsString(idx_triptype));
                NewTextChild(rangeNode, "f",            Qry->FieldAsString(idx_f));
                NewTextChild(rangeNode, "c",            Qry->FieldAsString(idx_c));
                NewTextChild(rangeNode, "y",            Qry->FieldAsString(idx_y));
                NewTextChild(rangeNode, "unitrip",      Qry->FieldAsString(idx_unitrip));
                NewTextChild(rangeNode, "suffix",       Qry->FieldAsString(idx_suffix));  

                Qry->Next();
                i++;
            }
            ProgTrace(TRACE5, "Building resDoc %d", tm.Print());
            ProgTrace(TRACE5, "rows processed: %d", i);
        }

    } catch(...) {
        OraSession.DeleteQuery(*Qry);
        throw;
    }
    OraSession.DeleteQuery(*Qry);
}

void SeasonInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

}
