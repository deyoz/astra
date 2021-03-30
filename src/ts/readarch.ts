include(ts/macro.ts)
include(ts/adm_macro.ts)
include(ts/fr_forms.ts)
include(ts/spp/read_trips_macro.ts)

# meta: suite readarch

#########################################################################################
###
#   Тест №1
#   Чтение архива из passenger.cc из функции LoadPaxDoc вызывающейся из
#   функции RunTrferPaxStat из статистики stat_trfer_pax.cc
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

$(run_arch_step $(ddmmyy +151))

$(dump_table MOVE_ARX_EXT fields = "move_id, part_key, date_range")
$(dump_table ARX_TRFER_PAX_STAT)

!! capture=on
$(RUN_TRFER_PAX_STAT $(date_format %d.%m.%Y -160) $(date_format %d.%m.%Y +21))

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
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#
#   Чтение архива из events.cc функции GetEvents
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

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")

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
        <ev_user>КОВАЛЕВ Р.А.</ev_user>
        <station>МОВРОМ</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>На рейсе запрещена web-регистрация.</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>КОВАЛЕВ Р.А.</ev_user>
        <station>МОВРОМ</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>Этап 'Подготовка к регистрации': план. время 00:15 $(date_format %d.%m.%y +1) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>КОВАЛЕВ Р.А.</ev_user>
        <station>МОВРОМ</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>Этап 'Открытие регистрации': план. время 04:14 $(date_format %d.%m.%y +1) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>КОВАЛЕВ Р.А.</ev_user>
        <station>МОВРОМ</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>Этап 'Открытие web-регистрации': план. время 07:15 $(date_format %d.%m.%y) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>КОВАЛЕВ Р.А.</ev_user>
        <station>МОВРОМ</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>Этап 'Открытие kiosk-регистрации': план. время 07:15 $(date_format %d.%m.%y) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>КОВАЛЕВ Р.А.</ev_user>
        <station>МОВРОМ</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>Этап 'Закрытие регистрации': план. время 06:35 $(date_format %d.%m.%y +1) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>КОВАЛЕВ Р.А.</ev_user>
        <station>МОВРОМ</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>Этап 'Запрет отмены web-регистрации': план. время 06:25 $(date_format %d.%m.%y +1) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>КОВАЛЕВ Р.А.</ev_user>
        <station>МОВРОМ</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>Этап 'Закрытие web-регистрации': план. время 04:15 $(date_format %d.%m.%y +1) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>КОВАЛЕВ Р.А.</ev_user>
        <station>МОВРОМ</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>Этап 'Закрытие kiosk-регистрации': план. время 05:15 $(date_format %d.%m.%y +1) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>КОВАЛЕВ Р.А.</ev_user>
        <station>МОВРОМ</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>Этап 'Готовность ВС к посадке': план. время 06:30 $(date_format %d.%m.%y +1) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>КОВАЛЕВ Р.А.</ev_user>
        <station>МОВРОМ</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>Этап 'Окончание посадки (оформление докум.)': план. время 06:50 $(date_format %d.%m.%y +1) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>КОВАЛЕВ Р.А.</ev_user>
        <station>МОВРОМ</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>Этап 'Откат трапа': план. время 07:00 $(date_format %d.%m.%y +1) (UTC)</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>КОВАЛЕВ Р.А.</ev_user>
        <station>МОВРОМ</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>Назначение весов пассажиров на рейс: </msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>КОВАЛЕВ Р.А.</ev_user>
        <station>МОВРОМ</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>Сбор статистики по рейсу</msg>
        <ev_order>...</ev_order>
      </row>
      <row>
        <point_id>$(get point_dep)</point_id>
        <ev_user>КОВАЛЕВ Р.А.</ev_user>
        <station>МОВРОМ</station>
        <time>...</time>
        <fmt_time>...</fmt_time>
        <msg>Рейс перемещен в архив</msg>
        <ev_order>...</ev_order>
      </row>
    </events_log>
    <form_data>
      <variables>
        <trip>ЮТ300</trip>
        <scd_out>01.12.0002</scd_out>
        <real_out>01.12.0002</real_out>
        <scd_date>01.12</scd_date>
        <date_issue>$(date_format %d.%m.%y) $(date_format %H:%M)</date_issue>
        <day_issue>$(date_format %d.%m.%y)</day_issue>
        <lang>RU</lang>
        <own_airp_name>АЭРОПОРТ АМСТЕРДАМ</own_airp_name>
        <own_airp_name_lat>AMSTERDAM AIRPORT</own_airp_name_lat>
        <airp_dep_name>АМСТЕРДАМ</airp_dep_name>
        <airp_dep_city>АМС</airp_dep_city>
        <airline_name>ОАО АВИАКОМПАНИЯ ЮТЭЙР</airline_name>
        <flt>ЮТ300</flt>
        <bort/>
        <craft>ТУ5</craft>
        <park/>
        <scd_time>xxxxx</scd_time>
        <long_route>АМСТЕРДАМ(АМС)-ПРАГА(ПРХ)</long_route>
        <test_server>1</test_server>
        <page_number_fmt>Стр. %u из %u</page_number_fmt>
        <landscape>0</landscape>
        <caption>Журнал операций по ЮТ300/01.12.0002 за $(date_format %d.%m.%y)</caption>
        <cap_test>ТЕСТ</cap_test>
        <short_page_number_fmt>Стр. %u</short_page_number_fmt>
      </variables>
    </form_data>
    <form name='EventsLog'...>$(EventsLogForm)
