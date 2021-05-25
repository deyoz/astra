include(ts/macro.ts)
include(ts/adm_macro.ts)
include(ts/spp/write_dests_macro.ts)
include(ts/spp/write_trips_macro.ts)
include(ts/pnl/pnl_ut_580_461.ts)
include(ts/pax/checkin_macro.ts)

# meta: suite mob_payment

$(defmacro PREPARE_MOBILE_PAYMENT
{

### доступ для входящих http-запросов от мобильной регистрации

$(CREATE_USER AERMPY AERMPY)
$(CREATE_DESK AERMPY 1)
$(ADD_HTTP_CLIENT MOBILE_PAYMENT AERMPY AERMPY AERMPY)

$(set http_heading
{POST / HTTP/1.1
Host: /
Accept-Encoding: gzip,deflate
CLIENT-ID: AERMPY
Content-Type: text/xml;charset=UTF-8
})

})

$(defmacro GET_CLIENT_PERMS_REQUEST
  http_heading
  lang=RU
{
!! capture=on http_heading=$(http_heading)
<?xml version='1.0' encoding='CP866'?>
<term>
  <query lang='$(lang)'>
    <get_client_perms/>
  </query>
</term>

})

$(defmacro GET_CLIENT_PERMS_RESPONSE
  content
  lang=RU
{
>> lines=auto
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer lang='$(lang)' handle='0'>\
$(if $(eq $(content) "") {
    <get_client_perms/>} {
    <get_client_perms>
$(content)
    </get_client_perms>})
  </answer>
</term>

})

$(defmacro xml_param
  name
  value
{$(if $(eq $(value) "undetermined") ""
{$(if $(eq $(value) "") {
      <$(name)/>} {
      <$(name)>$(value)</$(name)>})})})

$(defmacro SEARCH_FLIGHTS_REQUEST
  http_heading
  departure          =undetermined
  search_depth       =undetermined
  departure_datetime =undetermined
  carrier            =undetermined
  lang=RU
{
!! capture=on http_heading=$(http_heading)
<?xml version='1.0' encoding='CP866'?>
<term>
  <query lang='$(lang)'>
    <search_flights>\
$(xml_param departure $(departure))\
$(xml_param search_depth $(search_depth))\
$(xml_param departure_datetime $(departure_datetime))\
$(xml_param carrier $(carrier))
    </search_flights>
  </query>
</term>

})

$(defmacro SEARCH_FLIGHTS_RESPONSE
  content
  lang=RU
{
>> lines=auto
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer lang='$(lang)' handle='0'>\
$(if $(eq $(content) "") {
    <search_flights/>} {
    <search_flights>
$(content)
    </search_flights>})
  </answer>
</term>

})

$(defmacro SEARCH_FLIGHTS_ERROR
  message
  code=0
  lang=RU
{
>> lines=auto
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer lang='$(lang)' handle='0'>
    <search_flights>
      <error code='$(code)' message='$(message)'/>
    </search_flights>
  </answer>
</term>

})

$(defmacro SEARCH_PASSENGERS_REQUEST
  http_heading
  departure           =undetermined
  barcode             =undetermined
  carrier             =undetermined
  flight_no           =undetermined
  departure_date_scd  =undetermined
  destination         =undetermined
  recloc              =undetermined
  ticket_no           =undetermined
  search_depth        =undetermined
  document_no         =undetermined
  lastname            =undetermined
  lang=RU
{
!! capture=on http_heading=$(http_heading)
<?xml version='1.0' encoding='CP866'?>
<term>
  <query lang='$(lang)'>
    <search_passengers>\
$(xml_param departure $(departure))\
$(xml_param barcode $(barcode))\
$(xml_param carrier $(carrier))\
$(xml_param flight_no $(flight_no))\
$(xml_param departure_date_scd $(departure_date_scd))\
$(xml_param destination $(destination))\
$(xml_param recloc $(recloc))\
$(xml_param ticket_no $(ticket_no))\
$(xml_param search_depth $(search_depth))\
$(xml_param document_no $(document_no))\
$(xml_param lastname $(lastname))
    </search_passengers>
  </query>
</term>

})

$(defmacro SEARCH_PASSENGERS_RESPONSE
  content
  lang=RU
{
>> lines=auto
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer lang='$(lang)' handle='0'>\
$(if $(eq $(content) "") {
    <search_passengers/>} {
    <search_passengers>
$(content)
    </search_passengers>})
  </answer>
</term>

})

