include(ts/macro.ts)
include(ts/adm_macro.ts)
include(ts/pax/checkin_macro.ts)

# meta: suite libra


###
#   Описание: get_schedule
#
###
#########################################################################################

$(desc_test 1)

$(settcl LIBRA_HTTP_HOST localhost)
$(settcl LIBRA_HTTP_PORT 8008)

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
#   Описание: AHM and BALANCE log
#
###
#########################################################################################

$(desc_test 6)

$(settcl LIBRA_HTTP_PORT 0)

$(init_term)

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set awk_UT 'ЮТ')

$(sql "begin LIBRA.WRITE_AHM_LOG_MSG($(get awk_UT), null,  null, 'ОПЕРАТОР AHM', 'МОВРОМ', 'СООБЩЕНИЕ 1 В ЖУРНАЛ ОТ РЕДАКТОРА AHM'); end;")
$(sql "begin LIBRA.WRITE_AHM_LOG_MSG($(get awk_UT), 'DOW', null, 'ОПЕРАТОР AHM', 'МОВРОМ', 'СООБЩЕНИЕ 2 В ЖУРНАЛ ОТ РЕДАКТОРА AHM'); end;")
$(sql "begin LIBRA.WRITE_AHM_LOG_MSG($(get awk_UT), 'TOW', 'ZZ-738', 'ОПЕРАТОР AHM', 'МОВРОМ', 'СООБЩЕНИЕ 3 В ЖУРНАЛ ОТ РЕДАКТОРА AHM'); end;")
$(sql "begin LIBRA.WRITE_AHM_LOG_MSG($(get awk_UT), null,  'BB-321', 'ОПЕРАТОР AHM', 'МОВРОМ', 'СООБЩЕНИЕ 4 В ЖУРНАЛ ОТ РЕДАКТОРА AHM'); end;")
$(sql "begin LIBRA.WRITE_AHM_LOG_MSG($(get awk_UT), null,  null, 'ОПЕРАТОР AHM', 'МОВРОМ', 'СООБЩЕНИЕ 5 В ЖУРНАЛ ОТ РЕДАКТОРА AHM'); end;")
$(sql "begin LIBRA.WRITE_AHM_LOG_MSG($(get awk_UT), 'DOW', null, 'ОПЕРАТОР AHM', 'МОВРОМ', 'СООБЩЕНИЕ 6 В ЖУРНАЛ ОТ РЕДАКТОРА AHM'); end;")
$(sql "begin LIBRA.WRITE_AHM_LOG_MSG($(get awk_UT), 'TOW', 'ZZ-738', 'ОПЕРАТОР AHM', 'МОВРОМ', 'СООБЩЕНИЕ 7 В ЖУРНАЛ ОТ РЕДАКТОРА AHM'); end;")
$(sql "begin LIBRA.WRITE_AHM_LOG_MSG($(get awk_UT), null,  'BB-321', 'ОПЕРАТОР AHM', 'МОВРОМ', 'СООБЩЕНИЕ 8 В ЖУРНАЛ ОТ РЕДАКТОРА AHM'); end;")

$(sql "begin LIBRA.WRITE_BALANCE_LOG_MSG($(get point_dep), 'ЦЕНТРОВЩИК', 'МОВРОМ', 'СООБЩЕНИЕ В ЖУРНАЛ РЕЙСА ОТ КАЛЬКУЛЯТОРА ЦЕНТРОВКИ'); end;")

$(run_daemon libra_log_events_handler)

!! capture=on
$(GET_EVENTS $(get point_dep))

>> lines=auto
        <msg>СООБЩЕНИЕ В ЖУРНАЛ РЕЙСА ОТ КАЛЬКУЛЯТОРА ЦЕНТРОВКИ</msg>

??
$(db_dump_table AHM_DICT display="on" fields="AIRLINE, CATEGORY, BORT_NUM" order="ID")
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

$(desc_test 7)

$(settcl LIBRA_HTTP_HOST localhost)
$(settcl LIBRA_HTTP_PORT 8008)

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
Host: localhost
Content-Type: application/xml; charset=utf-8
Content-Length: 0
$()
;;

