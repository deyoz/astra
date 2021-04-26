include(ts/macro.ts)
include(ts/adm_macro.ts)

# meta: suite arch

#########################################################################################
###
#   Тест №1
#
#
###
#########################################################################################

$(init_jxt_pult МОВРОМ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

#$(PREPARE_SEASON_SCD ЮТ ПРХ АМС 200 -1 TU5 $(date_format %d.%m.%Y -12) $(date_format %d.%m.%Y +12))

$(PREPARE_SEASON_SCD ЮТ АМС ПРХ 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD ЮТ СОЧ ЛХР 100 -1 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +15))
$(make_spp $(ddmmyy +13))

$(INB_PNL_UT AMS PRG 300 $(ddmon +1 en))
$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(set move_id $(get_move_id $(get point_dep)))

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt, pr_del")

#На дату архивации через 133 дня от текущей даты сохранятся рейсы , которые были через 12 дней, от текущей даты
$(run_arch_step $(ddmmyy +133))

??
$(check_dump ARX_POINTS display="on")
>>
[$(get point_dep)] [$(get move_id)] [0] [АМС] [0] [NULL] [ЮТ] [300] [NULL] [ТУ5] [NULL] [NULL] [NULL] [NULL] [$(date_format %d.%m.%Y +1)] [NULL] [NULL] [п] [NULL] [NULL] [NULL] [NULL] [1] [0] [0] [0] [1] [NULL] [...] [$(date_format %d.%m.%Y +1)] $()
[$(get point_arv)] [$(get move_id)] [1] [ПРХ] [0] [$(get point_dep)] [NULL] [NULL] [NULL] [NULL] [NULL] [$(date_format %d.%m.%Y +1)] [NULL] [NULL] [NULL] [NULL] [NULL] [NULL] [NULL] [NULL] [NULL] [NULL] [0] [0] [0] [NULL] [NULL] [NULL] [...] [$(date_format %d.%m.%Y +1)] $()
$()

%%
#########################################################################################
###
#   Тест №2
#   Проверка архивации MOVE_ARX_EXT и ARX_MOVE_REF
#
###
#########################################################################################


$(init_jxt_pult МОВРОМ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

#$(PREPARE_SEASON_SCD ЮТ ПРХ АМС 200 -1 TU5 $(date_format %d.%m.%Y -12) $(date_format %d.%m.%Y +12))

$(PREPARE_SEASON_SCD ЮТ АМС ПРХ 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD ЮТ СОЧ ЛХР 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30) 30.12.1899 31.12.1899)
$(make_spp $(ddmmyy +20))

$(dump_table POINTS fields="point_id, move_id, pr_del, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")
$(dump_table MOVE_ARX_EXT fields = "date_range, move_id, part_key")

#$(run_arch_step $(ddmmyy +121))

#$(dump_table MOVE_ARX_EXT fields = "move_id, part_key, date_range")

#$(run_arch_step $(ddmmyy +131))

#$(dump_table MOVE_ARX_EXT fields = "move_id, part_key, date_range")

$(run_arch_step $(ddmmyy +151))

??
$(check_dump MOVE_ARX_EXT display="on")
>>
[2] [...] [$(date_format %d.%m.%Y +20)] $()
[2] [...] [$(date_format %d.%m.%Y +21)] $()
$()

??
$(check_dump ARX_MOVE_REF display="on")
>>
[...] [$(date_format %d.%m.%Y +1)] [NULL] $()
[...] [$(date_format %d.%m.%Y +20)] [NULL] $()
[...] [$(date_format %d.%m.%Y +21)] [NULL] $()
$()

%%
#########################################################################################
###
#   Тест №3
#   Проверка архивации ARX_EVENTS
#
###
#########################################################################################


$(init_jxt_pult МОВРОМ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD ЮТ АМС ПРХ 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD ЮТ СОЧ ЛХР 100 -1 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +267))
$(make_spp $(ddmmyy +265))

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")
$(dump_table ARX_EVENTS)
$(dump_table Events_Bilingual)

#$(run_arch_step $(ddmmyy +221))
#$(run_arch_step $(ddmmyy +386))

$(run_arch_step $(ddmmyy +387))

