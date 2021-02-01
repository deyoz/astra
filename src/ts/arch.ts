include(ts/macro.ts)
include(ts/adm_macro.ts)

# meta: suite arch

#########################################################################################
###
#   ���� �1
#
#
###
#########################################################################################

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

#$(PREPARE_SEASON_SCD �� ��� ��� 200 -1 TU5 $(date_format %d.%m.%Y -12) $(date_format %d.%m.%Y +12))

$(PREPARE_SEASON_SCD �� ��� ��� 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +15))
$(make_spp $(ddmmyy +13))

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")
#$(dump_pg_table ARX_POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, part_key, airp_fmt")

#��१ 121 ��� �� ⥪�饩 ����, ��祣� �� ��娢������
$(run_arch_step $(ddmmyy +121))

#�� ���� ��娢� �१ 122 ��� �� ⥪�饩 ����, ��娢������ 1 ������ , ����� �१ 1 ���� �� ⥪�饩 ����
$(run_arch_step $(ddmmyy +122))

#$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")
#$(dump_table ARX_POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, part_key, airp_fmt")
#$(dump_pg_table ARX_POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, part_key, airp_fmt")

#�� ���� ��娢�樨 �१ 133 ��� �� ⥪�饩 ���� ��࠭���� ३�� , ����� �뫨 �१ 12 ����, �� ⥪�饩 ����
$(run_arch_step $(ddmmyy +133))

#$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")
#$(dump_table ARX_POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, part_key, airp_fmt")
#$(dump_pg_table ARX_POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, part_key, airp_fmt")

$(are_tables_equal ARX_POINTS)

%%
#########################################################################################
###
#   ���� �2
#   �஢�ઠ ��娢�樨 MOVE_ARX_EXT � ARX_MOVE_REF
#
###
#########################################################################################


$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

#$(PREPARE_SEASON_SCD �� ��� ��� 200 -1 TU5 $(date_format %d.%m.%Y -12) $(date_format %d.%m.%Y +12))

$(PREPARE_SEASON_SCD �� ��� ��� 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30) 30.12.1899 31.12.1899)
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
#   ���� �3
#   �஢�ઠ ��娢�樨 ARX_EVENTS
#
###
#########################################################################################


$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD �� ��� ��� 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +267))
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
#   ���� �4
#   �஢�ઠ ��娢�樨 ARX_MARK_TRIPS � ARX_PAX_GRP
#
###
#########################################################################################


$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD �� ��� ��� 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(deny_ets_interactive �� 300 ���)
$(make_spp $(ddmmyy +1))
$(INB_PNL_UT AMS PRG 300 $(ddmon +1 en))
$(set point_dep_UT_300 $(last_point_id_spp))
$(set point_arv_UT_300 $(get_next_trip_point_id $(get point_dep_UT_300)))


$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive �� 100 ���)
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))

$(set pax_id_ALIMOV $(get_pax_id $(get point_dep_UT_300) ALIMOV TALGAT))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_300) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id_ALIMOV) $(get point_dep_UT_300) $(get point_arv_UT_300) �� 300 ��� ��� ALIMOV TALGAT 2982425696898 �� KZ N11024936 KZ 11.05.1996 04.10.2026 M)

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_300) $(get point_arv_UT_100) �� 300 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")

#��娢������ ⮫쪮 ���-���
$(run_arch_step $(ddmmyy +122))
$(are_tables_equal ARX_MARK_TRIPS)

#��娢������ � ���-���
$(run_arch_step $(ddmmyy +141))

$(dump_table PAX_GRP)
$(dump_table MARK_TRIPS)

$(are_tables_equal ARX_MARK_TRIPS)
$(are_tables_equal ARX_PAX_GRP)


%%
#########################################################################################
###
#   ���� �5
#   �஢�ઠ ��娢�樨  STAT ⠡���
#
###
#########################################################################################

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

