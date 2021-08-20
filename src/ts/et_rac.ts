include(ts/macro.ts)
include(ts/spp/write_dests_macro.ts)
include(ts/pax/checkin_macro.ts)
include(ts/pax/boarding_macro.ts)
include(ts/pax/et_emd_macro.ts)

# meta: suite eticket

$(defmacro PREPARE_SABRE_EXCHANGE_SETTINGS
  airline
  flt_no
{
$(cache PIKE RU ROT $(cache_iface_ver ROT) ""
  insert canon_name:1H1SP
         ip_address:0.0.0.0
         ip_port:8888
         h2h:1
         our_h2h_addr:1H1SIETQ
         h2h_addr:1S1HIETR
         h2h_rem_addr_num:1
         resp_timeout:20
         router_translit:1)

$(cache PIKE RU EDI_ADDRS $(cache_iface_ver EDI_ADDRS) ""
  insert addr:1SIET canon_name:1H1SP)

$(cache PIKE RU EDIFACT_PROFILES $(cache_iface_ver EDIFACT_PROFILES) ""
  insert name:CONTROL_SABRE_SU
         version:0
         sub_version:1
         ctrl_agency:IA
         syntax_name:IATA
         syntax_ver:1)

$(cache PIKE RU ET_ADDR_SET $(cache_iface_ver ET_ADDR_SET) ""
  insert airline:$(get_elem_id etAirline $(airline))
         flt_no:$(flt_no)
         edi_addr:1SIET
         edi_addr_ext:SU
         edi_own_addr:1HIET
         edi_own_addr_ext:1HDCS
         edifact_profile:CONTROL_SABRE_SU)
})

$(defmacro PREPARE_AC_SETTINGS
  airline
  flt_no
  airp_dep
{
$(cache PIKE RU MISC_SET $(cache_iface_ver MISC_SET) ""
  insert type_code:11
         airline:$(get_elem_id etAirline $(airline))
         flt_no:$(flt_no)
         airp_dep:$(get_elem_id etAirp $(airp_dep))
         pr_misc:0)

$(cache PIKE RU MISC_SET $(cache_iface_ver MISC_SET) ""
  insert type_code:43
         airline:$(get_elem_id etAirline $(airline))
         flt_no:$(flt_no)
         airp_dep:$(get_elem_id etAirp $(airp_dep))
         pr_misc:1)
})

$(defmacro CHECK_ET_STATUS_VIEW
  point_dep
  ticket_no
  coupon_no
  status
{
$(SEARCH_ET_BY_TICK_NO_ADVANCED capture=on $(point_dep) $(ticket_no) $(coupon_no))

>> lines=auto
              <coup_status index='16'>$(status)</coup_status>

})

# test 1
#########################################################################################

$(init_term)

$(init_eds ЮТ UTET UTDC translit)

$(PREPARE_FLIGHT_1PAX_1SEG ЮТ 103 ДМД ПЛК REPIN IVAN)

# забирают контроль, которого ещё и нет вовсе у нас
<<
UNB+SIRE:1+UTET+UTDC+170703:1027+FFD507DE140001+++O"
UNH+1+TKCREQ:96:2:IA+FFD507DE14"
MSG+:142"
ORG+UT+52519950+++A++SYSTEM"
EQN+1:TD"
TKT+2982348111616:T:1"
CPN+1:701"
UNT+7+1"
UNZ+1+FFD507DE140001"

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+FFD507DE140001+++T"
UNH+1+TKCRES:96:2:IA+FFD507DE14"
MSG+:142+7"
ERC+401"
UNT+4+1"
UNZ+1+FFD507DE140001"


$(REQUEST_AC_BY_TICK_NO_CPN_NO $(last_point_id_spp) 2982348111610 1)
>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:734"
ORG+1H:MOW+++UT+Y+::EN+MOVROM"
EQN+1:TD"
TKT+2982348111610:T:1:3"
CPN+1:AL::E"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+UTET+UTDC+170706:0859+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:96:2:IA+$(last_edifact_ref)"
MSG+:734+7"
ERC+396"
UNT+4+1"
UNZ+1+$(last_edifact_ref)0001"