??
$(check_dump ARX_EVENTS display="on" order="ev_order, lang")
>>
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Web check-in forbidden for flight.] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [РЕЙ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [На рейсе запрещена web-регистрация.] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [РЕЙ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Stage 'Preparation for check-in': scheduled time 00:15 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Этап 'Подготовка к регистрации': план. время 00:15 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Stage 'Check-in opening': scheduled time 04:14 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Этап 'Открытие регистрации': план. время 04:14 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Stage 'Web check-in opening': scheduled time 07:15 $(date_format %d.%m.%y) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Этап 'Открытие web-регистрации': план. время 07:15 $(date_format %d.%m.%y) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Stage 'Kiosk check-in opening': scheduled time 07:15 $(date_format %d.%m.%y) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Этап 'Открытие kiosk-регистрации': план. время 07:15 $(date_format %d.%m.%y) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Stage 'Check-in closing': scheduled time 06:35 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Этап 'Закрытие регистрации': план. время 06:35 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Stage 'Ban of the cancel web check-in': scheduled time 06:25 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Этап 'Запрет отмены web-регистрации': план. время 06:25 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Stage 'Web check-in closing': scheduled time 04:15 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Этап 'Закрытие web-регистрации': план. время 04:15 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Stage 'Kiosk check-in closing': scheduled time 05:15 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Этап 'Закрытие kiosk-регистрации': план. время 05:15 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Stage 'Aircraft readiness for boarding': scheduled time 06:30 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Этап 'Готовность ВС к посадке': план. время 06:30 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Stage 'Boarding (doc. processing) completion': scheduled time 06:50 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Этап 'Окончание посадки (оформление докум.)': план. время 06:50 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Stage 'Rollback stairs': scheduled time 07:00 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Этап 'Откат трапа': план. время 07:00 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Passenger standard weight data input for the flight: ] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [РЕЙ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Назначение весов пассажиров на рейс: ] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [РЕЙ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [System task EMD_REFRESH <CloseCheckIn 0> created; Scheduled time: $(date_format %d.%m.%y +1) 06:35:00 (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ЗДЧ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Задача EMD_REFRESH <CloseCheckIn 0> создана; План. вр.: $(date_format %d.%m.%y +1) 06:35:00 (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ЗДЧ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Web check-in forbidden for flight.] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [РЕЙ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [На рейсе запрещена web-регистрация.] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [РЕЙ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Stage 'Preparation for check-in': scheduled time 00:15 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Этап 'Подготовка к регистрации': план. время 00:15 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Stage 'Check-in opening': scheduled time 04:14 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Этап 'Открытие регистрации': план. время 04:14 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Stage 'Web check-in opening': scheduled time 07:15 $(date_format %d.%m.%y +264) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Этап 'Открытие web-регистрации': план. время 07:15 $(date_format %d.%m.%y +264) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Stage 'Kiosk check-in opening': scheduled time 07:15 $(date_format %d.%m.%y +264) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Этап 'Открытие kiosk-регистрации': план. время 07:15 $(date_format %d.%m.%y +264) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Stage 'Check-in closing': scheduled time 06:35 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Этап 'Закрытие регистрации': план. время 06:35 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Stage 'Ban of the cancel web check-in': scheduled time 06:25 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Этап 'Запрет отмены web-регистрации': план. время 06:25 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Stage 'Web check-in closing': scheduled time 04:15 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Этап 'Закрытие web-регистрации': план. время 04:15 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Stage 'Kiosk check-in closing': scheduled time 05:15 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Этап 'Закрытие kiosk-регистрации': план. время 05:15 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Stage 'Aircraft readiness for boarding': scheduled time 06:30 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Этап 'Готовность ВС к посадке': план. время 06:30 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Stage 'Boarding (doc. processing) completion': scheduled time 06:50 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Этап 'Окончание посадки (оформление докум.)': план. время 06:50 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Stage 'airstairs driving away': scheduled time 07:00 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Этап 'отгон трапа': план. время 07:00 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ГРФ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Passenger standard weight data input for the flight: ] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [РЕЙ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Назначение весов пассажиров на рейс: ] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [РЕЙ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [System task EMD_REFRESH <CloseCheckIn 0> created; Scheduled time: $(date_format %d.%m.%y +265) 06:35:00 (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ЗДЧ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Задача EMD_REFRESH <CloseCheckIn 0> создана; План. вр.: $(date_format %d.%m.%y +265) 06:35:00 (UTC)] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [ЗДЧ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Flight's statistics collection] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [РЕЙ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Сбор статистики по рейсу] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [РЕЙ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Flight is transferred to archives] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [РЕЙ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Рейс перемещен в архив] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [РЕЙ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Flight is transferred to archives] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [РЕЙ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Рейс перемещен в архив] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [РЕЙ] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Flight's statistics collection] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [РЕЙ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Сбор статистики по рейсу] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [РЕЙ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Flight is transferred to archives] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [РЕЙ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Рейс перемещен в архив] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [РЕЙ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [EN] [Flight is transferred to archives] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [РЕЙ] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [КОВАЛЕВ Р.А.] [...] [NULL] [NULL] [RU] [Рейс перемещен в архив] [1] [AIR.EXE] [МОВРОМ] [$(date_format %d.%m.%Y)] [РЕЙ] [NULL] [$(date_format %d.%m.%Y +265)] $()
$()

#$(dump_table ARX_EVENTS order="ev_order, lang)


%%
#########################################################################################
###
#   Тест №4
#   Проверка архивации ARX_MARK_TRIPS и ARX_PAX_GRP
#
###
#########################################################################################


$(init_jxt_pult МОВРОМ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD ЮТ АМС ПРХ 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(deny_ets_interactive ЮТ 300 АМС)
$(make_spp $(ddmmyy +1))
$(INB_PNL_UT AMS PRG 300 $(ddmon +1 en))
$(set point_dep_UT_300 $(last_point_id_spp))
$(set point_arv_UT_300 $(get_next_trip_point_id $(get point_dep_UT_300)))

$(PREPARE_SEASON_SCD ЮТ СОЧ ЛХР 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive ЮТ 100 СОЧ)
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))

$(set pax_id_ALIMOV $(get_pax_id $(get point_dep_UT_300) ALIMOV TALGAT))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_300) TUMALI VALERII))


!!
$(CHECKIN_PAX $(get pax_id_ALIMOV) $(get point_dep_UT_300) $(get point_arv_UT_300) ЮТ 300 АМС ПРХ ALIMOV TALGAT 2982425696898 ВЗ KZ N11024936 KZ 11.05.1996 04.10.2026 M)

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_300) $(get point_arv_UT_100) ЮТ 300 АМС ПРХ TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")

#Архивируется только АМС-ПРХ
$(run_arch_step $(ddmmyy +122))
#$(check_dump ARX_MARK_TRIPS)

#Архивируется и СОЧ-ЛХР
$(run_arch_step $(ddmmyy +141))


??
$(check_dump ARX_MARK_TRIPS display="on")
>>
[ЮТ] [АМС] [300] [...] [$(date_format %d.%m.%Y)] [NULL] [$(date_format %d.%m.%Y +1)] $()
$()

??
$(check_dump ARX_PAX_GRP display="on")
>>
[ПРХ] [АМС] [0] [Э] [12] [TERM] [МОВРОМ] [0] [0] [...] [1] [0] [$(get point_arv_UT_300)] [$(get point_dep_UT_300)] [...] [0] [K] [...] [$(date_format %d.%m.%Y)] [5] [NULL] [NULL] [$(date_format %d.%m.%Y +1)] $()
$()



%%
#########################################################################################
###
#   Тест №5
#   Проверка архивации  STAT таблиц
#
###
#########################################################################################

$(init_jxt_pult МОВРОМ)
$(set_desk_version 201707-0195750)
$(login)

# $(PREPARE_SEASON_SCD ЮТ АМС ПРХ 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
# $(deny_ets_interactive ЮТ 300 АМС)
# $(make_spp $(ddmmyy +1))
# $(INB_PNL_UT AMS PRG 300 $(ddmon +1 en))
# $(set point_dep_UT_300 $(last_point_id_spp))
# $(set point_arv_UT_300 $(get_next_trip_point_id $(get point_dep_UT_300)))
# $(set pax_id_ALIMOV $(get_pax_id $(get point_dep_UT_300) TUMALI VALERII))
#
# !!
# $(CHECKIN_PAX $(get pax_id_ALIMOV) $(get point_dep_UT_300) $(get point_arv_UT_300) ЮТ 300 АМС ПРХ ALIMOV TALGAT 2982425696898 ВЗ KZ N11024936 KZ 11.05.1996 04.10.2026 M KIOSK2)

$(PREPARE_SEASON_SCD ЮТ СОЧ ЛХР 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive ЮТ 100 СОЧ)
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))

# это нужно для того чтобы pr_brd = 1 в таблице PAX,то есть посадить пассажира
# А это в свою очередь нужно чтобы заполнилась таблица STAT_AD и потом ARX_STAT_AD
$(sql {INSERT INTO trip_hall(point_id, type, hall, pr_misc)
       VALUES($(get point_dep_UT_100), 101, NULL, 1)})

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100) ЮТ 100 СОЧ ЛХР TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