$(defmacro SEARCH_PASSENGERS_ERROR
  message
  code=0
  lang=RU
{
>> lines=auto
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer lang='$(lang)' handle='0'>
    <search_passengers>
      <error code='$(code)' message='$(message)'/>
    </search_passengers>
  </answer>
</term>

})



$(defmacro PREPARE_FLIGHTS
  date1
  date2
  date3
{

$(set now+2h $(date_format {%d.%m.%Y %H:%M} +2h))
$(set now+3h $(date_format {%d.%m.%Y %H:%M} +3h))
$(set now+4h $(date_format {%d.%m.%Y %H:%M} +4h))
$(set now+5h $(date_format {%d.%m.%Y %H:%M} +5h))
$(set now+7h $(date_format {%d.%m.%Y %H:%M} +7h))
$(set now+8h $(date_format {%d.%m.%Y %H:%M} +8h))

$(NEW_SPP_FLIGHT_REQUEST
{ $(new_spp_point S7 371 777 "" ""            ВНК $(get now+2h))
  $(new_spp_point_last          $(get now+4h) СОЧ ) })

$(NEW_SPP_FLIGHT_REQUEST
{ $(new_spp_point SU 553 321 "" ""            ВНК $(get now+5h))
  $(new_spp_point_last          $(get now+8h) ЧЛБ ) })

$(NEW_SPP_FLIGHT_REQUEST
{ $(new_spp_point U6 159 737 "" ""            СОЧ $(get now+2h) suffix=D)
  $(new_spp_point U6 159 737 "" $(get now+3h) ВНК $(get now+3h) suffix=D)
  $(new_spp_point U6 159 737 "" $(get now+4h) LED $(get now+5h) suffix=D)
  $(new_spp_point_last           $(get now+7h) КГД ) })

$(NEW_SPP_FLIGHT_REQUEST
{ $(new_spp_point ЮТ 580 TU3 65021 ""               СОЧ "$(date1) 12:00")
  $(new_spp_point_last             "$(date1) 15:00" ВНК ) })

$(NEW_SPP_FLIGHT_REQUEST
{ $(new_spp_point ЮТ 461 TU3 65021 ""               ВНК "$(date1) 16:00")
  $(new_spp_point_last             "$(date1) 21:20" РЩН ) })

$(NEW_SPP_FLIGHT_REQUEST
{ $(new_spp_point UT 804 TU3 65021 ""               VKO "$(date1) 22:30")
  $(new_spp_point_last             "$(date2) 01:15" LED ) })

$(NEW_SPP_FLIGHT_REQUEST
{ $(new_spp_point UT 804 TU3 65021 ""               VKO "$(date2) 22:30")
  $(new_spp_point_last             "$(date3) 01:15" LED ) })

$(NEW_SPP_FLIGHT_REQUEST
{ $(new_spp_point UT 298 TU3 65021 ""               VKO "$(date1) 15:30")
  $(new_spp_point_last             "$(date1) 16:45" PRG ) })

})

$(defmacro PAX_RESPONSE_1479_1
{      <passenger lastname='KOTOVA' name='IRINA' date_of_birth='01.05.1976' category='adult' gender='female' document_no='7774441110' pax_id='$(get pax_id_1479_1)' status='checked'>
        <flight carrier='ЮТ' flight_no='580' departure='СОЧ' destination='ВНК' departure_time='$(get tomor+0) 12:00' arrival_time='$(get tomor+0) 15:00' check_in_status='open' web_check_in_status='open' boarding_status='close'>
          <ticket>2982410821479</ticket>
          <reclocs>
            <recloc crs='DT'>04VSFC</recloc>
            <recloc crs='ЮТ'>054C82</recloc>
          </reclocs>
        </flight>
      </passenger>})

$(defmacro PAX_RESPONSE_1480_1
{      <passenger lastname='MOTOVA' name='IRINA' date_of_birth='01.05.1976' category='adult' gender='female' document_no='7774441110' pax_id='$(get pax_id_1480_1)' status='not_checked'>
        <flight carrier='ЮТ' flight_no='580' departure='СОЧ' destination='ВНК' departure_time='$(get tomor+0) 12:00' arrival_time='$(get tomor+0) 15:00' check_in_status='open' web_check_in_status='open' boarding_status='close'>
          <ticket>2982410821480</ticket>
          <reclocs>
            <recloc crs='DT'>04VSFC</recloc>
            <recloc crs='ЮТ'>054C82</recloc>
          </reclocs>
        </flight>
      </passenger>})

