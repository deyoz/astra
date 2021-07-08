# meta: suite crypt

include(ts/macro.ts)
include(ts/adm_macro.ts)

### test 1 - простейшие тесты на запись
### кэш CRYPT_SETS
#########################################################################################

$(init_term)

!! capture=on
$(cache PIKE RU CRYPT_SETS $(cache_iface_ver CRYPT_SETS) ""
  update OLD_ID:35249       id:35249
         Old_desk_grp_id:1  desk_grp_id:1
         old_desk:МОВЖЕК    desk:МОВЖЕК
         old_pr_crypt:0     pr_crypt:1)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <interface id='cache'/>
    <data>
      <code>CRYPT_SETS</code>
      <Forbidden>0</Forbidden>
      <ReadOnly>0</ReadOnly>
      <keep_locally>0</keep_locally>
      <keep_deleted_rows>0</keep_deleted_rows>
      <user_depend>1</user_depend>
      <rows tid='-1'>
        <row pr_del='0'>
          <col>35248</col>
          <col>1</col>
          <col>Группа разработчиков</col>
          <col/>
          <col>0</col>
        </row>
        <row pr_del='0'>
          <col>35249</col>
          <col>1</col>
          <col>Группа разработчиков</col>
          <col>МОВЖЕК</col>
          <col>1</col>
        </row>
        <row pr_del='0'>
          <col>39844</col>
          <col>1</col>
          <col>Группа разработчиков</col>
          <col>МОВВЛА</col>
          <col>0</col>
        </row>
        <row pr_del='0'>
          <col>39845</col>
          <col>1</col>
          <col>Группа разработчиков</col>
          <col>MOVDEN</col>
          <col>0</col>
        </row>
        <row pr_del='0'>
          <col>39954</col>
          <col>1</col>
          <col>Группа разработчиков</col>
          <col>МОВТ03</col>
          <col>0</col>
        </row>
      </rows>
    </data>
    <command>
      <message lexema_id='MSG.CHANGED_DATA_COMMIT' code='0'>Изменения успешно сохранены</message>
    </command>
  </answer>
</term>

!! capture=on
$(cache PIKE RU CRYPT_SETS $(cache_iface_ver CRYPT_SETS) ""
  insert desk_grp_id:1
         pr_crypt:1)

$(USER_ERROR_RESPONSE MSG.UNIQUE_CONSTRAINT_VIOLATED)

%%

### test 2 - простейшие тесты на запись
### кэш CRYPT_REQ_DATA
#########################################################################################


$(init_term)

!! capture=on
$(cache PIKE RU CRYPT_REQ_DATA $(cache_iface_ver CRYPT_REQ_DATA) ""
  insert desk_grp_id:35251
         desk:
         country:RU
         state:Регион
         city:Город
         organization:Организация
         organizational_unit:Подразделение
         title:Должность
         user_name:Пользователь
         email:E-Mail
         pr_denial:0)

$(set id $(last_history_row_id CRYPT_REQ_DATA))

>> lines=auto
      <rows tid='-1'>
        <row pr_del='0'>
          <col>$(get id)</col>
          <col>35251</col>
          <col>АК ПУПКИН</col>
          <col/>
          <col>RU</col>
          <col>Регион</col>
          <col>Город</col>
          <col>Организация</col>
          <col>Подразделение</col>
          <col>Должность</col>
          <col>Пользователь</col>
          <col>E-Mail</col>
          <col>0</col>
        </row>
      </rows>

!! capture=on
$(cache PIKE RU CRYPT_REQ_DATA $(cache_iface_ver CRYPT_REQ_DATA) ""
  insert desk_grp_id:35251
         desk:
         country:RU
         state:Регион
         city:Город
         organization:Организация
         organizational_unit:Подразделение
         title:Должность
         user_name:Пользователь
         email:E-Mail
         pr_denial:0)

$(USER_ERROR_RESPONSE MSG.UNIQUE_CONSTRAINT_VIOLATED)

!! capture=on
$(cache PIKE RU CRYPT_REQ_DATA $(cache_iface_ver CRYPT_REQ_DATA) ""
  update old_id:$(get id)                        id:$(get id)
         old_desk_grp_id:35251                   desk_grp_id:1000
         old_desk:                               desk:МОВВЛА
         old_country:RU                          country:US
         old_state:Регион                        state:State
         old_city:Город                          city:City
         old_organization:Организация            organization:Organization
         old_organizational_unit:Подразделение   organizational_unit:OrganizationalUnit
         old_title:Должность                     title:Title
         old_user_name:Пользователь              user_name:UserName
         old_email:E-Mail                        email:Мейл
         old_pr_denial:0                         pr_denial:1)

>> lines=auto
      <rows tid='-1'>
        <row pr_del='0'>
          <col>$(get id)</col>
          <col>1000</col>
          <col>Группа для Влада</col>
          <col>МОВВЛА</col>
          <col>US</col>
          <col>State</col>
          <col>City</col>
          <col>Organization</col>
          <col>OrganizationalUnit</col>
          <col>Title</col>
          <col>UserName</col>
          <col>Мейл</col>
          <col>1</col>
        </row>
      </rows>

!! capture=on
$(cache PIKE RU CRYPT_REQ_DATA $(cache_iface_ver CRYPT_REQ_DATA) ""
  delete old_id:$(get id)
         old_desk_grp_id:1000
         old_desk:МОВВЛА
         old_country:US
         old_state:State
         old_city:City
         old_organization:Organization
         old_organizational_unit:OrganizationalUnit
         old_title:Title
         old_user_name:UserName
         old_email:Мейл
         old_pr_denial:1)

>> lines=auto
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <interface id='cache'/>
    <data>
      <code>CRYPT_REQ_DATA</code>
      <Forbidden>0</Forbidden>
      <ReadOnly>0</ReadOnly>
      <keep_locally>0</keep_locally>
      <keep_deleted_rows>0</keep_deleted_rows>
      <user_depend>1</user_depend>
      <rows tid='-1'/>
    </data>
    <command>
      <message lexema_id='MSG.CHANGED_DATA_COMMIT' code='0'>...</message>
    </command>
  </answer>
</term>