</form>
  </answer>
</term>


%%
#########################################################################################

###
#   Тест №3
#   Чтение архива из stat_arx.cc Функция ArxPaxListRun
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


!!
$(CHECKIN_PAX_TRANSFER $(get pax_id1) $(get point_dep_UT_298) $(get point_arv_UT_298) ЮТ 298 СОЧ ПРХ OZ OFER 2985523437721
                       $(get pax_id2) $(get point_dep_UT_190) $(get point_arv_UT_190) ЮТ 190 ПРХ АМС OZ OFER
                       $(get pax_id3) $(get point_dep_UT_450) $(get point_arv_UT_450) ЮТ 450 АМС ЛХР OZ OFER
                       ВЗ UA 32427293 UA 16.04.1968 25.06.2025 M)

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
          <seat_no>05А</seat_no>
          <grp_id>$(get grp_id)</grp_id>
          <airp_arv>ПРХ</airp_arv>
          <status>Зарег.</status>
          <class>Э</class>
          <document>32427293 UKR</document>
          <ticket_no>2985523437721</ticket_no>
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
#   Тест №4
#   Чтение архива из arx_stat_ad.cc
#
######################################################

$(init_jxt_pult МОВРОМ)
$(set_desk_version 201707-0195750)
$(login)

$(PREPARE_SEASON_SCD ЮТ СОЧ ЛХР 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive ЮТ 100 СОЧ)
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))

# это нужно для того чтобы pr_brd = 1 в таблице PAX,то есть посадить пассажира
# А это в свою очередь нужно чтобы заполнилась таблица STAT_AD и потом ARX_STAT_AD
$(sql {INSERT INTO trip_hall(point_id, type, hall, pr_misc)
       VALUES($(get point_dep_UT_100), 101, NULL, 1)})

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100) ЮТ 100 СОЧ ЛХР TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

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
    <form name='stat'...>$(statForm)