$(REQUEST_AC_BY_TICK_NO_CPN_NO $(last_point_id_spp) 2982348111616 1)
>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:734"
ORG+1H:MOW+++UT+Y+::EN+MOVROM"
EQN+1:TD"
TKT+2982348111616:T:1:3"
CPN+1:AL::E"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+UTET+UTDC+170706:0859+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref)"
MSG+:734+3"
EQN+1:TD"
TIF+KOLLEROV:A+DANILA"
TAI+7906+WS/SU:B"
RCI+1A:SKWZAE:1+5N:033DC7:1"
MON+G:00010+T:4150:RUB+B:2450:RUB"
FOP+CA:3:4150"
PTK+++$(ddmmyy -100)"
ODI+MOW+REN"
ORG+1A:MUC+92229196:121255+MOW++A++SYSTEM"
EQN+1:TF"
TXD+700+1500:::YQ+200:::YR"
IFT+4:39+RUSSIA+WHITE TREVEL"
IFT+4:5+79033908109"
IFT+4:10+CHNG BEF DEP/REFUND RESTR"
IFT+4:15:0+MOW 5N REN2450.00RUB2450.00END"
TKT+2982348111616:T:1:3"
CPN+1:I::E"
TVL+$(ddmmyy):0150+DME+LED+UT+103:A++1"
TVL+$(ddmmyy):0150+DME+LED+:UR+103:A++1"
RPI++OK"
PTS++APROW"
EBD++1::N"
DAT+B:$(ddmmyy)+A:$(ddmmyy)"
UNT+26+1"
UNZ+1+$(last_edifact_ref)0001"


# считаем купон зарегистрированным - контроль вернуть не сможем
# $(sql "update wc_coupon set status=8")

$(update_pg_coupon 8 2982348111616 1)

# забирают контроль обратно - не отдадим
<<
UNB+SIRE:1+UTET+UTDC+170703:1027+FFD507DE140001+++O"
UNH+1+TKCREQ:96:2:IA+FFD507DE14"
MSG+:142"
ORG+UT+52519950+++A++SYSTEM"
EQN+1:TD"
TKT+2982348111616:T:1"
CPN+1:701"
UNT+7+1"
UNZ+1+FFD507DE140001"


>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+FFD507DE140001+++T"
UNH+1+TKCRES:96:2:IA+FFD507DE14"
MSG+:142+7"
ERC+396"
UNT+4+1"
UNZ+1+FFD507DE140001"


# считаем контроль открытым - контроль вернём
# $(sql "update wc_coupon set status=1")

$(update_pg_coupon 1 2982348111616 1)

# забирают контроль обратно - отдадим
<<
UNB+SIRE:1+UTET+UTDC+170703:1027+FFD507DE140001+++O"
UNH+1+TKCREQ:96:2:IA+FFD507DE14"
MSG+:142"
ORG+UT+52519950+++A++SYSTEM"
EQN+1:TD"
TKT+2982348111616:T:1"
CPN+1:701"
UNT+7+1"
UNZ+1+FFD507DE140001"

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+FFD507DE140001+++T"
UNH+1+TKCRES:96:2:IA+FFD507DE14"
MSG+:142+3"
TKT+2982348111616:T:1:3"
CPN+1:I::E:::AL"
UNT+5+1"
UNZ+1+FFD507DE140001"


%%

# test 2
#########################################################################################
#
# Забирают Flown у нас
#

$(init_term)

$(init_eds ЮТ UTET UTDC translit)

$(PREPARE_FLIGHT_1PAX_1SEG ЮТ 103 ДМД ПЛК REPIN IVAN)


$(REQUEST_AC_BY_TICK_NO_CPN_NO $(last_point_id_spp) 2982348111616 1)
>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:734"
ORG+1H:MOW+++UT+Y+::EN+MOVROM"
EQN+1:TD"
TKT+2982348111616:T:1:3"
CPN+1:AL::E"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+UTET+UTDC+170706:0859+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref)"
MSG+:734+3"
EQN+1:TD"
TIF+KOLLEROV:A+DANILA"
TAI+7906+WS/SU:B"
RCI+1A:SKWZAE:1+5N:033DC7:1"
MON+G:00010+T:4150:RUB+B:2450:RUB"
FOP+CA:3:4150"
PTK+++$(ddmmyy -100)"
ODI+MOW+REN"
ORG+1A:MUC+92229196:121255+MOW++A++SYSTEM"
EQN+1:TF"
TXD+700+1500:::YQ+200:::YR"
IFT+4:39+RUSSIA+WHITE TREVEL"
IFT+4:5+79033908109"
IFT+4:10+CHNG BEF DEP/REFUND RESTR"
IFT+4:15:0+MOW 5N REN2450.00RUB2450.00END"
TKT+2982348111616:T:1:3"
CPN+1:I::E"
TVL+$(ddmmyy):0150+DME+LED+UT+103:A++1"
TVL+$(ddmmyy):0150+DME+LED+:UR+103:A++1"
RPI++OK"
PTS++APROW"
EBD++1::N"
DAT+B:$(ddmmyy)+A:$(ddmmyy)"
UNT+26+1"
UNZ+1+$(last_edifact_ref)0001"


# делаем купон Flown - контроль вернём контроль
#$(sql "update wc_coupon set status=10")

$(update_pg_coupon 10 2982348111616 1)

