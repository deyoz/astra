include(ts/macro.ts)
include(ts/adm_macro.ts)

# meta: suite readarch

#########################################################################################
###
#   ���� �1
#   �⥭�� ��娢� �� passenger.cc �� �㭪樨 LoadPaxDoc ��뢠�饩�� ��
#   �㭪樨 RunTrferPaxStat �� ����⨪� stat_trfer_pax.cc
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

$(run_arch_step $(ddmmyy +151))

$(dump_table MOVE_ARX_EXT fields = "move_id, part_key, date_range")
$(dump_table ARX_TRFER_PAX_STAT)

!! capture=on
$(RUN_TRFER_PAX_STAT $(date_format %d.%m.%Y -160) $(date_format %d.%m.%Y +21))

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...>
    <airline>��</airline>
    <grd>
      <header>
        <col width='60' align='0' sort='0'>��</col>
        <col width='30' align='0' sort='0'>���</col>
        <col width='50' align='0' sort='0'>���.1</col>
        <col width='55' align='0' sort='0'>���</col>
        <col width='30' align='0' sort='0'>���</col>
        <col width='60' align='0' sort='0'>���.2</col>
        <col width='55' align='0' sort='0'>���</col>
        <col width='30' align='0' sort='0'>���</col>
        <col width='60' align='0' sort='0'>��⥣���</col>
        <col width='60' align='0' sort='0'>��� ���ᠦ��</col>
        <col width='70' align='0' sort='0'>���㬥��</col>
        <col width='60' align='0' sort='0'>�</col>
        <col width='60' align='0' sort='0'>��</col>
        <col width='60' align='0' sort='0'>��</col>
        <col width='60' align='0' sort='0'>��</col>
        <col width='60' align='0' sort='0'>�/�</col>
        <col width='60' align='0' sort='0'>�� ����</col>
        <col width='60' align='0' sort='0'>�� ���</col>
        <col width='90' align='0' sort='0'>��ન</col>
      </header>
      <rows>
        <row>
          <col>��</col>
          <col>���</col>
          <col>298</col>
          <col>$(date_format %d.%m.%y)</col>
          <col>���</col>
          <col>��190</col>
          <col>$(date_format %d.%m.%y)</col>
          <col>���</col>
          <col>���-���</col>
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
          <col>��</col>
          <col>���</col>
          <col>190</col>
          <col>$(date_format %d.%m.%y)</col>
          <col>���</col>
          <col>��450</col>
          <col>$(date_format %d.%m.%y)</col>
          <col>���</col>
          <col>���-���</col>
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
          <col>�⮣�:</col>
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
        <print_date>$(date_format %d.%m.%Y) $(date_format %H:%M:%S) (���)</print_date>
        <print_oper>PIKE</print_oper>
        <print_term>������</print_term>
        <use_seances>0</use_seances>
        <test_server>1</test_server>
        <cap_test>����</cap_test>
        <page_number_fmt>���. %u �� %u</page_number_fmt>
        <short_page_number_fmt>���. %u</short_page_number_fmt>
        <oper_info>���� ��ନ஢�� $(date_format %d.%m.%Y) $(date_format %H:%M:%S) (���)
�����஬ PIKE
� �ନ���� ������</oper_info>
        <skip_header>0</skip_header>
        <stat_type>21</stat_type>
        <stat_mode>�࠭���</stat_mode>
        <stat_type_caption>���஡���</stat_type_caption>
      </variables>
    </form_data>
  </answer>
</term>

%%
#########################################################################################

###
#   ���� �2
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#
#   �⥭�� ��娢� �� events.cc �㭪樨 GetEvents
###
#########################################################################################

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD �� ��� ��� 300 1003 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD �� ��� ��� 100 1004 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +267))
$(make_spp $(ddmmyy +265))

$(set point_dep $(get_dep_point_id ��� �� 300 $(yymmdd +1)))

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")

$(run_arch_step $(ddmmyy +387))

