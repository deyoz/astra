// #include <numeric>
#include "astra_ticket.h"
#include "astra_tick_read_edi.h"
#include "etick/tick_data.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include "test.h"
#include "etick/edi_cast.h"

namespace Ticketing{
namespace TickReader{

    using namespace std;
    using namespace edilib;
    using namespace boost::gregorian;
    using namespace boost::posix_time;

    struct CoupCounter{
        CoupCounter(){}
        int operator () (int init, const Ticket& t)
        {
            ProgTrace(TRACE3,"%d coupon[s] in the ticket #%s/%d",
                      t.getCoupon().size(), t.ticknum().c_str(), t.actCode());
            return (init + ((t.actCode() == TickStatAction::oldtick)?0:t.getCoupon().size()));
        }
    };

//Проверка на эквивалентность кол-ва переданных элементов
//реально пришедшим
    void CheckNumberOfUnits(EDI_REAL_MES_STRUCT *pMes, const std::list<Ticket> lTick)
    {
        PushEdiPointG(pMes);
        SetEdiPointToSegmentG(pMes, "EQN",0, "MISS_TOTAL_FLIGHTS");
        unsigned Num=GetNumComposite(pMes, "C523", "MISS_TOTAL_FLIGHTS");
        if(Num>2){
            ProgError(STDLOG,"Too many units in an EQN");
            throw Exception("Too many units in an EQN");
        }
        PushEdiPointG(pMes);
        for(unsigned i=0;i<Num;i++){
            SetEdiPointToCompositeG(pMes, "C523",i,"PROG_ERR");
            unsigned eNum=GetDBNumCast<unsigned>(EdiDigitCast<unsigned>(), pMes, 6350);
            if(eNum <= 0){
                throw Exception("Missing number of units");
            }
            const char *code = GetDBNum(pMes, 6353,0, "UNKN_UNIT_CODE");
            if(!strcmp(code, "TF")){
            // Кол-во сегментов
                unsigned count = std::accumulate(lTick.begin(), lTick.end(), 0, CoupCounter());
                if(count != eNum){

                    ProgError(STDLOG, "Total number of flight segments in \"EQN\""
                                " %d different from %d",
                                eNum, count);
                    throw Exception("Wrong total number of flight segments");
                }
            } else if(!strcmp(code, "TD")){
            // Кол-во билетов
                unsigned numoftick = lTick.size();
                if(numoftick != eNum){
                    ProgError(STDLOG,
                            "Total number of tickets in \"EQN\" %d different from %d",
                              numoftick, eNum);
                    throw Exception("Wrong total number of flight segments");
                }
            } else {
                ProgError(STDLOG, "Unknown unit code [%s]", code);
                throw Exception ("Unknown unit code");
            }
            PopEdiPoint_wdG(pMes);
        }
        PopEdiPointG(pMes);
        PopEdiPointG(pMes);
    }

namespace {
date getIssueDate(EDI_REAL_MES_STRUCT *pMes)
{
    date DateOfIssue;
    PushEdiPointG(pMes);
    if(GetNumSegment(pMes, "PTK") ||
       SetEdiPointToSegGrG(pMes, SegGrElement(3,0)) )
    {
	// Берем PTK либо с самого верха, либо пониже (под TIF)
        SetEdiPointToSegmentG(pMes, SegmElement("PTK"), "INV_DATE_OF_ISSUE");
        DateOfIssue = GetDBFNameCast <date>
        (EdiCast::DateCast("%d%m%y","INV_DATE_OF_ISSUE"),
                                             pMes,
                                             DataElement(9916), "INV_DATE_OF_ISSUE",
                                             CompElement("C310"));
    }
    PopEdiPointG(pMes);

    return DateOfIssue;
}
    }// namespace ...
    ResContrInfo ResContrInfoEdiR::operator() (ReaderData &RData) const
{
    REdiData &Data = dynamic_cast<REdiData &>(RData);
    EDI_REAL_MES_STRUCT *pMes = Data.EdiMes();

    date DateOfIssue;
    DateOfIssue=getIssueDate(pMes);

    PushEdiPointG(pMes);
    SetEdiPointToSegmentG(pMes, "RCI",0, "INV_SYS_CONTROL");
    unsigned Num=GetNumComposite(pMes, "C330", "INV_SYS_CONTROL");

    if(/*Num>2 || */Num<1){
        ProgError(STDLOG,
                           "Bad number of record locators [%d]. Must be 1 or 2", Num);
        throw Exception("Bad number of record locators");
    }

    std::string OurAwk, CrsAwk;
    string CrsRecloc, AirRecloc, Recloc;

    PushEdiPointG(pMes);
    for(unsigned i=0;i<Num;i++)
    {
        SetEdiPointToCompositeG(pMes, "C330",i, "PROG_ERR");
        string Awk = GetDBNum(pMes, 9906,0, "INV_AIRLINE");
        string recloc = GetDBNum(pMes, 9956,0, "NEED_RECLOC");
        char typeChr=*GetDBNum(pMes, 9958);
        if (!typeChr){
            typeChr='1';
        }
    switch(typeChr){
        case '1':
            if(i==0 && Num>1){
                CrsRecloc = recloc;
                CrsAwk = Awk;
            } else {
                AirRecloc = recloc;
                OurAwk = Awk;
            }
            break;
        case '2':
            if(Recloc.size()){
                ProgError(STDLOG, "Too many reclocs with type 2");
                throw Exception("Too many reclocs with type 2");
            }
            Recloc = recloc;
            break;
        default:
            ProgError(STDLOG,
                                   "Invalid reservation control type [%c]", typeChr);
            throw Exception("Invalid reservation control type");
    }

    PopEdiPoint_wdG(pMes);
    }

    PopEdiPointG(pMes);
    PopEdiPointG(pMes);

    return ResContrInfo(Recloc, OurAwk, AirRecloc, CrsRecloc, CrsAwk, DateOfIssue);
}

OrigOfRequest OrigOfRequestEdiR::operator ( )(ReaderData &RData) const
{
    REdiData &Data = dynamic_cast<REdiData &>(RData);
    EDI_REAL_MES_STRUCT *pMes = Data.EdiMes();

    PushEdiPointG(pMes);
    SetEdiPointToSegmentG(pMes, "ORG",0, "NEED_ORG");
    PushEdiPointG(pMes);

    SetEdiPointToCompositeG(pMes, "C336", 0, "NEED_ORG");
    string Awk = GetDBNum(pMes, 9906,0, "INV_ORG");
    string Location = GetDBNum(pMes, 3225,0, "INV_ORG");
    PopEdiPoint_wdG(pMes); //ORG

    string PprNumber;
    string Agn;
    if (SetEdiPointToCompositeG(pMes, "C300")){
        GetDBNum(pMes, 9900,0, "INV_ORG");
        PprNumber = GetDBNum(pMes, 9900,0, "INV_ORG");
        if(PprNumber.size()<8 || PprNumber.size()>9)
        {
            ProgError(STDLOG,
                               "Invalid length of Travel Agent Id %s [%d, min/max 8/9]",
                               PprNumber.c_str(),PprNumber.size());
            throw Exception("Invalid length of Travel Agent Id");
        }
      Agn = GetDBNum(pMes, 9902);
      PopEdiPoint_wdG(pMes); //ORG
    }


    string OrigLocation = GetDBFName(pMes, DataElement(3225), "",CompElement("C328"));

    char Type = *GetDBNum(pMes, 9972);
    const char *lng = GetDBFName(pMes, DataElement(3453), "", CompElement("C354"));
    Lang::Language Lang;
    if(!strcmp(lng, "RU")){
        Lang = Lang::RUSSIAN;
    } else {
        ProgTrace(TRACE3,"Lang=%s, Use EN", lng);
        Lang = Lang::ENGLISH;
    }

    string AuthCode = GetDBNum(pMes, 9904,0, "INV_ORG");
    string Pult = GetDBNum(pMes, 3148);
    if(Pult.size()>6){
        ProgError(STDLOG, "Invalid length of the communication number %s [%d/6 max]",
                               Pult.c_str(), Pult.size());
        throw Exception("Invalid length of the communication number");
    }

    PopEdiPointG(pMes);
    PopEdiPointG(pMes);

    return OrigOfRequest(Awk, Location, PprNumber, Agn, OrigLocation, Type, Pult, AuthCode, Lang);
}

Passenger::SharedPtr PassengerEdiR::operator () (ReaderData &RData) const
{
    REdiData &Data = dynamic_cast<REdiData &>(RData);
    EDI_REAL_MES_STRUCT *pMes = Data.EdiMes();

    if(GetNumSegGr(pMes, 3, "EDI_INCOMLETE")>1)
    {
        ProgError(STDLOG,"Too many passengers in the ticket request [%d]",
                                GetNumSegGr(pMes, 3));
        throw Exception("Too many passengers in the ticket request");
    }
    SetEdiPointToSegGrG(pMes, 3,0, "PROG_ERR");
    //SegGr 2 (TIF) - информация по пассажиру

    PushEdiPointG(pMes);

    SetEdiPointToSegmentG(pMes, "TIF",0, "NEED_SURNAME");
    PushEdiPointG(pMes);
    SetEdiPointToCompositeG(pMes, "C322",0, "NEED_SURNAME");

    //Фамилия (обязательно)
    string Surname = GetDBNum(pMes, 9936,0, "NEED_SURNAME");
    // Тип
    string Type = GetDBNum(pMes, 6353);
    // Возраст
    tst();
    int Age = GetDBNumCast<int> (edilib::EdiDigitCast<int>(), pMes, 6060);
    tst();
    PopEdiPointG(pMes);

    // Имя (Обязательно/или нет (при разборе для поиска))
    string Name = GetDBFName(pMes, DataElement(9942),
                             "NEED_NAME",
                             CompElement("C324",0));

    if(Surname.size() > Passenger::MaxPassSurname){
        ProgError(STDLOG,"Surname too long! %d is a maximum",
                               Passenger::MaxPassSurname);
        throw Exception("Surname too long!");
    }
    if(Surname.size() < Passenger::MinPassSurname){
        ProgError(STDLOG, "Surname too short! %d is a minimum",
                               Passenger::MinPassSurname);
        throw Exception("Surname too short!");
    }
    if(Name.size() > Passenger::MaxPassName){
        ProgError(STDLOG, "Name too long! %d is a maximum",
                               Passenger::MaxPassName);
        throw Exception("Name too long!");
    }

    PopEdiPointG(pMes);

    return Passenger::SharedPtr (new Passenger(Surname, Name, Age, Type));
}

void TaxDetailsEdiR::operator () (ReaderData &RData, list<TaxDetails> &ltax) const
{
    REdiData &Data = dynamic_cast<REdiData &>(RData);
    EDI_REAL_MES_STRUCT *pMes = Data.EdiMes();

    PushEdiPointG(pMes);

    unsigned tax_mnum=GetNumSegment(pMes, "TXD");
    for(unsigned t=0; t<tax_mnum;t++){
        SetEdiPointToSegmentG(pMes, "TXD", t);
        unsigned tax_num=GetNumComposite(pMes, "C668","INV_TAX_AMOUNT");
        PushEdiPointG(pMes);
        TaxCategory TCat = GetDBNumCast<TaxCategory>
                (EdiCast::TaxCategoryCast(EtErr::INV_TAX_AMOUNT, TaxCategory::Current),
                 pMes, DataElement(5305)); //Category
        for (unsigned i=0; i<tax_num; i++){
            SetEdiPointToCompositeG(pMes, "C668",i, "INV_TAX_AMOUNT");
            TaxAmount::Amount Am =
                    GetDBNumCast<TaxAmount::Amount>
                    (EdiCast::AmountCast("INV_TAX_AMOUNT"), pMes, 5278,0,
                     "INV_TAX_AMOUNT"); //Amount
            std::string Type =
                    GetDBNum(pMes, 5153,0, "INV_TAX_CODE"); //Code
            ltax.push_back(TaxDetails(Am, TCat, Type));
            PopEdiPoint_wdG(pMes);
        }
        PopEdiPointG(pMes);
    }

    PopEdiPointG(pMes);
}

void MonetaryInfoEdiR::operator () (ReaderData &RData, list<MonetaryInfo> &lmon) const
{
    REdiData &Data = dynamic_cast<REdiData &>(RData);
    EDI_REAL_MES_STRUCT *pMes = Data.EdiMes();

    PushEdiPointG(pMes);

    if(!SetEdiPointToSegmentG(pMes, "MON",0,"MISS_MONETARY_INF"))
    {
        tst();
        return;
    }
    unsigned MonNum = GetNumComposite(pMes, "C663", "MISS_MONETARY_INF");

    bool have_base=false;
    bool have_total=false;

    PushEdiPointG(pMes);
    for(unsigned i=0;i<MonNum;i++)
    {
        SetEdiPointToCompositeG(pMes, "C663",i, "PROG_ERR");
        AmountCode::AmCode Ac = GetDBNumCast<AmountCode::AmCode>
                (EdiCast::AmountCodeCast("MISS_MONETARY_INF"), pMes, 5025,0,
                 "MISS_MONETARY_INF");

        std::string AmStr = GetDBNum(pMes, DataElement(1230), "INV_AMOUNT"); //Amount

        if(MonetaryType::checkExistence(AmStr))
        {
            lmon.push_back(MonetaryInfo(Ac, MonetaryType(AmStr)));
        }
        else
        {
            TaxAmount::Amount::AmountType_e type = Ac.codeInt() == AmountCode::CommissionRate?
                    TaxAmount::Amount::Percents : TaxAmount::Amount::Ordinary;
            TaxAmount::Amount Am = GetDBNumCast<TaxAmount::Amount>
                    (EdiCast::AmountCast("INV_AMOUNT", type),
                     pMes, DataElement(1230), "INV_AMOUNT"); //Amount

            std::string curr;
            if (!Am.isPercents() && Ac.codeInt() != AmountCode::ExchRate)
            {
                curr = GetDBNum(pMes, DataElement(6345), "INV_CURRENCY"); //Currency
            }
            lmon.push_back(MonetaryInfo(Ac, Am, curr));
        }
        PopEdiPoint_wdG(pMes);
    }
    PopEdiPointG(pMes);
    PopEdiPointG(pMes);

    if(!have_total){
        ProgError(STDLOG, "Missing total ticket amount");
        throw Exception("Missing total ticket amount");
    }
    if(!have_base){
        ProgError(STDLOG,"Missing base ticket amount");
        throw Exception("Missing base ticket amount");
    }
}

void FormOfPaymentEdiR::operator () (ReaderData &RData, list<FormOfPayment> &lfop) const
{
    REdiData &Data = dynamic_cast<REdiData &>(RData);
    EDI_REAL_MES_STRUCT *pMes = Data.EdiMes();

    PushEdiPointG(pMes);

    if(!SetEdiPointToSegmentG(pMes, "FOP",0,"MISS_FOP"))
    {
        tst();
        return;
    }
    unsigned Num=GetNumComposite(pMes, "C641", "PROG_ERR");
    PushEdiPointG(pMes); // Save FOP point
    for(unsigned i=0;i<Num;i++)
    {
        SetEdiPointToCompositeG(pMes, "C641", i, "PROG_ERR");

        //FOP type
        string FOPcode = GetDBNum(pMes, 9888,0, "PROG_ERR");

        FopIndicator FopInd = GetDBNumCast<FopIndicator>
                (EdiCast::FopIndicatorCast("INV_FOP", FopIndicator::New), pMes,
                 DataElement(9988)); //Indicator

        // Amount
        TaxAmount::Amount Am = GetDBNumCast<TaxAmount::Amount>
                (EdiCast::AmountCast("INV_AMOUNT"), pMes, 5004); //Amount

        string Vendor = GetDBNum(pMes, 9906);
        if(Vendor.size() && (Vendor.size()>FormOfPayment::VCMax ||
           Vendor.size()<FormOfPayment::VCMin))
        {
            ProgError(STDLOG, "Invalid vendor code");
            throw Exception("Invalid vendor code");
        }
        string AccountNum = GetDBNum(pMes, 1154);
        if(AccountNum.size() && Vendor.size() &&
           (AccountNum.size()>FormOfPayment::CrdNoMax ||
           AccountNum.size()<FormOfPayment::CrdNoMin))
        {
            ProgError(STDLOG,"Invalid account number");
            throw Exception("Invalid account number");
        }

        date date1=GetDBNumCast<date>(EdiCast::DateCast("%m%y","INV_DATE"),
                                      pMes, 9916);

        string AppCode = GetDBNum(pMes, 9889);
        if(AppCode.size()>FormOfPayment::AppCodeMax){
            ProgError(STDLOG, "Invalid approval code");
            throw Exception("Invalid approval code");
        }

        lfop.push_back(FormOfPayment(FOPcode, FopInd, Am, Vendor, AccountNum, date1, AppCode));

        PopEdiPoint_wdG(pMes); // Point to FOP
    }

    PopEdiPointG(pMes);
    PopEdiPointG(pMes);
}

void TicketEdiR::operator () (ReaderData &RData, list<Ticket> &ltick,
	               const CouponReader &cpnRead) const
{
    REdiData &Data = dynamic_cast<REdiData &>(RData);
    EDI_REAL_MES_STRUCT *pMes = Data.EdiMes();

    PushEdiPointG(pMes);

    unsigned numTKT=0;
    numTKT = GetNumSegGr(pMes, 4, "INV_TUCKNUM");
    if(numTKT>4){ // с учетом раздвоения
        ProgError(STDLOG, "Invalid number of conjunction tickets (%d), 4 maximum",
                               numTKT);
        throw Exception("Invalid number of conjunction tickets, 4 maximum");
    }
    for(unsigned short itick=0; itick<numTKT; itick++){
        SetEdiPointToSegGrG(pMes, 4, itick, "PROG_ERR");

        PushEdiPointG(pMes);
        SetEdiPointToSegmentG(pMes, "TKT", 0, "PROG_ERR");

        SetEdiPointToCompositeG(pMes, "C667",0, "INV_TUCKNUM");
        std::string ticknum = GetDBNum(pMes, 1004,0, "INV_TUCKNUM");
        const char *type=GetDBNum(pMes, 1001,0, "INV_DOC_TYPE");
        if(strcmp(type,"T")){
            //Странно, прислали вовсе не билет
            ProgError(STDLOG, "Invalid document type %s", type);
            throw Exception("Invalid document type");
        }
        //Общее кол-во билетов на пассажира (до 4х)
        unsigned Nbooklets = GetDBNumCast <unsigned> (EdiDigitCast<unsigned>("INV_COUPON"), pMes, 7240);
        if(!Nbooklets)
            Nbooklets = numTKT;
        if(Nbooklets!=numTKT){
            //Странное кол-во купонов
            ProgError(STDLOG,"Invalid number of booklets in TKT (%d). "
                    "Different from total number of conjunction tickets (%d)",
                    Nbooklets, numTKT);
            throw Exception("Invalid number of booklets in TKT");
        }
        TickStatAction::TickStatAction_t tick_act_code =
                GetDBNumCast<TickStatAction::TickStatAction_t>
                (EdiCast::TickActCast("INV_TICK_ACT"), pMes, 9988);
        PopEdiPointG(pMes);

        // Далее пошли по внутренностям...

        std::list<Coupon> lCpn;
        Data.setTickNum( make_pair(ticknum, tick_act_code) );
        cpnRead(Data, lCpn);

        ltick.push_back(Ticket(ticknum, tick_act_code, itick+1, lCpn));
        PopEdiPoint_wdG(pMes);
    }

    ResetEdiPointG(pMes);
    //CheckNumberOfUnits(pMes, ltick);

    PopEdiPointG(pMes);
}

Coupon_info MakeCouponInfo(EDI_REAL_MES_STRUCT *pMes)
{
    PushEdiPointG(pMes);
    SetEdiPointToSegmentG(pMes, "CPN",0, "INV_COUPON");

    int numCPN = GetNumComposite(pMes, "C640", "INV_COUPON");
    if( numCPN >1 ){
        ProgError(STDLOG,"Bad number of C640 (%d)! (More then 1)", numCPN);
        throw Exception("Bad number of C640! (More then 1)");
    }

    SetEdiPointToCompositeG(pMes, "C640");
    int num = GetDBNumCast<int>(EdiDigitCast<int>("INV_COUPON"),
                                pMes, 1050,0, "INV_COUPON");

    TicketMedia media=GetDBNumCast<TicketMedia>(EdiCast::TicketMediaCast("INV_MEDIA"),pMes, 1159);
    if(!media){
        media=TicketMedia::Electro;
    }
    if(media != TicketMedia::Electro &&
       media != TicketMedia::Paper){
        LogError(STDLOG) << "Invalid coupon media [" << media << "]";
        throw Exception("Invalid coupon media");
       }

       CouponStatus Status;
       if(GetNumDataElem(pMes, 4405)){
           Status = GetDBNumCast<CouponStatus>
                   (EdiCast::CoupStatCast("INV_COUPON"),
                    pMes, 4405,0, "INV_COUPON");
       } else {
           if(media == TicketMedia::Electro){
               Status = CouponStatus(CouponStatus::OriginalIssue);
           } else {
               Status = CouponStatus(CouponStatus::Paper);
           }
       }

       string sac = GetDBNum(pMes, 9887);
       PopEdiPointG(pMes);

       return Coupon_info(num, Status, media, sac);
}
namespace {
    inline Luggage MakeLuggage(EDI_REAL_MES_STRUCT *pMes)
    {
        int quantity=0;
        string charge, measure;
        bool have_lugg=false;
        PushEdiPointG(pMes);
        if(SetEdiPointToSegmentG(pMes, "EBD")){
            SetEdiPointToCompositeG(pMes, "C675",0, "INV_LUGGAGE");
            quantity   =  GetDBNumCast <int>
                    (EdiDigitCast<int>("INV_LUGGAGE"), pMes, 6060,0,
                     "INV_LUGGAGE");
            charge     =  GetDBNum(pMes, 5463);
            measure    =  GetDBNum(pMes, 6411);
            have_lugg=true;
        }
        PopEdiPointG(pMes);

        return have_lugg?Luggage(quantity, charge, measure):Luggage();
    }