<<
UNB+SIRE:1+UTET+UTDC+170703:1027+FFD507DE140001+++O"
UNH+1+TKCREQ:96:2:IA+FFD507DE14"
MSG+:142"
ORG+UT+52519950+++A++SYSTEM"
EQN+1:TD"
TKT+2982348111616:T:1"
CPN+1:701"
UNT+7+1"
UNZ+1+FFD507DE140001"

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+FFD507DE140001+++T"
UNH+1+TKCRES:96:2:IA+FFD507DE14"
MSG+:142+3"
TKT+2982348111616:T:1:3"
CPN+1:B::E:::AL"
UNT+5+1"
UNZ+1+FFD507DE140001"

%%

### test 3
###  1. Отдают контроль по некоторым пассажирам до прихода PNL
###  2. Загружаем PNL
###  3. Отдают контроль по некоторым пассажирам после прихода PNL, но не по всем
###  4. Забирают контроль у одного дважды, отдают контроль обратно дважды
###  5. Пытаются забрать контроль, которого не было никогда
###  6. Регистрируем пассажиров с полученным контролем
###  7. Пытаемся регистрировать пассажира без контроля, пытаемся забирать контроль - ошибка
###  8. Пытаемся регистрировать пассажира опять, забираем контроль, регистрируем
###  9. Пытаются забрать контроль по зарегистрированным пассажирам - не отдаем
### 10. Сажаем пассажиров
### 11. Пытаются забрать контроль по посаженным пассажирам - не отдаем
### 12. Отменяем регистрацию пассажира по 2 разным причинам, в т.ч. ошибка агента
### 13. Пытаются забрать контроль по разрегистрированным пассажирам - отдаем
### 14. Забираем обратно контроль по разрегистрированным пассажирам (ответы приходят вразнобой)
### 15. Проставляем вылет.
### 16. Отдаем контроль только по незарегистрированным и посаженным пассажирам.
### 17. Некоторые контроли не забирают сразу, но забирают позже
#########################################################################################

$(init_term)

$(set airline UT)
$(set flt_no 280)
$(set craft TU5)
$(set airp_dep DME)
$(set time_dep "$(date_format %d.%m.%Y -1) 07:00")
$(set time_arv "$(date_format %d.%m.%Y -1) 10:00")
$(set airp_arv AER)

$(PREPARE_SABRE_EXCHANGE_SETTINGS $(get airline))
$(PREPARE_AC_SETTINGS $(get airline) $(get flt_no))

#########################################################################################
###  1. Отдают контроль по некоторым пассажирам до прихода PNL

$(UAC_OK 1HIET::1HDCS 1SIET::SU ediref=FFD507DE13
         2986145108615 1 BUMBURIDI "EKATERINA SERGEEVNA" F50234 K
         $(get airline) $(get flt_no) $(get airp_dep) $(ddmmyy -1) 0700 $(get airp_arv))

$(UAC_OK 1HIET::1HDCS 1SIET::SU ediref=FFD507DE14
         2982425618100 1 DMITRIEVA "IULIIA ALEKSANDROVNA" F43LF1 P
         $(get airline) $(get flt_no) $(get airp_dep) $(ddmmyy -1) 0700 $(get airp_arv))

$(UAC_OK 1HIET::1HDCS 1SIET::SU ediref=FFD507DE15
         2982425618102 1 CHARKOV "MIKHAIL GENNADEVICH" F43LF1 P
         $(get airline) $(get flt_no) $(get airp_dep) $(ddmmyy -1) 0700 $(get airp_arv))

$(UAC_OK 1HIET::1HDCS 1SIET::SU ediref=FFD507DE16
         2982425618101 1 CHARKOV "NIKOLAI" F43LF1 P
         $(get airline) $(get flt_no) $(get airp_dep) $(ddmmyy -1) 0700 $(get airp_arv))

#########################################################################################
###  2. Загружаем PNL

$(INB_PNL_UT $(get airp_dep) $(get airp_arv) $(get flt_no) $(ddmon -1))

#########################################################################################
###  3. Отдают контроль по некоторым пассажирам после прихода PNL, но не по всем

$(UAC_OK 1HIET::1HDCS 1SIET::SU ediref=FFD507DE17
         2986145143703 2 VASILIADI "KSENIYA VALEREVNA" F52MM0 L
         $(get airline) $(get flt_no) $(get airp_dep) $(ddmmyy -1) 0700 $(get airp_arv))

$(UAC_OK 1HIET::1HDCS 1SIET::SU ediref=FFD507DE18
         2986145143704 2 CHEKMAREV "RONALD" F52MM0 L
         $(get airline) $(get flt_no) $(get airp_dep) $(ddmmyy -1) 0700 $(get airp_arv))

###  Тут и рейс подоспел в СПП