>>
GET /libra/get_config?airline=%9E%92&bort=44444&plan_id=561 HTTP/1.1
Host: localhost
Content-Type: application/xml; charset=utf-8
Content-Length: 0
$()
;;

>>
GET /libra/get_config?airline=%9E%92&bort=44444&plan_id=561 HTTP/1.1
Host: localhost
Content-Type: application/xml; charset=utf-8
Content-Length: 0
$()
;;

>>
GET /libra/get_class_rows?conf_id=461 HTTP/1.1
Host: localhost
Content-Type: application/xml; charset=utf-8
Content-Length: 0
$()
;;

>>
GET /libra/get_seat_props?plan_id=561 HTTP/1.1
Host: localhost
Content-Type: application/xml; charset=utf-8
Content-Length: 0
$()
;;

>>
GET /libra/get_aisle_data?plan_id=561 HTTP/1.1
Host: localhost
Content-Type: application/xml; charset=utf-8
Content-Length: 0
$()
;;

>>
GET /libra/get_config?airline=%9E%92&bort=44444&conf_id=461&plan_id=561 HTTP/1.1
Host: localhost
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
;;

%%
###
#   Описание: get_seating_details
#
###
#########################################################################################

$(desc_test 8)

$(init_term)
$(set_user_time_type LocalAirp PIKE)

$(PREPARE_SEASON_SCD_TRANSIT ЮТ СОЧ АМС ПРХ 298) #сочи АМСТЕРДАМ прага
$(make_spp)

$(INB_PNL_UT СОЧ ПРХ 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get_next_trip_point_id $(get point_dep))))
$(set point_arv1 $(get_next_trip_point_id $(get point_dep)))
$(auto_set_craft $(get point_dep))
$(set comp_id $(get_comp_id $(get point_dep)))

$(deny_ets_interactive ЮТ 298 СОЧ)

$(set pax_id $(get_pax_id $(get point_dep) CHEKMAREV "KONSTANTIN ALEKSANDROVICH"))
$(set pax_id_chekmarev $(get pax_id))

!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ CHEKMAREV KONSTANTIN 2986145143701 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

$(set grp_id1 $(get_single_grp_id $(get pax_id)))

$(CHANGE_TCHECKIN_REQUEST capture=off lang=EN hall=1
{$(CHANGE_CHECKIN_SEGMENT $(get point_dep) $(get point_arv) СОЧ ПРХ
                          $(get grp_id1) $(get_single_grp_tid $(get pax_id))
{<passengers>
$(CHANGE_CHECKIN $(get pax_id) CHEKMAREV KONSTANTIN ВЗ Э bag_pool_num=1)
</passengers>})
#$(CHANGE_CHECKIN_SEGMENT $(get point_dep) $(get point_arv) СОЧ ПРХ
#                         $(get grp_id1) $(get_single_grp_tid $(get pax_id))
#)
}
{<value_bags/>
<bags>
$(BAG_WT 1 "" ЮТ pr_cabin=1 amount=4 weight= 6 bag_pool_num=1)
$(BAG_WT 2 "" ЮТ pr_cabin=0 amount=1 weight=100 bag_pool_num=1)
</bags>
<tags pr_print=\"0\">
$(TAG  1 RUCH 0000000001 bag_num=2)
</tags>}
)

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))
$(set pax_id_tumali $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

#группа до АМС
$(set pax_id_adl1 $(get_pax_id $(get point_dep) КОТЛЯР "ВЛАДИМИР НИКОЛАЕВИЧ"))
$(set pax_id_adl2 $(get_pax_id $(get point_dep) AGAFONOV "DENIS DMITRIEVICH"))

$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv1) СОЧ АМС hall=1 capture=off
{<passengers>
$(NEW_CHECKIN $(get pax_id_adl1) КОТЛЯР "ВЛАДИМИР НИКОЛАЕВИЧ" ВЗ 1 Э "")
$(NEW_CHECKIN $(get pax_id_adl2) AGAFONOV "DENIS DMITRIEVICH" ВЗ 1 Э "")
</passengers>})

