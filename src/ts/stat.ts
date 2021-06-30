include(ts/macro.ts)
include(ts/adm_macro.ts)
include(ts/fr_forms.ts)
include(ts/spp/read_trips_macro.ts)
include(ts/pax/checkin_macro.ts)

# meta: suite readarch

#########################################################################################
###
#   Тест №1
#   Чтение из функции RunTrferPaxStat из статистики stat_trfer_pax.cc
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


$(NEW_UNACCOMP_REQUEST capture=off lang=EN hall=1 ""
{$(NEW_UNACCOMP_SEGMENT $(get point_dep_UT_298) $(get point_arv_UT_298) СОЧ ПРХ)})

$(set grp_id_unacc1 $(get_unaccomp_id $(get point_dep_UT_298) 1))

$(CHANGE_UNACCOMP_REQUEST capture=off lang=EN hall=1
{$(CHANGE_UNACCOMP_SEGMENT $(get point_dep_UT_298) $(get point_arv_UT_298) СОЧ ПРХ
                           $(get grp_id_unacc1) $(get_unaccomp_tid $(get grp_id_unacc1)))}
{<value_bags/>
<bags>
$(BAG_WT 1 "" ЮТ pr_cabin=1 amount=1  weight=11  bag_pool_num=1)
$(BAG_WT 2 "" ЮТ pr_cabin=0 amount=3  weight=24  bag_pool_num=1)
</bags>
<tags pr_print=\"0\">
$(TAG 1 RUCH 1298401555 bag_num=2 color=СИ)
$(TAG 2 RUCH 1298401556 bag_num=2 color=Ж)
$(TAG 3 RUCH 0298401557 bag_num=2 color=О)
</tags>}
)


!!
$(CHECKIN_PAX_TRANSFER $(get pax_id1) $(get point_dep_UT_298) $(get point_arv_UT_298) ЮТ 298 СОЧ ПРХ OZ OFER 2985523437721
                       $(get pax_id2) $(get point_dep_UT_190) $(get point_arv_UT_190) ЮТ 190 ПРХ АМС OZ OFER
                       $(get pax_id3) $(get point_dep_UT_450) $(get point_arv_UT_450) ЮТ 450 АМС ЛХР OZ OFER
                       ВЗ UA 32427293 UA 16.04.1968 25.06.2025 M)

$(exec_stage $(get point_dep_UT_298) Takeoff)

$(db_dump_table TRFER_PAX_STAT)

!! capture=on
$(RUN_TRFER_PAX_STAT $(date_format %d.%m.%Y -1) $(date_format %d.%m.%Y +21))

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...>
    <airline>ЮТ</airline>
    <grd>
      <header>
        <col width='60' align='0' sort='0'>АК</col>
        <col width='30' align='0' sort='0'>АПВ</col>
        <col width='50' align='0' sort='0'>Сег.1</col>
        <col width='55' align='0' sort='0'>Дата</col>
        <col width='30' align='0' sort='0'>АПТ</col>
        <col width='60' align='0' sort='0'>Сег.2</col>
        <col width='55' align='0' sort='0'>Дата</col>
        <col width='30' align='0' sort='0'>АПП</col>
        <col width='60' align='0' sort='0'>Категория</col>
        <col width='60' align='0' sort='0'>ФИО пассажира</col>
        <col width='70' align='0' sort='0'>Документ</col>
        <col width='60' align='0' sort='0'>П</col>
        <col width='60' align='0' sort='0'>ВЗ</col>
        <col width='60' align='0' sort='0'>РБ</col>
        <col width='60' align='0' sort='0'>РМ</col>
        <col width='60' align='0' sort='0'>Р/к</col>
        <col width='60' align='0' sort='0'>БГ мест</col>
        <col width='60' align='0' sort='0'>БГ вес</col>
        <col width='90' align='0' sort='0'>Бирки</col>
      </header>
      <rows>
        <row>
          <col>ЮТ</col>
          <col>СОЧ</col>
          <col>298</col>
          <col>$(date_format %d.%m.%y)</col>
          <col>ПРХ</col>
          <col>ЮТ190</col>
          <col>$(date_format %d.%m.%y)</col>
          <col>АМС</col>
          <col>МВЛ-МВЛ</col>
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
          <col>ЮТ</col>
          <col>ПРХ</col>
          <col>190</col>
          <col>$(date_format %d.%m.%y)</col>
          <col>АМС</col>
          <col>ЮТ450</col>
          <col>$(date_format %d.%m.%y)</col>
          <col>ЛХР</col>
          <col>МВЛ-МВЛ</col>
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
          <col>Итого:</col>
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
        <print_date>... (МОВ)</print_date>
        <print_oper>PIKE</print_oper>
        <print_term>МОВРОМ</print_term>
        <use_seances>0</use_seances>
        <test_server>1</test_server>
        <cap_test>ТЕСТ</cap_test>
        <page_number_fmt>Стр. %u из %u</page_number_fmt>
        <short_page_number_fmt>Стр. %u</short_page_number_fmt>
        <oper_info>Отчет сформирован ... (МОВ)