#$(dump_pg_table ARX_POINTS)
#$(dump_pg_table ARX_EVENTS order="ev_order, lang")

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
        <ev_user>������� �.�.</ev_user>
        <station>������</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>�� ३� ����饭� web-ॣ������.</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>������� �.�.</ev_user>
        <station>������</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>�⠯ '�����⮢�� � ॣ����樨': ����. �६� 00:15 $(date_format %d.%m.%y +1) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>������� �.�.</ev_user>
        <station>������</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>�⠯ '����⨥ ॣ����樨': ����. �६� 04:14 $(date_format %d.%m.%y +1) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>������� �.�.</ev_user>
        <station>������</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>�⠯ '����⨥ web-ॣ����樨': ����. �६� 07:15 $(date_format %d.%m.%y) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>������� �.�.</ev_user>
        <station>������</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>�⠯ '����⨥ kiosk-ॣ����樨': ����. �६� 07:15 $(date_format %d.%m.%y) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>������� �.�.</ev_user>
        <station>������</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>�⠯ '�����⨥ ॣ����樨': ����. �६� 06:35 $(date_format %d.%m.%y +1) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>������� �.�.</ev_user>
        <station>������</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>�⠯ '����� �⬥�� web-ॣ����樨': ����. �६� 06:25 $(date_format %d.%m.%y +1) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>������� �.�.</ev_user>
        <station>������</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>�⠯ '�����⨥ web-ॣ����樨': ����. �६� 04:15 $(date_format %d.%m.%y +1) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>������� �.�.</ev_user>
        <station>������</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>�⠯ '�����⨥ kiosk-ॣ����樨': ����. �६� 05:15 $(date_format %d.%m.%y +1) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>������� �.�.</ev_user>
        <station>������</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>�⠯ '��⮢����� �� � ��ᠤ��': ����. �६� 06:30 $(date_format %d.%m.%y +1) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>������� �.�.</ev_user>
        <station>������</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>�⠯ '����砭�� ��ᠤ�� (��ଫ���� ����.)': ����. �६� 06:50 $(date_format %d.%m.%y +1) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>������� �.�.</ev_user>
        <station>������</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>�⠯ '�⪠� �࠯�': ����. �६� 07:00 $(date_format %d.%m.%y +1) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>������� �.�.</ev_user>
        <station>������</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>�����祭�� ��ᮢ ���ᠦ�஢ �� ३�: </msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>������� �.�.</ev_user>
        <station>������</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>���� ����⨪� �� ३��</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>������� �.�.</ev_user>
        <station>������</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>���� ��६�饭 � ��娢</msg>
        <ev_order>...</ev_order>
      </row>
    </events_log>
    <form_data>
      <variables>
        <trip>��300</trip>
        <scd_out>01.12.0002</scd_out>
        <real_out>01.12.0002</real_out>
        <scd_date>01.12</scd_date>
        <date_issue>$(date_format %d.%m.%y) $(date_format %H:%M)</date_issue>
        <day_issue>$(date_format %d.%m.%y)</day_issue>
        <lang>RU</lang>
        <own_airp_name>�������� ���������</own_airp_name>
        <own_airp_name_lat>AMSTERDAM AIRPORT</own_airp_name_lat>
        <airp_dep_name>���������</airp_dep_name>
        <airp_dep_city>���</airp_dep_city>
        <airline_name>��� ������������ �����</airline_name>
        <flt>��300</flt>
        <bort/>
        <craft>��5</craft>
        <park/>
        <scd_time>xxxxx</scd_time>
        <long_route>���������(���)-�����(���)</long_route>
        <test_server>1</test_server>
        <page_number_fmt>���. %u �� %u</page_number_fmt>
        <landscape>0</landscape>
        <caption>��ୠ� ����権 �� ��300/01.12.0002 �� $(date_format %d.%m.%y)</caption>
        <cap_test>����</cap_test>
        <short_page_number_fmt>���. %u</short_page_number_fmt>
      </variables>
    </form_data>
    <FormNotExists>EventsLog</FormNotExists>
    <form name='EventsLog'/>
  </answer>
</term>


%%
#########################################################################################

###
#   ���� �3
#   �⥭�� ��娢� �� stat_arx.cc �㭪�� PaxListRun
#
###

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

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

$(set grp_id $(get_single_grp_id $(get point_dep_UT_298) OZ OFER))

$(run_arch_step $(ddmmyy +151))

$(dump_table MOVE_ARX_EXT fields = "move_id, part_key, date_range")
$(dump_table ARX_TRFER_PAX_STAT)

$(dump_table arx_pax_grp)
$(dump_table arx_pax)
$(dump_table arx_points)

