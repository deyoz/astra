include(ts/macro.ts)
include(ts/adm_macro.ts)

# meta: suite arch

#########################################################################################
###
#   ’¥áâ ü1
#
#
###
#########################################################################################

$(init_jxt_pult ŒŽ‚ŽŒ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

#$(PREPARE_SEASON_SCD ž’ • €Œ‘ 200 -1 TU5 $(date_format %d.%m.%Y -12) $(date_format %d.%m.%Y +12))

$(PREPARE_SEASON_SCD ž’ €Œ‘ • 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD ž’ ‘Ž— ‹• 100 -1 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +15))
$(make_spp $(ddmmyy +13))

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt, pr_del")

#—¥à¥§ 121 ¤­ï ®â â¥ªãé¥© ¤ âë, ­¨ç¥£® ­¥  àå¨¢¨àã¥âáï
#$(run_arch_step $(ddmmyy +121))

#  ¤ âã  àå¨¢  ç¥à¥§ 122 ¤­ï ®â â¥ªãé¥© ¤ âë,  àå¨¢¨àã¥âáï 1 § ¯¨áì , ª®â®à ï ç¥à¥§ 1 ¤¥­ì ®â â¥ªãé¥© ¤ âë
#$(run_arch_step $(ddmmyy +122))

#$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt, pr_del")
#$(dump_table ARX_POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, part_key, airp_fmt")

#  ¤ âã  àå¨¢ æ¨¨ ç¥à¥§ 133 ¤­ï ®â â¥ªãé¥© ¤ âë á®åà ­ïâáï à¥©áë , ª®â®àë¥ ¡ë«¨ ç¥à¥§ 12 ¤­¥©, ®â â¥ªãé¥© ¤ âë
$(run_arch_step $(ddmmyy +133))

#$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt, pr_del")
#$(dump_table ARX_POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, part_key, airp_fmt")

$(are_tables_equal ARX_POINTS)

%%
#########################################################################################
###
#   ’¥áâ ü2
#   à®¢¥àª   àå¨¢ æ¨¨ MOVE_ARX_EXT ¨ ARX_MOVE_REF
#
###
#########################################################################################


$(init_jxt_pult ŒŽ‚ŽŒ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

#$(PREPARE_SEASON_SCD ž’ • €Œ‘ 200 -1 TU5 $(date_format %d.%m.%Y -12) $(date_format %d.%m.%Y +12))

$(PREPARE_SEASON_SCD ž’ €Œ‘ • 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD ž’ ‘Ž— ‹• 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30) 30.12.1899 31.12.1899)
$(make_spp $(ddmmyy +20))

$(dump_table POINTS fields="point_id, move_id, pr_del, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")
$(dump_table MOVE_ARX_EXT fields = "date_range, move_id, part_key")

$(run_arch_step $(ddmmyy +121))

#$(dump_table MOVE_ARX_EXT fields = "move_id, part_key, date_range")

$(run_arch_step $(ddmmyy +131))

#$(dump_table MOVE_ARX_EXT fields = "move_id, part_key, date_range")

$(run_arch_step $(ddmmyy +151))

$(are_tables_equal MOVE_ARX_EXT)
$(are_tables_equal ARX_MOVE_REF)

%%
#########################################################################################
###
#   ’¥áâ ü3
#   à®¢¥àª   àå¨¢ æ¨¨ ARX_EVENTS
#
###
#########################################################################################


$(init_jxt_pult ŒŽ‚ŽŒ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD ž’ €Œ‘ • 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD ž’ ‘Ž— ‹• 100 -1 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +267))
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
#   ’¥áâ ü4
#   à®¢¥àª   àå¨¢ æ¨¨ ARX_MARK_TRIPS ¨ ARX_PAX_GRP
#
###
#########################################################################################


$(init_jxt_pult ŒŽ‚ŽŒ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD ž’ €Œ‘ • 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(deny_ets_interactive ž’ 300 €Œ‘)
$(make_spp $(ddmmyy +1))
$(INB_PNL_UT AMS PRG 300 $(ddmon +1 en))
$(set point_dep_UT_300 $(last_point_id_spp))
$(set point_arv_UT_300 $(get_next_trip_point_id $(get point_dep_UT_300)))


$(PREPARE_SEASON_SCD ž’ ‘Ž— ‹• 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive ž’ 100 ‘Ž—)
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))

