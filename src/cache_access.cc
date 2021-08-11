#include "cache_access.h"

#include "astra_locale.h"
#include "PgOraConfig.h"
#include "exceptions.h"

#define NICKNAME "VLAD"
#include "serverlib/slogger.h"

using namespace std;
using namespace AstraLocale;
using namespace EXCEPTIONS;

namespace CacheTable
{

std::string getSQLFilter(const std::string& sqlFieldName, const AccessControl accessControl)
{
  const auto& access=accessControl==AccessControl::PermittedAirlines ||
                     accessControl==AccessControl::PermittedAirlinesOrNull ?
                     TReqInfo::Instance()->user.access.airlines():
                     TReqInfo::Instance()->user.access.airps();

  if (access.totally_permitted()) return " (1=1) ";
  if (access.totally_not_permitted()) return " (1<>1) ";

  string where1, where2;

  if (!access.elems().empty())
    where1 = sqlFieldName + (access.elems_permit()?" IN ":" NOT IN ") + GetSQLEnum(access.elems());

  if (accessControl==AccessControl::PermittedAirlinesOrNull ||
      accessControl==AccessControl::PermittedAirportsOrNull)
    where2 = sqlFieldName + " IS NULL";

  if (where1.empty())
  {
    if (where2.empty())
      return " (1<>1) ";
    else
      return " (" + where2 + ") ";
  }
  else
  {
    if (where2.empty())
      return " (" + where1 + ") ";
    else
      return " (" + where1 + " OR " + where2 + ") ";
  }
}

bool isPermitted(const std::optional<AirportCode_t>& airportOpt)
{
  if (airportOpt)
    return TReqInfo::Instance()->user.access.airps().permitted(airportOpt.value().get());
  else
    return TReqInfo::Instance()->user.access.airps().totally_permitted();
}

bool isPermitted(const std::optional<AirlineCode_t>& airlineOpt)
{
  if (airlineOpt)
    return TReqInfo::Instance()->user.access.airlines().permitted(airlineOpt.value().get());
  else
    return TReqInfo::Instance()->user.access.airlines().totally_permitted();
}

void checkAirportAccess(const std::string& fieldName,
                        const std::optional<CacheTable::Row>& oldRow,
                        const std::optional<CacheTable::Row>& newRow)
{
  for(bool isNewRow : {false, true})
  {
    const std::optional<CacheTable::Row>& row = isNewRow?newRow:oldRow;
    if (!row) continue;

    string value=row.value().getAsString(fieldName);
    std::optional<AirportCode_t> airportOpt;
    if (!value.empty()) airportOpt.emplace(value);

    if (isPermitted(airportOpt)) continue;

    if (airportOpt) {
      throw UserException(isNewRow?"MSG.NO_PERM_ENTER_AIRPORT":"MSG.NO_PERM_MODIFY_AIRPORT",
                          LParams() << LParam("airp", ElemIdToCodeNative(etAirp, airportOpt.value().get())));
    }
    else {
      throw UserException(isNewRow?"MSG.NEED_SET_CODE_AIRP":"MSG.NO_PERM_MODIFY_INDEFINITE_AIRPORT");
    }
  }
}

void checkAirlineAccess(const std::string& fieldName,
                        const std::optional<CacheTable::Row>& oldRow,
                        const std::optional<CacheTable::Row>& newRow)
{
  for(bool isNewRow : {false, true})
  {
    const std::optional<CacheTable::Row>& row = isNewRow?newRow:oldRow;
    if (!row) continue;

    string value=row.value().getAsString(fieldName);
    std::optional<AirlineCode_t> airlineOpt;
    if (!value.empty()) airlineOpt.emplace(value);

    if (isPermitted(airlineOpt)) continue;

    if (airlineOpt) {
      throw UserException(isNewRow?"MSG.NO_PERM_ENTER_AIRLINE":"MSG.NO_PERM_MODIFY_AIRLINE",
                          LParams() << LParam("airline", ElemIdToCodeNative(etAirline, airlineOpt.value().get())));
    }
    else {
      throw UserException(isNewRow?"MSG.NEED_SET_CODE_AIRLINE":"MSG.NO_PERM_MODIFY_INDEFINITE_AIRLINE");
    }
  }
}

//�஢�ઠ �ࠢ ࠡ��� � �⠯��

void checkNotNullStageAccess(const std::string& stageIdFieldName,
                             const std::string& airlineFieldName,
                             const std::string& airportFieldName,
                             const std::optional<CacheTable::Row>& oldRow,
                             const std::optional<CacheTable::Row>& newRow)
{
  checkAirlineAccess(airlineFieldName, oldRow, newRow);
  checkAirportAccess(airportFieldName, oldRow, newRow);

  for(bool isNewRow : {false, true})
  {
    const std::optional<CacheTable::Row>& row = isNewRow?newRow:oldRow;
    if (!row) continue;

    std::optional<int> valueOpt=row.value().getAsInteger(stageIdFieldName);
    if (!valueOpt)
      throw Exception("%s: empty field '%s'", __func__, stageIdFieldName.c_str());

    bool isAirpStage=dynamic_cast<const TGraphStagesRow&>(base_tables.get("graph_stages").get_row("id",valueOpt.value())).pr_airp_stage;

    if (row.value().getAsString(airlineFieldName).empty() &&
        TReqInfo::Instance()->user.user_type==utAirport && !isAirpStage)
      throw UserException(isNewRow?"MSG.NEED_SET_CODE_AIRLINE":"MSG.NO_PERM_MODIFY_INDEFINITE_AIRLINE");

    if (row.value().getAsString(airportFieldName).empty() &&
        TReqInfo::Instance()->user.user_type==utAirline && isAirpStage)
      throw UserException(isNewRow?"MSG.NEED_SET_CODE_AIRP":"MSG.NO_PERM_MODIFY_INDEFINITE_AIRPORT");
  }
}

template<>
void Access<DeskGrpId_t>::downloadPermissions()
{
  ostringstream sql;
  sql << "SELECT grp_id FROM desk_grp WHERE "
      << getSQLFilter("airline", AccessControl::PermittedAirlines) << " AND "
      << getSQLFilter("airp",    AccessControl::PermittedAirports);

  if (id_) sql << " AND grp_id=:grp_id";


  auto cur = make_db_curs(sql.str(), PgOra::getROSession("DESK_GRP"));


  if (id_) cur.bind(":grp_id", id_.value().get());

  int grpId;
  cur.def(grpId)
     .exec();

  while (!cur.fen()) permitted.emplace(grpId);
}

template<>
void Access<DeskCode_t>::downloadPermissions()
{
  ostringstream sql;
  sql << "SELECT desks.code FROM desk_grp, desks WHERE desk_grp.grp_id=desks.grp_id AND "
      << getSQLFilter("desk_grp.airline", AccessControl::PermittedAirlines) << " AND "
      << getSQLFilter("desk_grp.airp",    AccessControl::PermittedAirports);

  if (id_) sql << " AND desks.code=:desk";


  auto cur = make_db_curs(sql.str(), PgOra::getROSession({"DESK_GRP", "DESKS"}));


  if (id_) cur.bind(":desk", id_.value().get());

  std::string desk;
  cur.def(desk)
     .exec();

  while (!cur.fen()) permitted.emplace(desk);
}

template<>
void ViewAccess<DeskGrpId_t>::downloadPermissions()
{
  ostringstream sql;
  sql << "SELECT DISTINCT desks.grp_id FROM desk_grp, desks, desk_owners "
         "WHERE desk_grp.grp_id=desks.grp_id AND "
         "      desks.code=desk_owners.desk AND "
         "      desk_owners.airline IS NOT NULL AND "
      << getSQLFilter("desk_owners.airline", AccessControl::PermittedAirlines) << " AND "
      << getSQLFilter("desk_grp.airp",       AccessControl::PermittedAirports);

  if (id_) sql << " AND desk_grp.grp_id=:grp_id";


  auto cur = make_db_curs(sql.str(), PgOra::getROSession({"DESK_GRP", "DESKS", "DESK_OWNERS"}));

  if (id_) cur.bind(":grp_id", id_.value().get());

  int grpId;
  cur.def(grpId)
     .exec();

  permitted.emplace();

  while (!cur.fen()) permitted.value().emplace(grpId);
}

template<>
void ViewAccess<DeskCode_t>::downloadPermissions()
{
  ostringstream sql;
  sql << "SELECT DISTINCT desks.code FROM desk_grp, desks, desk_owners "
         "WHERE desk_grp.grp_id=desks.grp_id AND "
         "      desks.code=desk_owners.desk AND "
         "      desk_owners.airline IS NOT NULL AND "
      << getSQLFilter("desk_owners.airline", AccessControl::PermittedAirlines) << " AND "
      << getSQLFilter("desk_grp.airp",       AccessControl::PermittedAirports);

  if (id_) sql << " AND desk_owners.desk=:desk";


  auto cur = make_db_curs(sql.str(), PgOra::getROSession({"DESK_GRP", "DESKS", "DESK_OWNERS"}));

  if (id_) cur.bind(":desk", id_.value().get());

  std::string desk;
  cur.def(desk)
     .exec();

  permitted.emplace();

  while (!cur.fen()) permitted.value().emplace(desk);
}



//�஢�ઠ �ࠢ ࠡ��� � ��㯯�� ���⮢

void checkNotNullDeskGrpAccess(const std::string& deskGrpIdFieldName,
                               const std::optional<CacheTable::Row>& oldRow,
                               const std::optional<CacheTable::Row>& newRow)
{
  for(bool isNewRow : {false, true})
  {
    const std::optional<CacheTable::Row>& row = isNewRow?newRow:oldRow;
    if (!row) continue;

    std::optional<int> valueOpt=row.value().getAsInteger(deskGrpIdFieldName);
    if (!valueOpt)
      throw Exception("%s: empty field '%s'", __func__, deskGrpIdFieldName.c_str());

    DeskGrpId_t deskGrpId(valueOpt.value());

    if (!Access(deskGrpId).check(deskGrpId))
      throw UserException(isNewRow?"MSG.ACCESS.NO_PERM_ENTER_DESK_GRP":"MSG.ACCESS.NO_PERM_MODIFY_DESK_GRP"); //!!! ��� �� � ᮮ�饭�� �뢮���� �������� ��㯯�
  }
}

void checkNullableDeskAccess(const std::string& deskFieldName,
                             const std::optional<CacheTable::Row>& oldRow,
                             const std::optional<CacheTable::Row>& newRow)
{
  for(bool isNewRow : {false, true})
  {
    const std::optional<CacheTable::Row>& row = isNewRow?newRow:oldRow;
    if (!row) continue;

    std::string value=row.value().getAsString(deskFieldName);
    if (value.empty()) continue;

    DeskCode_t deskCode(value);

    if (!Access(deskCode).check(deskCode))
      throw UserException(isNewRow?"MSG.ACCESS.NO_PERM_ENTER_DESK":"MSG.ACCESS.NO_PERM_MODIFY_DESK",
                          LParams() << LParam("desk", deskCode.get()));
  }
}

void checkDeskAndDeskGrp(const std::string& deskFieldName,
                         const std::string& deskGrpIdFieldName,
                         std::optional<CacheTable::Row>& row)
{
  if (!row) return;

  std::string desk=row.value().getAsString(deskFieldName);
  std::optional<int> deskGrpIdOpt=row.value().getAsInteger(deskGrpIdFieldName);
  if (desk.empty())
  {
    if (!deskGrpIdOpt)
      throw UserException("MSG.NEED_SET_DESK_OR_DESK_GRP");
    return;
  }

  auto cur=make_db_curs("SELECT grp_id FROM desks WHERE code=:code",
                        PgOra::getROSession("DESKS"));

  int grpId;
  cur.bind(":code", desk)
     .def(grpId)
     .EXfet();

  if(cur.err() == DbCpp::ResultCode::NoDataFound)
    throw UserException("MSG.INVALID_DESK");

  if (deskGrpIdOpt && deskGrpIdOpt.value()!=grpId)
    throw UserException("MSG.DESK_GRP_DOES_NOT_MEET_DESK");

  row.value().setFromInteger(deskGrpIdFieldName, grpId);
}

} //namespace CacheTable

