include(ts/macro.ts)
include(ts/adm_macro.ts)
include(ts/spp/write_dests_macro.ts)
include(ts/spp/write_trips_macro.ts)
include(ts/spp/read_trips_macro.ts)

# meta: suite stages

### test 1 - проверяем trip_final_stages
### куча рейсов в разных статусах - проверяем их доступность в списках рейсов в разных модулях
#########################################################################################

$(init_jxt_pult МОВВЛА)
$(login VLAD GHJHSD)

$(UPDATE_USER PIKE "КОВАЛЕВ Р.А." 1
{          <airps>
            <item>ВНК</item>
            <item>СОЧ</item>
          </airps>}
 opr=VLAD)

$(init_term)
$(set_user_time_type LocalAirp PIKE)

$(PREPARE_FLIGHTS
  $(date_format %d.%m.%Y +2)
  $(date_format %d.%m.%Y +3)
  $(date_format %d.%m.%Y +4))

$(set point_dep_UT580 $(get_point_dep_for_flight UT 580 "" $(yymmdd +2) AER))
$(set point_dep_UT461 $(get_point_dep_for_flight UT 461 "" $(yymmdd +2) VKO))
$(set point_dep_UT804 $(get_point_dep_for_flight UT 804 "" $(yymmdd +2) VKO))
$(set point_dep_UT298 $(get_point_dep_for_flight UT 298 "" $(yymmdd +2) VKO))


$(GET_ADV_TRIP_LIST_PREP_CHECKIN_REQUEST $(date_format %d.%m.%Y +2) lang=EN capture=on)
$(GET_ADV_TRIP_LIST_RESPONSE $(date_format %d.%m.%Y +2))

$(GET_ADV_TRIP_LIST_CHECKIN_REQUEST $(date_format %d.%m.%Y +2) lang=EN capture=on)
$(GET_ADV_TRIP_LIST_RESPONSE $(date_format %d.%m.%Y +2))

$(GET_ADV_TRIP_LIST_BOARDING_REQUEST $(date_format %d.%m.%Y +2) lang=EN capture=on)
$(GET_ADV_TRIP_LIST_RESPONSE $(date_format %d.%m.%Y +2))


$(OPEN_CHECKIN $(get point_dep_UT461))
$(PREP_CHECKIN $(get point_dep_UT804))
$(PREP_CHECKIN $(get point_dep_UT298))

$(GET_ADV_TRIP_LIST_PREP_CHECKIN_REQUEST $(date_format %d.%m.%Y +2) lang=EN capture=on)
$(GET_ADV_TRIP_LIST_RESPONSE $(date_format %d.%m.%Y +2)
{$(adv_trip_list_item $(get point_dep_UT298) UT298 $(dd +2) VKO 0)
$(adv_trip_list_item $(get point_dep_UT461) UT461 $(dd +2) VKO 1)
$(adv_trip_list_item $(get point_dep_UT804) UT804 $(dd +2) VKO 2)})

$(GET_ADV_TRIP_LIST_CHECKIN_REQUEST $(date_format %d.%m.%Y +2) lang=EN capture=on)
$(GET_ADV_TRIP_LIST_RESPONSE $(date_format %d.%m.%Y +2)
{$(adv_trip_list_item $(get point_dep_UT461) UT461 $(dd +2) VKO 0)})

$(GET_ADV_TRIP_LIST_BOARDING_REQUEST $(date_format %d.%m.%Y +2) lang=EN capture=on)
$(GET_ADV_TRIP_LIST_RESPONSE $(date_format %d.%m.%Y +2))


$(OPEN_BOARDING $(get point_dep_UT580))
$(CLOSE_CHECKIN $(get point_dep_UT461))
$(OPEN_CHECKIN $(get point_dep_UT804))
$(OPEN_BOARDING $(get point_dep_UT298))