$(set pax_id_ALIMOV $(get_pax_id $(get point_dep_UT_300) ALIMOV TALGAT))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_300) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id_ALIMOV) $(get point_dep_UT_300) $(get point_arv_UT_300) ž’ 300 €Œ‘ • ALIMOV TALGAT 2982425696898 ‚‡ KZ N11024936 KZ 11.05.1996 04.10.2026 M)

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_300) $(get point_arv_UT_100) ž’ 300 €Œ‘ • TUMALI VALERII 2986145115578 ‚‡ UA FA144642 UA 16.04.1968 25.06.2025 M)

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")

#€àå¨¢¨àã¥âáï â®«ìª® €Œ‘-•
$(run_arch_step $(ddmmyy +122))
$(are_tables_equal ARX_MARK_TRIPS)

#€àå¨¢¨àã¥âáï ¨ ‘Ž—-‹•
$(run_arch_step $(ddmmyy +141))

$(dump_table PAX_GRP)
$(dump_table MARK_TRIPS)

$(are_tables_equal ARX_MARK_TRIPS)
$(are_tables_equal ARX_PAX_GRP)


%%
#########################################################################################
###
#   ’¥áâ ü5
#   à®¢¥àª   àå¨¢ æ¨¨  STAT â ¡«¨æ
#
###
#########################################################################################

$(init_jxt_pult ŒŽ‚ŽŒ)
$(set_desk_version 201707-0195750)
$(login)

# $(PREPARE_SEASON_SCD ž’ €Œ‘ • 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
# $(deny_ets_interactive ž’ 300 €Œ‘)
# $(make_spp $(ddmmyy +1))
# $(INB_PNL_UT AMS PRG 300 $(ddmon +1 en))
# $(set point_dep_UT_300 $(last_point_id_spp))
# $(set point_arv_UT_300 $(get_next_trip_point_id $(get point_dep_UT_300)))
# $(set pax_id_ALIMOV $(get_pax_id $(get point_dep_UT_300) TUMALI VALERII))
#
# !!
# $(CHECKIN_PAX $(get pax_id_ALIMOV) $(get point_dep_UT_300) $(get point_arv_UT_300) ž’ 300 €Œ‘ • ALIMOV TALGAT 2982425696898 ‚‡ KZ N11024936 KZ 11.05.1996 04.10.2026 M KIOSK2)

$(PREPARE_SEASON_SCD ž’ ‘Ž— ‹• 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive ž’ 100 ‘Ž—)
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))

# íâ® ­ã¦­® ¤«ï â®£® çâ®¡ë pr_brd = 1 ¢ â ¡«¨æ¥ PAX,â® ¥áâì ¯®á ¤¨âì ¯ áá ¦¨à 
# € íâ® ¢ á¢®î ®ç¥à¥¤ì ­ã¦­® çâ®¡ë § ¯®«­¨« áì â ¡«¨æ  STAT_AD ¨ ¯®â®¬ ARX_STAT_AD
$(sql {INSERT INTO trip_hall(point_id, type, hall, pr_misc)
       VALUES($(get point_dep_UT_100), 101, NULL, 1)})

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100) ž’ 100 ‘Ž— ‹• TUMALI VALERII 2986145115578 ‚‡ UA FA144642 UA 16.04.1968 25.06.2025 M)

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
#   ’¥áâ ü6
#   à®¢¥àª   àå¨¢ æ¨¨ TRIP â ¡«¨æ
#
###
#########################################################################################


$(init_jxt_pult ŒŽ‚ŽŒ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD ž’ ‘Ž— ‹• 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive ž’ 100 ‘Ž—)
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100) ž’ 100 ‘Ž— ‹• TUMALI VALERII 2986145115578 ‚‡ UA FA144642 UA 16.04.1968 25.06.2025 M)

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
#   ’¥áâ ü7
#   à®¢¥àª   àå¨¢ æ¨¨ BAG â ¡«¨æ
#
###
#########################################################################################


