include(ts/macro.ts)

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

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")
#$(dump_pg_table ARX_POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, part_key, airp_fmt")

#Через 121 дня от текущей даты, ничего не архивируется
$(run_arch_step $(ddmmyy +121))

#На дату архива через 122 дня от текущей даты, архивируется 1 запись , которая через 1 день от текущей даты
$(run_arch_step $(ddmmyy +122))

#$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")
#$(dump_table ARX_POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, part_key, airp_fmt")
#$(dump_pg_table ARX_POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, part_key, airp_fmt")

#На дату архивации через 133 дня от текущей даты сохранятся рейсы , которые были через 12 дней, от текущей даты
$(run_arch_step $(ddmmyy +133))

#$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")
#$(dump_table ARX_POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, part_key, airp_fmt")
#$(dump_pg_table ARX_POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, part_key, airp_fmt")

$(are_tables_equal ARX_POINTS)

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

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")
$(dump_table MOVE_ARX_EXT fields = "date_range, move_id, part_key")

$(run_arch_step $(ddmmyy +121))

#$(dump_table MOVE_ARX_EXT fields = "move_id, part_key, date_range")
#$(dump_pg_table MOVE_ARX_EXT fields = "move_id, part_key, date_range")

$(run_arch_step $(ddmmyy +131))

#$(dump_table MOVE_ARX_EXT fields = "move_id, part_key, date_range")
#$(dump_pg_table MOVE_ARX_EXT fields = "move_id, part_key, date_range")

$(run_arch_step $(ddmmyy +151))

$(are_tables_equal MOVE_ARX_EXT)
$(are_tables_equal ARX_MOVE_REF)

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

#$(dump_table ARX_EVENTS order="ev_order, lang)
$(are_tables_equal ARX_EVENTS order="ev_order, lang")

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
$(are_tables_equal ARX_MARK_TRIPS)

#Архивируется и СОЧ-ЛХР
$(run_arch_step $(ddmmyy +141))

$(dump_table PAX_GRP)
$(dump_table MARK_TRIPS)

$(are_tables_equal ARX_MARK_TRIPS)
$(are_tables_equal ARX_PAX_GRP)

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

################################################################################

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


!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100) ЮТ 100 СОЧ ЛХР TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

$(run_arch_step $(ddmmyy +141))
$(are_tables_equal ARX_SELF_CKIN_STAT)
$(are_tables_equal ARX_RFISC_STAT)
$(are_tables_equal ARX_STAT_SERVICES)
$(are_tables_equal ARX_STAT_REM)
$(are_tables_equal ARX_LIMITED_CAPABILITY_STAT)
$(are_tables_equal ARX_PFS_STAT)
$(are_tables_equal ARX_STAT_AD)
$(are_tables_equal ARX_STAT_HA)
$(are_tables_equal ARX_STAT_VO)
$(are_tables_equal ARX_STAT_REPRINT)
$(are_tables_equal ARX_TRFER_PAX_STAT)
$(are_tables_equal ARX_BI_STAT)
$(are_agent_stat_equal)
$(are_tables_equal ARX_STAT)
$(are_tables_equal ARX_TRFER_STAT)

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

$(are_tables_equal ARX_TRIP_CLASSES)
$(are_tables_equal ARX_TRIP_DELAYS)
$(are_tables_equal ARX_TRIP_LOAD)
$(are_tables_equal ARX_TRIP_SETS)
$(are_tables_equal ARX_CRS_DISPLACE2)
$(are_tables_equal ARX_TRIP_STAGES)


%%
#########################################################################################
###
#   Тест №7
#   Проверка архивации BAG таблиц
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
$(are_tables_equal ARX_BAG_RECEIPTS)
$(are_tables_equal ARX_BAG_PAY_TYPES)

%%
#########################################################################################
###
#   Тест №8
#   Проверка архивации таблиц Loop by pax groups
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
$(are_tables_equal ARX_ANNUL_BAG)
$(are_tables_equal ARX_ANNUL_TAGS)
$(are_tables_equal ARX_UNACCOMP_BAG_INFO)
$(are_tables_equal ARX_BAG2)
$(are_tables_equal ARX_BAG_PREPAY)
$(are_tables_equal ARX_PAID_BAG)
$(are_tables_equal ARX_VALUE_BAG)
$(are_tables_equal ARX_GRP_NORMS)

%%
#########################################################################################
###
#   Тест №9
#   Проверка архивации таблиц Loop by groups
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


$(dump_table TRANSFER)
$(dump_table TRFER_TRIPS)
$(dump_table TCKIN_SEGMENTS)
$(dump_table PAX_GRP)
$(dump_table POINTS)

$(run_arch_step $(ddmmyy +141))
$(are_tables_equal ARX_PAX)

$(are_tables_equal ARX_TRANSFER)
$(are_tables_equal ARX_TCKIN_SEGMENTS)


%%
#########################################################################################
###
#   Тест №10
#   Проверка архивации таблиц Loop by pax_id
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

$(dump_table PAX_DOC)

$(run_arch_step $(ddmmyy +161))
$(are_tables_equal ARX_PAX_NORMS)
$(are_tables_equal ARX_PAX_REM)
$(are_tables_equal ARX_TRANSFER_SUBCLS)

$(are_tables_equal ARX_PAX_DOC)
$(are_tables_equal ARX_PAX_DOCO)
$(are_tables_equal ARX_PAX_DOCA)


%%
#########################################################################################
###
#   Тест №11
#   Проверка архивации таблиц для второго шага архивации
#   arx_events, arx_tlg_out, arx_stat_zamar
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
#$(dump_table TLG_OUT)
#$(dump_table STAT_ZAMAR)
#$(dump_table Events_Bilingual)