$(NEW_SPP_FLIGHT_ONE_LEG $(get airline) $(get flt_no) $(get craft) $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv))

$(set point_dep $(last_point_id_spp))

###  После того как рейс СПП связался с PNL, начинаем писать в журнал операций получение контроля

$(UAC_OK 1HIET::1HDCS 1SIET::SU ediref=FFD507DE19
         2986145134262 2 STIPIDI "ANGELINA" F522FC V
         $(get airline) $(get flt_no) $(get airp_dep) $(ddmmyy -1) 0700 $(get airp_arv))

$(UAC_OK 1HIET::1HDCS 1SIET::SU ediref=FFD507DE1A
         2986145134263 2 AKOPOVA "OLIVIIA" F522FC V
         $(get airline) $(get flt_no) $(get airp_dep) $(ddmmyy -1) 0700 $(get airp_arv))

$(CHECK_ET_STATUS_VIEW $(get point_dep) 2982425618100 1 status=O)

#########################################################################################
###  4. Забирают контроль у одного дважды, отдают контроль обратно дважды

$(EXTRACT_AC_OK 1HIET::1HDCS 1SIET::SU ediref=FFD507DE1B 2986145143703 2 status=I)

$(EXTRACT_AC_ERROR 1HIET::1HDCS 1SIET::SU ediref=FFD507DE1C 2986145143703 2 error=396)

$(UAC_OK 1HIET::1HDCS 1SIET::SU ediref=FFD507DE1D
         2986145143703 2 VASILIADI "KSENIYA VALEREVNA" F52MM0 L
         $(get airline) $(get flt_no) $(get airp_dep) $(ddmmyy -1) 0700 $(get airp_arv))

$(UAC_OK 1HIET::1HDCS 1SIET::SU ediref=FFD507DE1E
         2986145143703 2 VASILIADI "KSENIYA VALEREVNA" F52MM0 L
         $(get airline) $(get flt_no) $(get airp_dep) $(ddmmyy -1) 0700 $(get airp_arv))

#########################################################################################
###  5. Пытаются забрать контроль, которого не было никогда

$(EXTRACT_AC_ERROR 1HIET::1HDCS 1SIET::SU ediref=FFD507DE1F 2986145143703 1 error=401)

#########################################################################################
###  6. Регистрируем пассажиров с полученным контролем

$(PREPARE_HALLS_FOR_BOARDING $(get airp_dep))

$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id_4262 $(get_pax_id $(get point_dep) STIPIDI ANGELINA))
$(set pax_id_4263 $(get_pax_id $(get point_dep) AKOPOVA OLIVIIA))
$(set pax_id_3703 $(get_pax_id $(get point_dep) VASILIADI "KSENIYA VALEREVNA"))
$(set pax_id_3704 $(get_pax_id $(get point_dep) CHEKMAREV "RONALD"))
$(set pax_id_2943 $(get_pax_id $(get point_dep) VERGUNOV "VASILII LEONIDOVICH"))
$(set pax_id_8100 $(get_pax_id $(get point_dep) ДМИТРИЕВА "ЮЛИЯ АЛЕКСАНДРОВНА"))
$(set pax_id_8102 $(get_pax_id $(get point_dep) ЧАРКОВ "МИХАИЛ ГЕННАДЬЕВИЧ"))
$(set pax_id_8101 $(get_pax_id $(get point_dep) ЧАРКОВ "НИКОЛАЙ"))

$(NEW_CHECKIN_REQUEST capture=on $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv) hall=1
{
<passengers>
  <pax>
$(NEW_CHECKIN_2982425618100 $(get pax_id_8100))
  </pax>
  <pax>
$(NEW_CHECKIN_2982425618102 $(get pax_id_8102))
  </pax>
  <pax>
$(NEW_CHECKIN_2982425618101 $(get pax_id_8101))
  </pax>
</passengers>
})

>> mode=regex
.*
            <reg_no>1</reg_no>.*
            <reg_no>2</reg_no>.*
            <reg_no>3</reg_no>.*


$(NEW_CHECKIN_REQUEST capture=on $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv) hall=1
{
<passengers>
  <pax>
$(NEW_CHECKIN_2986145143703 $(get pax_id_3703))
  </pax>
  <pax>
$(NEW_CHECKIN_2986145143704 $(get pax_id_3704))
  </pax>
</passengers>
})

>> mode=regex
.*
            <reg_no>4</reg_no>.*
            <reg_no>5</reg_no>.*

$(NEW_CHECKIN_REQUEST capture=on $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv) hall=1
{
<passengers>
  <pax>
$(NEW_CHECKIN_2986145134262 $(get pax_id_4262))
  </pax>
  <pax>
$(NEW_CHECKIN_2986145134263 $(get pax_id_4263))
  </pax>
</passengers>
})