$(init_jxt_pult ŒŽ‚ŽŒ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD ž’ ‘Ž— ‹• 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive ž’ 100 ‘Ž—)
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100) ž’ 100 ‘Ž— ‹• TUMALI VALERII 2986145115578 ‚‡ UA FA144642 UA 16.04.1968 25.06.2025 M)

$(run_arch_step $(ddmmyy +141))
$(are_tables_equal ARX_BAG_RECEIPTS)
$(are_tables_equal ARX_BAG_PAY_TYPES)

%%
#########################################################################################
###
#   ’¥áâ ü8
#   à®¢¥àª   àå¨¢ æ¨¨ â ¡«¨æ Loop by pax groups
#
###
#########################################################################################


$(init_jxt_pult ŒŽ‚ŽŒ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD ž’ ‘Ž— ‹• 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive ž’ 100 ‘Ž—)
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100) ž’ 100 ‘Ž— ‹• TUMALI VALERII 2986145115578 ‚‡ UA FA144642 UA 16.04.1968 25.06.2025 M)

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
#   ’¥áâ ü9
#   à®¢¥àª   àå¨¢ æ¨¨ â ¡«¨æ Loop by groups
#
###
#########################################################################################


$(init_jxt_pult ŒŽ‚ŽŒ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD ž’ ‘Ž— • 298)
$(PREPARE_SEASON_SCD ž’ • €Œ‘ 190)
$(PREPARE_SEASON_SCD ž’ €Œ‘ ‹• 450)

$(make_spp)

$(deny_ets_interactive ž’ 298 ‘Ž—)
$(deny_ets_interactive ž’ 190 •)
$(deny_ets_interactive ž’ 450 €Œ‘)

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
$(CHECKIN_PAX_TRANSFER $(get pax_id1) $(get point_dep_UT_298) $(get point_arv_UT_298) ž’ 298 ‘Ž— • OZ OFER 2985523437721
                       $(get pax_id2) $(get point_dep_UT_190) $(get point_arv_UT_190) ž’ 190 • €Œ‘ OZ OFER
                       $(get pax_id3) $(get point_dep_UT_450) $(get point_arv_UT_450) ž’ 450 €Œ‘ ‹• OZ OFER
                       ‚‡ UA 32427293 UA 16.04.1968 25.06.2025 M)


$(dump_table TRANSFER)
$(dump_table TRFER_TRIPS)
$(dump_table TCKIN_SEGMENTS)
$(dump_table PAX_GRP)
$(dump_table POINTS)

$(run_arch_step $(ddmmyy +141))
$(are_tables_equal ARX_PAX)
$(are_tables_equal ARX_PAX_GRP)
$(are_tables_equal ARX_POINTS)

$(are_tables_equal ARX_STAT_SERVICES)
$(are_tables_equal ARX_TRANSFER)
$(are_tables_equal ARX_TCKIN_SEGMENTS)



%%
#########################################################################################
###
#   ’¥áâ ü10
#   à®¢¥àª   àå¨¢ æ¨¨ â ¡«¨æ Loop by pax_id
#
###
#########################################################################################


$(init_jxt_pult ŒŽ‚ŽŒ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD ž’ ‘Ž— ‹• 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive ž’ 100 ‘Ž—)
$(make_spp $(ddmmyy +22))
$(INB_PNL_UT AER LHR 100 $(ddmon +22 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100) ž’ 100 ‘Ž— ‹• TUMALI VALERII 2986145115578 ‚‡ UA FA144642 UA 16.04.1968 25.06.2025 M)

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
#   ’¥áâ ü11
#   à®¢¥àª   àå¨¢ æ¨¨ â ¡«¨æ ¤«ï ¢â®à®£® è £   àå¨¢ æ¨¨
#   arx_events, arx_tlg_out, arx_stat_zamar
#
###
#########################################################################################


