include(ts/macro.ts)
include(ts/adm_macro.ts)
include(ts/spp/write_dests_macro.ts)
include(ts/pax/boarding_macro.ts)

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
            <ticket_no>2986120030297</ticket_no>
            <coupon_no>1</coupon_no>
            <ticket_rem>TKNE</ticket_rem>
            <ticket_confirm>1</ticket_confirm>

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
            <ticket_no>7706120030297</ticket_no>
            <coupon_no>1</coupon_no>
            <ticket_rem>TKNE</ticket_rem>
            <ticket_confirm>1</ticket_confirm>

%%

### test 5 - δ¨­ «μ­λ© αβ βγα
#########################################################################################

$(init_term)

$(set_user_time_type LocalAirp PIKE)

$(set airline UT)
$(set flt_no 280)
$(set craft TU5)
$(set airp_dep DME)
$(set time_dep "$(date_format %d.%m.%Y -1) 07:00")
$(set time_arv "$(date_format %d.%m.%Y -1) 10:00")
$(set airp_arv AER)

$(NEW_SPP_FLIGHT_ONE_LEG $(get airline) $(get flt_no) $(get craft) $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv))

$(INB_PNL_UT $(get airp_dep) $(get airp_arv) $(get flt_no) $(ddmon -1))

$(init_eds ’ UTET UTDC)
$(PREPARE_HALLS_FOR_BOARDING $(get airp_dep))

$(set point_dep $(last_point_id_spp))

$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id_01 $(get_pax_id $(get point_dep) STIPIDI ANGELINA))
$(set pax_id_02 $(get_pax_id $(get point_dep) AKOPOVA OLIVIIA))
$(set pax_id_03 $(get_pax_id $(get point_dep) VASILIADI "KSENIYA VALEREVNA"))
$(set pax_id_04 $(get_pax_id $(get point_dep) CHEKMAREV "RONALD"))
$(set pax_id_05 $(get_pax_id $(get point_dep) VERGUNOV "VASILII LEONIDOVICH"))
$(set pax_id_06 $(get_pax_id $(get point_dep) „’…‚€ "‹ €‹…‘€„‚€"))
$(set pax_id_07 $(get_pax_id $(get point_dep) —€‚ "•€‹ ƒ…€„…‚—"))
$(set pax_id_08 $(get_pax_id $(get point_dep) —€‚ "‹€‰"))


$(SAVE_ET_DISP $(get point_dep) 2986145134262 cpnno=2 STIPIDI "ANGELINA"
  recloc=F522FC depp=$(get airp_dep) arrp=$(get airp_arv))
$(SAVE_ET_DISP $(get point_dep) 2986145134263 cpnno=2 AKOPOVA "OLIVIIA"
  recloc=F522FC depp=$(get airp_dep) arrp=$(get airp_arv))
$(SAVE_ET_DISP $(get point_dep) 2986145143703 cpnno=2 VASILIADI "KSENIYA VALEREVNA"
  recloc=F52MM0 depp=$(get airp_dep) arrp=$(get airp_arv))
$(SAVE_ET_DISP $(get point_dep) 2986145143704 cpnno=2 CHEKMAREV "RONALD"
  recloc=F52MM0 depp=$(get airp_dep) arrp=$(get airp_arv))
$(SAVE_ET_DISP $(get point_dep) 2982425618100 cpnno=1 „’…‚€ "‹ €‹…‘€„‚€"
  recloc=F43LF1 depp=$(get airp_dep) arrp=$(get airp_arv))
$(SAVE_ET_DISP $(get point_dep) 2982425618102 cpnno=1 —€‚ "•€‹ ƒ…€„…‚—"
  recloc=F43LF1 depp=$(get airp_dep) arrp=$(get airp_arv))
$(SAVE_ET_DISP $(get point_dep) 2982425618101 cpnno=1 —€‚ "‹€‰"
  recloc=F43LF1 depp=$(get airp_dep) arrp=$(get airp_arv))