$(run_arch_step $(ddmmyy +141))

??
$(check_dump ARX_AGENT_STAT display="on")
>>
[0] [0] [0] [0] [МОВРОМ] [1] [0] [0] [0] [0] [0] [0] [0] [$(date_format %d.%m.%Y)] [1] [3] [$(get point_dep_UT_100)] [0] [$(date_format %d.%m.%Y)] [$(date_format %d.%m.%Y +20)] $()
$()


??
$(check_dump ARX_PFS_STAT display="on")
>>
[ЛХР] [02.02.1982] [M] [PAVEL VALEREVICH MR] [...] [F0K77C] [$(get point_dep_UT_100)] [1] [NOSHO] [Ф] [BUGAEV] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [21.12.1979] [F] [JULIA RAVILEVNA MS] [...] [F0K77C] [$(get point_dep_UT_100)] [1] [NOSHO] [Ф] [CHETVERIKOVA] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [07.07.1990] [F] [LIUDMILA MS] [...] [F4828M] [$(get point_dep_UT_100)] [1] [NOSHO] [Ф] [FUKS] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [26.01.1967] [F] [ALBINA VALENTINOVNA] [...] [F3K91D] [$(get point_dep_UT_100)] [1] [NOSHO] [Ф] [KARUNA] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [24.07.2014] [F] [EKATERINA SERGEEVNA] [...] [F3K91D] [$(get point_dep_UT_100)] [1] [NOSHO] [Ф] [KARUNA] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [24.07.2014] [F] [ELIZAVETA SERGEEVNA] [...] [F3K91D] [$(get point_dep_UT_100)] [1] [NOSHO] [Ф] [KARUNA] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [24.07.2014] [M] [SERGEY SERGEEVICH] [...] [F3K91D] [$(get point_dep_UT_100)] [1] [NOSHO] [Ф] [KARUNA] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [01.01.1961] [M] [SERGEY VIKTOROVICH] [...] [F3K91D] [$(get point_dep_UT_100)] [1] [NOSHO] [Ф] [KARUNA] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [24.07.2014] [F] [SOFIYA SERGEEVNA] [...] [F3K91D] [$(get point_dep_UT_100)] [1] [NOSHO] [Ф] [KARUNA] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [07.11.1992] [M] [GEVORGSIMAVONOVICH MR] [...] [D8159D] [$(get point_dep_UT_100)] [1] [NOSHO] [Ф] [MAKEYAN] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [01.09.1958] [F] [ДЖИХАН] [...] [F36M42] [$(get point_dep_UT_100)] [1] [NOSHO] [Ф] [АВИДЗБА] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [20.12.2014] [M] [МАРК] [...] [F36M42] [$(get point_dep_UT_100)] [1] [NOSHO] [Ф] [АВИДЗБА] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [28.12.1987] [F] [САЛИМА] [...] [F36M42] [$(get point_dep_UT_100)] [1] [NOSHO] [Ф] [МКЕЛБА] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [23.04.1978] [F] [НАТАЛЬЯ ВАЛЕРИЕВНА] [...] [F29745] [$(get point_dep_UT_100)] [1] [NOSHO] [Ф] [АТОМАС] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [16.10.1963] [F] [ТАТЬЯНА] [...] [F44592] [$(get point_dep_UT_100)] [1] [NOSHO] [Ф] [БАРСУК] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [02.03.1975] [M] [ЕВГЕНИЙ ЕВГЕНЬЕВИЧ] [...] [F451B5] [$(get point_dep_UT_100)] [1] [NOSHO] [Ф] [БУРЯКОВ] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [24.10.1982] [F] [МАРИЯ СЕРГЕЕВНА] [...] [F3808K] [$(get point_dep_UT_100)] [1] [NOSHO] [Ф] [ДИКОВА] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [23.08.1985] [F] [ЮЛИЯ АЛЕКСАНДРОВНА] [...] [F43LF1] [$(get point_dep_UT_100)] [1] [NOSHO] [Ф] [ДМИТРИЕВА] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [21.11.2015] [M] [МИХАИЛ ГЕННАДЬЕВИЧ] [...] [F43LF1] [$(get point_dep_UT_100)] [1] [NOSHO] [Ф] [ЧАРКОВ] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [19.06.1967] [M] [ИГОРЬ АЛЕКСАНДРОВИЧ] [...] [F451F4] [$(get point_dep_UT_100)] [1] [NOSHO] [Ф] [САМОЙЛЕНКО] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [27.05.1986] [M] [АЛЕКСАНДР ВИКТОРОВИЧ] [...] [F1K182] [$(get point_dep_UT_100)] [1] [NOSHO] [Ф] [СЕРГИЕНКО] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [11.01.1990] [F] [ОЛЬГА АНДРЕЕВНА] [...] [F47290] [$(get point_dep_UT_100)] [1] [NOSHO] [Ф] [СТАРОДУБЦЕВА] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [14.07.2015] [F] [МАРИЯ СЕМЕНОВНА] [...] [F2L743] [$(get point_dep_UT_100)] [1] [NOSHO] [Ф] [ХАРЧЕНКО] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [12.04.1991] [F] [НАТАЛЬЯ АЛЕКСАНДРОВНА] [...] [F2L743] [$(get point_dep_UT_100)] [1] [NOSHO] [Ф] [ХАРЧЕНКО] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [25.05.1980] [M] [СЕМЕН ВЯЧЕСЛАВОВИЧ] [...] [F2KFMB] [$(get point_dep_UT_100)] [1] [NOSHO] [Ф] [ХАРЧЕНКО] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [21.06.1993] [M] [ANDREI] [...] [F522FC] [$(get point_dep_UT_100)] [1] [NOSHO] [В] [AKOPOV] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [05.04.2010] [F] [KIRA] [...] [F522FC] [$(get point_dep_UT_100)] [1] [NOSHO] [В] [BABAKHANOVA] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [23.07.1982] [F] [ANGELINA] [...] [F522FC] [$(get point_dep_UT_100)] [1] [NOSHO] [В] [STIPIDI] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [31.12.1986] [M] [ALEKSEY MR] [...] [F55681] [$(get point_dep_UT_100)] [1] [NOSHO] [В] [KOBYLINSKIY] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [NULL] [NULL] [OFER] [...] [F554G3] [$(get point_dep_UT_100)] [1] [NOSHO] [В] [OZ] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [22.10.1977] [F] [ОКСАНА] [...] [F543BB] [$(get point_dep_UT_100)] [1] [NOSHO] [В] [ЛУЧАК] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [30.12.1988] [M] [DENIS DMITRIEVICH] [...] [F4F4F0] [$(get point_dep_UT_100)] [1] [NOSHO] [К] [AGAFONOV] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [07.08.1993] [F] [MARIIA DMITRIEVNA] [...] [F4F4F0] [$(get point_dep_UT_100)] [1] [NOSHO] [К] [POLETAEVA] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [07.07.1979] [M] [DMITRII VLADIMIROVICH] [...] [F4F2D2] [$(get point_dep_UT_100)] [1] [NOSHO] [К] [ASTAFEV] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [13.09.1987] [F] [KRISTINA VALEREVNA] [...] [F4F2D2] [$(get point_dep_UT_100)] [1] [NOSHO] [К] [TIKHOMIROVA] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [31.08.1951] [M] [SERGEI MIKHAILOVICH] [...] [F4F617] [$(get point_dep_UT_100)] [1] [NOSHO] [К] [BALASHOV] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [31.03.1952] [F] [EKATERINA SERGEEVNA] [...] [F50234] [$(get point_dep_UT_100)] [1] [NOSHO] [К] [BUMBURIDI] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [03.02.2011] [M] [OLEG VIKTOROVICH] [...] [F4L8L3] [$(get point_dep_UT_100)] [1] [NOSHO] [К] [MOKSOKHOEV] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [07.09.1981] [M] [VICTOR SERGEEVICH] [...] [F4L8L3] [$(get point_dep_UT_100)] [1] [NOSHO] [К] [MOKSOKHOEV] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [12.10.1980] [F] [MARINA MRS] [...] [F4C271] [$(get point_dep_UT_100)] [1] [NOSHO] [К] [RUBLEVA] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [24.08.1972] [F] [СВЕТЛАНА ВЛАДИСЛАВОВНА] [...] [F4DM92] [$(get point_dep_UT_100)] [1] [NOSHO] [К] [КАПАНОВА] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [27.06.1973] [F] [ЕЛЕНА ВЛАДИМИРОВНА] [...] [F4B34L] [$(get point_dep_UT_100)] [1] [NOSHO] [К] [КРАВЦОВА] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [04.08.1978] [M] [ИЛЬЯ ВЛАДИМИРОВИЧ] [...] [F4D1G4] [$(get point_dep_UT_100)] [1] [NOSHO] [К] [КУЗНЕЦОВ] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [06.09.1974] [M] [СЕРГЕЙ] [...] [F4CMMB] [$(get point_dep_UT_100)] [1] [NOSHO] [К] [ТАГИРОВ] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [07.02.1968] [M] [ЕВГЕНИЙ ВЛАДИМИРОВИЧ] [...] [F4K2C3] [$(get point_dep_UT_100)] [1] [NOSHO] [К] [ЧЕТВЕРТКОВ] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [30.04.1979] [M] [МАКСИМ АЛЕКСЕЕВИЧ] [...] [F4BG9L] [$(get point_dep_UT_100)] [1] [NOSHO] [К] [ЯРИНЕНКО] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [14.05.1951] [M] [ODISSEI AFANASEVICH] [...] [F50266] [$(get point_dep_UT_100)] [1] [NOSHO] [Л] [BUMBURIDI] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [17.03.1990] [M] [KONSTANTIN ALEKSANDROVICH] [...] [F52MLM] [$(get point_dep_UT_100)] [1] [NOSHO] [Л] [CHEKMAREV] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [13.09.1984] [F] [KSENIYA VALEREVNA] [...] [F52MM0] [$(get point_dep_UT_100)] [1] [NOSHO] [Л] [VASILIADI] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [02.01.1959] [F] [RAISA GRIGOREVNA] [...] [F5203D] [$(get point_dep_UT_100)] [1] [NOSHO] [Л] [ZAINULLINA] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [17.01.1966] [M] [ВЛАДИМИР НИКОЛАЕВИЧ] [...] [F514B8] [$(get point_dep_UT_100)] [1] [NOSHO] [Л] [КОТЛЯР] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [23.09.1983] [M] [RUSLAN NAILYEVICH MR] [...] [F56KFM] [$(get point_dep_UT_100)] [1] [NOSHO] [О] [SELIVANOV] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [03.10.1972] [F] [ИРИНА ГЕННАДЬЕВНА] [...] [F58262] [$(get point_dep_UT_100)] [1] [NOSHO] [О] [РЯЗАНОВА] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [11.05.1996] [M] [TALGAT] [...] [F57K6C] [$(get point_dep_UT_100)] [1] [NOSHO] [У] [ALIMOV] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [06.11.1974] [F] [ZULFIYA] [...] [F57K6C] [$(get point_dep_UT_100)] [1] [NOSHO] [У] [KHASSENOVA] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [02.06.1987] [F] [ANNA GRIGOREVNA] [...] [F5659B] [$(get point_dep_UT_100)] [1] [NOSHO] [Ц] [KURGINSKAYA] [$(date_format %d.%m.%Y +20)] $()
[ЛХР] [04.11.1960] [M] [VASILII LEONIDOVICH] [...] [F58457] [$(get point_dep_UT_100)] [1] [NOSHO] [Ю] [VERGUNOV] [$(date_format %d.%m.%Y +20)] $()
$()

