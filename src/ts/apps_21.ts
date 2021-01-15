include(ts/macro.ts)
include(ts/adm_macro.ts)

# meta: suite apps

###
#   Тест №1
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#            версия apps: 21
#
#   APPS-запрос на посадку ОДНОГО пассажира уходит во время регистрации
#   APPS-запрос на отмену ОДНОГО пассажира уходит при отмене регистрации
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps ЮТ ЦЗ APPS_21 closeout=true inbound=true outbound=false)

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set move_id $(get_move_id $(get point_dep)))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/1/////////

!! capture=on
$(GET_EVENTS $(get point_dep))

>> lines=auto
        <msg>Запрос на посадкудля пассажира TUMALI. Результатдля страны ЦЗ: Посадка разрешена.</msg>

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))
$(set tid $(get_single_tid $(get point_dep) TUMALI VALERII))

!!
$(CANCEL_PAX $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ)


>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/.*

# регистрация члена экипажа

$(CHECKIN_CREW $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ VOLODIN SEMEN ВЗ RUS 2124134 RUS 11.05.1978 15.05.2025 M)

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/C/RUS/RUS/2124134//P/20250515////VOLODIN/SEMEN/19780511/M///N/N////.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/C/RUS/RUS/2124134//P/20250515////VOLODIN/SEMEN/19780511/M///8501/B/10////////

$(set grp_id $(get_single_grp_id $(get point_dep) VOLODIN SEMEN))
$(set pax_id $(get_single_pax_id $(get point_dep) VOLODIN SEMEN))
$(set tid $(get_single_tid $(get point_dep) VOLODIN SEMEN))

# отмена регистрации члена экипажа (CICX уйдет только если был ответ на CIRS)

!!
$(CANCEL_CHECKIN_CREW $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ VOLODIN SEMEN ВЗ)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/10/C/RUS/RUS/2124134//P/20250515////VOLODIN/SEMEN/19780511/M///N/N/.*


# закрытие рейса. По настройке APPS должен уйти CIMR

!! err=ignore
$(WRITE_DESTS $(get point_dep) $(get point_arv) $(get move_id) ЮТ 298 СОЧ ПРХ
              $(date_format %d.%m.%Y) 10:15:00
              $(date_format %d.%m.%Y) 11:00:00)

>> lines=auto mode=regex
.*CIMR:([0-9]+)/UTUTA1/21/INM/3/UT298/AER/$(yyyymmdd)/MRQ/3/CZ/C/C/.*

%%
#########################################################################################

###
#   Тест №2
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#            версия apps: 21
#
#   APPS-запросы на посадку пассажиров уходят в момент прихода PNL.
#   При регистрации запросы уже не уходят
#
###

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps ЮТ ЦЗ APPS_21 closeout=false inbound=true outbound=false)

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set move_id $(get_move_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(run_trip_task send_apps $(get point_dep))

# уходят CIRQ-запросы
$(CIRQ_61_UT_REQS_APPS_VERSION_21 UT 298 AER PRG)
# Эмулируем приход нескольких apps-ответов:

# по пассажиру KURGINSKAYA/ANNA GRIGOREVNA

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id13)/PRS/27/001/CZ/P/RU/RU/0319189298//P//20201008////KURGINSKAYA/ANNA GRIGOREVNA/19870602/F//8501/B/1////////


# по пассажиру SELIVANOV RUSLAN

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id4)/PRS/27/001/CZ/P/RU/RU/9205589611//P//20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M//8501/B/2////////


# по пассажиру TUMALI VALERII

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id16)/PRS/27/001/CZ/P/UA/UA/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/3////////


# по пассажиру ALIMOV TALGAT

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id2)/PRS/27/001/CZ/P/KZ/KZ/N11024936//P//20261004////ALIMOV/TALGAT/19960511/M//8501/B/4////////

# по пассажиру KHASSENOVA ZULFIYA

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id3)/PRS/27/001/CZ/P/KZ/KZ/N07298275//P//20210329////KHASSENOVA/ZULFIYA/19741106/F//8501/B/5////////

# по пассажиру FUKS LIUDMILA

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id38)/PRS/27/001/CZ/////////////////8502/D////////


$(set pax_id_TUMALI $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 $(date_format %d.%m.%Y +1y) M)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/3/P/UKR/UKR/FA144642//P/$(yyyymmdd +1y)////TUMALI/VALERII/19680416/M///N/N/.*
>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/UKR/UKR/FA144642//P/$(yyyymmdd +1y)////TUMALI/VALERII/19680416/M///N/N////.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UA/UA/FA144642//P//20250625////TUMALI/VALERII/19680416/U//8509/X/////////


# регистрация пассажира SELIVANOV RUSLAN с измененным документом
# apps должен пойти

$(set pax_id_SELIVANOV $(get_pax_id $(get point_dep) SELIVANOV "RUSLAN NAILYEVICH MR"))

!!
$(CHECKIN_PAX $(get pax_id_SELIVANOV) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ SELIVANOV "RUSLAN NAILYEVICH" 2985085963078 ВЗ UA 12342131 UA 23.09.1983 $(date_format %d.%m.%Y +1y) M)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/2/P/RUS/RUS/9205589611//P/$(yyyymmdd +1y)////SELIVANOV/RUSLAN NAILYEVICH/19830923/M///N/N/.*
>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/UKR/UKR/12342131//P/$(yyyymmdd +1y)////SELIVANOV/RUSLAN NAILYEVICH/19830923/M///N/N////.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/RU/RU/9205589611//P//20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M//8502/D/////////


!! capture=on
$(GET_EVENTS $(get point_dep))

>> mode=regex
.*<msg>Запрос на посадкудля пассажира TUMALI. Результатдля страны ЦЗ: Посадка запрещена. Перезапись невозможна.</msg>.*
.*<msg>Запрос на посадкудля пассажира SELIVANOV. Результатдля страны ЦЗ: Посадка запрещена.</msg>.*

# приходит ADL с удалением двух пассажиров
$(INB_ADL_UT_DEL2PAXES AER PRG 298 $(ddmon +0 en))

$(run_trip_task send_apps $(get point_dep))

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/4/P/KAZ/KAZ/N11024936//P/$(yyyymmdd +1y)////ALIMOV/TALGAT/19960511/M///N/N/.*

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/5/P/KAZ/KAZ/N07298275//P/$(yyyymmdd +1y)////KHASSENOVA/ZULFIYA/19741106/F///N/N/.*

# приходит ADL с изменением данных по одному пассажиру
$(INB_ADL_UT_CHG1PAX AER PRG 298 $(ddmon +0 en))

$(run_trip_task send_apps $(get point_dep))


# уходит пара CICX/CIRQ, но ТОЛЬКО при условии получения ответа
# ранее по этому пассажиру!!!

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/1/P/RUS/RUS/0319189298//P/$(yyyymmdd +1y)////KURGINSKAYA/ANNA GRIGOREVNA/19870602/F///N/N/.*

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/RUS/RUS/0319189297//P/$(yyyymmdd +1y)////KURGINSKAYA/ANNA GRIGOREVNA/19870602/F///N/N////.*


# закрытие рейса. По настройке APPS НЕ должен уходить CIMR

!! err=ignore
$(WRITE_DESTS $(get point_dep) $(get point_arv) $(get move_id) ЮТ 298 СОЧ ПРХ
              $(date_format %d.%m.%Y) 10:15:00
              $(date_format %d.%m.%Y) 11:00:00)


%%
#########################################################################################

###
#   Тест №3
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#            версия apps: 21
#
#   APPS-запросы на посадку пассажиров уходят при привязке рейса
#
###

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps ЮТ ЦЗ APPS_21 inbound=true outbound=false)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(auto_set_craft $(get point_dep))

$(run_trip_task send_all_apps $(get point_dep))

