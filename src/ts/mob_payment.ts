include(ts/macro.ts)
include(ts/adm_macro.ts)
include(ts/spp/write_dests_macro.ts)
include(ts/spp/write_trips_macro.ts)
include(ts/pnl/pnl_ut_580_461.ts)
include(ts/pax/checkin_macro.ts)

# meta: suite mob_payment

$(defmacro PREPARE_MOBILE_PAYMENT
{

### ����� ��� �室��� http-����ᮢ �� �����쭮� ॣ����樨

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

$(defmacro PAX_RESPONSE_1479_1_RU
{      <passenger lastname='KOTOVA' name='IRINA' date_of_birth='01.05.1976' category='adult' gender='female' document_no='7774441110' pax_id='$(get pax_id_1479_1)' status='checked'>
        <flight carrier='��' flight_no='580' departure='���' destination='���' departure_time='$(get tomor+0) 12:00' arrival_time='$(get tomor+0) 15:00' check_in_status='open' web_check_in_status='open' boarding_status='close'>
          <ticket>2982410821479</ticket>
          <reclocs>
            <recloc crs='DT'>04VSFC</recloc>
            <recloc crs='��'>054C82</recloc>
          </reclocs>
        </flight>
      </passenger>})

$(defmacro PAX_RESPONSE_1479_1_EN
{      <passenger lastname='KOTOVA' name='IRINA' date_of_birth='01.05.1976' category='adult' gender='female' document_no='7774441110' pax_id='$(get pax_id_1479_1)' status='checked'>
        <flight carrier='UT' flight_no='580' departure='AER' destination='VKO' departure_time='$(get tomor+0) 12:00' arrival_time='$(get tomor+0) 15:00' check_in_status='open' web_check_in_status='open' boarding_status='close'>
          <ticket>2982410821479</ticket>
          <reclocs>
            <recloc crs='DT'>04VSFC</recloc>
            <recloc crs='UT'>054C82</recloc>
          </reclocs>
        </flight>
      </passenger>})

$(defmacro PAX_RESPONSE_1480_1_RU
{      <passenger lastname='MOTOVA' name='IRINA' date_of_birth='01.05.1976' category='adult' gender='female' document_no='7774441110' pax_id='$(get pax_id_1480_1)' status='not_checked'>
        <flight carrier='��' flight_no='580' departure='���' destination='���' departure_time='$(get tomor+0) 12:00' arrival_time='$(get tomor+0) 15:00' check_in_status='open' web_check_in_status='open' boarding_status='close'>
          <ticket>2982410821480</ticket>
          <reclocs>
            <recloc crs='DT'>04VSFC</recloc>
            <recloc crs='��'>054C82</recloc>
          </reclocs>
        </flight>
      </passenger>})

$(defmacro PAX_RESPONSE_1480_1_EN
{      <passenger lastname='MOTOVA' name='IRINA' date_of_birth='01.05.1976' category='adult' gender='female' document_no='7774441110' pax_id='$(get pax_id_1480_1)' status='not_checked'>
        <flight carrier='UT' flight_no='580' departure='AER' destination='VKO' departure_time='$(get tomor+0) 12:00' arrival_time='$(get tomor+0) 15:00' check_in_status='open' web_check_in_status='open' boarding_status='close'>
          <ticket>2982410821480</ticket>
          <reclocs>
            <recloc crs='DT'>04VSFC</recloc>
            <recloc crs='UT'>054C82</recloc>
          </reclocs>
        </flight>
      </passenger>})

$(defmacro PAX_RESPONSE_1479_2_RU
{      <passenger lastname='KOTOVA' name='IRINA' date_of_birth='01.05.1976' category='adult' gender='female' document_no='7774441110' pax_id='$(get pax_id_1479_2)' status='checked'>
        <flight carrier='��' flight_no='461' departure='���' destination='���' departure_time='$(get tomor+0) 16:00' arrival_time='$(get tomor+0) 21:20' check_in_status='close' web_check_in_status='close' boarding_status='close'>
          <ticket>2982410821479</ticket>
          <reclocs>
            <recloc crs='DT'>04VSFC</recloc>
            <recloc crs='��'>054C82</recloc>
          </reclocs>
        </flight>
      </passenger>})

$(defmacro PAX_RESPONSE_1480_2_RU
{      <passenger lastname='MOTOVA' name='IRINA' date_of_birth='01.05.1976' category='adult' gender='female' document_no='7774441110' pax_id='$(get pax_id_1480_2)' status='not_checked'>
        <flight carrier='��' flight_no='461' departure='���' destination='���' departure_time='$(get tomor+0) 16:00' arrival_time='$(get tomor+0) 21:20' check_in_status='close' web_check_in_status='close' boarding_status='close'>
          <ticket>2982410821480</ticket>
          <reclocs>
            <recloc crs='DT'>04VSFC</recloc>
            <recloc crs='��'>054C82</recloc>
          </reclocs>
        </flight>
      </passenger>})


### test 1 - ����� get_client_perms � ࠧ�묨 ����㯠��
#########################################################################################

$(init_term)
$(set_user_time_type LocalAirp PIKE)

### ����� �� �ᥬ �� � ��

$(PREPARE_MOBILE_PAYMENT)

$(GET_CLIENT_PERMS_REQUEST $(get http_heading) lang=EN)

$(GET_CLIENT_PERMS_RESPONSE lang=EN
{      <points/>})


### ����� � ��।������ �� � �ᥬ ��

$(UPDATE_USER AERMPY AERMPY 0 lang=EN
{          <airps>
            <item>���</item>
            <item>���</item>
          </airps>})

$(GET_CLIENT_PERMS_REQUEST $(get http_heading) lang=EN)

$(GET_CLIENT_PERMS_RESPONSE lang=EN
{      <points>
        <point>VKO</point>
        <point>AER</point>
      </points>})


### ����� �� �ᥬ �� � ��।������ ��

$(UPDATE_USER AERMPY AERMPY 0 lang=EN
{          <airlines>
            <item>��</item>
            <item>��</item>
          </airlines>})

$(GET_CLIENT_PERMS_REQUEST $(get http_heading) lang=EN)

$(GET_CLIENT_PERMS_RESPONSE lang=EN
{      <points/>
      <carriers>
        <carrier>FV</carrier>
        <carrier>UT</carrier>
      </carriers>})


### ����� � ��।������ �� � ��

$(UPDATE_USER AERMPY AERMPY 0 lang=EN
{          <airps>
            <item>���</item>
            <item>���</item>
          </airps>}
{          <airlines>
            <item>��</item>
            <item>��</item>
          </airlines>})

$(GET_CLIENT_PERMS_REQUEST $(get http_heading) lang=RU)

$(GET_CLIENT_PERMS_RESPONSE lang=RU
{      <points>
        <point>���</point>
        <point>���</point>
      </points>
      <carriers>
        <carrier>��</carrier>
        <carrier>��</carrier>
      </carriers>})


### ����� � ��।������ �� ��� ����㯠 � �� �� ��

$(UPDATE_USER AERMPY AERMPY 2
{          <airps>
            <item>���</item>
            <item>���</item>
          </airps>})

$(GET_CLIENT_PERMS_REQUEST $(get http_heading))

$(GET_CLIENT_PERMS_RESPONSE
{      <points/>})


### ����� � ��।������ �� ��� ����㯠 � ��� �� ��

$(UPDATE_USER AERMPY AERMPY 1
{          <airlines>
            <item>��</item>
            <item>��</item>
          </airlines>})

$(GET_CLIENT_PERMS_REQUEST $(get http_heading))

$(GET_CLIENT_PERMS_RESPONSE
{      <points/>})

%%

### test 2 - ����� search_flights � ࠧ�묨 ��ࠬ��ࠬ�
#########################################################################################

$(init_term)
$(set_user_time_type LocalAirp PIKE)

$(PREPARE_MOBILE_PAYMENT)

$(UPDATE_USER AERMPY AERMPY 0 lang=EN
{          <airps>
            <item>���</item>
            <item>���</item>
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

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) ��� 1 "" "")
$(SEARCH_FLIGHTS_RESPONSE)

$(set wrong_departure_datetime $(date_format {%d.%m.%Y 23:00:00} +2))
$(SEARCH_FLIGHTS_REQUEST $(get http_heading) VKO 23 $(get wrong_departure_datetime) lang=RU)
$(SEARCH_FLIGHTS_ERROR "Wrong &lt;departure_datetime&gt; '$(get wrong_departure_datetime)'")

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) VKO 23 departure_datetime=����� lang=RU)
$(SEARCH_FLIGHTS_ERROR "Wrong &lt;departure_datetime&gt; '�����'")

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) AER 48 carrier=ZZZZ)
$(SEARCH_FLIGHTS_ERROR "Unknown &lt;carrier&gt; 'ZZZZ'")