$(defmacro PAX_RESPONSE_1479_2
{      <passenger lastname='KOTOVA' name='IRINA' date_of_birth='01.05.1976' category='adult' gender='female' document_no='7774441110' pax_id='$(get pax_id_1479_2)' status='checked'>
        <flight carrier='ЮТ' flight_no='461' departure='ВНК' destination='РЩН' departure_time='$(get tomor+0) 16:00' arrival_time='$(get tomor+0) 21:20' check_in_status='close' web_check_in_status='close' boarding_status='close'>
          <ticket>2982410821479</ticket>
          <reclocs>
            <recloc crs='DT'>04VSFC</recloc>
            <recloc crs='ЮТ'>054C82</recloc>
          </reclocs>
        </flight>
      </passenger>})

$(defmacro PAX_RESPONSE_1480_2
{      <passenger lastname='MOTOVA' name='IRINA' date_of_birth='01.05.1976' category='adult' gender='female' document_no='7774441110' pax_id='$(get pax_id_1480_2)' status='not_checked'>
        <flight carrier='ЮТ' flight_no='461' departure='ВНК' destination='РЩН' departure_time='$(get tomor+0) 16:00' arrival_time='$(get tomor+0) 21:20' check_in_status='close' web_check_in_status='close' boarding_status='close'>
          <ticket>2982410821480</ticket>
          <reclocs>
            <recloc crs='DT'>04VSFC</recloc>
            <recloc crs='ЮТ'>054C82</recloc>
          </reclocs>
        </flight>
      </passenger>})


### test 1 - запрос get_client_perms с разными доступами
#########################################################################################

$(init_term)
$(set_user_time_type LocalAirp PIKE)

### доступ ко всем ап и ак

$(PREPARE_MOBILE_PAYMENT)

$(GET_CLIENT_PERMS_REQUEST $(get http_heading) lang=EN)

$(GET_CLIENT_PERMS_RESPONSE lang=EN
{      <points/>})


### доступ к определенным ап и всем ак

$(UPDATE_USER AERMPY AERMPY 0 lang=EN
{          <airps>
            <item>ВНК</item>
            <item>СОЧ</item>
          </airps>})

$(GET_CLIENT_PERMS_REQUEST $(get http_heading) lang=EN)

$(GET_CLIENT_PERMS_RESPONSE lang=EN
{      <points>
        <point>VKO</point>
        <point>AER</point>
      </points>})


### доступ ко всем ап и определенным ак

$(UPDATE_USER AERMPY AERMPY 0 lang=EN
{          <airlines>
            <item>ЮТ</item>
            <item>ФВ</item>
          </airlines>})

$(GET_CLIENT_PERMS_REQUEST $(get http_heading) lang=EN)

$(GET_CLIENT_PERMS_RESPONSE lang=EN
{      <points/>
      <carriers>
        <carrier>FV</carrier>
        <carrier>UT</carrier>
      </carriers>})


### доступ к определенным ап и ак

$(UPDATE_USER AERMPY AERMPY 0 lang=EN
{          <airps>
            <item>ВНК</item>
            <item>СОЧ</item>
          </airps>}
{          <airlines>
            <item>ЮТ</item>
            <item>ФВ</item>
          </airlines>})

$(GET_CLIENT_PERMS_REQUEST $(get http_heading) lang=RU)

$(GET_CLIENT_PERMS_RESPONSE lang=RU
{      <points>
        <point>ВНК</point>
        <point>СОЧ</point>
      </points>
      <carriers>
        <carrier>ФВ</carrier>
        <carrier>ЮТ</carrier>
      </carriers>})


### доступ к определенным ап без доступа к любой из ак

$(UPDATE_USER AERMPY AERMPY 2
{          <airps>
            <item>ВНК</item>
            <item>СОЧ</item>
          </airps>})

$(GET_CLIENT_PERMS_REQUEST $(get http_heading))

$(GET_CLIENT_PERMS_RESPONSE
{      <points/>})


### доступ к определенным ак без доступа к любому из ап