    inline string GetFareBasis(EDI_REAL_MES_STRUCT *pMes)
    {
        string fare = GetDBFName(pMes, DataElement(5242), "",
                                 CompElement("C643"),
                                 SegmElement("PTS"));
        if(fare.size()>Itin::FareBasisLen){
            ProgError(STDLOG,"Invalid length of fare basis '%s'",
                      fare.c_str());
            throw Exception("Invalid length of fare basis");
        }
        return fare;
    }

inline pair<date,date> GetValidDates (EDI_REAL_MES_STRUCT *pMes)
{
    date DBefore;
    date DAfter;
    PushEdiPointG(pMes);
    if(SetEdiPointToSegmentG(pMes, SegmElement("DAT")))
    {
        unsigned num = GetNumComposite(pMes, "C688");
        for(unsigned i=0;i<num;i++){
            PushEdiPointG(pMes);

            SetEdiPointToCompositeG(pMes, CompElement("C688",i));
            const char *code = GetDBNum(pMes, DataElement(2005));
            if(!*code){
                PopEdiPointG(pMes);
                continue;
            }
            if(!strcmp(code, "B")){
                if(DBefore.is_special())
                {
                    DBefore = GetDBFNameCast <date>
                            (EdiCast::DateCast("%d%m%y","INV_DATE"),
                             pMes, DataElement(9916));
                } else {
                    ProgError(STDLOG,"Duplicate code B for DAT");
                }
            } else if(!strcmp(code, "A"))
            {
                if(DAfter.is_special())
                {
                    DAfter = GetDBFNameCast <date>
                            (EdiCast::DateCast("%d%m%y","INV_DATE"),
                             pMes, DataElement(9916));
                } else {
                    ProgError(STDLOG,"Duplicate code A for DAT");
                }
            } else {
                WriteLog(STDLOG, "Unknown code for DAT: %s", code);
            }
            PopEdiPointG(pMes);
        }
    }
    PopEdiPointG(pMes);
    return make_pair(DBefore, DAfter);
}


//TVL++BER+TJM+UT+OPEN:Y'
Itin MakeItin(EDI_REAL_MES_STRUCT *pMes, const string &tnum)
{
    //Маршрут/Itinerary
    PushEdiPointG(pMes);

    SetEdiPointToSegmentG(pMes, "TVL",0, "INV_DATE");

    // Date/Time
    PushEdiPointG(pMes);
    date Date1;
    time_duration Time1(not_a_date_time);
    bool open=false;
    if(SetEdiPointToCompositeG(pMes, "C310")) {
        Date1 = GetDBNumCast  <date> (EdiCast::DateCast("%d%m%y","INV_DATE"),
                                      pMes, 9916,0, "INV_DATE");
        Time1 = GetDBNumCast  <time_duration>
                (EdiCast::TimeCast("%H%M",  "INV_TIME"),
                 pMes, 9918,0, "INV_TIME");
        PopEdiPoint_wdG(pMes);
    } else {
        open = true;
    }

    // City/Port Dep/Arr
    string Dep_point = GetDBFName
            (pMes, DataElement(3225), "INV_CITY_PORT",
             CompElement("C328",0));

    string Arr_point = GetDBFName
            (pMes, DataElement(3225), "INV_CITY_PORT",
             CompElement("C328",1));

    SetEdiPointToCompositeG(pMes, "C306",0, "INV_AIRLINE");
    //Sold marketing airline designator code
    string Awk = GetDBNum(pMes, 9906,0, "INV_AIRLINE");
    //Sold operating airline code when different from marketing airline
    string Oper_awk = GetDBNum(pMes, 9906,1);
    PopEdiPoint_wdG();

    SetEdiPointToCompositeG(pMes, "C308",0, "INV_FL_NUM");
    //Flight Number
    int Flightnum=0;
    if(!open){
        Flightnum = GetDBNumCast <int> (EdiDigitCast<int>("INV_FL_NUM"),
                                        pMes, 9908,0, "INV_FL_NUM");
    }
    //Reservation Booking Designator
    SubClass Class =GetDBNumCast <SubClass> (EdiCast::RBDCast("INV_RBD"),
                                         pMes, DataElement(7037), "INV_RBD");
    //PopEdiPoint_wdG();
//     SetEdiPointToCompositeG(pMes, "C309",0, );
//     LineNum = GetDBNumCast<EdiDigitCast, int> (pMes, 1082);

    PopEdiPointG(pMes); //Вышли из TVL

    //Status
    PopEdiPoint_wdG(pMes);
    ItinStatus RpiStat;
    if(SetEdiPointToSegmentG(pMes, "RPI")){
        RpiStat = GetDBNumCast <ItinStatus>
                (EdiCast::ItinStatCast("INV_ITIN_STATUS"),
                 pMes, 4405,0, "INV_ITIN_STATUS");

        if (open && RpiStat != ItinStatus::OpenDate){

        }
    }

    PopEdiPoint_wdG(pMes);
    //Luggage
    Luggage Lugg = MakeLuggage(pMes);
    string FareBasis = GetFareBasis(pMes);
    pair<date,date> ValidDates = GetValidDates(pMes);

    PopEdiPointG(pMes);
    return Itin(tnum,
                Awk, Oper_awk,
                Flightnum,0, Class,
                Date1, Time1,
                date(), time_duration(),
                Dep_point, Arr_point, ValidDates,
                RpiStat, FareBasis, 1, Lugg);
}
} // namespace ...
void CouponEdiR::operator () (ReaderData &RData, list<Coupon> &lCpn) const
{
    REdiData &Data = dynamic_cast<REdiData &>(RData);
    EDI_REAL_MES_STRUCT *pMes = Data.EdiMes();

    int numCoup = GetNumSegGr(pMes, 5, "NO_COUPONS");
    if(numCoup>4){
        ProgError(STDLOG, "Too many coupons for the ticket %s",
                  Data.currTicket().first.c_str());
        throw Exception("Too many coupons for the ticket");
    }

    PushEdiPoint();
    for(int i=0; i<numCoup; i++){
        SetEdiPointToSegGrG(pMes, 5,i, "PROG_ERR");
        //Считываем купоны для текущего буклета
        //Coupon
        Coupon_info Ci = MakeCouponInfo(pMes);
        Data.setCurrCoupon(Ci.num());

        list<FrequentPass> lFti;
        frequentPassRead()(RData, lFti);
        lCpn.push_back(Coupon(Ci, MakeItin(pMes, Data.currTicket().first),
                       lFti, Data.currTicket().first));
        PopEdiPoint_wd();
    }
    PopEdiPoint();
}

void FormOfIdEdiR::operator ( )(ReaderData &RData, std::list< FormOfId > & lfoid) const
{
    REdiData &Data = dynamic_cast<REdiData &>(RData);
    EDI_REAL_MES_STRUCT *pMes = Data.EdiMes();

    PushEdiPointG(pMes);
    if(SetEdiPointToSegmentG(pMes, SegmElement("CRI")))
    {
        PushEdiPointG(pMes);
        int numFOID = GetNumComposite(pMes, "C967");
        for (int curFoid=0; curFoid < numFOID; curFoid++)
        {
            if(!SetEdiPointToCompositeG(pMes, CompElement("C967", curFoid)))
            {
                throw EXCEPTIONS::Exception("SetEdiPointToCompositeG() failed");
            }

            FoidType TFoid = GetDBNumCast <FoidType> (EdiCast::FoidTypeCast(EtErr::INV_FOID),
                    pMes, DataElement(1153));

            std::string DocNum = GetDBNum(pMes,1154);
            std::string Owner  = GetDBNum(pMes,3036);

            if(DocNum.length() > FormOfId::MaxNumberLen)
            {
                throw Exception("Invalid FOID document length");
            }

            if(Owner.length() > FormOfId::MaxOwnerLen)
            {
                throw Exception("Invalid FOID owner length");
            }

            lfoid.push_back(FormOfId(TFoid, DocNum, Owner));
            PopEdiPoint_wdG(pMes);
        }
        PopEdiPointG(pMes);
    }

    PopEdiPointG(pMes);
}


void FrequentPassEdiR::operator () (ReaderData &RData, list<FrequentPass> &lFti) const
{
    REdiData &Data = dynamic_cast<REdiData &>(RData);
    EDI_REAL_MES_STRUCT *pMes = Data.EdiMes();

    // Частолетающий пассажир
    PushEdiPointG(pMes);
    if(!SetEdiPointToSegmentG(pMes, "FTI")){
        PopEdiPointG(pMes);
        ProgTrace(TRACE3,"There are no FTI information found");
        return;
    }

    unsigned num = GetNumComposite(pMes, "C326", "INV_FTI");
    for(unsigned i=0;i<num;i++){
        SetEdiPointToCompositeG(pMes, "C326",i, "PROG_ERR");

        string comp = GetDBNum(pMes, 9906,0, "INV_FTI");
        string docnum = GetDBNum(pMes, 9948,0, "INV_FTI");
        if(docnum.size() > 20 || docnum.size() < 5){
            ProgError(STDLOG, "Bad length of FQTV number len=%d, num=%s",
                                   docnum.size(), docnum.c_str());
            throw Exception("Bad length of FQTV number");
        }
        lFti.push_back(FrequentPass(Data.currTicket().first, Data.currCoupon(), comp, docnum));
    }
    PopEdiPointG(pMes);
}

namespace {
    void makeIFT__(EDI_REAL_MES_STRUCT *pMes,
                 unsigned level,
                 const string &currTicket,
                 int currCoupon,
                 list<FreeTextInfo> &lIft)
    {
        PushEdiPointG(pMes);
        unsigned num = GetNumSegment(pMes, "IFT");
        if(!num){
            ProgTrace(TRACE3,"There are no IFT information found at level %d", level);
            PopEdiPointG(pMes);
            return;
        }
        ProgTrace(TRACE3,"IFT count=%d", num);
        for(unsigned i=0;i<num;i++)
        {
            SetEdiPointToSegmentG(pMes, "IFT",i, "PROG_ERR");

            PushEdiPointG(pMes);
            SetEdiPointToCompositeG(pMes, "C346",0, "INV_IFT_QUALIFIER");
            string Qualifier = GetDBNum(pMes, 4451,0, "INV_IFT_QUALIFIER");
            string Type = GetDBNum(pMes, 9980);
            PopEdiPointG(pMes);

            unsigned numDText = GetNumDataElem(pMes, 4440);
            if (!numDText){
                ProgError(STDLOG,"Missing IFT data");
                throw Exception("Missing IFT data");
            }
            ProgTrace(TRACE3,"IFT DATA count=%d", numDText);
            for(unsigned j=0;j<numDText;j++)
            {
                ProgTrace(TRACE3,"getting 4440,%d", j);
                string Text = GetDBNum(pMes, 4440,j, "PROG_ERR");
                ProgTrace(TRACE3,"%s:%s:%s", Qualifier.c_str(), Type.c_str(), Text.c_str());
                if(j==0){
                    lIft.push_back(
                            FreeTextInfo(i,
                                         level,
                                         FreeTextType::FTxtType(Type,Qualifier),
                                         Text,
                                         currTicket,
                                         currCoupon));
                } else {
                    lIft.back().addText(Text);
                }
            }
            PopEdiPoint_wdG(pMes);
        }
        PopEdiPointG(pMes);
    }