# уходят CIRQ-запросы
$(CIRQ_61_UT_REQS_APPS_VERSION_21 UT 298 AER PRG)


%%
#########################################################################################
###
#   Тест №4
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#            версия apps: 21
#
#   не происходит APPS-запрос на посадку ОДНОГО пассажира  во время регистрации
#   не происходит APPS-запрос на отмену ОДНОГО пассажира  при отмене регистрации
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps ЮТ ЦЗ APPS_21 closeout=true inbound=false outbound=false)

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set move_id $(get_move_id $(get point_dep)))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))
$(set tid $(get_single_tid $(get point_dep) TUMALI VALERII))

!!
$(CANCEL_PAX $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ)

# регистрация члена экипажа

$(CHECKIN_CREW $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ VOLODIN SEMEN ВЗ RUS 2124134 RUS 11.05.1978 15.05.2025 M)

$(set grp_id $(get_single_grp_id $(get point_dep) VOLODIN SEMEN))
$(set pax_id $(get_single_pax_id $(get point_dep) VOLODIN SEMEN))
$(set tid $(get_single_tid $(get point_dep) VOLODIN SEMEN))

# отмена регистрации члена экипажа (CICX уйдет только если был ответ на CIRS)

!!
$(CANCEL_CHECKIN_CREW $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ VOLODIN SEMEN ВЗ)

# закрытие рейса. По настройке APPS должен уйти CIMR

!! err=ignore
$(WRITE_DESTS $(get point_dep) $(get point_arv) $(get move_id) ЮТ 298 СОЧ ПРХ
              $(date_format %d.%m.%Y) 10:15:00
              $(date_format %d.%m.%Y) 11:00:00)

%%
#########################################################################################

###
#   Тест №5
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#            версия apps: 21
#
#   не проходят APPS-запросы на посадку пассажиров в момент прихода PNL.
#   При регистрации запросы уже не уходят
#
###

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps ЮТ ЦЗ APPS_21 closeout=false inbound=false outbound=false)

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set move_id $(get_move_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

??
$(check_flight_tasks $(get point_dep))
>>
EMD_REFRESH
SYNC_ALL_CHKD
$()

$(run_trip_task send_apps $(get point_dep) "uncheck")

# уходят CIRQ-запросы
# $(CIRQ_61_UT_REQS_APPS_VERSION_21 UT 298 AER PRG)


# Эмулируем приход нескольких apps-ответов:

# по пассажиру KURGINSKAYA/ANNA GRIGOREVNA

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(get msg_id13)/PRS/27/001/CZ/P/RU/RU/0319189298//P//20201008////KURGINSKAYA/ANNA GRIGOREVNA/19870602/F//8501/B/1////////


# по пассажиру SELIVANOV RUSLAN

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(get msg_id4)/PRS/27/001/CZ/P/RU/RU/9205589611//P//20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M//8501/B/2////////


# по пассажиру TUMALI VALERII

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(get msg_id16)/PRS/27/001/CZ/P/UA/UA/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/3////////


# по пассажиру ALIMOV TALGAT

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(get msg_id2)/PRS/27/001/CZ/P/KZ/KZ/N11024936//P//20261004////ALIMOV/TALGAT/19960511/M//8501/B/4////////


# по пассажиру FUKS LIUDMILA

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(get msg_id38)/PRS/27/001/CZ/////////////////8502/D////////


# регистрация пассажира TUMALI VALERII с документом как в PNL
# apps не должен по идее пойти, но УХОДИТ

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

# >> lines=auto mode=regex
# .*CICX:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/110000/PCX/20/1/3/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/.*
# >> lines=auto mode=regex
# .*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/110000/PRQ/22/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N////.*

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(capture 1)/PRS/27/001/CZ/P/UA/UA/FA144642//P//20250625////TUMALI/VALERII/19680416/U//8509/X/////////


# регистрация пассажира SELIVANOV RUSLAN с измененным документом
# apps должен пойти

$(set pax_id $(get_pax_id $(get point_dep) SELIVANOV "RUSLAN NAILYEVICH MR"))

!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ SELIVANOV "RUSLAN NAILYEVICH" 2985085963078 ВЗ UA 12342131 UA 23.09.1983 20.12.2025 M)

# >> lines=auto mode=regex
# .*CICX:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/110000/PCX/20/1/2/P/RUS/RUS/9205589611//P/20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M///N/N/.*
# >> lines=auto mode=regex
# .*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/110000/PRQ/22/1/P/UKR/UKR/12342131//P/20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M///N/N////.*

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(capture 1)/PRS/27/001/CZ/P/RU/RU/9205589611//P//20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M//8502/D/////////


# !! capture=on
# $(GET_EVENTS $(get point_dep))

# >> mode=regex
# .*<msg>Запрос на посадкудля пассажира TUMALI. Результатдля страны ЦЗ: Посадка запрещена. Перезапись невозможна.</msg>.*
# .*<msg>Запрос на посадкудля пассажира SELIVANOV. Результатдля страны ЦЗ: Посадка запрещена.</msg>.*


# приходит ADL с удалением двух пассажиров
$(INB_ADL_UT_DEL2PAXES AER PRG 298 $(ddmon +0 en))

??
$(check_flight_tasks $(get point_dep))
>>
EMD_REFRESH
SYNC_ALL_CHKD
CHECK_ALARM
$()


# должна пойти отмена, но не уходит - ОШИБКА ??
$(run_trip_task send_apps $(get point_dep) "uncheck")


# приходит ADL с изменением данных по одному пассажиру
$(INB_ADL_UT_CHG1PAX AER PRG 298 $(ddmon +0 en))

??
$(check_flight_tasks $(get point_dep))
>>
EMD_REFRESH
SYNC_ALL_CHKD
CHECK_ALARM
$()

$(run_trip_task send_apps $(get point_dep) "uncheck")

# уходит пара CICX/CIRQ, но ТОЛЬКО при условии получения ответа
# ранее по этому пассажиру!!!

# >> lines=auto mode=regex
# .*CICX:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/110000/PCX/20/1/1/P/RUS/RUS/0319189298//P/20201008////KURGINSKAYA/ANNA GRIGOREVNA/19870602/F///N/N/.*

# >> lines=auto mode=regex
# .*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/110000/PRQ/22/1/P/RUS/RUS/0319189297//P/20201008////KURGINSKAYA/ANNA GRIGOREVNA/19870602/F///N/N////.*


# закрытие рейса. По настройке APPS НЕ должен уходить CIMR

!! err=ignore
$(WRITE_DESTS $(get point_dep) $(get point_arv) $(get move_id) ЮТ 298 СОЧ ПРХ
              $(date_format %d.%m.%Y) 10:15:00
              $(date_format %d.%m.%Y) 11:00:00)


%%
#########################################################################################

###
#   Тест №6
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#            версия apps: 21
#
#   не уходят APPS-запросы на посадку пассажиров при привязке рейса
#
###

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps ЮТ ЦЗ APPS_21 inbound=false outbound=false)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(auto_set_craft $(get point_dep))

$(run_trip_task send_all_apps $(get point_dep) "uncheck")

# уходят CIRQ-запросы
# $(CIRQ_61_UT_REQS_APPS_VERSION_21 UT 298 AER PRG)

%%
#########################################################################################
###
#   Тест №7
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#            версия apps: 21
#
#   Ответ CIRS возвращает ошибку. Удаляется apps запись из APPS_PAX_DATA. Создается Alarm APPSCONFlICT
#   При запрос CICX APPS не с pax_id нет, флаг is_cancel=true. Сообщение не формируется(Это первый кейс для функции typeOfAction)
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps ЮТ ЦЗ APPS_21 closeout=true inbound=true outbound=false)

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set move_id $(get_move_id $(get point_dep)))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

#################################  1 case
###### Проверим удалются ли Apps Alarms при коде CICX и при (несуществующем паксе)