$(UPDATE_USER AERMPY AERMPY 1
{          <airlines>
            <item>ЮТ</item>
            <item>ФВ</item>
          </airlines>})

$(GET_CLIENT_PERMS_REQUEST $(get http_heading))

$(GET_CLIENT_PERMS_RESPONSE
{      <points/>})

%%

### test 2 - запрос search_flights с разными параметрами
#########################################################################################

$(init_term)
$(set_user_time_type LocalAirp PIKE)

$(PREPARE_MOBILE_PAYMENT)

$(UPDATE_USER AERMPY AERMPY 0 lang=EN
{          <airps>
            <item>ВНК</item>
            <item>СОЧ</item>
          </airps>})

$(SEARCH_FLIGHTS_REQUEST $(get http_heading))
$(SEARCH_FLIGHTS_ERROR "Node 'departure' does not exists")

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) "")
$(SEARCH_FLIGHTS_ERROR "Empty &lt;departure&gt;")

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) ZZ)
$(SEARCH_FLIGHTS_ERROR "Unknown &lt;departure&gt; 'ZZ'")

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) AER)
$(SEARCH_FLIGHTS_ERROR "Node 'search_depth' does not exists")

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) AER "")
$(SEARCH_FLIGHTS_ERROR "Wrong &lt;search_depth&gt;")

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) AER 49)
$(SEARCH_FLIGHTS_ERROR "Wrong &lt;search_depth&gt;")

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) AER 0)
$(SEARCH_FLIGHTS_ERROR "Wrong &lt;search_depth&gt;")

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) AER -1)
$(SEARCH_FLIGHTS_ERROR "Wrong &lt;search_depth&gt;")

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) AER 48)
$(SEARCH_FLIGHTS_RESPONSE)

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) СОЧ 1 "" "")
$(SEARCH_FLIGHTS_RESPONSE)

$(set wrong_departure_datetime $(date_format {%d.%m.%Y 23:00:00} +2))
$(SEARCH_FLIGHTS_REQUEST $(get http_heading) VKO 23 $(get wrong_departure_datetime) lang=RU)
$(SEARCH_FLIGHTS_ERROR "Wrong &lt;departure_datetime&gt; '$(get wrong_departure_datetime)'")

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) VKO 23 departure_datetime=ХРЕНЬ lang=RU)
$(SEARCH_FLIGHTS_ERROR "Wrong &lt;departure_datetime&gt; 'ХРЕНЬ'")

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) AER 48 carrier=ZZZZ)
$(SEARCH_FLIGHTS_ERROR "Unknown &lt;carrier&gt; 'ZZZZ'")

$(set tomor+1 $(date_format %d.%m.%Y +2))
$(set tomor+2 $(date_format %d.%m.%Y +3))
$(set tomor+3 $(date_format %d.%m.%Y +4))

$(PREPARE_FLIGHTS $(get tomor+1) $(get tomor+2) $(get tomor+3))