$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv) hall=1
{
<passengers>
  <pax>
    <pax_id>$(get pax_id_06)</pax_id>
    <surname>„’…‚€</surname>
    <name>‹ €‹…‘€„‚€</name>
    <pers_type>‚‡</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>2982425618100</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>RUS</issue_country>
      <no>0310526187</no>
      <nationality>RUS</nationality>
      <birth_date>23.08.1985 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>„’…‚€</surname>
      <first_name>‹ €‹…‘€„‚€</first_name>
    </document>
    <subclass>P</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id_07)</pax_id>
    <surname>—€‚</surname>
    <name>•€‹ ƒ…€„…‚—</name>
    <pers_type></pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>2982425618102</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>F</type>
      <issue_country>RUS</issue_country>
      <no>V€ƒ568572</no>
      <nationality>RUS</nationality>
      <birth_date>21.11.$(date_format %Y -5y) 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>—€‚</surname>
      <first_name>•€‹ ƒ…€„…‚—</first_name>
    </document>
    <subclass>P</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id_08 )</pax_id>
    <surname>—€‚</surname>
    <name>‹€‰</name>
    <pers_type></pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>2982425618101</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>F</type>
      <issue_country>RUS</issue_country>
      <no>V€ƒ841650</no>
      <nationality>RUS</nationality>
      <birth_date>11.08.$(date_format %Y -2y) 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>—€‚</surname>
      <first_name>•€‹ ƒ…€„…‚—</first_name>
    </document>
    <subclass>P</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
</passengers>
})

$(set edi_ref2 $(last_edifact_ref 2))
$(set edi_ref1 $(last_edifact_ref 1))
$(set edi_ref0 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref2) ’ 2982425618100 1 CK xxxxxx „„ ‘— 280 depd=$(ddmmyy -1))
>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref1) ’ 2982425618101 1 CK xxxxxx „„ ‘— 280 depd=$(ddmmyy -1))
>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref0) ’ 2982425618102 1 CK xxxxxx „„ ‘— 280 depd=$(ddmmyy -1))

<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref2) 2982425618100 1 CK)
<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref1) 2982425618101 1 CK)
<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref0) 2982425618102 1 CK)

$(KICK_IN_SILENT)

$(set grp_id $(get_single_grp_id $(get point_dep) „’…‚€ "‹ €‹…‘€„‚€"))
$(set grp_tid $(get_single_tid $(get point_dep) „’…‚€ "‹ €‹…‘€„‚€"))
$(set pax_tid_06 $(get_single_pax_tid $(get point_dep) „’…‚€ "‹ €‹…‘€„‚€"))
$(set pax_tid_07 $(get_single_pax_tid $(get point_dep) —€‚ "•€‹ ƒ…€„…‚—"))
$(set pax_tid_08 $(get_single_pax_tid $(get point_dep) —€‚ "‹€‰"))

$(CHANGE_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv) $(get grp_id) $(get grp_tid)
{
<passengers>
  <pax>
    <pax_id>$(get pax_id_06)</pax_id>
    <surname>„’…‚€</surname>
    <name>‹ €‹…‘€„‚€</name>
    <pers_type>‚‡</pers_type>
    <refuse>€</refuse>
    <ticket_no>2982425618100</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>RUS</issue_country>
      <no>0310526187</no>
      <nationality>RUS</nationality>
      <birth_date>23.08.1985 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>„’…‚€</surname>
      <first_name>‹ €‹…‘€„‚€</first_name>
    </document>
    <subclass>P</subclass>
    <bag_pool_num/>
    <transfer/>
    <tid>$(get pax_tid_06)</tid>
  </pax>
  <pax>
    <pax_id>$(get pax_id_07)</pax_id>
    <surname>—€‚</surname>
    <name>•€‹ ƒ…€„…‚—</name>
    <pers_type></pers_type>
    <refuse></refuse>
    <ticket_no>2982425618102</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>F</type>
      <issue_country>RUS</issue_country>
      <no>V€ƒ568572</no>
      <nationality>RUS</nationality>
      <birth_date>21.11.$(date_format %Y -5y) 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>—€‚</surname>
      <first_name>•€‹ ƒ…€„…‚—</first_name>
    </document>
    <subclass>P</subclass>
    <bag_pool_num/>
    <transfer/>
    <tid>$(get pax_tid_07)</tid>
  </pax>
  <pax>
    <pax_id>$(get pax_id_08)</pax_id>
    <surname>—€‚</surname>
    <name>‹€‰</name>
    <pers_type></pers_type>
    <refuse>€</refuse>
    <ticket_no>2982425618101</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>F</type>
      <issue_country>RUS</issue_country>
      <no>V€ƒ841650</no>
      <nationality>RUS</nationality>
      <birth_date>11.08.$(date_format %Y -2y) 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>—€‚</surname>
      <first_name>•€‹ ƒ…€„…‚—</first_name>
    </document>
    <subclass>P</subclass>
    <bag_pool_num/>
    <transfer/>
    <tid>$(get pax_tid_08)</tid>
  </pax>
</passengers>
})

