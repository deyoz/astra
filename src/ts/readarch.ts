include(ts/macro.ts)
include(ts/adm_macro.ts)
include(ts/fr_forms.ts)
include(ts/spp/read_trips_macro.ts)
include(ts/pax/checkin_macro.ts)

# meta: suite readarch

#########################################################################################
###
#   „įā ü1
#   ā„­Ø„  ąåØ¢  Ø§ passenger.cc Ø§ äć­ŖęØØ LoadPaxDoc ¢ė§ė¢ īé„©įļ Ø§
#   äć­ŖęØØ RunTrferPaxStat Ø§ įā āØįāØŖØ stat_trfer_pax.cc
#
###
#########################################################################################

$(init_jxt_pult )
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD    298)
$(PREPARE_SEASON_SCD    190)
$(PREPARE_SEASON_SCD    450)

$(make_spp)

$(deny_ets_interactive  298 )
$(deny_ets_interactive  190 )
$(deny_ets_interactive  450 )

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
$(CHECKIN_PAX_TRANSFER $(get pax_id1) $(get point_dep_UT_298) $(get point_arv_UT_298)  298   OZ OFER 2985523437721
                       $(get pax_id2) $(get point_dep_UT_190) $(get point_arv_UT_190)  190   OZ OFER
                       $(get pax_id3) $(get point_dep_UT_450) $(get point_arv_UT_450)  450   OZ OFER
                        UA 32427293 UA 16.04.1968 25.06.2025 M)

$(run_arch_step $(ddmmyy +151))


!! capture=on
$(RUN_TRFER_PAX_STAT $(date_format %d.%m.%Y -160) $(date_format %d.%m.%Y +21))

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...>
    <airline></airline>
    <grd>
      <header>
        <col width='60' align='0' sort='0'></col>
        <col width='30' align='0' sort='0'></col>
        <col width='50' align='0' sort='0'>„£.1</col>
        <col width='55' align='0' sort='0'> ā </col>
        <col width='30' align='0' sort='0'></col>
        <col width='60' align='0' sort='0'>„£.2</col>
        <col width='55' align='0' sort='0'> ā </col>
        <col width='30' align='0' sort='0'></col>
        <col width='60' align='0' sort='0'> ā„£®ąØļ</col>
        <col width='60' align='0' sort='0'> Æ įį ¦Øą </col>
        <col width='70' align='0' sort='0'>®Ŗć¬„­ā</col>
        <col width='60' align='0' sort='0'></col>
        <col width='60' align='0' sort='0'></col>
        <col width='60' align='0' sort='0'></col>
        <col width='60' align='0' sort='0'></col>
        <col width='60' align='0' sort='0'>/Ŗ</col>
        <col width='60' align='0' sort='0'> ¬„įā</col>
        <col width='60' align='0' sort='0'> ¢„į</col>
        <col width='90' align='0' sort='0'>ØąŖØ</col>
      </header>
      <rows>
        <row>
          <col></col>
          <col></col>
          <col>298</col>
          <col>$(date_format %d.%m.%y)</col>
          <col></col>
          <col>190</col>
          <col>$(date_format %d.%m.%y)</col>
          <col></col>
          <col>-</col>
          <col>OZ OFER</col>
          <col>32427293</col>
          <col>1</col>
          <col>1</col>
          <col>0</col>
          <col>0</col>
          <col>0</col>
          <col>0</col>
          <col>0</col>
          <col/>
        </row>
        <row>
          <col></col>
          <col></col>
          <col>190</col>
          <col>$(date_format %d.%m.%y)</col>
          <col></col>
          <col>450</col>
          <col>$(date_format %d.%m.%y)</col>
          <col></col>
          <col>-</col>
          <col>OZ OFER</col>
          <col>32427293</col>
          <col>0</col>
          <col>0</col>
          <col>0</col>
          <col>0</col>
          <col>0</col>
          <col>0</col>
          <col>0</col>
          <col/>
        </row>
        <row>
          <col>ā®£®:</col>
          <col/>
          <col/>
          <col/>
          <col/>
          <col/>
          <col/>
          <col/>
          <col/>
          <col/>
          <col/>
          <col>1</col>
          <col>1</col>
          <col>0</col>
          <col>0</col>
          <col>0</col>
          <col>0</col>
          <col>0</col>
          <col/>
        </row>
      </rows>
    </grd>
    <form_data>
      <variables>
        <print_date>... ()</print_date>
        <print_oper>PIKE</print_oper>
        <print_term></print_term>
        <use_seances>0</use_seances>
        <test_server>1</test_server>
        <cap_test></cap_test>
        <page_number_fmt>āą. %u Ø§ %u</page_number_fmt>
        <short_page_number_fmt>āą. %u</short_page_number_fmt>
        <oper_info>āē„ā įä®ą¬Øą®¢ ­ ... ()
®Æ„ą ā®ą®¬ PIKE
į ā„ą¬Ø­ «  </oper_info>
        <skip_header>0</skip_header>
        <stat_type>21</stat_type>
        <stat_mode>ą ­įä„ą</stat_mode>
        <stat_type_caption>®¤ą®”­ ļ</stat_type_caption>
      </variables>
    </form_data>
  </answer>
</term>

%%
#########################################################################################

###
#   „įā ü2
#
#   ÆØį ­Ø„: Æ įį ¦Øą®¢: 61,
#             Ø­ā„ą ŖāØ¢: ¢ėŖ«
#
#   ā„­Ø„  ąåØ¢  Ø§ events.cc äć­ŖęØØ GetEvents
###
#########################################################################################

$(init_jxt_pult )
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD    300 1003 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD    100 1004 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +267))
$(make_spp $(ddmmyy +265))

$(set point_dep $(get_dep_point_id   300 $(yymmdd +1)))

$(run_arch_step $(ddmmyy +387))

#!! capture=on
#$(GET_EVENTS $(get point_dep))

!! capture=on
$(GET_ARX_EVENTS $(get point_dep) $(date_format %d.%m.%Y +1))

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...>
    <events_log>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user> ..</ev_user>
        <station></station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>  ą„©į„ § Æą„é„­  web-ą„£Øįāą ęØļ.</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user> ..</ev_user>
        <station></station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>ā Æ '®¤£®ā®¢Ŗ  Ŗ ą„£Øįāą ęØØ': Æ« ­. ¢ą„¬ļ 00:15 $(date_format %d.%m.%y +1) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user> ..</ev_user>
        <station></station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>ā Æ 'āŖąėāØ„ ą„£Øįāą ęØØ': Æ« ­. ¢ą„¬ļ 04:14 $(date_format %d.%m.%y +1) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user> ..</ev_user>
        <station></station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>ā Æ 'āŖąėāØ„ web-ą„£Øįāą ęØØ': Æ« ­. ¢ą„¬ļ 07:15 $(date_format %d.%m.%y) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user> ..</ev_user>
        <station></station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>ā Æ 'āŖąėāØ„ kiosk-ą„£Øįāą ęØØ': Æ« ­. ¢ą„¬ļ 07:15 $(date_format %d.%m.%y) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user> ..</ev_user>
        <station></station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>ā Æ ' ŖąėāØ„ ą„£Øįāą ęØØ': Æ« ­. ¢ą„¬ļ 06:35 $(date_format %d.%m.%y +1) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user> ..</ev_user>
        <station></station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>ā Æ ' Æą„ā ®ā¬„­ė web-ą„£Øįāą ęØØ': Æ« ­. ¢ą„¬ļ 06:25 $(date_format %d.%m.%y +1) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user> ..</ev_user>
        <station></station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>ā Æ ' ŖąėāØ„ web-ą„£Øįāą ęØØ': Æ« ­. ¢ą„¬ļ 04:15 $(date_format %d.%m.%y +1) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user> ..</ev_user>
        <station></station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>ā Æ ' ŖąėāØ„ kiosk-ą„£Øįāą ęØØ': Æ« ­. ¢ą„¬ļ 05:15 $(date_format %d.%m.%y +1) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user> ..</ev_user>
        <station></station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>ā Æ '®ā®¢­®įāģ  Ŗ Æ®į ¤Ŗ„': Æ« ­. ¢ą„¬ļ 06:30 $(date_format %d.%m.%y +1) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user> ..</ev_user>
        <station></station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>ā Æ 'Ŗ®­ē ­Ø„ Æ®į ¤ŖØ (®ä®ą¬«„­Ø„ ¤®Ŗć¬.)': Æ« ­. ¢ą„¬ļ 06:50 $(date_format %d.%m.%y +1) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user> ..</ev_user>
        <station></station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>ā Æ 'āŖ ā āą Æ ': Æ« ­. ¢ą„¬ļ 07:00 $(date_format %d.%m.%y +1) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user> ..</ev_user>
        <station></station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg> §­ ē„­Ø„ ¢„į®¢ Æ įį ¦Øą®¢ ­  ą„©į: </msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user> ..</ev_user>
        <station></station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>”®ą įā āØįāØŖØ Æ® ą„©įć</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user> ..</ev_user>
        <station></station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>„©į Æ„ą„¬„é„­ ¢  ąåØ¢</msg>
        <ev_order>...</ev_order>
      </row>
    </events_log>
    <form_data>
      <variables>
        <trip>300</trip>
        <scd_out>01.12.0002</scd_out>
        <real_out>01.12.0002</real_out>
        <scd_date>01.12</scd_date>
        <date_issue>$(date_format %d.%m.%y) xx:xx</date_issue>
        <day_issue>$(date_format %d.%m.%y)</day_issue>
        <lang>RU</lang>
        <own_airp_name> </own_airp_name>
        <own_airp_name_lat>AMSTERDAM AIRPORT</own_airp_name_lat>
        <airp_dep_name></airp_dep_name>
        <airp_dep_city></airp_dep_city>
        <airline_name>  </airline_name>
        <flt>300</flt>
        <bort/>
        <craft>5</craft>
        <park/>
        <scd_time>xxxxx</scd_time>
        <long_route>()-()</long_route>
        <test_server>1</test_server>
        <page_number_fmt>āą. %u Ø§ %u</page_number_fmt>
        <landscape>0</landscape>
        <caption>ćą­ « ®Æ„ą ęØ© Æ® 300/01.12.0002 §  $(date_format %d.%m.%y)</caption>
        <cap_test></cap_test>
        <short_page_number_fmt>āą. %u</short_page_number_fmt>
      </variables>
    </form_data>
    <form name='EventsLog'...>$(EventsLogForm)
