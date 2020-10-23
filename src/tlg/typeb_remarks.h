#pragma once

#include <string.h>
#include "date_time.h"
#include "astra_consts.h"
#include "oralib.h"
#include "remarks.h"
#include "astra_types.h"
#include "passenger.h"

using BASIC::date_time::TDateTime;

namespace TypeB
{

class TDetailRemAncestor
{
  public:
    char rem_code[6],rem_status[3];
    bool pr_inf;
    TDetailRemAncestor()
    {
      Clear();
    }
    virtual ~TDetailRemAncestor() {}

    bool isHKReservationsStatus() const
    {
      return strcmp(rem_status, "HK")==0;
    }

    bool infIndicatorExists() const
    {
      return pr_inf;
    }

  protected:
    void Clear()
    {
      *rem_code=0;
      *rem_status=0;
      pr_inf=false;
    }
    bool equalExcludingInfIndicator(const TDetailRemAncestor &item) const
    {
      return strcmp(rem_code, item.rem_code)==0 &&
             strcmp(rem_status, item.rem_status)==0;
    }
    bool equalOnlyRemCode(const TDetailRemAncestor &item) const
    {
      return strcmp(rem_code, item.rem_code)==0;
    }
};

class TDocItem : public TDetailRemAncestor
{
  public:
    char type[3],issue_country[4],no[16];
    char nationality[4],gender[3];
    TDateTime birth_date,expiry_date;
    std::string surname,first_name,second_name;
    bool pr_multi;
    TDocItem()
    {
      Clear();
    }
    void Clear()
    {
      TDetailRemAncestor::Clear();
      *type=0;
      *issue_country=0;
      *no=0;
      *nationality=0;
      *gender=0;
      birth_date=ASTRA::NoExists;
      expiry_date=ASTRA::NoExists;
      surname.clear();
      first_name.clear();
      second_name.clear();
      pr_multi=false;
    }
    bool Empty() const
    {
      return *type==0 &&
             *issue_country==0 &&
             *no==0 &&
             *nationality==0 &&
             *gender==0 &&
             birth_date==ASTRA::NoExists &&
             expiry_date==ASTRA::NoExists &&
             surname.empty() &&
             first_name.empty() &&
             second_name.empty() &&
             pr_multi==false;
    }
    bool operator == (const TDocItem &item) const
    {
      return TDetailRemAncestor::equalExcludingInfIndicator(item) &&
             strcmp(type, item.type)==0 &&
             strcmp(issue_country, item.issue_country)==0 &&
             strcmp(no, item.no)==0 &&
             strcmp(nationality, item.nationality)==0 &&
             strcmp(gender, item.gender)==0 &&
             birth_date==item.birth_date &&
             expiry_date==item.expiry_date &&
             surname==item.surname &&
             first_name==item.first_name &&
             second_name==item.second_name &&
             pr_multi==item.pr_multi;
    }
    bool suitableForDB() const { return !Empty(); }
};

class TDocExtraItem
{
  public:
    char type_rcpt[4],no[16];
    TDocExtraItem()
    {
      Clear();
    }
    void Clear()
    {
      *type_rcpt=0;
      *no=0;
    }
    bool Empty() const
    {
      return  *type_rcpt==0 &&
              *no==0;
    }
    bool operator == (const TDocExtraItem &item) const
    {
      return strcmp(type_rcpt, item.type_rcpt)==0 &&
             strcmp(no, item.no)==0;
    }
    bool valid() const
    {
      return *type_rcpt!=0 &&
             *no!=0;
    }
};

class TDocoItem : public TDetailRemAncestor
{
  public:
    char type[3],no[26],applic_country[4];
    TDateTime issue_date;
    std::string birth_place, issue_place;
    TDocoItem()
    {
      Clear();
    }
    void Clear()
    {
      TDetailRemAncestor::Clear();
      *type=0;
      *no=0;
      *applic_country=0;
      issue_date=ASTRA::NoExists;
      birth_place.clear();
      issue_place.clear();
    }
    bool Empty() const
    {
      return *type==0 &&
             *no==0 &&
             *applic_country==0 &&
             issue_date==ASTRA::NoExists &&
             birth_place.empty() &&
             issue_place.empty();
    }
    bool operator == (const TDocoItem &item) const
    {
      return TDetailRemAncestor::equalExcludingInfIndicator(item) &&
             strcmp(type, item.type)==0 &&
             strcmp(no, item.no)==0 &&
             strcmp(applic_country, item.applic_country)==0 &&
             issue_date==item.issue_date &&
             birth_place==item.birth_place &&
             issue_place==item.issue_place;
    }
    bool suitableForDB() const { return !Empty(); }
};

class TDocaItem : public TDetailRemAncestor
{
  public:
    char type[2],country[4];
    std::string address, city, region, postal_code;
    TDocaItem()
    {
      Clear();
    }
    void Clear()
    {
      TDetailRemAncestor::Clear();
      *type=0;
      *country=0;
      address.clear();
      city.clear();
      region.clear();
      postal_code.clear();
    }
    bool Empty() const
    {
      return *type==0 &&
             *country==0 &&
             address.empty() &&
             city.empty() &&
             region.empty() &&
             postal_code.empty();
    }
    bool operator == (const TDocaItem &item) const
    {
      return TDetailRemAncestor::equalExcludingInfIndicator(item) &&
             strcmp(type, item.type)==0 &&
             strcmp(country, item.country)==0 &&
             address==item.address &&
             city==item.city &&
             region==item.region &&
             postal_code==item.postal_code;
    }
    bool suitableForDB() const { return !Empty(); }
};

class TTKNItem : public TDetailRemAncestor
{
  public:
    char ticket_no[16];
    int coupon_no;
    TTKNItem()
    {
      Clear();
    }
    TTKNItem(TQuery &Qry)
    {
      fromDB(Qry);
    }