$(set tomor+1 $(date_format %d.%m.%Y +2))
$(set tomor+2 $(date_format %d.%m.%Y +3))
$(set tomor+3 $(date_format %d.%m.%Y +4))

$(PREPARE_FLIGHTS $(get tomor+1) $(get tomor+2) $(get tomor+3))

$(set point_dep $(get_point_dep_for_flight UT 804 "" $(yymmdd +2) ���))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(OPEN_WEB_CHECKIN $(get point_dep))
$(OPEN_CHECKIN $(get point_dep))

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) VKO 4 lang=RU)
$(SEARCH_FLIGHTS_RESPONSE
{      <flight carrier='�7' flight_no='371' departure='���' check_in_status='close' web_check_in_status='close' boarding_status='close'>
        <departure_datetime_scd>$(get now+2h)</departure_datetime_scd>
        <destinations>
          <destination id='0'>���</destination>
        </destinations>
        <check_in_desks/>
        <gates/>
      </flight>
      <flight carrier='�6' flight_no='159�' departure='���' check_in_status='close' web_check_in_status='close' boarding_status='close'>
        <departure_datetime_scd>$(get now+3h)</departure_datetime_scd>
        <destinations>
          <destination id='0'>���</destination>
          <destination id='1'>���</destination>
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

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) VKO 24 $(date_format {%d.%m.%Y 23:00} +1) �� lang=EN)
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

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) VKO 24 $(date_format {%d.%m.%Y 23:00} +2) �� lang=RU)
$(SEARCH_FLIGHTS_RESPONSE lang=RU
{      <flight carrier='��' flight_no='804' departure='���' check_in_status='close' web_check_in_status='close' boarding_status='close'>
        <departure_datetime_scd>$(get tomor+2) 22:30</departure_datetime_scd>
        <destinations>
          <destination id='0'>���</destination>
        </destinations>
        <check_in_desks/>
        <gates/>
      </flight>})

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) VKO 23 $(date_format {%d.%m.%Y 23:00} +2) lang=RU)
$(SEARCH_FLIGHTS_RESPONSE)