!! capture=on
$(PAX_LIST_RUN  $(get point_dep_UT_298) $(date_format %d.%m.%Y))

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...>
    <paxList>
      <rows>
        <pax>
          <part_key>$(date_format %d.%m.%Y) 09:00:00</part_key>
          <point_id>$(get point_dep_UT_298)</point_id>
          <airline>��</airline>
          <flt_no>298</flt_no>
          <suffix/>
          <trip>��298 ���</trip>
          <scd_out>$(date_format %d.%m.%Y) 10:15:00</scd_out>
          <reg_no>1</reg_no>
          <full_name>OZ OFER</full_name>
          <bag_amount>0</bag_amount>
          <bag_weight>0</bag_weight>
          <rk_weight>0</rk_weight>
          <excess>0</excess>
          <grp_id>$(get grp_id)</grp_id>
          <airp_arv>���</airp_arv>
          <tags/>
          <status>��ॣ.</status>
          <class>�</class>
          <seat_no>05�</seat_no>
          <document>32427293 UKR</document>
          <ticket_no>2985523437721</ticket_no>
          <hall>��� 1</hall>
        </pax>
      </rows>
      <header>
        <col>����</col>
      </header>
    </paxList>
    <form_data>
      <variables>
        <print_date>$(date_format %d.%m.%Y) $(date_format %H:%M:%S) (���)</print_date>
        <print_oper>PIKE</print_oper>
        <print_term>������</print_term>
        <use_seances>0</use_seances>
        <test_server>1</test_server>
        <cap_test>����</cap_test>
        <page_number_fmt>���. %u �� %u</page_number_fmt>
        <short_page_number_fmt>���. %u</short_page_number_fmt>
        <oper_info>���� ��ନ஢�� $(date_format %d.%m.%Y) $(date_format %H:%M:%S) (���)
�����஬ PIKE
� �ନ���� ������</oper_info>
        <skip_header>0</skip_header>
      </variables>
    </form_data>
  </answer>
</term>


%%
#########################################################################################

###
#   ���� �4
#   �⥭�� ��娢� �� arx_stat_ad.cc
#
######################################################

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

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

$(dump_table points)

$(run_arch_step $(ddmmyy +141))

$(are_tables_equal ARX_PAX)
$(are_tables_equal ARX_PAX_GRP)
$(are_tables_equal ARX_STAT_AD)
$(are_tables_equal ARX_STAT_SERVICES)
$(are_tables_equal ARX_POINTS)

!! capture=on
$(RUN_ACTUAL_DEPARTURED_STAT $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21))

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...>
    <FormNotExists>stat</FormNotExists>
    <form name='stat'/>
    <airline>��</airline>
    <grd>
      <header>
        <col width='50' align='0' sort='0'>��</col>
        <col width='50' align='0' sort='0'>��</col>
        <col width='75' align='0' sort='0'>����� ३�</col>
        <col width='50' align='0' sort='3'>���</col>
        <col width='50' align='0' sort='3'>PNR</col>
        <col width='150' align='0' sort='3'>�.�.�.</col>
        <col width='50' align='0' sort='3'>���</col>
        <col width='50' align='0' sort='3'>�����</col>
        <col width='50' align='0' sort='3'>��� ॣ.</col>
        <col width='50' align='0' sort='3'>�����</col>
        <col width='50' align='0' sort='3'>���. �� ��ᠤ��</col>
        <col width='50' align='0' sort='3'>� �</col>
      </header>
      <rows>
        <row>
          <col>��</col>
          <col>���</col>
          <col>100</col>
          <col>$(date_format %d.%m.%y +20)</col>
          <col>F50CF0</col>
          <col>TUMALI VALERII</col>
          <col>��</col>
          <col>�</col>
          <col>TERM</col>
          <col/>
          <col>������</col>
          <col>5�</col>
        </row>
        <row>
          <col>�⮣�:</col>
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
        <print_date>$(date_format %d.%m.%Y) $(date_format %H:%M:%S) (���)</print_date>
        <print_oper>PIKE</print_oper>
        <print_term>������</print_term>
        <use_seances>0</use_seances>
        <test_server>1</test_server>
        <cap_test>����</cap_test>
        <page_number_fmt>���. %u �� %u</page_number_fmt>
        <short_page_number_fmt>���. %u</short_page_number_fmt>
        <oper_info>���� ��ନ஢�� $(date_format %d.%m.%Y) $(date_format %H:%M:%S) (���)