$(set point_dep $(get_point_dep_for_flight UT 804 "" $(yymmdd +2) ВНК))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(OPEN_WEB_CHECKIN $(get point_dep))
$(OPEN_CHECKIN $(get point_dep))

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) VKO 4 lang=RU)
$(SEARCH_FLIGHTS_RESPONSE
{      <flight carrier='С7' flight_no='371' departure='ВНК' check_in_status='close' web_check_in_status='close' boarding_status='close'>
        <departure_datetime_scd>$(get now+2h)</departure_datetime_scd>
        <destinations>
          <destination id='0'>СОЧ</destination>
        </destinations>
        <check_in_desks/>
        <gates/>
      </flight>
      <flight carrier='У6' flight_no='159Д' departure='ВНК' check_in_status='close' web_check_in_status='close' boarding_status='close'>
        <departure_datetime_scd>$(get now+3h)</departure_datetime_scd>
        <destinations>
          <destination id='0'>ПЛК</destination>
          <destination id='1'>КГД</destination>
        </destinations>
        <check_in_desks/>
        <gates/>
      </flight>})

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) VKO 4 carrier=U6 lang=EN)
$(SEARCH_FLIGHTS_RESPONSE lang=EN
{      <flight carrier='U6' flight_no='159D' departure='VKO' check_in_status='close' web_check_in_status='close' boarding_status='close'>
        <departure_datetime_scd>$(get now+3h)</departure_datetime_scd>
        <destinations>
          <destination id='0'>LED</destination>
          <destination id='1'>KGF</destination>
        </destinations>
        <check_in_desks/>
        <gates/>
      </flight>})

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) VKO 24 $(date_format {%d.%m.%Y 23:00} +1) U6 lang=EN)
$(SEARCH_FLIGHTS_RESPONSE lang=EN)

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) VKO 24 $(date_format {%d.%m.%Y 23:00} +1) ЮТ lang=EN)
$(SEARCH_FLIGHTS_RESPONSE lang=EN
{      <flight carrier='UT' flight_no='461' departure='VKO' check_in_status='close' web_check_in_status='close' boarding_status='close'>
        <departure_datetime_scd>$(get tomor+1) 16:00</departure_datetime_scd>
        <destinations>
          <destination id='0'>TJM</destination>
        </destinations>
        <check_in_desks/>
        <gates/>
      </flight>
      <flight carrier='UT' flight_no='804' departure='VKO' check_in_status='open' web_check_in_status='open' boarding_status='close'>
        <departure_datetime_scd>$(get tomor+1) 22:30</departure_datetime_scd>
        <destinations>
          <destination id='0'>LED</destination>
        </destinations>
        <check_in_desks/>
        <gates/>
      </flight>
      <flight carrier='UT' flight_no='298' departure='VKO' check_in_status='close' web_check_in_status='close' boarding_status='close'>
        <departure_datetime_scd>$(get tomor+1) 15:30</departure_datetime_scd>
        <destinations>
          <destination id='0'>PRG</destination>
        </destinations>
        <check_in_desks/>
        <gates/>
      </flight>})

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) VKO 24 $(date_format {%d.%m.%Y 23:00} +2) ЮТ lang=RU)
$(SEARCH_FLIGHTS_RESPONSE lang=RU
{      <flight carrier='ЮТ' flight_no='804' departure='ВНК' check_in_status='close' web_check_in_status='close' boarding_status='close'>
        <departure_datetime_scd>$(get tomor+2) 22:30</departure_datetime_scd>
        <destinations>
          <destination id='0'>ПЛК</destination>
        </destinations>
        <check_in_desks/>
        <gates/>
      </flight>})

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) VKO 23 $(date_format {%d.%m.%Y 23:00} +2) lang=RU)
$(SEARCH_FLIGHTS_RESPONSE)


### проверим поиск задержанного рейса, заодно закроем веб-регистрацию, закроем регистрацию, откроем посадку

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) VKO 1 $(date_format {%d.%m.%Y 23:00} +2) ЮТ lang=RU)
$(SEARCH_FLIGHTS_RESPONSE)

$(CHANGE_SPP_FLIGHT_REQUEST $(get point_dep)
{ $(change_spp_point $(get point_dep) UT 804 TU3 65021 ""                     ""                     "" VKO "$(get tomor+1) 22:30" "$(get tomor+1) 23:00")
  $(change_spp_point_last $(get point_arv)             "$(get tomor+2) 01:15" "$(get tomor+2) 01:45" "" LED )
})

$(CLOSE_WEB_CHECKIN $(get point_dep))
$(CLOSE_CHECKIN $(get point_dep))
$(OPEN_BOARDING $(get point_dep))

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) VKO 1 $(date_format {%d.%m.%Y 23:00} +2) ЮТ lang=RU)
$(SEARCH_FLIGHTS_RESPONSE lang=RU
{      <flight carrier='ЮТ' flight_no='804' departure='ВНК' check_in_status='close' web_check_in_status='close' boarding_status='open'>
        <departure_datetime_scd>$(get tomor+1) 22:30</departure_datetime_scd>
        <departure_datetime_est>$(get tomor+1) 23:00</departure_datetime_est>
        <destinations>
          <destination id='0'>ПЛК</destination>
        </destinations>
        <check_in_desks/>
        <gates/>
      </flight>})


### закроем посадку

$(CLOSE_BOARDING $(get point_dep))

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) VKO 1 $(date_format {%d.%m.%Y 23:00} +2) ЮТ lang=RU)
$(SEARCH_FLIGHTS_RESPONSE lang=RU
{      <flight carrier='ЮТ' flight_no='804' departure='ВНК' check_in_status='close' web_check_in_status='close' boarding_status='close'>
        <departure_datetime_scd>$(get tomor+1) 22:30</departure_datetime_scd>
        <departure_datetime_est>$(get tomor+1) 23:00</departure_datetime_est>
        <destinations>
          <destination id='0'>ПЛК</destination>
        </destinations>
        <check_in_desks/>
        <gates/>
      </flight>})