$(set pax_id_MYPASSENGER $(get_pax_id $(get point_dep) TUMALI VALERII))
!!
$(CHECKIN_PAX $(get pax_id_MYPASSENGER) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
          P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/ERR/3/CZ/6999/AP ERROR: PL-SQL FAILED/

# Проверяем есть ли в базе Алармы для пассажира. Должен быть APPS_CONFLICT
??
$(check_pax_alarms $(get pax_id_MYPASSENGER))
>>
APPS_CONFLICT
$()

!! capture=on
$(GET_EVENTS $(get point_dep))

>> lines=auto
        <msg>Запрос на посадкудля пассажира TUMALI. Результатдля страны ЦЗ: Ошибка AP: не удалось выполнить PL-SQL запрос</msg>

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))
$(set tid $(get_single_tid $(get point_dep) TUMALI VALERII))

!!
$(CANCEL_PAX $(get pax_id_MYPASSENGER) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ)

# Не пойдет CICX APPS
# >> lines=auto mode=regex
# .*CICX:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/.*

# Проверяем есть ли в базе алармы для пассжира. Не должно быть ничего после удаления
??
$(check_pax_alarms $(get pax_id_MYPASSENGER))
>>
$()

#################################
#################################

%%
#########################################################################################
###
#   Тест №8
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#            версия apps: 21
#
#   Ответ CIRS возвращает статус D. Создается Alarm APPS_NEGATIVE_DIRECTIVE
#   При запрос CANCEL_PAX is_cancel=true, is_exists=true. APPS Сообщение не формируется(Это второй кейс для функции typeOfAction)
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps ЮТ ЦЗ APPS_21 closeout=true inbound=true outbound=false)

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set move_id $(get_move_id $(get point_dep)))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

#################################  2 case
###### Проверим удалются ли Apps Alarms при существующем паксе при коде CICX и непустом статусе)

$(set pax_id_MYPASSENGER $(get_pax_id $(get point_dep) TUMALI VALERII))
!!
$(CHECKIN_PAX $(get pax_id_MYPASSENGER) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
          P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8502/D/1/////////

# Проверяем есть ли в базе Алармы для пассажира. Статус D(P) значит выставится аларм APPS_NEGATIVE_DIRECTIVE
??
$(check_pax_alarms $(get pax_id_MYPASSENGER))
>>
APPS_NEGATIVE_DIRECTIVE
$()

!! capture=on
$(GET_EVENTS $(get point_dep))

>> lines=auto
        <msg>Запрос на посадкудля пассажира TUMALI. Результатдля страны ЦЗ: Посадка запрещена.</msg>

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))
$(set tid $(get_single_tid $(get point_dep) TUMALI VALERII))

!!
$(CANCEL_PAX $(get pax_id_MYPASSENGER) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ)

# Не пойдет CICX APPS
#>> lines=auto mode=regex
#.*CICX:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/.*

# Проверяем есть ли в базе алармы для пассжира. Не должно быть ничего после удаления во втором case
??
$(check_pax_alarms $(get pax_id_MYPASSENGER))
>>
$()

#################################
#################################

%%
#########################################################################################
###
#   Тест №9
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#            версия apps: 21
#
#   Ответ CIRS нормально возвращает статус B. Удаляются алармы
#   При запрос CICX с pax_id нет, флаг is_cancel=true, статус равен B. Сообщение формируется(если статус B)
#   Это второй кейс для функции typeOfAction
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps ЮТ ЦЗ APPS_21 closeout=true inbound=true outbound=false)

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set move_id $(get_move_id $(get point_dep)))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

#################################  2 case
###### Проверим удалются ли Apps Alarms при существующем паксе при коде CICX и непустом статусе)

$(set pax_id_MYPASSENGER $(get_pax_id $(get point_dep) TUMALI VALERII))
!!
$(CHECKIN_PAX $(get pax_id_MYPASSENGER) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
          P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/1/////////

# Проверяем есть ли в базе Алармы для пассажира. Не должно быть
??
$(check_pax_alarms $(get pax_id_MYPASSENGER))
>>
$()

!! capture=on
$(GET_EVENTS $(get point_dep))

>> lines=auto
        <msg>Запрос на посадкудля пассажира TUMALI. Результатдля страны ЦЗ: Посадка разрешена.</msg>

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))
$(set tid $(get_single_tid $(get point_dep) TUMALI VALERII))

!!
$(CANCEL_PAX $(get pax_id_MYPASSENGER) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/.*

# Проверяем есть ли в базе алармы для пассжира. Не должно быть ничего после удаления во втором case
??
$(check_pax_alarms $(get pax_id_MYPASSENGER))
>>
$()

#################################

%%
#########################################################################################
###
#   Тест №10
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#            версия apps: 21
#
#   После PNL CIRQ получаем два ответа со статусами B и P
#   В случае B - Update(CICX,CIRQ) в случае P(U,E,T..)- New(CIRQ)
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps ЮТ ЦЗ APPS_21 closeout=true inbound=true outbound=false)

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))
$(set move_id $(get_move_id $(get point_dep)))

$(run_trip_task send_apps $(get point_dep))

$(CIRQ_61_UT_REQS_APPS_VERSION_21 UT 298 AER PRG)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id16)/PRS/27/001/CZ/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/1/////////

$(set pax_id_MYPASSENGER $(get_pax_id $(get point_dep) TUMALI VALERII))

# Меняем паспорт чтобы is_the_same был false.Хотя в этом случае он итак false, потому что поле pre_checkin после PNL
!!
$(CHECKIN_PAX $(get pax_id_MYPASSENGER) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ UA FA144500 UA 16.04.1968 25.06.2025 M)

#отмена идет с флагом precheckin P потому что предыдущий CIRQ был отправлен при обработке PNL
$(CICX_21 P UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
          1 P UKR UKR FA144642 P $(yyyymmdd +1y) TUMALI VALERII 19680416 M N N)
$(set cicx_msg_id $(capture 1))

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
          P UKR UKR FA144500 P 20250625 TUMALI VALERII 19680416 M N N)
$(set cirq_msg_id $(capture 1))

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CICC:$(get cicx_msg_id)/PCC/26/001/CZ/P/UKR/UKR/FA144642//P//$(yyyymmdd +1y)////TUMALI/VALERII/19680416/M//8505/C///////

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get cirq_msg_id)/PRS/27/001/CZ/P/UKR/UKR/FA144500//P//$(yyyymmdd +1y)////TUMALI/VALERII/19680416/M//8507/U/1/////////

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))
$(set pax_id $(get_single_pax_id $(get point_dep) TUMALI VALERII))
$(set tid    $(get_single_tid    $(get point_dep) TUMALI VALERII))

!!
$(UPDATE_PAX_PASSPORT $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ UA FA144777 UA 16.04.1968 25.06.2025 M)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
          P UKR UKR FA144777 P 20250625 TUMALI VALERII 19680416 M N N)


%%
#########################################################################################
###
#   Тест №11
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#            версия apps: 21
#
#   После PNL CIRQ получаем два ответа с пустыми статусами
#   Это 7 кейс для функции typeOfAction проверка outOfSync
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps ЮТ ЦЗ APPS_21 closeout=true inbound=true outbound=false)

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))
$(set move_id $(get_move_id $(get point_dep)))

$(run_trip_task send_apps $(get point_dep))

$(CIRQ_61_UT_REQS_APPS_VERSION_21 UT 298 AER PRG)

#<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
#CIRS:$(get msg_id16)/PRS/27/001/CZ/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M////1/////////

$(set pax_id_MYPASSENGER $(get_pax_id $(get point_dep) TUMALI VALERII))

# Меняем паспорт чтобы is_the_same был false.Хотя в этом случае он итак false, потому что поле pre_checkin после PNL
!!
$(CHECKIN_PAX $(get pax_id_MYPASSENGER) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ UA FA144500 UA 16.04.1968 25.06.2025 M)