</form>
    <airline>ЮТ</airline>
    <grd>
      <header>
        <col width='50' align='0' sort='0'>АК</col>
        <col width='50' align='0' sort='0'>АП</col>
        <col width='75' align='0' sort='0'>Номер рейса</col>
        <col width='50' align='0' sort='3'>Дата</col>
        <col width='50' align='0' sort='3'>PNR</col>
        <col width='150' align='0' sort='3'>Ф.И.О.</col>
        <col width='50' align='0' sort='3'>Тип</col>
        <col width='50' align='0' sort='3'>Класс</col>
        <col width='50' align='0' sort='3'>Тип рег.</col>
        <col width='50' align='0' sort='3'>Багаж</col>
        <col width='50' align='0' sort='3'>Вых. на посадку</col>
        <col width='50' align='0' sort='3'>№ м</col>
      </header>
      <rows>
        <row>
          <col>ЮТ</col>
          <col>СОЧ</col>
          <col>100</col>
          <col>$(date_format %d.%m.%y +20)</col>
          <col>F50CF0</col>
          <col>TUMALI VALERII</col>
          <col>ВЗ</col>
          <col>Э</col>
          <col>TERM</col>
          <col/>
          <col>МОВРОМ</col>
          <col>5Г</col>
        </row>
        <row>
          <col>Итого:</col>
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
        <stat_type>29</stat_type>
        <stat_mode>Факт. вылет</stat_mode>
        <stat_type_caption>Подробная</stat_type_caption>
      </variables>
    </form_data>
  </answer>
</term>


%%
#########################################################################################

###
#   Тест №5
#   Чтение архива из stat_services.cc
#
######################################################

include(ts/sirena_exchange_macro.ts)

$(init_term)

$(PREPARE_SEASON_SCD ЮТ СОЧ ЛХР 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive ЮТ 100 СОЧ)
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))

$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))

$(set pax_id_BURYAKOV $(get_pax_id $(get point_dep_UT_100) БУРЯКОВ "ЕВГЕНИЙ ЕВГЕНЬЕВИЧ"))

$(CHANGE_TRIP_SETS $(get point_dep_UT_100) piece_concept=1)
$(settcl SIRENA_HOST localhost)
$(settcl SIRENA_PORT 8008)

!! capture=on
$(cache PIKE RU RFISC_SETS $(cache_iface_ver RFISC_SETS) ""
  insert airline:$(get_elem_id etAirline ЮТ)
         rfic:A
         rfisc:0B5
         auto_checkin:1)

>> lines=auto
$(MESSAGE_TAG MSG.CHANGED_DATA_COMMIT)


$(http_forecast content=$(get_svc_availability_resp))


#1БУРЯКОВ/ЕВГЕНИЙ ЕВГЕНЬЕВИЧ
#.L/F451B5/UT
#.L/5C0NXZ/1H
#.R/TKNE HK1 2982425622093/1
#.R/SEAT HK1 10D
#.R/OTHS HK1 FQTSTATUS BRONZE
#.R/FQTV UT 1020894422
#.R/ASVC HI1 A/0B5/SEAT/ПРЕДВАРИТЕЛЬНЫЙ ВЫБОР МЕСТА/A
#.RN//2984555892312C1
#.R/ASVC HI1 C/08A//РУЧНАЯ КЛАДЬ ДО10КГ 55Х40Х25СМ/A
#.RN//2984555892311C1
#.R/ASVC HI1 A/O6O//ПАССАЖИР С РУЧНОЙ КЛАДЬЮ/A/2984555892336C1
#.R/DOCS HK1/P/RU/4501742939/RU/02MAR75/M/$(ddmonyy +1y)/БУРЯКОВ
#.RN//ЕВГЕНИЙ ЕВГЕНЬЕВИЧ
#.R/PSPT HK1 4501742939/RU/02MAR75/БУРЯКОВ/ЕВГЕНИЙ ЕВГЕНЬЕВИЧ/M
#.R/FOID PP4501742939

!!
$(CHECKIN_PAX $(get pax_id_BURYAKOV) $(get point_dep_UT_100) $(get point_arv_UT_100)
  ЮТ 100 СОЧ ЛХР БУРЯКОВ "ЕВГЕНИЙ ЕВГЕНЬЕВИЧ" 2982425622093 ВЗ RU 4501742939 RU 02.03.1975 $(date_format %d.%m.%Y) M)