�����஬ PIKE
� �ନ���� ������</oper_info>
        <skip_header>0</skip_header>
        <stat_type>29</stat_type>
        <stat_mode>����. �뫥�</stat_mode>
        <stat_type_caption>���஡���</stat_type_caption>
      </variables>
    </form_data>
  </answer>
</term>


%%
#########################################################################################

###
#   ���� �5
#   �⥭�� ��娢� �� arx_stat_services.cc
#
######################################################

include(ts/sirena_exchange_macro.ts)

$(init_term)

$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive �� 100 ���)
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))

$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))

$(set pax_id_BURYAKOV $(get_pax_id $(get point_dep_UT_100) ������� "������� ����������"))

$(CHANGE_TRIP_SETS $(get point_dep_UT_100) piece_concept=1)
$(settcl SIRENA_HOST localhost)
$(settcl SIRENA_PORT 8008)

$(sql {INSERT INTO rfic_types(id, code, name, name_lat)
       VALUES(id__seq.nextval, 'A', '��', 'pro')})

$(sql {INSERT INTO rfisc_sets(id, airline, flt_no, airp_dep, rfic, rfisc, auto_checkin)
       VALUES(id__seq.nextval, '��', NULL, NULL, 'A', '0B5', 1)})

$(http_forecast content=$(get_svc_availability_resp))


#1�������/������� ����������
#.L/F451B5/UT
#.L/5C0NXZ/1H
#.R/TKNE HK1 2982425622093/1
#.R/SEAT HK1 10D
#.R/OTHS HK1 FQTSTATUS BRONZE
#.R/FQTV UT 1020894422
#.R/ASVC HI1 A/0B5/SEAT/��������������� ����� �����/A
#.RN//2984555892312C1
#.R/ASVC HI1 C/08A//������ ����� ��10�� 55�40�25��/A
#.RN//2984555892311C1
#.R/ASVC HI1 A/O6O//�������� � ������ ������/A/2984555892336C1
#.R/DOCS HK1/P/RU/4501742939/RU/02MAR75/M/$(ddmonyy +1y)/�������
#.RN//������� ����������
#.R/PSPT HK1 4501742939/RU/02MAR75/�������/������� ����������/M
#.R/FOID PP4501742939

!!
$(CHECKIN_PAX $(get pax_id_BURYAKOV) $(get point_dep_UT_100) $(get point_arv_UT_100)
  �� 100 ��� ��� ������� "������� ����������" 2982425622093 �� RU 4501742939 RU 02.03.1975 $(date_format %d.%m.%Y) M)

>> lines=auto
<query>
  <svc_availability show_brand_info=\"true\" show_all_svc=\"true\" show_free_carry_on_norm=\"true\">
    <passenger id=\"$(get pax_id_BURYAKOV)\" surname=\"�������\" name=\"������� ����������\" category=\"ADT\" birthdate=\"1975-03-02\" sex=\"male\">
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

$(dump_table rfisc_list_items)
$(dump_table pax_services_auto)
$(dump_table points)

$(run_arch_step $(ddmmyy +141))

$(are_tables_equal ARX_STAT_SERVICES)

!!capture = on
$(RUN_SERVICES_STAT $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21))

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...>
    <airline>��</airline>
    <grd>
      <header>
        <col width='50' align='0' sort='0'>��</col>
        <col width='50' align='0' sort='0'>��</col>
        <col width='75' align='0' sort='0'>����� ३�</col>
        <col width='50' align='0' sort='0'>���</col>
        <col width='150' align='0' sort='3'>�.�.�.</col>
        <col width='150' align='0' sort='3'>�����</col>
        <col width='50' align='0' sort='0'>��</col>
        <col width='50' align='0' sort='0'>��</col>
        <col width='30' align='0' sort='0'>RFIC</col>
        <col width='40' align='0' sort='0'>RFISC</col>
        <col width='100' align='0' sort='0'>� ���⠭樨</col>
      </header>
      <rows>
        <row>
          <col>���</col>
          <col>��</col>
          <col>100</col>
          <col>$(date_format %d.%m.%y +20)</col>
          <col>������� ������� ����������</col>
          <col>2982425622093/1</col>
          <col>���</col>
          <col>���</col>
          <col>A</col>
          <col>0B5</col>
          <col>2984555892312/1</col>
        </row>
      </rows>
    </grd>
    <form_data>
      <variables>
        <print_date>$(date_format %d.%m.%Y) $(date_format %H:%M:%S) (���)</print_date>
        <print_oper>PIKE</print_oper>
        <print_term>������</print_term>
        <use_seances>0</use_seances>
        <test_server>1</test_server>
        <cap_test>����</cap_test>
        <page_number_fmt>���. %u �� %u</page_number_fmt>
        <short_page_number_fmt>���. %u</short_page_number_fmt>
        <oper_info>���� ��ନ஢�� $(date_format %d.%m.%Y) $(date_format %H:%M:%S) (���)
