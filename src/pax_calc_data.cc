#include "pax_calc_data.h"
#include "exceptions.h"
#include "passenger.h"
#include "alarms.h"
#include "tlg/typeb_db.h"

#define NICKNAME "VLAD"
#include "serverlib/slogger.h"

using namespace std;

const std::vector<std::string> APIAttrs::columnNames={"crs_not_empty_attrs",
                                                      "not_empty_attrs",
                                                      "equal_attrs",
                                                      "scanned_attrs"};

void APIAttrs::setPrefixAndAbbriviation()
{
  switch(apiType)
  {
      case apiDoc: columnPrefix="docs_";  abbriviation="S";  break;
     case apiDoco: columnPrefix="doco_";  abbriviation="O";  break;
    case apiDocaB: columnPrefix="docab_"; abbriviation="AB"; break;
    case apiDocaR: columnPrefix="docar_"; abbriviation="AR"; break;
    case apiDocaD: columnPrefix="docad_"; abbriviation="AD"; break;
          default: throw EXCEPTIONS::Exception("%s: apiType not supported", __func__);
  }
}

APIAttrs::BindParamContainer APIAttrs::getBindParams() const
{
  return { {":"+columnPrefix+"crs_not_empty_attrs", crsNotEmpty},
           {":"+columnPrefix+"not_empty_attrs",     notEmpty   },
           {":"+columnPrefix+"equal_attrs",         equal      },
           {":"+columnPrefix+"scanned_attrs",       scanned    } };
}

std::string APIAttrs::insertIntoSQL() const
{
  std::ostringstream sql;
  for(string::size_type i=0; i<columnNames.size(); ++i)
    sql << (i==0?"":", ") << columnPrefix << columnNames[i];
  return sql.str();
}

std::string APIAttrs::insertValuesSQL() const
{
  std::ostringstream sql;
  for(string::size_type i=0; i<columnNames.size(); ++i)
    sql << (i==0?"":", ") << ":" << columnPrefix << columnNames[i];
  return sql.str();
}

std::string APIAttrs::updateValuesSQL() const
{
  std::ostringstream sql;
  for(string::size_type i=0; i<columnNames.size(); ++i)
    sql << (i==0?"":", ") << columnPrefix << columnNames[i] << "=:" << columnPrefix << columnNames[i];
  return sql.str();
}

void APIAttrs::get(TQuery& Qry)
{
  if (!colCrsNotEmpty) colCrsNotEmpty=Qry.FieldIndex(columnPrefix+"crs_not_empty_attrs");
  if (!colNotEmpty) colNotEmpty=Qry.FieldIndex(columnPrefix+"not_empty_attrs");
  if (!colEqual) colEqual=Qry.FieldIndex(columnPrefix+"equal_attrs");
  if (!colScanned) colScanned=Qry.FieldIndex(columnPrefix+"scanned_attrs");
  if (!colCrsDeleted) colCrsDeleted=Qry.FieldIndex("crs_deleted");

  bool crsIgnore=Qry.FieldIsNULL(colCrsDeleted.get()) || Qry.FieldAsInteger(colCrsDeleted.get())!=0;

  if (!Qry.FieldIsNULL(colCrsNotEmpty.get()) && !crsIgnore)
    crsNotEmpty=Qry.FieldAsInteger(colCrsNotEmpty.get());
  else
    crsNotEmpty=boost::none;

  if (!Qry.FieldIsNULL(colNotEmpty.get()))
    notEmpty=Qry.FieldAsInteger(colNotEmpty.get());
  else
    notEmpty=boost::none;

  if (!Qry.FieldIsNULL(colEqual.get()) && !crsIgnore)
    equal=Qry.FieldAsInteger(colEqual.get());
  else
    equal=boost::none;

  if (!Qry.FieldIsNULL(colScanned.get()))
    scanned=Qry.FieldAsInteger(colScanned.get());
  else
    scanned=boost::none;
}

