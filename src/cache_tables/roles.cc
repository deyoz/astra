#include "roles.h"
#include "PgOraConfig.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "cache_access.h"

#define NICKNAME "VLAD"
#include "serverlib/slogger.h"

using AstraLocale::UserException;

namespace {

// Roles

bool checkRoleNameStartWith(const std::string& name)
{
    std::string lowerName = StrUtils::ToLower(name);
    const std::string_view prefixes[] = {
       "����� ��",
       "�����",
       "��ᯮ��� ����஫�",
       "��ᬮ��",
    };

    for (const auto prefix: prefixes) {
        if (prefix.size() <= lowerName.size()
         && 0 == strncmp(lowerName.data(), prefix.data(), prefix.size())) {
            return true;
        }
    }

    return false;
}

void insertIntoTripListDays(const int id, const int role_id, const int shift_down, const int shigt_up)
{
    make_db_curs(
       "INSERT INTO trip_list_days(id, role_id, shift_down, shift_up) "
                          "VALUES(:id,:role_id,:shift_down,:shift_up)",
        PgOra::getRWSession("TRIP_LIST_DAYS"))
       .bind(":id", id)
       .bind(":role_id", role_id)
       .bind(":shift_down", shift_down)
       .bind(":shigt_up", shigt_up)
       .exec();
    HistoryTable("TRIP_LIST_DAYS").synchronize(RowId_t(id));
}

void insertIntoTripListDaysOnRoleName(const std::string& name, const int role_id)
{
    if (checkRoleNameStartWith(name)) {
        insertIntoTripListDays(role_id, role_id, -1, 1);
    }
}

void deleteFromTripListDays(const int role_id)
{
    make_db_curs(
       "DELETE FROM trip_list_days WHERE role_id = :role_id",
        PgOra::getRWSession("TRIP_LIST_DAYS"))
       .bind(":role_id", role_id)
       .exec();
    HistoryTable("TRIP_LIST_DAYS").synchronize(RowId_t(role_id));
}

void deleteFromRoleRights(const int role_id)
{
    auto cur = make_db_curs(
       "DELETE FROM role_rights WHERE role_id = :role_id RETURNING id",
        PgOra::getRWSession("ROLE_RIGHTS")
    );

    int id;

    cur.bind(":role_id", role_id)
       .def(id)
       .exec();

    while (!cur.fen()) {
        HistoryTable("ROLE_RIGHTS").synchronize(RowId_t(id));
    }
}

void deleteFromRoleAssignRights(const int role_id)
{
    auto cur = make_db_curs(
       "DELETE FROM role_assign_rights WHERE role_id = :role_id RETURNING id",
        PgOra::getRWSession("ROLE_ASSIGN_RIGHTS")
    );

    int id;

    cur.bind(":role_id", role_id)
       .def(id)
       .exec();

    while (!cur.fen()) {
        HistoryTable("ROLE_ASSIGN_RIGHTS").synchronize(RowId_t(id));
    }
}

void insertIntoRoles(
    const int role_id,
    const std::string& name,
    const std::string& airline,
    const std::string& airp)
{
    make_db_curs(
       "INSERT INTO roles(role_id, name, airline, airp) "
                 "VALUES(:role_id,:name,:airline,:airp)",
        PgOra::getRWSession("ROLES"))
       .bind(":role_id", role_id)
       .bind(":name", name)
       .bind(":airline", airline)
       .bind(":airp", airp)
       .exec();
    HistoryTable("ROLES").synchronize(RowId_t(role_id));
}

void updateRoles(
    const int role_id,
    const std::string& name,
    const std::string& airline,
    const std::string& airp)
{
    make_db_curs(
       "UPDATE roles "
       "SET name = :name, airline = :airline, airp = :airp "
       "WHERE role_id = :role_id",
        PgOra::getRWSession("ROLES"))
       .bind(":name", name)
       .bind(":airline", airline)
       .bind(":airp", airp)
       .bind(":role_id", role_id)
       .exec();
    HistoryTable("ROLES").synchronize(RowId_t(role_id));
}

void deleteFromRoles(const int role_id)
{
    make_db_curs(
       "DELETE FROM roles WHERE role_id = :role_id",
        PgOra::getRWSession("ROLES"))
       .bind(":role_id", role_id)
       .exec();
    HistoryTable("ROLES").synchronize(RowId_t(role_id));
}

// TripListDays

void checkShiftRangeThrowOnError(const int shift_down, const int shift_up)
{
    if (shift_down > shift_up) {
        throw UserException("������ ��������. ���孥� ᬥ饭�� ����� �������");
    } else if (shift_up - shift_down >= 10) {
        throw UserException("�������� �� ����� �ॢ���� 10 ����");
    }
}

void updateTripListDays(const int id, const int old_role_id, const int role_id, const int shift_down, const int shift_up)
{
    make_db_curs(
       "UPDATE trip_list_days "
       "SET role_id = :role_id, shift_down = :shift_down, shift_up = :shift_up "
       "WHERE role_id = :old_role_id",
        PgOra::getRWSession("TRIP_LIST_DAYS"))
       .bind(":role_id", role_id)
       .bind(":shift_down", shift_down)
       .bind(":shift_up", shift_up)
       .bind(":old_role_id", old_role_id)
       .exec();
    HistoryTable("TRIP_LIST_DAYS").synchronize(RowId_t(id));
}

void insertRoles(
    const std::string& name,
    const std::string& airline,
    const std::string& airp)
{
    int role_id = PgOra::getSeqNextVal_int("ID__SEQ");
    insertIntoRoles(role_id, name, airline, airp);
    insertIntoTripListDaysOnRoleName(name, role_id);
}

void modifyRoles(
    const int role_id,
    const std::string& name,
    const std::string& airline,
    const std::string& airp)
{
    deleteFromTripListDays(role_id);
    updateRoles(role_id, name, airline, airp);
    insertIntoTripListDaysOnRoleName(name, role_id);
}

void deleteRoles(const int role_id)
{
    deleteFromTripListDays(role_id);
    deleteFromRoleRights(role_id);
    deleteFromRoleAssignRights(role_id);
    deleteFromRoles(role_id);
}


void insertTripListDays(const int role_id, const int shift_down, const int shift_up)
{
    checkShiftRangeThrowOnError(shift_down, shift_up);
    const int id = PgOra::getSeqNextVal_int("ID__SEQ");
    insertIntoTripListDays(id, role_id, shift_down, shift_up);
}


void modifyTripListDays(const int id, const int old_role_id, const int role_id, const int shift_down, const int shift_up)
{
    checkShiftRangeThrowOnError(shift_down, shift_up);
    updateTripListDays(id, old_role_id, role_id, shift_down, shift_up);
}

void deleteTripListDays(const int role_id)
{
    deleteFromTripListDays(role_id);
}

}

