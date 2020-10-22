#pragma once

#include <list>
#include <string>
#include "astra_consts.h"
#include <set>

class DCSServiceApplyingParams
{
  public:
    std::string airline;
    DCSAction::Enum dcs_action;
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
    static bool isAllowed(int pax_id, DCSAction::Enum dcsAction, RFISCsSet& reqRFISCs);
    static void throwIfNotAllowed(int pax_id, DCSAction::Enum dcsAction);
};