#Проверяем есть ли в базе алармы для пассажира. Не должно быть ничего после удаления во втором case
??
$(check_pax_alarms $(get pax_id_MYPASSENGER))
>>
APPS_CONFLICT
$()

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(get msg_id16)/PRS/27/001/CZ/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/1/////////

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))
$(set pax_id $(get_single_pax_id $(get point_dep) TUMALI VALERII))
$(set tid    $(get_single_tid    $(get point_dep) TUMALI VALERII))

!!
$(CANCEL_PAX $(get pax_id_MYPASSENGER) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ)

## Проверяем есть ли в базе алармы для пассжира. Не должно быть ничего после удаления во втором case
??
$(check_pax_alarms $(get pax_id_MYPASSENGER))
>>
$()

%%
#########################################################################################

###
#   Тест №12
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#            версия apps: 21
#
#   При приходе ADL CICX посылаем только в случае , если пассажир не зарегестрирован
#
###

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps ЮТ ЦЗ APPS_21 closeout=false inbound=true outbound=false)

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set move_id $(get_move_id $(get point_dep)))

$(run_trip_task send_apps $(get point_dep))

$(CIRQ_61_UT_REQS_APPS_VERSION_21 UT 298 AER PRG)

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set pax_id_ALIMOV $(get_pax_id $(get point_dep) ALIMOV TALGAT))


<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id2)/PRS/27/001/CZ/P/KZ/KZ/N11024936//P//20261004////ALIMOV/TALGAT/19960511/M//8501/B/1////////

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id3)/PRS/27/001/CZ/P/KZ/KZ/N07298275//P//20210329////KHASSENOVA/ZULFIYA/19741106/F//8501/B/5////////

# Регистрируем одного из пассажиров

!!
$(CHECKIN_PAX $(get pax_id_ALIMOV) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ ALIMOV TALGAT 2982425696898 ВЗ KZ N11024936 KZ 11.05.1996 04.10.2026 M)

$(CICX_21 P UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
        1 P KAZ KAZ N11024936 P $(yyyymmdd +1y) ALIMOV TALGAT 19960511 M N N)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000 P KAZ KAZ N11024936
        P 20261004 ALIMOV TALGAT 19960511 M N N)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CICC:$(get cicx_msg_id)/PCC/26/001/CZ/P/KAZ/KAZ/N11024936//P//20261004////ALIMOV/TALGAT/19960511/M//8505/C///////

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get cirq_msg_id)/PRS/27/001/CZ/P/KZ/KZ/N11024936//P//20261004////ALIMOV/TALGAT/19960511/M//8501/B/88////////


# приходит ADL с удалением двух пассажиров , один из которых зарегестрирован(ALIMOV), а другой нет
# CICX пойдет только для незарегестрированного пассажира
$(INB_ADL_UT_DEL2PAXES AER PRG 298 $(ddmon +0 en))

$(run_trip_task send_apps $(get point_dep))

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/5/P/KAZ/KAZ/N07298275//P/$(yyyymmdd +1y)////KHASSENOVA/ZULFIYA/19741106/F///N/N/.*


%%
#########################################################################################

###
#   Тест №13
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#            версия apps: 21
#
#   В реальной жизни на APPS-запросы с признаком pre-checkin
#   (т.е. запросы, посылаемые при обработке PNL/ADL)
#   SITA APPS всегда отвечает ошибкой
#
###

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps ЮТ ЦЗ APPS_21 closeout=false inbound=true outbound=false)

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set move_id $(get_move_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(run_trip_task send_apps $(get point_dep))

# уходят CIRQ-запросы
$(CIRQ_61_UT_REQS_APPS_VERSION_21 UT 298 AER PRG)

# Эмулируем ответ по пассажиру TUMALI VALERII

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id16)/ERR/3//5569/Participating countries cannot process these transactions/

# проверяем что после ошибки пассажир удаляется

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

# CICX не отправляется
#>> lines=auto mode=regex
#.*CICX:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/3/P/UKR/UKR/FA144642//P/$(yyyymmdd +1y)////TUMALI/VALERII/19680416/M///N/N/.*

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N////.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UA/UA/FA144642//P//20250625////TUMALI/VALERII/19680416/U//8509/X/////////



%%
##########################################################################################################
###
#   Тест №14
#
#   Описание: пассажиров: 3,
#             интерактив: выкл
#            версия apps: 21
#
#   APPS-запросы на посадку пассажиров уходят в момент прихода PNL c трансферным пассажиром,
#   который летит после рейса PNL(точка O) и пока не посылается для пассжира с точкой(I)
#   При регистрации запросы уже не уходят
#
###

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps ЮТ ЦЗ APPS_21 closeout=false inbound=true outbound=true)
$(init_apps ЮТ НЛ APPS_21 closeout=false inbound=true outbound=true)
$(init_apps ЮТ ГБ APPS_21 closeout=false inbound=true outbound=true)

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(PREPARE_SEASON_SCD ЮТ ПРХ АМС 190)
$(PREPARE_SEASON_SCD ЮТ АМС ЛХР 450)

$(make_spp)

$(INB_PNL_UT_TRANSFER3 AMS LHR 450 $(ddmon +0 en))
$(INB_PNL_UT_TRANSFER2 PRG AMS 190 $(ddmon +0 en))
$(INB_PNL_UT_TRANSFER1 AER PRG 298 $(ddmon +0 en))


$(set point_dep_UT_298 $(last_point_id_spp 0))
$(set point_dep_UT_190 $(last_point_id_spp 1))
$(set point_dep_UT_450 $(last_point_id_spp 2))

$(deny_ets_interactive ЮТ 298 СОЧ)
$(deny_ets_interactive ЮТ 190 ПРХ)
$(deny_ets_interactive ЮТ 450 АМС)


# $(dump_table CRS_PAX fields="pax_id, surname, name, pnr_id, pr_del ")
# $(dump_table CRS_PNR fields="pnr_id, airp_arv, system, point_id ")
# $(dump_table TLG_BINDING)
# $(dump_table POINTS fields="point_id, airline, flt_no, airp, scd_out, suffix")
# $(dump_table APPS_PAX_DATA fields="pax_id, apps_pax_id, given_names, cirq_msg_id, cicx_msg_id, version, point_id")

$(run_trip_task send_apps $(get point_dep_UT_298))

# уходят CIRQ-запросы
# $(CIRQ_3_UT_REQS_APPS_VERSION_21_TRANSFER UT 298 AER PRG)

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/KAZ/KAZ/N11024936//P/20261004////ALIMOV/TALGAT/19960511/M///N/N////.*