>> lines=auto
<query>
  <svc_availability show_brand_info=\"true\" show_all_svc=\"true\" show_free_carry_on_norm=\"true\">
    <passenger id=\"$(get pax_id_BURYAKOV)\" surname=\"БУРЯКОВ\" name=\"ЕВГЕНИЙ ЕВГЕНЬЕВИЧ\" category=\"ADT\" birthdate=\"1975-03-02\" sex=\"male\">
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
    <airline>ЮТ</airline>
    <grd>
      <header>
        <col width='50' align='0' sort='0'>АП</col>
        <col width='50' align='0' sort='0'>АК</col>
        <col width='75' align='0' sort='0'>Номер рейса</col>
        <col width='50' align='0' sort='0'>Дата</col>
        <col width='150' align='0' sort='3'>Ф.И.О.</col>
        <col width='150' align='0' sort='3'>Билет</col>
        <col width='50' align='0' sort='0'>От</col>
        <col width='50' align='0' sort='0'>До</col>
        <col width='30' align='0' sort='0'>RFIC</col>
        <col width='40' align='0' sort='0'>RFISC</col>
        <col width='100' align='0' sort='0'>№ квитанции</col>
      </header>
      <rows>
        <row>
          <col>СОЧ</col>
          <col>ЮТ</col>
          <col>100</col>
          <col>$(date_format %d.%m.%y +20)</col>
          <col>БУРЯКОВ ЕВГЕНИЙ ЕВГЕНЬЕВИЧ</col>
          <col>2982425622093/1</col>
          <col>СОЧ</col>
          <col>ЛХР</col>
          <col>A</col>
          <col>0B5</col>
          <col>2984555892312/1</col>
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
        <stat_type>34</stat_type>
        <stat_mode>Услуги</stat_mode>
        <stat_type_caption>Подробная</stat_type_caption>
      </variables>
    </form_data>
  </answer>
</term>


%%
#########################################################################################

###
#   Тест №6
#   Чтение архива arx_events из astra_utils.cc TRegEvents::fromDB файла stat_departed.cc
#   в функции departed_flt()
######################################################

$(init_term)

$(PREPARE_SEASON_SCD ЮТ СОЧ ЛХР 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive ЮТ 100 СОЧ)
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))

# это нужно для того чтобы pr_brd = 1 в таблице PAX,то есть посадить пассажира
# А это в свою очередь нужно чтобы заполнилась таблица STAT_AD и потом ARX_STAT_AD
$(sql {INSERT INTO trip_hall(point_id, type, hall, pr_misc)
       VALUES($(get point_dep_UT_100), 101, NULL, 1)})

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100) ЮТ 100 СОЧ ЛХР TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

$(dump_table points)
$(run_arch_step $(ddmmyy +141))

$(are_tables_equal ARX_POINTS)
$(are_tables_equal MOVE_ARX_EXT)

$(nosir_departed_flt $(yyyymmdd +10) $(yyyymmdd +30))

#??
#$(read_file departed.2102.csv)
#>> lines=auto
#ФИО;Дата рождения;Пол;Тип документа;Серия и номер документа;Номер билета;Номер бронирования;Рейс;Дата вылета;От;До;Багаж мест;Багаж вес;Время регистрации (UTC);Номер места;Способ регистрации;Печать ПТ на стойке
#VALERII TUMALI;16.АПР.68;M;P;FA144642;2986145115578/1;;ЮТ100;24ФЕВ;СОЧ;ЛХР;0;0;04.02.2021 09:49:53;TERM
#=======
#$(nosir_departed_flt 20201220 20210120)

%%
#########################################################################################

###
#   Тест №7
#   Выбираются данные из архива по рейсам за текущую дату
#   в функции internal_ReadData_N в sopp.cc
######################################################