### заново откроем все этапы

$(CLOSE_WEB_CHECKIN_CANCEL $(get point_dep))
$(CLOSE_CHECKIN_CANCEL $(get point_dep))
$(CLOSE_BOARDING_CANCEL $(get point_dep))

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) VKO 1 $(date_format {%d.%m.%Y 23:00} +2) ЮТ lang=RU)
$(SEARCH_FLIGHTS_RESPONSE lang=RU
{      <flight carrier='ЮТ' flight_no='804' departure='ВНК' check_in_status='open' web_check_in_status='open' boarding_status='open'>
        <departure_datetime_scd>$(get tomor+1) 22:30</departure_datetime_scd>
        <departure_datetime_est>$(get tomor+1) 23:00</departure_datetime_est>
        <destinations>
          <destination id='0'>ПЛК</destination>
        </destinations>
        <check_in_desks/>
        <gates/>
      </flight>})

### проверим поиск вылетевшего рейса
### из-за вылета все этапы возвращаем закрытыми

$(CHANGE_SPP_FLIGHT_REQUEST $(get point_dep)
{ $(change_spp_point $(get point_dep) UT 804 TU3 65021 ""                     ""                     "" VKO "$(get tomor+1) 22:30" "$(get tomor+1) 23:00" "$(get tomor+1) 22:55")
  $(change_spp_point_last $(get point_arv)             "$(get tomor+2) 01:15" "$(get tomor+2) 01:45" "" LED )
})

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) VKO 1 $(date_format {%d.%m.%Y 23:00} +2) ЮТ lang=RU)
$(SEARCH_FLIGHTS_RESPONSE)

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) VKO 1 $(date_format {%d.%m.%Y 22:00} +2) ЮТ lang=RU)
$(SEARCH_FLIGHTS_RESPONSE lang=RU
{      <flight carrier='ЮТ' flight_no='804' departure='ВНК' check_in_status='close' web_check_in_status='close' boarding_status='close'>
        <departure_datetime_scd>$(get tomor+1) 22:30</departure_datetime_scd>
        <departure_datetime_est>$(get tomor+1) 23:00</departure_datetime_est>
        <departure_datetime_act>$(get tomor+1) 22:55</departure_datetime_act>
        <destinations>
          <destination id='0'>ПЛК</destination>
        </destinations>
        <check_in_desks/>
        <gates/>
      </flight>})

%%

### test 3 - запрос search_passengers с разными параметрами
#########################################################################################

$(init_term)
$(set_user_time_type LocalAirp PIKE)

$(PREPARE_MOBILE_PAYMENT)

$(UPDATE_USER AERMPY AERMPY 0 lang=EN
{          <airps>
            <item>ВНК</item>
            <item>СОЧ</item>
          </airps>})

$(set tomor+0 $(date_format %d.%m.%Y +1))
$(set tomor+1 $(date_format %d.%m.%Y +2))
$(set tomor+2 $(date_format %d.%m.%Y +3))

$(PREPARE_FLIGHTS $(get tomor+0) $(get tomor+1) $(get tomor+2))

$(PNL_UT_580)
$(PNL_UT_461)
#$(PNL_UT_804_1 date_dep=$(ddmon +1 en))
#$(PNL_UT_804_2 date_dep=$(ddmon +2 en))
#$(INB_PNL_UT VKO PRG 298 $(ddmon +1 en))

$(deny_ets_interactive)


$(SEARCH_PASSENGERS_REQUEST $(get http_heading))
$(SEARCH_PASSENGERS_ERROR "Node 'departure' does not exists")

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) VKO)
$(SEARCH_PASSENGERS_ERROR "Not enough parameters to search")


### регистрируем одного пассажира на двух сегментах

$(set point_dep_580 $(get_point_dep_for_flight ЮТ 580 "" $(yymmdd +1) СОЧ))
$(set point_arv_580 $(get_next_trip_point_id $(get point_dep_580)))
$(set point_dep_461 $(get_point_dep_for_flight ЮТ 461 "" $(yymmdd +1) ВНК))
$(set point_arv_461 $(get_next_trip_point_id $(get point_dep_461)))