$(set edi_ref2 $(last_edifact_ref 2))
$(set edi_ref1 $(last_edifact_ref 1))
$(set edi_ref0 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref2) ’ 2982425618100 1 I xxxxxx „„ ‘— 280 depd=$(ddmmyy -1))
>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref1) ’ 2982425618101 1 I xxxxxx „„ ‘— 280 depd=$(ddmmyy -1))
>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref0) ’ 2982425618102 1 I xxxxxx „„ ‘— 280 depd=$(ddmmyy -1))

<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref2) 2982425618100 1 I)
<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref1) 2982425618101 1 I)
<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref0) 2982425618102 1 I)

$(KICK_IN_SILENT)

$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv) hall=1
{
<passengers>
  <pax>
    <pax_id>$(get pax_id_03)</pax_id>
    <surname>VASILIADI</surname>
    <name>KSENIYA VALEREVNA</name>
    <pers_type>‚‡</pers_type>
    <seat_no>6ƒ</seat_no>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>2986145143703</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>RUS</issue_country>
      <no>0307611933</no>
      <nationality>RUS</nationality>
      <birth_date>13.09.1984 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>VASILIADI</surname>
      <first_name>KSENIYA VALEREVNA</first_name>
    </document>
    <subclass>L</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id_04)</pax_id>
    <surname>CHEKMAREV</surname>
    <name>RONALD</name>
    <pers_type></pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>0</seats>
    <ticket_no>2986145143704</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>F</type>
      <issue_country>RUS</issue_country>
      <no>V€ƒ815247</no>
      <nationality>RUS</nationality>
      <birth_date>29.01.$(date_format %Y -1y) 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>CHEKMAREV</surname>
      <first_name>RONALD KONSTANTINOVICH</first_name>
    </document>
    <subclass>L</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
</passengers>
})

$(set edi_ref1 $(last_edifact_ref 1))
$(set edi_ref0 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref1) ’ 2986145143703 2 CK xxxxxx „„ ‘— 280 depd=$(ddmmyy -1))
>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref0) ’ 2986145143704 2 CK xxxxxx „„ ‘— 280 depd=$(ddmmyy -1))

<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref1) 2986145143703 2 CK)
<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref0) 2986145143704 2 CK)

$(KICK_IN_SILENT)

$(combine_brd_with_reg $(get point_dep))

$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv) hall=1
{
<passengers>
  <pax>
    <pax_id>$(get pax_id_01)</pax_id>
    <surname>STIPIDI</surname>
    <name>ANGELINA</name>
    <pers_type>‚‡</pers_type>
    <seat_no>11</seat_no>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>2986145134262</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>RU</issue_country>
      <no>0305555064</no>
      <nationality>RU</nationality>
      <birth_date>23.07.1982 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>STIPIDI</surname>
      <first_name>ANGELINA</first_name>
    </document>
    <subclass>V</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id_02)</pax_id>
    <surname>AKOPOVA</surname>
    <name>OLIVIIA</name>
    <pers_type></pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>0</seats>
    <ticket_no>2986145134263</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>F</type>
      <issue_country>RU</issue_country>
      <no>VIAG519994</no>
      <nationality>RU</nationality>
      <birth_date>22.08.$(date_format %Y -1y) 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>AKOPOVA</surname>
      <first_name>OLIVIIA</first_name>
    </document>
    <subclass>V</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
</passengers>
})

$(set edi_ref1 $(last_edifact_ref 1))
$(set edi_ref0 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref1) ’ 2986145134262 2 BD xxxxxx „„ ‘— 280 depd=$(ddmmyy -1))
>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref0) ’ 2986145134263 2 BD xxxxxx „„ ‘— 280 depd=$(ddmmyy -1))

<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref1) 2986145134262 2 BD)
<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref0) 2986145134263 2 BD)

$(KICK_IN_SILENT)

$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_03) 777 "" 1)

### ―ΰ®αβ Ά«ο¥¬ Ά§«¥β

$(CHANGE_SPP_FLIGHT_ONE_LEG $(get point_dep) $(get time_dep) "" $(get airline) $(get flt_no) $(get craft) $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv))

$(run_et_flt_task)

