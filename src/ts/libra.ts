include(ts/macro.ts)
include(ts/adm_macro.ts)

# meta: suite libra


###
#   Описание: get_schedule
#
###
#########################################################################################

$(desc_test 1)

$(init_term)

$(http_forecast content="<result><status>OK</status><answer><root name=\"get_schedule\" result=\"ok\"><block name=\"Text\"/></root></answer></result>")

# $(http_forecast content="<result><status>ERR</status><code>DB-01</code><reason>Exception happened</reason></result>")

{<?xml version='1.0' encoding='CP866'?>
<term>
<query handle='0' id='libra' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='RU' term_id='2479792165'>
  <request>
    <libra_id>2</libra_id>
    <data>&lt;?xml version='1.0' encoding='utf-8'?&gt;&lt;root name='get_schedule' date_begin='19.04.2020' date_end='21.04.2020'&gt;&lt;/root&gt;</data>
  </request>
</query>
</term>}

>> lines=auto
<root name=\"get_schedule\" date_begin=\"19.04.2020\" date_end=\"21.04.2020\"...

$(KICK_IN_SILENT)


%%
###
#   Описание: astra_call/load_tlg
#
###
#########################################################################################

$(desc_test 2)

$(init_term)

$(sql "insert into TYPEB_ORIGINATORS(ID, ADDR, DESCR, FIRST_DATE, LAST_DATE, TID, PR_DEL) values (ID__SEQ.nextval, 'MOWKB1H', 'DEFAULT', sysdate - 10, sysdate + 10, TID__SEQ.nextval, 0)")

{<?xml version='1.0' encoding='CP866'?>
<term>
<query handle='0' id='libra' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='RU' term_id='2479792165'>
  <request>
    <libra_id>2</libra_id>
    <data>&lt;?xml version='1.0' encoding='utf-8'?&gt;&lt;root name='astra_call'&gt;&lt;func&gt;load_tlg&lt;/func&gt;&lt;args&gt;&lt;text&gt;This is telegram text&lt;/text&gt;&lt;/args&gt;&lt;/root&gt;</data>
  </request>
</query>
</term>}


%%
###
#   Описание: astra_call/bad
#
###
#########################################################################################

$(desc_test 3)

$(init_term)

{<?xml version='1.0' encoding='CP866'?>
<term>
<query handle='0' id='libra' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='RU' term_id='2479792165'>
  <request>
    <libra_id>2</libra_id>
    <data>&lt;?xml version='1.0' encoding='utf-8'?&gt;&lt;root name='astra_call'&gt;&lt;func&gt;bad&lt;/func&gt;&lt;/root&gt;</data>
  </request>
</query>
</term>}


%%
###
#   Описание: astra_call/get_user_info
#
###
#########################################################################################

$(desc_test 4)

$(init_term)

{<?xml version='1.0' encoding='CP866'?>
<term>
<query handle='0' id='libra' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='RU' term_id='2479792165'>
  <request>
    <libra_id>2</libra_id>
    <data>&lt;?xml version='1.0' encoding='utf-8'?&gt;&lt;root name='astra_call'&gt;&lt;func&gt;get_user_info&lt;/func&gt;&lt;/root&gt;</data>
  </request>
</query>
</term>}


%%
###
#   Описание: astra_call/load_doc
#
###
#########################################################################################

$(desc_test 5)

$(init_term)

{<?xml version='1.0' encoding='CP866'?>
<term>
<query handle='0' id='libra' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='RU' term_id='2479792165'>
  <request>
    <libra_id>2</libra_id>
    <data>&lt;?xml version='1.0' encoding='utf-8'?&gt;&lt;root name='astra_call'&gt;&lt;func&gt;load_doc&lt;/func&gt;&lt;args&gt;&lt;point_id&gt;4853359&lt;/point_id&gt;&lt;type&gt;LOADSHEET&lt;/type&gt;&lt;content&gt;L O A D S H E E T&lt;/content&gt;&lt;/args&gt;&lt;/root&gt;</data>
  </request>
</query>
</term>}