$(set msg_id2 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/RUS/RUS/0306355301//P/20291005////KOBYLINSKIY/ALEKSEY/19861231/M///N/N////.*

$(set msg_id10 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/////P/////OZ/OFER//U///N/Y////.*

$(set msg_id11 $(capture 1))

#$(dump_table APPS_PAX_DATA fields="pax_id, apps_pax_id, cirq_msg_id, cicx_msg_id, family_name")

$(run_trip_task send_apps $(get point_dep_UT_190))
>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT190/PRG/AMS/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/////P/////OZ/OFER//U///N/Y////.*
>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT190/PRG/AMS/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/////P/////OZ/OFER//U///N/Y////.*

$(run_trip_task send_apps $(get point_dep_UT_450))
>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT450/AMS/LHR/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/0[0-9]?0000/PRQ/22/1/P/////P/////OZ/OFER//U///N/N////.*
>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT450/AMS/LHR/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/0[0-9]?0000/PRQ/22/1/P/////P/////OZ/OFER//U///N/N////.*

# >> lines=auto mode=regex
#.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/1[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/RUS/RUS/9205589611//P/20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M///N/Y////.*

$(set msg_id13 $(capture 1))

# Эмулируем приход нескольких apps-ответов:

# по пассажиру ALIMOV TALGAT

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(get msg_id2)/PRS/27/001/CZ/P/KZ/KZ/N11024936//P//20261004////ALIMOV/TALGAT/19960511/M//8501/B/4////////

# по пассажиру KOBYLINSKIY ALEKSEY

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(get msg_id10)/PRS/27/001/CZ/P/RUS/RUS/0306355301//P//20291005////KOBYLINSKIY/ALEKSEY/19861231/M//8501/B/5////////

# по пассажиру OZ OFER

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(get msg_id11)/PRS/27/001/CZ/////////////////8502/D////////



%%
########################################################################################
#   Тест №15
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#            версия apps: 21
#
#   APPS-запросы на посадку ОДНОГО трансферного пассажира уходит во время Сквозной регистрации
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps ЮТ ЦЗ APPS_21 closeout=false inbound=true outbound=true)
$(init_apps ЮТ НЛ APPS_21 closeout=false inbound=true outbound=true)
$(init_apps ЮТ ГБ APPS_21 closeout=false inbound=true outbound=true)

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(PREPARE_SEASON_SCD ЮТ ПРХ АМС 190)
$(PREPARE_SEASON_SCD ЮТ АМС ЛХР 450)

$(make_spp)

$(deny_ets_interactive ЮТ 298 СОЧ)
$(deny_ets_interactive ЮТ 190 ПРХ)
$(deny_ets_interactive ЮТ 450 АМС)

$(INB_PNL_UT_TRANSFER3 AMS LHR 450 $(ddmon +0 en))
$(INB_PNL_UT_TRANSFER2 PRG AMS 190 $(ddmon +0 en))
$(INB_PNL_UT_TRANSFER1 AER PRG 298 $(ddmon +0 en))

$(set point_dep_UT_298 $(last_point_id_spp 0))
$(set point_dep_UT_190 $(last_point_id_spp 1))
$(set point_dep_UT_450 $(last_point_id_spp 2))

$(set point_arv_UT_298 $(get_next_trip_point_id $(get point_dep_UT_298)))
$(set point_arv_UT_190 $(get_next_trip_point_id $(get point_dep_UT_190)))
$(set point_arv_UT_450 $(get_next_trip_point_id $(get point_dep_UT_450)))

# $(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep_UT_298))
$(auto_set_craft $(get point_dep_UT_190))
$(auto_set_craft $(get point_dep_UT_450))

# $(set move_id $(get_move_id $(get point_dep)))

$(set pax_id1 $(get_pax_id $(get point_dep_UT_298) OZ OFER))
$(set pax_id2 $(get_pax_id $(get point_dep_UT_190) OZ OFER))
$(set pax_id3 $(get_pax_id $(get point_dep_UT_450) OZ OFER))

!!
$(CHECKIN_PAX_TRANSFER $(get pax_id1) $(get point_dep_UT_298) $(get point_arv_UT_298) ЮТ 298 СОЧ ПРХ OZ OFER 2985523437721
                       $(get pax_id2) $(get point_dep_UT_190) $(get point_arv_UT_190) ЮТ 190 ПРХ АМС OZ OFER
                       $(get pax_id3) $(get point_dep_UT_450) $(get point_arv_UT_450) ЮТ 450 АМС ЛХР OZ OFER
                       ВЗ UA 32427293 UA 16.04.1968 25.06.2025 M)


# отправка apps для рейса 298 в чехию , флаг трансфера Y в DEST.
$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
P UKR UKR 32427293 P 20250625 OZ OFER 19680416 M N Y)

# отправка apps для рейса 190 в чехию , флаг трансфера Y в DEST и ORIG.
$(CIRQ_21 "" UT 190 PRG AMS $(yyyymmdd) 0[0-9]?1500 $(yyyymmdd) 1[0-9]?0000
P UKR UKR 32427293 P 20250625 OZ OFER 19680416 M Y Y "CHK/2/AER/UT298/")

# отправка apps для рейса 190 в голландию , флаг трансфера Y в DEST и ORIG.
$(CIRQ_21 "" UT 190 PRG AMS $(yyyymmdd) 0[0-9]?1500 $(yyyymmdd) 1[0-9]?0000
P UKR UKR 32427293 P 20250625 OZ OFER 19680416 M Y Y "CHK/2/AER/UT298/")

# отправка apps для рейса 450 в голландию , флаг трансфера Y ORIG.
$(CIRQ_21 "" UT 450 AMS LHR $(yyyymmdd) 0[0-9]?1500 $(yyyymmdd) 090000
P UKR UKR 32427293 P 20250625 OZ OFER 19680416 M Y N "CHK/2/AER/UT298/")

# отправка apps для рейса 450 в англию , флаг трансфера Y в ORIG.
$(CIRQ_21 "" UT 450 AMS LHR $(yyyymmdd) 0[0-9]?1500 $(yyyymmdd) 090000
P UKR UKR 32427293 P 20250625 OZ OFER 19680416 M Y N "CHK/2/AER/UT298/")

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(capture 1)/PRS/27/001/CZ/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/1/////////

# !! capture=on
# $(GET_EVENTS $(get point_dep_UT_298))

# >> lines=auto
#         <msg>Запрос на посадкудля пассажира OZ. Результатдля страны ЦЗ: Посадка разрешена.</msg>

# $(set grp_id1 $(get_single_grp_id $(get point_dep_UT_298) OZ OFER))
# $(set tid1 $(get_single_tid $(get point_dep_UT_298) OZ OFER))


# !!
# $(CANCEL_PAX $(get pax_id1) $(get grp_id1) $(get tid1) $(get point_dep_UT_298) $(get point_arv_UT_298) ЮТ 298 СОЧ ПРХ OZ OFER 2986145115578 ВЗ)


# >> lines=auto mode=regex
# .*CICX:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/1[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/.*



%%
#########################################################################################

###
#   Тест №16
#
#   Описание: пассажиров: 61,
#             интерактив: вкл(Идут дополнительно запросы в сэб)
#            версия apps: 21
#
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_eds ЮТ UTET UTDC)

$(init_apps ЮТ ЦЗ APPS_21 closeout=true)

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(make_spp)


$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(ET_DISP_61_UT_REQS)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

# $(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set move_id $(get_move_id $(get point_dep)))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

$(SAVE_ET_DISP $(get point_dep) 2986145115578 TUMALI VALERII ЮТ UTDC UTET G4LK6W AER PRG)

!! err=ignore
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

>>
$(TKCREQ_ET_COS UTDC UTET $(last_edifact_ref) ЮТ 2986145115578 1 CK xxxxxx СОЧ ПРХ 298)
<<
$(TKCRES_ET_COS UTET UTDC $(last_edifact_ref) 2986145115578 1 CK)


>> lines=auto
    <kick req_ctxt_id...

!!
$(lastRedisplay)


>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N////.*


<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/1////////

!! capture=on
$(GET_EVENTS $(get point_dep))

>> lines=auto
        <msg>Запрос на посадкудля пассажира TUMALI. Результатдля страны ЦЗ: Посадка разрешена.</msg>

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))
$(set tid $(get_single_tid $(get point_dep) TUMALI VALERII))

!! err=ignore
$(CANCEL_PAX $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ)

>>
$(TKCREQ_ET_COS UTDC UTET $(last_edifact_ref) ЮТ 2986145115578 1 I xxxxxx СОЧ ПРХ 298)
<<
$(TKCRES_ET_COS UTET UTDC $(last_edifact_ref) 2986145115578 1 I)


>> lines=auto
    <kick req_ctxt_id...

!!
$(lastRedisplay)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/.*


%%
########################################################################################
###
#   Тест №17
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#            версия apps: 21
#
#   Тест на отправление apps манифест для закрытия рейса
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps ЮТ ЦЗ APPS_21 closeout=true inbound=true outbound=false )

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

