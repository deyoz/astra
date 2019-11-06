include(ts/macro.ts)

# meta: suite eticket


### test 1 - γα―¥θ­ ο α¬¥­  αβ βγα 
#########################################################################################

$(init)
$(init_jxt_pult ‚)
$(login)
$(init_eds ’ UTET UTDC)

$(PREPARE_FLIGHT_1PAX_1SEG ’ 103 „„ ‹ REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_pax_id $(get point_dep) REPIN IVAN))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep))


!! err=ignore
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv)
              ’ 454 „„ ‹
              REPIN IVAN 2986120030297 ‚‡
              RUS 12312311 UKR 20.01.1976 10.10.2025 M)

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+1H:‚+++’+Y+::RU+‚"
EQN+1:TD"
TKT+2986120030297:T"
CPN+1:CK"
TVL+$(ddmmyy)+„„+‹+’+103++1"
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
### test 2 - ®θ¨΅ª  Ά ®βΆ¥β¥ ­  COS-req
#########################################################################################

$(init)
$(init_jxt_pult ‚)
$(login)
$(init_eds ’ UTET UTDC)

$(PREPARE_FLIGHT_1PAX_1SEG ’ 103 „„ ‹ REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_pax_id $(get point_dep) REPIN IVAN))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep))

!! err=ignore
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv)
              ’ 454 „„ ‹
              REPIN IVAN 2986120030297 ‚‡
              RUS 12123123 RUS 10.01.1970 20.10.2025 M)

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+1H:‚+++’+Y+::RU+‚"
EQN+1:TD"
TKT+2986120030297:T"
CPN+1:CK"
TVL+$(ddmmyy)+„„+‹+’+103++1"
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
    <ets_error>‘: € 401



%%
### test 3 - β ©¬ γβ ­  COS-req
#########################################################################################

$(init)
$(init_jxt_pult ‚)
$(login)
$(init_eds ’ UTET UTDC)

$(PREPARE_FLIGHT_1PAX_1SEG ’ 103 „„ ‹ REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_pax_id $(get point_dep) REPIN IVAN))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep))

!! err=ignore
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv)
              ’ 454 „„ ‹
              REPIN IVAN 2986120030297 ‚‡
              RUS 123123413 RUS 10.01.1980 10.10.2025 M)

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+1H:‚+++’+Y+::RU+‚"
EQN+1:TD"
TKT+2986120030297:T"
CPN+1:CK"
TVL+$(ddmmyy)+„„+‹+’+103++1"
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
$(init_jxt_pult ‚)
$(login)
$(init_eds KAR IKET IKDC translit)

$(PREPARE_FLIGHT_1PAX_1SEG KAR 103 „„ ‹ REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_pax_id $(get point_dep) REPIN IVAN))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 7706120030297 REPIN IVAN IK IKDC IKET)

!! err=ignore
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv)
              KAR 103 „„ ‹
              REPIN IVAN 7706120030297 ‚‡
              RUS 12123123 RUS 10.01.1990 20.01.2025 M)

>>
UNB+SIRE:1+IKDC+IKET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+1H:MOW+++IK+Y+::RU+MOVROM"
EQN+1:TD"
TKT+7706120030297:T"
CPN+1:CK"
TVL+$(ddmmyy)+DME+LED+IK+103++1"
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
