include(ts/macro.ts)
include(ts/adm_macro.ts)
include(ts/fr_forms.ts)
include(ts/spp/read_trips_macro.ts)
include(ts/pax/checkin_macro.ts)

# meta: suite readarch

#########################################################################################
###
#   ���� �1
#   �⥭�� �� �㭪樨 RunTrferPaxStat �� ����⨪� stat_trfer_pax.cc
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


$(NEW_UNACCOMP_REQUEST capture=off lang=EN hall=1 ""
{$(NEW_UNACCOMP_SEGMENT $(get point_dep_UT_298) $(get point_arv_UT_298) ��� ���)})

$(set grp_id_unacc1 $(get_unaccomp_id $(get point_dep_UT_298) 1))

$(CHANGE_UNACCOMP_REQUEST capture=off lang=EN hall=1
{$(CHANGE_UNACCOMP_SEGMENT $(get point_dep_UT_298) $(get point_arv_UT_298) ��� ���
                           $(get grp_id_unacc1) $(get_unaccomp_tid $(get grp_id_unacc1)))}
{<value_bags/>
<bags>
$(BAG_WT 1 "" �� pr_cabin=1 amount=1  weight=11  bag_pool_num=1)
$(BAG_WT 2 "" �� pr_cabin=0 amount=3  weight=24  bag_pool_num=1)
</bags>
<tags pr_print=\"0\">
$(TAG 1 RUCH 1298401555 bag_num=2 color=��)
$(TAG 2 RUCH 1298401556 bag_num=2 color=�)
$(TAG 3 RUCH 0298401557 bag_num=2 color=�)
</tags>}
)


!!
$(CHECKIN_PAX_TRANSFER $(get pax_id1) $(get point_dep_UT_298) $(get point_arv_UT_298) �� 298 ��� ��� OZ OFER 2985523437721
                       $(get pax_id2) $(get point_dep_UT_190) $(get point_arv_UT_190) �� 190 ��� ��� OZ OFER
                       $(get pax_id3) $(get point_dep_UT_450) $(get point_arv_UT_450) �� 450 ��� ��� OZ OFER
                       �� UA 32427293 UA 16.04.1968 25.06.2025 M)

$(exec_stage $(get point_dep_UT_298) Takeoff)

$(db_dump_table TRFER_PAX_STAT)

!! capture=on
$(RUN_TRFER_PAX_STAT $(date_format %d.%m.%Y -1) $(date_format %d.%m.%Y +21))

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
        <print_date>... (���)</print_date>
        <print_oper>PIKE</print_oper>
        <print_term>������</print_term>
        <use_seances>0</use_seances>
        <test_server>1</test_server>
        <cap_test>����</cap_test>
        <page_number_fmt>���. %u �� %u</page_number_fmt>
        <short_page_number_fmt>���. %u</short_page_number_fmt>
        <oper_info>���� ��ନ஢�� ... (���)
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

$(NEW_UNACCOMP_REQUEST capture=off lang=EN hall=1 ""
{$(NEW_UNACCOMP_SEGMENT $(get point_dep_UT_298) $(get point_arv_UT_298) ��� ���)})

$(set grp_id_unacc1 $(get_unaccomp_id $(get point_dep_UT_298) 1))

$(CHANGE_UNACCOMP_REQUEST capture=off lang=EN hall=1
{$(CHANGE_UNACCOMP_SEGMENT $(get point_dep_UT_298) $(get point_arv_UT_298) ��� ���
                           $(get grp_id_unacc1) $(get_unaccomp_tid $(get grp_id_unacc1)))}
{<value_bags/>
<bags>
$(BAG_WT 1 "" �� pr_cabin=1 amount=1  weight=11  bag_pool_num=1)
$(BAG_WT 2 "" �� pr_cabin=0 amount=3  weight=24  bag_pool_num=1)
</bags>
<tags pr_print=\"0\">
$(TAG 1 RUCH 1298401555 bag_num=2 color=��)
$(TAG 2 RUCH 1298401556 bag_num=2 color=�)
$(TAG 3 RUCH 0298401557 bag_num=2 color=�)
</tags>}
)