$(GET_ADV_TRIP_LIST_PREP_CHECKIN_REQUEST $(date_format %d.%m.%Y +2) lang=EN capture=on)
$(GET_ADV_TRIP_LIST_RESPONSE $(date_format %d.%m.%Y +2)
{$(adv_trip_list_item $(get point_dep_UT298) UT298 $(dd +2) VKO 0)
$(adv_trip_list_item $(get point_dep_UT461) UT461 $(dd +2) VKO 1)
$(adv_trip_list_item $(get point_dep_UT804) UT804 $(dd +2) VKO 2)})

$(GET_ADV_TRIP_LIST_CHECKIN_REQUEST $(date_format %d.%m.%Y +2) lang=EN capture=on)
$(GET_ADV_TRIP_LIST_RESPONSE $(date_format %d.%m.%Y +2)
{$(adv_trip_list_item $(get point_dep_UT461) UT461 $(dd +2) VKO 0)
$(adv_trip_list_item $(get point_dep_UT804) UT804 $(dd +2) VKO 1)})

$(GET_ADV_TRIP_LIST_BOARDING_REQUEST $(date_format %d.%m.%Y +2) lang=EN capture=on)
$(GET_ADV_TRIP_LIST_RESPONSE $(date_format %d.%m.%Y +2)
{$(adv_trip_list_item $(get point_dep_UT580) UT580 $(dd +2) AER 1 0 0)
$(adv_trip_list_item $(get point_dep_UT298) UT298 $(dd +2) VKO 0 1 1)})


$(CLOSE_BOARDING $(get point_dep_UT580))
$(OPEN_BOARDING $(get point_dep_UT461))
$(CLOSE_CHECKIN $(get point_dep_UT804))
$(CLOSE_BOARDING $(get point_dep_UT298))

$(GET_ADV_TRIP_LIST_PREP_CHECKIN_REQUEST $(date_format %d.%m.%Y +2) lang=EN capture=on)
$(GET_ADV_TRIP_LIST_RESPONSE $(date_format %d.%m.%Y +2)
{$(adv_trip_list_item $(get point_dep_UT580) UT580 $(dd +2) AER 2 0 0)
$(adv_trip_list_item $(get point_dep_UT298) UT298 $(dd +2) VKO 0 1 1)
$(adv_trip_list_item $(get point_dep_UT461) UT461 $(dd +2) VKO 1 2 2)
$(adv_trip_list_item $(get point_dep_UT804) UT804 $(dd +2) VKO 3)})

$(GET_ADV_TRIP_LIST_CHECKIN_REQUEST $(date_format %d.%m.%Y +2) lang=EN capture=on)
$(GET_ADV_TRIP_LIST_RESPONSE $(date_format %d.%m.%Y +2)
{$(adv_trip_list_item $(get point_dep_UT580) UT580 $(dd +2) AER 1 0 0)
$(adv_trip_list_item $(get point_dep_UT461) UT461 $(dd +2) VKO 0 1 1)
$(adv_trip_list_item $(get point_dep_UT804) UT804 $(dd +2) VKO 2)})

$(GET_ADV_TRIP_LIST_BOARDING_REQUEST $(date_format %d.%m.%Y +2) lang=EN capture=on)
$(GET_ADV_TRIP_LIST_RESPONSE $(date_format %d.%m.%Y +2)
{$(adv_trip_list_item $(get point_dep_UT461) UT461 $(dd +2) VKO 0)})


$(REMOVE_GANGWAY $(get point_dep_UT580))
$(CLOSE_BOARDING $(get point_dep_UT461))
$(REMOVE_GANGWAY $(get point_dep_UT804))
$(REMOVE_GANGWAY $(get point_dep_UT298))

$(GET_ADV_TRIP_LIST_BOARDING_REQUEST $(date_format %d.%m.%Y +2) lang=EN capture=on)
$(GET_ADV_TRIP_LIST_RESPONSE $(date_format %d.%m.%Y +2))