$(init_term)

$(PREPARE_SEASON_SCD ЮТ СОЧ ЛХР 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive ЮТ 100 СОЧ)
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))
$(set move_id $(get_move_id $(get point_dep_UT_100)))

# это нужно для того чтобы pr_brd = 1 в таблице PAX,то есть посадить пассажира
# А это в свою очередь нужно чтобы заполнилась таблица STAT_AD и потом ARX_STAT_AD
$(sql {INSERT INTO trip_hall(point_id, type, hall, pr_misc)
       VALUES($(get point_dep_UT_100), 101, NULL, 1)})

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100) ЮТ 100 СОЧ ЛХР TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

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
          <airp>СОЧ</airp>
          <airline_out>ЮТ</airline_out>
          <flt_no_out>100</flt_no_out>
          <craft_out>TU5</craft_out>
          <scd_out>$(date_format %d.%m.%Y +20) 10:15:00</scd_out>
          <triptype_out>п</triptype_out>
          <pr_reg>1</pr_reg>
          <places_out>
            <airp>ЛХР</airp>
          </places_out>
          <classes>
            <class cfg='11'>Б</class>
            <class cfg='63'>Э</class>
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
          <airline_in>ЮТ</airline_in>
          <flt_no_in>100</flt_no_in>
          <craft_in>TU5</craft_in>
          <scd_in>$(date_format %d.%m.%Y +20) 12:00:00</scd_in>
          <triptype_in>п</triptype_in>
          <places_in>
            <airp>СОЧ</airp>
          </places_in>
          <airp>ЛХР</airp>
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
#   Тест №8
#   Проверка чтения из архивных таблиц при запуске basel_aero_flat_stat
######################################################

$(init_term)

$(sql {INSERT INTO file_sets(code, airp, name, dir, last_create, pr_denial)
       VALUES('BASEL_AERO', 'СОЧ', 'BASEL_AERO', 'basel_aero/', TO_DATE('07.02.21', 'dd.mm.yy'), 0)})


$(PREPARE_SEASON_SCD ЮТ СОЧ ЛХР 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive ЮТ 100 СОЧ)
$(make_spp $(ddmmyy +20))


$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))
$(set move_id $(get_move_id $(get point_dep_UT_100)))

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100) ЮТ 100 СОЧ ЛХР TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

$(run_arch_step $(ddmmyy +141))

$(nosir_basel_stat $(date_format %d.%m.%Y +20) 09:00:00 $(get point_dep_UT_100))

??
$(dump_table basel_stat display="on")
>> lines=auto
[СОЧ] [$(get pax_id_TUMALI)] [$(get point_dep_UT_100)] [...] [NULL] [NULL] [0] [NULL] [1] [...] [ЭКОНОМ] [NULL] [$(yymmdd +20)] [$(yymmdd +20)] [NULL] [ЮТ100] [...] [TUMALI/VALERII] [0] [0] [NULL] [NULL] [зарегистрирован] [NULL] [NULL] [0] $()


%%
#########################################################################################

###
#   Тест №9
#   Чтение архива из stat_arx.cc Функция PAX_SRC_RUN
#
###

