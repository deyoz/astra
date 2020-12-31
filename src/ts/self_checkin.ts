include(ts/macro.ts)
include(ts/adm_macro.ts)


# meta: suite checkin


###
#   Ž¯¨á ­¨¥: á ¬®à¥£¨áâà æ¨ï
#
###
#########################################################################################

$(desc_test 1)

$(init_term)

$(PREPARE_SEASON_SCD ž’ ‘Ž— • 298)
$(make_spp)
$(deny_ets_interactive ž’ 298 ‘Ž—)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set move_id $(get_move_id $(get point_dep)))
$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

$(init_kiosk)

!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) ž’ 298 ‘Ž— • TUMALI VALERII 2986145115578 ‚‡ UA FA144642 UA 16.04.1968 25.06.2025 M KIOSK2)

$(collect_flight_stat $(get point_dep))

??
$(dump_table SELF_CKIN_STAT fields="ADULT, CLIENT_TYPE,DESK" display=on)

>> lines=auto
[1] [KIOSK] [KIOSK2] $()