std::string APIAttrs::view(bool paxNotRefused) const
{
  std::ostringstream res;

  if (!crsNotEmpty || crsNotEmpty.get()==NO_FIELDS)
  {
    if (!notEmpty || notEmpty.get()==NO_FIELDS || !scanned)
    {
      return "";
    }
    else
    {
      if ((scanned.get() & notEmpty.get()) != notEmpty.get())
        res << "+"; // По крайней мере один непустой атрибут добавили при регистрации вручную
      else
        res << "*"; // Все непустные атрибуты добавили при регистрации сканированием
    }
  }
  else
  {
    if (!notEmpty || notEmpty.get()==NO_FIELDS || !equal || !scanned)
    {
      if (paxNotRefused)
        res << "-"; // Удалили
    }
    else if (crsNotEmpty.get()!=notEmpty.get() || (equal.get() & crsNotEmpty.get()) != crsNotEmpty.get())
    {
      //атрибуты бронирования и регистрации отличаются
      if ((scanned.get() & notEmpty.get()) != notEmpty.get())
        res << "#"; // По крайней мере один непустой атрибут изменили при регистрации вручную
      else
        res << "="; // Все непустные атрибуты изменили при регистрации сканированием
    }
  }

  res << abbriviation;
  return res.str();
}

bool APIAttrs::differsFromBooking(bool crsExists) const
{
  if (!crsExists) return false;

  long int equalFields=equal?equal.get():NO_FIELDS;
  long int crsNotEmptyFields=crsNotEmpty?crsNotEmpty.get():NO_FIELDS;

  return (equalFields & crsNotEmptyFields) != crsNotEmptyFields;
}

bool APIAttrs::incomplete(ASTRA::TPaxTypeExt paxTypeExt,
                          bool docoConfirmed,
                          const TCompleteAPICheckInfo &checkInfo) const
{
  long int notEmptyFields=notEmpty?notEmpty.get():NO_FIELDS;

  if (apiType==apiDoco && notEmptyFields==NO_FIELDS && docoConfirmed) return false;

  long int requiredFields=checkInfo.get(paxTypeExt).get(apiType).required_fields;
  return (notEmptyFields & requiredFields) != requiredFields;
}

bool APIAttrs::manualInput(ASTRA::TPaxTypeExt paxTypeExt,
                           const TCompleteAPICheckInfo &checkInfo) const
{
  if (apiType!=apiDoc) return false;

  long int notEmptyFields=notEmpty?notEmpty.get():NO_FIELDS;
  long int scannedFields=scanned?scanned.get():NO_FIELDS;

  long int requiredNotEmptyFields=checkInfo.get(paxTypeExt).get(apiType).required_fields & notEmptyFields;
  return (scannedFields & requiredNotEmptyFields) != requiredNotEmptyFields;
}

void AllAPIAttrs::get(TQuery& Qry)
{
  if (attrs.empty())
  {
    attrs.reserve(5);
    attrs.emplace_back(apiDoc, Qry);
    attrs.emplace_back(apiDoco, Qry);
    attrs.emplace_back(apiDocaB, Qry);
    attrs.emplace_back(apiDocaR, Qry);
    attrs.emplace_back(apiDocaD, Qry);
  }
  else
  {
    for(APIAttrs& i : attrs) i.get(Qry);
  }
}

std::string getDocsFlags( int pax_id, bool pr_checkin ); //!!! потом удалить

AllAPIAttrs::AllAPIAttrs(const TDateTime& scdOut)
{
  if (scdOut!=ASTRA::NoExists)
  {
    TDateTime demarcationDate;
    BASIC::date_time::StrToDateTime("15.08.2020", "dd.mm.yyyy", demarcationDate);
    beforeDemarcationDate=(scdOut<demarcationDate);
  }
}

