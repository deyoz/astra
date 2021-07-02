include(ts/macro.ts)
include(ts/adm_macro.ts)
include(ts/season/macro.ts)
include(ts/spp/read_trips_macro.ts)


# meta: suite demo

$(init_term)

$(PREPARE_SEASON_SCD UT UFA AER 422 -1 TU5 $(date_format %d.%m.%Y -1000) $(date_format %d.%m.%Y +100))
$(PREPARE_SEASON_SCD UT AER UFA 423 -1 TU5 $(date_format %d.%m.%Y -1000) $(date_format %d.%m.%Y +100))

# запрос доступных сертификатов шифрования
$(GET_CERTIFICATES)

# вход в систему
$(login)

# загрузка кэшей
$(cache PIKE RU TERM_PROFILE_RIGHTS $(cache_iface_ver TERM_PROFILE_RIGHTS) "")
$(cache PIKE RU PRN_FORMS_LAYOUT $(cache_iface_ver PRN_FORMS_LAYOUT) "")
$(cache PIKE RU AIRPS $(cache_iface_ver AIRPS) "")
$(cache PIKE RU AIRLINES $(cache_iface_ver AIRLINES) "")
$(cache PIKE RU TRIP_SUFFIXES $(cache_iface_ver TRIP_SUFFIXES) "")
$(cache PIKE RU TRIP_TYPES $(cache_iface_ver TRIP_TYPES) "")

# открытие сезонного расписания
!! capture=on
$(SEASON_READ PIKE)

>> lines=auto mode=regex
    .*<rangeList>
        <trip_id>([0-9]+)</trip_id>
        <exec>.*
        <noexec/>
        <trips>
          <trip>
            <move_id>([0-9]+)</move_id>
            <name>UT422</name>
            <crafts>TU5</crafts>
            <ports>UFA/AER</ports>
          </trip>
        </trips>
      </rangeList>
      <rangeList>
        <trip_id>([0-9]+)</trip_id>
        <exec>.*
        <noexec/>
        <trips>
          <trip>
            <move_id>([0-9]+)</move_id>
            <name>UT423</name>
            <crafts>TU5</crafts>
            <ports>AER/UFA</ports>
          </trip>
        </trips>
      </rangeList>.*

$(set trip_id1 $(capture 1))
$(set trip_id2 $(capture 3))

# открытие рейса сезонного расписания
!! capture=on
$(EDIT_SEASON_TRIP PIKE $(get trip_id1))

>> lines=auto
      <trips>
        <trip>
          <trip_id>$(get trip_id1)</trip_id>
          <name>UT422</name>
        </trip>
        <trip>
          <trip_id>$(get trip_id2)</trip_id>
          <name>UT423</name>
        </trip>
      </trips>

!! capture=on
$(GET_SPP PIKE "$(date_format %d.%m.%Y) 00:00:00")

>> lines=auto
    <command>
      <message lexema_id='MSG.DATA_SAVED'...
    </command>

$(set point_id_422 $(get_point_dep_for_flight UT 422 "" $(yymmdd) UFA))
$(set move_id_422 $(get_move_id $(get point_id_422)))

$(set point_id_423 $(get_point_dep_for_flight UT 423 "" $(yymmdd) AER))
$(set move_id_423 $(get_move_id $(get point_id_423)))