??
$(check_dump ARX_STAT_AD display="on")
>>
[NULL] [NULL] [Э] [TERM] [МОВРОМ] [...] [F50CF0] [$(get point_dep_UT_100)] [$(date_format %d.%m.%Y +20)] [5Г] [5D] [NULL] [$(date_format %d.%m.%Y +20)] $()
$()

??
$(check_dump ARX_STAT display="on")
>>
[1] [ЛХР] [0] [0] [0] [0] [0] [TERM] [0] [0] [0] [1] [0] [$(get point_dep_UT_100)] [N] [0] [0] [0] [0] [0] [1] [$(date_format %d.%m.%Y +20)] $()
$()


#??
#$(check_dump ARX_SELF_CKIN_STAT display="on")
#>> lines=auto
#??
#$(check_dump ARX_RFISC_STAT display="on")
#>> lines=auto
#??
#$(check_dump ARX_STAT_SERVICES display="on")
#>> lines=auto
#??
#$(check_dump ARX_STAT_REM display="on")
#>> lines=auto
#??
#$(check_dump ARX_LIMITED_CAPABILITY_STAT display="on")
#>> lines=auto
#??
#$(check_dump ARX_STAT_HA display="on")
#>> lines=auto
#??
#$(check_dump ARX_STAT_VO display="on")
#>> lines=auto
#??
#$(check_dump ARX_STAT_REPRINT display="on")
#>> lines=auto
#??
#$(check_dump ARX_TRFER_PAX_STAT display="on")
#>> lines=auto
#??
#$(check_dump ARX_BI_STAT display="on")
#>> lines=auto