$(init_jxt_pult ŒŽ‚ŽŒ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################
$(PREPARE_SEASON_SCD ž’ €Œ‘ • 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD ž’ ‘Ž— ‹• 100 -1 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +267))
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
#   ’¥áâ ü12
#   à®¢¥àª   àå¨¢ æ¨¨ â ¡«¨æ ¤«ï âà¥âì¥£® è £   àå¨¢ æ¨¨ TArxTlgTrips
#
###
#########################################################################################


$(init_jxt_pult ŒŽ‚ŽŒ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################
$(PREPARE_SEASON_SCD ž’ €Œ‘ • 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD ž’ ‘Ž— ‹• 100 -1 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +267))
$(make_spp $(ddmmyy +265))

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")

$(run_arch_step $(ddmmyy +387) 3)


%%
#########################################################################################
###
#   ’¥áâ ü13
#   à®¢¥àª   àå¨¢ æ¨¨ â ¡«¨æ ¤«ï ç¥â¢¥àâ®£® è £   àå¨¢ æ¨¨ TArxTypeBIn
#
###
#########################################################################################


$(init_jxt_pult ŒŽ‚ŽŒ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################
$(PREPARE_SEASON_SCD ž’ €Œ‘ • 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD ž’ ‘Ž— ‹• 100 -1 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +267))
$(make_spp $(ddmmyy +265))

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")
$(dump_table TLGS_IN)

$(run_arch_step $(ddmmyy +387) 4)


%%
#########################################################################################
###
#   ’¥áâ ü14
#   à®¢¥àª   àå¨¢ æ¨¨ â ¡«¨æ ¤«ï ¯ïâ®£® è £   àå¨¢ æ¨¨ TArxNormsRatesEtc
#   arx_bag_norms, arx_bag_rates, arx_value_bag_taxes, arx_exchange_rates
#
###
#########################################################################################


$(init_jxt_pult ŒŽ‚ŽŒ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################
$(PREPARE_SEASON_SCD ž’ €Œ‘ • 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD ž’ ‘Ž— ‹• 100 -1 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +267))
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
#   ’¥áâ ü15
#   à®¢¥àª   àå¨¢ æ¨¨ â ¡«¨æ ¤«ï è¥áâ®£® è £   àå¨¢ æ¨¨ TArxTlgsFilesEtc
#   arx_tlgs, arx_tlg_stat, files, kiosk_events, rozysk, aodb_spp_files, eticks_display,
#   eticks_display_tlgs, emdocs_display
#
###
#########################################################################################


$(init_jxt_pult ŒŽ‚ŽŒ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################
$(PREPARE_SEASON_SCD ž’ €Œ‘ • 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD ž’ ‘Ž— ‹• 100 -1 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +267))
$(make_spp $(ddmmyy +265))


$(run_arch_step $(ddmmyy +387) 6)

$(are_tables_equal ARX_TLG_STAT)


%%
#########################################################################################
###
#   ’¥áâ ü16
#   à®¢¥àª   àå¨¢ æ¨¨ SELF_CKIN_STAT
#
###
#########################################################################################

$(init_term)

$(PREPARE_SEASON_SCD ž’ ‘Ž— ‹• 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive ž’ 100 ‘Ž—)
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))

$(init_kiosk)

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100) ž’ 100 ‘Ž— ‹• TUMALI VALERII 2986145115578 ‚‡ UA FA144642 UA 16.04.1968 25.06.2025 M KIOSK2)

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")

$(collect_flight_stat $(get point_dep_UT_100))

$(run_arch_step $(ddmmyy +141))
$(are_tables_equal ARX_SELF_CKIN_STAT)


%%
#########################################################################################
###
#   ’¥áâ ü17
#   à®¢¥àª   àå¨¢ æ¨¨ ¢á¥å â ¡«¨æ ¯®á«¥ à ¡®âë arx_daily
#
###
#########################################################################################