void AllAPIAttrs::initColumnIndexes(TQuery& Qry)
{
  if (!init)
  {
    col_pax_id=Qry.GetFieldIndex("pax_id");
    col_grp_id=Qry.GetFieldIndex("grp_id");
    col_docs_crs_not_empty_attrs=Qry.GetFieldIndex("docs_crs_not_empty_attrs");
    col_docs_not_empty_attrs=Qry.GetFieldIndex("docs_not_empty_attrs");
    col_crs_deleted=Qry.GetFieldIndex("crs_deleted");
    init=true;
  }
}

std::string AllAPIAttrs::view(TQuery& Qry, bool paxNotRefused)
{
  initColumnIndexes(Qry);

  if (useOldAlgorithm())
  {
    return getDocsFlags( Qry.FieldAsInteger(col_pax_id), !Qry.FieldIsNULL(col_grp_id) );
  }

  //!!! все что выше потом удалить

  get(Qry);

  static const char separator='/';

  std::string res;

  for(const APIAttrs& i : attrs)
  {
    res+=i.view(paxNotRefused);
    if (!res.empty() && res.back()!=separator) res+=separator;
  }
  if (!res.empty() && res.back()==separator) res.pop_back();

  return res;
}

std::set<APIS::TAlarmType> AllAPIAttrs::getAlarms(TQuery& Qry,
                                                  bool apiDocApplied,
                                                  ASTRA::TPaxTypeExt paxTypeExt,
                                                  bool docoConfirmed,
                                                  bool crsExists,
                                                  const TCompleteAPICheckInfo &checkInfo,
                                                  const std::set<APIS::TAlarmType> &requiredAlarms)
{
  initColumnIndexes(Qry);

  if ((Qry.FieldIsNULL(col_docs_crs_not_empty_attrs) &&
       Qry.FieldIsNULL(col_docs_not_empty_attrs)) ||
      (!Qry.FieldIsNULL(col_docs_crs_not_empty_attrs) &&
       Qry.FieldIsNULL(col_crs_deleted)))
  {
    return {};
  }

  //!!! все что выше потом удалить

  std::set<APIS::TAlarmType> result;

  if (!apiDocApplied) return result;

  get(Qry);

  if (requiredAlarms.count(APIS::atDiffersFromBooking)>0)
    for(const APIAttrs& i : attrs)
      if (i.differsFromBooking(crsExists))
      {
        result.insert(APIS::atDiffersFromBooking);
        break;
      }

  if (requiredAlarms.count(APIS::atIncomplete)>0)
    for(const APIAttrs& i : attrs)
      if (i.incomplete(paxTypeExt, docoConfirmed, checkInfo))
      {
        result.insert(APIS::atIncomplete);
        break;
      }

  if (requiredAlarms.count(APIS::atManualInput)>0)
    for(const APIAttrs& i : attrs)
      if (i.manualInput(paxTypeExt, checkInfo))
      {
        result.insert(APIS::atManualInput);
        break;
      }

  return result;
}

namespace PaxCalcData
{

void addChanges(const ModifiedPax& modifiedPax,
                ChangesHolder& changesHolder)
{
  for(const PaxIdWithSegmentPair& paxId : modifiedPax.getPaxIds(PaxChanges::New))
    changesHolder.add(Changes::New, paxId);
  for(const PaxIdWithSegmentPair& paxId : modifiedPax.getPaxIds(PaxChanges::Cancel))
    changesHolder.add(Changes::Cancel, paxId);
}

void addChanges(const ModifiedPaxRem& modifiedPaxRem,
                ChangesHolder& changesHolder)
{
  for(const PaxIdWithSegmentPair& paxId : modifiedPaxRem.getPaxIds(remDOC))
    changesHolder.add(Changes::Doc, paxId);
  for(const PaxIdWithSegmentPair& paxId : modifiedPaxRem.getPaxIds(remDOCO))
    changesHolder.add(Changes::Doco, paxId);
  for(const PaxIdWithSegmentPair& paxId : modifiedPaxRem.getPaxIds(remDOCA))
    changesHolder.add(Changes::Doca, paxId);
  for(const PaxIdWithSegmentPair& paxId : modifiedPaxRem.getPaxIds(remFQT))
    changesHolder.add(Changes::Fqt, paxId);
}

//важно, что мы делаем апдейты по паксам, сортированным по paxId, иначе возможны дэдлоки (вызовы onChange в порядке сортировки по PaxId)
//именно из-за дэдлоков мы сначала собираем в общий контейнер разнотипные ивенты TRemCategory и PaxChanges, а потом уже применяем в базу

class PaxEvents: public PaxEventCallbacks<Changes>
{
  private:
    void onChange(TRACE_SIGNATURE,
                  const PaxOrigin& paxOrigin,
                  const PaxIdWithSegmentPair& paxId,
                  const std::set<Changes>& changes);

