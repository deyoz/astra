#pragma once

#include <list>
#include <string>
#include "astra_consts.h"

class DCSService
{
  public:
    enum Enum
    {
      PrintBPOnDesk,
      ChangeSeatOnDesk
    };

    static const std::list< std::pair<Enum, std::string> >& pairs()
    {
      static std::list< std::pair<Enum, std::string> > l =
      {
        {PrintBPOnDesk,    "PRINT_BP_ON_DESK"},
        {ChangeSeatOnDesk, "CHG_SEAT_ON_DESK"}
      };
      return l;
    }
};

class DCSServices : public ASTRA::PairList<DCSService::Enum, std::string>
{
  private:
    virtual std::string className() const { return "DCSServices"; }
  public:
    DCSServices() : ASTRA::PairList<DCSService::Enum, std::string>(DCSService::pairs(),
                                                                   boost::none,
                                                                   boost::none) {}
};

const DCSServices& dcsServices();

class DCSServiceApplyingParams
{
  public:
    std::string airline;
    DCSService::Enum dcs_service;
    std::string brand_airline;
    std::string brand_code;
    std::string fqt_airline;
    std::string fqt_tier_level;
    std::string cl;
};

class RFISCsSet : public std::set<std::string> {};

class DCSServiceApplying
{
  private:
    static void addRequiredRFISCs(const DCSServiceApplyingParams& params, RFISCsSet& rfiscs);
  public:
    static bool isAllowed(int pax_id, DCSService::Enum dcs_service);
    static void throwIfNotAllowed(int pax_id, DCSService::Enum dcs_service);
};