�����஬ PIKE
� �ନ���� ������</oper_info>
        <skip_header>0</skip_header>
        <stat_type>34</stat_type>
        <stat_mode>��㣨</stat_mode>
        <stat_type_caption>���஡���</stat_type_caption>
      </variables>
    </form_data>
  </answer>
</term>


%%
#########################################################################################

###
#   ���� �6
#   �⥭�� ��娢� arx_events �� astra_utils.cc TRegEvents::fromDB 䠩�� stat_departed.cc
#   � �㭪樨 departed_flt()
######################################################

$(init_term)

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

$(dump_table points)
$(run_arch_step $(ddmmyy +141))

$(are_tables_equal ARX_POINTS)
$(are_tables_equal MOVE_ARX_EXT)

$(nosir_departed_flt 20201220 20210120)


%%
#########################################################################################

###
#   ���� �7
#   �롨����� ����� �� ��娢� �� ३ᠬ �� ⥪���� ����
#   � �㭪樨 internal_ReadData_N � sopp.cc
######################################################

$(init_term)

$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive �� 100 ���)
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))
$(set move_id $(get_move_id $(get point_dep_UT_100)))

# �� �㦭� ��� ⮣� �⮡� pr_brd = 1 � ⠡��� PAX,� ���� ��ᠤ��� ���ᠦ��
# � �� � ᢮� ��।� �㦭� �⮡� ����������� ⠡��� STAT_AD � ��⮬ ARX_STAT_AD
$(sql {INSERT INTO trip_hall(point_id, type, hall, pr_misc)
       VALUES($(get point_dep_UT_100), 101, NULL, 1)})

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100) �� 100 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

$(dump_table points)
$(dump_table airlines)

$(run_arch_step $(ddmmyy +141))

$(are_tables_equal ARX_POINTS)
$(are_tables_equal ARX_TRIP_CLASSES)
#$(are_tables_equal ARX_EVENTS order="ev_order, lang")

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
          <airp>���</airp>
          <airline_out>��</airline_out>
          <flt_no_out>100</flt_no_out>
          <craft_out>TU5</craft_out>
          <scd_out>$(date_format %d.%m.%Y +20) 10:15:00</scd_out>
          <triptype_out>�</triptype_out>
          <pr_reg>1</pr_reg>
          <places_out>
            <airp>���</airp>
          </places_out>
          <classes>
            <class cfg='11'>�</class>
            <class cfg='63'>�</class>
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
          <airline_in>��</airline_in>
          <flt_no_in>100</flt_no_in>
          <craft_in>TU5</craft_in>
          <scd_in>$(date_format %d.%m.%Y +20) 12:00:00</scd_in>
          <triptype_in>�</triptype_in>
          <places_in>
            <airp>���</airp>
          </places_in>
          <airp>���</airp>
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
#   ���� �8
#   �஢�ઠ �⥭�� �� ��娢��� ⠡��� �� ����᪥ basel_aero_flat_stat
######################################################

$(init_term)

$(sql {INSERT INTO file_sets(code, airp, name, dir, last_create, pr_denial)
       VALUES('BASEL_AERO', '���', 'BASEL_AERO', 'basel_aero/', TO_DATE('07.02.21', 'dd.mm.yy'), 0)})


$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive �� 100 ���)
$(make_spp $(ddmmyy +20))


$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))
$(set move_id $(get_move_id $(get point_dep_UT_100)))

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100) �� 100 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