    static boost::optional<std::string> updatePLSQL(const std::vector<APIAttrs>& api,
                                                    const boost::optional<bool>& crsDeleted,
                                                    const boost::optional<string>& crsDocNo,
                                                    const boost::optional<std::string>& crsFqtTierLevel);
};

void PaxEvents::onChange(TRACE_SIGNATURE,
                         const PaxOrigin& paxOrigin,
                         const PaxIdWithSegmentPair& paxId,
                         const std::set<Changes>& changes)
{
  std::vector<APIAttrs> attrs;
  bool syncDoc, syncDoco, syncDoca, checkinCancelled;
  bool newCategoryExists=changes.count(Changes::New)>0;
  bool cancelCategoryExists=changes.count(Changes::Cancel)>0;

  if (paxOrigin==paxCheckIn && newCategoryExists!=cancelCategoryExists)
  {
    syncDoc=true;
    syncDoco=true;
    syncDoca=true;
    checkinCancelled=cancelCategoryExists;
  }
  else
  {
    syncDoc= changes.count(Changes::Doc)>0;
    syncDoco=changes.count(Changes::Doco)>0;
    syncDoca=changes.count(Changes::Doca)>0;
    checkinCancelled=false;
  }

  if (syncDoc)
  {
    attrs.emplace_back(apiDoc,
                       CheckIn::TPaxDocItem::get(paxPnl, paxId()),
                       !checkinCancelled?CheckIn::TPaxDocItem::get(paxCheckIn, paxId()):boost::none);
  }
  if (syncDoco)
  {
    attrs.emplace_back(apiDoco,
                       CheckIn::TPaxDocoItem::get(paxPnl, paxId()),
                       !checkinCancelled?CheckIn::TPaxDocoItem::get(paxCheckIn, paxId()):boost::none);
  }
  if (syncDoca)
  {
    CheckIn::TDocaMap crs=CheckIn::TDocaMap::get(paxPnl, paxId());
    CheckIn::TDocaMap checkin;
    if (!checkinCancelled) checkin=CheckIn::TDocaMap::get(paxCheckIn, paxId());

    attrs.emplace_back(apiDocaB, crs.get(apiDocaB), checkin.get(apiDocaB));
    attrs.emplace_back(apiDocaR, crs.get(apiDocaR), checkin.get(apiDocaR));
    attrs.emplace_back(apiDocaD, crs.get(apiDocaD), checkin.get(apiDocaD));
  }

  boost::optional<bool> crsDeleted;
  if (paxOrigin==paxPnl && newCategoryExists!=cancelCategoryExists)
  {
    crsDeleted=cancelCategoryExists;
  }

  boost::optional<std::string> crsDocNo;
  if (paxOrigin==paxPnl && changes.count(Changes::Doc)>0)
  {
    crsDocNo=TypeB::getPSPT(paxId().get(), true /*with_issue_country*/, TReqInfo::Instance()->desk.lang);
  }

  boost::optional<std::string> crsFqtTierLevel;
  if (paxOrigin==paxPnl && changes.count(Changes::Fqt)>0)
  {
    auto item=CheckIn::TPaxFQTItem::getNotEmptyTierLevel(paxPnl, paxId(), true);
    crsFqtTierLevel=item?item.get().tier_level:"";
  }

  boost::optional<std::string> sql=updatePLSQL(attrs, crsDeleted, crsDocNo, crsFqtTierLevel);
  if (!sql) return; //ничего не меняем

  auto cur=make_curs(sql.get());
  cur.bind(":pax_calc_data_id", paxId().get());
  if (crsDeleted)
    cur.bind(":crs_deleted", crsDeleted.get());
  if (crsDocNo)
    cur.bind(":crs_doc_no", crsDocNo.get());
  if (crsFqtTierLevel)
    cur.bind(":crs_fqt_tier_level", crsFqtTierLevel.get());

  short null = -1, nnull = 0;
  APIAttrs::BindParamContainer bindParams;
  for(const APIAttrs& i : attrs)
    algo::append(bindParams, i.getBindParams());

  for(const auto& param : bindParams)
    cur.bind(param.name, param.value, param.notNull?&nnull:&null);

  //обязательно значения всех параметров запроса в контейнерах и переменных должены быть актуальны к моменту exec()
  cur.exec();

  if (paxOrigin==paxCheckIn && paxId.getSegmentPair())
    TTripAlarmHook::set(Alarm::APISControl, paxId.getSegmentPair().get().point_dep);
}

boost::optional<std::string> PaxEvents::updatePLSQL(const std::vector<APIAttrs>& attrs,
                                                    const boost::optional<bool>& crsDeleted,
                                                    const boost::optional<std::string>& crsDocNo,
                                                    const boost::optional<std::string>& crsFqtTierLevel)
{
  std::string insertIntoSQL;
  std::string insertValuesSQL;
  std::string updateValuesSQL;
  bool firstIteration=true;

  if (crsDeleted)
  {
    insertIntoSQL+="crs_deleted";
    insertValuesSQL+=":crs_deleted";
    updateValuesSQL+="crs_deleted=:crs_deleted";
    firstIteration=false;
  }

  for(const APIAttrs& i : attrs)
  {
    if (!firstIteration)
    {
      insertIntoSQL+=", \n";
      insertValuesSQL+=", \n";
      updateValuesSQL+=", \n";
    }
    insertIntoSQL+=i.insertIntoSQL();
    insertValuesSQL+=i.insertValuesSQL();
    updateValuesSQL+=i.updateValuesSQL();
    firstIteration=false;
  }

  if (crsDocNo) {
    if (!firstIteration)
    {
      insertIntoSQL+=", \n";
      insertValuesSQL+=", \n";
      updateValuesSQL+=", \n";
    }
    insertIntoSQL+="crs_doc_no";
    insertValuesSQL+=":crs_doc_no";
    updateValuesSQL+="crs_doc_no=:crs_doc_no";
    firstIteration=false;
  }

  if (crsFqtTierLevel)
  {
    if (!firstIteration)
    {
      insertIntoSQL+=", \n";
      insertValuesSQL+=", \n";
      updateValuesSQL+=", \n";
    }
    insertIntoSQL+="crs_fqt_tier_level";
    insertValuesSQL+=":crs_fqt_tier_level";
    updateValuesSQL+="crs_fqt_tier_level=:crs_fqt_tier_level";
    firstIteration=false;
  }

  if (firstIteration) return boost::none; //ничего не меняется в pax_calc_data

  ostringstream sql;
  sql << "DECLARE \n"
         "  pass BINARY_INTEGER; \n"
         "BEGIN \n"
         "  FOR pass IN 1..2 LOOP \n"
         "    UPDATE pax_calc_data \n"
         "    SET " << updateValuesSQL << " \n"
         "    WHERE pax_calc_data_id=:pax_calc_data_id; \n"
         "    IF SQL%FOUND THEN EXIT; END IF; \n"
         "    BEGIN \n"
         "      INSERT INTO pax_calc_data(pax_calc_data_id, " << insertIntoSQL << ") \n"
         "      VALUES (:pax_calc_data_id, " << insertValuesSQL << "); \n"
         "      EXIT; \n"
         "    EXCEPTION \n"
         "      WHEN DUP_VAL_ON_INDEX THEN \n"
         "        IF pass=1 THEN NULL; ELSE RAISE; END IF; \n"
         "    END; \n"
         "  END LOOP; \n"
         "END; \n";

  return sql.str();
}

void init_callbacks()
{
  static bool init=false;
  if (init) return;

  CallbacksSuite< PaxEventCallbacks<Changes> >::Instance()->addCallbacks(new PaxEvents);
  init=true;
}

} //namespace PaxCalcData