$(init_jxt_pult ŒŽ‚ŽŒ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

#$(PREPARE_SEASON_SCD ž’ €Œ‘ • 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
#$(make_spp $(ddmmyy +1))
#$(PREPARE_SEASON_SCD ž’ ‘Ž— ‹• 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30) 30.12.1899 31.12.1899)
#$(make_spp $(ddmmyy +20))


$(PREPARE_SEASON_SCD ž’ ‘Ž— • 298)
$(PREPARE_SEASON_SCD ž’ • €Œ‘ 190)
$(PREPARE_SEASON_SCD ž’ €Œ‘ ‹• 450)

$(make_spp)

$(deny_ets_interactive ž’ 298 ‘Ž—)
$(deny_ets_interactive ž’ 190 •)
$(deny_ets_interactive ž’ 450 €Œ‘)

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
$(CHECKIN_PAX_TRANSFER $(get pax_id1) $(get point_dep_UT_298) $(get point_arv_UT_298) ž’ 298 ‘Ž— • OZ OFER 2985523437721
                       $(get pax_id2) $(get point_dep_UT_190) $(get point_arv_UT_190) ž’ 190 • €Œ‘ OZ OFER
                       $(get pax_id3) $(get point_dep_UT_450) $(get point_arv_UT_450) ž’ 450 €Œ‘ ‹• OZ OFER
                       ‚‡ UA 32427293 UA 16.04.1968 25.06.2025 M)


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
#   ’¥áâ ü18
#   à®¢¥àª  ®¡à ¡®âª¨ ¨áª«îç¥­¨ï DUP_VAL_ON_INDEX
#
###
#########################################################################################


$(init_jxt_pult ŒŽ‚ŽŒ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD ž’ ‘Ž— • 298)
$(PREPARE_SEASON_SCD ž’ ‘Ž— • 300)

$(make_spp)

$(deny_ets_interactive ž’ 298 ‘Ž—)
$(deny_ets_interactive ž’ 300 ‘Ž—)

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
$(CHECKIN_PAX $(get pax_id1) $(get point_dep_UT_298) $(get point_arv_UT_298) ž’ 298 ‘Ž— • ALIMOV TALGAT 2982425696898 ‚‡ KZ N11024936 KZ 11.05.1996 04.10.2026 M)


!!
$(CHECKIN_PAX $(get pax_id2) $(get point_dep_UT_300) $(get point_arv_UT_300) ž’ 300 ‘Ž— • OZ OFER 2985523437721 ‚‡ UA 32427293 UA 16.04.1968 25.06.2025 M)


$(dump_table POINTS)
$(dump_table MARK_TRIPS)
$(dump_table PAX_GRP)


$(run_arch_step $(ddmmyy +141))

$(are_tables_equal ARX_MARK_TRIPS)
$(are_tables_equal ARX_PAX_GRP)

%%
#########################################################################################
###
#   ’¥áâ ü19
#   à®¢¥àª  ã¤ «¥­¨ï áâà®ª ¢ 6 è £¥ TArxTlgsFilesEtc
#
###
#########################################################################################

$(init_jxt_pult ŒŽ‚ŽŒ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(set first_date "$(date_format %y%m%d +0)")

$(db_sql TLGS "INSERT INTO TLGS(id, receiver, sender, time, tlg_num, type)  VALUES(1, 'ab', 'AC', TO_DATE('$(date_format %Y-%m-%d)', 'yyyy-mm-dd'), 1, 'A')")
$(db_sql TLGS "INSERT INTO TLGS(id, receiver, sender, time, tlg_num, type)  VALUES(2, 'ab', 'AC', TO_DATE('$(date_format %Y-%m-%d)', 'yyyy-mm-dd'), 2, 'B')")
$(db_sql TLGS "INSERT INTO TLGS(id, receiver, sender, time, tlg_num, type)  VALUES(3, 'ab', 'AC', TO_DATE('$(date_format %Y-%m-%d -130)', 'yyyy-mm-dd'), 3, 'C')")
$(db_sql TLGS "INSERT INTO TLGS(id, receiver, sender, time, tlg_num, type)  VALUES(4, 'ab', 'AC', TO_DATE('$(date_format %Y-%m-%d -130)', 'yyyy-mm-dd'), 4, 'D')")

$(db_sql aodb_spp_files "INSERT INTO aodb_spp_files(filename, point_addr, rec_no, airline)  VALUES('SPP$(get first_date).txt', 'RASTRV', -1, 'AR')")
$(db_sql aodb_spp_files "INSERT INTO aodb_spp_files(filename, point_addr, rec_no, airline)  VALUES('SPP200618.txt', 'RASTRV', -1, 'AR')")
$(db_sql aodb_spp_files "INSERT INTO aodb_spp_files(filename, point_addr, rec_no, airline)  VALUES('SPP$(get first_date).txt', 'SINSVO', 349, 'AD')")
$(db_sql aodb_spp_files "INSERT INTO aodb_spp_files(filename, point_addr, rec_no, airline)  VALUES('SPP210218.txt', 'SINSVO', 349, 'AD')")

$(db_sql FILES "INSERT INTO FILES(id, receiver, sender, time, type)  VALUES(1, 'AB', 'BC', '$(date_format %Y-%m-%d)', '000')")
$(db_sql FILES "INSERT INTO FILES(id, receiver, sender, time, type)  VALUES(2, 'AB', 'BC', '$(date_format %Y-%m-%d)', '000')")
$(db_sql FILES "INSERT INTO FILES(id, receiver, sender, time, type)  VALUES(3, 'AB', 'BC', '$(date_format %Y-%m-%d -130)', '000')")
$(db_sql FILES "INSERT INTO FILES(id, receiver, sender, time, type)  VALUES(4, 'AB', 'BC', '$(date_format %Y-%m-%d -130)' , '000')")

$(db_sql KIOSK_EVENTS "INSERT INTO KIOSK_EVENTS(id, ev_order, time)  VALUES(1, 1, TO_DATE('$(date_format %Y-%m-%d)', 'yyyy-mm-dd'))")
$(db_sql KIOSK_EVENTS "INSERT INTO KIOSK_EVENTS(id, ev_order, time)  VALUES(2, 5, TO_DATE('$(date_format %Y-%m-%d)', 'yyyy-mm-dd'))")
$(db_sql KIOSK_EVENTS "INSERT INTO KIOSK_EVENTS(id, ev_order, time)  VALUES(3, 8, TO_DATE('$(date_format %Y-%m-%d -130)', 'yyyy-mm-dd'))")
$(db_sql KIOSK_EVENTS "INSERT INTO KIOSK_EVENTS(id, ev_order, time)  VALUES(4, 9, TO_DATE('$(date_format %Y-%m-%d -130)', 'yyyy-mm-dd'))")

$(db_sql ETICKS_DISPLAY "INSERT INTO ETICKS_DISPLAY(coupon_no, fare_basis, issue_date, last_display, surname, ticket_no)  VALUES(1, 'AB', TO_DATE('$(date_format %Y-%m-%d)', 'yyyy-mm-dd'),  '$(date_format %Y-%m-%d)',      'BC', '000')")
$(db_sql ETICKS_DISPLAY "INSERT INTO ETICKS_DISPLAY(coupon_no, fare_basis, issue_date, last_display, surname, ticket_no)  VALUES(2, 'AB', TO_DATE('$(date_format %Y-%m-%d)', 'yyyy-mm-dd'),  '$(date_format %Y-%m-%d)',      'BG', '001')")
$(db_sql ETICKS_DISPLAY "INSERT INTO ETICKS_DISPLAY(coupon_no, fare_basis, issue_date, last_display, surname, ticket_no)  VALUES(3, 'AB', TO_DATE('$(date_format %Y-%m-%d)', 'yyyy-mm-dd'),  '$(date_format %Y-%m-%d -130)', 'BH', '002')")
$(db_sql ETICKS_DISPLAY "INSERT INTO ETICKS_DISPLAY(coupon_no, fare_basis, issue_date, last_display, surname, ticket_no)  VALUES(4, 'AB', TO_DATE('$(date_format %Y-%m-%d)', 'yyyy-mm-dd'),  '$(date_format %Y-%m-%d -130)' ,'BK', '003')")

$(db_sql ETICKS_DISPLAY_TLGS "INSERT INTO ETICKS_DISPLAY_TLGS(coupon_no, last_display, page_no, ticket_no, tlg_text, tlg_type)  VALUES(1, TO_DATE('$(date_format %Y-%m-%d)','yyyy-mm-dd'),      10, 'AB', 'DR', 0)")
$(db_sql ETICKS_DISPLAY_TLGS "INSERT INTO ETICKS_DISPLAY_TLGS(coupon_no, last_display, page_no, ticket_no, tlg_text, tlg_type)  VALUES(2, TO_DATE('$(date_format %Y-%m-%d)', 'yyyy-mm-dd'),     11, 'AC', 'DE', 2)")
$(db_sql ETICKS_DISPLAY_TLGS "INSERT INTO ETICKS_DISPLAY_TLGS(coupon_no, last_display, page_no, ticket_no, tlg_text, tlg_type)  VALUES(3, TO_DATE('$(date_format %Y-%m-%d -130)', 'yyyy-mm-dd'),12, 'AD', 'DH', 3)")
$(db_sql ETICKS_DISPLAY_TLGS "INSERT INTO ETICKS_DISPLAY_TLGS(coupon_no, last_display, page_no, ticket_no, tlg_text, tlg_type)  VALUES(4, TO_DATE('$(date_format %Y-%m-%d -130)', 'yyyy-mm-dd'),13, 'AE', 'DN', 9)")


#  ¤ âã  àå¨¢ æ¨¨ ç¥à¥§ 133 ¤­ï ®â â¥ªãé¥© ¤ âë á®åà ­ïâáï à¥©áë , ª®â®àë¥ ¡ë«¨ ç¥à¥§ 12 ¤­¥©, ®â â¥ªãé¥© ¤ âë
$(run_arch_step $(ddmmyy +0) 6)

#‚ â ¡«¨æ¥ ¤®«¦­ë ®áâ âìáï â®«ìª® 2 áâà®ª¨ ª®â®àë¥ ­¥ ¯®¯ «¨ ¯®¤  àå¨¢ æ¨î, á ¨¬¥­ ¬¨ ä ©«®¢ ®â â¥ªãé¥© ¤ âë

??
$(db_dump_table TLGS fields="id, receiver, sender, time, tlg_num, type" display="on")
>> lines=auto
[1] [ab] [AC] [$(date_format %Y-%m-%d) 00:00:00] [1] [A] $()
[2] [ab] [AC] [$(date_format %Y-%m-%d) 00:00:00] [2] [B] $()

??
$(db_dump_table FILES display="on")
>> lines=auto
[NULL] [NULL] [1] [AB] [BC] [$(date_format %Y-%m-%d) 00:00:00] [000] $()
[NULL] [NULL] [2] [AB] [BC] [$(date_format %Y-%m-%d) 00:00:00] [000] $()

??
$(db_dump_table KIOSK_EVENTS display="on")
>> lines=auto
[NULL] [1] [1] [NULL] [NULL] [NULL] [$(date_format %y%m%d)] [NULL] $()
[NULL] [5] [2] [NULL] [NULL] [NULL] [$(date_format %y%m%d)] [NULL] $()

??
$(db_dump_table ETICKS_DISPLAY fields="coupon_no, ticket_no, last_display" display="on")
>> lines=auto
[1] [000] [$(date_format %Y-%m-%d) 00:00:00] $()
[2] [001] [$(date_format %Y-%m-%d) 00:00:00] $()

??
$(db_dump_table ETICKS_DISPLAY_TLGS fields="coupon_no, ticket_no, last_display, tlg_type, tlg_text" display="on")
>> lines=auto
[1] [AB] [$(date_format %Y-%m-%d) 00:00:00] [0] [DR] $()
[2] [AC] [$(date_format %Y-%m-%d) 00:00:00] [2] [DE] $()

??
$(db_dump_table AODB_SPP_FILES display="on")
>> lines=auto
[AR] [SPP$(get first_date).txt] [RASTRV] [-1] $()
[AD] [SPP$(get first_date).txt] [SINSVO] [349] $()

