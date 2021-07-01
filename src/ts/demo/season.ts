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
$(GET_SPP PIKE "25.06.2021 00:00:00")

>> lines=auto
    <command>
      <message lexema_id='MSG.DATA_SAVED'...
    </command>

    
# открытие экрана Перевозки
!! capture=on
$(READ_TRIPS)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer execute_time='...' lang='RU' handle='0'>
    <data>
      <flight_date>$(date_format %d.%m.%Y) 00:00:00</flight_date>
    </data>
  </answer>
</term>

# выход из системы
!! capture=on
$(logoff PIKE)


>> lines=auto
      <message lexema_id='MSG.WORK_SEANCE_FINISHED' code='0'>...