оператором PIKE
с терминала МОВРОМ</oper_info>
        <skip_header>0</skip_header>
        <stat_type>21</stat_type>
        <stat_mode>Трансфер</stat_mode>
        <stat_type_caption>Подробная</stat_type_caption>
      </variables>
    </form_data>
  </answer>
</term>


%%
#########################################################################################

###
#   Тест №2
#   Чтение архива из stat_arx.cc Функция PaxListRun
#
###

$(init_jxt_pult МОВРОМ)
$(set_desk_version 201707-0195750)
$(login)

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

$(NEW_UNACCOMP_REQUEST capture=off lang=EN hall=1 ""
{$(NEW_UNACCOMP_SEGMENT $(get point_dep_UT_298) $(get point_arv_UT_298) СОЧ ПРХ)})

$(set grp_id_unacc1 $(get_unaccomp_id $(get point_dep_UT_298) 1))

$(CHANGE_UNACCOMP_REQUEST capture=off lang=EN hall=1
{$(CHANGE_UNACCOMP_SEGMENT $(get point_dep_UT_298) $(get point_arv_UT_298) СОЧ ПРХ
                           $(get grp_id_unacc1) $(get_unaccomp_tid $(get grp_id_unacc1)))}
{<value_bags/>
<bags>
$(BAG_WT 1 "" ЮТ pr_cabin=1 amount=1  weight=11  bag_pool_num=1)
$(BAG_WT 2 "" ЮТ pr_cabin=0 amount=3  weight=24  bag_pool_num=1)
</bags>
<tags pr_print=\"0\">
$(TAG 1 RUCH 1298401555 bag_num=2 color=СИ)
$(TAG 2 RUCH 1298401556 bag_num=2 color=Ж)
$(TAG 3 RUCH 0298401557 bag_num=2 color=О)
</tags>}
)

!!
$(CHECKIN_PAX_TRANSFER $(get pax_id1) $(get point_dep_UT_298) $(get point_arv_UT_298) ЮТ 298 СОЧ ПРХ OZ OFER 2985523437721
                       $(get pax_id2) $(get point_dep_UT_190) $(get point_arv_UT_190) ЮТ 190 ПРХ АМС OZ OFER
                       $(get pax_id3) $(get point_dep_UT_450) $(get point_arv_UT_450) ЮТ 450 АМС ЛХР OZ OFER
                       ВЗ UA 32427293 UA 16.04.1968 25.06.2025 M)

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
          <airline>ЮТ</airline>
          <flt_no>298</flt_no>
          <suffix/>
          <trip>ЮТ298 СОЧ</trip>
          <scd_out>$(date_format %d.%m.%Y) 10:15:00</scd_out>
          <reg_no>1</reg_no>
          <full_name>OZ OFER</full_name>
          <bag_amount>0</bag_amount>
          <bag_weight>0</bag_weight>
          <rk_weight>0</rk_weight>
          <excess>0</excess>
          <tags/>
          <seat_no>5А</seat_no>
          <grp_id>$(get grp_id)</grp_id>
          <airp_arv>ПРХ</airp_arv>
          <status>Зарег.</status>
          <class>Э</class>
          <document>32427293 UKR</document>
          <ticket_no>2985523437721</ticket_no>
          <hall>Зал 1</hall>
        </pax>
        <pax>
          <point_id>$(get point_dep_UT_298)</point_id>
          <airline/>
          <flt_no>0</flt_no>
          <suffix/>
          <trip>ЮТ298 СОЧ</trip>
          <scd_out>$(date_format %d.%m.%Y) 10:15:00</scd_out>
          <reg_no>0</reg_no>
          <full_name>Багаж без сопровождения</full_name>
          <bag_amount>3</bag_amount>
          <bag_weight>24</bag_weight>
          <rk_weight>11</rk_weight>
          <excess>35</excess>
          <tags>Ж1298401556, О0298401557, СИ1298401555</tags>
          <grp_id>...</grp_id>
          <airp_arv>ПРХ</airp_arv>
          <status/>
          <class/>
          <seat_no/>
          <document/>
          <ticket_no/>
          <hall>Зал 1</hall>
        </pax>
      </rows>
      <header>
        <col>Рейс</col>
      </header>
    </paxList>
    <form_data>
      <variables>
        <print_date>... (МОВ)</print_date>
        <print_oper>PIKE</print_oper>
        <print_term>МОВРОМ</print_term>
        <use_seances>0</use_seances>
        <test_server>1</test_server>
        <cap_test>ТЕСТ</cap_test>
        <page_number_fmt>Стр. %u из %u</page_number_fmt>
        <short_page_number_fmt>Стр. %u</short_page_number_fmt>
        <oper_info>Отчет сформирован ... (МОВ)
