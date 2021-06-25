# meta: suite crypt

include(ts/macro.ts)
include(ts/adm_macro.ts)

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
          <col>39845</col>
          <col>1</col>
          <col>Группа разработчиков</col>
          <col>MOVDEN</col>
          <col>0</col>
        </row>
        <row pr_del='0'>
          <col>39844</col>
          <col>1</col>
          <col>Группа разработчиков</col>
          <col>МОВВЛА</col>
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
          <col>39954</col>
          <col>1</col>
          <col>Группа разработчиков</col>
          <col>МОВТ03</col>
          <col>0</col>
        </row>
        <row pr_del='0'>
          <col>35248</col>
          <col>1</col>
          <col>Группа разработчиков</col>
          <col/>
          <col>0</col>
        </row>
      </rows>
    </data>
    <command>
      <message lexema_id='MSG.CHANGED_DATA_COMMIT' code='0'>Изменения успешно сохранены</message>
    </command>
  </answer>
</term>