</form>
  </answer>
</term>


%%
#########################################################################################

###
#   „įā ü3
#   ā„­Ø„  ąåØ¢  Ø§ stat_arx.cc ć­ŖęØļ ArxPaxListRun
#
###

$(init_jxt_pult )
$(set_desk_version 201707-0195750)
$(login)

$(PREPARE_SEASON_SCD    298)
$(PREPARE_SEASON_SCD    190)
$(PREPARE_SEASON_SCD    450)

$(make_spp)

$(deny_ets_interactive  298 )
$(deny_ets_interactive  190 )
$(deny_ets_interactive  450 )

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

$(NEW_UNACCOMP_REQUEST capture=off lang=EN hall=1 ""
{$(NEW_UNACCOMP_SEGMENT $(get point_dep_UT_298) $(get point_arv_UT_298)  )})

$(set grp_id_unacc1 $(get_unaccomp_id $(get point_dep_UT_298) 1))

$(CHANGE_UNACCOMP_REQUEST capture=off lang=EN hall=1
{$(CHANGE_UNACCOMP_SEGMENT $(get point_dep_UT_298) $(get point_arv_UT_298)  
                           $(get grp_id_unacc1) $(get_unaccomp_tid $(get grp_id_unacc1)))}
{<value_bags/>
<bags>
$(BAG_WT 1 ""  pr_cabin=1 amount=1  weight=11  bag_pool_num=1)
$(BAG_WT 2 ""  pr_cabin=0 amount=3  weight=24  bag_pool_num=1)
</bags>
<tags pr_print=\"0\">
$(TAG 1 RUCH 1298401555 bag_num=2 color=)
$(TAG 2 RUCH 1298401556 bag_num=2 color=)
$(TAG 3 RUCH 0298401557 bag_num=2 color=)
</tags>}
)


!!
$(CHECKIN_PAX_TRANSFER $(get pax_id1) $(get point_dep_UT_298) $(get point_arv_UT_298)  298   OZ OFER 2985523437721
                       $(get pax_id2) $(get point_dep_UT_190) $(get point_arv_UT_190)  190   OZ OFER
                       $(get pax_id3) $(get point_dep_UT_450) $(get point_arv_UT_450)  450   OZ OFER
                        UA 32427293 UA 16.04.1968 25.06.2025 M)

$(set grp_id $(get_single_grp_id $(get point_dep_UT_298) OZ OFER))


$(run_arch_step $(ddmmyy +151))



!! capture=on
$(ARX_PAX_LIST_RUN  $(get point_dep_UT_298) $(date_format %d.%m.%Y))

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...>
    <paxList>
      <rows>
        <pax>
          <part_key>$(date_format %d.%m.%Y) 09:00:00</part_key>
          <point_id>$(get point_dep_UT_298)</point_id>
          <airline></airline>
          <flt_no>298</flt_no>
          <suffix/>
          <trip>298 </trip>
          <scd_out>$(date_format %d.%m.%Y) 10:15:00</scd_out>
          <reg_no>1</reg_no>
          <full_name>OZ OFER</full_name>
          <bag_amount>0</bag_amount>
          <bag_weight>0</bag_weight>
          <rk_weight>0</rk_weight>
          <excess>0</excess>
          <tags/>
          <seat_no>05</seat_no>
          <grp_id>$(get grp_id)</grp_id>
          <airp_arv></airp_arv>
          <status> ą„£.</status>
          <class></class>
          <document>32427293 UKR</document>
          <ticket_no>2985523437721</ticket_no>
          <hall> « 1</hall>
        </pax>
        <pax>
          <part_key>$(date_format %d.%m.%Y) 09:00:00</part_key>
          <point_id>$(get point_dep_UT_298)</point_id>
          <airline/>
          <flt_no>0</flt_no>
          <suffix/>
          <trip>298 </trip>
          <scd_out>$(date_format %d.%m.%Y) 10:15:00</scd_out>
          <reg_no>0</reg_no>
          <full_name> £ ¦ ”„§ į®Æą®¢®¦¤„­Øļ</full_name>
          <bag_amount>3</bag_amount>
          <bag_weight>24</bag_weight>
          <rk_weight>11</rk_weight>
          <excess>35</excess>
          <tags>1298401556, 0298401557, 1298401555</tags>
          <grp_id>...</grp_id>
          <airp_arv></airp_arv>
          <status/>
          <class/>
          <seat_no/>
          <document/>
          <ticket_no/>
          <hall> « 1</hall>
        </pax>
      </rows>
      <header>
        <col>„©į</col>
      </header>
    </paxList>
    <form_data>
      <variables>
        <print_date>... ()</print_date>
        <print_oper>PIKE</print_oper>
        <print_term></print_term>
        <use_seances>0</use_seances>
        <test_server>1</test_server>
        <cap_test></cap_test>
        <page_number_fmt>āą. %u Ø§ %u</page_number_fmt>
        <short_page_number_fmt>āą. %u</short_page_number_fmt>
        <oper_info>āē„ā įä®ą¬Øą®¢ ­ ... ()
®Æ„ą ā®ą®¬ PIKE
į ā„ą¬Ø­ «  </oper_info>
        <skip_header>0</skip_header>
      </variables>
    </form_data>
  </answer>
</term>


%%
#########################################################################################

###
#   „įā ü4
#   ā„­Ø„  ąåØ¢  Ø§ arx_stat_ad.cc
#
######################################################

$(init_jxt_pult )
$(set_desk_version 201707-0195750)
$(login)

$(PREPARE_SEASON_SCD    100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive  100 )
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))

# ķā® ­ć¦­® ¤«ļ ā®£® ēā®”ė pr_brd = 1 ¢ ā ”«Øę„ PAX,ā® „įāģ Æ®į ¤Øāģ Æ įį ¦Øą 
#  ķā® ¢ į¢®ī ®ē„ą„¤ģ ­ć¦­® ēā®”ė § Æ®«­Ø« įģ ā ”«Øę  STAT_AD Ø Æ®ā®¬ ARX_STAT_AD
$(db_sql TRIP_HALL {INSERT INTO trip_hall(point_id, type, hall, pr_misc)
                    VALUES($(get point_dep_UT_100), 101, NULL, 1)})

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100)  100   TUMALI VALERII 2986145115578  UA FA144642 UA 16.04.1968 25.06.2025 M)

$(run_arch_step $(ddmmyy +141))

!! capture=on
$(RUN_ACTUAL_DEPARTURED_STAT $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21))

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...>
    <form name='stat'...>$(statForm)
</form>
    <airline></airline>
    <grd>
      <header>
        <col width='50' align='0' sort='0'></col>
        <col width='50' align='0' sort='0'></col>
        <col width='75' align='0' sort='0'>®¬„ą ą„©į </col>
        <col width='50' align='0' sort='3'> ā </col>
        <col width='50' align='0' sort='3'>PNR</col>
        <col width='150' align='0' sort='3'>...</col>
        <col width='50' align='0' sort='3'>ØÆ</col>
        <col width='50' align='0' sort='3'>« įį</col>
        <col width='50' align='0' sort='3'>ØÆ ą„£.</col>
        <col width='50' align='0' sort='3'> £ ¦</col>
        <col width='50' align='0' sort='3'>ėå. ­  Æ®į ¤Ŗć</col>
        <col width='50' align='0' sort='3'>ü ¬</col>
      </header>
      <rows>
        <row>
          <col></col>
          <col></col>
          <col>100</col>
          <col>$(date_format %d.%m.%y +20)</col>
          <col>F50CF0</col>
          <col>TUMALI VALERII</col>
          <col></col>
          <col></col>
          <col>TERM</col>
          <col/>
          <col></col>
          <col>5</col>
        </row>
        <row>
          <col>ā®£®:</col>
          <col/>
          <col/>
          <col/>
          <col/>
          <col>1</col>
          <col/>
          <col/>
          <col/>
          <col/>
          <col/>
          <col/>
        </row>
      </rows>
    </grd>
    <form_data>
      <variables>
        <print_date>... ()</print_date>
        <print_oper>PIKE</print_oper>
        <print_term></print_term>
        <use_seances>0</use_seances>
        <test_server>1</test_server>
        <cap_test></cap_test>
        <page_number_fmt>āą. %u Ø§ %u</page_number_fmt>
        <short_page_number_fmt>āą. %u</short_page_number_fmt>
        <oper_info>āē„ā įä®ą¬Øą®¢ ­ ... ()