$(set edi_ref2 $(last_edifact_ref 2))
$(set edi_ref1 $(last_edifact_ref 1))
$(set edi_ref0 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref2) ’ 2986145134262 2 B xxxxxx „„ ‘— 280 V depd=$(ddmmyy -1))
>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref1) ’ 2986145134263 2 B xxxxxx „„ ‘— 280 V depd=$(ddmmyy -1))
>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref0) ’ 2986145143703 2 B xxxxxx „„ ‘— 280 L depd=$(ddmmyy -1))

<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref2) 2986145134262 2 B)
<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref1) 2986145134263 2 B)
<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref0) 2986145143703 2 B)

%%

### test 6 - ¨§¬¥­¥­¨¥ αβ βγα®Ά ―ΰ¨ ®β«®¦¥­­®© α¬¥­¥ αβ βγα  
#########################################################################################

$(init_term)

$(set_user_time_type LocalAirp PIKE)

$(set airline UT)
$(set flt_no 280)
$(set craft TU5)
$(set airp_dep DME)
$(set time_dep "$(date_format %d.%m.%Y -1) 07:00")
$(set time_arv "$(date_format %d.%m.%Y -1) 10:00")
$(set airp_arv AER)

$(NEW_SPP_FLIGHT_ONE_LEG $(get airline) $(get flt_no) $(get craft) $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv))

$(INB_PNL_UT $(get airp_dep) $(get airp_arv) $(get flt_no) $(ddmon -1))

$(init_eds ’ UTET UTDC)
$(cache PIKE RU DESK_GRP_SETS $(cache_iface_ver DESK_GRP_SETS) ""
  insert grp_id:1 defer_etstatus:1)

$(set point_dep $(last_point_id_spp))

$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id_06 $(get_pax_id $(get point_dep) „’…‚€ "‹ €‹…‘€„‚€"))
$(set pax_id_07 $(get_pax_id $(get point_dep) —€‚ "•€‹ ƒ…€„…‚—"))
$(set pax_id_08 $(get_pax_id $(get point_dep) —€‚ "‹€‰"))

$(SAVE_ET_DISP $(get point_dep) 2982425618100 „’…‚€ "‹ €‹…‘€„‚€"
  recloc=F43LF1 depp=$(get airp_dep) arrp=$(get airp_arv))
$(SAVE_ET_DISP $(get point_dep) 2982425618102 —€‚ "•€‹ ƒ…€„…‚—"
  recloc=F43LF1 depp=$(get airp_dep) arrp=$(get airp_arv))
$(SAVE_ET_DISP $(get point_dep) 2982425618101 —€‚ "‹€‰"
  recloc=F43LF1 depp=$(get airp_dep) arrp=$(get airp_arv))

$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv) hall=1
{
<passengers>
  <pax>
    <pax_id>$(get pax_id_06)</pax_id>
    <surname>„’…‚€</surname>
    <name>‹ €‹…‘€„‚€</name>
    <pers_type>‚‡</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>2982425618100</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>1</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>RUS</issue_country>
      <no>0310526187</no>
      <nationality>RUS</nationality>
      <birth_date>23.08.1985 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>„’…‚€</surname>
      <first_name>‹ €‹…‘€„‚€</first_name>
    </document>
    <subclass>P</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id_07)</pax_id>
    <surname>—€‚</surname>
    <name>•€‹ ƒ…€„…‚—</name>
    <pers_type></pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>2982425618102</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>1</ticket_confirm>
    <document>
      <type>F</type>
      <issue_country>RUS</issue_country>
      <no>V€ƒ568572</no>
      <nationality>RUS</nationality>
      <birth_date>21.11.$(date_format %Y -5y) 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>—€‚</surname>
      <first_name>•€‹ ƒ…€„…‚—</first_name>
    </document>
    <subclass>P</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id_08 )</pax_id>
    <surname>—€‚</surname>
    <name>‹€‰</name>
    <pers_type></pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>2982425618101</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>1</ticket_confirm>
    <document>
      <type>F</type>
      <issue_country>RUS</issue_country>
      <no>V€ƒ841650</no>
      <nationality>RUS</nationality>
      <birth_date>11.08.$(date_format %Y -2y) 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>—€‚</surname>
      <first_name>•€‹ ƒ…€„…‚—</first_name>
    </document>
    <subclass>P</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
</passengers>
})