$(init_term)

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
          <seat_no>05А</seat_no>
          <grp_id>$(get grp_id)</grp_id>
          <airp_arv>ПРХ</airp_arv>
          <status>Зарег.</status>
          <class>Э</class>
          <document>32427293 UKR</document>
          <ticket_no>2985523437721</ticket_no>
          <hall>Зал 1</hall>
        </pax>
        <pax>
          <part_key>$(date_format %d.%m.%Y) 09:00:00</part_key>
          <point_id>$(get point_dep_UT_190)</point_id>
          <airline>ЮТ</airline>
          <flt_no>190</flt_no>
          <suffix/>
          <trip>ЮТ190 ПРХ</trip>
          <scd_out>$(date_format %d.%m.%Y) 10:15:00</scd_out>
          <reg_no>1</reg_no>
          <full_name>OZ OFER</full_name>
          <bag_amount>0</bag_amount>
          <bag_weight>0</bag_weight>
          <rk_weight>0</rk_weight>
          <excess>0</excess>
          <tags/>
          <seat_no>05А</seat_no>
          <grp_id>$(get grp_id2)</grp_id>
          <airp_arv>АМС</airp_arv>
          <status>Зарег.</status>
          <class>Э</class>
          <document>32427293 UKR</document>
          <ticket_no>2985523437721</ticket_no>
          <hall>Зал 1</hall>
        </pax>
        <pax>
          <part_key>$(date_format %d.%m.%Y) 09:00:00</part_key>
          <point_id>$(get point_dep_UT_450)</point_id>
          <airline>ЮТ</airline>
          <flt_no>450</flt_no>
          <suffix/>
          <trip>ЮТ450 АМС</trip>
          <scd_out>$(date_format %d.%m.%Y) 10:15:00</scd_out>
          <reg_no>1</reg_no>
          <full_name>OZ OFER</full_name>
          <bag_amount>0</bag_amount>
          <bag_weight>0</bag_weight>
          <rk_weight>0</rk_weight>
          <excess>0</excess>
          <tags/>
          <seat_no>05А</seat_no>
          <grp_id>$(get grp_id3)</grp_id>
          <airp_arv>ЛХР</airp_arv>
          <status>Зарег.</status>
          <class>Э</class>
          <document>32427293 UKR</document>
          <ticket_no>2985523437721</ticket_no>
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
    <form name='ArxPaxList'...>$(ArxPaxListForm)
</form>
  </answer>
</term>

%%
#########################################################################################

###
#   Тест №10
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

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")
$(run_arch_step $(ddmmyy +387))
$(dump_table ARX_POINTS)

!! capture=on
$(RUN_FLT_TASK_LOG $(get point_dep) $(date_format %d.%m.%Y +1))

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
#   Тест №11
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

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")
$(run_arch_step $(ddmmyy +387))
$(dump_table ARX_POINTS)

!! capture=on
$(RUN_FLT_LOG $(get point_dep) $(date_format %d.%m.%Y +1))

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
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Сбор статистики по рейсу</msg>
          <ev_order>...</ev_order>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>Регистрация</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Рейс перемещен в архив</msg>
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
#   Тест №12
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#
#   Чтение архива из stat_utils.cc функции FltCBoxDropDown
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

$(set point_dep_300 $(get_dep_point_id АМС ЮТ 300 $(yymmdd +1)))
$(set point_dep_100 $(get_dep_point_id СОЧ ЮТ 100 $(yymmdd +265)))

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
        <name>ЮТ100  /$(date_format %d.%m +265 ) СОЧ</name>
        <point_id>$(get point_dep_100)</point_id>
        <part_key>$(date_format %d.%m.%Y +265) 09:00:00</part_key>
      </f>
      <f>
        <name>ЮТ300  /$(date_format %d +1) АМС</name>
        <point_id>$(get point_dep_300)</point_id>
        <part_key>$(date_format %d.%m.%Y +1) 09:00:00</part_key>
      </f>
    </cbox>
  </answer>
</term>


%%
#########################################################################################

###
#   Тест №13
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

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")
$(dump_table PAX)

$(run_arch_step $(ddmmyy +150))
$(dump_table ARX_POINTS)

!! capture=on
$(RUN_LOG_RUN $(get point_dep) $(get grp_id) $(date_format %d.%m.%Y +20) 1)
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
#   Тест №14
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

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")
$(dump_table PAX)

$(run_arch_step $(ddmmyy +150))

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


%%
#########################################################################################

###
#   Тест №15
#
#   Описание: пассажиров: 61,
#             интерактив: выкл
#
#   Чтение архива из stat_arx.cc функции SystemLogRun
###
#########################################################################################