# Не происходит одновременная регистрация вместе с посадкой
# $(combine_brd_with_reg $(get point_dep))

$(auto_set_craft $(get point_dep))

$(set move_id $(get_move_id $(get point_dep)))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))
$(set pax_id_ALIMOV $(get_pax_id $(get point_dep) ALIMOV TALGAT))
$(set pax_id_ANNA $(get_pax_id $(get point_dep) KURGINSKAYA "ANNA GRIGOREVNA"))
$(set pax_id_SELIVANOV $(get_pax_id $(get point_dep) SELIVANOV "RUSLAN NAILYEVICH MR"))

##----------------------------------------------------------------------------------
# Первый пассажир зареган, поэтому по нему при отмене рейса отправится CICX
!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/1/////////

##----------------------------------------------------------------------------------
# Второй пассажир не зареган, потому что в ответе CIRS статус D и код ошибки 8502
# Поэтому по нему не нужно слать отмену
!!
$(CHECKIN_PAX $(get pax_id_ALIMOV) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ ALIMOV TALGAT 2982425696898 ВЗ KZ N11024936 KZ 11.05.1996 04.10.2026 M)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
P KAZ KAZ N11024936 P 20261004 ALIMOV TALGAT 19960511 M N N)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/KAZ/KAZ/N11024936//P//20261004////ALIMOV/TALGAT/19960511/M//8502/D/2/////////

##----------------------------------------------------------------------------------
# Третий пассажир зареган и по нему отправляется отмена, отработает
# поэтому не надо слать отмену

!!
$(CHECKIN_PAX $(get pax_id_ANNA) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ KURGINSKAYA "ANNA GRIGOREVNA" 2982425690987 ВЗ UA 12342131 UA 23.09.1983 20.12.2025 M)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
P UKR UKR 12342131 P 20251220 KURGINSKAYA "ANNA GRIGOREVNA" 19830923 M N N)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/RU/RU/0319189298//P//20201008////KURGINSKAYA/ANNA GRIGOREVNA/19830923/F//8501/B/3////////

$(set grp_id $(get_single_grp_id $(get point_dep) KURGINSKAYA "ANNA GRIGOREVNA"))
$(set tid $(get_single_tid $(get point_dep) KURGINSKAYA "ANNA GRIGOREVNA"))

!!
$(CANCEL_PAX $(get pax_id_ANNA) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv)
    ЮТ 298 СОЧ ПРХ KURGINSKAYA "ANNA GRIGOREVNA" 2982425690987 ВЗ)

$(CICX_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
       3 P UKR UKR 12342131 P 20251220 KURGINSKAYA "ANNA GRIGOREVNA" 19830923 M N N)


<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CICC:$(capture 1)/PCC/26/001/CZ/P/UKR/UKR/12342131//P//20251220////KURGINSKAYA/ANNA GRIGOREVNA/19830923/M//8506/D///////

#-----------------------------------------------------------------------------------------

# 4 Пассажир зареган по нему пришел ответ, потом апдейтим его же, по нему приходит ответ 2 раз
# В таблице APPS_PAX_DATA должно быть две записи по 1му пассажиру
# По ним должна отправиться отмена?

!!
$(CHECKIN_PAX $(get pax_id_SELIVANOV) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ SELIVANOV "RUSLAN NAILYEVICH" 2985085963078 ВЗ RU 9205589611 RU 23.09.1983 20.12.2025 M)

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/RUS/RUS/9205589611//P/20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M///N/N////.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/RU/RU/9205589611//P//20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M//8501/B/3////////

$(set grp_id $(get_single_grp_id $(get point_dep) SELIVANOV "RUSLAN NAILYEVICH"))
$(set tid    $(get_single_tid    $(get point_dep) SELIVANOV "RUSLAN NAILYEVICH"))


# изменили документ
!!
$(UPDATE_PAX_PASSPORT $(get pax_id_SELIVANOV) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ SELIVANOV "RUSLAN NAILYEVICH" 2985085963078 ВЗ UA FA144777 UA 16.04.1968 25.06.2025 M)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/3/P/RUS/RUS/9205589611//P/20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M///N/N/.*
>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/UKR/UKR/FA144777//P/20250625////SELIVANOV/RUSLAN NAILYEVICH/19680416/M///N/N////.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UKR/UKR/FA144777//P//20250625////SELIVANOV/RUSLAN NAILYEVICH/19680416/M//8501/B/4////////

#---------------------------------------------------------------------------------------

!! capture=on
$(GET_EVENTS $(get point_dep))

>> lines=auto
        <msg>Запрос на посадкудля пассажира TUMALI. Результатдля страны ЦЗ: Посадка разрешена.</msg>

# закрытие рейса. По настройке APPS должен уйти CIMR

!! err=ignore
$(WRITE_DESTS $(get point_dep) $(get point_arv) $(get move_id) ЮТ 298 СОЧ ПРХ
              $(date_format %d.%m.%Y) 10:15:00
              $(date_format %d.%m.%Y) 11:00:00)

# Отправляется отмена непосаженных пассажира
$(CICX_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
       1 P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N)

$(CICX_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
        4 P UKR UKR FA144777 P 20250625 SELIVANOV "RUSLAN NAILYEVICH" 19680416 M N N)

>> lines=auto mode=regex
.*CIMR:([0-9]+)/UTUTA1/21/INM/3/UT298/AER/$(yyyymmdd)/MRQ/3/CZ/C/C/.*


%%
#########################################################################################

###
#   Тест №18
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#            версия apps: 21
#
#   APPS-запросы не уходят потому что pre_checkin флаг выставлен в false
#
###

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps ЮТ ЦЗ APPS_21 closeout=false inbound=true outbound=false pre_checkin=false)

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set move_id $(get_move_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(run_trip_task send_apps $(get point_dep))


%%
########################################################################
###
#   Тест №19
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#            версия apps: 21
#
#   APPS-запрос на посадку ОДНОГО пассажира Транзитный рейс
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps ЮТ ЦЗ APPS_21 closeout=true inbound=true outbound=true)
$(init_apps ЮТ НЛ APPS_21 closeout=false inbound=false outbound=false)

$(PREPARE_SEASON_SCD_TRANSIT ЮТ СОЧ АМС ПРХ 298) #сочи внуково прага
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get_next_trip_point_id $(get point_dep))))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set move_id $(get_move_id $(get point_dep)))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

#$(dump_table POINTS fields="point_id, airline, flt_no, airp, scd_out, suffix")

!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

### По сертификации SITA потребовала выставлять трансферный флаг DEST в транзитных рейсах

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 1[0-9]?1500 $(yyyymmdd) 1[0-9]?0000
P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N Y)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/1/////////

!! capture=on
$(GET_EVENTS $(get point_dep))

>> lines=auto
        <msg>Запрос на посадкудля пассажира TUMALI. Результатдля страны ЦЗ: Посадка разрешена.</msg>

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))
$(set tid $(get_single_tid $(get point_dep) TUMALI VALERII))

!!
$(CANCEL_PAX $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ)


$(CICX_21 "" UT 298 AER PRG $(yyyymmdd) 1[0-9]?1500 $(yyyymmdd) 1[0-9]?0000
          1 P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N Y)

# закрытие рейса. По настройке APPS должен уйти CIMR

!! err=ignore
$(WRITE_DESTS $(get point_dep) $(get point_arv) $(get move_id) ЮТ 298 СОЧ ПРХ
              $(date_format %d.%m.%Y) 10:15:00
              $(date_format %d.%m.%Y) 11:00:00)

>> lines=auto mode=regex
.*CIMR:([0-9]+)/UTUTA1/21/INM/3/UT298/AER/$(yyyymmdd)/MRQ/3/CZ/C/C/.*