>> mode=regex
.*
            <reg_no>6</reg_no>.*
            <reg_no>7</reg_no>.*

$(CHECK_ET_STATUS_VIEW $(get point_dep) 2982425618100 1 status=C)

#########################################################################################
###  7. Пытаемся регистрировать пассажира без контроля, пытаемся забирать контроль - ошибка

$(NEW_CHECKIN_REQUEST capture=on $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv) hall=1
{
<passengers>
  <pax>
$(NEW_CHECKIN_2986145212943 $(get pax_id_2943))
  </pax>
</passengers>
})

$(USER_ERROR_RESPONSE MSG.ETICK.NEED_DISPLAY)


$(SEARCH_ET_BY_TICK_NO_ADVANCED $(get point_dep) 2986145212943 1)

$(RAC_ERROR 1HIET::1HDCS 1SIET::SU $(last_edifact_ref) 2986145212943 1 $(get airline) error=396)

$(KICK_IN)

$(USER_ERROR_RESPONSE MSG.ETICK.ETS_ERROR)

#########################################################################################
###  8. Пытаемся регистрировать пассажира опять, забираем контроль, регистрируем

$(NEW_CHECKIN_REQUEST capture=on $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv) hall=1
{
<passengers>
  <pax>
$(NEW_CHECKIN_2986145212943 $(get pax_id_2943))
  </pax>
</passengers>
})

$(USER_ERROR_RESPONSE MSG.ETICK.NEED_DISPLAY)

$(pg_sql "DELETE FROM remote_results") ### !!! это специально, скорее всего из-за того что RAC_ERROR выше откатил транзакцию, в т.ч. и очистку remote_results

$(SEARCH_ET_BY_TICK_NO_ADVANCED $(get point_dep) 2986145212943 1)

$(RAC_OK 1HIET::1HDCS 1SIET::SU ediref=$(last_edifact_ref)
         2986145212943 1 VERGUNOV "VASILII LEONIDOVICH" F58457 W
         $(get airline) $(get flt_no) $(get airp_dep) $(ddmmyy -1) 0700 $(get airp_arv))

$(KICK_IN)

>> lines=auto
              <coup_status index='16'>A</coup_status>


$(NEW_CHECKIN_REQUEST capture=on $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv) hall=1
{
<passengers>
  <pax>
$(NEW_CHECKIN_2986145212943 $(get pax_id_2943))
  </pax>
</passengers>
})

>> mode=regex
.*
            <reg_no>8</reg_no>.*


$(CHECK_ET_STATUS_VIEW $(get point_dep) 2986145212943 1 status=C)

#########################################################################################
###  9. Пытаются забрать контроль по зарегистрированным пассажирам - не отдаем

$(EXTRACT_AC_ERROR 1HIET::1HDCS 1SIET::SU ediref=FFD507DE22 2986145143703 2 error=396)
$(EXTRACT_AC_ERROR 1HIET::1HDCS 1SIET::SU ediref=FFD507DE23 2986145143704 2 error=396)
$(EXTRACT_AC_ERROR 1HIET::1HDCS 1SIET::SU ediref=FFD507DE24 2982425618100 1 error=396)
$(EXTRACT_AC_ERROR 1HIET::1HDCS 1SIET::SU ediref=FFD507DE25 2982425618101 1 error=396)
$(EXTRACT_AC_ERROR 1HIET::1HDCS 1SIET::SU ediref=FFD507DE26 2982425618102 1 error=396)

#########################################################################################
### 10. Сажаем пассажиров

### убедимся, что статус не изменился на Boarded, потому что не в режиме изменения статуса при посадке

$(BOARDING_REQUEST_BY_PAX_ID capture=on $(get point_dep) $(get pax_id_4262) 777 "" 1)

>> lines=auto
      <updated>
        <pax_id>$(get pax_id_4262)</pax_id>
      </updated>
      <trip_sets>
        <pr_etl_only>1</pr_etl_only>
        <pr_etstatus>0</pr_etstatus>
      </trip_sets>

$(CHANGE_PAX_STATUS_REQUEST $(get pax_id_4262))

$(CHECK_ET_STATUS_VIEW $(get point_dep) 2986145134262 2 status=C)


### включим режим когда при посадке изменяется статус на Boarded

$(cache PIKE RU MISC_SET $(cache_iface_ver MISC_SET) ""
  insert type_code:81
         airline:$(get_elem_id etAirline $(get airline))
         flt_no:$(get flt_no)
         airp_dep:$(get_elem_id etAirp $(get airp_dep))
         pr_misc:1
 )

$(BOARDING_REQUEST_BY_PAX_ID capture=on $(get point_dep) $(get pax_id_4263) 777 "" 1)

