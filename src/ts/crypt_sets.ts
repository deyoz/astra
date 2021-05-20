# meta: suite crypt

include(ts/macro.ts)

$(init_term)
$(set_user_time_type LocalAirp PIKE)

!! capture=on
<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='cache' ver='1' opr='PIKE' screen='MAINDCS.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <cache_apply>
      <params>
        <code>CRYPT_SETS</code>
        <interface_ver>$(cache_iface_ver CRYPT_SETS)</interface_ver>
        <data_ver>-1</data_ver>
      </params>
      <rows>
        <row index='0' status='modified'>
          <col index='0'>
            <old>35249</old>
            <new>35249</new>
          </col>
          <col index='1'>
            <old>1</old>
            <new>1</new>
          </col>
          <col index='2'>
            <old/>
            <new/>
          </col>
          <col index='3'>
            <old>МОВЖЕК</old>
            <new>МОВЖЕК</new>
          </col>
          <col index='4'>
            <old>0</old>
            <new>1</new>
          </col>
        </row>
      </rows>
    </cache_apply>
  </query>
</term>
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