$(set grp_id2 $(get_single_grp_id $(get pax_id_adl2)))

$(CHANGE_TCHECKIN_REQUEST capture=off lang=EN hall=1
{$(CHANGE_CHECKIN_SEGMENT $(get point_dep) $(get point_arv1) СОЧ АМС
                          $(get grp_id2) $(get_single_grp_tid $(get pax_id_adl1))
{<passengers>
$(CHANGE_CHECKIN $(get pax_id_adl1) КОТЛЯР "ВЛАДИМИР НИКОЛАЕВИЧ" ВЗ Э bag_pool_num=1)
$(CHANGE_CHECKIN $(get pax_id_adl2) AGAFONOV "DENIS DMITRIEVICH" ВЗ Э bag_pool_num=2)
</passengers>})
}
{<value_bags/>
<bags>
$(BAG_WT 1 "" ЮТ pr_cabin=1 amount=2  weight=13   bag_pool_num=1)
$(BAG_WT 2 "" ЮТ pr_cabin=0 amount=1  weight=44   bag_pool_num=1)
$(BAG_WT 3 "" ЮТ pr_cabin=0 amount=10 weight=75   bag_pool_num=2)
</bags>
<tags pr_print=\"0\">
$(TAG  1 RUCH 1100000044 bag_num=2)
$(TAG  2 RUCH 1000000001 bag_num=3)
$(TAG  3 RUCH 1000000009 bag_num=3)
$(TAG  4 RUCH 1000000010 bag_num=3)
$(TAG  5 RUCH 1000000099 bag_num=3)
$(TAG  6 RUCH 1000000101 bag_num=3)
$(TAG  7 RUCH 1000000102 bag_num=3)
$(TAG  8 DON  1000000103 bag_num=3)
$(TAG  9 DON  1000000104 bag_num=3)
$(TAG 10 TEST 1000000105 bag_num=3)
$(TAG 11 TEST 1000000106 bag_num=3)
</tags>}
)

#несопровождаемый багаж
$(NEW_UNACCOMP_REQUEST capture=off lang=EN hall=1 ""
{$(NEW_UNACCOMP_SEGMENT $(get point_dep) $(get point_arv) СОЧ ПРХ)})

$(set grp_id_unacc1 $(get_unaccomp_id $(get point_dep) 1))

$(CHANGE_UNACCOMP_REQUEST capture=off lang=EN hall=1
{$(CHANGE_UNACCOMP_SEGMENT $(get point_dep) $(get point_arv) СОЧ ПРХ
                           $(get grp_id_unacc1) $(get_unaccomp_tid $(get grp_id_unacc1)))}
{<value_bags/>
<bags>
$(BAG_WT 1 "" ЮТ pr_cabin=1 amount=1  weight=11  bag_pool_num=1)
$(BAG_WT 2 "" ЮТ pr_cabin=0 amount=3  weight=24  bag_pool_num=1)
</bags>
<tags pr_print=\"0\">
$(TAG 1 RUCH 1298401555 bag_num=2 color=СИ)
$(TAG 2 RUCH 1298401556 bag_num=2 color=Ж)
$(TAG 3 RUCH 0298401557 bag_num=2 color=О)
</tags>}
)

$(NEW_UNACCOMP_REQUEST capture=off lang=EN hall=2 ""
{$(NEW_UNACCOMP_SEGMENT $(get point_dep) $(get point_arv1) СОЧ АМС)})

$(set grp_id_unacc2 $(get_unaccomp_id $(get point_dep) 2))

$(CHANGE_UNACCOMP_REQUEST capture=off lang=EN hall=2
{$(CHANGE_UNACCOMP_SEGMENT $(get point_dep) $(get point_arv1) СОЧ АМС
                           $(get grp_id_unacc2) $(get_unaccomp_tid $(get grp_id_unacc2)))}
{<value_bags/>
<bags>
$(BAG_WT 1 "" ЮТ pr_cabin=1 amount=1  weight=4  bag_pool_num=1)
$(BAG_WT 2 "" ЮТ pr_cabin=0 amount=3  weight=5  bag_pool_num=1)
</bags>
<tags pr_print=\"0\">
$(TAG 1 RUCH 1298401558 bag_num=2 color=СИ)
$(TAG 2 RUCH 1298401559 bag_num=2 color=Ж)
$(TAG 3 RUCH 0298401560 bag_num=2 color=О)
</tags>}
)