>> lines=auto
      <updated>
        <pax_id>$(get pax_id_4263)</pax_id>
      </updated>
      <trip_sets>
        <pr_etl_only>0</pr_etl_only>
        <pr_etstatus>0</pr_etstatus>
      </trip_sets>

$(CHANGE_PAX_STATUS_REQUEST $(get pax_id_4263))

$(CHECK_ET_STATUS_VIEW $(get point_dep) 2986145134263 2 status=L)


$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_8101) 777 "" 1)
$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_8100) 777 "" 1)

#########################################################################################
### 11. Пытаются забрать контроль по посаженным пассажирам - не отдаем

$(EXTRACT_AC_ERROR 1HIET::1HDCS 1SIET::SU ediref=FFD507DE27 2982425618100 1 error=396)
$(EXTRACT_AC_ERROR 1HIET::1HDCS 1SIET::SU ediref=FFD507DE28 2982425618101 1 error=396)
$(EXTRACT_AC_ERROR 1HIET::1HDCS 1SIET::SU ediref=FFD507DE29 2986145134262 2 error=396)

#########################################################################################
### 12. Отменяем регистрацию пассажира по 2 разным причинам, в т.ч. ошибка агента

$(set grp_id $(get_single_grp_id $(get pax_id_8100)))
$(set grp_tid $(get_single_grp_tid $(get pax_id_8100)))
$(set pax_tid_8100 $(get_single_pax_tid $(get pax_id_8100)))
$(set pax_tid_8102 $(get_single_pax_tid $(get pax_id_8102)))
$(set pax_tid_8101 $(get_single_pax_tid $(get pax_id_8101)))

$(CHANGE_CHECKIN_REQUEST capture=on $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv) $(get grp_id) $(get grp_tid) hall=1
{
<passengers>
  <pax>
$(CHANGE_CHECKIN_2982425618100 $(get pax_id_8100) $(get pax_tid_8100) refuse=О)
  </pax>
  <pax>
$(CHANGE_CHECKIN_2982425618102 $(get pax_id_8102) $(get pax_tid_8102) refuse=О)
  </pax>
  <pax>
$(CHANGE_CHECKIN_2982425618101 $(get pax_id_8101) $(get pax_tid_8101) refuse=А)
  </pax>
</passengers>
})

>> mode=regex
.*
            <reg_no>1</reg_no>.*
            <reg_no>2</reg_no>.*


$(CHECK_ET_STATUS_VIEW $(get point_dep) 2982425618100 1 status=O)
$(CHECK_ET_STATUS_VIEW $(get point_dep) 2982425618102 1 status=O)
$(CHECK_ET_STATUS_VIEW $(get point_dep) 2982425618101 1 status=O)

#########################################################################################
### 13. Пытаются забрать контроль по разрегистрированным пассажирам - отдаем

$(EXTRACT_AC_OK 1HIET::1HDCS 1SIET::SU ediref=FFD507DE2A 2982425618100 1 status=I)
$(EXTRACT_AC_OK 1HIET::1HDCS 1SIET::SU ediref=FFD507DE2B 2982425618101 1 status=I)
$(EXTRACT_AC_OK 1HIET::1HDCS 1SIET::SU ediref=FFD507DE2C 2982425618102 1 status=I)

#########################################################################################
### 14. Забираем обратно контроль по разрегистрированным пассажирам (ответы приходят вразнобой)

### есть разница между тем, разрегистрирован пассажир по ошибке агента или по другой причине
### если пассажир разрегистрирован по другой причине, то мы не cможем взять контроль сами, а лишь покажем статус O

$(SEARCH_ET_BY_TICK_NO_ADVANCED $(get point_dep) 2982425618101 1)
$(set edi_ref0 $(last_edifact_ref))
$(RAC_REQUEST 1HIET::1HDCS 1SIET::SU $(get edi_ref0) 2982425618101 1 $(get airline))

$(SEARCH_ET_BY_TICK_NO_ADVANCED capture=on $(get point_dep) 2982425618100 1)

>> lines=auto
              <coup_status index='16'>O</coup_status>

$(SEARCH_ET_BY_TICK_NO_ADVANCED capture=on $(get point_dep) 2982425618102 1)

>> lines=auto
              <coup_status index='16'>O</coup_status>

$(RAC_RESPONSE_OK 1HIET::1HDCS 1SIET::SU $(get edi_ref0)
                  2982425618101 1 CHARKOV "NIKOLAI" F43LF1 P
                  $(get airline) $(get flt_no) $(get airp_dep) $(ddmmyy -1) 0700 $(get airp_arv))

$(KICK_IN)

>> lines=auto
              <coup_status index='16'>A</coup_status>

