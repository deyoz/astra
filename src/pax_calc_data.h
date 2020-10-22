#pragma once

#include <boost/optional.hpp>
#include <serverlib/cursctl.h>
#include "passenger.h"
#include "oralib.h"
#include "apis_utils.h"

class APIAttrs
{
  private:
    static const std::vector<std::string> columnNames;

    TAPIType apiType;
    std::string columnPrefix;
    std::string abbriviation;

    void setPrefixAndAbbriviation();

    template<class T>
    void set(const boost::optional<T>& crs, const boost::optional<T>& checkin)
    {
      if (crs) crsNotEmpty=crs.get().getNotEmptyFieldsMask(); else crsNotEmpty=boost::none;
      if (checkin) notEmpty=checkin.get().getNotEmptyFieldsMask(); else notEmpty=boost::none;
      if (crs && checkin) equal=crs.get().getEqualAttrsFieldsMask(checkin.get()); else equal=boost::none;
      if (checkin) scanned=checkin.get().scanned_attrs; else scanned=boost::none;
    }

    boost::optional<long int> crsNotEmpty;
    boost::optional<long int> notEmpty;
    boost::optional<long int> equal;
    boost::optional<long int> scanned;

    boost::optional<int> colCrsNotEmpty;
    boost::optional<int> colNotEmpty;
    boost::optional<int> colEqual;
    boost::optional<int> colScanned;
    boost::optional<int> colCrsDeleted;
  public:
    template<class T>
    APIAttrs(TAPIType type,
             const boost::optional<T>& crs,
             const boost::optional<T>& checkin) : apiType(type)
    {
      setPrefixAndAbbriviation();
      set(crs, checkin);
    }

    APIAttrs(TAPIType type, TQuery& Qry) : apiType(type)
    {
      setPrefixAndAbbriviation();
      get(Qry);
    }

    void get(TQuery& Qry);
    std::string view(bool paxNotRefused) const;

    class BindParam
    {
      public:
        std::string name;
        long int value;
        bool notNull;

        BindParam(const std::string& name_,
                  const boost::optional<long int>& value_) :
          name(name_), value(value_?value_.get():0), notNull(value_) {}
    };

    typedef std::vector<BindParam> BindParamContainer;

    BindParamContainer getBindParams() const;
    std::string insertIntoSQL() const;
    std::string insertValuesSQL() const;
    std::string updateValuesSQL() const;

    bool differsFromBooking(bool crsExists) const;
    bool incomplete(ASTRA::TPaxTypeExt paxTypeExt,
                    bool docoConfirmed,
                    const TCompleteAPICheckInfo &checkInfo) const;
    bool manualInput(ASTRA::TPaxTypeExt paxTypeExt,
                     const TCompleteAPICheckInfo &checkInfo) const;
};

class AllAPIAttrs
{
  private:
    std::vector<APIAttrs> attrs;
    void get(TQuery& Qry);

    bool init=false;
    int col_pax_id=-1;
    int col_grp_id=-1;
    int col_docs_crs_not_empty_attrs=-1;
    int col_docs_not_empty_attrs=-1;
    int col_crs_deleted=-1;
    boost::optional<bool> beforeDemarcationDate;
    void initColumnIndexes(TQuery& Qry);
  public:
    AllAPIAttrs(const TDateTime& scdOut);
    bool useOldAlgorithm()
    {
      return beforeDemarcationDate && beforeDemarcationDate.get();
    }

    std::string view(TQuery& Qry, bool paxNotRefused);
    std::set<APIS::TAlarmType> getAlarms(TQuery& Qry,
                                         bool apiDocApplied,
                                         ASTRA::TPaxTypeExt paxTypeExt,
                                         bool docoConfirmed,
                                         bool crsExists,
                                         const TCompleteAPICheckInfo &checkInfo,
                                         const std::set<APIS::TAlarmType> &requiredAlarms);
};

namespace PaxCalcData
{

enum class Changes { New, Cancel, Doc, Doco, Doca, Fqt, last=Fqt};

typedef PaxEventHolder<Changes> ChangesHolder;

void addChanges(const ModifiedPax& modifiedPax,
                ChangesHolder& changesHolder);
void addChanges(const ModifiedPaxRem& modifiedPaxRem,
                ChangesHolder& changesHolder);

void init_callbacks();

}

template <class TDOC>
std::string getDocsFlag( const TDOC &crs_pax_doc, const TDOC &pax_doc, bool pr_checkin, std::string flagStr, bool firstFlag )
{
  std::ostringstream res;
  if (!firstFlag) res << "/";
  if (crs_pax_doc.empty())
  {
    if (pax_doc.empty())
    {
      return "";
    }
    else
    {
      if ((pax_doc.scanned_attrs & pax_doc.getNotEmptyFieldsMask()) != pax_doc.getNotEmptyFieldsMask())
        res << "+"; // Добавили при регистрации в ручную
      else
        res << "*"; // Добавили при регистрации с помощью сканирования
    }
  }
  else // !crs_pax_doc.empty()
  {
    if (pax_doc.empty())
    {
      if (pr_checkin)
        res << "-"; // Удалили
    }
    else if (!crs_pax_doc.equalAttrs( pax_doc ))
    {
      if ((pax_doc.scanned_attrs & pax_doc.getNotEmptyFieldsMask()) != pax_doc.getNotEmptyFieldsMask())
        res << "#"; // Изменили в ручную
      else
        res << "="; // Изменение данных через сканирование
    }
  }
  res << flagStr;
  return res.str();
}
