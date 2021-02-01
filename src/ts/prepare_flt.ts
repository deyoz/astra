include(ts/macro.ts)
include(ts/adm_macro.ts)

# ” ©« § ―γαª ¥βαο Ά bin/createdb.sh
# ­¥ γη αβΆγ¥β Ά β¥αβ ε

$(init_term 201509-0173355)

#############################################

$(PREPARE_SEASON_SCD ’ ‚ ‘“ 245)
$(make_spp)
$(deny_ets_interactive ’ 245 ‚)

$(INB_PNL_UT VKO SGC 245 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
# $(set move_id $(get_move_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

#############################################

$(PREPARE_SEASON_SCD ’ ‘— ‚ 250)
$(make_spp)
$(deny_ets_interactive ’ 250 ‘—)

$(INB_PNL_UT AER VKO 250 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
# $(set move_id $(get_move_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

#############################################

$(PREPARE_SEASON_SCD ’ ‘— ‚ 580)
$(make_spp)
$(deny_ets_interactive ’ 580 ‘—)

$(INB_PNL_UT AER VKO 580 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
# $(set move_id $(get_move_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

#############################################

$(sql "commit")