$(UAC_OK 1HIET::1HDCS 1SIET::SU ediref=FFD507DE2D
         2982425618100 1 DMITRIEVA "IULIIA ALEKSANDROVNA" F43LF1 P
         $(get airline) $(get flt_no) $(get airp_dep) $(ddmmyy -1) 0700 $(get airp_arv))


$(set grp_tid $(get_single_grp_tid $(get pax_id_8100)))
$(set pax_tid_8100 $(get_single_pax_tid $(get pax_id_8100)))
$(set pax_tid_8102 $(get_single_pax_tid $(get pax_id_8102)))

$(CHANGE_CHECKIN_REQUEST capture=on $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv) $(get grp_id) $(get grp_tid) hall=1
{
<passengers>
  <pax>
$(CHANGE_CHECKIN_2982425618100 $(get pax_id_8100) $(get pax_tid_8100) refuse=О)
  </pax>
  <pax>
$(CHANGE_CHECKIN_2982425618102 $(get pax_id_8102) $(get pax_tid_8102) refuse=А)
  </pax>
</passengers>
})

>> mode=regex
.*
            <reg_no>1</reg_no>.*


$(SEARCH_ET_BY_TICK_NO_ADVANCED $(get point_dep) 2982425618102 1)

$(RAC_OK 1HIET::1HDCS 1SIET::SU ediref=$(last_edifact_ref)
         2982425618102 1 CHARKOV "MIKHAIL GENNADEVICH" F43LF1 P
         $(get airline) $(get flt_no) $(get airp_dep) $(ddmmyy -1) 0700 $(get airp_arv))

$(KICK_IN)

>> lines=auto
              <coup_status index='16'>A</coup_status>


$(CHECK_ET_STATUS_VIEW $(get point_dep) 2982425618100 1 status=O)  #получен контроль только когда нам отдали они
$(CHECK_ET_STATUS_VIEW $(get point_dep) 2982425618102 1 status=A)  #сами взяли контроль
$(CHECK_ET_STATUS_VIEW $(get point_dep) 2982425618101 1 status=A)  #сами взяли контроль

### если мы взяли контроль, даже если пассажир не зарегистрирован, то они не смогут взять контроль сами обратно! это очень плохой момент!
### по реальному обмену видно, что в UAC статус I(O), а в RAC статус AL(A). В этом причина

$(EXTRACT_AC_ERROR    1HIET::1HDCS 1SIET::SU ediref=FFD507DE31 2982425618102 1 error=396)

#########################################################################################
### 15. Проставляем вылет.

$(CHANGE_SPP_FLIGHT_ONE_LEG $(get point_dep) $(get time_dep) "" $(get airline) $(get flt_no) $(get craft) $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv))

#########################################################################################
### 16. Отдаем контроль только по незарегистрированным и посаженным пассажирам.

$(defmacro TRY_PUSH_FINAL_STATUS
{

$(run_et_flt_task)

$(set edi_ref4 $(last_edifact_ref 4))
$(set edi_ref3 $(last_edifact_ref 3))
$(set edi_ref2 $(last_edifact_ref 2))
$(set edi_ref1 $(last_edifact_ref 1))
$(set edi_ref0 $(last_edifact_ref 0))

$(PUSH_AC_REQUEST 1HIET::1HDCS 1SIET::SU $(get edi_ref4)
                  2982425618101 1 status=I ""
                  $(get airline) $(get flt_no) $(get airp_dep) $(ddmmyy -1) 0700 $(get airp_arv))

$(PUSH_AC_REQUEST 1HIET::1HDCS 1SIET::SU $(get edi_ref3)
                  2982425618102 1 status=I ""
                  $(get airline) $(get flt_no) $(get airp_dep) $(ddmmyy -1) 0700 $(get airp_arv))

$(PUSH_AC_REQUEST 1HIET::1HDCS 1SIET::SU $(get edi_ref2)
                  2986145108615 1 status=I ""
                  $(get airline) $(get flt_no) $(get airp_dep) $(ddmmyy -1) 0700 $(get airp_arv))

$(PUSH_AC_REQUEST 1HIET::1HDCS 1SIET::SU $(get edi_ref1)
                  2986145134262 2 status=B subcl=V
                  $(get airline) $(get flt_no) $(get airp_dep) $(ddmmyy -1) 0700 $(get airp_arv))

$(PUSH_AC_REQUEST 1HIET::1HDCS 1SIET::SU $(get edi_ref0)
                  2986145134263 2 status=B subcl=V
                  $(get airline) $(get flt_no) $(get airp_dep) $(ddmmyy -1) 0700 $(get airp_arv))

})

$(TRY_PUSH_FINAL_STATUS)

??
$(dump_table trip_sets fields="pr_etstatus, et_final_attempt" display="on")

>> lines=auto
[0] [1] $()