    inline void makeIFT(REdiData &Data, unsigned level, list<FreeTextInfo> &lIft)
    {
        makeIFT__(Data.EdiMes(),
                       level,
                       Data.currTicket().first,
                       Data.currCoupon(),
                       lIft);
    }
} // namespace ...

void readEdiIFT(EDI_REAL_MES_STRUCT *pMes, list<FreeTextInfo> &lIft)
{
    return makeIFT__(pMes, 0, "", 0, lIft);
}

void FreeTextInfoEdiR::operator () (ReaderData &RData, list<FreeTextInfo> &lIft) const
{
    REdiData &Data = dynamic_cast<REdiData &>(RData);
    EDI_REAL_MES_STRUCT *pMes = Data.EdiMes();

    unsigned lvl=0;
    if(Data.currTicket().first.size()==0){
	// Если вызов из Сегм Гр 2 (TIF)
        PushEdiPointG(pMes);
        ResetEdiPointG(pMes);
        // Переход на самый верх
        makeIFT(Data, lvl, lIft);
        PopEdiPointG(pMes);
        // Переход туда, где были
        lvl=1;
    } else if(Data.currCoupon()){
	// Текст к купону
        lvl=3;
    } else {
	// Текст к билету
        lvl=2;
    }

    makeIFT(Data, lvl, lIft);
}

} //namespace Ticketing
} //namespace TickReader
