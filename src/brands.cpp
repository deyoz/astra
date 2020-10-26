#include "brands.h"
#include "qrys.h"
#include "passenger.h"

#define NICKNAME "DEN"
#include "serverlib/slogger.h"

using namespace std;

void TBrands::get(int pax_id)
{
    clear();
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
            get(airline, TETickItem().fromDB(tkn.no, tkn.coupon, TETickItem::Display, false));
        }
    }
}

void TBrands::get(const std::string &airline, const TETickItem& etick)
{
    clear();
    if (airline.empty() ||
        etick.fare_basis.empty() ||
        etick.issue_date==ASTRA::NoExists) return;

    getsTotal++;

    const auto i = secretMap.emplace(make_pair(airline, etick.fare_basis), BrandIdsWithDateRanges());

    if (i.second)
    {
      TCachedQuery brandQry(
            "SELECT "
            "   brands.id, "
            "   brand_fares.sale_first_date, "
            "   brand_fares.sale_last_date, "
            "   LENGTH(:fare_basis)-REGEXP_COUNT(brand_fares.fare_basis, '[^*]') AS brand_priority "
            "FROM "
            "   brand_fares, "
            "   brands "
            "WHERE "
            "   brand_fares.airline = :airline AND "
            "   :fare_basis LIKE REPLACE(fare_basis, '*', '%') AND "
            "   brand_fares.airline = brands.airline AND "
            "   brand_fares.brand = brands.code "
            "ORDER BY brand_priority, brand_fares.brand",
            QParams() << QParam("airline", otString, airline)
                      << QParam("fare_basis", otString, etick.fare_basis));
      brandQry.get().Execute();
      for(; not brandQry.get().Eof; brandQry.get().Next())
      {
        const boost::optional<TDateTime>& first_date=
          brandQry.get().FieldIsNULL("sale_first_date")?
            boost::none:
            boost::optional<TDateTime>(brandQry.get().FieldAsDateTime("sale_first_date"));
        const boost::optional<TDateTime>& last_date=
          brandQry.get().FieldIsNULL("sale_last_date")?
            boost::none:
            boost::optional<TDateTime>(brandQry.get().FieldAsDateTime("sale_last_date"));

        i.first->second.emplace_back(brandQry.get().FieldAsInteger("id"),
                                     ASTRA::Range<TDateTime>(first_date, last_date));
      }
    }
    else getsCached++;

    for(const auto& j : i.first->second)
      if (j.second.contains(etick.issue_date)) emplace_back(j.first, airline);
}

TBrand TBrands::getSingleBrand() const
{
  return empty()?TBrand():front();
}

void TBrands::traceCaching() const
{
  LogTrace(TRACE5) << "getsTotal: " << getsTotal << "; getsCached: " << getsCached;
}

std::string TBrand::code() const
{
  if (id==ASTRA::NoExists) return "";
  return ElemIdToCodeNative(etBrand, id);
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