namespace CacheTable
{

// Roles

void Roles::onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const
{
  DB::TQuery Qry(PgOra::getROSession("ROLES"),
                 STDLOG);

  Qry.SQLText=
       "SELECT role_id, name, airline, airp "
       "FROM roles "
       "ORDER BY name, airline, airp";
  Qry.Execute();

  if (Qry.Eof) return;

  RoleViewAccess viewAccess;

  int idxRoleId=Qry.FieldIndex("role_id");
  int idxName=Qry.FieldIndex("name");
  int idxAirline=Qry.FieldIndex("airline");
  int idxAirp=Qry.FieldIndex("airp");

  for(; !Qry.Eof; Qry.Next())
  {
    RoleId_t roleId(Qry.FieldAsInteger(idxRoleId));

    if (!viewAccess.check(roleId)) continue;

    rows.setFromInteger(Qry, idxRoleId)
        .setFromString (Qry, idxName)
        .setFromString (Qry, idxAirline)
        .setFromString (ElemIdToCodeNative(etAirline, Qry.FieldAsString(idxAirline)))
        .setFromString (Qry, idxAirp)
        .setFromString (ElemIdToCodeNative(etAirp, Qry.FieldAsString(idxAirp)))
        .setFromString (ElemIdToCodeNative(etRoles, Qry.FieldAsInteger(idxRoleId)))
        .addRow();
  }
}

void Roles::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                            const std::optional<CacheTable::Row>& oldRow,
                                            std::optional<CacheTable::Row>& newRow) const
{
  checkNotNullRoleAccess("role_id", oldRow, newRow);
  checkAirlineAccess("airline", oldRow, newRow);
  checkAirportAccess("airp", oldRow, newRow);
}

void Roles::onApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const
{
  if (status==usInserted)
  {
    const CacheTable::Row& row=newRow.value();

    ::insertRoles(row.getAsString("name"),
                          row.getAsString("airline"),
                          row.getAsString("airp"));
  }

  if (status==usModified)
  {
    ::modifyRoles(oldRow.value().getAsInteger_ThrowOnEmpty("role_id"),
                          newRow.value().getAsString("name"),
                          newRow.value().getAsString("airline"),
                          newRow.value().getAsString("airp"));
  }

  if (status==usDeleted)
  {
    ::deleteRoles(oldRow.value().getAsInteger_ThrowOnEmpty("role_id"));
  }
}

std::list<std::string> Roles::dbSessionObjectNames() const {
    return {"ROLES", "TRIP_LIST_DAYS", "ROLE_ASSIGN_RIGHTS", "ROLE_RIGHTS"};
}

// TripListDays