??
$(pg_dump_table WB_MSG fields="point_id, msg_type, source" display="on")

>> lines=auto
[4853359] [LOADSHEET] [LIBRA] $()

??
$(pg_dump_table WB_MSG_TEXT fields="page_no, text" display="on")
>> lines=auto
[1] [L O A D S H E E T] $()

%%
###
#   Описание: DEFERRED astra_call/load_tlg
#
###
#########################################################################################

$(desc_test 6)

$(init)

$(sql "insert into TYPEB_ORIGINATORS(ID, ADDR, DESCR, FIRST_DATE, LAST_DATE, TID, PR_DEL) values (ID__SEQ.nextval, 'MOWKB1H', 'DEFAULT', sysdate - 10, sysdate + 10, TID__SEQ.nextval, 0)")

$(sql {begin LIBRA.DEFERR_ASTRA_CALL2(to_clob('<?xml version=\"1.0\" encoding=\"utf-8\"?><root name=\"astra_call\"><func>load_tlg</func><args><text>This is telegram text</text></args></root>')); end;})

$(run_daemon astra_calls_handler)

??
$(dump_table DEFERRED_ASTRA_CALLS display="on" fields="time_handle")

>> lines=auto
[xx.xx.xx xx:xx:xx,xxxxxx] $()


$(run_daemon astra_calls_cleaner)

??
$(dump_table DEFERRED_ASTRA_CALLS display="on" fields="time_handle")

>> lines=auto
[xx.xx.xx xx:xx:xx,xxxxxx] $()


$(sql {update DEFERRED_ASTRA_CALLS set TIME_HANDLE = TIME_HANDLE - 1})

$(run_daemon astra_calls_cleaner)

??
$(dump_table DEFERRED_ASTRA_CALLS display="on" fields="time_handle")

>> mode=regex
.*ND DEFERRED_ASTRA_CALLS DUMP COUNT=0.*


%%
###
#   Описание: get_schedule
#
###
#########################################################################################

$(desc_test 7)

$(init_term)

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set awk_UT 226)

$(sql "begin LIBRA.WRITE_AHM_LOG_MSG($(get awk_UT), null,  null, 'ОПЕРАТОР AHM', 'МОВРОМ', 'СООБЩЕНИЕ 1 В ЖУРНАЛ ОТ РЕДАКТОРА AHM'); end;")
$(sql "begin LIBRA.WRITE_AHM_LOG_MSG($(get awk_UT), 'DOW', null, 'ОПЕРАТОР AHM', 'МОВРОМ', 'СООБЩЕНИЕ 2 В ЖУРНАЛ ОТ РЕДАКТОРА AHM'); end;")
$(sql "begin LIBRA.WRITE_AHM_LOG_MSG($(get awk_UT), 'TOW', 'ZZ-738', 'ОПЕРАТОР AHM', 'МОВРОМ', 'СООБЩЕНИЕ 3 В ЖУРНАЛ ОТ РЕДАКТОРА AHM'); end;")
$(sql "begin LIBRA.WRITE_AHM_LOG_MSG($(get awk_UT), null,  'BB-321', 'ОПЕРАТОР AHM', 'МОВРОМ', 'СООБЩЕНИЕ 4 В ЖУРНАЛ ОТ РЕДАКТОРА AHM'); end;")
$(sql "begin LIBRA.WRITE_AHM_LOG_MSG($(get awk_UT), null,  null, 'ОПЕРАТОР AHM', 'МОВРОМ', 'СООБЩЕНИЕ 5 В ЖУРНАЛ ОТ РЕДАКТОРА AHM'); end;")
$(sql "begin LIBRA.WRITE_AHM_LOG_MSG($(get awk_UT), 'DOW', null, 'ОПЕРАТОР AHM', 'МОВРОМ', 'СООБЩЕНИЕ 6 В ЖУРНАЛ ОТ РЕДАКТОРА AHM'); end;")
$(sql "begin LIBRA.WRITE_AHM_LOG_MSG($(get awk_UT), 'TOW', 'ZZ-738', 'ОПЕРАТОР AHM', 'МОВРОМ', 'СООБЩЕНИЕ 7 В ЖУРНАЛ ОТ РЕДАКТОРА AHM'); end;")
$(sql "begin LIBRA.WRITE_AHM_LOG_MSG($(get awk_UT), null,  'BB-321', 'ОПЕРАТОР AHM', 'МОВРОМ', 'СООБЩЕНИЕ 8 В ЖУРНАЛ ОТ РЕДАКТОРА AHM'); end;")