$(run_arch_step $(ddmmyy +387) 2)

#$(dump_table ARX_EVENTS order="ev_order, lang)
$(are_tables_equal ARX_EVENTS order="ev_order, lang")
$(are_tables_equal ARX_TLG_OUT)
$(are_tables_equal ARX_STAT_ZAMAR)

%%
#########################################################################################
###
#   Тест №12
#   Проверка архивации таблиц для третьего шага архивации TArxTlgTrips
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

$(run_arch_step $(ddmmyy +387) 3)


%%
#########################################################################################
###
#   Тест №13
#   Проверка архивации таблиц для четвертого шага архивации TArxTypeBIn
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

$(run_arch_step $(ddmmyy +387) 4)


%%
#########################################################################################
###
#   Тест №14
#   Проверка архивации таблиц для пятого шага архивации TArxNormsRatesEtc
#   arx_bag_norms, arx_bag_rates, arx_value_bag_taxes, arx_exchange_rates
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

$(dump_table MARK_TRIPS)

$(run_arch_step $(ddmmyy +387) 5)

$(are_tables_equal ARX_BAG_NORMS)
$(are_tables_equal ARX_BAG_RATES)
$(are_tables_equal ARX_VALUE_BAG_TAXES)
$(are_tables_equal ARX_EXCHANGE_RATES)


%%
#########################################################################################
###
#   Тест №15
#   Проверка архивации таблиц для шестого шага архивации TArxTlgsFilesEtc
#   arx_tlgs, arx_tlg_stat, files, kiosk_events, rozysk, aodb_spp_files, eticks_display,
#   eticks_display_tlgs, emdocs_display
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


$(run_arch_step $(ddmmyy +387) 6)

$(are_tables_equal ARX_TLG_STAT)


%%
#########################################################################################
###
#   Тест №16
#   Проверка архивации SELF_CKIN_STAT
#
###
#########################################################################################

$(desc_test 1)

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

$(run_arch_step $(ddmmyy +141))
$(are_tables_equal ARX_SELF_CKIN_STAT)


%%
#########################################################################################
###
#   Тест №17
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

$(are_tables_equal ARX_POINTS)
$(are_tables_equal MOVE_ARX_EXT)
$(are_tables_equal ARX_MOVE_REF)
$(are_tables_equal ARX_EVENTS order="ev_order, lang")
$(are_tables_equal ARX_MARK_TRIPS)
$(are_tables_equal ARX_PAX_GRP)

$(are_tables_equal ARX_SELF_CKIN_STAT)
$(are_tables_equal ARX_RFISC_STAT)
$(are_tables_equal ARX_STAT_SERVICES)
$(are_tables_equal ARX_STAT_REM)
$(are_tables_equal ARX_LIMITED_CAPABILITY_STAT)
$(are_tables_equal ARX_PFS_STAT)
$(are_tables_equal ARX_STAT_AD)
$(are_tables_equal ARX_STAT_HA)
$(are_tables_equal ARX_STAT_VO)
$(are_tables_equal ARX_STAT_REPRINT)
$(are_tables_equal ARX_TRFER_PAX_STAT)
$(are_tables_equal ARX_BI_STAT)
$(are_agent_stat_equal)
$(are_tables_equal ARX_STAT)
$(are_tables_equal ARX_TRFER_STAT)
$(are_tables_equal ARX_TRIP_CLASSES)
$(are_tables_equal ARX_TRIP_DELAYS)
$(are_tables_equal ARX_TRIP_LOAD)
$(are_tables_equal ARX_TRIP_SETS)
$(are_tables_equal ARX_CRS_DISPLACE2)
$(are_tables_equal ARX_TRIP_STAGES)

$(are_tables_equal ARX_BAG_RECEIPTS)
$(are_tables_equal ARX_BAG_PAY_TYPES)

$(are_tables_equal ARX_ANNUL_BAG)
$(are_tables_equal ARX_ANNUL_TAGS)
$(are_tables_equal ARX_UNACCOMP_BAG_INFO)
$(are_tables_equal ARX_BAG2)
$(are_tables_equal ARX_BAG_PREPAY)
$(are_tables_equal ARX_PAID_BAG)
$(are_tables_equal ARX_VALUE_BAG)
$(are_tables_equal ARX_GRP_NORMS)

$(are_tables_equal ARX_PAX)
$(are_tables_equal ARX_TRANSFER)
$(are_tables_equal ARX_TCKIN_SEGMENTS)

$(are_tables_equal ARX_PAX_NORMS)
$(are_tables_equal ARX_PAX_REM)
$(are_tables_equal ARX_TRANSFER_SUBCLS)

$(are_tables_equal ARX_PAX_DOC)
$(are_tables_equal ARX_PAX_DOCO)
$(are_tables_equal ARX_PAX_DOCA)

$(are_tables_equal ARX_EVENTS order="ev_order, lang")
$(are_tables_equal ARX_TLG_OUT)
$(are_tables_equal ARX_STAT_ZAMAR)

$(are_tables_equal ARX_BAG_NORMS)
$(are_tables_equal ARX_BAG_RATES)
$(are_tables_equal ARX_VALUE_BAG_TAXES)
$(are_tables_equal ARX_EXCHANGE_RATES)

$(are_tables_equal ARX_TLG_STAT)


%%
#########################################################################################
###
#   Тест №18
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


$(dump_table POINTS)
$(dump_table MARK_TRIPS)
$(dump_table PAX_GRP)


$(run_arch_step $(ddmmyy +141))

$(are_tables_equal ARX_MARK_TRIPS)
$(are_tables_equal ARX_PAX_GRP)