#??
#$(check_dump ARX_TRFER_STAT display="on")
#>> lines=auto


%%
#########################################################################################
###
#   Тест №6
#   Проверка архивации TRIP таблиц
#
###
#########################################################################################


$(init_jxt_pult МОВРОМ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD ЮТ СОЧ ЛХР 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive ЮТ 100 СОЧ)
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100) ЮТ 100 СОЧ ЛХР TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

$(run_arch_step $(ddmmyy +141))

??
$(check_dump ARX_TRIP_CLASSES  display="on")
>>
[0] [11] [Б] [$(get point_dep_UT_100)] [0] [$(date_format %d.%m.%Y +20)] $()
[0] [63] [Э] [$(get point_dep_UT_100)] [0] [$(date_format %d.%m.%Y +20)] $()
$()

??
$(check_dump ARX_TRIP_SETS  display="on")
>>
[0] [...] [0] [NULL] [$(date_format %d.%m.%Y +20)] [$(get point_dep_UT_100)] [0] [1] [0] [-1] $()
$()

??
$(check_dump ARX_TRIP_STAGES  display="on")
>>
[NULL] [NULL] [$(date_format %d.%m.%Y +20)] [$(get point_dep_UT_100)] [0] [0] [$(date_format %d.%m.%Y +20)] [10] $()
[NULL] [NULL] [$(date_format %d.%m.%Y +20)] [$(get point_dep_UT_100)] [0] [0] [$(date_format %d.%m.%Y +20)] [20] $()
[NULL] [NULL] [$(date_format %d.%m.%Y +20)] [$(get point_dep_UT_100)] [1] [0] [$(date_format %d.%m.%Y +19)] [25] $()
[NULL] [NULL] [$(date_format %d.%m.%Y +20)] [$(get point_dep_UT_100)] [1] [0] [$(date_format %d.%m.%Y +19)] [26] $()
[NULL] [NULL] [$(date_format %d.%m.%Y +20)] [$(get point_dep_UT_100)] [0] [0] [$(date_format %d.%m.%Y +20)] [30] $()
[NULL] [NULL] [$(date_format %d.%m.%Y +20)] [$(get point_dep_UT_100)] [1] [0] [$(date_format %d.%m.%Y +20)] [31] $()
[NULL] [NULL] [$(date_format %d.%m.%Y +20)] [$(get point_dep_UT_100)] [0] [0] [$(date_format %d.%m.%Y +20)] [35] $()
[NULL] [NULL] [$(date_format %d.%m.%Y +20)] [$(get point_dep_UT_100)] [1] [0] [$(date_format %d.%m.%Y +20)] [36] $()
[NULL] [NULL] [$(date_format %d.%m.%Y +20)] [$(get point_dep_UT_100)] [0] [0] [$(date_format %d.%m.%Y +20)] [40] $()
[NULL] [NULL] [$(date_format %d.%m.%Y +20)] [$(get point_dep_UT_100)] [0] [0] [$(date_format %d.%m.%Y +20)] [50] $()
[NULL] [NULL] [$(date_format %d.%m.%Y +20)] [$(get point_dep_UT_100)] [0] [0] [$(date_format %d.%m.%Y +20)] [70] $()
$()