$(run_arch_step $(ddmmyy +141))

$(nosir_basel_stat $(date_format %d.%m.%Y +20) 09:00:00 $(get point_dep_UT_100))

??
$(dump_table basel_stat display="on")
>> lines=auto
[���] [$(get pax_id_TUMALI)] [$(get point_dep_UT_100)] [$(yymmdd)] [NULL] [NULL] [0] [NULL] [1] [$(yymmdd)] [������] [NULL] [$(yymmdd +20)] [$(yymmdd +20)] [NULL] [��100] [...] [TUMALI/VALERII] [0] [0] [NULL] [NULL] [��ॣ����஢��] [NULL] [NULL] [0] $()


%%
#########################################################################################

###
#   ���� �9
#   �⥭�� ��娢� �� stat_arx.cc �㭪�� PaxListRun
#
###

$(init_term)

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

$(set grp_id $(get_single_grp_id $(get point_dep_UT_298) OZ OFER))
$(set grp_id2 $(get_single_grp_id $(get point_dep_UT_190) OZ OFER))
$(set grp_id3 $(get_single_grp_id $(get point_dep_UT_450) OZ OFER))

$(run_arch_step $(ddmmyy +151))

$(dump_table MOVE_ARX_EXT fields = "move_id, part_key, date_range")
$(dump_table ARX_TRFER_PAX_STAT)

$(dump_table arx_pax_grp)
$(dump_table arx_pax)
$(dump_table arx_points)

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
          <airline>��</airline>
          <flt_no>298</flt_no>
          <suffix/>
          <trip>��298 ���</trip>
          <scd_out>$(date_format %d.%m.%Y) 10:15:00</scd_out>
          <reg_no>1</reg_no>
          <full_name>OZ OFER</full_name>
          <bag_amount>0</bag_amount>
          <bag_weight>0</bag_weight>
          <rk_weight>0</rk_weight>
          <excess>0</excess>
          <grp_id>$(get grp_id)</grp_id>
          <airp_arv>���</airp_arv>
          <tags/>
          <status>��ॣ.</status>
          <class>�</class>
          <seat_no>05�</seat_no>
          <document>32427293 UKR</document>
          <ticket_no>2985523437721</ticket_no>
          <hall>��� 1</hall>
        </pax>
        <pax>
          <part_key>$(date_format %d.%m.%Y) 09:00:00</part_key>
          <point_id>$(get point_dep_UT_190)</point_id>
          <airline>��</airline>
          <flt_no>190</flt_no>
          <suffix/>
          <trip>��190 ���</trip>
          <scd_out>$(date_format %d.%m.%Y) 10:15:00</scd_out>
          <reg_no>1</reg_no>
          <full_name>OZ OFER</full_name>
          <bag_amount>0</bag_amount>
          <bag_weight>0</bag_weight>
          <rk_weight>0</rk_weight>
          <excess>0</excess>
          <grp_id>$(get grp_id2)</grp_id>
          <airp_arv>���</airp_arv>
          <tags/>
          <status>��ॣ.</status>
          <class>�</class>
          <seat_no>05�</seat_no>
          <document>32427293 UKR</document>
          <ticket_no>2985523437721</ticket_no>
          <hall>��� 1</hall>
        </pax>
        <pax>
          <part_key>$(date_format %d.%m.%Y) 09:00:00</part_key>
          <point_id>$(get point_dep_UT_450)</point_id>
          <airline>��</airline>
          <flt_no>450</flt_no>
          <suffix/>
          <trip>��450 ���</trip>
          <scd_out>$(date_format %d.%m.%Y) 10:15:00</scd_out>
          <reg_no>1</reg_no>
          <full_name>OZ OFER</full_name>
          <bag_amount>0</bag_amount>
          <bag_weight>0</bag_weight>
          <rk_weight>0</rk_weight>
          <excess>0</excess>
          <grp_id>$(get grp_id3)</grp_id>
          <airp_arv>���</airp_arv>
          <tags/>
          <status>��ॣ.</status>
          <class>�</class>
          <seat_no>05�</seat_no>
          <document>32427293 UKR</document>
          <ticket_no>2985523437721</ticket_no>
          <hall>��� 1</hall>
        </pax>
      </rows>
      <header>
        <col>����</col>
      </header>
    </paxList>
    <form_data>
      <variables>
        <print_date>$(date_format %d.%m.%Y) $(date_format %H:%M:%S) (���)</print_date>
        <print_oper>PIKE</print_oper>
        <print_term>������</print_term>
        <use_seances>0</use_seances>
        <test_server>1</test_server>
        <cap_test>����</cap_test>
        <page_number_fmt>���. %u �� %u</page_number_fmt>
        <short_page_number_fmt>���. %u</short_page_number_fmt>
        <oper_info>���� ��ନ஢�� $(date_format %d.%m.%Y) $(date_format %H:%M:%S) (���)
