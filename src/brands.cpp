#include "brands.h"
#include "qrys.h"
#include "passenger.h"

#define NICKNAME "DEN"
#include "serverlib/slogger.h"

using namespace std;

void TBrands::get(int pax_id)
{
    clear();
    DB::TCachedQuery paxQry(
          PgOra::getROSession({"PAX","PAX_GRP","POINTS"}),
          "SELECT "
          "   ticket_no, "
          "   coupon_no, "
          "   ticket_rem, "
          "   ticket_confirm, "
          "   points.airline "
          "FROM "
          "   pax, "
          "   pax_grp, "
          "   points "
          "WHERE "
          "   pax_id = :pax_id and "
          "   pax.grp_id = pax_grp.grp_id and "
          "   pax_grp.point_dep = points.point_id ",
          QParams() << QParam("pax_id", otInteger, pax_id),
          STDLOG);
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

static int countNotAsterisk(const std::string& text)
{
  int result = 0;
  for (size_t i = 0; i < text.size(); i++) {
    if (text.at(i) == '*') {
      continue;
    }
    result++;
  }
  return result;
}

int calcBrandPriority(const std::string& fare_basis)
{
  return (countNotAsterisk(fare_basis) * (-1));
}

static bool compareBrands(const BrandIdWithDateRange& b1, const BrandIdWithDateRange& b2)
{
  if (b1.priority != b2.priority) {
    return b1.priority < b2.priority;
  }
  return b1.brand < b2.brand;
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
      DB::TCachedQuery brandQry(
            PgOra::getROSession({"BRAND_FARES","BRANDS"}),
            "SELECT "
            "   brands.id, "
            "   brand_fares.sale_first_date, "
            "   brand_fares.sale_last_date, "
            "   brand_fares.fare_basis, "
            "   brand_fares.brand "
            "FROM "
            "   brand_fares, "
            "   brands "
            "WHERE "
            "   brand_fares.airline = :airline AND "
            "   :fare_basis LIKE REPLACE(fare_basis, '*', '%') AND "
            "   brand_fares.airline = brands.airline AND "
            "   brand_fares.brand = brands.code "
            "ORDER BY brand_fares.brand ",
            QParams() << QParam("airline", otString, airline)
                      << QParam("fare_basis", otString, etick.fare_basis),
            STDLOG);
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

        const int id = brandQry.get().FieldAsInteger("id");
        const ASTRA::Range<TDateTime> date_range(first_date, last_date);
        const std::string fare_basis = brandQry.get().FieldAsString("fare_basis");
        const std::string brand = brandQry.get().FieldAsString("brand");
        i.first->second.push_back(BrandIdWithDateRange {
                                    id,
                                    date_range,
                                    calcBrandPriority(fare_basis),
                                    brand
                                  });
      }
      i.first->second.sort(compareBrands);
    }
    else getsCached++;


    for(const auto& j : i.first->second)
      if (j.date_range.contains(etick.issue_date)) emplace_back(j.id , airline);
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

std::ostream& operator<<(std::ostream& os, const TBrand::Key& brand)
{
  os << brand.airlineOper << ":" << brand.code;
  return os;
}

void checkBrandFareDates(int id, TDateTime first_date, TDateTime last_date,
                            const string& airline, const string& fare_basis)
{
  checkDateRange(first_date, last_date);

  QParams qryParams;
  qryParams << QParam("airline", otString, airline)
            << QParam("fare_basis", otString, fare_basis);
  DB::TCachedQuery Qry(
        PgOra::getROSession("BRAND_FARES"),
        "SELECT id, sale_first_date, sale_last_date "
        "FROM brand_fares "
        "WHERE airline=:airline AND fare_basis=:fare_basis ",
        qryParams,
        STDLOG);
  Qry.get().Execute();

  for(; !Qry.get().Eof; Qry.get().Next()) {
    const int brand_fare_id = Qry.get().FieldAsInteger("id");
    if (id == brand_fare_id) {
      continue;
    }
    const TDateTime prev_first_date = Qry.get().FieldIsNULL("sale_first_date") ? ASTRA::NoExists
                                                                               : Qry.get().FieldAsDateTime("sale_first_date");
    const TDateTime prev_last_date = Qry.get().FieldIsNULL("sale_last_date") ? ASTRA::NoExists
                                                                             : Qry.get().FieldAsDateTime("sale_last_date");
    checkPeriodOverlaps(first_date, last_date, prev_first_date, prev_last_date);
  }
}

void insertBrandFare(int id, TDateTime first_datetime, TDateTime last_datetime,
                     const string& airline, const string& fare_basis, const string& brand)
{
  TDateTime first_date = ASTRA::NoExists;
  TDateTime last_date  = ASTRA::NoExists;
  dateTimeToDatePeriod(first_datetime, last_datetime,
                       first_date, last_date);
  checkBrandFareDates(id, first_date, last_date, airline, fare_basis);
  QParams qryParams;
  qryParams << QParam("airline", otString, airline)
            << QParam("fare_basis", otString, fare_basis)
            << QParam("first_date", otDate, first_date);
  if (last_date == ASTRA::NoExists) {
    qryParams << QParam("last_date", otDate, FNull);
  } else {
    qryParams << QParam("last_date", otDate, last_date + 1);
  }
  qryParams << QParam("brand", otString, brand);
  if (id == ASTRA::NoExists) {
    id = PgOra::getSeqNextVal_int("ID__SEQ");
  }
  DB::TCachedQuery insQry(
        PgOra::getRWSession("BRAND_FARES"),
        "INSERT INTO brand_fares(id, airline, fare_basis, brand, sale_first_date, sale_last_date) "
        "VALUES(:id, :airline, :fare_basis, :brand, :first_date, :last_date) ",
        qryParams << QParam("id", otInteger, id),
        STDLOG);
  insQry.get().Execute();
}

void updateBrandFare(int id, TDateTime first_datetime, TDateTime last_datetime,
                     const string& airline, const string& fare_basis, const string& brand)
{
  TDateTime first_date = ASTRA::NoExists;
  TDateTime last_date  = ASTRA::NoExists;
  dateTimeToDatePeriod(first_datetime, last_datetime,
                       first_date, last_date);
  checkBrandFareDates(id, first_date, last_date, airline, fare_basis);
  QParams qryParams;
  qryParams << QParam("airline", otString, airline)
            << QParam("fare_basis", otString, fare_basis)
            << QParam("first_date", otDate, first_date);
  if (last_date == ASTRA::NoExists) {
    qryParams << QParam("last_date", otDate, FNull);
  } else {
    qryParams << QParam("last_date", otDate, last_date + 1);
  }
  qryParams << QParam("brand", otString, brand);
  DB::TCachedQuery updQry(
        PgOra::getRWSession("BRAND_FARES"),
        "UPDATE brand_fares "
        "SET airline=:airline, fare_basis=:fare_basis, brand=:brand, "
        "    sale_first_date=:first_date, sale_last_date=:last_date "
        "WHERE id=:id ",
        qryParams << QParam("id", otInteger, id),
        STDLOG);
  updQry.get().Execute();
}