!!
$(CHECKIN_PAX_TRANSFER $(get pax_id1) $(get point_dep_UT_298) $(get point_arv_UT_298) �� 298 ��� ��� OZ OFER 2985523437721
                       $(get pax_id2) $(get point_dep_UT_190) $(get point_arv_UT_190) �� 190 ��� ��� OZ OFER
                       $(get pax_id3) $(get point_dep_UT_450) $(get point_arv_UT_450) �� 450 ��� ��� OZ OFER
                       �� UA 32427293 UA 16.04.1968 25.06.2025 M)

$(set grp_id $(get_single_grp_id $(get point_dep_UT_298) OZ OFER))


!! capture=on
$(PAX_LIST_RUN  $(get point_dep_UT_298))

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...>
    <paxList>
      <rows>
        <pax>
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
          <tags/>
          <seat_no>5�</seat_no>
          <grp_id>$(get grp_id)</grp_id>
          <airp_arv>���</airp_arv>
          <status>��ॣ.</status>
          <class>�</class>
          <document>32427293 UKR</document>
          <ticket_no>2985523437721</ticket_no>
          <hall>��� 1</hall>
        </pax>
        <pax>
          <point_id>$(get point_dep_UT_298)</point_id>
          <airline/>
          <flt_no>0</flt_no>
          <suffix/>
          <trip>��298 ���</trip>
          <scd_out>$(date_format %d.%m.%Y) 10:15:00</scd_out>
          <reg_no>0</reg_no>
          <full_name>����� ��� ᮯ஢�������</full_name>
          <bag_amount>3</bag_amount>
          <bag_weight>24</bag_weight>
          <rk_weight>11</rk_weight>
          <excess>35</excess>
          <tags>�1298401556, �0298401557, ��1298401555</tags>
          <grp_id>...</grp_id>
          <airp_arv>���</airp_arv>
          <status/>
          <class/>
          <seat_no/>
          <document/>
          <ticket_no/>
          <hall>��� 1</hall>
        </pax>
      </rows>
      <header>
        <col>����</col>
      </header>
    </paxList>
    <form_data>
      <variables>
        <print_date>... (���)</print_date>
        <print_oper>PIKE</print_oper>
        <print_term>������</print_term>
        <use_seances>0</use_seances>
        <test_server>1</test_server>
        <cap_test>����</cap_test>
        <page_number_fmt>���. %u �� %u</page_number_fmt>
        <short_page_number_fmt>���. %u</short_page_number_fmt>
        <oper_info>���� ��ନ஢�� ... (���)
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
#   ���� �3
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

!! capture=on
$(RUN_FLT_TASK_LOG $(get point_dep))

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...>
    <form name='FltTaskLog'...>$(FltTaskLogForm)
</form>
    <form_data>
      <variables>
        <print_date>... (���)</print_date>
        <print_oper>PIKE</print_oper>
        <print_term>������</print_term>
        <use_seances>0</use_seances>
        <test_server>1</test_server>
        <cap_test>����</cap_test>
        <page_number_fmt>���. %u �� %u</page_number_fmt>
        <short_page_number_fmt>���. %u</short_page_number_fmt>
        <oper_info>���� ��ନ஢�� ... (���)
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
          <time>...</time>
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
#   ���� �4
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


!! capture=on
$(RUN_FLT_LOG $(get point_dep))

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...>
    <form name='FltLog'...>$(FltLogForm)
