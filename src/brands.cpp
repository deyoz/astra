#include "brands.h"
#include "qrys.h"
#include "passenger.h"
#include "etick.h"

#define NICKNAME "DEN"
#include "serverlib/slogger.h"

using namespace std;

void TBrands::get(int pax_id)
{
    items.clear();
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
        if (tkn.validET()) {
            string ticket_no = paxQry.get().FieldAsString("ticket_no");
            int coupon_no = paxQry.get().FieldAsInteger("coupon_no");
            string airline = paxQry.get().FieldAsString("airline");
            TETickItem etick;
            etick.fromDB(ticket_no, coupon_no, TETickItem::Display, false);
            if(not etick.fare_basis.empty()) {
                return get(airline, etick.fare_basis);
            }
        }
    }
}

void TBrands::get(const std::string &airline, const std::string &fare_basis)
{
    oper_airline = airline;
    items.clear();
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
            QParams()
            << QParam("airline", otString, airline)
            << QParam("fare_basis", otString, fare_basis));
    brandQry.get().Execute();
    for(; not brandQry.get().Eof; brandQry.get().Next())
        items.push_back(brandQry.get().FieldAsInteger("id"));
}

bool TBrands::getBrand( TBrand &brand, std::string lang ) const
{
  brand.clear();
  if ( items.empty() ) {
    return false;
  }
  return getBrand( *items.begin(), brand, lang );
}

bool TBrands::getBrand( int id, TBrand &brand, std::string lang ) const
{
  bool res = false;
  brand.clear();
  for ( TItems::const_iterator item=items.begin(); item!=items.end(); item++ ) {
    if ( *item == id ) {
       brand.oper_airline = oper_airline;
       brand.code = ElemIdToElem(etBrand, id, efmtCodeNative, lang);
       brand.name = ElemIdToElem(etBrand, id, efmtNameLong, lang);
       res = true;
       break;
    }
  }
  ProgTrace( TRACE5, "getBrand return oper_airline=%s, code=%s, name=%s, res=%d", brand.oper_airline.c_str(), brand.code.c_str(), brand.name.c_str(), res );
  return res;
}

void TBrand::toXML( xmlNodePtr brandNode ) const
{
  if ( !oper_airline.empty() || !code.empty() || !name.empty() ) {
    NewTextChild( brandNode, "airline", oper_airline );
    NewTextChild( brandNode, "brand_code", code );
    NewTextChild( brandNode, "name", name );
  }
}