®Æ„ą ā®ą®¬ PIKE
į ā„ą¬Ø­ «  </oper_info>
        <skip_header>0</skip_header>
        <stat_type>29</stat_type>
        <stat_mode> Ŗā. ¢ė«„ā</stat_mode>
        <stat_type_caption>®¤ą®”­ ļ</stat_type_caption>
      </variables>
    </form_data>
  </answer>
</term>


%%
#########################################################################################

###
#   „įā ü5
#   ā„­Ø„  ąåØ¢  Ø§ stat_services.cc
#
######################################################

include(ts/sirena_exchange_macro.ts)

$(init_term)

$(PREPARE_SEASON_SCD    100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive  100 )
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))

$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))

$(set pax_id_BURYAKOV $(get_pax_id $(get point_dep_UT_100)  " "))

$(CHANGE_TRIP_SETS $(get point_dep_UT_100) piece_concept=1)
$(settcl SIRENA_HOST localhost)
$(settcl SIRENA_PORT 8008)

!! capture=on
$(cache PIKE RU RFISC_SETS $(cache_iface_ver RFISC_SETS) ""
  insert airline:$(get_elem_id etAirline )
         rfic:A
         rfisc:0B5
         auto_checkin:1)

>> lines=auto
$(MESSAGE_TAG MSG.CHANGED_DATA_COMMIT)


$(http_forecast content=$(get_svc_availability_resp))


#1/ 
#.L/F451B5/UT
#.L/5C0NXZ/1H
#.R/TKNE HK1 2982425622093/1
#.R/SEAT HK1 10D
#.R/OTHS HK1 FQTSTATUS BRONZE
#.R/FQTV UT 1020894422
#.R/ASVC HI1 A/0B5/SEAT/  /A
#.RN//2984555892312C1
#.R/ASVC HI1 C/08A//  10 554025/A
#.RN//2984555892311C1
#.R/ASVC HI1 A/O6O//   /A/2984555892336C1
#.R/DOCS HK1/P/RU/4501742939/RU/02MAR75/M/$(ddmonyy +1y)/
#.RN// 
#.R/PSPT HK1 4501742939/RU/02MAR75// /M
#.R/FOID PP4501742939

!!
$(CHECKIN_PAX $(get pax_id_BURYAKOV) $(get point_dep_UT_100) $(get point_arv_UT_100)
   100    " " 2982425622093  RU 4501742939 RU 02.03.1975 $(date_format %d.%m.%Y) M)

>> lines=auto
<query>
  <svc_availability show_brand_info=\"true\" show_all_svc=\"true\" show_free_carry_on_norm=\"true\">
    <passenger id=\"$(get pax_id_BURYAKOV)\" surname=\"\" name=\" \" category=\"ADT\" birthdate=\"1975-03-02\" sex=\"male\">
      <document number=\"4501742939\" expiration_date=\"$(date_format %Y-%m-%d)\" country=\"RUS\"/>
      <segment id=\"0\" company=\"UT\" flight=\"100\" operating_company=\"UT\" operating_flight=\"100\" departure=\"AER\" arrival=\"LHR\"\
 departure_time=\"xxxx-xx-xxTxx:xx:xx\" arrival_time=\"xxxx-xx-xxTxx:xx:xx\" equipment=\"TU5\" subclass=\"Y\">
        <ticket number=\"2982425622093\" coupon_num=\"1\"/>
        <recloc crs=\"1H\">5C0NXZ</recloc>
        <recloc crs=\"UT\">F451B5</recloc>
      </segment>
    </passenger>
  </svc_availability>
</query>

$(KICK_IN_SILENT)


$(run_arch_step $(ddmmyy +141))


!!capture = on
$(RUN_SERVICES_STAT $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21))

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...>
    <airline></airline>
    <grd>
      <header>
        <col width='50' align='0' sort='0'></col>
        <col width='50' align='0' sort='0'></col>
        <col width='75' align='0' sort='0'>®¬„ą ą„©į </col>
        <col width='50' align='0' sort='0'> ā </col>
        <col width='150' align='0' sort='3'>...</col>
        <col width='150' align='0' sort='3'>Ø«„ā</col>
        <col width='50' align='0' sort='0'>ā</col>
        <col width='50' align='0' sort='0'>®</col>
        <col width='30' align='0' sort='0'>RFIC</col>
        <col width='40' align='0' sort='0'>RFISC</col>
        <col width='100' align='0' sort='0'>ü Ŗ¢Øā ­ęØØ</col>
      </header>
      <rows>
        <row>
          <col></col>
          <col></col>
          <col>100</col>
          <col>$(date_format %d.%m.%y +20)</col>
          <col>  </col>
          <col>2982425622093/1</col>
          <col></col>
          <col></col>
          <col>A</col>
          <col>0B5</col>
          <col>2984555892312/1</col>
        </row>
      </rows>
    </grd>
    <form_data>
      <variables>
        <print_date>... ()</print_date>
        <print_oper>PIKE</print_oper>
        <print_term></print_term>
        <use_seances>0</use_seances>
        <test_server>1</test_server>
        <cap_test></cap_test>
        <page_number_fmt>āą. %u Ø§ %u</page_number_fmt>
        <short_page_number_fmt>āą. %u</short_page_number_fmt>
        <oper_info>āē„ā įä®ą¬Øą®¢ ­ ... ()
®Æ„ą ā®ą®¬ PIKE
į ā„ą¬Ø­ «  </oper_info>
        <skip_header>0</skip_header>
        <stat_type>34</stat_type>
        <stat_mode>į«ć£Ø</stat_mode>
        <stat_type_caption>®¤ą®”­ ļ</stat_type_caption>
      </variables>
    </form_data>
  </answer>
</term>


%%
#########################################################################################

###
#   „įā ü6
#   ā„­Ø„  ąåØ¢  arx_events Ø§ astra_utils.cc TRegEvents::fromDB ä ©«  stat_departed.cc
#   ¢ äć­ŖęØØ departed_flt()
######################################################

$(init_term)

$(PREPARE_SEASON_SCD    100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive  100 )
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))

# ķā® ­ć¦­® ¤«ļ ā®£® ēā®”ė pr_brd = 1 ¢ ā ”«Øę„ PAX,ā® „įāģ Æ®į ¤Øāģ Æ įį ¦Øą 
#  ķā® ¢ į¢®ī ®ē„ą„¤ģ ­ć¦­® ēā®”ė § Æ®«­Ø« įģ ā ”«Øę  STAT_AD Ø Æ®ā®¬ ARX_STAT_AD
$(db_sql TRIP_HALL {INSERT INTO trip_hall(point_id, type, hall, pr_misc)
                    VALUES($(get point_dep_UT_100), 101, NULL, 1)})

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100)  100   TUMALI VALERII 2986145115578  UA FA144642 UA 16.04.1968 25.06.2025 M)

$(run_arch_step $(ddmmyy +141))


$(nosir_departed_flt $(yyyymmdd +10) $(yyyymmdd +30))

#??
#$(read_file departed.2102.csv)
#>> lines=auto
#; ā  ą®¦¤„­Øļ;®«;ØÆ ¤®Ŗć¬„­ā ;„ąØļ Ø ­®¬„ą ¤®Ŗć¬„­ā ;®¬„ą ”Ø«„ā ;®¬„ą ”ą®­Øą®¢ ­Øļ;„©į; ā  ¢ė«„ā ;ā;®; £ ¦ ¬„įā; £ ¦ ¢„į;ą„¬ļ ą„£Øįāą ęØØ (UTC);®¬„ą ¬„įā ;Æ®į®” ą„£Øįāą ęØØ;„ē āģ  ­  įā®©Ŗ„
#VALERII TUMALI;16..68;M;P;FA144642;2986145115578/1;;100;24;;;0;0;04.02.2021 09:49:53;TERM
#=======
#$(nosir_departed_flt 20201220 20210120)

%%
#########################################################################################