$(set grp_id $(get_single_grp_id $(get point_dep) „’…‚€ "‹ €‹…‘€„‚€"))
$(set grp_tid $(get_single_tid $(get point_dep) „’…‚€ "‹ €‹…‘€„‚€"))
$(set pax_tid_06 $(get_single_pax_tid $(get point_dep) „’…‚€ "‹ €‹…‘€„‚€"))
$(set pax_tid_07 $(get_single_pax_tid $(get point_dep) —€‚ "•€‹ ƒ…€„…‚—"))
$(set pax_tid_08 $(get_single_pax_tid $(get point_dep) —€‚ "‹€‰"))

!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='ETStatus' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <ChangeGrpStatus>
      <segments>
        <segment>
          <grp_id>$(get grp_id)</grp_id>
        </segment>
      </segments>
    </ChangeGrpStatus>
  </query>
</term>}

$(set edi_ref2 $(last_edifact_ref 2))
$(set edi_ref1 $(last_edifact_ref 1))
$(set edi_ref0 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref2) ’ 2982425618100 1 CK xxxxxx „„ ‘— 280 depd=$(ddmmyy -1))
>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref1) ’ 2982425618101 1 CK xxxxxx „„ ‘— 280 depd=$(ddmmyy -1))
>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref0) ’ 2982425618102 1 CK xxxxxx „„ ‘— 280 depd=$(ddmmyy -1))

<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref2) 2982425618100 1 CK)
<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref1) 2982425618101 1 CK)
<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref0) 2982425618102 1 CK)

$(KICK_IN_SILENT)

$(CHANGE_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv) $(get grp_id) $(get grp_tid) hall=1
{
<passengers>
  <pax>
    <pax_id>$(get pax_id_06)</pax_id>
    <surname>„’…‚€</surname>
    <name>‹ €‹…‘€„‚€</name>
    <pers_type>‚‡</pers_type>
    <refuse/>
    <ticket_no>2982425618100</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>RUS</issue_country>
      <no>0310526187</no>
      <nationality>RUS</nationality>
      <birth_date>23.08.1985 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>„’…‚€</surname>
      <first_name>‹ €‹…‘€„‚€</first_name>
    </document>
    <subclass>P</subclass>
    <bag_pool_num/>
    <transfer/>
    <tid>$(get pax_tid_06)</tid>
  </pax>
  <pax>
    <pax_id>$(get pax_id_07)</pax_id>
    <surname>—€‚</surname>
    <name>•€‹ ƒ…€„…‚—</name>
    <pers_type></pers_type>
    <refuse></refuse>
    <ticket_no>2982425618102</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>F</type>
      <issue_country>RUS</issue_country>
      <no>V€ƒ568572</no>
      <nationality>RUS</nationality>
      <birth_date>21.11.$(date_format %Y -5y) 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>—€‚</surname>
      <first_name>•€‹ ƒ…€„…‚—</first_name>
    </document>
    <subclass>P</subclass>
    <bag_pool_num/>
    <transfer/>
    <tid>$(get pax_tid_07)</tid>
  </pax>
  <pax>
    <pax_id>$(get pax_id_08)</pax_id>
    <surname>—€‚</surname>
    <name>‹€‰</name>
    <pers_type></pers_type>
    <refuse>€</refuse>
    <ticket_no>2982425618101</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>F</type>
      <issue_country>RUS</issue_country>
      <no>V€ƒ841650</no>
      <nationality>RUS</nationality>
      <birth_date>11.08.$(date_format %Y -2y) 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>—€‚</surname>
      <first_name>•€‹ ƒ…€„…‚—</first_name>
    </document>
    <subclass>P</subclass>
    <bag_pool_num/>
    <transfer/>
    <tid>$(get pax_tid_08)</tid>
  </pax>
</passengers>
})

!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='ETStatus' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <ChangeGrpStatus>
      <segments>
        <segment>
          <grp_id>$(get grp_id)</grp_id>
          <check_point_id>$(get point_dep)</check_point_id>
        </segment>
      </segments>
    </ChangeGrpStatus>
  </query>
</term>}

$(set edi_ref1 $(last_edifact_ref 1))
$(set edi_ref0 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref1) ’ 2982425618102 1 I xxxxxx „„ ‘— 280 depd=$(ddmmyy -1))
>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref0) ’ 2982425618101 1 I xxxxxx „„ ‘— 280 depd=$(ddmmyy -1))

<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref1) 2982425618102 1 I)
<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref0) 2982425618101 1 I)

$(KICK_IN_SILENT)



