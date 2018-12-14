#include "brands.h"
#include "qrys.h"
#include "passenger.h"
#include "etick.h"

#define NICKNAME "DEN"
#include "serverlib/slogger.h"

using namespace std;

void TBrands::get(int pax_id)
{
    brandIds.clear();
    TCachedQuery paxQry(
            "select "
            "   ticket_no, "
            "   coupon_no, "
            "   ticket_rem, "
            "   ticket_confirm, "
            "   points.airline "
            "from "
            "   pax, "
            "   pax_grp, "
            "   points "
            "where "
            "   pax_id = :pax_id and "
            "   pax.grp_id = pax_grp.grp_id and "
            "   pax_grp.point_dep = points.point_id ",
            QParams() << QParam("pax_id", otInteger, pax_id));
    paxQry.get().Execute();
    if(not paxQry.get().Eof) {
        CheckIn::TPaxTknItem tkn;
        tkn.fromDB(paxQry.get());
        if (tkn.validET())
        {
            string airline = paxQry.get().FieldAsString("airline");
            TETickItem etick;
            etick.fromDB(tkn.no, tkn.coupon, TETickItem::Display, false);
            if(not etick.fare_basis.empty()) {
                return get(airline, etick.fare_basis);
            }
        }
    }
}

void TBrands::get(const std::string &airline, const std::string &fare_basis)
{
    oper_airline = airline;
    brandIds.clear();
    if (airline.empty() || fare_basis.empty()) return;

    getsTotal++;

    const auto i = secretMap.emplace(make_pair(airline, fare_basis), BrandIds());

    if (i.second)
    {
      TCachedQuery brandQry(
            "select "
            "   brands.id "
            "from "
            "   brand_fares, "
            "   brands "
            "where "
            "   brand_fares.airline = :airline and "
            "   :fare_basis like replace(fare_basis, '*', '%') and "
            "   brand_fares.airline = brands.airline and "
            "   brand_fares.brand = brands.code ",
            QParams() << QParam("airline", otString, airline)
                      << QParam("fare_basis", otString, fare_basis));
      brandQry.get().Execute();
      for(; not brandQry.get().Eof; brandQry.get().Next())
        i.first->second.push_back(brandQry.get().FieldAsInteger("id"));
    }
    else getsCached++;

    brandIds=i.first->second;
}

TBrand TBrands::getSingleBrand() const
{
  return brandIds.empty()?TBrand():TBrand(brandIds.front(), oper_airline);
}

void TBrands::traceCaching() const
{
  LogTrace(TRACE5) << "getsTotal: " << getsTotal << "; getsCached: " << getsCached;
}

std::string TBrand::name(const AstraLocale::OutputLang& lang) const
{
  if (id==ASTRA::NoExists) return "";
  return ElemIdToPrefferedElem(etBrand, id, efmtNameLong, lang.get());
}

const TBrand& TBrand::toWebXML(xmlNodePtr node,
                               const AstraLocale::OutputLang& lang) const
{
  if (node==NULL) return *this;
  if (id==ASTRA::NoExists) return *this;
  NewTextChild(node, "airline", airlineToPrefferedCode(oper_airline, lang));
  NewTextChild(node, "brand_code", ElemIdToPrefferedElem(etBrand, id, efmtCodeNative, lang.get()));
  NewTextChild(node, "name", ElemIdToPrefferedElem(etBrand, id, efmtNameLong, lang.get()));
  return *this;
}