###
#   „įā ü7
#   ė”Øą īāįļ ¤ ­­ė„ Ø§  ąåØ¢  Æ® ą„©į ¬ §  ā„Ŗćéćī ¤ āć
#   ¢ äć­ŖęØØ internal_ReadData_N ¢ sopp.cc
######################################################

$(init_term)

$(PREPARE_SEASON_SCD    100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive  100 )
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))
$(set move_id $(get_move_id $(get point_dep_UT_100)))

# ķā® ­ć¦­® ¤«ļ ā®£® ēā®”ė pr_brd = 1 ¢ ā ”«Øę„ PAX,ā® „įāģ Æ®į ¤Øāģ Æ įį ¦Øą 
#  ķā® ¢ į¢®ī ®ē„ą„¤ģ ­ć¦­® ēā®”ė § Æ®«­Ø« įģ ā ”«Øę  STAT_AD Ø Æ®ā®¬ ARX_STAT_AD
$(db_sql TRIP_HALL {INSERT INTO trip_hall(point_id, type, hall, pr_misc)
                    VALUES($(get point_dep_UT_100), 101, NULL, 1)})

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100)  100   TUMALI VALERII 2986145115578  UA FA144642 UA 16.04.1968 25.06.2025 M)

$(run_arch_step $(ddmmyy +141))


!!capture = on
$(READ_ARX_TRIPS $(date_format %d.%m.%Y +20))
>> lines=auto
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...>
    <data>
      <arx_date>$(date_format %d.%m.%Y +20) 00:00:00</arx_date>
      <trips>
        <trip>
          <move_id>$(get move_id)</move_id>
          <point_id>$(get point_dep_UT_100)</point_id>
          <part_key>$(date_format %d.%m.%Y +20) 09:00:00</part_key>
          <pr_del_in>-1</pr_del_in>
          <airp></airp>
          <airline_out></airline_out>
          <flt_no_out>100</flt_no_out>
          <craft_out>TU5</craft_out>
          <scd_out>$(date_format %d.%m.%Y +20) 10:15:00</scd_out>
          <triptype_out>Æ</triptype_out>
          <pr_reg>1</pr_reg>
          <places_out>
            <airp></airp>
          </places_out>
          <classes>
            <class cfg='11'></class>
            <class cfg='63'></class>
          </classes>
          <reg>1</reg>
          <stages>
            <stage>
              <stage_id>10</stage_id>
              <scd>$(date_format %d.%m.%Y +20) 03:15:00</scd>
              <pr_auto>0</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>20</stage_id>
              <scd>$(date_format %d.%m.%Y +20) 07:14:00</scd>
              <pr_auto>0</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>25</stage_id>
              <scd>$(date_format %d.%m.%Y +19) 10:15:00</scd>
              <pr_auto>1</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>26</stage_id>
              <scd>$(date_format %d.%m.%Y +19) 10:15:00</scd>
              <pr_auto>1</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>30</stage_id>
              <scd>$(date_format %d.%m.%Y +20) 09:35:00</scd>
              <pr_auto>0</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>31</stage_id>
              <scd>$(date_format %d.%m.%Y +20) 09:25:00</scd>
              <pr_auto>1</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>35</stage_id>
              <scd>$(date_format %d.%m.%Y +20) 07:15:00</scd>
              <pr_auto>0</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>36</stage_id>
              <scd>$(date_format %d.%m.%Y +20) 08:15:00</scd>
              <pr_auto>1</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>40</stage_id>
              <scd>$(date_format %d.%m.%Y +20) 09:30:00</scd>
              <pr_auto>0</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>50</stage_id>
              <scd>$(date_format %d.%m.%Y +20) 09:50:00</scd>
              <pr_auto>0</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>70</stage_id>
              <scd>$(date_format %d.%m.%Y +20) 10:00:00</scd>
              <pr_auto>0</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
          </stages>
        </trip>
        <trip>
          <move_id>$(get move_id)</move_id>
          <point_id>$(get point_arv_UT_100)</point_id>
          <part_key>$(date_format %d.%m.%Y +20) 09:00:00</part_key>
          <airline_in></airline_in>
          <flt_no_in>100</flt_no_in>
          <craft_in>TU5</craft_in>
          <scd_in>$(date_format %d.%m.%Y +20) 12:00:00</scd_in>
          <triptype_in>Æ</triptype_in>
          <places_in>
            <airp></airp>
          </places_in>
          <airp></airp>
          <pr_del_out>-1</pr_del_out>
          <pr_reg>0</pr_reg>
        </trip>
      </trips>
    </data>
  </answer>
</term>

%%
#########################################################################################
###
#   „įā ü8
#   ą®¢„ąŖ  ēā„­Øļ Ø§  ąåØ¢­ėå ā ”«Øę ÆąØ § ÆćįŖ„ basel_aero_flat_stat
######################################################

$(init_term)

$(sql {INSERT INTO file_sets(code, airp, name, dir, last_create, pr_denial)
       VALUES('BASEL_AERO', '', 'BASEL_AERO', 'basel_aero/', TO_DATE('07.02.21', 'dd.mm.yy'), 0)})


$(PREPARE_SEASON_SCD    100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive  100 )
$(make_spp $(ddmmyy +20))


$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))
$(set move_id $(get_move_id $(get point_dep_UT_100)))

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100)  100   TUMALI VALERII 2986145115578  UA FA144642 UA 16.04.1968 25.06.2025 M)

$(run_arch_step $(ddmmyy +141))

$(nosir_basel_stat $(date_format %d.%m.%Y +20) 09:00:00 $(get point_dep_UT_100))

#-3h Æ®ā®¬ć ēā® utc ¢ą„¬ļ
??
$(check_dump basel_stat)
>>
[] [$(get pax_id_TUMALI)] [$(get point_dep_UT_100)] [$(date_format %d.%m.%Y -3h)] [NULL] [NULL] [0] [NULL] [1] [$(date_format %d.%m.%Y -3h)] [] [NULL] [$(date_format %d.%m.%Y +20)] [$(date_format %d.%m.%Y +20)] [NULL] [100] [...] [TUMALI/VALERII] [0] [0] [NULL] [NULL] [§ ą„£ØįāąØą®¢ ­] [NULL] [NULL] [0] $()
$()

%%
#########################################################################################

###
#   „įā ü9
#   ā„­Ø„  ąåØ¢  Ø§ stat_arx.cc ć­ŖęØļ PAX_SRC_RUN
#
###

$(init_term)

$(PREPARE_SEASON_SCD    298)
$(PREPARE_SEASON_SCD    190)
$(PREPARE_SEASON_SCD    450)

$(make_spp)

$(deny_ets_interactive  298 )
$(deny_ets_interactive  190 )
$(deny_ets_interactive  450 )

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
$(CHECKIN_PAX_TRANSFER $(get pax_id1) $(get point_dep_UT_298) $(get point_arv_UT_298)  298   OZ OFER 2985523437721
                       $(get pax_id2) $(get point_dep_UT_190) $(get point_arv_UT_190)  190   OZ OFER
                       $(get pax_id3) $(get point_dep_UT_450) $(get point_arv_UT_450)  450   OZ OFER
                        UA 32427293 UA 16.04.1968 25.06.2025 M)

$(set grp_id $(get_single_grp_id $(get point_dep_UT_298) OZ OFER))
$(set grp_id2 $(get_single_grp_id $(get point_dep_UT_190) OZ OFER))
$(set grp_id3 $(get_single_grp_id $(get point_dep_UT_450) OZ OFER))

$(run_arch_step $(ddmmyy +151))