</form>
    <form_data>
      <variables>
        <print_date>... (���)</print_date>
        <print_oper>PIKE</print_oper>
        <print_term>������</print_term>
        <use_seances>0</use_seances>
        <test_server>1</test_server>
        <cap_test>����</cap_test>
        <page_number_fmt>���. %u �� %u</page_number_fmt>
        <short_page_number_fmt>���. %u</short_page_number_fmt>
        <oper_info>���� ��ନ஢�� ... (���)
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
          <time>...</time>
          <msg>�� ३� ����饭� web-ॣ������.</msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>�⠯ '�����⮢�� � ॣ����樨': ����. �६� 00:15 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>�⠯ '����⨥ ॣ����樨': ����. �६� 04:14 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>�⠯ '����⨥ web-ॣ����樨': ����. �६� 07:15 $(date_format %d.%m.%y) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>�⠯ '����⨥ kiosk-ॣ����樨': ����. �६� 07:15 $(date_format %d.%m.%y) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>�⠯ '�����⨥ ॣ����樨': ����. �६� 06:35 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>�⠯ '����� �⬥�� web-ॣ����樨': ����. �६� 06:25 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>�⠯ '�����⨥ web-ॣ����樨': ����. �६� 04:15 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>�⠯ '�����⨥ kiosk-ॣ����樨': ����. �६� 05:15 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>�⠯ '��⮢����� �� � ��ᠤ��': ����. �६� 06:30 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>�⠯ '����砭�� ��ᠤ�� (��ଫ���� ����.)': ����. �६� 06:50 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>�⠯ '�⪠� �࠯�': ����. �६� 07:00 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
          <screen>���������</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>�����祭�� ��ᮢ ���ᠦ�஢ �� ३�: </msg>
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
#   ���� �5
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#
#   �⥭�� ��娢� �� stat_arx.cc �㭪樨 LogRun
###
#########################################################################################

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(make_spp $(ddmmyy +20))
$(deny_ets_interactive �� 100 ���)
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep) $(get point_arv) �� 100 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))

!! capture=on
$(RUN_LOG_RUN $(get point_dep) $(get grp_id) 1)
>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...>
    <form_data>
      <variables>
        <print_date>...</print_date>
        <print_oper>PIKE</print_oper>
        <print_term>������</print_term>
        <use_seances>0</use_seances>
        <test_server>1</test_server>
        <cap_test>����</cap_test>
        <page_number_fmt>���. %u �� %u</page_number_fmt>
        <short_page_number_fmt>���. %u</short_page_number_fmt>
        <oper_info>���� ��ନ஢�� ... (���)
�����஬ PIKE
� �ନ���� ������</oper_info>
        <skip_header>0</skip_header>
        <report_title>����樨 �� ���ᠦ���</report_title>
      </variables>
    </form_data>
    <PaxLog>
      <header>
        <col>�����</col>
      </header>
      <rows>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>���ᠦ�� TUMALI VALERII (��) ��ॣ����஢��. �/�: ���, �����: �, �����: �஭�, ����: 5�. ���.����: ���</msg>
          <ev_order>...</ev_order>
          <grp_id>$(get grp_id)</grp_id>
          <reg_no>1</reg_no>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>���ᠦ�� TUMALI VALERII (��). DOCS: P/UKR/FA144642/UKR/16APR68/M/25JUN25/TUMALI/VALERII/. ��筮� ����</msg>
          <ev_order>...</ev_order>
          <grp_id>$(get grp_id)</grp_id>
          <reg_no>1</reg_no>
          <ev_user>������� �.�.</ev_user>
          <station>������</station>
        </row>
      </rows>
    </PaxLog>
    <airline>��</airline>
  </answer>
</term>

%%
#########################################################################################

###
#   ���� �6
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#
#   �⥭�� ��娢� �� stat_general.cc
###
#########################################################################################

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(make_spp $(ddmmyy +20))
$(deny_ets_interactive �� 100 ���)
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep) $(get point_arv) �� 100 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))

$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ��⠫���஢����� ����)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ��⠫���஢����� ����ॣ������)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ��⠫���஢����� "���. ⥫��ࠬ��")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ���� ����)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ���� "���. ⥫��ࠬ��")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ���� �������)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ���� ����ॣ������)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ���� "�� ����⠬")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ���஡��� "���. ⥫��ࠬ��")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ���஡��� ����ॣ������)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ���஡��� ����)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ���஡��� "�� ����⠬")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ���஡��� "������� RFISC")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ���஡��� "���. ������.")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ���஡��� "����. ��ન")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ���஡��� PFS)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ���஡��� �࠭���)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ���஡��� ���ᥫ����)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ���஡��� "������ �ਣ��襭��")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ���஡��� ������)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ���஡��� "����. �뫥�")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ���஡��� ���ਭ�)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ���஡��� ��㣨)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ���஡��� ����ન)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ���஡��� "��������� ᠫ���")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ���஡��� "SBDO (Zamar)")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) �⮣� "�� ����⠬")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) ���஡��� "��ᮯ�. �����")