    void Clear()
    {
      TDetailRemAncestor::Clear();
      *ticket_no=0;
      coupon_no=ASTRA::NoExists;
    }
    bool Empty() const
    {
      return *ticket_no==0 &&
             coupon_no==ASTRA::NoExists;
    }
    bool operator == (const TTKNItem &item) const
    {
      return TDetailRemAncestor::equalOnlyRemCode(item) &&
             strcmp(ticket_no, item.ticket_no)==0 &&
             coupon_no==item.coupon_no;
    }
    bool suitableForDB() const { return !Empty(); }
    void toDB(TQuery &Qry) const;
    void fromDB(TQuery &Qry);
};

class TFQTItem
{
  public:
    char rem_code[6],airline[4],no[26];
    std::string extra;
    TFQTItem()
    {
      Clear();
    }
    void Clear()
    {
      *rem_code=0;
      *airline=0;
      *no=0;
      extra.clear();
    }
    bool Empty() const
    {
      return *airline==0 &&
             *no==0 &&
             extra.empty();
    }
    bool operator == (const TFQTItem &item) const
    {
      return strcmp(rem_code, item.rem_code)==0 &&
             strcmp(airline, item.airline)==0 &&
             strcmp(no, item.no)==0 &&
             extra==item.extra;
    }
    bool suitableForDB() const { return !Empty(); }
};

class TFQTExtraItem
{
  public:
    std::string tier_level;
    TFQTExtraItem()
    {
      Clear();
    }
    void Clear()
    {
      tier_level.clear();
    }
    bool Empty() const
    {
      return tier_level.empty();
    }
    bool operator < (const TFQTExtraItem &item) const
    {
      return tier_level<item.tier_level;
    }
    bool operator == (const TFQTExtraItem &item) const
    {
      return tier_level==item.tier_level;
    }
    bool valid() const
    {
      return !tier_level.empty();
    }
};

class TCHKDItem : public TDetailRemAncestor
{
  public:
    long int reg_no;
    TCHKDItem()
    {
      Clear();
    }
    void Clear()
    {
      TDetailRemAncestor::Clear();
      reg_no=ASTRA::NoExists;
    }
    bool Empty() const
    {
      return reg_no==ASTRA::NoExists;
    }
};

class TASVCItem : public TDetailRemAncestor
{
  public:
    char RFIC[2], RFISC[16], ssr_code[5], service_name[31];
    int service_quantity;
    char emd_type[2], emd_no[14];
    int emd_coupon;
    TASVCItem()
    {
      Clear();
    }
    void Clear()
    {
      TDetailRemAncestor::Clear();
      *RFIC=0;
      *RFISC=0;
      service_quantity=ASTRA::NoExists;
      *ssr_code=0;
      *service_name=0;
      *emd_type=0;
      *emd_no=0;
      emd_coupon=ASTRA::NoExists;
    }
    bool Empty() const
    {
      return *RFIC==0 &&
             *RFISC==0 &&
             service_quantity==ASTRA::NoExists &&
             *ssr_code==0 &&
             *service_name==0 &&
             *emd_type==0 &&
             *emd_no==0 &&
             emd_coupon==ASTRA::NoExists;
    }
    bool emdRequired() const
    {
      return strncmp(rem_status, "HD", 2)==0;
    }