$(set pax_id_1479_1 $(get_pax_id $(get point_dep_580) KOTOVA IRINA))
$(set pax_id_1480_1 $(get_pax_id $(get point_dep_580) MOTOVA IRINA))
$(set pax_id_1479_2 $(get_pax_id $(get point_dep_461) KOTOVA IRINA))
$(set pax_id_1480_2 $(get_pax_id $(get point_dep_461) MOTOVA IRINA))

$(OPEN_WEB_CHECKIN $(get point_dep_580))
$(OPEN_CHECKIN $(get point_dep_580))

$(NEW_TCHECKIN_REQUEST capture=off lang=EN hall=1
$(TRANSFER_SEGMENT ЮТ 461 "" $(dd +1) ВНК РЩН)
{$(NEW_CHECKIN_SEGMENT $(get point_dep_580) $(get point_arv_580) СОЧ ВНК
{<passengers>
  <pax>
$(NEW_CHECKIN_2982410821479 $(get pax_id_1479_1) 1 Y)
  </pax>
</passengers>})
$(NEW_CHECKIN_SEGMENT $(get point_dep_461) $(get point_arv_461) ВНК РЩН
{<passengers>
  <pax>
$(NEW_CHECKIN_2982410821479 $(get pax_id_1479_2) 2)
  </pax>
</passengers>})})

### поиск по номеру рейса

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) VKO carrier=UT)
$(SEARCH_PASSENGERS_ERROR "Not enough parameters to search")

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) VKO
  carrier=UT
  departure_date_scd=$(date_format %d.%m.%Y +1)
  destination=LED)
$(SEARCH_PASSENGERS_ERROR "Not enough parameters to search")

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) AER
  carrier=UT
  flight_no=580
  departure_date_scd=$(date_format %d.%m.%Y +1))
$(SEARCH_PASSENGERS_RESPONSE
{$(PAX_RESPONSE_1479_1)
$(PAX_RESPONSE_1480_1)})

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) VKO
  carrier=UT
  flight_no=461
  departure_date_scd=$(date_format %d.%m.%Y +1)
  destination=LED)
$(SEARCH_PASSENGERS_RESPONSE)

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) VKO
  carrier=UT
  flight_no=461
  departure_date_scd=$(date_format %d.%m.%Y +1)
  destination=TJM)
$(SEARCH_PASSENGERS_RESPONSE
{$(PAX_RESPONSE_1479_2)
$(PAX_RESPONSE_1480_2)})

### поиск по номеру PNR

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) VKO recloc=04VSFC)
$(SEARCH_PASSENGERS_RESPONSE
{$(PAX_RESPONSE_1479_2)
$(PAX_RESPONSE_1480_2)})

### поиск по номеру билета

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) AER ticket_no=2982410821480)
$(SEARCH_PASSENGERS_RESPONSE $(PAX_RESPONSE_1480_1))

### поиск по номеру документа

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) AER document_no=7774441110)
$(SEARCH_PASSENGERS_ERROR "Not enough parameters to search")

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) AER search_depth=0 document_no=7774441110)
$(SEARCH_PASSENGERS_ERROR "Wrong &lt;search_depth&gt;")

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) AER search_depth=49 document_no=7774441110)
$(SEARCH_PASSENGERS_ERROR "Wrong &lt;search_depth&gt;")

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) AER search_depth=3 document_no=7774441110)
$(SEARCH_PASSENGERS_RESPONSE)

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) AER search_depth=48 document_no=13578642)
$(SEARCH_PASSENGERS_RESPONSE)

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) AER search_depth=48 document_no=7774441110)
$(SEARCH_PASSENGERS_RESPONSE
{$(PAX_RESPONSE_1479_1)
$(PAX_RESPONSE_1480_1)})

### поиск по фамилии

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) AER search_depth=3 lastname=МОТОВА)
$(SEARCH_PASSENGERS_RESPONSE)

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) AER search_depth=48 lastname=МОТОВА)
$(SEARCH_PASSENGERS_RESPONSE $(PAX_RESPONSE_1480_1))

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) VKO search_depth=48 lastname=KOTOVA)
$(SEARCH_PASSENGERS_RESPONSE $(PAX_RESPONSE_1479_2))