$(sql "begin LIBRA.WRITE_BALANCE_LOG_MSG($(get point_dep), 'ЦЕНТРОВЩИК', 'МОВРОМ', 'СООБЩЕНИЕ В ЖУРНАЛ РЕЙСА ОТ КАЛЬКУЛЯТОРА ЦЕНТРОВКИ'); end;")

!! capture=on
$(GET_EVENTS $(get point_dep))

>> lines=auto
        <msg>СООБЩЕНИЕ В ЖУРНАЛ РЕЙСА ОТ КАЛЬКУЛЯТОРА ЦЕНТРОВКИ</msg>

??
$(dump_table AHM_DICT display="on" fields="AIRLINE, CATEGORY, BORT_NUM" order="ID")
>> lines=auto
[226] [NULL] [NULL] $()
[226] [DOW] [NULL] $()
[226] [TOW] [ZZ-738] $()
[226] [NULL] [BB-321] $()


%%
###
#   Описание: get_schedule
#
###
#########################################################################################

$(desc_test 8)

$(init_term)

include(ts/spp/write_dests_macro.ts)
include(ts/spp/write_trips_macro.ts)
include(ts/spp/read_trips_macro.ts)
include(ts/pax/checkin_macro.ts)
include(ts/pax/boarding_macro.ts)

$(init_term)
$(set_user_time_type LocalAirp PIKE)

$(set airline UT)
$(set flt_no 280)
$(set craft TU5)
$(set airp_dep DME)
$(set time_dep "$(date_format %d.%m.%Y -1) 07:00")
$(set time_arv "$(date_format %d.%m.%Y -1) 10:00")
$(set airp_arv AER)

$(cache PIKE RU MISC_SET $(cache_iface_ver MISC_SET) ""
  insert type_code:66
         airline:$(get_elem_id etAirline $(get airline))
         pr_misc:1)

$(http_forecast content=$(utf8
{<result>
    <status>OK</status>
    <answer>
        <root>
            <row num=\"1\">
                <plan_id type=\"int\">561</plan_id>
            </row>
        </root>
    </answer>
</result>}))

$(http_forecast content=$(utf8
{<result>
    <status>OK</status>
    <answer>
        <root>
            <row num=\"1\">
                <comp_id type=\"int\">461</comp_id>
                <f type=\"int\">0</f>
                <c type=\"int\">30</c>
                <y type=\"int\">270</y>
                <invalid_class type=\"int\">0</invalid_class>
            </row>
        </root>
    </answer>
</result>}))

$(http_forecast content=$(utf8
{<result>
    <status>OK</status>
    <answer>
        <root>
            <row num=\"1\">
                <comp_id type=\"int\">461</comp_id>
                <f type=\"int\">0</f>
                <c type=\"int\">30</c>
                <y type=\"int\">270</y>
                <invalid_class type=\"int\">0</invalid_class>
            </row>
        </root>
    </answer>
</result>}))