$(TRY_PUSH_FINAL_STATUS)

$(PUSH_AC_RESPONSE_OK 1HIET::1HDCS 1SIET::SU $(get edi_ref0)
                      2986145134263 2 status=B)

$(PUSH_AC_RESPONSE_ERROR 1HIET::1HDCS 1SIET::SU $(get edi_ref2) 666) #666 от балды

$(PUSH_AC_RESPONSE_OK 1HIET::1HDCS 1SIET::SU $(get edi_ref4)
                      2982425618101 1 status=I)

$(PUSH_AC_RESPONSE_ERROR 1HIET::1HDCS 1SIET::SU $(get edi_ref1) 555) #555 от балды


$(CHECK_ET_STATUS_VIEW $(get point_dep) 2982425618101 1 status=O)
$(CHECK_ET_STATUS_VIEW $(get point_dep) 2982425618101 1 status=O)
$(CHECK_ET_STATUS_VIEW $(get point_dep) 2986145108615 1 status=O)
$(CHECK_ET_STATUS_VIEW $(get point_dep) 2986145134262 2 status=F)
$(CHECK_ET_STATUS_VIEW $(get point_dep) 2986145134263 2 status=F)

??
$(dump_table trip_sets fields="pr_etstatus, et_final_attempt" display="on")

>> lines=auto
[0] [2] $()

$(run_et_flt_task)

$(set edi_ref2 $(last_edifact_ref 2))
$(set edi_ref1 $(last_edifact_ref 1))
$(set edi_ref0 $(last_edifact_ref 0))

$(PUSH_AC_REQUEST 1HIET::1HDCS 1SIET::SU $(get edi_ref2)
                  2982425618102 1 status=I ""
                  $(get airline) $(get flt_no) $(get airp_dep) $(ddmmyy -1) 0700 $(get airp_arv))

$(PUSH_AC_REQUEST 1HIET::1HDCS 1SIET::SU $(get edi_ref1)
                  2986145108615 1 status=I ""
                  $(get airline) $(get flt_no) $(get airp_dep) $(ddmmyy -1) 0700 $(get airp_arv))

$(PUSH_AC_REQUEST 1HIET::1HDCS 1SIET::SU $(get edi_ref0)
                  2986145134262 2 status=B subcl=V
                  $(get airline) $(get flt_no) $(get airp_dep) $(ddmmyy -1) 0700 $(get airp_arv))

??
$(dump_table trip_sets fields="pr_etstatus, et_final_attempt" display="on")

>> lines=auto
[0] [3] $()

#########################################################################################
### 17. Некоторые контроли не забирают сразу, но забирают позже

$(EXTRACT_AC_OK    1HIET::1HDCS 1SIET::SU ediref=FFD507DE30 2982425618100 1 status=I)  #берут контроль, который мы не сможем отдать сами
$(EXTRACT_AC_OK    1HIET::1HDCS 1SIET::SU ediref=FFD507DE31 2982425618102 1 status=I)  #берут контроль, по которому раньше они не ответили
$(EXTRACT_AC_OK    1HIET::1HDCS 1SIET::SU ediref=FFD507DE32 2986145108615 1 status=I)  #берут контроль, по которому раньше они ругались
$(EXTRACT_AC_OK    1HIET::1HDCS 1SIET::SU ediref=FFD507DE33 2986145134262 2 status=B)  #берут контроль, по которому раньше они ругались
$(EXTRACT_AC_ERROR 1HIET::1HDCS 1SIET::SU ediref=FFD507DE34 2986145143703 2 error=396)
$(EXTRACT_AC_ERROR 1HIET::1HDCS 1SIET::SU ediref=FFD507DE35 2986145143704 2 error=396)
$(EXTRACT_AC_ERROR 1HIET::1HDCS 1SIET::SU ediref=FFD507DE36 2986145212943 1 error=396)


$(run_et_flt_task)

??
$(dump_table trip_sets fields="pr_etstatus, et_final_attempt" display="on")

>> lines=auto
[1] [3] $()

??
$(pg_dump_table etickets fields="ticket_no, coupon_no, coupon_status, error" order="ticket_no" display="on")
# Странные итоговые статусы в etickets:
# SELECT ticket_no,  coupon_no,  coupon_status,  error FROM etickets ORDER BY ticket_no

>> lines=auto
[2982425618100] [1] [O] [NULL] $()
[2982425618101] [1] [NULL] [NULL] $() # странный статус
[2982425618102] [1] [O] [NULL] $()
[2986145134262] [2] [C] [NULL] $()    # странный статус
[2986145134263] [2] [F] [NULL] $()
[2986145143703] [2] [C] [NULL] $()
[2986145143704] [2] [C] [NULL] $()
[2986145212943] [1] [C] [NULL] $()



