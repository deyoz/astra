#include "dcs_services.h"
#include "qrys.h"
#include "passenger.h"
#include "brands.h"
#include "rfisc.h"

#define NICKNAME "VLAD"
#include "serverlib/slogger.h"

using namespace std;

const DCSServices& dcsServices() { return ASTRA::singletone<DCSServices>(); }


std::ostream& operator<<(std::ostream& os, const DCSServiceApplyingParams& params)
{
  os << endl << "  airline:        " << params.airline
     << endl << "  dcs_service:    " << dcsServices().encode(params.dcs_service);
  if (!params.brand_airline.empty())
    os << endl << "  brand_airline:  " << params.brand_airline;
  if (!params.brand_code.empty())
    os << endl << "  brand_code:     " << params.brand_code;
  if (!params.fqt_airline.empty())
    os << endl << "  fqt_airline:    " << params.fqt_airline;
  if (!params.fqt_tier_level.empty())
    os << endl << "  fqt_tier_level: " << params.fqt_tier_level;
  if (!params.cl.empty())
    os << endl << "  cl:             " << params.cl;
  return os;
}

std::ostream& operator<<(std::ostream& os, const RFISCsSet& rfiscs)
{
  for(const auto& i : rfiscs)
    os << " " << i;
  return os;
}

void DCSServiceApplying::addRequiredRFISCs(const DCSServiceApplyingParams& params, RFISCsSet& rfiscs)
{
  LogTrace(TRACE5) << __FUNCTION__ << ": " << params;

  TCachedQuery Qry(
    "SELECT id, rfisc "
    "FROM dcs_service_applying "
    "WHERE airline=:airline AND "
    "      dcs_service=:dcs_service AND "
    "      (brand_airline IS NULL OR brand_airline=:brand_airline) AND "
    "      (brand_code IS NULL OR brand_code=:brand_code) AND "
    "      (fqt_airline IS NULL OR fqt_airline=:fqt_airline) AND "
    "      (fqt_tier_level IS NULL OR fqt_tier_level=:fqt_tier_level) AND "
    "      (class IS NULL OR class=:class) AND "
    "      pr_denial=0 ",
    QParams() << QParam("airline", otString, params.airline)
              << QParam("dcs_service", otString, dcsServices().encode(params.dcs_service))
              << QParam("brand_airline", otString, params.brand_airline)
              << QParam("brand_code", otString, params.brand_code)
              << QParam("fqt_airline", otString, params.fqt_airline)
              << QParam("fqt_tier_level", otString, params.fqt_tier_level)
              << QParam("class", otString, params.cl)
  );

  ostringstream s;
  Qry.get().Execute();
  for(; !Qry.get().Eof; Qry.get().Next())
  {
    int id=Qry.get().FieldAsInteger("id");
    string rfisc=Qry.get().FieldAsString("rfisc");
    s << "(" << id << ", " << rfisc << ") ";
    rfiscs.insert(rfisc);
  }
  LogTrace(TRACE5) << __FUNCTION__ << ": " << s.str();
}

bool DCSServiceApplying::isAllowed(int pax_id, DCSService::Enum dcs_service)
{
  LogTrace(TRACE5) << __FUNCTION__ << ": pax_id=" << pax_id <<
                                      ", dcs_service=" << dcsServices().encode(dcs_service);

  CheckIn::TSimplePaxItem pax;
  if (!pax.getByPaxId(pax_id)) return false;

  CheckIn::TSimplePaxGrpItem grp;
  if (!grp.getByGrpId(pax.grp_id)) return false;

  TTripInfo flt;
  if (!flt.getByPointId(grp.point_dep)) return false;

  TBrands brands;
  brands.get(pax.id);
  if (brands.brandIds.empty()) brands.brandIds.push_back(ASTRA::NoExists);

  set<CheckIn::TPaxFQTItem> fqts;
  CheckIn::LoadPaxFQT(pax.id, fqts);
  if (fqts.empty()) fqts.insert(CheckIn::TPaxFQTItem());

  RFISCsSet reqRFISCs;

  DCSServiceApplyingParams params;
  params.airline=flt.airline;
  params.dcs_service=dcs_service;
  params.cl=grp.cl;
  for(int brandId : brands.brandIds)
  {
    params.brand_airline=brandId!=ASTRA::NoExists?flt.airline:"";
    params.brand_code=   brandId!=ASTRA::NoExists?ElemIdToCodeNative(etBrand, brandId):"";

    for(const CheckIn::TPaxFQTItem& fqt : fqts)
    {
      params.fqt_airline=fqt.airline;
      params.fqt_tier_level=fqt.tier_level;
      addRequiredRFISCs(params, reqRFISCs);
    }
  }

  if (reqRFISCs.empty()) return true;  //�� ��諨 �� ��������/����樨 �� ������ �ॡ㥬��� RFISC�

  RFISCsSet paxRFISCs, intersectRFISCs;

  TPaidRFISCListWithAuto paid;
  paid.fromDB(pax.id, false);
  paid.getUniqRFISCSs(pax.id, paxRFISCs);

  set_intersection(reqRFISCs.begin(), reqRFISCs.end(),
                   paxRFISCs.begin(), paxRFISCs.end(),
                   inserter(intersectRFISCs, intersectRFISCs.end()));

  LogTrace(TRACE5) << __FUNCTION__ << ": reqRFISCs:" << reqRFISCs;
  LogTrace(TRACE5) << __FUNCTION__ << ": paxRFISCs:" << paxRFISCs;
  LogTrace(TRACE5) << __FUNCTION__ << ": intersectRFISCs:" << intersectRFISCs;

  return !intersectRFISCs.empty();
}