# $(PREPARE_SEASON_SCD �� ��� ��� 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
# $(deny_ets_interactive �� 300 ���)
# $(make_spp $(ddmmyy +1))
# $(INB_PNL_UT AMS PRG 300 $(ddmon +1 en))
# $(set point_dep_UT_300 $(last_point_id_spp))
# $(set point_arv_UT_300 $(get_next_trip_point_id $(get point_dep_UT_300)))
# $(set pax_id_ALIMOV $(get_pax_id $(get point_dep_UT_300) TUMALI VALERII))
#
# !!
# $(CHECKIN_PAX $(get pax_id_ALIMOV) $(get point_dep_UT_300) $(get point_arv_UT_300) �� 300 ��� ��� ALIMOV TALGAT 2982425696898 �� KZ N11024936 KZ 11.05.1996 04.10.2026 M KIOSK2)

$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive �� 100 ���)
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))

# �� �㦭� ��� ⮣� �⮡� pr_brd = 1 � ⠡��� PAX,� ���� ��ᠤ��� ���ᠦ��
# � �� � ᢮� ��।� �㦭� �⮡� ����������� ⠡��� STAT_AD � ��⮬ ARX_STAT_AD
$(sql {INSERT INTO trip_hall(point_id, type, hall, pr_misc)
       VALUES($(get point_dep_UT_100), 101, NULL, 1)})

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100) �� 100 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

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
#   ���� �6
#   �஢�ઠ ��娢�樨 TRIP ⠡���
#
###
#########################################################################################


$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive �� 100 ���)
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100) �� 100 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

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
#   ���� �7
#   �஢�ઠ ��娢�樨 BAG ⠡���
#
###
#########################################################################################


$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive �� 100 ���)
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100) �� 100 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

$(run_arch_step $(ddmmyy +141))
$(are_tables_equal ARX_BAG_RECEIPTS)
$(are_tables_equal ARX_BAG_PAY_TYPES)

%%
#########################################################################################
###
#   ���� �8
#   �஢�ઠ ��娢�樨 ⠡��� Loop by pax groups
#
###
#########################################################################################


$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive �� 100 ���)
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100) �� 100 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

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
#   ���� �9
#   �஢�ઠ ��娢�樨 ⠡��� Loop by groups
#
###
#########################################################################################


$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(PREPARE_SEASON_SCD �� ��� ��� 190)
$(PREPARE_SEASON_SCD �� ��� ��� 450)

$(make_spp)

$(deny_ets_interactive �� 298 ���)
$(deny_ets_interactive �� 190 ���)
$(deny_ets_interactive �� 450 ���)

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
$(CHECKIN_PAX_TRANSFER $(get pax_id1) $(get point_dep_UT_298) $(get point_arv_UT_298) �� 298 ��� ��� OZ OFER 2985523437721
                       $(get pax_id2) $(get point_dep_UT_190) $(get point_arv_UT_190) �� 190 ��� ��� OZ OFER
                       $(get pax_id3) $(get point_dep_UT_450) $(get point_arv_UT_450) �� 450 ��� ��� OZ OFER
                       �� UA 32427293 UA 16.04.1968 25.06.2025 M)


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
#   ���� �10
#   �஢�ઠ ��娢�樨 ⠡��� Loop by pax_id
#
###
#########################################################################################


$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive �� 100 ���)
$(make_spp $(ddmmyy +22))
$(INB_PNL_UT AER LHR 100 $(ddmon +22 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100) �� 100 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

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
#   ���� �11
#   �஢�ઠ ��娢�樨 ⠡��� ��� ��ண� 蠣� ��娢�樨
#   arx_events, arx_tlg_out, arx_stat_zamar
#
###
#########################################################################################


$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################
$(PREPARE_SEASON_SCD �� ��� ��� 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +267))
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
#   ���� �12
#   �஢�ઠ ��娢�樨 ⠡��� ��� ���쥣� 蠣� ��娢�樨 TArxTlgTrips
#
###
#########################################################################################


$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################
$(PREPARE_SEASON_SCD �� ��� ��� 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +267))
$(make_spp $(ddmmyy +265))

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")

$(run_arch_step $(ddmmyy +387) 3)


%%
#########################################################################################
###
#   ���� �13
#   �஢�ઠ ��娢�樨 ⠡��� ��� �⢥�⮣� 蠣� ��娢�樨 TArxTypeBIn
#
###
#########################################################################################