void TripListDays::onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const
{
  DB::TQuery Qry(PgOra::getROSession("TRIP_LIST_DAYS"),
                 STDLOG);

  Qry.SQLText=
        "SELECT role_id, "
               "shift_down, "
               "shift_up, "
               "id "
          "FROM trip_list_days "
         "ORDER BY role_id";
  Qry.Execute();

  if (Qry.Eof) return;

  RoleViewAccess viewAccess;

  int idxRoleId=Qry.FieldIndex("role_id");
  int idxShiftDown=Qry.FieldIndex("shift_down");
  int idxShiftUp=Qry.FieldIndex("shift_up");
  int idxId=Qry.FieldIndex("id");

  for(; !Qry.Eof; Qry.Next())
  {
    RoleId_t roleId(Qry.FieldAsInteger(idxRoleId));

    if (!viewAccess.check(roleId)) continue;

    rows.setFromInteger(Qry, idxRoleId)
        .setFromString (ElemIdToCodeNative(etRoles, Qry.FieldAsInteger(idxRoleId)))
        .setFromInteger(Qry, idxShiftDown)
        .setFromInteger(Qry, idxShiftUp)
        .setFromInteger(Qry, idxId)
        .addRow();
  }
}

void TripListDays::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                            const std::optional<CacheTable::Row>& oldRow,
                                            std::optional<CacheTable::Row>& newRow) const
{
  checkNotNullRoleAccess("role_id", oldRow, newRow);
}


void TripListDays::onApplyingRowChanges(const TCacheUpdateStatus status,
                                        const std::optional<CacheTable::Row>& oldRow,
                                        const std::optional<CacheTable::Row>& newRow) const
{
  if (status==usInserted)
  {
    ::insertTripListDays(newRow.value().getAsInteger_ThrowOnEmpty("role_id"),
                                 newRow.value().getAsInteger_ThrowOnEmpty("shift_down"),
                                 newRow.value().getAsInteger_ThrowOnEmpty("shift_up"));
  }

  if (status==usModified)
  {
    ::modifyTripListDays(oldRow.value().getAsInteger_ThrowOnEmpty("id"),
                                 oldRow.value().getAsInteger_ThrowOnEmpty("role_id"),
                                 newRow.value().getAsInteger_ThrowOnEmpty("role_id"),
                                 newRow.value().getAsInteger_ThrowOnEmpty("shift_down"),
                                 newRow.value().getAsInteger_ThrowOnEmpty("shift_up"));
  }

  if (status==usDeleted)
  {
    ::deleteTripListDays(oldRow.value().getAsInteger_ThrowOnEmpty("role_id"));
  }
}

std::list<std::string> TripListDays::dbSessionObjectNames() const {
    return {"TRIP_LIST_DAYS"};
}

// Rights

bool Rights::userDependence() const {
  return true;
}

std::string Rights::selectSql() const {
  return "SELECT a.right_id, "
                "name, "
                "name_lat FROM rights_list "
           "JOIN (SELECT DISTINCT role_assign_rights.right_id "
                            "FROM user_roles "
                            "JOIN role_assign_rights "
                              "ON role_assign_rights.role_id = user_roles.role_id "
                             "AND user_roles.user_id = :user_id) a "
             "ON a.right_id = rights_list.ida ORDER BY a.right_id";
}

void Rights::beforeSelectOrRefresh(const TCacheQueryType queryType,
                                   const TParams& sqlParams,
                                   DB::TQuery& Qry) const
{
  Qry.CreateVariable("user_id", otInteger, TReqInfo::Instance()->user.user_id);
}

std::list<std::string> Rights::dbSessionObjectNames() const {
  return {"RIGHTS_LIST", "ROLE_ASSIGN_RIGHTS", "USER_ROLES"};
}

// ProfiledRightsList

bool ProfiledRightsList::userDependence() const {
  return false;
}

std::string ProfiledRightsList::selectSql() const {
  return "SELECT right_id, name, name_lat "
           "FROM profiled_rights_list "
           "JOIN rights_list "
             "ON right_id = ida "
          "ORDER BY right_id";
}

std::list<std::string> ProfiledRightsList::dbSessionObjectNames() const {
  return {"PROFILED_RIGHTS_LIST", "RIGHTS_LIST"};
}

// Users

bool Users::userDependence() const {
  return true;
}

std::string Users::selectSql() const {
  return "SELECT users2.user_id, "
                "login, "
                "descr, "
                "type AS type_code, "
                "type AS type_name, "
                "pr_denial "
           "FROM users2 "
          "WHERE adm.check_user_view_access(users2.user_id,:user_id)<>0 "
            "AND pr_denial != -1 "
          "ORDER BY descr";
}

void Users::beforeSelectOrRefresh(const TCacheQueryType queryType,
                                  const TParams& sqlParams,
                                  DB::TQuery& Qry) const
{
  Qry.CreateVariable("user_id", otInteger, TReqInfo::Instance()->user.user_id);
}

std::list<std::string> Users::dbSessionObjectNames() const {
  return {"USERS2"};
}

// UserTypes

bool UserTypes::userDependence() const {
  return false;
}

std::string UserTypes::selectSql() const {
  return "SELECT code id, name, name_lat FROM user_types ORDER BY code";
}

std::list<std::string> UserTypes::dbSessionObjectNames() const {
  return {"USER_TYPES"};
}

} // CacheTable