### �஢�ਬ ���� ����ঠ����� ३�, ������ ���஥� ���-ॣ������, ���஥� ॣ������, ��஥� ��ᠤ��

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) VKO 1 $(date_format {%d.%m.%Y 23:00} +2) �� lang=RU)
$(SEARCH_FLIGHTS_RESPONSE)

$(CHANGE_SPP_FLIGHT_REQUEST $(get point_dep)
{ $(change_spp_point $(get point_dep) UT 804 TU3 65021 ""                     ""                     "" VKO "$(get tomor+1) 22:30" "$(get tomor+1) 23:00")
  $(change_spp_point_last $(get point_arv)             "$(get tomor+2) 01:15" "$(get tomor+2) 01:45" "" LED )
})

$(CLOSE_WEB_CHECKIN $(get point_dep))
$(CLOSE_CHECKIN $(get point_dep))
$(OPEN_BOARDING $(get point_dep))

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) VKO 1 $(date_format {%d.%m.%Y 23:00} +2) �� lang=RU)
$(SEARCH_FLIGHTS_RESPONSE lang=RU
{      <flight carrier='��' flight_no='804' departure='���' check_in_status='close' web_check_in_status='close' boarding_status='open'>
        <departure_datetime_scd>$(get tomor+1) 22:30</departure_datetime_scd>
        <departure_datetime_est>$(get tomor+1) 23:00</departure_datetime_est>
        <destinations>
          <destination id='0'>���</destination>
        </destinations>
        <check_in_desks/>
        <gates/>
      </flight>})


### ���஥� ��ᠤ��

$(CLOSE_BOARDING $(get point_dep))

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) VKO 1 $(date_format {%d.%m.%Y 23:00} +2) �� lang=RU)
$(SEARCH_FLIGHTS_RESPONSE lang=RU
{      <flight carrier='��' flight_no='804' departure='���' check_in_status='close' web_check_in_status='close' boarding_status='close'>
        <departure_datetime_scd>$(get tomor+1) 22:30</departure_datetime_scd>
        <departure_datetime_est>$(get tomor+1) 23:00</departure_datetime_est>
        <destinations>
          <destination id='0'>���</destination>
        </destinations>
        <check_in_desks/>
        <gates/>
      </flight>})