�����஬ PIKE
� �ନ���� ������</oper_info>
        <skip_header>0</skip_header>
      </variables>
    </form_data>
    <FormNotExists>ArxPaxList</FormNotExists>
    <form name='ArxPaxList'/>
  </answer>
</term>

%%
#########################################################################################

###
#   ���� �10
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#
#   �⥭�� ��娢� �� stat_arx.cc �㭪樨 FltTaskLogRun
###
#########################################################################################

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD �� ��� ��� 300 1003 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD �� ��� ��� 100 1004 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +267))
$(make_spp $(ddmmyy +265))

$(set point_dep $(get_dep_point_id ��� �� 300 $(yymmdd +1)))

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")
$(run_arch_step $(ddmmyy +387))
$(dump_table ARX_POINTS)

!! capture=on
$(RUN_FLT_TASK_LOG $(get point_dep) $(date_format %d.%m.%Y +1))

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...>
    <FormNotExists>FltTaskLog</FormNotExists>
    <form name='FltTaskLog'/>
    <form_data>
      <variables>
        <print_date>$(date_format %d.%m.%Y) $(date_format %H:%M:%S) (���)</print_date>
        <print_oper>PIKE</print_oper>
        <print_term>������</print_term>
        <use_seances>0</use_seances>
        <test_server>1</test_server>
        <cap_test>����</cap_test>
        <page_number_fmt>���. %u �� %u</page_number_fmt>
        <short_page_number_fmt>���. %u</short_page_number_fmt>
        <oper_info>���� ��ନ஢�� $(date_format %d.%m.%Y) $(date_format %H:%M:)... (���)
�����஬ PIKE
� �ନ���� ������</oper_info>
        <skip_header>0</skip_header>
        <report_title>��ୠ� ����� ३�</report_title>
      </variables>
    </form_data>
    <PaxLog>
      <header>
        <col>�����</col>
      </header>
      <rows>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>$(date_format %d.%m.%Y) $(date_format %H:%M:)...</time>
          <msg>����� EMD_REFRESH &lt;CloseCheckIn 0&gt; ᮧ����; ����. ��.: $(date_format %d.%m.%y +1) 06:35:00 (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
      </rows>
    </PaxLog>
    <airline>��</airline>
  </answer>
</term>


%%
#########################################################################################

###
#   ���� �11
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#
#   �⥭�� ��娢� �� stat_arx.cc �㭪樨 FltLogRun
###
#########################################################################################

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD �� ��� ��� 300 1003 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD �� ��� ��� 100 1004 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +267))
$(make_spp $(ddmmyy +265))

$(set point_dep $(get_dep_point_id ��� �� 300 $(yymmdd +1)))

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")
$(run_arch_step $(ddmmyy +387))
$(dump_table ARX_POINTS)

!! capture=on
$(RUN_FLT_LOG $(get point_dep) $(date_format %d.%m.%Y +1))

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...>
    <FormNotExists>FltLog</FormNotExists>
    <form name='FltLog'/>
    <form_data>
      <variables>
        <print_date>$(date_format %d.%m.%Y) $(date_format %H:%M:%S) (���)</print_date>
        <print_oper>PIKE</print_oper>
        <print_term>������</print_term>
        <use_seances>0</use_seances>
        <test_server>1</test_server>
        <cap_test>����</cap_test>
        <page_number_fmt>���. %u �� %u</page_number_fmt>
        <short_page_number_fmt>���. %u</short_page_number_fmt>
        <oper_info>���� ��ନ஢�� $(date_format %d.%m.%Y) $(date_format %H:%M:%S) (���)