    bool operator == (const TASVCItem &item) const
    {
      return TDetailRemAncestor::equalExcludingInfIndicator(item) &&
             strcmp(RFIC, item.RFIC)==0 &&
             strcmp(RFISC, item.RFISC)==0 &&
             service_quantity==item.service_quantity &&
             strcmp(ssr_code, item.ssr_code)==0 &&
             strcmp(service_name, item.service_name)==0 &&
             strcmp(emd_type, item.emd_type)==0 &&
             strcmp(emd_no, item.emd_no)==0 &&
             emd_coupon==item.emd_coupon;
    }

    bool suitableForDB() const { return !Empty() && !std::string(rem_status).empty(); }
};

class TSeatBlockingRem : public TDetailRemAncestor
{
  public:
    int numberOfSeats;
    TSeatBlockingRem() { clear(); }
    void clear()
    {
      TDetailRemAncestor::Clear();
      numberOfSeats=0;
    }
    bool isSTCR() const
    {
      return strcmp(rem_code, "STCR")==0;
    }
};

class TRemItem
{
  public:
    std::string text;
    char code[6];
    TRemItem()
    {
      *code=0;
    }
    bool isPDRem() const
    {
      return strlen(code)==4 && strncmp(code, "PD", 2)==0;
    }
};

class TPDRemItem : public TRemItem
{
  public:
    TPDRemItem(const TRemItem &item) : TRemItem(item) {}
    TPDRemItem() : TRemItem() {}
    bool operator < (const TPDRemItem &item) const
    {
      return text<item.text;
    }
    bool operator == (const TPDRemItem &item) const
    {
      return text==item.text;
    }
};

void SaveDOCSRem(const PaxIdWithSegmentPair& paxId,
                 const bool paxIdUsedBefore,
                 const std::vector<TDocItem> &doc,
                 const std::map<std::string, TDocExtraItem> &doc_extra,
                 ModifiedPaxRem& modifiedPaxRem);
void SaveDOCORem(const PaxIdWithSegmentPair& paxId,
                 const bool paxIdUsedBefore,
                 const std::vector<TDocoItem> &doc,
                 ModifiedPaxRem& modifiedPaxRem);
void SaveDOCARem(const PaxIdWithSegmentPair& paxId,
                 const bool paxIdUsedBefore,
                 const std::vector<TDocaItem> &doca,
                 ModifiedPaxRem& modifiedPaxRem);
void SaveTKNRem(const PaxIdWithSegmentPair& paxId,
                const bool paxIdUsedBefore,
                const std::vector<TTKNItem> &tkn,
                ModifiedPaxRem& modifiedPaxRem);
void SaveFQTRem(const PaxIdWithSegmentPair& paxId,
                const bool paxIdUsedBefore,
                const std::vector<TFQTItem> &fqt,
                const std::set<TFQTExtraItem> &fqt_extra,
                ModifiedPaxRem& modifiedPaxRem);
bool SaveCHKDRem(const PaxIdWithSegmentPair& paxId,
                 const std::vector<TCHKDItem> &chkd);
void SaveASVCRem(const PaxIdWithSegmentPair& paxId,
                 const std::vector<TASVCItem> &asvc,
                 ModifiedPaxRem& modifiedPaxRem);
void SavePNLADLRemarks(const PaxIdWithSegmentPair& paxId,
                       const std::vector<TRemItem> &rem);
void DeletePDRem(const PaxIdWithSegmentPair& paxId,
                 const std::vector<TRemItem> &rem1,
                 const std::vector<TRemItem> &rem2,
                 ModifiedPaxRem& modifiedPaxRem);




} //namespace TypeB