std::string getDocsFlags( int pax_id, bool pr_checkin ) //!!! потом удалить
{
  //ProgTrace( TRACE5, "getDocsFlags: pax_id=%d, pr_checkin=%d", pax_id, pr_checkin);
  std::string res;
  CheckIn::TPaxDocItem pax_doc, crs_pax_doc;
  CheckIn::TPaxDocoItem pax_doco, crs_pax_doco;
  CheckIn::LoadCrsPaxDoc(pax_id,crs_pax_doc);
  if ( pr_checkin ) {
    CheckIn::LoadPaxDoc(pax_id,pax_doc);
  }
  res += getDocsFlag( crs_pax_doc, pax_doc, pr_checkin, "S", res.empty() );
  CheckIn::LoadCrsPaxVisa(pax_id,crs_pax_doco);
  if ( pr_checkin ) {
    CheckIn::LoadPaxDoco(pax_id,pax_doco);
  }
  res += getDocsFlag( crs_pax_doco, pax_doco, pr_checkin, "O", res.empty() );
  CheckIn::TDocaMap doca_map, crs_doca_map;
  CheckIn::LoadCrsPaxDoca(pax_id,crs_doca_map);
  if ( pr_checkin ) {
    CheckIn::LoadPaxDoca(pax_id,doca_map);
  }
  CheckIn::TPaxDocaItem pax_doca, crs_pax_doca;
  if ( doca_map.find( apiDocaB ) != doca_map.end() ||
       !doca_map[ apiDocaB ].empty() ) {
    pax_doca = doca_map[ apiDocaB ];
  }
  if ( crs_doca_map.find( apiDocaB ) != crs_doca_map.end() ||
       !crs_doca_map[ apiDocaB ].empty() ) {
    crs_pax_doca = crs_doca_map[ apiDocaB ];
  }
  res += getDocsFlag( crs_pax_doca, pax_doca, pr_checkin, "AB", res.empty() );
  pax_doca.clear();
  crs_pax_doca.clear();
  if ( doca_map.find( apiDocaR ) != doca_map.end() ||
       !doca_map[ apiDocaR ].empty() ) {
    pax_doca = doca_map[ apiDocaR ];
  }
  if ( crs_doca_map.find( apiDocaR ) != crs_doca_map.end() ||
       !crs_doca_map[ apiDocaR ].empty() ) {
    crs_pax_doca = crs_doca_map[ apiDocaR ];
  }
  res += getDocsFlag( crs_pax_doca, pax_doca, pr_checkin, "AR", res.empty() );
  pax_doca.clear();
  crs_pax_doca.clear();
  if ( doca_map.find( apiDocaD ) != doca_map.end() ||
       !doca_map[ apiDocaD ].empty() ) {
    pax_doca = doca_map[ apiDocaD ];
  }
  if ( crs_doca_map.find( apiDocaD ) != crs_doca_map.end() ||
       !crs_doca_map[ apiDocaD ].empty() ) {
    crs_pax_doca = crs_doca_map[ apiDocaD ];
  }
  res += getDocsFlag( crs_pax_doca, pax_doca, pr_checkin, "AD", res.empty() );
  return res;
}