!! capture=on
$(RUN_PAX_SRC_STAT $(date_format %d.%m.%Y -10) $(date_format %d.%m.%Y +10)  OZ)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...>
    <paxList>
      <rows>
        <pax>
          <part_key>$(date_format %d.%m.%Y) 09:00:00</part_key>
          <point_id>$(get point_dep_UT_298)</point_id>
          <airline></airline>
          <flt_no>298</flt_no>
          <suffix/>
          <trip>298 </trip>
          <scd_out>$(date_format %d.%m.%Y) 10:15:00</scd_out>
          <reg_no>1</reg_no>
          <full_name>OZ OFER</full_name>
          <bag_amount>0</bag_amount>
          <bag_weight>0</bag_weight>
          <rk_weight>0</rk_weight>
          <excess>0</excess>
          <tags/>
          <seat_no>05</seat_no>
          <grp_id>$(get grp_id)</grp_id>
          <airp_arv></airp_arv>
          <status> ą„£.</status>
          <class></class>
          <document>32427293 UKR</document>
          <ticket_no>2985523437721</ticket_no>
          <hall> « 1</hall>
        </pax>
        <pax>
          <part_key>$(date_format %d.%m.%Y) 09:00:00</part_key>
          <point_id>$(get point_dep_UT_190)</point_id>
          <airline></airline>
          <flt_no>190</flt_no>
          <suffix/>
          <trip>190 </trip>
          <scd_out>$(date_format %d.%m.%Y) 10:15:00</scd_out>
          <reg_no>1</reg_no>
          <full_name>OZ OFER</full_name>
          <bag_amount>0</bag_amount>
          <bag_weight>0</bag_weight>
          <rk_weight>0</rk_weight>
          <excess>0</excess>
          <tags/>
          <seat_no>05</seat_no>
          <grp_id>$(get grp_id2)</grp_id>
          <airp_arv></airp_arv>
          <status> ą„£.</status>
          <class></class>
          <document>32427293 UKR</document>
          <ticket_no>2985523437721</ticket_no>
          <hall> « 1</hall>
        </pax>
        <pax>
          <part_key>$(date_format %d.%m.%Y) 09:00:00</part_key>
          <point_id>$(get point_dep_UT_450)</point_id>
          <airline></airline>
          <flt_no>450</flt_no>
          <suffix/>
          <trip>450 </trip>
          <scd_out>$(date_format %d.%m.%Y) 10:15:00</scd_out>
          <reg_no>1</reg_no>
          <full_name>OZ OFER</full_name>
          <bag_amount>0</bag_amount>
          <bag_weight>0</bag_weight>
          <rk_weight>0</rk_weight>
          <excess>0</excess>
          <tags/>
          <seat_no>05</seat_no>
          <grp_id>$(get grp_id3)</grp_id>
          <airp_arv></airp_arv>
          <status> ą„£.</status>
          <class></class>
          <document>32427293 UKR</document>
          <ticket_no>2985523437721</ticket_no>
          <hall> « 1</hall>
        </pax>
      </rows>
      <header>
        <col>„©į</col>
      </header>
    </paxList>
    <form_data>
      <variables>
        <print_date>... ()</print_date>
        <print_oper>PIKE</print_oper>
        <print_term></print_term>
        <use_seances>0</use_seances>
        <test_server>1</test_server>
        <cap_test></cap_test>
        <page_number_fmt>āą. %u Ø§ %u</page_number_fmt>
        <short_page_number_fmt>āą. %u</short_page_number_fmt>
        <oper_info>āē„ā įä®ą¬Øą®¢ ­ ... ()
®Æ„ą ā®ą®¬ PIKE
į ā„ą¬Ø­ «  </oper_info>
        <skip_header>0</skip_header>
      </variables>
    </form_data>
    <form name='ArxPaxList'...>$(ArxPaxListForm)
</form>
  </answer>
</term>

%%
#########################################################################################

###
#   „įā ü10
#
#   ÆØį ­Ø„: Æ įį ¦Øą®¢: 61,
#             Ø­ā„ą ŖāØ¢: ¢ėŖ«
#
#   ā„­Ø„  ąåØ¢  Ø§ stat_arx.cc äć­ŖęØØ FltTaskLogRun
###
#########################################################################################

$(init_jxt_pult )
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD    300 1003 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD    100 1004 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +267))
$(make_spp $(ddmmyy +265))

$(set point_dep $(get_dep_point_id   300 $(yymmdd +1)))

$(run_arch_step $(ddmmyy +387))

!! capture=on
$(ARX_RUN_FLT_TASK_LOG $(get point_dep) $(date_format %d.%m.%Y +1))

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...>
    <form name='FltTaskLog'...>$(FltTaskLogForm)
</form>
    <form_data>
      <variables>
        <print_date>... ()</print_date>
        <print_oper>PIKE</print_oper>
        <print_term></print_term>
        <use_seances>0</use_seances>
        <test_server>1</test_server>
        <cap_test></cap_test>
        <page_number_fmt>āą. %u Ø§ %u</page_number_fmt>
        <short_page_number_fmt>āą. %u</short_page_number_fmt>
        <oper_info>āē„ā įä®ą¬Øą®¢ ­ ... ()
®Æ„ą ā®ą®¬ PIKE
į ā„ą¬Ø­ «  </oper_info>
        <skip_header>0</skip_header>
        <report_title>ćą­ « § ¤ ē ą„©į </report_title>
      </variables>
    </form_data>
    <PaxLog>
      <header>
        <col>£„­ā</col>
      </header>
      <rows>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg> ¤ ē  EMD_REFRESH &lt;CloseCheckIn 0&gt; į®§¤ ­ ; « ­. ¢ą.: $(date_format %d.%m.%y +1) 06:35:00 (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>„£Øįāą ęØļ</screen>
        </row>
      </rows>
    </PaxLog>
    <airline></airline>
  </answer>
</term>


%%
#########################################################################################

###
#   „įā ü11
#
#   ÆØį ­Ø„: Æ įį ¦Øą®¢: 61,
#             Ø­ā„ą ŖāØ¢: ¢ėŖ«
#
#   ā„­Ø„  ąåØ¢  Ø§ stat_arx.cc äć­ŖęØØ FltLogRun
###
#########################################################################################

$(init_jxt_pult )
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD    300 1003 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD    100 1004 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +267))
$(make_spp $(ddmmyy +265))

$(set point_dep $(get_dep_point_id   300 $(yymmdd +1)))

$(run_arch_step $(ddmmyy +387))

!! capture=on
$(ARX_RUN_FLT_LOG $(get point_dep) $(date_format %d.%m.%Y +1))

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...>
    <form name='FltLog'...>$(FltLogForm)
</form>
    <form_data>
      <variables>
        <print_date>... ()</print_date>
        <print_oper>PIKE</print_oper>
        <print_term></print_term>
        <use_seances>0</use_seances>
        <test_server>1</test_server>
        <cap_test></cap_test>
        <page_number_fmt>āą. %u Ø§ %u</page_number_fmt>
        <short_page_number_fmt>āą. %u</short_page_number_fmt>
        <oper_info>āē„ā įä®ą¬Øą®¢ ­ ... ()
®Æ„ą ā®ą®¬ PIKE
į ā„ą¬Ø­ «  </oper_info>
        <skip_header>0</skip_header>
        <report_title>ćą­ « ®Æ„ą ęØ© ą„©į </report_title>
      </variables>
    </form_data>
    <PaxLog>
      <header>
        <col>£„­ā</col>
      </header>
      <rows>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>  ą„©į„ § Æą„é„­  web-ą„£Øįāą ęØļ.</msg>
          <ev_order>...</ev_order>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>„£Øįāą ęØļ</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>ā Æ '®¤£®ā®¢Ŗ  Ŗ ą„£Øįāą ęØØ': Æ« ­. ¢ą„¬ļ 00:15 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>„£Øįāą ęØļ</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>ā Æ 'āŖąėāØ„ ą„£Øįāą ęØØ': Æ« ­. ¢ą„¬ļ 04:14 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>„£Øįāą ęØļ</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>ā Æ 'āŖąėāØ„ web-ą„£Øįāą ęØØ': Æ« ­. ¢ą„¬ļ 07:15 $(date_format %d.%m.%y) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>„£Øįāą ęØļ</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>ā Æ 'āŖąėāØ„ kiosk-ą„£Øįāą ęØØ': Æ« ­. ¢ą„¬ļ 07:15 $(date_format %d.%m.%y) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>„£Øįāą ęØļ</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>ā Æ ' ŖąėāØ„ ą„£Øįāą ęØØ': Æ« ­. ¢ą„¬ļ 06:35 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>„£Øįāą ęØļ</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>ā Æ ' Æą„ā ®ā¬„­ė web-ą„£Øįāą ęØØ': Æ« ­. ¢ą„¬ļ 06:25 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>„£Øįāą ęØļ</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>ā Æ ' ŖąėāØ„ web-ą„£Øįāą ęØØ': Æ« ­. ¢ą„¬ļ 04:15 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>„£Øįāą ęØļ</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>ā Æ ' ŖąėāØ„ kiosk-ą„£Øįāą ęØØ': Æ« ­. ¢ą„¬ļ 05:15 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>„£Øįāą ęØļ</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>ā Æ '®ā®¢­®įāģ  Ŗ Æ®į ¤Ŗ„': Æ« ­. ¢ą„¬ļ 06:30 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>„£Øįāą ęØļ</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>ā Æ 'Ŗ®­ē ­Ø„ Æ®į ¤ŖØ (®ä®ą¬«„­Ø„ ¤®Ŗć¬.)': Æ« ­. ¢ą„¬ļ 06:50 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>„£Øįāą ęØļ</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>ā Æ 'āŖ ā āą Æ ': Æ« ­. ¢ą„¬ļ 07:00 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>„£Øįāą ęØļ</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg> §­ ē„­Ø„ ¢„į®¢ Æ įį ¦Øą®¢ ­  ą„©į: </msg>
          <ev_order>...</ev_order>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>„£Øįāą ęØļ</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>”®ą įā āØįāØŖØ Æ® ą„©įć</msg>
          <ev_order>...</ev_order>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>„£Øįāą ęØļ</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>„©į Æ„ą„¬„é„­ ¢  ąåØ¢</msg>
          <ev_order>...</ev_order>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>„£Øįāą ęØļ</screen>
        </row>
      </rows>
    </PaxLog>
    <airline></airline>
  </answer>
</term>

%%
#########################################################################################