### ������ ��஥� �� �⠯�

$(CLOSE_WEB_CHECKIN_CANCEL $(get point_dep))
$(CLOSE_CHECKIN_CANCEL $(get point_dep))
$(CLOSE_BOARDING_CANCEL $(get point_dep))

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) VKO 1 $(date_format {%d.%m.%Y 23:00} +2) �� lang=RU)
$(SEARCH_FLIGHTS_RESPONSE lang=RU
{      <flight carrier='��' flight_no='804' departure='���' check_in_status='open' web_check_in_status='open' boarding_status='open'>
        <departure_datetime_scd>$(get tomor+1) 22:30</departure_datetime_scd>
        <departure_datetime_est>$(get tomor+1) 23:00</departure_datetime_est>
        <destinations>
          <destination id='0'>���</destination>
        </destinations>
        <check_in_desks/>
        <gates/>
      </flight>})

### �஢�ਬ ���� �뫥⥢襣� ३�
### ��-�� �뫥� �� �⠯� �����頥� ������묨

$(CHANGE_SPP_FLIGHT_REQUEST $(get point_dep)
{ $(change_spp_point $(get point_dep) UT 804 TU3 65021 ""                     ""                     "" VKO "$(get tomor+1) 22:30" "$(get tomor+1) 23:00" "$(get tomor+1) 22:55")
  $(change_spp_point_last $(get point_arv)             "$(get tomor+2) 01:15" "$(get tomor+2) 01:45" "" LED )
})

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) VKO 1 $(date_format {%d.%m.%Y 23:00} +2) �� lang=RU)
$(SEARCH_FLIGHTS_RESPONSE)

$(SEARCH_FLIGHTS_REQUEST $(get http_heading) VKO 1 $(date_format {%d.%m.%Y 22:00} +2) �� lang=RU)
$(SEARCH_FLIGHTS_RESPONSE lang=RU
{      <flight carrier='��' flight_no='804' departure='���' check_in_status='close' web_check_in_status='close' boarding_status='close'>
        <departure_datetime_scd>$(get tomor+1) 22:30</departure_datetime_scd>
        <departure_datetime_est>$(get tomor+1) 23:00</departure_datetime_est>
        <departure_datetime_act>$(get tomor+1) 22:55</departure_datetime_act>
        <destinations>
          <destination id='0'>���</destination>
        </destinations>
        <check_in_desks/>
        <gates/>
      </flight>})

%%

### test 3 - ����� search_passengers � ࠧ�묨 ��ࠬ��ࠬ�
#########################################################################################

$(init_term)
$(set_user_time_type LocalAirp PIKE)

$(PREPARE_MOBILE_PAYMENT)

$(UPDATE_USER AERMPY AERMPY 0 lang=EN
{          <airps>
            <item>���</item>
            <item>���</item>
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


### ॣ�����㥬 ������ ���ᠦ�� �� ���� ᥣ�����

$(set point_dep_580 $(get_point_dep_for_flight �� 580 "" $(yymmdd +1) ���))
$(set point_arv_580 $(get_next_trip_point_id $(get point_dep_580)))
$(set point_dep_461 $(get_point_dep_for_flight �� 461 "" $(yymmdd +1) ���))
$(set point_arv_461 $(get_next_trip_point_id $(get point_dep_461)))

$(set pax_id_1479_1 $(get_pax_id $(get point_dep_580) KOTOVA IRINA))
$(set pax_id_1480_1 $(get_pax_id $(get point_dep_580) MOTOVA IRINA))
$(set pax_id_1479_2 $(get_pax_id $(get point_dep_461) KOTOVA IRINA))
$(set pax_id_1480_2 $(get_pax_id $(get point_dep_461) MOTOVA IRINA))

$(OPEN_WEB_CHECKIN $(get point_dep_580))
$(OPEN_CHECKIN $(get point_dep_580))

$(NEW_TCHECKIN_REQUEST capture=off lang=EN hall=1
$(TRANSFER_SEGMENT �� 461 "" $(dd +1) ��� ���)
{$(NEW_CHECKIN_SEGMENT $(get point_dep_580) $(get point_arv_580) ��� ���
{<passengers>
  <pax>
$(NEW_CHECKIN_2982410821479 $(get pax_id_1479_1) 1 Y)
  </pax>
</passengers>})
$(NEW_CHECKIN_SEGMENT $(get point_dep_461) $(get point_arv_461) ��� ���
{<passengers>
  <pax>
$(NEW_CHECKIN_2982410821479 $(get pax_id_1479_2) 2)
  </pax>
</passengers>})})

$(set grp_id_1479_1 $(get_single_grp_id $(get pax_id_1479_1)))
$(set grp_id_1479_2 $(get_single_grp_id $(get pax_id_1479_2)))

### ���� �� ������ ३�

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
{$(PAX_RESPONSE_1479_1_RU)
$(PAX_RESPONSE_1480_1_RU)})

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
{$(PAX_RESPONSE_1479_2_RU)
$(PAX_RESPONSE_1480_2_RU)})

