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

$(defmacro deny_ets_interactive
  airline
  flt_no
  airp_dep
{
$(cache PIKE RU MISC_SET $(cache_iface_ver MISC_SET) ""
  insert type_code:11
         airline:$(get_elem_id etAirline $(airline))
         flt_no:$(flt_no)
         airp_dep:$(get_elem_id etAirp $(airp_dep))
         pr_misc:1
 )
})

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