###
#   „įā ü12
#
#   ÆØį ­Ø„: Æ įį ¦Øą®¢: 61,
#             Ø­ā„ą ŖāØ¢: ¢ėŖ«
#
#   ā„­Ø„  ąåØ¢  Ø§ stat_utils.cc äć­ŖęØØ FltCBoxDropDown
###
#########################################################################################

$(init_jxt_pult )
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD    300 1003 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD    100 1004 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +267))
$(make_spp $(ddmmyy +265))

$(set point_dep_300 $(get_dep_point_id   300 $(yymmdd +1)))
$(set point_dep_100 $(get_dep_point_id   100 $(yymmdd +265)))

$(run_arch_step $(ddmmyy +387))

!! capture=on
$(RUN_FLT_CBOX_DROP_DOWN $(date_format %d.%m.%Y ) $(date_format %d.%m.%Y +266))

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...>
    <cbox>
      <f>
        <name>100  /... </name>
        <point_id>$(get point_dep_100)</point_id>
        <part_key>$(date_format %d.%m.%Y +265) 09:00:00</part_key>
      </f>
      <f>
        <name>300  /... </name>
        <point_id>$(get point_dep_300)</point_id>
        <part_key>$(date_format %d.%m.%Y +1) 09:00:00</part_key>
      </f>
    </cbox>
  </answer>
</term>


%%
#########################################################################################

###
#   „įā ü13
#
#   ÆØį ­Ø„: Æ įį ¦Øą®¢: 61,
#             Ø­ā„ą ŖāØ¢: ¢ėŖ«
#
#   ā„­Ø„  ąåØ¢  Ø§ stat_arx.cc äć­ŖęØØ LogRun
###
#########################################################################################

$(init_jxt_pult )
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD    100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(make_spp $(ddmmyy +20))
$(deny_ets_interactive  100 )
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep) $(get point_arv)  100   TUMALI VALERII 2986145115578  UA FA144642 UA 16.04.1968 25.06.2025 M)

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))

$(run_arch_step $(ddmmyy +150))

!! capture=on
$(ARX_RUN_LOG_RUN $(get point_dep) $(get grp_id) $(date_format %d.%m.%Y +20) 1)
>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...>
    <form_data>
      <variables>
        <print_date>...</print_date>
        <print_oper>PIKE</print_oper>
        <print_term></print_term>
        <use_seances>0</use_seances>
        <test_server>1</test_server>
        <cap_test></cap_test>
        <page_number_fmt>āą. %u Ø§ %u</page_number_fmt>
        <short_page_number_fmt>āą. %u</short_page_number_fmt>
        <oper_info>āē„ā įä®ą¬Øą®¢ ­ ... ()
®Æ„ą ā®ą®¬ PIKE
į ā„ą¬Ø­ «  </oper_info>
        <skip_header>0</skip_header>
        <report_title>Æ„ą ęØØ Æ® Æ įį ¦Øąć</report_title>
      </variables>
    </form_data>
    <PaxLog>
      <header>
        <col>£„­ā</col>
      </header>
      <rows>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg> įį ¦Øą TUMALI VALERII () § ą„£ØįāąØą®¢ ­. /­: , Ŗ« įį: , įā āćį: ą®­ģ, ¬„įā®: 5.  £.­®ą¬ė: ­„ā</msg>
          <ev_order>...</ev_order>
          <grp_id>$(get grp_id)</grp_id>
          <reg_no>1</reg_no>
          <ev_user> ..</ev_user>
          <station></station>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg> įį ¦Øą TUMALI VALERII (). DOCS: P/UKR/FA144642/UKR/16APR68/M/25JUN25/TUMALI/VALERII/. ćē­®© ¢¢®¤</msg>
          <ev_order>...</ev_order>
          <grp_id>$(get grp_id)</grp_id>
          <reg_no>1</reg_no>
          <ev_user> ..</ev_user>
          <station></station>
        </row>
      </rows>
    </PaxLog>
    <airline></airline>
  </answer>
</term>

%%
#########################################################################################

###
#   „įā ü14
#
#   ÆØį ­Ø„: Æ įį ¦Øą®¢: 61,
#             Ø­ā„ą ŖāØ¢: ¢ėŖ«
#
#   ā„­Ø„  ąåØ¢  Ø§ stat_general.cc
###
#########################################################################################

$(init_jxt_pult )
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD    100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(make_spp $(ddmmyy +20))
$(deny_ets_interactive  100 )
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep) $(get point_arv)  100   TUMALI VALERII 2986145115578  UA FA144642 UA 16.04.1968 25.06.2025 M)

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))


$(run_arch_step $(ddmmyy +150))

$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) „ā «Ø§Øą®¢ ­­ ļ ”é ļ)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) „ā «Ø§Øą®¢ ­­ ļ  ¬®ą„£Øįāą ęØļ)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) „ā «Ø§Øą®¢ ­­ ļ "āÆą. ā„«„£ą ¬¬ė")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ”é ļ ”é ļ)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ”é ļ "āÆą. ā„«„£ą ¬¬ė")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ”é ļ ®£®¢®ą)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ”é ļ  ¬®ą„£Øįāą ęØļ)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ”é ļ "®  £„­ā ¬")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ "āÆą. ā„«„£ą ¬¬ė")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ  ¬®ą„£Øįāą ęØļ)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ ”é ļ)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ "®  £„­ā ¬")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ " £ ¦­ė„ RFISC")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ "£ą. ¢®§¬®¦.")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ "­­ć«. ”ØąŖØ")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ PFS)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ ą ­įä„ą)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ  įį„«„­Ø„)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ "Ø§­„į ÆąØ£« č„­Øļ")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ  ćē„ąė)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ " Ŗā. ¢ė«„ā")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ „ÆąØ­ā)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ į«ć£Ø)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ „¬ ąŖØ)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ "§¬„­„­Øļ į «®­ ")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ "SBDO (Zamar)")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ā®£® "®  £„­ā ¬")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ "„į®Æą. ” £ ¦")


%%
#########################################################################################

###
#   „įā ü15
#
#   ÆØį ­Ø„: Æ įį ¦Øą®¢: 61,
#             Ø­ā„ą ŖāØ¢: ¢ėŖ«
#
#   ā„­Ø„  ąåØ¢  Ø§ stat_arx.cc äć­ŖęØØ SystemLogRun
###
#########################################################################################

$(init_jxt_pult )
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD    100 -1 TU5 $(date_format %d.%m.%Y +0) $(date_format %d.%m.%Y +5))
$(make_spp $(ddmmyy +1))
$(deny_ets_interactive  100 )
$(INB_PNL_UT AER LHR 100 $(ddmon +1 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep) $(get point_arv)  100   TUMALI VALERII 2986145115578  UA FA144642 UA 16.04.1968 25.06.2025 M)

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))

$(run_arch_step $(ddmmyy +140))

$(set first_date "$(date_format %d.%m.%Y +0 -3h) $(date_format %H:%M:%S -4h)")
$(set last_date "$(date_format %d.%m.%Y +1 -3h) $(date_format %H:%M:%S -4h)")

$(db_dump_table ARX_EVENTS)

!! capture=on
$(RUN_SYSTEM_LOG $(get first_date) $(get last_date))
>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...>
    <form name='SystemLog'...>$(SystemLogForm)
</form>
    <form_data>
      <variables>
        <print_date>...</print_date>
        <print_oper>PIKE</print_oper>
        <print_term></print_term>
        <use_seances>0</use_seances>
        <test_server>1</test_server>
        <cap_test></cap_test>
        <page_number_fmt>āą. %u Ø§ %u</page_number_fmt>
        <short_page_number_fmt>āą. %u</short_page_number_fmt>
        <oper_info>āē„ā įä®ą¬Øą®¢ ­ ... ()