### ���� �� ������ PNR

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) VKO recloc=04VSFC)
$(SEARCH_PASSENGERS_RESPONSE
{$(PAX_RESPONSE_1479_2_RU)
$(PAX_RESPONSE_1480_2_RU)})

### ���� �� ������ �����

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) AER ticket_no=2982410821480)
$(SEARCH_PASSENGERS_RESPONSE $(PAX_RESPONSE_1480_1_RU))

### ���� �� ������ ���㬥��

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

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) AER search_depth=48 document_no=7774441110 lang=EN)
$(SEARCH_PASSENGERS_RESPONSE lang=EN
{$(PAX_RESPONSE_1479_1_EN)
$(PAX_RESPONSE_1480_1_EN)})

### ���� �� 䠬����

$(dump_table CRS_PAX_TRANSLIT)
$(dump_table PAX_TRANSLIT)
$(dump_table TEST_PAX_TRANSLIT)

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) AER search_depth=3 lastname=������)
$(SEARCH_PASSENGERS_RESPONSE)

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) AER search_depth=48 lastname=������)
$(SEARCH_PASSENGERS_RESPONSE $(PAX_RESPONSE_1480_1_RU))

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) VKO search_depth=48 lastname=KOTOVA)
$(SEARCH_PASSENGERS_RESPONSE $(PAX_RESPONSE_1479_2_RU))


### ���� �� ����-����

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) AER barcode=�����)
$(SEARCH_PASSENGERS_ERROR "Wrong &lt;barcode&gt;: unknown item 1 &lt;Format Code&gt;")

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) AER barcode=$(get_bcbp $(get grp_id_1479_1) $(get pax_id_1479_1) RU))
$(SEARCH_PASSENGERS_RESPONSE $(PAX_RESPONSE_1479_1_RU))

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) VKO barcode=$(get_bcbp $(get grp_id_1479_2) $(get pax_id_1479_2) EN))
$(SEARCH_PASSENGERS_RESPONSE $(PAX_RESPONSE_1479_2_RU))

### ���� �� �ᥬ �����

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) ���
  barcode=$(get_bcbp $(get grp_id_1479_1) $(get pax_id_1479_1) EN)
  carrier=��
  flight_no=580
  departure_date_scd=$(date_format %d.%m.%Y +1)
  destination=���
  recloc=04����
  ticket_no=2982410821479
  search_depth=48
  document_no=7774441110
  lastname=������)
$(SEARCH_PASSENGERS_RESPONSE $(PAX_RESPONSE_1479_1_RU))

### ���� � �ᥬ� ����묨 ����ﬨ

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) ���
  barcode=""
  carrier=""
  flight_no=""
  departure_date_scd=""
  destination=""
  recloc=""
  ticket_no=""
  search_depth=""
  document_no=""
  lastname="")
$(SEARCH_PASSENGERS_ERROR "Not enough parameters to search")

### ���� �� ������ � �ᥬ� ��⠫�묨 ����묨 ����ﬨ

$(SEARCH_PASSENGERS_REQUEST $(get http_heading) ���
  barcode=""
  carrier=""
  flight_no=""
  departure_date_scd=""
  destination=""
  recloc=""
  ticket_no=2982410821479
  search_depth=""
  document_no=""
  lastname="")
$(SEARCH_PASSENGERS_RESPONSE $(PAX_RESPONSE_1479_1_RU))