#??
#$(check_dump ARX_TRIP_DELAYS  display="on")
#>> lines=auto
#??
#$(check_dump ARX_TRIP_LOAD  display="on")
#>> lines=auto
#??
#$(check_dump ARX_CRS_DISPLACE2 display="on")
#>> lines=auto


%%
#########################################################################################
###
#   Тест №7
#   Проверка архивации таблиц ARX_TCKIN_SEGMENTS
#
###
#########################################################################################

$(init_jxt_pult МОВРОМ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

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

$(set grp_id1 $(get_single_grp_id $(get point_dep_UT_298) OZ OFER))
$(set grp_id2 $(get_single_grp_id $(get point_dep_UT_190) OZ OFER))
$(set grp_id3 $(get_single_grp_id $(get point_dep_UT_450) OZ OFER))

$(run_arch_step $(ddmmyy +141))

??
$(check_dump ARX_TCKIN_SEGMENTS)
>>
[ЮТ] [АМС] [ПРХ] [190] [$(get grp_id1)] [$(date_format %d.%m.%Y)] [0] [$(date_format %d.%m.%Y)] [1] [NULL] $()
[ЮТ] [ЛХР] [АМС] [450] [$(get grp_id1)] [$(date_format %d.%m.%Y)] [1] [$(date_format %d.%m.%Y)] [2] [NULL] $()
[ЮТ] [ЛХР] [АМС] [450] [$(get grp_id2)] [$(date_format %d.%m.%Y)] [1] [$(date_format %d.%m.%Y)] [1] [NULL] $()
$()

#$(check_dump ARX_PAX)
#$(check_dump ARX_PAX_GRP)
#$(check_dump ARX_POINTS)
#$(check_dump ARX_STAT_SERVICES)
#$(check_dump ARX_TRANSFER)

%%
#########################################################################################
###
#   Тест №8
#   Проверка архивации таблицы ARX_PAX_DOC
#
###
#########################################################################################


$(init_jxt_pult МОВРОМ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD ЮТ СОЧ ЛХР 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive ЮТ 100 СОЧ)
$(make_spp $(ddmmyy +22))
$(INB_PNL_UT AER LHR 100 $(ddmon +22 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100) ЮТ 100 СОЧ ЛХР TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

$(run_arch_step $(ddmmyy +161))

??
$(check_dump ARX_PAX_DOC)
>>
[16.04.1968] [25.06.2025] [VALERII] [M] [UKR] [UKR] [FA144642] [$(get pax_id_TUMALI)] [0] [0] [NULL] [NULL] [TUMALI] [P] [NULL] [$(date_format %d.%m.%Y +22)] $()
$()

#??
#$(check_dump ARX_PAX_NORMS)
#>> lines=auto
#
#??
#$(check_dump ARX_PAX_REM)
#>> lines=auto
#
#??
#$(check_dump ARX_TRANSFER_SUBCLS)
#>> lines=auto
#
#
#??
#$(check_dump ARX_PAX_DOCO)
#>> lines=auto
#
#??
#$(check_dump ARX_PAX_DOCA)
#>> lines=auto


%%
#########################################################################################
###
#   Тест №9
#   Проверка архивации SELF_CKIN_STAT
#
###
#########################################################################################

$(init_term)

$(PREPARE_SEASON_SCD ЮТ СОЧ ЛХР 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive ЮТ 100 СОЧ)
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))

