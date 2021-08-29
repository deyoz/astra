#include "cache_access.h"

#include "astra_locale.h"
#include "PgOraConfig.h"
#include "exceptions.h"

#define NICKNAME "VLAD"
#include "serverlib/slogger.h"

using namespace std;
using namespace AstraLocale;
using namespace EXCEPTIONS;

TAccessElems<CityCode_t> getCitiesAccess(const TAccessElems<std::string>& airpsAccess)
{
  TAccessElems<CityCode_t> citiesAccess;

  if (!airpsAccess.elems_permit())
    citiesAccess.set_total_permit();
  else
  {
    const auto& airpsAccessElems=airpsAccess.elems();
    for(const auto& airp : airpsAccessElems)
      citiesAccess.add_elem(getCityByAirp(AirportCode_t(airp)));
  }

  return citiesAccess;
}



template<class T>
std::string getSQLFilter(const std::string& sqlFieldName, const TAccessElems<T>& access, const bool addNulls)
{
  if (access.totally_permitted()) return " (1=1) ";
  if (access.totally_not_permitted()) return " (1<>1) ";

  string where1, where2;

  if (!access.elems().empty())
    where1 = sqlFieldName + (access.elems_permit()?" IN ":" NOT IN ") + getSQLEnum(access.elems());

  if (addNulls)
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

std::string getSQLFilter(const std::string& sqlFieldName, const AccessControl accessControl)
{
  bool addNulls=accessControl==AccessControl::PermittedAirlinesOrNull ||
                accessControl==AccessControl::PermittedAirportsOrNull ||
                accessControl==AccessControl::PermittedCitiesOrNull;

  switch(accessControl)
  {
    case AccessControl::PermittedAirlines:
    case AccessControl::PermittedAirlinesOrNull:
      return getSQLFilter(sqlFieldName, TReqInfo::Instance()->user.access.airlines(), addNulls);
    case AccessControl::PermittedAirports:
    case AccessControl::PermittedAirportsOrNull:
      return getSQLFilter(sqlFieldName, TReqInfo::Instance()->user.access.airps(), addNulls);
    case AccessControl::PermittedCities:
    case AccessControl::PermittedCitiesOrNull:
      return getSQLFilter(sqlFieldName, getCitiesAccess(TReqInfo::Instance()->user.access.airps()), addNulls);
  }

  throw Exception("%s: unknown accessControl", __func__);
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

bool isPermitted(const std::optional<CityCode_t>& cityOpt)
{
  const auto cityAccess=getCitiesAccess(TReqInfo::Instance()->user.access.airps());

  if (cityOpt)
    return cityAccess.permitted(cityOpt.value());
  else
    return cityAccess.totally_permitted();
}

namespace CacheTable
{

void throwIfNotPermitted(const std::optional<AirportCode_t>& airportOpt, bool isNewRow)
{
  if (isPermitted(airportOpt)) return;

  if (airportOpt) {
    throw UserException(isNewRow?"MSG.NO_PERM_ENTER_AIRPORT":"MSG.NO_PERM_MODIFY_AIRPORT",
                        LParams() << LParam("airp", ElemIdToCodeNative(etAirp, airportOpt.value().get())));
  }
  else {
    throw UserException(isNewRow?"MSG.NEED_SET_CODE_AIRP":"MSG.NO_PERM_MODIFY_INDEFINITE_AIRPORT");
  }
}

void checkAirportAccess(const std::string& fieldName,
                        const std::optional<CacheTable::Row>& oldRow,
                        const std::optional<CacheTable::Row>& newRow)
{
  for(bool isNewRow : {false, true})
  {
    const std::optional<CacheTable::Row>& row = isNewRow?newRow:oldRow;
    if (!row) continue;

    std::optional<AirportCode_t> airportOpt;
    if (!fieldName.empty())
    {
      string value=row.value().getAsString(fieldName);
      if (!value.empty()) airportOpt.emplace(value);
    }

    throwIfNotPermitted(airportOpt, isNewRow);
  }
}

void throwIfNotPermitted(const std::optional<AirlineCode_t>& airlineOpt, bool isNewRow)
{
  if (isPermitted(airlineOpt)) return;

  if (airlineOpt) {
    throw UserException(isNewRow?"MSG.NO_PERM_ENTER_AIRLINE":"MSG.NO_PERM_MODIFY_AIRLINE",
                        LParams() << LParam("airline", ElemIdToCodeNative(etAirline, airlineOpt.value().get())));
  }
  else {
    throw UserException(isNewRow?"MSG.NEED_SET_CODE_AIRLINE":"MSG.NO_PERM_MODIFY_INDEFINITE_AIRLINE");
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

    std::optional<AirlineCode_t> airlineOpt;
    if (!fieldName.empty())
    {
      string value=row.value().getAsString(fieldName);
      if (!value.empty()) airlineOpt.emplace(value);
    }

    throwIfNotPermitted(airlineOpt, isNewRow);
  }
}

void checkCityAccess(const std::string& fieldName,
                     const std::optional<CacheTable::Row>& oldRow,
                     const std::optional<CacheTable::Row>& newRow)
{
  for(bool isNewRow : {false, true})
  {
    const std::optional<CacheTable::Row>& row = isNewRow?newRow:oldRow;
    if (!row) continue;

    std::optional<CityCode_t> cityOpt;
    if (!fieldName.empty())
    {
      string value=row.value().getAsString(fieldName);
      if (!value.empty()) cityOpt.emplace(value);
    }

    if (isPermitted(cityOpt)) continue;

    if (cityOpt) {
      throw UserException(isNewRow?"MSG.NO_PERM_ENTER_CITY":"MSG.NO_PERM_MODIFY_CITY",
                          LParams() << LParam("city", ElemIdToCodeNative(etCity, cityOpt.value().get())));
    }
    else {
      throw UserException(isNewRow?"MSG.NEED_SET_CODE_CITY":"MSG.NO_PERM_MODIFY_INDEFINITE_CITY");
    }
  }
}

//проверка прав работы с этапом

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

} //namespace CacheTable

template<>
void Access<DeskGrpId_t>::init()
{
  totally_permitted=TReqInfo::Instance()->user.access.airlines().totally_permitted() &&
                    TReqInfo::Instance()->user.access.airps().totally_permitted();
}

template<>
void Access<DeskCode_t>::init()
{
  totally_permitted=TReqInfo::Instance()->user.access.airlines().totally_permitted() &&
                    TReqInfo::Instance()->user.access.airps().totally_permitted();
}

template<>
void Access<DeskGrpId_t>::downloadPermissions()
{
  permitted.emplace();

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

  while (!cur.fen()) permitted.value().emplace(grpId);
}

template<>
void Access<DeskCode_t>::downloadPermissions()
{
  permitted.emplace();

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

  while (!cur.fen()) permitted.value().emplace(desk);
}

template<>
void ViewAccess<DeskGrpId_t>::downloadPermissions()
{
  permitted.emplace();

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

  while (!cur.fen()) permitted.value().emplace(grpId);
}

template<>
void ViewAccess<DeskCode_t>::downloadPermissions()
{
  permitted.emplace();

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

  while (!cur.fen()) permitted.value().emplace(desk);
}

template<>
void Access<ValidatorCode_t>::init()
{
  totally_permitted=false;
}

template<>
void Access<ValidatorCode_t>::downloadPermissions()
{
  permitted.emplace();

  ostringstream sql;
  sql << "SELECT validator FROM operators WHERE login=:login AND pr_denial=0";

  if (id_) sql << " AND validator=:validator";

  auto cur = make_db_curs(sql.str(), PgOra::getROSession("OPERATORS"));

  if (id_) cur.bind(":validator", id_.value().get());

  std::string validator;
  cur.bind(":login", TReqInfo::Instance()->user.login)
     .def(validator)
     .exec();

  while (!cur.fen()) permitted.value().emplace(validator);
}

template<>
void ViewAccess<ValidatorCode_t>::downloadPermissions()
{
  permitted.emplace();
}

void RoleAccessAncestor::init()
{
  totally_permitted=TReqInfo::Instance()->user.access.airlines().totally_permitted() &&
                    TReqInfo::Instance()->user.access.airps().totally_permitted();
}

std::tuple<std::string, std::list<std::string>> RoleAccessAncestor::getSqlProps() const
{
  return {"SELECT role_id FROM roles "
          "WHERE " + getSQLFilter("airline", AccessControl::PermittedAirlines) + " AND "
                   + getSQLFilter("airp",    AccessControl::PermittedAirports),
          {"ROLES"}};
}

std::tuple<std::string, std::list<std::string>> RoleAccessAncestor::getSqlPropsExtra() const
{
  const auto airlines_access=TReqInfo::Instance()->user.access.airlines();
  const auto airps_access=TReqInfo::Instance()->user.access.airps();

  if (airlines_access.totally_not_permitted() ||
      airps_access.totally_not_permitted()) return {"", {}};

  ostringstream sql;
  sql << "SELECT roles.role_id "
         "FROM extra_role_access, roles "
         "WHERE (extra_role_access.airline_to=roles.airline OR "
                "extra_role_access.airline_to IS NULL AND roles.airline IS NULL) AND "
               "(extra_role_access.airp_to=roles.airp OR "
                "extra_role_access.airp_to IS NULL AND roles.airp IS NULL) ";

  if (airlines_access.totally_permitted())
    sql << " AND extra_role_access.airline_from IS NULL";
  else
    sql << " AND extra_role_access.airline_from"
        << (airlines_access.elems_permit()?" IN ":" NOT IN ")
        << getSQLEnum(airlines_access.elems());

  if (airps_access.totally_permitted())
    sql << " AND extra_role_access.airp_from IS NULL";
  else
    sql << " AND extra_role_access.airp_from"
        << (airps_access.elems_permit()?" IN ":" NOT IN ")
        << getSQLEnum(airps_access.elems());

  if (!viewOnly_)
    sql << " AND full_access<>0";

  return {sql.str(), {"ROLES", "EXTRA_ROLE_ACCESS"}};
}

void RoleAccessAncestor::addPermissions(const std::tuple<std::string, std::list<std::string>>& sqlProps)
{
  if (!permitted) permitted.emplace();

  std::string sql;
  std::list<std::string> sqlTables;
  std::tie(sql, sqlTables) = sqlProps;

  if (sql.empty()) return;

  if (id_) sql += " AND roles.role_id=:role_id";

  auto cur = make_db_curs(sql, PgOra::getROSession(sqlTables));

  if (id_) cur.bind(":role_id", id_.value().get());

  int roleId;
  cur.def(roleId)
     .exec();

  while (!cur.fen()) permitted.value().emplace(roleId);
}

bool RoleAccessAncestor::check(const RoleId_t& id)
{
  if (totally_permitted) return true;

  if (!permitted)
  {
    addPermissions(getSqlProps());
    addPermissions(getSqlPropsExtra());
  }

  return algo::contains(permitted.value(), id);
}

bool RoleAccess::check(const RoleId_t& id)
{
  if (!RoleAccessAncestor::check(id)) return false;

  auto cur = make_db_curs(
    "SELECT role_rights.right_id "
    "FROM "
    "(SELECT role_rights.right_id "
    " FROM role_rights "
    " WHERE role_id=:role_id "
    " UNION "
    " SELECT role_assign_rights.right_id "
    " FROM role_assign_rights "
    " WHERE role_id=:role_id) role_rights "
    "LEFT OUTER JOIN "
    "(SELECT role_assign_rights.right_id "
    " FROM user_roles INNER JOIN role_assign_rights "
    " ON user_roles.role_id=role_assign_rights.role_id "
    " WHERE user_roles.user_id=:user_id) user_rights "
    "ON role_rights.right_id=user_rights.right_id "
    "WHERE user_rights.right_id IS NULL "
    "FETCH FIRST 1 ROWS ONLY",
    PgOra::getROSession({"ROLE_RIGHTS", "ROLE_ASSIGN_RIGHTS", "USER_ROLES"}));

  int rightId;
  cur.stb()
     .bind(":role_id", id.get())
     .bind(":user_id", TReqInfo::Instance()->user.user_id)
     .def(rightId)
     .EXfet();

  return (cur.err() == DbCpp::ResultCode::NoDataFound);
}

namespace CacheTable
{

//проверка прав работы с группой пультов

void checkDeskGrpAccess(const std::string& deskGrpIdFieldName,
                        const bool deskGrpIdFieldIsNullable,
                        const std::optional<CacheTable::Row>& oldRow,
                        const std::optional<CacheTable::Row>& newRow)

{
  for(bool isNewRow : {false, true})
  {
    const std::optional<CacheTable::Row>& row = isNewRow?newRow:oldRow;
    if (!row) continue;

    if (deskGrpIdFieldIsNullable &&
        row.value().getAsString(deskGrpIdFieldName).empty()) continue;

    DeskGrpId_t deskGrpId(row.value().getAsInteger_ThrowOnEmpty(deskGrpIdFieldName));

    if (!Access(deskGrpId).check(deskGrpId))
      throw UserException(isNewRow?"MSG.ACCESS.NO_PERM_ENTER_DESK_GRP":
                                   "MSG.ACCESS.NO_PERM_MODIFY_DESK_GRP"); //!!! хорошо бы в сообщении выводить название группы
  }
}

void checkDeskAccess(const std::string& deskFieldName,
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
                         const bool deskGrpIdFieldIsNullable,
                         std::optional<CacheTable::Row>& row)
{
  if (!row) return;

  std::string desk=row.value().getAsString(deskFieldName);
  std::optional<int> deskGrpIdOpt=row.value().getAsInteger(deskGrpIdFieldName);
  if (desk.empty())
  {
    if (!deskGrpIdFieldIsNullable && !deskGrpIdOpt)
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

  if (!deskGrpIdFieldIsNullable)
    row.value().setFromInteger(deskGrpIdFieldName, grpId);
}

void checkUserTypesAccess(const std::string& userTypeFieldName1,
                          const std::string& userTypeFieldName2,
                          const std::optional<CacheTable::Row>& oldRow,
                          const std::optional<CacheTable::Row>& newRow)
{
  TUserType ownUserType=TReqInfo::Instance()->user.user_type;

  if (ownUserType==utSupport) return;

  for(bool isNewRow : {false, true})
  {
    const std::optional<CacheTable::Row>& row = isNewRow?newRow:oldRow;
    if (!row) continue;

    TUserType userType1=static_cast<TUserType>(row.value().getAsInteger_ThrowOnEmpty(userTypeFieldName1));
    TUserType userType2=static_cast<TUserType>(row.value().getAsInteger_ThrowOnEmpty(userTypeFieldName2));

    if (ownUserType!=userType1 || ownUserType!=userType2)
      throw UserException(isNewRow?"MSG.ACCESS.NO_PERM_ENTER_USER_TYPES":
                                   "MSG.ACCESS.NO_PERM_MODIFY_USER_TYPES");
  }
}

void checkNotNullAirportOrAirportAccess(const std::string& fieldName1,
                                        const std::string& fieldName2,
                                        const std::optional<CacheTable::Row>& oldRow,
                                        const std::optional<CacheTable::Row>& newRow)
{
  for(bool isNewRow : {false, true})
  {
    const std::optional<CacheTable::Row>& row = isNewRow?newRow:oldRow;
    if (!row) continue;

    AirportCode_t airport1(row.value().getAsString_ThrowOnEmpty(fieldName1));
    AirportCode_t airport2(row.value().getAsString_ThrowOnEmpty(fieldName2));

    if (isPermitted(airport1) || isPermitted(airport2)) continue;

    throw UserException(isNewRow?"MSG.NO_PERM_ENTER_AP_AND_AP":"MSG.NO_PERM_MODIFY_AP_AND_AP",
                        LParams() << LParam("airp1", ElemIdToCodeNative(etAirp, airport1.get()))
                                  << LParam("airp2", ElemIdToCodeNative(etAirp, airport2.get())));
  }
}

void checkAirlineOrAirlineAccess(const std::string& fieldName1,
                                 const std::string& fieldName2,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow)
{
  for(bool isNewRow : {false, true})
  {
    const std::optional<CacheTable::Row>& row = isNewRow?newRow:oldRow;
    if (!row) continue;

    std::optional<AirlineCode_t> airline1;
    std::optional<AirlineCode_t> airline2;

    std::string value1=row.value().getAsString(fieldName1);
    if (!value1.empty()) airline1.emplace(value1);
    std::string value2=row.value().getAsString(fieldName2);
    if (!value2.empty()) airline2.emplace(value2);

    if (airline1==airline2)
      throwIfNotPermitted(airline1, isNewRow);
    else
    {
      if (isPermitted(airline1) || isPermitted(airline2)) continue;

      if (airline1 && airline2)
        throw UserException(isNewRow?"MSG.NO_PERM_ENTER_AL_AND_AL":"MSG.NO_PERM_MODIFY_AL_AND_AL",
                            LParams() << LParam("airline1", ElemIdToCodeNative(etAirline, airline1.value().get()))
                                      << LParam("airline2", ElemIdToCodeNative(etAirline, airline2.value().get())));
      else
        throw UserException(isNewRow?"MSG.NO_PERM_ENTER_AL_AND_INDEFINITE_AL":"MSG.NO_PERM_MODIFY_AL_AND_INDEFINITE_AL",
                            LParams() << LParam("airline", ElemIdToCodeNative(etAirline, (airline1?airline1:airline2).value().get())));
    }
  }
}

void checkNotNullRoleAccess(const std::string& roleIdFieldName,
                            const std::optional<CacheTable::Row>& oldRow,
                            const std::optional<CacheTable::Row>& newRow)
{
  for(bool isNewRow : {false, true})
  {
    const std::optional<CacheTable::Row>& row = isNewRow?newRow:oldRow;
    if (!row) continue;

    RoleId_t roleId(row.value().getAsInteger_ThrowOnEmpty(roleIdFieldName));

    if (!RoleAccess(roleId).check(roleId))
      throw UserException(isNewRow?"MSG.ACCESS.NO_PERM_ENTER_USER_ROLE":"MSG.ACCESS.NO_PERM_MODIFY_USER_ROLE");
  }
}

} //namespace CacheTable