# открытие экрана Перевозки
!! capture=on
$(READ_TRIPS)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer execute_time='...' lang='RU' handle='0'>
    <data>
      <flight_date>$(date_format %d.%m.%Y) 00:00:00</flight_date>
      <trips>
        <trip>
          <move_id>$(get move_id_422)</move_id>
          <point_id>$(get point_id_422)</point_id>
          <pr_del_in>-1</pr_del_in>
          <airp>UFA</airp>
          <airline_out>UT</airline_out>
          <flt_no_out>422</flt_no_out>
          <craft_out>TU5</craft_out>
          <scd_out>$(date_format %d.%m.%Y) 10:15:00</scd_out>
          <triptype_out>п</triptype_out>
          <pr_reg>1</pr_reg>
          <places_out>
            <airp>AER</airp>
          </places_out>
          <stages>
            <stage>
              <stage_id>10</stage_id>
              <scd>$(date_format %d.%m.%Y) 03:15:00</scd>
              <pr_auto>0</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>20</stage_id>
              <scd>$(date_format %d.%m.%Y) 07:14:00</scd>
              <pr_auto>0</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>25</stage_id>
              <scd>$(date_format %d.%m.%Y -1) 10:15:00</scd>
              <pr_auto>1</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>26</stage_id>
              <scd>$(date_format %d.%m.%Y -1) 10:15:00</scd>
              <pr_auto>1</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>30</stage_id>
              <scd>$(date_format %d.%m.%Y) 09:35:00</scd>
              <pr_auto>0</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>31</stage_id>
              <scd>$(date_format %d.%m.%Y) 09:25:00</scd>
              <pr_auto>1</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>35</stage_id>
              <scd>$(date_format %d.%m.%Y) 07:15:00</scd>
              <pr_auto>0</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>36</stage_id>
              <scd>$(date_format %d.%m.%Y) 08:15:00</scd>
              <pr_auto>1</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>40</stage_id>
              <scd>$(date_format %d.%m.%Y) 09:30:00</scd>
              <pr_auto>0</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>50</stage_id>
              <scd>$(date_format %d.%m.%Y) 09:50:00</scd>
              <pr_auto>0</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>70</stage_id>
              <scd>$(date_format %d.%m.%Y) 10:00:00</scd>
              <pr_auto>0</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
          </stages>
        </trip>
        <trip>
          <move_id>$(get move_id_422)</move_id>
          <point_id>...</point_id>
          <airline_in>UT</airline_in>
          <flt_no_in>422</flt_no_in>
          <craft_in>TU5</craft_in>
          <scd_in>$(date_format %d.%m.%Y) 12:00:00</scd_in>
          <triptype_in>п</triptype_in>
          <places_in>
            <airp>UFA</airp>
          </places_in>
          <airp>AER</airp>
          <pr_del_out>-1</pr_del_out>
          <pr_reg>0</pr_reg>
        </trip>
        <trip>
          <move_id>$(get move_id_423)</move_id>
          <point_id>$(get point_id_423)</point_id>
          <pr_del_in>-1</pr_del_in>
          <airp>AER</airp>
          <airline_out>UT</airline_out>
          <flt_no_out>423</flt_no_out>
          <craft_out>TU5</craft_out>
          <scd_out>$(date_format %d.%m.%Y) 10:15:00</scd_out>
          <triptype_out>п</triptype_out>
          <pr_reg>1</pr_reg>
          <places_out>
            <airp>UFA</airp>
          </places_out>
          <stages>
            <stage>
              <stage_id>10</stage_id>
              <scd>$(date_format %d.%m.%Y) 03:15:00</scd>
              <pr_auto>0</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>20</stage_id>
              <scd>$(date_format %d.%m.%Y) 07:14:00</scd>
              <pr_auto>0</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>25</stage_id>
              <scd>$(date_format %d.%m.%Y -1) 10:15:00</scd>
              <pr_auto>1</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>26</stage_id>
              <scd>$(date_format %d.%m.%Y -1) 10:15:00</scd>
              <pr_auto>1</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>30</stage_id>
              <scd>$(date_format %d.%m.%Y) 09:35:00</scd>
              <pr_auto>0</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>31</stage_id>
              <scd>$(date_format %d.%m.%Y) 09:25:00</scd>
              <pr_auto>1</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>35</stage_id>
              <scd>$(date_format %d.%m.%Y) 07:15:00</scd>
              <pr_auto>0</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>36</stage_id>
              <scd>$(date_format %d.%m.%Y) 08:15:00</scd>
              <pr_auto>1</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>40</stage_id>
              <scd>$(date_format %d.%m.%Y) 09:30:00</scd>
              <pr_auto>0</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>50</stage_id>
              <scd>$(date_format %d.%m.%Y) 09:50:00</scd>
              <pr_auto>0</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
            <stage>
              <stage_id>70</stage_id>
              <scd>$(date_format %d.%m.%Y) 10:00:00</scd>
              <pr_auto>0</pr_auto>
              <pr_manual>0</pr_manual>
            </stage>
          </stages>
        </trip>
        <trip>
          <move_id>$(get move_id_423)</move_id>
          <point_id>...</point_id>
          <airline_in>UT</airline_in>
          <flt_no_in>423</flt_no_in>
          <craft_in>TU5</craft_in>
          <scd_in>$(date_format %d.%m.%Y) 12:00:00</scd_in>
          <triptype_in>п</triptype_in>
          <places_in>
            <airp>AER</airp>
          </places_in>
          <airp>UFA</airp>
          <pr_del_out>-1</pr_del_out>
          <pr_reg>0</pr_reg>
        </trip>
      </trips>
    </data>
  </answer>
</term>

# выход из системы
!! capture=on
$(logoff PIKE)


>> lines=auto
      <message lexema_id='MSG.WORK_SEANCE_FINISHED' code='0'>...