$(init_kiosk)

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100) ЮТ 100 СОЧ ЛХР TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M KIOSK2)

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")

$(collect_flight_stat $(get point_dep_UT_100))

$(db_dump_table SELF_CKIN_STAT)

$(run_arch_step $(ddmmyy +141))

$(db_dump_table SELF_CKIN_STAT)

??
$(check_dump ARX_SELF_CKIN_STAT display="on")
>>
[1] [0] [0] [KIOSK] [КИОСК САМОРЕГИСТРАЦИИ  ЮТ] [KIOSK2] [NULL] [$(get point_dep_UT_100)] [0] [0] [0] [0] [$(date_format %d.%m.%Y +20)] $()
$()


%%
#########################################################################################
###
#   Тест №10
#   Проверка архивации всех таблиц после работы arx_daily
#
###
#########################################################################################


$(init_jxt_pult МОВРОМ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

#$(PREPARE_SEASON_SCD ЮТ АМС ПРХ 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
#$(make_spp $(ddmmyy +1))
#$(PREPARE_SEASON_SCD ЮТ СОЧ ЛХР 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30) 30.12.1899 31.12.1899)
#$(make_spp $(ddmmyy +20))


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


$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")

$(run_arch $(ddmmyy +151))

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")

??
$(check_dump ARX_PAX)
>>
[NULL] [Э] [12] [Э] [1] [0] [NULL] [...] [0] [0] [OFER] [$(date_format %d.%m.%Y )] [$(get pax_id1)] [ВЗ] [0] [0] [NULL] [1] [1] [5А] [NULL] [Э] [OZ] [0] [2985523437721] [TKNE] [...] [NULL] $()
[NULL] [Э] [12] [Э] [2] [0] [NULL] [...] [0] [0] [OFER] [$(date_format %d.%m.%Y )] [$(get pax_id2)] [ВЗ] [0] [0] [NULL] [1] [1] [5А] [NULL] [Э] [OZ] [0] [2985523437721] [TKNE] [...] [NULL] $()
[NULL] [Э] [12] [Э] [3] [0] [NULL] [...] [0] [0] [OFER] [$(date_format %d.%m.%Y )] [$(get pax_id3)] [ВЗ] [0] [0] [NULL] [1] [1] [5А] [NULL] [Э] [OZ] [0] [2985523437721] [TKNE] [...] [NULL] $()
$()

??
$(check_dump ARX_TRFER_PAX_STAT)
>> lines=auto
[0] [0] [$(get pax_id1)] [$(get point_dep_UT_298)] [0] [$(date_format %d.%m.%Y)] [ЮТ,СОЧ,298,,$(date_format %d.%m.%Y) 07:15:00,ПРХ;ЮТ,ПРХ,190,,$(date_format %d.%m.%Y) 07:15:00,АМС;ЮТ,АМС,450,,$(date_format %d.%m.%Y) 07:15:00,ЛХР] [$(date_format %d.%m.%Y )] $()
$()

#Проверка всех таблиц сразу закоментирована, потому что в результате теста они пустые и проверять дамп не имеет смысла,
#либо их архивация уже проверена в других тестах

#$(check_dump ARX_POINTS)
#$(check_dump MOVE_ARX_EXT)
#$(check_dump ARX_MOVE_REF)
##$(check_dump ARX_EVENTS order="ev_order, lang")
#$(check_dump ARX_MARK_TRIPS)
#$(check_dump ARX_PAX_GRP)

#$(check_dump ARX_SELF_CKIN_STAT)
#$(check_dump ARX_RFISC_STAT)
#$(check_dump ARX_STAT_SERVICES)
#$(check_dump ARX_STAT_REM)
#$(check_dump ARX_LIMITED_CAPABILITY_STAT)
#$(check_dump ARX_PFS_STAT)
#$(check_dump ARX_STAT_AD)
#$(check_dump ARX_STAT_HA)
#$(check_dump ARX_STAT_VO)
#$(check_dump ARX_STAT_REPRINT)

#$(check_dump ARX_BI_STAT)
#$(are_agent_stat_equal)
#$(check_dump ARX_STAT)
#$(check_dump ARX_TRFER_STAT)
#$(check_dump ARX_TRIP_CLASSES)
#$(check_dump ARX_TRIP_DELAYS)
#$(check_dump ARX_TRIP_LOAD)
#$(check_dump ARX_TRIP_SETS)
#$(check_dump ARX_CRS_DISPLACE2)
#$(check_dump ARX_TRIP_STAGES)

#$(check_dump ARX_BAG_RECEIPTS)
#$(check_dump ARX_BAG_PAY_TYPES)

#$(check_dump ARX_ANNUL_BAG)
#$(check_dump ARX_ANNUL_TAGS)
#$(check_dump ARX_UNACCOMP_BAG_INFO)
#$(check_dump ARX_BAG2)
#$(check_dump ARX_BAG_PREPAY)
#$(check_dump ARX_PAID_BAG)
#$(check_dump ARX_VALUE_BAG)
#$(check_dump ARX_GRP_NORMS)

#$(check_dump ARX_TRANSFER)
#$(check_dump ARX_TCKIN_SEGMENTS)

#$(check_dump ARX_PAX_NORMS)
#$(check_dump ARX_PAX_REM)
#$(check_dump ARX_TRANSFER_SUBCLS)

#$(check_dump ARX_PAX_DOC)
#$(check_dump ARX_PAX_DOCO)
#$(check_dump ARX_PAX_DOCA)

##$(check_dump ARX_EVENTS order="ev_order, lang")
#$(check_dump ARX_TLG_OUT)
#$(check_dump ARX_STAT_ZAMAR)

#$(check_dump ARX_BAG_NORMS)
#$(check_dump ARX_BAG_RATES)
#$(check_dump ARX_VALUE_BAG_TAXES)
#$(check_dump ARX_EXCHANGE_RATES)

#$(check_dump ARX_TLG_STAT)


%%
#########################################################################################
###
#   Тест №11
#   Проверка обработки исключения DUP_VAL_ON_INDEX
#
###
#########################################################################################


$(init_jxt_pult МОВРОМ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 300)

$(make_spp)

$(deny_ets_interactive ЮТ 298 СОЧ)
$(deny_ets_interactive ЮТ 300 СОЧ)

$(INB_PNL_UT_MARK1 AER PRG 298 $(ddmon +0 en))
$(set point_dep_UT_298 $(last_point_id_spp 0))
$(set point_arv_UT_298 $(get_next_trip_point_id $(get point_dep_UT_298)))


$(INB_PNL_UT_MARK2 AER PRG 300 $(ddmon +0 en))
$(set point_dep_UT_300 $(last_point_id_spp 0))
$(set point_arv_UT_300 $(get_next_trip_point_id $(get point_dep_UT_300)))


# $(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep_UT_298))

# $(set move_id $(get_move_id $(get point_dep)))

$(set pax_id1 $(get_pax_id $(get point_dep_UT_298) ALIMOV TALGAT))
$(set pax_id2 $(get_pax_id $(get point_dep_UT_300) OZ OFER))

!!
$(CHECKIN_PAX $(get pax_id1) $(get point_dep_UT_298) $(get point_arv_UT_298) ЮТ 298 СОЧ ПРХ ALIMOV TALGAT 2982425696898 ВЗ KZ N11024936 KZ 11.05.1996 04.10.2026 M)


!!
$(CHECKIN_PAX $(get pax_id2) $(get point_dep_UT_300) $(get point_arv_UT_300) ЮТ 300 СОЧ ПРХ OZ OFER 2985523437721 ВЗ UA 32427293 UA 16.04.1968 25.06.2025 M)

$(set pax_tid1 $(get_single_pax_tid  $(get point_dep_UT_298) ALIMOV TALGAT))
$(set pax_tid2 $(get_single_pax_tid  $(get point_dep_UT_300) OZ OFER))


$(run_arch_step $(ddmmyy +141))
??
$(check_dump ARX_MARK_TRIPS)
>>
[ЮТ] [СОЧ] [298] [$(get point_dep_UT_298)] [$(date_format %d.%m.%Y)] [NULL] [$(date_format %d.%m.%Y)] $()
[ЮТ] [СОЧ] [300] [$(get point_dep_UT_300)] [$(date_format %d.%m.%Y)] [NULL] [$(date_format %d.%m.%Y)] $()
$()

??
$(check_dump ARX_PAX_GRP)
>>
[ПРХ] [СОЧ] [0] [Э] [12] [TERM] [МОВРОМ] [0] [0] [...] [1] [0] [$(get point_arv_UT_298)] [$(get point_dep_UT_298)] [$(get point_dep_UT_298)] [0] [K] [$(get pax_tid1)] [$(date_format %d.%m.%Y)] [5] [NULL] [NULL] [$(date_format %d.%m.%Y)] $()
[ПРХ] [СОЧ] [0] [Э] [12] [TERM] [МОВРОМ] [0] [0] [...] [1] [0] [$(get point_arv_UT_300)] [$(get point_dep_UT_300)] [$(get point_dep_UT_300)] [0] [K] [$(get pax_tid2)] [$(date_format %d.%m.%Y)] [5] [NULL] [NULL] [$(date_format %d.%m.%Y)] $()
$()



#%%
##########################################################################################
####
##   Тест шаблон для шагов архивации
##   Проверка архивации таблиц для -го шага архивации
##
####
##########################################################################################
#
#
#$(init_jxt_pult МОВРОМ)
#$(set_desk_version 201707-0195750)
#$(login)
#
#################################################################################
#$(PREPARE_SEASON_SCD ЮТ АМС ПРХ 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
#$(make_spp $(ddmmyy +1))
#$(PREPARE_SEASON_SCD ЮТ СОЧ ЛХР 100 -1 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +267))
#$(make_spp $(ddmmyy +265))
#
#$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")
#$(dump_table TLGS_IN)
#
#$(run_arch_step $(ddmmyy +387) 4)

#$(check_dump ARX_BAG_RECEIPTS display="on")
#$(check_dump ARX_BAG_PAY_TYPES display="on")
#$(check_dump ARX_ANNUL_BAG)
#$(check_dump ARX_ANNUL_TAGS)
#$(check_dump ARX_UNACCOMP_BAG_INFO)
#$(check_dump ARX_BAG2)
#$(check_dump ARX_BAG_PREPAY)
#$(check_dump ARX_PAID_BAG)
#$(check_dump ARX_VALUE_BAG)
#$(check_dump ARX_GRP_NORMS)
#$(check_dump ARX_TLG_OUT)
#$(check_dump ARX_STAT_ZAMAR)
#$(check_dump ARX_BAG_NORMS)
#$(check_dump ARX_BAG_RATES)
#$(check_dump ARX_VALUE_BAG_TAXES)
#$(check_dump ARX_EXCHANGE_RATES)
#$(check_dump ARX_TLG_STAT)