�����஬ PIKE
� �ନ���� ������</oper_info>
        <skip_header>0</skip_header>
        <report_title>��ୠ� ����権 ३�</report_title>
      </variables>
    </form_data>
    <PaxLog>
      <header>
        <col>�����</col>
      </header>
      <rows>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>$(date_format %d.%m.%Y) $(date_format %H:%M:)...</time>
          <msg>�� ३� ����饭� web-ॣ������.</msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>$(date_format %d.%m.%Y) $(date_format %H:%M:)...</time>
          <msg>�⠯ '�����⮢�� � ॣ����樨': ����. �६� 00:15 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>$(date_format %d.%m.%Y) $(date_format %H:%M:)...</time>
          <msg>�⠯ '����⨥ ॣ����樨': ����. �६� 04:14 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>$(date_format %d.%m.%Y) $(date_format %H:%M:)...</time>
          <msg>�⠯ '����⨥ web-ॣ����樨': ����. �६� 07:15 $(date_format %d.%m.%y) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>$(date_format %d.%m.%Y) $(date_format %H:%M:)...</time>
          <msg>�⠯ '����⨥ kiosk-ॣ����樨': ����. �६� 07:15 $(date_format %d.%m.%y) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>$(date_format %d.%m.%Y) $(date_format %H:%M:)...</time>
          <msg>�⠯ '�����⨥ ॣ����樨': ����. �६� 06:35 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>$(date_format %d.%m.%Y) $(date_format %H:%M:)...</time>
          <msg>�⠯ '����� �⬥�� web-ॣ����樨': ����. �६� 06:25 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>$(date_format %d.%m.%Y) $(date_format %H:%M:)...</time>
          <msg>�⠯ '�����⨥ web-ॣ����樨': ����. �६� 04:15 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>$(date_format %d.%m.%Y) $(date_format %H:%M:)...</time>
          <msg>�⠯ '�����⨥ kiosk-ॣ����樨': ����. �६� 05:15 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>$(date_format %d.%m.%Y) $(date_format %H:%M:)...</time>
          <msg>�⠯ '��⮢����� �� � ��ᠤ��': ����. �६� 06:30 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>$(date_format %d.%m.%Y) $(date_format %H:%M:)...</time>
          <msg>�⠯ '����砭�� ��ᠤ�� (��ଫ���� ����.)': ����. �६� 06:50 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>$(date_format %d.%m.%Y) $(date_format %H:%M:)...</time>
          <msg>�⠯ '�⪠� �࠯�': ����. �६� 07:00 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>$(date_format %d.%m.%Y) $(date_format %H:%M:)...</time>
          <msg>�����祭�� ��ᮢ ���ᠦ�஢ �� ३�: </msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>$(date_format %d.%m.%Y) $(date_format %H:%M:)...</time>
          <msg>���� ����⨪� �� ३��</msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>$(date_format %d.%m.%Y) $(date_format %H:%M:)...</time>
          <msg>���� ��६�饭 � ��娢</msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
      </rows>
    </PaxLog>
    <airline>��</airline>
  </answer>
</term>

%%
#########################################################################################

###
#   ���� �12
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#
#   �⥭�� ��娢� �� stat_utils.cc �㭪樨 FltCBoxDropDown
###
#########################################################################################

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD �� ��� ��� 300 1003 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD �� ��� ��� 100 1004 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +267))
$(make_spp $(ddmmyy +265))

$(set point_dep_300 $(get_dep_point_id ��� �� 300 $(yymmdd +1)))
$(set point_dep_100 $(get_dep_point_id ��� �� 100 $(yymmdd +265)))

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")
$(run_arch_step $(ddmmyy +387))
$(dump_table ARX_POINTS)

!! capture=on
$(RUN_FLT_CBOX_DROP_DOWN $(date_format %d.%m.%Y ) $(date_format %d.%m.%Y +266))

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...>
    <cbox>
      <f>
        <name>��100  /$(date_format %d.%m +265 ) ���</name>
        <point_id>$(get point_dep_100)</point_id>
        <part_key>$(date_format %d.%m.%Y +265) 09:00:00</part_key>
      </f>
      <f>
        <name>��300  /$(date_format %d +1) ���</name>
        <point_id>$(get point_dep_300)</point_id>
        <part_key>$(date_format %d.%m.%Y +1) 09:00:00</part_key>
      </f>
    </cbox>
  </answer>
</term>