%%
#########################################################################################
###
#   Тест №20
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#            версия apps: 21
#
#   Выставляется Alarm APPS_NOT_SCD_IN_TIME если на рейс не заведено время прилета
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps ЮТ ЦЗ APPS_21 closeout=true inbound=true outbound=true)

$(PREPARE_SEASON_SCD_WITHOUT_ARRIVE_TIME ЮТ СОЧ ПРХ 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set move_id $(get_move_id $(get point_dep)))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

$(run_trip_task send_apps $(get point_dep))
$(run_trip_task check_trip_alarms $(get point_dep))

# Проверяем есть ли в базе Алармы для пассажира. Должен быть APPS_NOT_SCD_IN_TIME
??
$(check_trip_alarms $(get point_dep))
>>
APPS_NOT_SCD_IN_TIME
$()

# Добавили дату, только для суточного плана полета
$(UPDATE_SPP_FLIGHT $(get point_dep) $(get point_arv) ЮТ СОЧ ПРХ 298 $(get move_id))

$(run_trip_task check_trip_alarms $(get point_dep))

# Проверяем есть ли в базе Алармы для пассажира. Не должен быть APPS_NOT_SCD_IN_TIME
??
$(check_trip_alarms $(get point_dep))
>>
$()

#################################

%%
#########################################################################################

###
#   Тест №21
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#            версия apps: 21
#
#   APPS-запросы не уходят потому что рейс не международный
#
###

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps ЮТ РФ APPS_21 closeout=false inbound=true outbound=true)

$(PREPARE_SEASON_SCD ЮТ СОЧ ВНК 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER VKO 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set move_id $(get_move_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(run_trip_task send_apps $(get point_dep) "uncheck")


%%
#########################################################################################
###
#   Тест №22
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#            версия apps: 21
#
#   Задача не должна выставляться, если  дата pnl не удовлетворяет условию ( now - data > 2)
#   Если дата пришедешего полета в будущем , то задача всегда выставляется
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

################################################################################
#Заводим настройки для стран обоих рейсов

$(init_apps ЮТ ЦЗ APPS_21 closeout=false inbound=true outbound=true)
$(init_apps ЮТ НЛ APPS_21 closeout=false inbound=true outbound=true)

#Проверка даты.
#Сначала завели рейс потом пришел PNL , SEND_NEW_APPS_INFO(tlg)

$(PREPARE_SEASON_SCD ЮТ АМС ПРХ 300 -1 TU5 $(date_format %d.%m.%Y -12) $(date_format %d.%m.%Y +12))
$(make_spp $(ddmmyy -3))
$(deny_ets_interactive ЮТ 300 АМС)

$(INB_PNL_UT AMS PRG 300 $(ddmon -3 en))
$(set point_dep $(last_point_id_spp))


# Проверяем есть ли в базе задачи для полета. Не должны быть
??
$(check_flight_tasks $(get point_dep))
>>
EMD_REFRESH
SYNC_ALL_CHKD
$()

$(INB_PNL_UT PRG AMS 100 $(ddmon -1 en))

$(PREPARE_SEASON_SCD ЮТ ПРХ АМС 100 -1 TU5 $(date_format %d.%m.%Y -12) $(date_format %d.%m.%Y +12))
$(make_spp $(ddmmyy -1))
$(deny_ets_interactive ЮТ 100 АМС)

$(set point_dep $(last_point_id_spp))

# Проверяем есть ли в базе задачи для полета.
??
$(check_flight_tasks $(get point_dep))
>>
SYNC_ALL_CHKD
SEND_ALL_APPS_INFO
EMD_REFRESH
$()


$(PREPARE_SEASON_SCD ЮТ ПРХ АМС 200 -1 TU5 $(date_format %d.%m.%Y -12) $(date_format %d.%m.%Y +12))
$(make_spp $(ddmmyy +11))
$(deny_ets_interactive ЮТ 200 АМС)

$(INB_PNL_UT PRG AMS 200 $(ddmon +11 en))

$(set point_dep $(last_point_id_spp))

# Проверяем есть ли в базе задачи для полета.
??
$(check_flight_tasks $(get point_dep))
>>
EMD_REFRESH
SYNC_ALL_CHKD
SEND_ALL_APPS_INFO
SEND_NEW_APPS_INFO
$()

%%
#########################################################################################
###
#   Тест №23
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#            версия apps: 21
#
#   Добавляется задача SEND_ALL_APPS_INFO, когда заведены настройки, и не добавляется, когда не заведены
#   Не добавляется задача SEND_NEW_APPS_INFO, когда не заведены APPS настройки
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

# Настроек нету
# $(init_apps ЮТ ЦЗ APPS_21 closeout=true inbound=true outbound=true)

#П ривязка рейса по ранее заведенному PNL, SEND_ALL_APPS_INFO(flt_binding)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))
$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(set point_dep $(last_point_id_spp))

# Проверяем есть ли в базе задачи для полета. НЕ должен быть SEND_ALL_APPS_INFO
??
$(check_flight_tasks $(get point_dep))
>>
SYNC_ALL_CHKD
EMD_REFRESH
$()

#Сначала завели рейс потом пришел PNL , SEND_NEW_APPS_INFO(tlg)
$(PREPARE_SEASON_SCD ЮТ ПРХ АМС 190)
$(make_spp)
$(deny_ets_interactive ЮТ 190 ПРХ)

$(INB_PNL_UT PRG AMS 190 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))

# Проверяем есть ли в базе задачи для полета. НЕ должен быть SEND_NEW_APPS_INFO
??
$(check_flight_tasks $(get point_dep))
>>
EMD_REFRESH
SYNC_ALL_CHKD
$()

# Заводим настройки
$(init_apps ЮТ ЦЗ APPS_21 closeout=false inbound=true outbound=true)
$(init_apps ЮТ НЛ APPS_21 closeout=false inbound=true outbound=true)

#Привязка рейса по ранее заведенному PNL, SEND_ALL_APPS_INFO(flt_binding)
$(INB_PNL_UT AER AMS 100 $(ddmon +0 en))
$(PREPARE_SEASON_SCD ЮТ СОЧ АМС 100)

$(make_spp)
$(deny_ets_interactive ЮТ 100 СОЧ)

$(set point_dep $(last_point_id_spp))

# Проверяем есть ли в базе задачи для полета. Должен быть SEND_ALL_APPS_INFO
??
$(check_flight_tasks $(get point_dep))
>> lines=auto
SEND_ALL_APPS_INFO


#Сначала завели рейс потом пришел PNL , SEND_NEW_APPS_INFO(tlg)
$(PREPARE_SEASON_SCD ЮТ АМС ПРХ 200)
$(make_spp)
$(deny_ets_interactive ЮТ 200 АМС)

$(INB_PNL_UT AMS PRG 200 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))

# Проверяем есть ли в базе задачи для полета. Должен быть SEND_NEW_APPS_INFO
??
$(check_flight_tasks $(get point_dep))
>> lines=auto
SEND_ALL_APPS_INFO
SEND_NEW_APPS_INFO

#################################

%%
###
#   Тест №24
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#            версия apps: 21
#
#   Посылка запросов в момент изменений информации по пассажиру на посадке
#   Должен посылаться APPS запрос на изменение
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps ЮТ ЦЗ APPS_21 closeout=true inbound=true outbound=true)

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set move_id $(get_move_id $(get point_dep)))
$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N)

$(set tid $(get_single_tid $(get point_dep) TUMALI VALERII))

# ответ по пассажиру TUMALI VALERII

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UA/UA/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/3////////

# измененение по пассажиру FA144642 -> FA144643
$(UPDATE_PAX_ON_BOARDING $(get pax_id) $(get point_dep) $(get tid) RUS FA144643 UA 16.04.1968 25.06.2025 M TUMALI VALERII)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/3/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/.*

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/UKR/RUS/FA144643//P/20250625////TUMALI/VALERII/19680416/M///N/N////.*

#####################################################################