$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################
$(PREPARE_SEASON_SCD �� ��� ��� 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +267))
$(make_spp $(ddmmyy +265))

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")
$(dump_table TLGS_IN)

$(run_arch_step $(ddmmyy +387) 4)


%%
#########################################################################################
###
#   ���� �14
#   �஢�ઠ ��娢�樨 ⠡��� ��� ��⮣� 蠣� ��娢�樨 TArxNormsRatesEtc
#   arx_bag_norms, arx_bag_rates, arx_value_bag_taxes, arx_exchange_rates
#
###
#########################################################################################


$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################
$(PREPARE_SEASON_SCD �� ��� ��� 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +267))
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
#   ���� �15
#   �஢�ઠ ��娢�樨 ⠡��� ��� ��⮣� 蠣� ��娢�樨 TArxTlgsFilesEtc
#   arx_tlgs, arx_tlg_stat, files, kiosk_events, rozysk, aodb_spp_files, eticks_display,
#   eticks_display_tlgs, emdocs_display
#
###
#########################################################################################


$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################
$(PREPARE_SEASON_SCD �� ��� ��� 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +267))
$(make_spp $(ddmmyy +265))


$(run_arch_step $(ddmmyy +387) 6)

$(are_tables_equal ARX_TLG_STAT)


%%
#########################################################################################
###
#   ���� �16
#   �஢�ઠ ��娢�樨 SELF_CKIN_STAT
#
###
#########################################################################################

$(init_term)

$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive �� 100 ���)
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))

$(init_kiosk)

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100) �� 100 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M KIOSK2)

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")

$(collect_flight_stat $(get point_dep_UT_100))

$(run_arch_step $(ddmmyy +141))
$(are_tables_equal ARX_SELF_CKIN_STAT)


%%
#########################################################################################
###
#   ���� �17
#   �஢�ઠ ��娢�樨 ��� ⠡��� ��᫥ ࠡ��� arx_daily
#
###
#########################################################################################


$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

#$(PREPARE_SEASON_SCD �� ��� ��� 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
#$(make_spp $(ddmmyy +1))
#$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30) 30.12.1899 31.12.1899)
#$(make_spp $(ddmmyy +20))


$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(PREPARE_SEASON_SCD �� ��� ��� 190)
$(PREPARE_SEASON_SCD �� ��� ��� 450)

$(make_spp)

$(deny_ets_interactive �� 298 ���)
$(deny_ets_interactive �� 190 ���)
$(deny_ets_interactive �� 450 ���)

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
$(CHECKIN_PAX_TRANSFER $(get pax_id1) $(get point_dep_UT_298) $(get point_arv_UT_298) �� 298 ��� ��� OZ OFER 2985523437721
                       $(get pax_id2) $(get point_dep_UT_190) $(get point_arv_UT_190) �� 190 ��� ��� OZ OFER
                       $(get pax_id3) $(get point_dep_UT_450) $(get point_arv_UT_450) �� 450 ��� ��� OZ OFER
                       �� UA 32427293 UA 16.04.1968 25.06.2025 M)


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
#   ���� �18
#   �஢�ઠ ��ࠡ�⪨ �᪫�祭�� DUP_VAL_ON_INDEX
#
###
#########################################################################################


$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(PREPARE_SEASON_SCD �� ��� ��� 300)

$(make_spp)

$(deny_ets_interactive �� 298 ���)
$(deny_ets_interactive �� 300 ���)

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
$(CHECKIN_PAX $(get pax_id1) $(get point_dep_UT_298) $(get point_arv_UT_298) �� 298 ��� ��� ALIMOV TALGAT 2982425696898 �� KZ N11024936 KZ 11.05.1996 04.10.2026 M)


!!
$(CHECKIN_PAX $(get pax_id2) $(get point_dep_UT_300) $(get point_arv_UT_300) �� 300 ��� ��� OZ OFER 2985523437721 �� UA 32427293 UA 16.04.1968 25.06.2025 M)


$(dump_table POINTS)
$(dump_table MARK_TRIPS)
$(dump_table PAX_GRP)


$(run_arch_step $(ddmmyy +141))

$(are_tables_equal ARX_MARK_TRIPS)
$(are_tables_equal ARX_PAX_GRP)
