include(ts/macro.ts)
include(ts/adm_macro.ts)
include(ts/spp/write_dests_macro.ts)
include(ts/spp/write_trips_macro.ts)
include(ts/spp/read_trips_macro.ts)

# meta: suite stages

$(defmacro PREPARE_STAGE_SETS
{
$(init_jxt_pult Œ‚‚‹€)
$(login VLAD GHJHSD)

$(cache VLAD RU STAGE_SETS $(cache_iface_ver STAGE_SETS) ""
  insert stage_id:20 airline:   airp:    pr_auto:1
  insert stage_id:25 airline:   airp:    pr_auto:0
  insert stage_id:20 airline:’ airp:    pr_auto:0
  insert stage_id:25 airline:’ airp:    pr_auto:1
  insert stage_id:20 airline:‘“ airp:    pr_auto:1
  insert stage_id:25 airline:‘“ airp:    pr_auto:0
  insert stage_id:20 airline:   airp:‚Š pr_auto:0
  insert stage_id:25 airline:   airp:‚Š pr_auto:1
  insert stage_id:20 airline:’ airp:‚Š pr_auto:1
  insert stage_id:25 airline:’ airp:‚Š pr_auto:0
  insert stage_id:20 airline:‘“ airp:‚Š pr_auto:0
  insert stage_id:25 airline:‘“ airp:‚Š pr_auto:1
  insert stage_id:20 airline:   airp:‘— pr_auto:1
  insert stage_id:25 airline:   airp:‘— pr_auto:0
  insert stage_id:20 airline:’ airp:‘— pr_auto:0
  insert stage_id:25 airline:’ airp:‘— pr_auto:1
  insert stage_id:20 airline:‘“ airp:‘— pr_auto:1
  insert stage_id:25 airline:‘“ airp:‘— pr_auto:0
)

$(set id_00 $(last_history_row_id stage_sets -17))
$(set id_01 $(last_history_row_id stage_sets -16))
$(set id_02 $(last_history_row_id stage_sets -15))
$(set id_03 $(last_history_row_id stage_sets -14))
$(set id_04 $(last_history_row_id stage_sets -13))
$(set id_05 $(last_history_row_id stage_sets -12))
$(set id_06 $(last_history_row_id stage_sets -11))
$(set id_07 $(last_history_row_id stage_sets -10))
$(set id_08 $(last_history_row_id stage_sets -09))
$(set id_09 $(last_history_row_id stage_sets -08))
$(set id_10 $(last_history_row_id stage_sets -07))
$(set id_11 $(last_history_row_id stage_sets -06))
$(set id_12 $(last_history_row_id stage_sets -05))
$(set id_13 $(last_history_row_id stage_sets -04))
$(set id_14 $(last_history_row_id stage_sets -03))
$(set id_15 $(last_history_row_id stage_sets -02))
$(set id_16 $(last_history_row_id stage_sets -01))
$(set id_17 $(last_history_row_id stage_sets))



$(init_term)
$(set_user_time_type LocalAirp PIKE)
})

$(defmacro stage_sets_row
  id
  stage_id
  stage_name_view
  airline
  airline_view
  airp
  airp_view
  pr_auto
{        <row pr_del='0'>
          <col>$(id)</col>
          <col>$(stage_id)</col>
          <col>$(stage_name_view)</col>\
$(if $(eq $(airline) "") {
          <col/>} {
          <col>$(airline)</col>})\
$(if $(eq $(airline_view) "") {
          <col/>} {
          <col>$(airline_view)</col>})\
$(if $(eq $(airp) "") {
          <col/>} {
          <col>$(airp)</col>})\
$(if $(eq $(airp) "") {
          <col/>} {
          <col>$(airp_view)</col>})
          <col>$(pr_auto)</col>
        </row>})

### test 1 - ¯à®¢¥àï¥¬ trip_final_stages
### ªãç  à¥©á®¢ ¢ à §­ëå áâ âãá å - ¯à®¢¥àï¥¬ ¨å ¤®áâã¯­®áâì ¢ á¯¨áª å à¥©á®¢ ¢ à §­ëå ¬®¤ã«ïå
#########################################################################################

