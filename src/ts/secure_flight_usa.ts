include(ts/macro.ts)

# meta: suite apis


###
#   ���ᠭ��:
#
###
#########################################################################################

$(desc_test 1)

$(init)
$(init_jxt_pult ������)
$(login)

$(PREPARE_SEASON_SCD �� ��� JFK 245)
$(make_spp)
$(deny_ets_interactive �� 245 ���)

$(INB_PNL_UT SVO JFK 245 $(ddmon +0 en))

$(set point_dep $(last_point_id_tlg))

??
$(get_crs_pax_unique_ref $(get point_dep) STIPIDI ANGELINA)
>>
2222


??
$(get_crs_pax_unique_ref $(get point_dep) AKOPOVA OLIVIIA)
>>
3333