#одного пакса с багажем в БИЗНЕС КЛАСС
$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv1) СОЧ АМС hall=1 class=Б
{
<passengers>
  <pax>
$(NEW_CHECKIN_NOREC PUPKIN VASYA ВЗ 1 Б
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

$(set pax_id_pupkin $(get_pax_id $(get point_dep) 5))
$(set grp_id3 $(get_single_grp_id $(get pax_id_pupkin)))

$(CHANGE_TCHECKIN_REQUEST capture=off lang=EN hall=1
{$(CHANGE_CHECKIN_SEGMENT $(get point_dep) $(get point_arv1) СОЧ АМС
                          $(get grp_id3) $(get_single_grp_tid $(get pax_id_pupkin))
{<passengers>
$(CHANGE_CHECKIN $(get pax_id_pupkin) PUPKIN VASYA ВЗ Б bag_pool_num=1)
</passengers>})
#$(CHANGE_CHECKIN_SEGMENT $(get point_dep) $(get point_arv1) СОЧ АМС
#                         $(get grp_id3) $(get_single_grp_tid $(get pax_id_pupkin))
#)
}
{<value_bags/>
<bags>
$(BAG_WT 1 "" ЮТ pr_cabin=1 amount=1 weight=34 bag_pool_num=1)
$(BAG_WT 2 "" ЮТ pr_cabin=0 amount=1 weight=21 bag_pool_num=1)
</bags>
<tags pr_print=\"0\">
$(TAG  1 RUCH 0000020001 bag_num=2)
</tags>}
)

#эмуляция режима Libra
$(sql "INSERT INTO libra_comps(plan_id, conf_id, comp_id, time_create, time_change) VALUES(1, 1, $(get comp_id), TO_DATE('$(date_format %Y-%m-%d)','yyyy-mm-dd'),TO_DATE('$(date_format %Y-%m-%d)','yyyy-mm-dd'))")
$(sql "UPDATE comps SET bort='LIBRA' WHERE comp_id=$(get comp_id)")
$(sql "UPDATE points SET bort='LIBRA' WHERE point_id=$(get point_dep)")

$(cache PIKE RU MISC_SET $(cache_iface_ver MISC_SET) ""
  insert type_code:66
         airline:$(get_elem_id etAirline ЮТ)
         pr_misc:1)

!! capture=on
{<?xml version='1.0' encoding='CP866'?>
<term>
<query handle='0' id='libra' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='RU' term_id='2479792165'>
  <request>
    <libra_id>3</libra_id>
    <data>&lt;?xml version='1.0' encoding='utf-8'?&gt;&lt;root name='astra_call'&gt;&lt;func&gt;get_seating_details&lt;/func&gt;&lt;args&gt;&lt;point_id&gt;$(get point_dep)&lt;/point_id&gt;&lt;/args&gt;&lt;/root&gt;</data>
  </request>
</query>
</term>}

>> lines=auto
&lt;root name='astra_call' func='get_seating_details' result='ok'&gt;...
  &lt;details&gt;
    &lt;compon plan_id='1' conf_id='1' comp_id='...'...&gt;
      &lt;persons&gt;
        &lt;code name='Adult'&gt;A&lt;/code&gt;
        &lt;code name='Male'&gt;M&lt;/code&gt;
      &lt;/persons&gt;
      &lt;layers&gt;
        &lt;code name='Рассадка при регистрации'&gt;CHECKIN&lt;/code&gt;
      &lt;/layers&gt;
      &lt;seats&gt;
        &lt;passenger pax_id='...'&gt;
          &lt;gender&gt;A&lt;/gender&gt;
          &lt;airp_dep&gt;СОЧ&lt;/airp_dep&gt;
          &lt;airp_arv&gt;ПРХ&lt;/airp_arv&gt;
          &lt;rkamount&gt;4&lt;/rkamount&gt;
          &lt;rkweight&gt;6&lt;/rkweight&gt;
          &lt;class&gt;Э&lt;/class&gt;
          &lt;pax_seats&gt;
            &lt;item&gt;
              ...
              ...
              &lt;layer&gt;CHECKIN&lt;/layer&gt;
            &lt;/item&gt;
          &lt;/pax_seats&gt;
        &lt;/passenger&gt;
        &lt;passenger pax_id='...'&gt;
          &lt;gender&gt;M&lt;/gender&gt;
          &lt;airp_dep&gt;СОЧ&lt;/airp_dep&gt;
          &lt;airp_arv&gt;ПРХ&lt;/airp_arv&gt;
          &lt;class&gt;Э&lt;/class&gt;
          &lt;pax_seats&gt;
            &lt;item&gt;
              ...
              ...
              &lt;layer&gt;CHECKIN&lt;/layer&gt;
            &lt;/item&gt;
          &lt;/pax_seats&gt;
        &lt;/passenger&gt;
        &lt;passenger pax_id='...'&gt;
          &lt;gender&gt;A&lt;/gender&gt;
          &lt;airp_dep&gt;СОЧ&lt;/airp_dep&gt;
          &lt;airp_arv&gt;АМС&lt;/airp_arv&gt;
          &lt;rkamount&gt;2&lt;/rkamount&gt;
          &lt;rkweight&gt;13&lt;/rkweight&gt;
          &lt;class&gt;Э&lt;/class&gt;
          &lt;pax_seats&gt;
            &lt;item&gt;
              ...
              ...
              &lt;layer&gt;CHECKIN&lt;/layer&gt;
            &lt;/item&gt;
          &lt;/pax_seats&gt;
        &lt;/passenger&gt;
        &lt;passenger pax_id='...'&gt;
          &lt;gender&gt;A&lt;/gender&gt;
          &lt;airp_dep&gt;СОЧ&lt;/airp_dep&gt;
          &lt;airp_arv&gt;АМС&lt;/airp_arv&gt;
          &lt;class&gt;Э&lt;/class&gt;
          &lt;pax_seats&gt;
            &lt;item&gt;
              ...
              ...
              &lt;layer&gt;CHECKIN&lt;/layer&gt;
            &lt;/item&gt;
          &lt;/pax_seats&gt;
        &lt;/passenger&gt;
        &lt;passenger pax_id='...'&gt;
          &lt;gender&gt;A&lt;/gender&gt;
          &lt;airp_dep&gt;СОЧ&lt;/airp_dep&gt;
          &lt;airp_arv&gt;АМС&lt;/airp_arv&gt;
          &lt;rkamount&gt;1&lt;/rkamount&gt;
          &lt;rkweight&gt;34&lt;/rkweight&gt;
          &lt;class&gt;Б&lt;/class&gt;
          &lt;pax_seats&gt;
            &lt;item&gt;
              ...
              ...
              &lt;layer&gt;CHECKIN&lt;/layer&gt;
            &lt;/item&gt;
          &lt;/pax_seats&gt;
        &lt;/passenger&gt;
      &lt;/seats&gt;
      &lt;baggage&gt;
        &lt;dest val='АМС'&gt;
          &lt;class bagamount='1' bagweight='21'&gt;Б&lt;/class&gt;
          &lt;class bagamount='11' bagweight='119'&gt;Э&lt;/class&gt;
        &lt;/dest&gt;
        &lt;dest val='ПРХ'&gt;
          &lt;class bagamount='1' bagweight='100'&gt;Э&lt;/class&gt;
        &lt;/dest&gt;
      &lt;/baggage&gt;
      &lt;unaccompanied&gt;
        &lt;airp_arv baggage='5' rk='4'&gt;АМС&lt;/airp_arv&gt;
        &lt;airp_arv baggage='24' rk='11'&gt;ПРХ&lt;/airp_arv&gt;
      &lt;/unaccompanied&gt;
    &lt;/compon&gt;
  &lt;/details&gt;


%%
###
#   Описание: libra call via http
#
###
#########################################################################################

$(desc_test 9)

$(settcl LIBRA_HTTP_HOST localhost)
$(settcl LIBRA_HTTP_PORT 8008)

$(init_term)

$(http_forecast content="<result><status>OK</status><answer></answer></result>")

$(http_forecast content="<result><status>OK</status><answer><root AP2='VKO$(lf)LED'></root></answer></result>")

$(http_forecast content="<result><status>OK</status><answer><root AP2='VKO$(lf)LED'/></answer></result>")


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

$(KICK_IN)

>> lines=auto
    <data/>

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

$(KICK_IN)

>> lines=auto
    <data>&lt;?xml version='1.0' encoding='CP866'?&gt;&lt;root AP2='VKO
LED'&gt;&lt;/root&gt;</data>


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

$(KICK_IN)

>> lines=auto
    <data>&lt;?xml version='1.0' encoding='CP866'?&gt;&lt;root AP2='VKO
LED'/&gt;</data>


%%
###
#   Описание: AHM and BALANCE log via http
#
###
#########################################################################################

$(desc_test 10)

$(settcl LIBRA_HTTP_HOST localhost)
$(settcl LIBRA_HTTP_PORT 8008)

$(init_term)

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

###
## первый ответ на чтение - с данными
###

$(http_forecast content=$(utf8
{<result>
    <status>OK</status>
    <answer>
        <root>
            <row num=\"1\">
                <log_type>AHM</log_type>
                <airline>ЮТ</airline>
                <bort_num>VQ-BCP</bort_num>
                <category/>
                <point_id type=\"int\" null=\"true\"/>
                <rus_msg>СООБЩЕНИЕ 1 НА РУССКОМ</rus_msg>
                <lat_msg>MESSAGE 1 ON ENGLISH</lat_msg>
                <log_time type=\"date\" format='ddmmyyyy hh:nn:ss'>17082021 19:10:05</log_time>
                <ev_user>Петров</ev_user>
                <ev_station>Стойка</ev_station>
            </row>
            <row num=\"2\">
                <log_type>BAL</log_type>
                <airline/>
                <bort_num/>
                <category/>
                <point_id type=\"int\">$(get point_dep)</point_id>
                <rus_msg>СООБЩЕНИЕ 2 НА РУССКОМ</rus_msg>
                <lat_msg>MESSAGE 2 ON ENGLISH</lat_msg>
                <log_time type=\"date\" format='ddmmyyyy hh:nn:ss'>17082021 19:12:04</log_time>
                <ev_user>Сидоров</ev_user>
                <ev_station>Комп</ev_station>
            </row>
        </root>
    </answer>
</result>}))


###
## второй ответ на чтение - пустой
###

$(http_forecast content=$(utf8
{<result>
    <status>OK</status>
    <answer>
        <root/>
    </answer>
</result>}))


###
## третий ответ на чтение - с ошибкой
###

$(http_forecast content=$(utf8
{<result>
    <status>ERR</status>
    <code>DB-01</code>
    <reason>Exception happened</reason>
</result>}))


###
## ответ на удаление
###

$(http_forecast content=$(utf8
{<result>
    <status>OK</status>
    <answer>
      <root>
        <row num=\"1\">
            <rowcount type=\"int\">0</rowcount>
        </row>
      </root>
    </answer>
</result>}))


$(run_daemon libra_log_events_handler)

>> lines=auto
GET /libra/get_log_events?from=xxxxxxxx_xxxxxx&to=xxxxxxxx_xxxxxx HTTP/1.1


$(run_daemon libra_log_events_handler)

>> lines=auto
GET /libra/get_log_events?from=xxxxxxxx_xxxxxx&to=xxxxxxxx_xxxxxx HTTP/1.1


!! capture=on
$(GET_EVENTS $(get point_dep))

>> lines=auto
        <msg>СООБЩЕНИЕ 2 НА РУССКОМ</msg>


$(run_daemon libra_log_events_handler)
>> lines=auto
GET /libra/get_log_events?from=xxxxxxxx_xxxxxx&to=xxxxxxxx_xxxxxx HTTP/1.1


# $(run_daemon libra_log_events_cleaner)