®Æ„ą ā®ą®¬ PIKE
į ā„ą¬Ø­ «  </oper_info>
        <skip_header>0</skip_header>
        <report_title>Æ„ą ęØØ ¢ įØįā„¬„</report_title>
      </variables>
    </form_data>
    <PaxLog>
      <header>
        <col>£„­ā</col>
      </header>
      <rows>
        <row>
          <point_id>0</point_id>
          <time>...</time>
          <msg>¢®¤ ­®¢®£® ą„©į </msg>
          <ev_order>...</ev_order>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>0</point_id>
          <time>...</time>
          <msg>¢®¤ ­®¢®£® Æ„ąØ®¤  $(date_format %d.%m.%y +0) $(date_format %d.%m.%y +5) 1234567 (Ø¤. ą„©į =...,Ø¤. ¬ ąčąćā =...)</msg>
          <ev_order>...</ev_order>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>0</point_id>
          <time>...</time>
          <msg> ąčąćā: 100,5,07:15(UTC)-09:00(UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>0</point_id>
          <time>...</time>
          <msg>®«ćē„­Ø„  §  ...</msg>
          <ev_order>...</ev_order>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>0</point_id>
          <time>...</time>
          <msg>¢®¤ įāą®ŖØ ¢ ā ”«Øę„ ' įāą®©ŖØ ą„©į®¢ ą §­ė„': TYPE_CODE='11',AIRLINE='',/Ŗ='',AIRP_DEP='',/Æ ¢ė«„ā ='',„©į='100',­ ē„­Ø„='1'. ¤„­āØäØŖ ā®ą: TYPE_CODE='11',AIRLINE='',/Ŗ='',AIRP_DEP='',/Æ ¢ė«„ā ='',„©į='100',­ ē„­Ø„='1'</msg>
          <ev_order>...</ev_order>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>MAINDCS.EXE</screen>
        </row>
      </rows>
    </PaxLog>
    <PaxLog>
      <header>
        <col>£„­ā</col>
      </header>
      <rows>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>  ą„©į„ § Æą„é„­  web-ą„£Øįāą ęØļ.</msg>
          <ev_order>...</ev_order>
          <trip>100/... </trip>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>ā Æ '®¤£®ā®¢Ŗ  Ŗ ą„£Øįāą ęØØ': Æ« ­. ¢ą„¬ļ 00:15 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <trip>100/... </trip>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>ā Æ 'āŖąėāØ„ ą„£Øįāą ęØØ': Æ« ­. ¢ą„¬ļ 04:14 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <trip>100/... </trip>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>ā Æ 'āŖąėāØ„ web-ą„£Øįāą ęØØ': Æ« ­. ¢ą„¬ļ 07:15 $(date_format %d.%m.%y +0) (UTC)</msg>
          <ev_order>...</ev_order>
          <trip>100/... </trip>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>ā Æ 'āŖąėāØ„ kiosk-ą„£Øįāą ęØØ': Æ« ­. ¢ą„¬ļ 07:15 $(date_format %d.%m.%y +0) (UTC)</msg>
          <ev_order>...</ev_order>
          <trip>100/... </trip>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>ā Æ ' ŖąėāØ„ ą„£Øįāą ęØØ': Æ« ­. ¢ą„¬ļ 06:35 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <trip>100/... </trip>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>ā Æ ' Æą„ā ®ā¬„­ė web-ą„£Øįāą ęØØ': Æ« ­. ¢ą„¬ļ 06:25 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <trip>100/... </trip>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>ā Æ ' ŖąėāØ„ web-ą„£Øįāą ęØØ': Æ« ­. ¢ą„¬ļ 04:15 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <trip>100/... </trip>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>ā Æ ' ŖąėāØ„ kiosk-ą„£Øįāą ęØØ': Æ« ­. ¢ą„¬ļ 05:15 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <trip>100/... </trip>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>ā Æ '®ā®¢­®įāģ  Ŗ Æ®į ¤Ŗ„': Æ« ­. ¢ą„¬ļ 06:30 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <trip>100/... </trip>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>ā Æ 'Ŗ®­ē ­Ø„ Æ®į ¤ŖØ (®ä®ą¬«„­Ø„ ¤®Ŗć¬.)': Æ« ­. ¢ą„¬ļ 06:50 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <trip>100/... </trip>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>ā Æ '®ā£®­ āą Æ ': Æ« ­. ¢ą„¬ļ 07:00 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <trip>100/... </trip>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg> §­ ē„­Ø„ ¢„į®¢ Æ įį ¦Øą®¢ ­  ą„©į: </msg>
          <ev_order>...</ev_order>
          <trip>100/... </trip>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg> ¤ ē  EMD_REFRESH &lt;CloseCheckIn 0&gt; į®§¤ ­ ; « ­. ¢ą.: $(date_format %d.%m.%y +1) 06:35:00 (UTC)</msg>
          <ev_order>...</ev_order>
          <trip>100/... </trip>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg> ¤ ē  SYNC_ALL_CHKD &lt;&gt; į®§¤ ­ ; « ­. ¢ą.: $(date_format %d.%m.%y +0 -3h) ... (UTC)</msg>
          <ev_order>...</ev_order>
          <trip>100/... </trip>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>MAINDCS.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg> §­ ē„­  ” §®¢ ļ Ŗ®¬Æ®­®¢Ŗ  (Ø¤=43345). « įįė: 11 63</msg>
          <ev_order>...</ev_order>
          <trip>100/... </trip>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>MAINDCS.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg> §­ ē„­Ø„ ¢„į®¢ Æ įį ¦Øą®¢ ­  ą„©į: </msg>
          <ev_order>...</ev_order>
          <trip>100/... </trip>
          <station>IATCIP</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg> įį ¦Øą TUMALI VALERII () § ą„£ØįāąØą®¢ ­. /­: , Ŗ« įį: , įā āćį: ą®­ģ, ¬„įā®: 5.  £.­®ą¬ė: ­„ā</msg>
          <ev_order>...</ev_order>
          <trip>100/... </trip>
          <grp_id>$(get grp_id)</grp_id>
          <reg_no>1</reg_no>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg> įį ¦Øą TUMALI VALERII (). DOCS: P/UKR/FA144642/UKR/16APR68/M/25JUN25/TUMALI/VALERII/. ćē­®© ¢¢®¤</msg>
          <ev_order>...</ev_order>
          <trip>100/... </trip>
          <grp_id>$(get grp_id)</grp_id>
          <reg_no>1</reg_no>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg> §­ ē„­Ø„ ¢„į®¢ Æ įį ¦Øą®¢ ­  ą„©į: </msg>
          <ev_order>...</ev_order>
          <trip>100/... </trip>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg> §­ ē„­Ø„ ¢„į®¢ Æ įį ¦Øą®¢ ­  ą„©į: </msg>
          <ev_order>...</ev_order>
          <trip>100/... </trip>
          <ev_user> ..</ev_user>
          <station></station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>”®ą įā āØįāØŖØ Æ® ą„©įć</msg>
          <ev_order>...</ev_order>
          <trip>100/... </trip>
          <station>IATCIP</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>„©į Æ„ą„¬„é„­ ¢  ąåØ¢</msg>
          <ev_order>...</ev_order>
          <trip>100/... </trip>
          <station>IATCIP</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>...</point_id>
          <time>...</time>
          <msg>„©į Æ„ą„¬„é„­ ¢  ąåØ¢</msg>
          <ev_order>...</ev_order>
          <trip/>
          <station>IATCIP</station>
          <screen>AIR.EXE</screen>
        </row>
      </rows>
    </PaxLog>
  </answer>
</term>

%%
#########################################################################################
###
#   „įā ü16
#
#   ÆØį ­Ø„: Æ įį ¦Øą®¢: 61,
#             Ø­ā„ą ŖāØ¢: ¢ėŖ«
#
#   ā„­Ø„  ąåØ¢  Ø§ ą„©į  ”„§ ¢ą„¬„­Ø ÆąØ«„ā .  ­­ė„ ­„ ¤®«¦­ė ć¤ «ļāģįļ „į«Ø ¢ą„¬ļ ÆąØ«„ā  ­„ § ¤ ­®
#
###
#########################################################################################

$(init_term)

$(init_apps   APPS_21 closeout=false inbound=true outbound=true)
$(init_apps   APPS_21 closeout=false inbound=true outbound=true)

$(PREPARE_SEASON_SCD_WITHOUT_ARRIVE_TIME    100  -1 TU5 $(date_format %d.%m.%Y +0) $(date_format %d.%m.%Y +5))

$(make_spp $(ddmmyy +1))
$(deny_ets_interactive  100 )

$(INB_PNL_UT AER LHR 100 $(ddmon +1 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(set move_id $(get_move_id $(get point_dep)))
$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

$(auto_set_craft $(get point_dep))

$(db_dump_table POINTS)

$(run_arch_step $(ddmmyy +1))

??
$(check_dump POINTS)
>>
[...] [...] [0] [] [0] [NULL] [] [100] [NULL] [5] [NULL] [NULL] [NULL] [NULL] [$(date_format %d.%m.%Y +1)] [NULL] [NULL] [NULL] [$(date_format %d.%m.%Y +1)] [Æ] [NULL] [NULL] [NULL] [NULL] [1] [0] [0] [0] [1] [NULL] [...] $()
[...] [...] [1] [] [0] [...] [NULL] [NULL] [NULL] [NULL] [NULL] [NULL] [NULL] [NULL] [NULL] [NULL] [NULL] [01.01.1900] [01.01.1900] [NULL] [NULL] [NULL] [NULL] [NULL] [0] [0] [0] [NULL] [NULL] [NULL] [...] $()
$()

$(run_arch_step $(ddmmyy +122))
??
$(check_dump POINTS)
>>
$()


%%
#########################################################################################

###
#   „įā ü17
#
#   ÆØį ­Ø„: Æ įį ¦Øą®¢: 61,
#             Ø­ā„ą ŖāØ¢: ¢ėŖ«
#
#   ā„­Ø„  ąåØ¢  Ø§ stat_general.cc
###
#########################################################################################


$(init_jxt_pult )
$(set_desk_version 201707-0195750)

$(sql "insert into ARO_AIRPS(ARO_ID, AIRP, ID) values (5, '', id__seq.nextval)")
$(sql "insert into ARO_AIRPS(ARO_ID, AIRP, ID) values (5, '', id__seq.nextval)")
$(sql "insert into ARO_AIRPS(ARO_ID, AIRP, ID) values (5, '', id__seq.nextval)")

$(sql "insert into ARO_AIRLINES(ARO_ID, AIRLINE, ID) values (5, '', id__seq.nextval)")
$(sql "insert into ARO_AIRLINES(ARO_ID, AIRLINE, ID) values (5, '', id__seq.nextval)")

$(db_sql PACTS "insert into PACTS(AIRLINE, AIRP, DESCR, FIRST_DATE, LAST_DATE, ID) values ('', '', '', TO_DATE('$(date_format %Y-%m-%d -30)', 'yyyy-mm-dd'), null, $(db_seq_nextval ID__SEQ))")

$(login)
################################################################################

$(PREPARE_SEASON_SCD    100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(make_spp $(ddmmyy +20))
$(deny_ets_interactive  100 )
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep) $(get point_arv)  100   TUMALI VALERII 2986145115578  UA FA144642 UA 16.04.1968 25.06.2025 M)

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))