$(init_jxt_pult Œ‚‚‹€)
$(login VLAD GHJHSD)

$(UPDATE_USER PIKE "Š‚€‹…‚ .€." 1
{          <airps>
            <item>‚Š</item>
            <item>‘—</item>
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

%%

### test 2 - ªíè STAGE_SETS
### çâ¥­¨¥ á à §­ë¬¨ ¯à ¢ ¬¨
#########################################################################################

$(PREPARE_STAGE_SETS)

$(SUPPORT_USER_PIKE)

!! capture=on
$(cache PIKE EN STAGE_SETS $(cache_iface_ver STAGE_SETS) "")

>> lines=auto
      <rows tid='-1'>
$(stage_sets_row $(get id_10) 20 "Check-in opening"     ‘“ SU ‚Š VKO 0)
$(stage_sets_row $(get id_11) 25 "Web check-in opening" ‘“ SU ‚Š VKO 1)
$(stage_sets_row $(get id_16) 20 "Check-in opening"     ‘“ SU ‘— AER 1)
$(stage_sets_row $(get id_17) 25 "Web check-in opening" ‘“ SU ‘— AER 0)
$(stage_sets_row $(get id_04) 20 "Check-in opening"     ‘“ SU ""  ""  1)
$(stage_sets_row $(get id_05) 25 "Web check-in opening" ‘“ SU ""  ""  0)
$(stage_sets_row $(get id_08) 20 "Check-in opening"     ’ UT ‚Š VKO 1)
$(stage_sets_row $(get id_09) 25 "Web check-in opening" ’ UT ‚Š VKO 0)
$(stage_sets_row $(get id_14) 20 "Check-in opening"     ’ UT ‘— AER 0)
$(stage_sets_row $(get id_15) 25 "Web check-in opening" ’ UT ‘— AER 1)
$(stage_sets_row $(get id_02) 20 "Check-in opening"     ’ UT ""  ""  0)
$(stage_sets_row $(get id_03) 25 "Web check-in opening" ’ UT ""  ""  1)
$(stage_sets_row $(get id_06) 20 "Check-in opening"     "" "" ‚Š VKO 0)
$(stage_sets_row $(get id_07) 25 "Web check-in opening" "" "" ‚Š VKO 1)
$(stage_sets_row $(get id_12) 20 "Check-in opening"     "" "" ‘— AER 1)
$(stage_sets_row $(get id_13) 25 "Web check-in opening" "" "" ‘— AER 0)
$(stage_sets_row $(get id_00) 20 "Check-in opening"     "" "" ""  ""  1)
$(stage_sets_row $(get id_01) 25 "Web check-in opening" "" "" ""  ""  0)
      </rows>


$(SUPPORT_USER_PIKE "" "" "" ’ ”‚ "")

!! capture=on
$(cache PIKE EN STAGE_SETS $(cache_iface_ver STAGE_SETS) "")

>> lines=auto
      <rows tid='-1'>
$(stage_sets_row $(get id_08) 20 "Check-in opening"     ’ UT ‚Š VKO 1)
$(stage_sets_row $(get id_09) 25 "Web check-in opening" ’ UT ‚Š VKO 0)
$(stage_sets_row $(get id_14) 20 "Check-in opening"     ’ UT ‘— AER 0)
$(stage_sets_row $(get id_15) 25 "Web check-in opening" ’ UT ‘— AER 1)
$(stage_sets_row $(get id_02) 20 "Check-in opening"     ’ UT ""  ""  0)
$(stage_sets_row $(get id_03) 25 "Web check-in opening" ’ UT ""  ""  1)
$(stage_sets_row $(get id_06) 20 "Check-in opening"     "" "" ‚Š VKO 0)
$(stage_sets_row $(get id_07) 25 "Web check-in opening" "" "" ‚Š VKO 1)
$(stage_sets_row $(get id_12) 20 "Check-in opening"     "" "" ‘— AER 1)
$(stage_sets_row $(get id_13) 25 "Web check-in opening" "" "" ‘— AER 0)
$(stage_sets_row $(get id_00) 20 "Check-in opening"     "" "" ""  ""  1)
$(stage_sets_row $(get id_01) 25 "Web check-in opening" "" "" ""  ""  0)
      </rows>


$(SUPPORT_USER_PIKE "" ‚Š ‹Š "" "" "")

!! capture=on
$(cache PIKE EN STAGE_SETS $(cache_iface_ver STAGE_SETS) "")

>> lines=auto
      <rows tid='-1'>
$(stage_sets_row $(get id_10) 20 "Check-in opening"     ‘“ SU ‚Š VKO 0)
$(stage_sets_row $(get id_11) 25 "Web check-in opening" ‘“ SU ‚Š VKO 1)
$(stage_sets_row $(get id_04) 20 "Check-in opening"     ‘“ SU ""  ""  1)
$(stage_sets_row $(get id_05) 25 "Web check-in opening" ‘“ SU ""  ""  0)
$(stage_sets_row $(get id_08) 20 "Check-in opening"     ’ UT ‚Š VKO 1)
$(stage_sets_row $(get id_09) 25 "Web check-in opening" ’ UT ‚Š VKO 0)
$(stage_sets_row $(get id_02) 20 "Check-in opening"     ’ UT ""  ""  0)
$(stage_sets_row $(get id_03) 25 "Web check-in opening" ’ UT ""  ""  1)
$(stage_sets_row $(get id_06) 20 "Check-in opening"     "" "" ‚Š VKO 0)
$(stage_sets_row $(get id_07) 25 "Web check-in opening" "" "" ‚Š VKO 1)
$(stage_sets_row $(get id_00) 20 "Check-in opening"     "" "" ""  ""  1)
$(stage_sets_row $(get id_01) 25 "Web check-in opening" "" "" ""  ""  0)
      </rows>

$(SUPPORT_USER_PIKE "" "" ‘— "" "" ‘“)

!! capture=on
$(cache PIKE EN STAGE_SETS $(cache_iface_ver STAGE_SETS) "")

>> lines=auto
      <rows tid='-1'>
$(stage_sets_row $(get id_16) 20 "Check-in opening"     ‘“ SU ‘— AER 1)
$(stage_sets_row $(get id_17) 25 "Web check-in opening" ‘“ SU ‘— AER 0)
$(stage_sets_row $(get id_04) 20 "Check-in opening"     ‘“ SU ""  ""  1)
$(stage_sets_row $(get id_05) 25 "Web check-in opening" ‘“ SU ""  ""  0)
$(stage_sets_row $(get id_12) 20 "Check-in opening"     "" "" ‘— AER 1)
$(stage_sets_row $(get id_13) 25 "Web check-in opening" "" "" ‘— AER 0)
$(stage_sets_row $(get id_00) 20 "Check-in opening"     "" "" ""  ""  1)
$(stage_sets_row $(get id_01) 25 "Web check-in opening" "" "" ""  ""  0)
      </rows>

$(AIRPORT_USER_PIKE ‚Š ‹Š ‘— "" "" "")

!! capture=on
$(cache PIKE EN STAGE_SETS $(cache_iface_ver STAGE_SETS) "")

>> lines=auto
      <rows tid='-1'>
$(stage_sets_row $(get id_10) 20 "Check-in opening"     ‘“ SU ‚Š VKO 0)
$(stage_sets_row $(get id_11) 25 "Web check-in opening" ‘“ SU ‚Š VKO 1)
$(stage_sets_row $(get id_16) 20 "Check-in opening"     ‘“ SU ‘— AER 1)
$(stage_sets_row $(get id_17) 25 "Web check-in opening" ‘“ SU ‘— AER 0)
$(stage_sets_row $(get id_04) 20 "Check-in opening"     ‘“ SU ""  ""  1)
$(stage_sets_row $(get id_05) 25 "Web check-in opening" ‘“ SU ""  ""  0)
$(stage_sets_row $(get id_08) 20 "Check-in opening"     ’ UT ‚Š VKO 1)
$(stage_sets_row $(get id_09) 25 "Web check-in opening" ’ UT ‚Š VKO 0)
$(stage_sets_row $(get id_14) 20 "Check-in opening"     ’ UT ‘— AER 0)
$(stage_sets_row $(get id_15) 25 "Web check-in opening" ’ UT ‘— AER 1)
$(stage_sets_row $(get id_02) 20 "Check-in opening"     ’ UT ""  ""  0)
$(stage_sets_row $(get id_03) 25 "Web check-in opening" ’ UT ""  ""  1)
$(stage_sets_row $(get id_06) 20 "Check-in opening"     "" "" ‚Š VKO 0)
$(stage_sets_row $(get id_07) 25 "Web check-in opening" "" "" ‚Š VKO 1)
$(stage_sets_row $(get id_12) 20 "Check-in opening"     "" "" ‘— AER 1)
$(stage_sets_row $(get id_13) 25 "Web check-in opening" "" "" ‘— AER 0)
$(stage_sets_row $(get id_00) 20 "Check-in opening"     "" "" ""  ""  1)
$(stage_sets_row $(get id_01) 25 "Web check-in opening" "" "" ""  ""  0)
      </rows>

$(AIRPORT_USER_PIKE "" "" ‘— "" "" ‘“)

!! capture=on
$(cache PIKE EN STAGE_SETS $(cache_iface_ver STAGE_SETS) "")

>> lines=auto
      <rows tid='-1'>
$(stage_sets_row $(get id_16) 20 "Check-in opening"     ‘“ SU ‘— AER 1)
$(stage_sets_row $(get id_17) 25 "Web check-in opening" ‘“ SU ‘— AER 0)
$(stage_sets_row $(get id_04) 20 "Check-in opening"     ‘“ SU ""  ""  1)
$(stage_sets_row $(get id_05) 25 "Web check-in opening" ‘“ SU ""  ""  0)
$(stage_sets_row $(get id_12) 20 "Check-in opening"     "" "" ‘— AER 1)
$(stage_sets_row $(get id_13) 25 "Web check-in opening" "" "" ‘— AER 0)
$(stage_sets_row $(get id_00) 20 "Check-in opening"     "" "" ""  ""  1)
$(stage_sets_row $(get id_01) 25 "Web check-in opening" "" "" ""  ""  0)
      </rows>

$(AIRLINE_USER_PIKE "" "" "" "" ”‚ ‘“)

!! capture=on
$(cache PIKE EN STAGE_SETS $(cache_iface_ver STAGE_SETS) "")

>> lines=auto
      <rows tid='-1'>
$(stage_sets_row $(get id_10) 20 "Check-in opening"     ‘“ SU ‚Š VKO 0)
$(stage_sets_row $(get id_11) 25 "Web check-in opening" ‘“ SU ‚Š VKO 1)
$(stage_sets_row $(get id_16) 20 "Check-in opening"     ‘“ SU ‘— AER 1)
$(stage_sets_row $(get id_17) 25 "Web check-in opening" ‘“ SU ‘— AER 0)
$(stage_sets_row $(get id_04) 20 "Check-in opening"     ‘“ SU ""  ""  1)
$(stage_sets_row $(get id_05) 25 "Web check-in opening" ‘“ SU ""  ""  0)
$(stage_sets_row $(get id_06) 20 "Check-in opening"     "" "" ‚Š VKO 0)
$(stage_sets_row $(get id_07) 25 "Web check-in opening" "" "" ‚Š VKO 1)
$(stage_sets_row $(get id_12) 20 "Check-in opening"     "" "" ‘— AER 1)
$(stage_sets_row $(get id_13) 25 "Web check-in opening" "" "" ‘— AER 0)
$(stage_sets_row $(get id_00) 20 "Check-in opening"     "" "" ""  ""  1)
$(stage_sets_row $(get id_01) 25 "Web check-in opening" "" "" ""  ""  0)
      </rows>

$(AIRLINE_USER_PIKE "" "" ‘— "" "" ‘“)

!! capture=on
$(cache PIKE EN STAGE_SETS $(cache_iface_ver STAGE_SETS) "")

>> lines=auto
      <rows tid='-1'>
$(stage_sets_row $(get id_16) 20 "Check-in opening"     ‘“ SU ‘— AER 1)
$(stage_sets_row $(get id_17) 25 "Web check-in opening" ‘“ SU ‘— AER 0)
$(stage_sets_row $(get id_04) 20 "Check-in opening"     ‘“ SU ""  ""  1)
$(stage_sets_row $(get id_05) 25 "Web check-in opening" ‘“ SU ""  ""  0)
$(stage_sets_row $(get id_12) 20 "Check-in opening"     "" "" ‘— AER 1)
$(stage_sets_row $(get id_13) 25 "Web check-in opening" "" "" ‘— AER 0)
$(stage_sets_row $(get id_00) 20 "Check-in opening"     "" "" ""  ""  1)
$(stage_sets_row $(get id_01) 25 "Web check-in opening" "" "" ""  ""  0)
      </rows>

$(AIRPORT_USER_PIKE)

!! capture=on
$(cache PIKE EN STAGE_SETS $(cache_iface_ver STAGE_SETS) "")

>> lines=auto
      <rows tid='-1'/>

$(AIRPORT_USER_PIKE "" "" "" ’ ”‚ ‘“)

!! capture=on
$(cache PIKE EN STAGE_SETS $(cache_iface_ver STAGE_SETS) "")

>> lines=auto
      <rows tid='-1'/>

$(AIRLINE_USER_PIKE)

!! capture=on
$(cache PIKE EN STAGE_SETS $(cache_iface_ver STAGE_SETS) "")

>> lines=auto
      <rows tid='-1'/>

$(AIRLINE_USER_PIKE ‚Š ‹Š ‘— "" "" "")

!! capture=on
$(cache PIKE EN STAGE_SETS $(cache_iface_ver STAGE_SETS) "")

>> lines=auto
      <rows tid='-1'/>

%%

### test 3 - ªíè SOPP_STAGE_STATUSES
#########################################################################################

$(init_term)

!! capture=on
$(cache PIKE RU SOPP_STAGE_STATUSES $(cache_iface_ver SOPP_STAGE_STATUSES) "")

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <interface id='cache'/>
    <data>
      <code>SOPP_STAGE_STATUSES</code>
      <Forbidden>0</Forbidden>
      <ReadOnly>1</ReadOnly>
      <keep_locally>0</keep_locally>
      <keep_deleted_rows>0</keep_deleted_rows>
      <user_depend>0</user_depend>
      <rows tid='-1'>
        <row pr_del='0'>
          <col>0</col>
          <col>1</col>
          <col>¥ ªâ¨¢¥­</col>
        </row>
        <row pr_del='0'>
          <col>10</col>
          <col>1</col>
          <col>®¤£®â®¢ª </col>
        </row>
        <row pr_del='0'>
          <col>20</col>
          <col>1</col>
          <col>¥£¨áâà æ¨ï</col>
        </row>
        <row pr_del='0'>
          <col>30</col>
          <col>1</col>
          <col>ª®­ç ­¨¥</col>
        </row>
        <row pr_del='0'>
          <col>50</col>
          <col>1</col>
          <col>‡ ªàëâ</col>
        </row>
        <row pr_del='0'>
          <col>99</col>
          <col>1</col>
          <col>‚ë«¥â¥«</col>
        </row>
        <row pr_del='0'>
          <col>0</col>
          <col>2</col>
          <col>®á ¤ª¨ ­¥â</col>
        </row>
        <row pr_del='0'>
          <col>40</col>
          <col>2</col>
          <col>®á ¤ª </col>
        </row>
        <row pr_del='0'>
          <col>50</col>
          <col>2</col>
          <col>®á ¤ª  § ¢¥àè¥­ </col>
        </row>
        <row pr_del='0'>
          <col>99</col>
          <col>2</col>
          <col>‚ë«¥â¥«</col>
        </row>
        <row pr_del='0'>
          <col>0</col>
          <col>3</col>
          <col>¥ £®â®¢ ª ¯®á ¤ª¥</col>
        </row>
        <row pr_del='0'>
          <col>40</col>
          <col>3</col>
          <col>ƒ®â®¢ ª ¯®á ¤ª¥</col>
        </row>
        <row pr_del='0'>
          <col>70</col>
          <col>3</col>
          <col>’à ¯ ã¡à ­</col>
        </row>
        <row pr_del='0'>
          <col>99</col>
          <col>3</col>
          <col>‚ë«¥â¥«</col>
        </row>
        <row pr_del='0'>
          <col>25</col>
          <col>4</col>
          <col>¥£¨áâà æ¨ï</col>
        </row>
        <row pr_del='0'>
          <col>35</col>
          <col>4</col>
          <col>ª®­ç ­¨¥</col>
        </row>
        <row pr_del='0'>
          <col>26</col>
          <col>5</col>
          <col>¥£¨áâà æ¨ï</col>
        </row>
        <row pr_del='0'>
          <col>36</col>
          <col>5</col>
          <col>ª®­ç ­¨¥</col>
        </row>
        <row pr_del='0'>
          <col>25</col>
          <col>6</col>
          <col>â¬¥­  à §à¥è¥­ </col>
        </row>
        <row pr_del='0'>
          <col>31</col>
          <col>6</col>
          <col>â¬¥­  § ¯à¥é¥­ </col>
        </row>
      </rows>
    </data>
  </answer>
</term>


!! capture=on
$(cache PIKE EN SOPP_STAGE_STATUSES $(cache_iface_ver SOPP_STAGE_STATUSES) "")

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <interface id='cache'/>
    <data>
      <code>SOPP_STAGE_STATUSES</code>
      <Forbidden>0</Forbidden>
      <ReadOnly>1</ReadOnly>
      <keep_locally>0</keep_locally>
      <keep_deleted_rows>0</keep_deleted_rows>
      <user_depend>0</user_depend>
      <rows tid='-1'>
        <row pr_del='0'>
          <col>0</col>
          <col>1</col>
          <col>Inactive</col>
        </row>
        <row pr_del='0'>
          <col>10</col>
          <col>1</col>
          <col>Preparation</col>
        </row>
        <row pr_del='0'>
          <col>20</col>
          <col>1</col>
          <col>Check-in</col>
        </row>
        <row pr_del='0'>
          <col>30</col>
          <col>1</col>
          <col>Completion</col>
        </row>
        <row pr_del='0'>
          <col>50</col>
          <col>1</col>
          <col>Closed</col>
        </row>
        <row pr_del='0'>
          <col>99</col>
          <col>1</col>
          <col>Departed</col>
        </row>
        <row pr_del='0'>
          <col>0</col>
          <col>2</col>
          <col>No boarding</col>
        </row>
        <row pr_del='0'>
          <col>40</col>
          <col>2</col>
          <col>Boarding</col>
        </row>
        <row pr_del='0'>
          <col>50</col>
          <col>2</col>
          <col>Boarding completed</col>
        </row>
        <row pr_del='0'>
          <col>99</col>
          <col>2</col>
          <col>Departed</col>
        </row>
        <row pr_del='0'>
          <col>0</col>
          <col>3</col>
          <col>Not ready landing</col>
        </row>
        <row pr_del='0'>
          <col>40</col>
          <col>3</col>
          <col>Ready landing</col>
        </row>
        <row pr_del='0'>
          <col>70</col>
          <col>3</col>
          <col>Airstairs removed</col>
        </row>
        <row pr_del='0'>
          <col>99</col>
          <col>3</col>
          <col>Departed</col>
        </row>
        <row pr_del='0'>
          <col>25</col>
          <col>4</col>
          <col>Check-in</col>
        </row>
        <row pr_del='0'>
          <col>35</col>
          <col>4</col>
          <col>Completion</col>
        </row>
        <row pr_del='0'>
          <col>26</col>
          <col>5</col>
          <col>Check-in</col>
        </row>
        <row pr_del='0'>
          <col>36</col>
          <col>5</col>
          <col>Completion</col>
        </row>
        <row pr_del='0'>
          <col>25</col>
          <col>6</col>
          <col>â¬¥­  à §à¥è¥­ </col>
        </row>
        <row pr_del='0'>
          <col>31</col>
          <col>6</col>
          <col>â¬¥­  § ¯à¥é¥­ </col>
        </row>
      </rows>
    </data>
  </answer>
</term>


%%

### test 4 - ¯à®áâ¥©è¨¥ â¥áâë ­  § ¯¨áì
### ªíè¨ STAGE_SETS, GRAPH_TIMES, STAGE_NAMES
#########################################################################################

$(init_term)


### STAGE_SETS
#########################################################################################

!! capture=on
$(cache PIKE EN STAGE_SETS $(cache_iface_ver STAGE_SETS) ""
  insert stage_id:20
         airline:’
         airp:‚Š
         pr_auto:1)

$(set id $(last_history_row_id STAGE_SETS))

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <interface id='cache'/>
    <data>
      <code>STAGE_SETS</code>
      <Forbidden>0</Forbidden>
      <ReadOnly>0</ReadOnly>
      <keep_locally>0</keep_locally>
      <keep_deleted_rows>0</keep_deleted_rows>
      <user_depend>1</user_depend>
      <rows tid='-1'>
        <row pr_del='0'>
          <col>$(get id)</col>
          <col>20</col>
          <col>Check-in opening</col>
          <col>’</col>
          <col>UT</col>
          <col>‚Š</col>
          <col>VKO</col>
          <col>1</col>
        </row>
      </rows>
    </data>
    <command>
      <message lexema_id='MSG.CHANGED_DATA_COMMIT' code='0'>...</message>
    </command>
  </answer>
</term>

!! capture=on
$(cache PIKE EN STAGE_SETS $(cache_iface_ver STAGE_SETS) ""
  update old_id:$(get id) id:$(get id)
         old_stage_id:20  stage_id:25
         old_airline:’   airline:
         old_airp:‚Š     airp:€€
         old_pr_auto:1    pr_auto:0)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <interface id='cache'/>
    <data>
      <code>STAGE_SETS</code>
      <Forbidden>0</Forbidden>
      <ReadOnly>0</ReadOnly>
      <keep_locally>0</keep_locally>
      <keep_deleted_rows>0</keep_deleted_rows>
      <user_depend>1</user_depend>
      <rows tid='-1'>
        <row pr_del='0'>
          <col>$(get id)</col>
          <col>25</col>
          <col>Web check-in opening</col>
          <col/>
          <col/>
          <col>€€</col>
          <col>AAQ</col>
          <col>0</col>
        </row>
      </rows>
    </data>
    <command>
      <message lexema_id='MSG.CHANGED_DATA_COMMIT' code='0'>...</message>
    </command>
  </answer>
</term>

!! capture=on
$(cache PIKE EN STAGE_SETS $(cache_iface_ver STAGE_SETS) ""
  delete old_id:$(get id)
         old_stage_id:25
         old_airline:
         old_airp:€€
         old_pr_auto:0)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <interface id='cache'/>
    <data>
      <code>STAGE_SETS</code>
      <Forbidden>0</Forbidden>
      <ReadOnly>0</ReadOnly>
      <keep_locally>0</keep_locally>
      <keep_deleted_rows>0</keep_deleted_rows>
      <user_depend>1</user_depend>
      <rows tid='-1'/>
    </data>
    <command>
      <message lexema_id='MSG.CHANGED_DATA_COMMIT' code='0'>...</message>
    </command>
  </answer>
</term>



### GRAPH_TIMES
#########################################################################################

!! capture=on
$(cache PIKE EN GRAPH_TIMES $(cache_iface_ver GRAPH_TIMES) ""
  insert stage_id:20
         airline:’
         airp:‚Š
         craft:
         trip_type:ç
         time:125)

$(set id $(last_history_row_id GRAPH_TIMES))

>> lines=auto
      <user_depend>1</user_depend>
      <rows tid='-1'>
        <row pr_del='0'>
          <col>$(get id)</col>
          <col>20</col>
          <col>Check-in opening</col>
          <col>’</col>
          <col>UT</col>
          <col>‚Š</col>
          <col>VKO</col>
          <col/>
          <col/>
          <col>ç</col>
          <col>h</col>
          <col>125</col>
        </row>
      </rows>

!! capture=on
$(cache PIKE EN GRAPH_TIMES $(cache_iface_ver GRAPH_TIMES) ""
  update old_id:$(get id)  id:$(get id)
         old_stage_id:20   stage_id:25
         old_airline:’    airline:‘“
         old_airp:‚Š      airp:
         old_craft:        craft:’“5
         old_trip_type:ç   trip_type:
         old_time:125      time:135)

>> lines=auto
      <user_depend>1</user_depend>
      <rows tid='-1'>
        <row pr_del='0'>
          <col>$(get id)</col>
          <col>25</col>
          <col>Web check-in opening</col>
          <col>‘“</col>
          <col>SU</col>
          <col/>
          <col/>
          <col>’“5</col>
          <col>TU5</col>
          <col/>
          <col/>
          <col>135</col>
        </row>
      </rows>

!! capture=on
$(cache PIKE EN GRAPH_TIMES $(cache_iface_ver GRAPH_TIMES) ""
  delete old_id:$(get id)
         old_stage_id:25
         old_airline:‘“
         old_airp:
         old_craft:’“5
         old_trip_type:
         old_time:135)

>> lines=auto
      <user_depend>1</user_depend>
      <rows tid='-1'/>



### STAGE_NAMES
#########################################################################################

!! capture=on
$(cache PIKE EN STAGE_NAMES $(cache_iface_ver STAGE_NAMES) ""
  insert stage_id:20
         airp:€€
         {name:¥£¨áâà æ¨ï ®âªàë¢ ¥âáï ¢ €­ ¯¥})

$(set id $(last_history_row_id STAGE_NAMES))

>> lines=auto
        <row pr_del='0'>
          <col>20</col>
          <col>Check-in opening</col>
          <col>€€</col>
          <col>AAQ</col>
          <col>¥£¨áâà æ¨ï ®âªàë¢ ¥âáï ¢ €­ ¯¥</col>
          <col/>
          <col>$(get id)</col>
        </row>

!! capture=on
$(cache PIKE EN STAGE_NAMES $(cache_iface_ver STAGE_NAMES) ""
  update old_id:$(get id)  id:$(get id)
         old_stage_id:20   stage_id:25
         old_airp:€€      airp:€€
         {old_name:¥£¨áâà æ¨ï ®âªàë¢ ¥âáï ¢ €­ ¯¥}
         {name:‚¥¡-à¥£¨áâà æ¨ï ®âªàë¢ ¥âáï ¢ €­ ¯¥})

>> lines=auto
        <row pr_del='0'>
          <col>25</col>
          <col>Web check-in opening</col>
          <col>€€</col>
          <col>AAQ</col>
          <col>‚¥¡-à¥£¨áâà æ¨ï ®âªàë¢ ¥âáï ¢ €­ ¯¥</col>
          <col/>
          <col>$(get id)</col>
        </row>

!! capture=on
$(cache PIKE EN STAGE_NAMES $(cache_iface_ver STAGE_NAMES) ""
  delete old_id:$(get id)
         old_stage_id:25
         old_airp:€€
         {old_name:‚¥¡-à¥£¨áâà æ¨ï ®âªàë¢ ¥âáï ¢ €­ ¯¥})

>> mode=!regex
.*
          <col>$(get id)</col>
.*