$(http_forecast content=$(utf8
{<result>
    <status>OK</status>
    <answer>
        <root>
            <row num=\"1\">
                <class_code type=\"str\">C</class_code>
                <first_row type=\"int\">1</first_row>
                <last_row type=\"int\">8</last_row>
            </row>
            <row num=\"2\">
                <class_code type=\"str\">Y</class_code>
                <first_row type=\"int\">9</first_row>
                <last_row type=\"int\">40</last_row>
            </row>
        </root>
    </answer>
</result>}))

$(http_forecast file="ts/libra/get_seat_props_response.xml")
$(http_forecast file="ts/libra/get_aisle_data_response.xml")

$(http_forecast content=$(utf8
{<result>
    <status>OK</status>
    <answer>
        <root>
            <row num=\"1\">
                <comp_id type=\"int\">461</comp_id>
                <f type=\"int\">0</f>
                <c type=\"int\">30</c>
                <y type=\"int\">270</y>
                <invalid_class type=\"int\">0</invalid_class>
            </row>
        </root>
    </answer>
</result>}))

$(set tomor $(date_format %d.%m.%Y +0))
$(NEW_SPP_FLIGHT_REQUEST
{ $(new_spp_point UT 111 100 44444 ""                   DME "$(get tomor) 07:00")
  $(new_spp_point_last             "$(get tomor) 15:00" TJM ) })

>>
GET /libra/get_plan_id?bort=44444 HTTP/1.1
Host: $()
Content-Type: application/xml; charset=utf-8
Content-Length: 0
$()
;;

>>
GET /libra/get_config?airline=%9E%92&bort=44444&plan_id=561 HTTP/1.1
Host: $()
Content-Type: application/xml; charset=utf-8
Content-Length: 0
$()
;;

>>
GET /libra/get_config?airline=%9E%92&bort=44444&plan_id=561 HTTP/1.1
Host: $()
Content-Type: application/xml; charset=utf-8
Content-Length: 0
$()
;;

>>
GET /libra/get_class_rows?conf_id=461 HTTP/1.1
Host: $()
Content-Type: application/xml; charset=utf-8
Content-Length: 0
$()
;;

>>
GET /libra/get_seat_props?plan_id=561 HTTP/1.1
Host: $()
Content-Type: application/xml; charset=utf-8
Content-Length: 0
$()
;;

>>
GET /libra/get_aisle_data?plan_id=561 HTTP/1.1
Host: $()
Content-Type: application/xml; charset=utf-8
Content-Length: 0
$()
;;

>>
GET /libra/get_config?airline=%9E%92&bort=44444&conf_id=461&plan_id=561 HTTP/1.1
Host: $()
Content-Type: application/xml; charset=utf-8
Content-Length: 0
$()
;;

??
$(dump_table TRIP_COMP_ELEMS display="on" order="x,y,yname")

>> lines=auto
...
[0] [Э] [К] [NULL] [1] [...] [NULL] [11] [K] [NULL] [15] [024] [NULL] $()
[0] [Б] [К] [NULL] [0] [...] [NULL] [13] [K] [NULL] [0] [001] [NULL] $()
[0] [Б] [К] [NULL] [0] [...] [NULL] [13] [K] [NULL] [1] [002] [NULL] $()
[0] [Б] [К] [NULL] [0] [...] [NULL] [13] [K] [NULL] [2] [003] [NULL] $()
[0] [Б] [К] [NULL] [0] [...] [NULL] [13] [K] [NULL] [3] [004] [NULL] $()
[0] [Б] [К] [NULL] [0] [...] [NULL] [13] [K] [NULL] [4] [005] [NULL] $()
[0] [Б] [К] [NULL] [0] [...] [NULL] [13] [K] [NULL] [5] [006] [NULL] $()
[0] [Б] [К] [NULL] [0] [...] [NULL] [13] [K] [NULL] [6] [007] [NULL] $()
...
------------------- END TRIP_COMP_ELEMS DUMP COUNT=244 -------------------
;;