$(run_arch_step $(ddmmyy +150))

$(defmacro RUN_GENERAL_STAT_AK
    first_date
    last_date
    stat_mode
    stat_type
{{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='stat' ver='1' opr='PIKE' screen='STAT.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <run_stat>
      <dev_model/>
      <fmt_type/>
      <prnParams>
        <pr_lat>0</pr_lat>
        <encoding>UTF-16LE</encoding>
        <offset>20</offset>
        <top>0</top>
      </prnParams>
      <stat_mode>$(stat_mode)</stat_mode>
      <stat_type>$(stat_type)</stat_type>
      <FirstDate>$(first_date) 00:00:00</FirstDate>
      <LastDate>$(last_date) 00:00:00</LastDate>
      <ak></ak>
      <ap/>
      <flt_no/>
      <Seance></Seance>
      <seance></seance>
      <source>STAT</source>
      <LoadForm/>
    </run_stat>
  </query>
</term>}
})  #end_of_macro

$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) „ā «Ø§Øą®¢ ­­ ļ ”é ļ)
$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) „ā «Ø§Øą®¢ ­­ ļ  ¬®ą„£Øįāą ęØļ)
$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) „ā «Ø§Øą®¢ ­­ ļ "āÆą. ā„«„£ą ¬¬ė")
$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ”é ļ ”é ļ)
$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ”é ļ "āÆą. ā„«„£ą ¬¬ė")
$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ”é ļ ®£®¢®ą)
$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ”é ļ  ¬®ą„£Øįāą ęØļ)
$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ”é ļ "®  £„­ā ¬")
$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ "āÆą. ā„«„£ą ¬¬ė")
$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ  ¬®ą„£Øįāą ęØļ)
$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ ”é ļ)
$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ "®  £„­ā ¬")
$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ " £ ¦­ė„ RFISC")
$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ "£ą. ¢®§¬®¦.")
$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ "­­ć«. ”ØąŖØ")
$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ PFS)
$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ ą ­įä„ą)
$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ  įį„«„­Ø„)
$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ "Ø§­„į ÆąØ£« č„­Øļ")
$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ  ćē„ąė)
$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ " Ŗā. ¢ė«„ā")
$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ „ÆąØ­ā)
$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ į«ć£Ø)
$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ „¬ ąŖØ)
$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ "§¬„­„­Øļ į «®­ ")
$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ "SBDO (Zamar)")
$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ā®£® "®  £„­ā ¬")
$(RUN_GENERAL_STAT_AK  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ "„į®Æą. ” £ ¦")


%%
#########################################################################################

###
#   „įā ü18
#
#   ÆØį ­Ø„: Æ įį ¦Øą®¢: 61,
#             Ø­ā„ą ŖāØ¢: ¢ėŖ«
#
#   ā„­Ø„  ąåØ¢  Ø§ stat_general.cc
###
#########################################################################################

$(init_jxt_pult )
$(set_desk_version 201707-0195750)

$(sql "insert into ARO_AIRPS(ARO_ID, AIRP, ID) values (5, '', id__seq.nextval)")
$(sql "insert into ARO_AIRPS(ARO_ID, AIRP, ID) values (5, '', id__seq.nextval)")
$(sql "insert into ARO_AIRPS(ARO_ID, AIRP, ID) values (5, '', id__seq.nextval)")

$(sql "insert into ARO_AIRLINES(ARO_ID, AIRLINE, ID) values (5, '', id__seq.nextval)")
$(sql "insert into ARO_AIRLINES(ARO_ID, AIRLINE, ID) values (5, '', id__seq.nextval)")

$(sql "insert into PACTS(AIRLINE, AIRP, DESCR, FIRST_DATE, LAST_DATE, ID) values ('', '', '', TO_DATE('$(date_format %Y-%m-%d -30)', 'yyyy-mm-dd'), null, $(db_seq_nextval ID__SEQ))")

$(login)

################################################################################

$(PREPARE_SEASON_SCD    100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(make_spp $(ddmmyy +20))
$(deny_ets_interactive  100 )
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep) $(get point_arv)  100   TUMALI VALERII 2986145115578  UA FA144642 UA 16.04.1968 25.06.2025 M)

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))


$(run_arch_step $(ddmmyy +150))

$(defmacro RUN_GENERAL_STAT_AP
    first_date
    last_date
    stat_mode
    stat_type
{{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='stat' ver='1' opr='PIKE' screen='STAT.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <run_stat>
      <dev_model/>
      <fmt_type/>
      <prnParams>
        <pr_lat>0</pr_lat>
        <encoding>UTF-16LE</encoding>
        <offset>20</offset>
        <top>0</top>
      </prnParams>
      <stat_mode>$(stat_mode)</stat_mode>
      <stat_type>$(stat_type)</stat_type>
      <FirstDate>$(first_date) 00:00:00</FirstDate>
      <LastDate>$(last_date) 00:00:00</LastDate>
      <ak/>
      <ap></ap>
      <flt_no/>
      <Seance></Seance>
      <seance></seance>
      <source>STAT</source>
      <LoadForm/>
    </run_stat>
  </query>
</term>}
})  #end_of_macro

$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) „ā «Ø§Øą®¢ ­­ ļ ”é ļ)
$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) „ā «Ø§Øą®¢ ­­ ļ  ¬®ą„£Øįāą ęØļ)
$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) „ā «Ø§Øą®¢ ­­ ļ "āÆą. ā„«„£ą ¬¬ė")
$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ”é ļ ”é ļ)
$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ”é ļ "āÆą. ā„«„£ą ¬¬ė")
$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ”é ļ ®£®¢®ą)
$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ”é ļ  ¬®ą„£Øįāą ęØļ)
$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ”é ļ "®  £„­ā ¬")
$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ "āÆą. ā„«„£ą ¬¬ė")
$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ  ¬®ą„£Øįāą ęØļ)
$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ ”é ļ)
$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ "®  £„­ā ¬")
$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ " £ ¦­ė„ RFISC")
$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ "£ą. ¢®§¬®¦.")
$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ "­­ć«. ”ØąŖØ")
$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ PFS)
$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ ą ­įä„ą)
$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ  įį„«„­Ø„)
$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ "Ø§­„į ÆąØ£« č„­Øļ")
$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ  ćē„ąė)
$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ " Ŗā. ¢ė«„ā")
$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ „ÆąØ­ā)
$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ į«ć£Ø)
$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ „¬ ąŖØ)
$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ "§¬„­„­Øļ į «®­ ")
$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ "SBDO (Zamar)")
$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ā®£® "®  £„­ā ¬")
$(RUN_GENERAL_STAT_AP  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ®¤ą®”­ ļ "„į®Æą. ” £ ¦")

%%
#########################################################################################
###
#   „įā ü19
#
#   ÆØį ­Ø„: Æ įį ¦Øą®¢: 61,
#             Ø­ā„ą ŖāØ¢: ¢ėŖ«
#
#   ā„­Ø„  ąåØ¢  Ø§ sopp.cc ReadDests -> arx_internal_ReadDests
#
###
#########################################################################################

$(init_term)

$(init_apps   APPS_21 closeout=false inbound=true outbound=true)
$(init_apps   APPS_21 closeout=false inbound=true outbound=true)

$(PREPARE_SEASON_SCD_WITHOUT_ARRIVE_TIME    100  -1 TU5 $(date_format %d.%m.%Y +0) $(date_format %d.%m.%Y +5))

$(make_spp $(ddmmyy +1))
$(deny_ets_interactive  100 )

$(INB_PNL_UT AER LHR 100 $(ddmon +1 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(set move_id $(get_move_id $(get point_dep)))
$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

$(auto_set_craft $(get point_dep))

$(db_dump_table POINTS)

$(run_arch_step $(ddmmyy +1))

$(READ_DESTS $(get move_id) $(date_format %d.%m.%Y +1))