#################################

%%
###
#   Тест №25
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#            версия apps: 21
#
#   Проверка перепосылки сообщения
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps ЮТ ЦЗ APPS_21 closeout=true inbound=true outbound=true)

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set move_id $(get_move_id $(get point_dep)))
$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N)

$(set msg_id1 $(capture 1))
$(set tid $(get_single_tid $(get point_dep) TUMALI VALERII))

# Не должна произойти перепосылка потому что прошло менее 10 секунд без ответа
$(update_msg $(get msg_id1) 5 2)
$(resend)

# Должна произойти перепосылка потому что прошло более 10 секунд без ответа
$(update_msg $(get msg_id1) 15 2)
$(resend)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N)

#Должна происходить перепосылка и выставляется ALARM::APPSOutage , потому что количество отправок равно 5
$(update_msg $(get msg_id1) 20 6)
$(resend)


$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N)

??
$(check_trip_alarms $(get point_dep))
>>
APPS_OUTAGE
$()

#Не должна происходить перепосылка , потому что количество отправок равно 99
$(update_msg $(get msg_id1) 30 99)
$(resend)

#####################################################################
%%
###
#   Тест №26
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#            версия apps: 21
#
#   APPS-запрос на посадку ОДНОГО пассажира уходит во время регистрации
#   APPS-запрос перепосылается для того же пассажира, когда есть ремарка OVRG
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps ЮТ ЦЗ APPS_21 closeout=true inbound=true outbound=false)

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set move_id $(get_move_id $(get point_dep)))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8507/U/1/////////

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))
$(set pax_id $(get_single_pax_id $(get point_dep) TUMALI VALERII))
$(set tid    $(get_single_tid    $(get point_dep) TUMALI VALERII))

#Апдейт пассажира с измененными данными , ответ со статусом U
!!
$(UPDATE_PAX_PASSPORT $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ UA FA144777 UA 16.04.1968 25.06.2025 M)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
          P UKR UKR FA144777 P 20250625 TUMALI VALERII 19680416 M N N)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UKR/UKR/FA144777//P//20250625////TUMALI/VALERII/19680416/M//8507/U/1/////////

$(set tid    $(get_single_tid    $(get point_dep) TUMALI VALERII))

#Принудительная перепосылка apps по тому же пассажиру , ответ со статусом B
!!
$(UPDATE_PAX_REM $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII OVRG "OVRG AE")

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
          P UKR UKR FA144777 P 20250625 TUMALI VALERII 19680416 M N N "" GAE)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UKR/UKR/FA144777//P//20250625////TUMALI/VALERII/19680416/M//8501/B/1/////////

$(set tid    $(get_single_tid    $(get point_dep) TUMALI VALERII))

#Принудительная перепосылка apps по тому же  пассажиру и изменением ремарки
!!
$(UPDATE_PAX_REM $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII OVRA "OVRA AE")


$(CICX_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
          1 P UKR UKR FA144777 P 20250625 TUMALI VALERII 19680416 M N N)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
          P UKR UKR FA144777 P 20250625 TUMALI VALERII 19680416 M N N "" AAE)

#####################################################################
%%
###
#   Тест №27
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#            версия apps: 21
#
#   Обработка PNL для APPS не должна падать, если пункт прилета PNL не совпадает с суточным планом полета
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps ЮТ ЦЗ APPS_21 closeout=true inbound=true outbound=false)

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER AMS 298 $(ddmon +0 en))
$(set point_dep $(last_point_id_spp))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

??
$(dump_table CRS_PAX fields="name, surname" where="name='OFER'" display=on)
>> lines=auto
[OFER] [OZ] $()

#################################

%%
###
#   Тест №28
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#            версия apps: 21
#
#   Проверка перепосылки сообщения CIMR
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult МОВРОМ)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps ЮТ ЦЗ APPS_21 closeout=true inbound=true outbound=true)
$(init_apps ЮТ НЛ APPS_21 closeout=false inbound=false outbound=false)

$(PREPARE_SEASON_SCD_TRANSIT ЮТ СОЧ АМС ПРХ 298) #сочи внуково прага
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get_next_trip_point_id $(get point_dep))))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set move_id $(get_move_id $(get point_dep)))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

# закрытие рейса. По настройке APPS должен уйти CIMR

!! err=ignore
$(WRITE_DESTS $(get point_dep) $(get point_arv) $(get move_id) ЮТ 298 СОЧ ПРХ
              $(date_format %d.%m.%Y) 10:15:00
              $(date_format %d.%m.%Y) 11:00:00)

>> lines=auto mode=regex
.*CIMR:([0-9]+)/UTUTA1/21/INM/3/UT298/AER/$(yyyymmdd)/MRQ/3/CZ/C/C/.*

$(set msg_id1 $(capture 1))

# Не должна произойти перепосылка потому что прошло менее 10 секунд без ответа
$(update_msg $(get msg_id1) 5 2)
$(resend)

# Должна произойти перепосылка потому что прошло более 10 секунд без ответа
$(update_msg $(get msg_id1) 15 2)
$(resend)

>> lines=auto mode=regex
.*CIMR:([0-9]+)/UTUTA1/21/INM/3/UT298/AER/$(yyyymmdd)/MRQ/3/CZ/C/C/.*

# Повторно должна произойти перепосылка потому что прошло более 10 секунд без ответа
$(update_msg $(get msg_id1) 15 2)
$(resend)

>> lines=auto mode=regex
.*CIMR:([0-9]+)/UTUTA1/21/INM/3/UT298/AER/$(yyyymmdd)/MRQ/3/CZ/C/C/.*


#Должна происходить перепосылка и выставляется ALARM::APPSOutage , потому что количество отправок равно 5
$(update_msg $(get msg_id1) 20 6)
$(resend)

>> lines=auto mode=regex
.*CIMR:([0-9]+)/UTUTA1/21/INM/3/UT298/AER/$(yyyymmdd)/MRQ/3/CZ/C/C/.*

??
$(check_trip_alarms $(get point_dep))
>>
APPS_OUTAGE
$()

#Не должна происходить перепосылка , потому что количество отправок равно 99
$(update_msg $(get msg_id1) 30 99)
$(resend)

########################################################

%%
########################################################################
###
#   Тест №29
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#            версия apps: 21
#
#   APPS CIMR обработка ответа CIMA и удаление сообщения из APPS_MESSAGES и APPS_MANIFEST_DATA
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult МОВРОМ)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps ЮТ ЦЗ APPS_21 closeout=true inbound=true outbound=true)
$(init_apps ЮТ НЛ APPS_21 closeout=false inbound=false outbound=false)

$(PREPARE_SEASON_SCD_TRANSIT ЮТ СОЧ АМС ПРХ 298) #сочи внуково прага
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get_next_trip_point_id $(get point_dep))))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set move_id $(get_move_id $(get point_dep)))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

# закрытие рейса. По настройке APPS должен уйти CIMR

!! err=ignore
$(WRITE_DESTS $(get point_dep) $(get point_arv) $(get move_id) ЮТ 298 СОЧ ПРХ
              $(date_format %d.%m.%Y) 10:15:00
              $(date_format %d.%m.%Y) 11:00:00)


>> lines=auto mode=regex
.*CIMR:([0-9]+)/UTUTA1/21/INM/3/UT298/AER/$(yyyymmdd)/MRQ/3/CZ/C/C/.*

#ответ на запрос о закрытии с ошибкой.
<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIMA:$(capture 1)/MAK/4/CZ/8701/6231/No movements found for this flight/

!! capture=on
$(GET_EVENTS $(get point_dep))

>> lines=auto
        <msg>Запрос на закрытие рейса. Результатдля страны ЦЗ: Запрос отклонен. Причина:No movements found for this flight</msg>

??
$(dump_pg_table apps_messages display=on)
>> lines=auto
$()