$(init_jxt_pult МОВРОМ)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD ЮТ СОЧ ЛХР 100 -1 TU5 $(date_format %d.%m.%Y +0) $(date_format %d.%m.%Y +5))
$(make_spp $(ddmmyy +1))
$(deny_ets_interactive ЮТ 100 СОЧ)
$(INB_PNL_UT AER LHR 100 $(ddmon +1 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep) $(get point_arv) ЮТ 100 СОЧ ЛХР TUMALI VALERII 2986145115578 ВЗ UA FA144642 UA 16.04.1968 25.06.2025 M)

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))

#$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")

$(run_arch_step $(ddmmyy +140))

#$(dump_table ARX_EVENTS order="ev_order, lang")

!! capture=on
$(RUN_SYSTEM_LOG $(date_format %d.%m.%Y +0 ) $(date_format %d.%m.%Y +1))
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
        <report_title>Операции в системе</report_title>
      </variables>
    </form_data>
    <PaxLog>
      <header>
        <col>Агент</col>
      </header>
      <rows>
        <row>
          <point_id>0</point_id>
          <time>...</time>
          <msg>Ввод нового рейса</msg>
          <ev_order>...</ev_order>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>0</point_id>
          <time>...</time>
          <msg>Ввод нового периода $(date_format %d.%m.%y +0) $(date_format %d.%m.%y +5) 1234567 (ид. рейса=...,ид. маршрута=...)</msg>
          <ev_order>...</ev_order>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>0</point_id>
          <time>...</time>
          <msg>Маршрут: ЮТ100,ТУ5,СОЧ07:15(UTC)-09:00(UTC)ЛХР</msg>
          <ev_order>...</ev_order>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>0</point_id>
          <time>...</time>
          <msg>Получение СПП за $(date_format %d.%m.%y +1)</msg>
          <ev_order>...</ev_order>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>0</point_id>
          <time>...</time>
          <msg>Ввод строки в таблице 'Настройки рейсов разные': TYPE_CODE='11',AIRLINE='ЮТ',А/к='',AIRP_DEP='СОЧ',А/п вылета='',Рейс='100',Значение='1'. Идентификатор: TYPE_CODE='11',AIRLINE='ЮТ',А/к='',AIRP_DEP='СОЧ',А/п вылета='',Рейс='100',Значение='1'</msg>
          <ev_order>...</ev_order>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>MAINDCS.EXE</screen>
        </row>
      </rows>
    </PaxLog>
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
          <trip>ЮТ100/$(date_format %d +1) СОЧ</trip>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Этап 'Подготовка к регистрации': план. время 00:15 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <trip>ЮТ100/$(date_format %d +1) СОЧ</trip>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Этап 'Открытие регистрации': план. время 04:14 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <trip>ЮТ100/$(date_format %d +1) СОЧ</trip>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Этап 'Открытие web-регистрации': план. время 07:15 $(date_format %d.%m.%y +0) (UTC)</msg>
          <ev_order>...</ev_order>
          <trip>ЮТ100/$(date_format %d +1) СОЧ</trip>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Этап 'Открытие kiosk-регистрации': план. время 07:15 $(date_format %d.%m.%y +0) (UTC)</msg>
          <ev_order>...</ev_order>
          <trip>ЮТ100/$(date_format %d +1) СОЧ</trip>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Этап 'Закрытие регистрации': план. время 06:35 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <trip>ЮТ100/$(date_format %d +1) СОЧ</trip>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Этап 'Запрет отмены web-регистрации': план. время 06:25 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <trip>ЮТ100/$(date_format %d +1) СОЧ</trip>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Этап 'Закрытие web-регистрации': план. время 04:15 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <trip>ЮТ100/$(date_format %d +1) СОЧ</trip>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Этап 'Закрытие kiosk-регистрации': план. время 05:15 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <trip>ЮТ100/$(date_format %d +1) СОЧ</trip>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Этап 'Готовность ВС к посадке': план. время 06:30 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <trip>ЮТ100/$(date_format %d +1) СОЧ</trip>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Этап 'Окончание посадки (оформление докум.)': план. время 06:50 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <trip>ЮТ100/$(date_format %d +1) СОЧ</trip>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Этап 'отгон трапа': план. время 07:00 $(date_format %d.%m.%y +1) (UTC)</msg>
          <ev_order>...</ev_order>
          <trip>ЮТ100/$(date_format %d +1) СОЧ</trip>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Назначение весов пассажиров на рейс: </msg>
          <ev_order>...</ev_order>
          <trip>ЮТ100/$(date_format %d +1) СОЧ</trip>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Задача EMD_REFRESH &lt;CloseCheckIn 0&gt; создана; План. вр.: $(date_format %d.%m.%y +1) 06:35:00 (UTC)</msg>
          <ev_order>...</ev_order>
          <trip>ЮТ100/$(date_format %d +1) СОЧ</trip>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Задача SYNC_ALL_CHKD &lt;&gt; создана; План. вр.: $(date_format %d.%m.%y +0) ... (UTC)</msg>
          <ev_order>...</ev_order>
          <trip>ЮТ100/$(date_format %d +1) СОЧ</trip>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>MAINDCS.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Назначена базовая компоновка (ид=43345). Классы: Б11 Э63</msg>
          <ev_order>...</ev_order>
          <trip>ЮТ100/$(date_format %d +1) СОЧ</trip>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>MAINDCS.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Назначение весов пассажиров на рейс: </msg>
          <ev_order>...</ev_order>
          <trip>ЮТ100/$(date_format %d +1) СОЧ</trip>
          <station>IATCIP</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Пассажир TUMALI VALERII (ВЗ) зарегистрирован. П/н: ЛХР, класс: Э, статус: Бронь, место: 5Г. Баг.нормы: нет</msg>
          <ev_order>...</ev_order>
          <trip>ЮТ100/$(date_format %d +1) СОЧ</trip>
          <grp_id>$(get grp_id)</grp_id>
          <reg_no>1</reg_no>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Пассажир TUMALI VALERII (ВЗ). DOCS: P/UKR/FA144642/UKR/16APR68/M/25JUN25/TUMALI/VALERII/. Ручной ввод</msg>
          <ev_order>...</ev_order>
          <trip>ЮТ100/$(date_format %d +1) СОЧ</trip>
          <grp_id>$(get grp_id)</grp_id>
          <reg_no>1</reg_no>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Назначение весов пассажиров на рейс: </msg>
          <ev_order>...</ev_order>
          <trip>ЮТ100/$(date_format %d +1) СОЧ</trip>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Назначение весов пассажиров на рейс: </msg>
          <ev_order>...</ev_order>
          <trip>ЮТ100/$(date_format %d +1) СОЧ</trip>
          <ev_user>КОВАЛЕВ Р.А.</ev_user>
          <station>МОВРОМ</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Сбор статистики по рейсу</msg>
          <ev_order>...</ev_order>
          <trip>ЮТ100/$(date_format %d +1) СОЧ</trip>
          <station>IATCIP</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>$(get point_dep)</point_id>
          <time>...</time>
          <msg>Рейс перемещен в архив</msg>
          <ev_order>...</ev_order>
          <trip>ЮТ100/$(date_format %d +1) СОЧ</trip>
          <station>IATCIP</station>
          <screen>AIR.EXE</screen>
        </row>
        <row>
          <point_id>...</point_id>
          <time>...</time>
          <msg>Рейс перемещен в архив</msg>
          <ev_order>...</ev_order>
          <trip/>
          <station>IATCIP</station>
          <screen>AIR.EXE</screen>
        </row>
      </rows>
    </PaxLog>
  </answer>
</term>

