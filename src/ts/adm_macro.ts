### функция cache:
### предназначена для формирования запроса от терминала по загрузке или изменению cache tables
### обязательные параметры (5 штук):
### $(cache <queryLogin>
###         <queryLang>
###         <название кэша (cache_tables.code)>
###         <версия интерфейса (cache_tables.tid)>
###         <версия данных (обычно - максимальный tid данных)>
###  )
### если указать только обязательные параметры, то будет загрузка данных
### для некоторых кэшей, если указать версию данных, то будет произведена частичная подгрузка данных (refresh)
###
### после обязательных параметров можно указать SQL-параметры (один или более) необходимые для некоторых кэшей
### для этого надо использовать функцию cache_sql_param:
### $(cache_sql_param <название параметра> <тип параметра> <значение параметра>)
### тип параметра: string/integer/double/datetime
###
### после обязательных параметров и необязательных SQL-параметров можно указывать параметры для изменения данных
### любое применяемое изменение состоит из типа изменения строки кэша и значений полей для изменения
### тип изменения: insert/update/delete
### имена полей и значения могут разделяться '=' (в этом случае придется применять двойные кавычки или фигурные скобки, например "имя=значение")
### имена полей и значения могут также разделяться ':'
### если значение поля не указано, оно принимает значение NULL
### в общем случае с помощью параметров может быть применено множество изменений одновременно:
### <тип_изменения1> <имена_полей_и_значения1> <тип_изменения2> <имена_полей_и_значения2> ...
###
### для получения текущей версии интерфейса кэша можно использовать $(cache_iface_ver <название кэша>)
### при применении изменений важно, чтобы версия интерфейса совпадала с текущей
###
### имена кэшей и полей изменения case insensitive
#######################################################################################################################################

$(defmacro combine_brd_with_reg
  point_dep
  hall
{
$(cache PIKE RU TRIP_BRD_WITH_REG $(cache_iface_ver TRIP_BRD_WITH_REG) ""
  $(cache_sql_param point_id integer $(point_dep))
  insert point_id:$(point_dep) type:101 pr_misc:1 hall:$(hall))
})

$(defmacro combine_exam_with_brd
  point_dep
  hall
{
$(cache PIKE RU TRIP_EXAM_WITH_BRD $(cache_iface_ver TRIP_EXAM_WITH_BRD) ""
  $(cache_sql_param point_id integer $(point_dep))
  insert point_id:$(point_dep) type:102 pr_misc:1 hall:$(hall))
})

$(defmacro prepare_bt_for_flight
  point_dep
  bt_code
{
$(cache PIKE RU TRIP_BT $(cache_iface_ver TRIP_BT) ""
  $(cache_sql_param point_id integer $(point_dep))
  insert point_id:$(point_dep)
         bt_code:$(bt_code))
})

$(defmacro deny_ets_interactive
  airline
  flt_no
  airp_dep
  deny=1
{$(cache PIKE RU MISC_SET $(cache_iface_ver MISC_SET) ""
  insert type_code:11
         airline:$(get_elem_id etAirline $(airline))
         flt_no:$(flt_no)
         airp_dep:$(get_elem_id etAirp $(airp_dep))
         pr_misc:$(deny))})

$(defmacro prepare_bp_printing
  airline
  flt_no
  airp_dep
  class
  bp_type
{
$(cache PIKE RU AIRLINE_BP_SET $(cache_iface_ver AIRLINE_BP_SET) ""
  $(cache_sql_param airline string $(get_elem_id etAirline $(airline)))
  insert flt_no:$(flt_no)
         airp_dep:$(get_elem_id etAirp $(airp_dep))
         class:$(get_elem_id etClass $(class))
         bp_code:$(if $(eq $(bp_type) "") $(get_random_bp_type) $(bp_type))
 )
})

$(defmacro allow_gds_exchange
  airline
  flt_no
  airp_dep
  allow=1
{
$(cache PIKE RU MISC_SET $(cache_iface_ver MISC_SET) ""
  insert type_code:30
         airline:$(get_elem_id etAirline $(airline))
         flt_no:$(flt_no)
         airp_dep:$(get_elem_id etAirp $(airp_dep))
         pr_misc:$(allow)
 )
})


$(defmacro CREATE_USER
  login
  descr
  user_type=0
  lang=RU
{

!! capture=on
<term>
  <query handle='0' id='access' ver='1' opr='PIKE' screen='ACCESS.EXE' mode='STAND' lang='$(lang)' term_id='2479792165'>
    <save_user>
      <descr>$(descr)</descr>
      <login>$(login)</login>
      <user_type>$(user_type)</user_type>
      <pr_denial>0</pr_denial>
    </save_user>
  </query>
</term>

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <user_id>...</user_id>
    <descr>$(descr)</descr>
    <login>$(login)</login>
    <type>$(user_type)</type>
    <pr_denial>0</pr_denial>
    <time_fmt_code>1</time_fmt_code>
    <disp_airline_fmt_code>9</disp_airline_fmt_code>
    <disp_airp_fmt_code>9</disp_airp_fmt_code>
    <disp_craft_fmt_code>9</disp_craft_fmt_code>
    <disp_suffix_fmt_code>17</disp_suffix_fmt_code>
  </answer>
</term>

})

$(defmacro CREATE_DESK
  code
  grp_id
{

$(cache PIKE RU DESKS $(cache_iface_ver DESKS) ""
  insert code:$(code)
         grp_id:$(grp_id)
 )

$(cache PIKE RU DESK_OWNERS $(cache_iface_ver DESK_OWNERS) ""
  insert desk:$(code)
         pr_denial:0
 )

})

$(defmacro ADD_HTTP_CLIENT
  exchange_type
  client_id
  user_login
  desk
  http_user
  http_pswd
{

$(sql {INSERT INTO web_clients(client_id, client_type, descr, desk, user_id, tracing_search, id)
       VALUES('$(client_id)', 'HTTP', '$(client_id)', '$(desk)', $(get_user_id $(user_login)), 0, id__seq.nextval)})

$(sql {INSERT INTO http_clients(id, http_user, http_pswd, exchange_type)
       VALUES('$(client_id)', '$(http_user)', '$(http_pswd)', '$(exchange_type)')})

})