оператором PIKE
с терминала МОВРОМ</oper_info>
        <skip_header>0</skip_header>
      </variables>
    </form_data>
  </answer>
</term>


%%
#########################################################################################
###
#   Тест №3
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#
#   Чтение архива из stat_arx.cc функции FltTaskLogRun
###
#########################################################################################

$(init_jxt_pult МОВРОМ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD ЮТ АМС ПРХ 300 1003 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD ЮТ СОЧ ЛХР 100 1004 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +267))
$(make_spp $(ddmmyy +265))

$(set point_dep $(get_dep_point_id АМС ЮТ 300 $(yymmdd +1)))

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
        <print_date>... (МОВ)</print_date>
        <print_oper>PIKE</print_oper>
        <print_term>МОВРОМ</print_term>
        <use_seances>0</use_seances>
        <test_server>1</test_server>
        <cap_test>ТЕСТ</cap_test>
        <page_number_fmt>Стр. %u из %u</page_number_fmt>
        <short_page_number_fmt>Стр. %u</short_page_number_fmt>
        <oper_info>Отчет сформирован ... (МОВ)
оператором PIKE
с терминала МОВРОМ</oper_info>
        <skip_header>0</skip_header>
        <report_title>Журнал задач рейса</report_title>
      </variables>
    </form_data>
    <PaxLog>
      <header>
        <col>Агент</col>
      </header>
      <rows>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Задача EMD_REFRESH &lt;CloseCheckIn 0&gt; создана; План. вр.: $(date_format %d.%m.%y +1) 06:35:00 (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>Регистрация</screen>
        </row>
      </rows>
    </PaxLog>
    <airline>ЮТ</airline>
  </answer>
</term>


%%
#########################################################################################

###
#   Тест №4
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#
#   Чтение архива из stat_arx.cc функции FltLogRun
###
#########################################################################################

$(init_jxt_pult МОВРОМ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD ЮТ АМС ПРХ 300 1003 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD ЮТ СОЧ ЛХР 100 1004 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +267))
$(make_spp $(ddmmyy +265))

$(set point_dep $(get_dep_point_id АМС ЮТ 300 $(yymmdd +1)))


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
        <print_date>... (МОВ)</print_date>
        <print_oper>PIKE</print_oper>
        <print_term>МОВРОМ</print_term>
        <use_seances>0</use_seances>
        <test_server>1</test_server>
        <cap_test>ТЕСТ</cap_test>
        <page_number_fmt>Стр. %u из %u</page_number_fmt>
        <short_page_number_fmt>Стр. %u</short_page_number_fmt>
        <oper_info>Отчет сформирован ... (МОВ)
