include(ts/macro.ts)

# meta: suite eticket


$(defmacro CHECKIN_PAX
    pax_id
    point_dep
    point_arv
    airp_dep=ДМД
    airp_arv=ПЛК
    airl=ЮТ
    flt=454
    surname=REPIN
    name=IVAN
    tickno=2986120030297
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>3</agent_stat_period>
      <transfer/>
      <segments>
        <segment>
          <point_dep>$(point_dep)</point_dep>
          <point_arv>$(point_arv)</point_arv>
          <airp_dep>$(airp_dep)</airp_dep>
          <airp_arv>$(airp_arv)</airp_arv>
          <class>Э</class>
          <status>K</status>
          <wl_type/>
          <mark_flight>
            <airline>$(airl)</airline>
            <flt_no>$(flt)</flt_no>
            <suffix/>
            <scd>$(date_format %d.%m.%Y) 00:00:00</scd>
            <airp_dep>$(airp_dep)</airp_dep>
            <pr_mark_norms>0</pr_mark_norms>
          </mark_flight>
          <passengers>
            <pax>
              <pax_id>$(pax_id)</pax_id>
              <surname>$(surname)</surname>
              <name>$(name)</name>
              <pers_type>ВЗ</pers_type>
              <seat_no/>
              <preseat_no/>
              <seat_type/>
              <seats>1</seats>
              <ticket_no>$(tickno)</ticket_no>
              <coupon_no>1</coupon_no>
              <ticket_rem>TKNE</ticket_rem>
              <ticket_confirm>0</ticket_confirm>
              <document>
                <type>P</type>
                <issue_country>RUS</issue_country>
                <no>7774441110</no>
                <nationality>RUS</nationality>
                <birth_date>01.05.1976 00:00:00</birth_date>
                <gender>M</gender>
                <surname>$(surname)</surname>
                <first_name>$(name)</first_name>
              </document>
              <doco/>
              <addresses/>
              <subclass>Э</subclass>
              <bag_pool_num/>
              <transfer/>
              <rems>
                <rem>
                  <rem_code>FOID</rem_code>
                  <rem_text>FOID PP7774441110</rem_text>
                </rem>
              </rems>
              <norms/>
            </pax>
          </passengers>
          <paid_bag_emd/>
        </segment>
      </segments>
      <excess>0</excess>
      <hall>1</hall>
      <paid_bags>
        <paid_bag>
          <bag_type/>
          <weight>0</weight>
          <rate_id/>
          <rate_trfer/>
        </paid_bag>
      </paid_bags>
    </TCkinSavePax>
  </query>
</term>}
) #end-of-macro


### test 1 - успешная смена статуса
#########################################################################################

$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_eds ЮТ UTET UTDC)

$(PREPARE_FLIGHT_1 ЮТ 103 ДМД ПЛК REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep))

!! err=ignore
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv))

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+1H:МОВ+++ЮТ+Y+::RU+МОВРОМ"
EQN+1:TD"
TKT+2986120030297:T"
CPN+1:CK"
TVL+$(ddmmyy)+ДМД+ПЛК+ЮТ+103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+UTET+UTDC+160408:0828+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref)"
MSG+:142+3"
EQN+2:TD"
TKT+2986120030297:T::3"
CPN+1:CK::E"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"


$(KICK_IN)


>> lines=auto
            <ticket_no>2986120030297...

%%
### test 2 - ошибка в ответе на COS-req
#########################################################################################

$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_eds ЮТ UTET UTDC)

$(PREPARE_FLIGHT_1 ЮТ 103 ДМД ПЛК REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep))

!! err=ignore
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv))

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+1H:МОВ+++ЮТ+Y+::RU+МОВРОМ"
EQN+1:TD"
TKT+2986120030297:T"
CPN+1:CK"
TVL+$(ddmmyy)+ДМД+ПЛК+ЮТ+103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"


<<
UNB+SIRE:1+UTET+UTDC+160408:0828+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref)"
MSG+:142+7"
ERC+401"
UNT+4+1"
UNZ+1+$(last_edifact_ref)0001"


$(KICK_IN)

>> lines=auto
    <ets_error>СЭБ: ОШИБКА 401



%%
### test 3 - таймаут на COS-req
#########################################################################################

$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_eds ЮТ UTET UTDC)

$(PREPARE_FLIGHT_1 ЮТ 103 ДМД ПЛК REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep))

!! err=ignore
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv))

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+1H:МОВ+++ЮТ+Y+::RU+МОВРОМ"
EQN+1:TD"
TKT+2986120030297:T"
CPN+1:CK"
TVL+$(ddmmyy)+ДМД+ПЛК+ЮТ+103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"


$(sql {update EDISESSION_TIMEOUTS set time_out = sysdate - 1})
$(run_daemon edi_timeout)

>> lines=auto
    <kick req_ctxt_id...


%%
### test 4 - KAR!!!
#########################################################################################

$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_eds KAR IKET IKDC translit)

$(PREPARE_FLIGHT_1 KAR 103 ДМД ПЛК REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 7706120030297 REPIN IVAN IK IKDC IKET)

!! err=ignore
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv)
              ДМД ПЛК KAR 103 REPIN IVAN 7706120030297)

>>
UNB+SIRE:1+IKDC+IKET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+1H:MOW+++IK+Y+::RU+MOVROM"
EQN+1:TD"
TKT+7706120030297:T"
CPN+1:CK"
TVL+$(ddmmyy)+DME+LED+IK+103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+IKET+IKDC+160408:0828+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref)"
MSG+:142+3"
EQN+2:TD"
TKT+7706120030297:T::3"
CPN+1:CK::E"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"


$(KICK_IN)


>> lines=auto
            <ticket_no>7706120030297...
