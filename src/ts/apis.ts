include(ts/macro.ts)
include(ts/adm_macro.ts)
include(ts/spp/write_dests_macro.ts)
include(ts/spp/write_trips_macro.ts)
include(ts/pax/checkin_macro.ts)

# meta: suite apis

$(defmacro CREATE_APIS_REQUEST
  point_id
  capture=off
{
!! capture=$(capture) err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='sopp' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <CreateAPIS>
      <point_id>$(point_id)</point_id>
    </CreateAPIS>
  </query>
</term>}

})


$(defmacro CREATE_APIS_OK
  point_id
{
$(CREATE_APIS_REQUEST $(point_id) capture=on)

>> lines=auto
$(MESSAGE_TAG MSG.APIS_CREATED)

})

$(defmacro CREATE_APIS_ERROR
  point_id
  error
{
$(CREATE_APIS_REQUEST $(point_id) capture=on)
$(USER_ERROR_RESPONSE $(error))
})


### test 1 - формирование XML_TR
#########################################################################################

$(init_term)
$(set_user_time_type LocalAirp PIKE)

$(set airline UT)
$(set flt_no 243)
$(set craft TU5)
$(set airp_dep DME)
$(set time_dep "$(date_format %d.%m.%Y +1) 07:00")
$(set time_arv "$(date_format %d.%m.%Y +1) 10:00")
$(set airp_arv AYT)

$(set country TR)

$(cache PIKE RU APIS_SETS $(cache_iface_ver APIS_SETS) ""
  insert airline:$(get_elem_id etAirline $(get airline))
         country_dep:
         country_arv:$(get_elem_id etCountry $(get country))
         country_control:$(get_elem_id etCountry $(get country))
         format:XML_TR
         transport_type:FILE
         transport_params:dummy
         pr_denial:0)

$(cache PIKE RU OUT_FILE_PARAM_SETS $(cache_iface_ver OUT_FILE_PARAM_SETS) ""
  insert type:APIS_TR
         point_addr:$(gettcl OWN_POINT_ADDR)
         airline:$(get_elem_id etAirline $(get airline))
         param_name:URL
         param_value:http://ws.gtb.gov.tr:8080/EXT/Gumruk/VOY/Provider/VOYWS
  insert type:APIS_TR
         point_addr:$(gettcl OWN_POINT_ADDR)
         airline:$(get_elem_id etAirline $(get airline))
         param_name:ACTION_CODE
         param_value:GTB_VOY2_XML_WebServices_VOY_WS_Binder_getFlightMessage
  insert type:APIS_TR
         point_addr:$(gettcl OWN_POINT_ADDR)
         airline:$(get_elem_id etAirline $(get airline))
         param_name:LOGIN
         param_value:login
  insert type:APIS_TR
         point_addr:$(gettcl OWN_POINT_ADDR)
         airline:$(get_elem_id etAirline $(get airline))
         param_name:PASSWORD
         param_value:password)

$(cache PIKE RU DESKS $(cache_iface_ver DESKS) ""
  insert code:$(gettcl OWN_POINT_ADDR) grp_id:1)

<<
MOWKK1H
.TJMRM1T $(dd)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART1
-$(get airp_arv)005Y
ENDPNL

$(NEW_SPP_FLIGHT_ONE_LEG $(get airline) $(get flt_no) $(get craft) $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))

$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv) hall=1
{
<passengers>
  <pax>
$(NEW_CHECKIN_NOREC ПУПКИН ВАСЯ
  document={<document>
              <type>P</type>
              <issue_country>TR</issue_country>
              <no>1357924680</no>
              <nationality>TR</nationality>
              <birth_date>08.02.1983 00:00:00</birth_date>
              <expiry_date/>
              <gender>M</gender>
              <surname>PUPKIN</surname>
              <first_name>VASYA</first_name>
            </document>}
)
  </pax>
</passengers>
})

$(CREATE_APIS_ERROR $(get point_dep) MSG.APIS_CREATION_ONLY_AFTER_CHECKIN_CLOSING)


### формируем по закрытию регистрации

$(CLOSE_CHECKIN $(get point_dep))

??
$(pg_dump_table trip_apis_params display="on")

>> lines=auto
[XML_TR] [version] [0] [$(get point_dep)] $()


### формируем по нажатию кнопки в терминале

$(CREATE_APIS_OK $(get point_dep))

??
$(pg_dump_table trip_apis_params display="on")

>> lines=auto
[XML_TR] [version] [1] [$(get point_dep)] $()


### формируем по вылету

$(CHANGE_SPP_FLIGHT_ONE_LEG $(get point_dep) "$(date_format %d.%m.%Y) $(date_format %H:%M)" "" $(get airline) $(get flt_no) $(get craft) $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv))

??
$(pg_dump_table trip_apis_params display="on")

>> lines=auto
[XML_TR] [version] [2] [$(get point_dep)] $()


### проверяем очередь files

??
$(db_dump_table file_queue display="on")

>> lines=auto
[...] [$(gettcl OWN_POINT_ADDR)] [$(gettcl OWN_POINT_ADDR)] [PUT] [...] [APIS_TR] $()
[...] [$(gettcl OWN_POINT_ADDR)] [$(gettcl OWN_POINT_ADDR)] [PUT] [...] [APIS_TR] $()
[...] [$(gettcl OWN_POINT_ADDR)] [$(gettcl OWN_POINT_ADDR)] [PUT] [...] [APIS_TR] $()

??
$(db_dump_table file_params fields="name, value" order="id, name" display="on")

>> lines=auto
[ACTION_CODE] [GTB_VOY2_XML_WebServices_VOY_WS_Binder_getFlightMessage] $()
[EVENT_ID1] [$(get point_dep)] $()
[EVENT_TYPE] [РЕЙ] $()
[LOGIN] [login] $()
[PASSWORD] [password] $()
[URL] [http://ws.gtb.gov.tr:8080/EXT/Gumruk/VOY/Provider/VOYWS] $()
[ACTION_CODE] [GTB_VOY2_XML_WebServices_VOY_WS_Binder_getFlightMessage] $()
[EVENT_ID1] [$(get point_dep)] $()
[EVENT_TYPE] [РЕЙ] $()
[LOGIN] [login] $()
[PASSWORD] [password] $()
[URL] [http://ws.gtb.gov.tr:8080/EXT/Gumruk/VOY/Provider/VOYWS] $()
[ACTION_CODE] [GTB_VOY2_XML_WebServices_VOY_WS_Binder_getFlightMessage] $()
[EVENT_ID1] [$(get point_dep)] $()
[EVENT_TYPE] [РЕЙ] $()
[LOGIN] [login] $()
[PASSWORD] [password] $()
[URL] [http://ws.gtb.gov.tr:8080/EXT/Gumruk/VOY/Provider/VOYWS] $()

??
$(db_dump_table files fields="sender, receiver, type, error" display="on")

>> lines=auto
[$(gettcl OWN_POINT_ADDR)] [$(gettcl OWN_POINT_ADDR)] [APIS_TR] [NULL] $()
[$(gettcl OWN_POINT_ADDR)] [$(gettcl OWN_POINT_ADDR)] [APIS_TR] [NULL] $()
[$(gettcl OWN_POINT_ADDR)] [$(gettcl OWN_POINT_ADDR)] [APIS_TR] [NULL] $()