оператором PIKE
с терминала МОВРОМ</oper_info>
        <skip_header>0</skip_header>
        <report_title>Журнал операций рейса</report_title>
      </variables>
    </form_data>
    <PaxLog>
      <header>
        <col>Агент</col>
      </header>
      <rows>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>На рейсе запрещена web-регистрация.</msg>
          <ev_order>...</ev_order>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>Регистрация</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Этап 'Подготовка к регистрации': план. время 00:15 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>Регистрация</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Этап 'Открытие регистрации': план. время 04:14 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>Регистрация</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Этап 'Открытие web-регистрации': план. время 07:15 $(date_format %d.%m.%y) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>Регистрация</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Этап 'Открытие kiosk-регистрации': план. время 07:15 $(date_format %d.%m.%y) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>Регистрация</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Этап 'Закрытие регистрации': план. время 06:35 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>Регистрация</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Этап 'Запрет отмены web-регистрации': план. время 06:25 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>Регистрация</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Этап 'Закрытие web-регистрации': план. время 04:15 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>Регистрация</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Этап 'Закрытие kiosk-регистрации': план. время 05:15 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>Регистрация</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Этап 'Готовность ВС к посадке': план. время 06:30 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>Регистрация</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Этап 'Окончание посадки (оформление докум.)': план. время 06:50 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>Регистрация</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Этап 'Откат трапа': план. время 07:00 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>Регистрация</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Назначение весов пассажиров на рейс: </msg>
          <ev_order>...</ev_order>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>Регистрация</screen>
        </row>
      </rows>
    </PaxLog>
    <airline>ЮТ</airline>
  </answer>
</term>


%%
#########################################################################################

###
#   Тест №5
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#
#   Чтение архива из stat_arx.cc функции LogRun
###
#########################################################################################

$(init_jxt_pult МОВРОМ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD ЮТ СОЧ ЛХР 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(make_spp $(ddmmyy +20))
$(deny_ets_interactive ЮТ 100 СОЧ)
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep) $(get point_arv) ЮТ 100 СОЧ ЛХР TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

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
        <print_term>МОВРОМ</print_term>
        <use_seances>0</use_seances>
        <test_server>1</test_server>
        <cap_test>ТЕСТ</cap_test>
        <page_number_fmt>Стр. %u из %u</page_number_fmt>
        <short_page_number_fmt>Стр. %u</short_page_number_fmt>
        <oper_info>Отчет сформирован ... (МОВ)
оператором PIKE
с терминала МОВРОМ</oper_info>
        <skip_header>0</skip_header>
        <report_title>Операции по пассажиру</report_title>
      </variables>
    </form_data>
    <PaxLog>
      <header>
        <col>Агент</col>
      </header>
      <rows>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Пассажир TUMALI VALERII (ВЗ) зарегистрирован. П/н: ЛХР, класс: Э, статус: Бронь, место: 5Г. Баг.нормы: нет</msg>
          <ev_order>...</ev_order>
          <grp_id>$(get grp_id)</grp_id>
          <reg_no>1</reg_no>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Пассажир TUMALI VALERII (ВЗ). DOCS: P/UKR/FA144642/UKR/16APR68/M/25JUN25/TUMALI/VALERII/. Ручной ввод</msg>
          <ev_order>...</ev_order>
          <grp_id>$(get grp_id)</grp_id>
          <reg_no>1</reg_no>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
        </row>
      </rows>
    </PaxLog>
    <airline>ЮТ</airline>
  </answer>
</term>

%%
#########################################################################################

###
#   Тест №6
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#
#   Чтение архива из stat_general.cc
###
#########################################################################################

$(init_jxt_pult МОВРОМ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD ЮТ СОЧ ЛХР 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(make_spp $(ddmmyy +20))
$(deny_ets_interactive ЮТ 100 СОЧ)
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep) $(get point_arv) ЮТ 100 СОЧ ЛХР TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))

$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Детализированная Общая)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Детализированная Саморегистрация)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Детализированная "Отпр. телеграммы")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Общая Общая)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Общая "Отпр. телеграммы")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Общая Договор)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Общая Саморегистрация)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Общая "По агентам")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Подробная "Отпр. телеграммы")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Подробная Саморегистрация)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Подробная Общая)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Подробная "По агентам")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Подробная "Багажные RFISC")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Подробная "Огр. возмож.")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Подробная "Аннул. бирки")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Подробная PFS)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Подробная Трансфер)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Подробная Расселение)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Подробная "Бизнес приглашения")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Подробная Ваучеры)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Подробная "Факт. вылет")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Подробная Репринт)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Подробная Услуги)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Подробная Ремарки)
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Подробная "Изменения салона")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Подробная "SBDO (Zamar)")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Итого "По агентам")
$(RUN_GENERAL_STAT  $(date_format %d.%m.%Y +20) $(date_format %d.%m.%Y +21) Подробная "Несопр. багаж")
