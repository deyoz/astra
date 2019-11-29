include(ts/macro.ts)

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

$(init_jxt_pult МОВРОМ)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps ЮТ ЦЗ APPS_21 closeout=true)

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(combine_brd_wirth_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set move_id $(get_move_id $(get point_dep)))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/100000/PRQ/22/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N////.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/1////////

!! capture=on
$(GET_EVENTS $(get point_dep))

>> lines=auto
        <msg>Запрос на посадкудля пассажира TUMALI. Результатдля страны ЦЗ: Посадка разрешена.</msg>

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))
$(set tid $(get_single_tid $(get point_dep) TUMALI VALERII))

!!
$(CANCEL_PAX $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/100000/PCX/20/1/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/.*


# регистрация члена экипажа

$(CHECKIN_CREW $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ VOLODIN SEMEN ВЗ RUS 2124134 RUS 11.05.1978 15.05.2025 M)

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/100000/PRQ/22/1/C/RUS/RUS/2124134//P/20250515////VOLODIN/SEMEN/19780511/M///N/N////.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/C/RUS/RUS/2124134//P/20250515////VOLODIN/SEMEN/19780511/M///8501/B/10////////

$(set grp_id $(get_single_grp_id $(get point_dep) VOLODIN SEMEN))
$(set pax_id $(get_single_pax_id $(get point_dep) VOLODIN SEMEN))
$(set tid $(get_single_tid $(get point_dep) VOLODIN SEMEN))

# отмена регистрации члена экипажа (CICX уйдет только если был ответ на CIRS)

!!
$(CANCEL_CHECKIN_CREW $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ VOLODIN SEMEN ВЗ)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/100000/PCX/20/1/10/C/RUS/RUS/2124134//P/20250515////VOLODIN/SEMEN/19780511/M///N/N/.*


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

$(init_jxt_pult МОВРОМ)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps ЮТ ЦЗ APPS_21 closeout=false)

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set move_id $(get_move_id $(get point_dep)))

$(combine_brd_wirth_reg $(get point_dep))
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


# по пассажиру FUKS LIUDMILA

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id38)/PRS/27/001/CZ/////////////////8502/D////////


# регистрация пассажира TUMALI VALERII с документом как в PNL
# apps не должен по идее пойти, но УХОДИТ

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/100000/PCX/20/1/3/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/.*
>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/100000/PRQ/22/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N////.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UA/UA/FA144642//P//20250625////TUMALI/VALERII/19680416/U//8509/X/////////


# регистрация пассажира SELIVANOV RUSLAN с измененным документом
# apps должен пойти

$(set pax_id $(get_pax_id $(get point_dep) SELIVANOV "RUSLAN NAILYEVICH MR"))

!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 298 СОЧ ПРХ SELIVANOV "RUSLAN NAILYEVICH" 2985085963078 ВЗ UA 12342131 UA 23.09.1983 20.12.2025 M)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/100000/PCX/20/1/2/P/RUS/RUS/9205589611//P/20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M///N/N/.*
>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/100000/PRQ/22/1/P/UKR/UKR/12342131//P/20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M///N/N////.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/RU/RU/9205589611//P//20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M//8502/D/////////


!! capture=on
$(GET_EVENTS $(get point_dep))

>> mode=regex
.*<msg>Запрос на посадкудля пассажира TUMALI. Результатдля страны ЦЗ: Посадка запрещена. Перезапись невозможна.</msg>.*
.*<msg>Запрос на посадкудля пассажира SELIVANOV. Результатдля страны ЦЗ: Посадка запрещена.</msg>.*


# приходит ADL с удалением двух пассажиров
$(INB_ADL_UT_DEL2PAXES AER PRG 298 $(ddmon +0 en))

# должна пойти отмена, но не уходит - ОШИБКА ??
$(run_trip_task send_apps $(get point_dep))


# приходит ADL с изменением данных по одному пассажиру
$(INB_ADL_UT_CHG1PAX AER PRG 298 $(ddmon +0 en))

$(run_trip_task send_apps $(get point_dep))

# уходит пара CICX/CIRQ, но ТОЛЬКО при условии получения ответа
# ранее по этому пассажиру!!!

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/100000/PCX/20/1/1/P/RUS/RUS/0319189298//P/20201008////KURGINSKAYA/ANNA GRIGOREVNA/19870602/F///N/N/.*

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/100000/PRQ/22/1/P/RUS/RUS/0319189297//P/20201008////KURGINSKAYA/ANNA GRIGOREVNA/19870602/F///N/N////.*


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

$(init_jxt_pult МОВРОМ)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps ЮТ ЦЗ APPS_21)

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
