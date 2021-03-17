include(ts/macro.ts)
include(ts/adm_macro.ts)
include(ts/spp/write_dests_macro.ts)
include(ts/spp/write_trips_macro.ts)
include(ts/spp/read_trips_macro.ts)

# meta: suite prep_checkin

$(defmacro GET_TRIP_INFO_REQUEST
  point_dep
  lang=RU
  capture=off
{

!! capture=$(capture)
<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='trips' ver='1' opr='PIKE' screen='PREPREG.EXE' mode='STAND' lang='$(lang)' term_id='2479792165'>
    <GetTripInfo>
      <point_id>$(point_dep)</point_id>
      <refresh_type>0</refresh_type>
      <tripheader/>
      <tripcounters/>
      <crsdata/>
    </GetTripInfo>
  </query>
</term>

})

$(defmacro tripcounters_item
  firstcol
  cfg
  resa
  tranzit
  block
  avail
  prot
{        <item>
          <firstcol>$(firstcol)</firstcol>\
$(if $(eq $(cfg) "") {
          <cfg/>} {
          <cfg>$(cfg)</cfg>})\
$(if $(eq $(resa) "") {
          <resa/>} {
          <resa>$(resa)</resa>})\
$(if $(eq $(tranzit) "") {
          <tranzit/>} {
          <tranzit>$(tranzit)</tranzit>})\
$(if $(eq $(block) "") {
          <block/>} {
          <block>$(block)</block>})\
$(if $(eq $(avail) "") {
          <avail/>} {
          <avail>$(avail)</avail>})\
$(if $(eq $(prot) "") {
          <prot/>} {
          <prot>$(prot)</prot>})
        </item>})

$(defmacro crs_item
  code
  name
  pr_charge
  pr_list
  pr_crs_main
{          <itemcrs>\
$(if $(eq $(code) "") {
            <code/>} {
            <code>$(code)</code>})
            <name>$(name)</name>
            <pr_charge>$(if $(eq $(pr_charge) "x") 1 0)</pr_charge>
            <pr_list>$(if $(eq $(pr_list) "x") 1 0)</pr_list>
            <pr_crs_main>$(if $(eq $(pr_crs_main) "x") 1 0)</pr_crs_main>
          </itemcrs>})

$(defmacro crsdata_item
  crs
  target
  class
  resa
  tranzit
{          <itemcrs>\
$(if $(eq $(crs) "") {
            <crs/>} {
            <crs>$(crs)</crs>})
            <target>$(target)</target>
            <class>$(class)</class>
            <resa>$(resa)</resa>
            <tranzit>$(tranzit)</tranzit>
          </itemcrs>})

$(defmacro crsdata_update
  target
  class
  resa
  tranzit
{        <item>
          <target>$(get_elem_id etAirp $(target))</target>
          <class>$(get_elem_id etClass $(class))</class>
          <resa>$(resa)</resa>
          <tranzit>$(tranzit)</tranzit>
        </item>})

$(defmacro CRS_DATA_UPDATE_REQUEST
  point_dep
  update_section
  lang=RU
  capture=off
{

!! capture=$(capture)
<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='prepreg' ver='1' opr='PIKE' screen='PREPREG.EXE' mode='STAND' lang='$(lang)' term_id='2479792165'>
    <CrsDataApplyUpdates>
      <point_id>$(point_dep)</point_id>
      <question>1</question>
      <crsdata>
$(update_section)
      </crsdata>
      <tripcounters/>
    </CrsDataApplyUpdates>
  </query>
</term>

})

### test 1 - одно плечо, телеграммы PNL/ADL не пришли, вводятся данные вручную
#########################################################################################

$(init_term)
$(set_user_time_type LocalAirp PIKE)

$(set airline UT)
$(set flt_no 243)
$(set craft TU3)
$(set bort 65021)
$(set airp_dep DME)
$(set time_dep "$(date_format %d.%m.%Y +1) 07:00")
$(set time_arv "$(date_format %d.%m.%Y +1) 10:00")
$(set airp_arv AYT)

$(NEW_SPP_FLIGHT_ONE_LEG $(get airline) $(get flt_no) $(get craft) $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv) bort="")

$(set point_dep $(get_point_dep_for_flight $(get airline) $(get flt_no) "" $(yymmdd +1) $(get airp_dep)))

$(PREP_CHECKIN $(get point_dep))

$(GET_TRIP_INFO_REQUEST capture=on $(get point_dep))

>> lines=auto
      <tripcounters>
$(tripcounters_item Всего)
      </tripcounters>
      <tripdata>
        <airps>
          <airp>$(get_elem_id etAirp $(get airp_arv))</airp>
        </airps>
        <classes/>
        <crs>
$(crs_item "" "Общие данные" "" "" "")
        </crs>
        <crsdata/>
      </tripdata>

$(CHANGE_SPP_FLIGHT_ONE_LEG $(get point_dep) "" "" $(get airline) $(get flt_no) $(get craft) $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv) $(get bort))

$(GET_TRIP_INFO_REQUEST capture=on $(get point_dep))

###                       cfg resa tranzit block avail prot
>> lines=auto
      <tripcounters>
$(tripcounters_item Всего  68    0       0     0    68    0)
$(tripcounters_item Б       8    0       0     0     8    0)
$(tripcounters_item Э      60    0       0     0    60    0)
$(tripcounters_item АЯТ    68    0       0     0    68    0)
      </tripcounters>
      <tripdata>
        <airps>
          <airp>$(get_elem_id etAirp $(get airp_arv))</airp>
        </airps>
        <classes>
          <class>$(get_elem_id etClass C)</class>
          <class>$(get_elem_id etClass Y)</class>
        </classes>
        <crs>
$(crs_item "" "Общие данные" "" "" "")
        </crs>
        <crsdata/>
      </tripdata>

$(CRS_DATA_UPDATE_REQUEST capture=on $(get point_dep)
{$(crsdata_update АЯТ П  1  0)
 $(crsdata_update АЯТ Б  8  2)
 $(crsdata_update АЯТ Э 22  4)})

###                       cfg resa tranzit block avail prot
>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...>
    <data>
      <tripcounters>
$(tripcounters_item Всего  68   30       0     0    38    0)
$(tripcounters_item Б       8    8       0     0     0    0)
$(tripcounters_item Э      60   22       0     0    38    0)
$(tripcounters_item АЯТ    68   30       0     0    38    0)
      </tripcounters>
    </data>
  </answer>
</term>

!! capture=on
$(READ_TRIPS $(date_format %d.%m.%Y +1) EN)

>> lines=auto
          <places_out>
            <airp>AYT</airp>
          </places_out>
          <classes>
            <class cfg='8'>C</class>
            <class cfg='60'>Y</class>
          </classes>
          <resa>31</resa>
          <stages>

%%

### test 2
### одно плечо с несовпадающими пунктами назначения
### потом поправляем маршрут
### далее экспериментируем с признаком нецифровой PNL и с приоритетами отправителей
#########################################################################################

$(init_term)
$(set_user_time_type LocalAirp PIKE)

$(set airline UT)
$(set flt_no 243)
$(set craft TU3)
$(set bort 65021)
$(set airp_dep DME)
$(set time_dep "$(date_format %d.%m.%Y +1) 07:00")
$(set time_arv "$(date_format %d.%m.%Y +1) 10:00")
$(set airp_arv_err AER)
$(set airp_arv AYT)

$(NEW_SPP_FLIGHT_ONE_LEG $(get airline) $(get flt_no) $(get craft) $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv_err) $(get bort))

$(set point_dep $(get_point_dep_for_flight $(get airline) $(get flt_no) "" $(yymmdd +1) $(get airp_dep)))

$(PREP_CHECKIN $(get point_dep))

$(sql "INSERT INTO typeb_senders(code, name) VALUES('SENDER2', 'SENDER2')")

$(cache PIKE RU CRS_SET $(cache_iface_ver CRS_SET) ""
  insert airline:$(get_elem_id etAirline $(get airline))
         airp_dep:$(get_elem_id etAirp $(get airp_dep))
         crs:SENDER2
         priority:0
         pr_numeric_pnl:1)

<<
MOWKK1H
.SENDER3 $(dd)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART1
RBD F/F C/CJIDA Y/YSTEQGNBXWUORVHLKPZ
AVAIL
 $(get airp_dep)  $(get airp_arv)
C005
Y033
ENDPNL

<<
MOWKK1H
.SENDER1 $(dd)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART1
RBD C/CJIDA Y/YSTEQGNBXWUORVHLKPZF
AVAIL
 $(get airp_dep)  $(get airp_arv)
C010
Y040
-$(get airp_arv)001C-PAD000
-$(get airp_arv)002J-PAD001
-$(get airp_arv)001Y
-$(get airp_arv)002S-PAD002
-$(get airp_arv)004T-PAD003
-$(get airp_arv)008F
ENDPNL

<<
MOWKK1H
.SENDER2 $(dd)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART1
RBD F/F C/CJIDA Y/YSTEQGNBXWUORVHLKPZ
-$(get airp_arv)004F
-$(get airp_arv)000C
-$(get airp_arv)000J
-$(get airp_arv)001Y
-$(get airp_arv)001S
-$(get airp_arv)000T
ENDPNL

$(GET_TRIP_INFO_REQUEST capture=on $(get point_dep) EN)

###                       cfg resa tranzit block avail prot
>> lines=auto
      <tripcounters>
$(tripcounters_item Total  68    0       0     0    68    0)
$(tripcounters_item C       8    0       0     0     8    0)
$(tripcounters_item Y      60    0       0     0    60    0)
$(tripcounters_item AER    68    0       0     0    68    0)
      </tripcounters>
      <tripdata>
        <airps>
          <airp>$(get_elem_id etAirp $(get airp_arv_err))</airp>
        </airps>
        <classes>
          <class>$(get_elem_id etClass C)</class>
          <class>$(get_elem_id etClass Y)</class>
        </classes>
        <crs>
$(crs_item ""        "Common data" ""  "" "")
$(crs_item "SENDER1" "SENDER1"     "x" "" "")
$(crs_item "SENDER2" "SENDER2"     "x" "" "")
$(crs_item "SENDER3" "SENDER3"     "x" "" "")
        </crs>
        <crsdata>
$(crsdata_item "SENDER1" АЯТ Б  3 -1)
$(crsdata_item "SENDER1" АЯТ Э 15 -1)
$(crsdata_item "SENDER2" АЯТ Б  0 -1)
$(crsdata_item "SENDER2" АЯТ П  4 -1)
$(crsdata_item "SENDER2" АЯТ Э  2 -1)
$(crsdata_item ""        АЯТ Б  3  0)
$(crsdata_item ""        АЯТ П  4  0)
$(crsdata_item ""        АЯТ Э 17  0)
        </crsdata>
      </tripdata>

!! capture=on
$(READ_TRIPS $(date_format %d.%m.%Y +1) EN)

>> lines=auto
          <places_out>
            <airp>AER</airp>
          </places_out>
          <classes>
            <class cfg='8'>C</class>
            <class cfg='60'>Y</class>
          </classes>
          <resa>24</resa>
          <stages>

$(CHANGE_SPP_FLIGHT_ONE_LEG $(get point_dep) "" "" $(get airline) $(get flt_no) $(get craft) $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv) $(get bort))

$(sql "INSERT INTO typeb_senders(code, name) VALUES('SENDER0', 'SENDER0')")

$(cache PIKE RU CRS_SET $(cache_iface_ver CRS_SET) ""
  insert airline:$(get_elem_id etAirline $(get airline))
         crs:SENDER0
         priority:0
         pr_numeric_pnl:0)

<<
MOWKK1H
.SENDER2 $(dd)1201
PNL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART1
RBD F/F C/CJIDA Y/YSTEQGNBXWUORVHLKPZ
-$(get airp_arv)004F
-$(get airp_arv)000C
-$(get airp_arv)000J
-$(get airp_arv)001Y
1ПУПКИН/ВАСЯ
-$(get airp_arv)001S
-$(get airp_arv)000T
ENDPNL

$(GET_TRIP_INFO_REQUEST capture=on $(get point_dep) EN)

###                       cfg resa tranzit block avail prot
>> lines=auto
      <tripcounters>
$(tripcounters_item Total  68   20       0     0    48    0)
$(tripcounters_item C       8    3       0     0     5    0)
$(tripcounters_item Y      60   17       0     0    43    0)
$(tripcounters_item AYT    68   20       0     0    48    0)
      </tripcounters>
      <tripdata>
        <airps>
          <airp>$(get_elem_id etAirp $(get airp_arv))</airp>
        </airps>
        <classes>
          <class>$(get_elem_id etClass C)</class>
          <class>$(get_elem_id etClass Y)</class>
        </classes>
        <crs>
$(crs_item ""        "Common data" " "  " "  " ")
$(crs_item "SENDER0" "SENDER0"     " "  " "  " ")
$(crs_item "SENDER1" "SENDER1"     "x"  " "  " ")
$(crs_item "SENDER2" "SENDER2"     "x"  "x"  " ")
$(crs_item "SENDER3" "SENDER3"     "x"  " "  " ")
        </crs>
        <crsdata>
$(crsdata_item "SENDER1" АЯТ Б  3 -1)
$(crsdata_item "SENDER1" АЯТ Э 15 -1)
$(crsdata_item "SENDER2" АЯТ Б  0 -1)
$(crsdata_item "SENDER2" АЯТ П  4 -1)
$(crsdata_item "SENDER2" АЯТ Э  2 -1)
$(crsdata_item ""        АЯТ Б  3  0)
$(crsdata_item ""        АЯТ П  4  0)
$(crsdata_item ""        АЯТ Э 17  0)
        </crsdata>
      </tripdata>

$(cache PIKE RU CRS_SET $(cache_iface_ver CRS_SET) ""
  insert airline:$(get_elem_id etAirline $(get airline))
         flt_no:$(get flt_no)
         airp_dep:$(get_elem_id etAirp $(get airp_dep))
         crs:SENDER0
         priority:1
         pr_numeric_pnl:0)

### посылаем пустую ADL, чтобы перерасчитались счетчики

<<
MOWKK1H
.SENDER2 $(dd)1202
ADL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART1
RBD F/F C/CJIDA Y/YSTEQGNBXWUORVHLKPZ
-$(get airp_arv)004F
-$(get airp_arv)000C
-$(get airp_arv)000J
-$(get airp_arv)001Y
-$(get airp_arv)001S
-$(get airp_arv)000T
ENDADL

$(GET_TRIP_INFO_REQUEST capture=on $(get point_dep) EN)

###                       cfg resa tranzit block avail prot
>> lines=auto
      <tripcounters>
$(tripcounters_item Total  68    0       0     0    68    0)
$(tripcounters_item C       8    0       0     0     8    0)
$(tripcounters_item Y      60    0       0     0    60    0)
$(tripcounters_item AYT    68    0       0     0    68    0)
      </tripcounters>
      <tripdata>
        <airps>
          <airp>$(get_elem_id etAirp $(get airp_arv))</airp>
        </airps>
        <classes>
          <class>$(get_elem_id etClass C)</class>
          <class>$(get_elem_id etClass Y)</class>
        </classes>
        <crs>
$(crs_item ""        "Common data" " "  " "  " ")
$(crs_item "SENDER0" "SENDER0"     " "  " "  "x")
$(crs_item "SENDER1" "SENDER1"     "x"  " "  " ")
$(crs_item "SENDER2" "SENDER2"     "x"  "x"  " ")
$(crs_item "SENDER3" "SENDER3"     "x"  " "  " ")
        </crs>
        <crsdata>
$(crsdata_item "SENDER1" АЯТ Б  3 -1)
$(crsdata_item "SENDER1" АЯТ Э 15 -1)
$(crsdata_item "SENDER2" АЯТ Б  0 -1)
$(crsdata_item "SENDER2" АЯТ П  4 -1)
$(crsdata_item "SENDER2" АЯТ Э  2 -1)
        </crsdata>
      </tripdata>

$(cache PIKE RU CRS_SET $(cache_iface_ver CRS_SET) ""
  insert airline:$(get_elem_id etAirline $(get airline))
         flt_no:$(get flt_no)
         airp_dep:$(get_elem_id etAirp $(get airp_dep))
         crs:SENDER3
         priority:1
         pr_numeric_pnl:0)

### посылаем пустую ADL, чтобы перерасчитались счетчики

<<
MOWKK1H
.SENDER2 $(dd)1203
ADL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART1
RBD F/F C/CJIDA Y/YSTEQGNBXWUORVHLKPZ
-$(get airp_arv)004F
-$(get airp_arv)000C
-$(get airp_arv)000J
-$(get airp_arv)001Y
-$(get airp_arv)001S
-$(get airp_arv)000T
ENDADL

$(GET_TRIP_INFO_REQUEST capture=on $(get point_dep) EN)

>> lines=auto
      <tripcounters>
$(tripcounters_item Total  68    0       0     0    68    0)
$(tripcounters_item C       8    0       0     0     8    0)
$(tripcounters_item Y      60    0       0     0    60    0)
$(tripcounters_item AYT    68    0       0     0    68    0)
      </tripcounters>
      <tripdata>
        <airps>
          <airp>$(get_elem_id etAirp $(get airp_arv))</airp>
        </airps>
        <classes>
          <class>$(get_elem_id etClass C)</class>
          <class>$(get_elem_id etClass Y)</class>
        </classes>
        <crs>
$(crs_item ""        "Common data" " "  " "  " ")
$(crs_item "SENDER0" "SENDER0"     " "  " "  "x")
$(crs_item "SENDER1" "SENDER1"     "x"  " "  " ")
$(crs_item "SENDER2" "SENDER2"     "x"  "x"  " ")
$(crs_item "SENDER3" "SENDER3"     "x"  " "  "x")
        </crs>
        <crsdata>
$(crsdata_item "SENDER1" АЯТ Б  3 -1)
$(crsdata_item "SENDER1" АЯТ Э 15 -1)
$(crsdata_item "SENDER2" АЯТ Б  0 -1)
$(crsdata_item "SENDER2" АЯТ П  4 -1)
$(crsdata_item "SENDER2" АЯТ Э  2 -1)
$(crsdata_item ""        АЯТ Б  0  0)
$(crsdata_item ""        АЯТ Э  0  0)
        </crsdata>
      </tripdata>

$(cache PIKE RU CRS_SET $(cache_iface_ver CRS_SET) ""
  insert airline:$(get_elem_id etAirline $(get airline))
         flt_no:$(get flt_no)
         airp_dep:$(get_elem_id etAirp $(get airp_dep))
         crs:SENDER2
         priority:1
         pr_numeric_pnl:0)

### посылаем пустую ADL, чтобы перерасчитались счетчики

<<
MOWKK1H
.SENDER2 $(dd)1204
ADL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART1
RBD F/F C/CJIDA Y/YSTEQGNBXWUORVHLKPZ
-$(get airp_arv)004F
-$(get airp_arv)000C
-$(get airp_arv)000J
-$(get airp_arv)001Y
-$(get airp_arv)001S
-$(get airp_arv)000T
ENDADL

$(GET_TRIP_INFO_REQUEST capture=on $(get point_dep) EN)

###                       cfg resa tranzit block avail prot
>> lines=auto
      <tripcounters>
$(tripcounters_item Total  68    2       0     0    66    0)
$(tripcounters_item C       8    0       0     0     8    0)
$(tripcounters_item Y      60    2       0     0    58    0)
$(tripcounters_item AYT    68    2       0     0    66    0)
      </tripcounters>
      <tripdata>
        <airps>
          <airp>$(get_elem_id etAirp $(get airp_arv))</airp>
        </airps>
        <classes>
          <class>$(get_elem_id etClass C)</class>
          <class>$(get_elem_id etClass Y)</class>
        </classes>
        <crs>
$(crs_item ""        "Common data" " "  " "  " ")
$(crs_item "SENDER0" "SENDER0"     " "  " "  "x")
$(crs_item "SENDER1" "SENDER1"     "x"  " "  " ")
$(crs_item "SENDER2" "SENDER2"     "x"  "x"  "x")
$(crs_item "SENDER3" "SENDER3"     "x"  " "  "x")
        </crs>
        <crsdata>
$(crsdata_item "SENDER1" АЯТ Б  3 -1)
$(crsdata_item "SENDER1" АЯТ Э 15 -1)
$(crsdata_item "SENDER2" АЯТ Б  0 -1)
$(crsdata_item "SENDER2" АЯТ П  4 -1)
$(crsdata_item "SENDER2" АЯТ Э  2 -1)
$(crsdata_item ""        АЯТ Б  0  0)
$(crsdata_item ""        АЯТ П  4  0)
$(crsdata_item ""        АЯТ Э  2  0)
        </crsdata>
      </tripdata>

$(cache PIKE RU CRS_SET $(cache_iface_ver CRS_SET) ""
  insert airline:$(get_elem_id etAirline $(get airline))
         flt_no:$(get flt_no)
         airp_dep:$(get_elem_id etAirp $(get airp_dep))
         crs:SENDER1
         priority:9
         pr_numeric_pnl:0)

### посылаем пустую ADL, чтобы перерасчитались счетчики

<<
MOWKK1H
.SENDER2 $(dd)1205
ADL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART1
RBD F/F C/CJIDA Y/YSTEQGNBXWUORVHLKPZ
-$(get airp_arv)004F
-$(get airp_arv)000C
-$(get airp_arv)000J
-$(get airp_arv)001Y
-$(get airp_arv)001S
-$(get airp_arv)000T
ENDADL

$(GET_TRIP_INFO_REQUEST capture=on $(get point_dep) EN)

###                       cfg resa tranzit block avail prot
>> lines=auto
      <tripcounters>
$(tripcounters_item Total  68   18       0     0    50    0)
$(tripcounters_item C       8    3       0     0     5    0)
$(tripcounters_item Y      60   15       0     0    45    0)
$(tripcounters_item AYT    68   18       0     0    50    0)
      </tripcounters>
      <tripdata>
        <airps>
          <airp>$(get_elem_id etAirp $(get airp_arv))</airp>
        </airps>
        <classes>
          <class>$(get_elem_id etClass C)</class>
          <class>$(get_elem_id etClass Y)</class>
        </classes>
        <crs>
$(crs_item ""        "Common data" " "  " "  " ")
$(crs_item "SENDER0" "SENDER0"     " "  " "  " ")
$(crs_item "SENDER1" "SENDER1"     "x"  " "  "x")
$(crs_item "SENDER2" "SENDER2"     "x"  "x"  " ")
$(crs_item "SENDER3" "SENDER3"     "x"  " "  " ")
        </crs>
        <crsdata>
$(crsdata_item "SENDER1" АЯТ Б  3 -1)
$(crsdata_item "SENDER1" АЯТ Э 15 -1)
$(crsdata_item "SENDER2" АЯТ Б  0 -1)
$(crsdata_item "SENDER2" АЯТ П  4 -1)
$(crsdata_item "SENDER2" АЯТ Э  2 -1)
$(crsdata_item ""        АЯТ Б  3  0)
$(crsdata_item ""        АЯТ Э 15  0)
        </crsdata>
      </tripdata>

%%

### test 3
### три плеча с дублирующимися пунктами вылета и прилета
### потом отменяем дублирование пунктов
### далее экспериментируем с приоритетами отправителей в пункте транзита и вводим частично данные вручную
#########################################################################################

$(init_term)
$(set_user_time_type LocalAirp PIKE)

$(set tomor $(date_format %d.%m.%Y +1))

$(NEW_SPP_FLIGHT_REQUEST
{ $(new_spp_point ЮТ 111 100 44444 ""                   DME "$(get tomor) 07:00")
  $(new_spp_point ЮТ 111 100 44444 "$(get tomor) 08:00" TJM "$(get tomor) 10:00")
  $(new_spp_point ЮТ 111 100 44444 "$(get tomor) 11:00" SGC "$(get tomor) 13:00")
  $(new_spp_point_last             "$(get tomor) 15:00" TJM ) })

$(dump_table points fields="move_id, point_id, point_num, first_point, pr_tranzit, pr_del" order="point_num")

$(set point_dep1 $(get_point_dep_for_flight ЮТ 111 "" $(yymmdd +1) DME))
$(set point_dep2 $(get_point_dep_for_flight ЮТ 111 "" $(yymmdd +1) TJM))
$(set point_dep3 $(get_point_dep_for_flight ЮТ 111 "" $(yymmdd +1) SGC))
$(set point_arv3 $(get_next_trip_point_id $(get point_dep3)))

$(PREP_CHECKIN $(get point_dep1))
$(PREP_CHECKIN $(get point_dep2))
$(PREP_CHECKIN $(get point_dep3))

$(GET_TRIP_INFO_REQUEST capture=on $(get point_dep1))

###                       cfg resa tranzit block avail prot
>> lines=auto
      <tripcounters>
$(tripcounters_item Всего 188    0       0     0   188    0)
$(tripcounters_item П      92    0       0     0    92    0)
$(tripcounters_item Б       4    0       0     0     4    0)
$(tripcounters_item Э      92    0       0     0    92    0)
$(tripcounters_item РЩН   188    0       0     0   188    0)
$(tripcounters_item СУР   188    0       0     0   188    0)
$(tripcounters_item РЩН   188    0       0     0   188    0)
      </tripcounters>
      <tripdata>
        <airps>
          <airp>$(get_elem_id etAirp TJM)</airp>
          <airp>$(get_elem_id etAirp SGC)</airp>
          <airp>$(get_elem_id etAirp TJM)</airp>
        </airps>
        <classes>
          <class>$(get_elem_id etClass F)</class>
          <class>$(get_elem_id etClass C)</class>
          <class>$(get_elem_id etClass Y)</class>
        </classes>
        <crs>
$(crs_item "" "Общие данные" "" "" "")
        </crs>
        <crsdata/>
      </tripdata>

!! capture=on
$(READ_TRIPS $(date_format %d.%m.%Y +1) EN)

>> mode=regex
.*
          <places_out>
            <airp>TJM</airp>
            <airp>SGC</airp>
            <airp>TJM</airp>
          </places_out>
          <classes>
            <class cfg='92'>F</class>
            <class cfg='4'>C</class>
            <class cfg='92'>Y</class>
          </classes>
          <stages>
.*
          <places_out>
            <airp>SGC</airp>
            <airp>TJM</airp>
          </places_out>
          <classes>
            <class cfg='92'>F</class>
            <class cfg='4'>C</class>
            <class cfg='92'>Y</class>
          </classes>
          <stages>
.*
          <places_out>
            <airp>TJM</airp>
          </places_out>
          <classes>
            <class cfg='92'>F</class>
            <class cfg='4'>C</class>
            <class cfg='92'>Y</class>
          </classes>
          <stages>
.*

<<
MOWKK1H
.SENDER3 $(dd)1200
PNL
UT111/$(ddmon +1 en) DME PART1
RBD F/FA C/CJID Y/YSTEQGNBXWUORVHLKPZ
AVAIL
 DME  TJM  SGC  AER
A000  002  003
J010  020  030
Z100  200  300
ENDPNL

<<
MOWKK1H
.SENDER3 $(dd)1200
PNL
UT111/$(ddmon +1 en) TJM PART1
RBD F/FA C/CJID Y/YSTEQGNBXWUORVHLKPZ
TRANSIT
 SGC  AER
A004  008
J001  002
Z016  032
ENDPNL

<<
MOWKK1H
.SENDER1 $(dd)1200
PNL
UT111/$(ddmon +1 en) DME PART1
RBD F/FA C/CJID Y/YSTEQGNBXWUORVHLKPZ
AVAIL
 DME  TJM  SGC  AER
Y005  004  003
-TJM000Y-PAD000
1KIM
-TJM001S-PAD000
-TJM002T-PAD001
-TJM003E-PAD000
-AER003Y-PAD000
-AER002S-PAD002
-AER001T-PAD000
-AER000E-PAD000
-SGC003Y-PAD000
-SGC002S-PAD001
-SGC004T-PAD000
-SGC001E-PAD000
-SGC000D-PAD000
ENDPNL

<<
MOWKK1H
.SENDER1 $(dd)1200
PNL
UT111/$(ddmon +1 en) TJM PART1
RBD F/FA C/CJID Y/YSTEQGNBXWUORVHLKPZ
TRANSIT
 SGC  AER
Y004  003
-AER003Y-PAD000
1KIM
-AER002S-PAD002
-AER001T-PAD000
-AER000E-PAD000
-SGC003Y-PAD000
-SGC002S-PAD001
-SGC004T-PAD000
-SGC001E-PAD000
-SGC000D-PAD000
ENDPNL

<<
MOWKK1H
.SENDER2 $(dd)1200
PNL
UT111/$(ddmon +1 en) DME PART1
RBD F/FA C/CJID Y/YSTEQGNBXWUORVHLKPZ
-TJM066Z
1BIM
-TJM015J
-TJM011A
ENDPNL

<<
MOWKK1H
.SENDER2 $(dd)1200
PNL
UT111/$(ddmon +1 en) TJM PART1
RBD F/FA C/CJID Y/YSTEQGNBXWUORVHLKPZ
-AER033Z
1BIM
-AER000J
-AER007A
ENDPNL

$(GET_TRIP_INFO_REQUEST capture=on $(get point_dep1))

###                       cfg resa tranzit block avail prot
>> lines=auto
      <tripcounters>
$(tripcounters_item Всего 188  108       0     0    91    0)
$(tripcounters_item П      92   11       0     0    81    0)
$(tripcounters_item Б       4   15       0     0     0    0)
$(tripcounters_item Э      92   82       0     0    10    0)
$(tripcounters_item РЩН   188   98       0     0    80    0)
$(tripcounters_item СУР   188   10       0     0    80    0)
$(tripcounters_item РЩН   188    0       0     0    80    0)
      </tripcounters>
      <tripdata>
        <airps>
          <airp>$(get_elem_id etAirp TJM)</airp>
          <airp>$(get_elem_id etAirp SGC)</airp>
          <airp>$(get_elem_id etAirp TJM)</airp>
        </airps>
        <classes>
          <class>$(get_elem_id etClass F)</class>
          <class>$(get_elem_id etClass C)</class>
          <class>$(get_elem_id etClass Y)</class>
        </classes>
        <crs>
$(crs_item ""        "Общие данные" " "  " "  " ")
$(crs_item "SENDER1" "SENDER1"      "x"  "x"  " ")
$(crs_item "SENDER2" "SENDER2"      "x"  "x"  " ")
$(crs_item "SENDER3" "SENDER3"      "x"  " "  " ")
        </crs>
        <crsdata>
$(crsdata_item "SENDER1" РЩН Э  6 -1)
$(crsdata_item "SENDER1" СУР Б  0 -1)
$(crsdata_item "SENDER1" СУР Э 10 -1)
$(crsdata_item "SENDER1" СОЧ Э  6 -1)
$(crsdata_item "SENDER2" РЩН Б 15 -1)
$(crsdata_item "SENDER2" РЩН П 11 -1)
$(crsdata_item "SENDER2" РЩН Э 66 -1)
$(crsdata_item ""        РЩН Б 15  0)
$(crsdata_item ""        РЩН П 11  0)
$(crsdata_item ""        РЩН Э 72  0)
$(crsdata_item ""        СОЧ Б  0  0)
$(crsdata_item ""        СОЧ П  0  0)
$(crsdata_item ""        СОЧ Э  6  0)
$(crsdata_item ""        СУР Б  0  0)
$(crsdata_item ""        СУР П  0  0)
$(crsdata_item ""        СУР Э 10  0)
        </crsdata>
      </tripdata>

$(GET_TRIP_INFO_REQUEST capture=on $(get point_dep2))

###                       cfg resa tranzit block avail prot
>> lines=auto
      <tripcounters>
$(tripcounters_item Всего 188   10      25     0   153    0)
$(tripcounters_item П      92    0       4     0    88    0)
$(tripcounters_item Б       4    0       1     0     3    0)
$(tripcounters_item Э      92   10      20     0    62    0)
$(tripcounters_item СУР   188   10      25     0   153    0)
$(tripcounters_item РЩН   188    0       0     0   153    0)
      </tripcounters>
      <tripdata>
        <airps>
          <airp>$(get_elem_id etAirp SGC)</airp>
          <airp>$(get_elem_id etAirp TJM)</airp>
        </airps>
        <classes>
          <class>$(get_elem_id etClass F)</class>
          <class>$(get_elem_id etClass C)</class>
          <class>$(get_elem_id etClass Y)</class>
        </classes>
        <crs>
$(crs_item ""        "Общие данные" " "  " "  " ")
$(crs_item "SENDER1" "SENDER1"      "x"  "x"  " ")
$(crs_item "SENDER2" "SENDER2"      "x"  "x"  " ")
$(crs_item "SENDER3" "SENDER3"      "x"  " "  " ")
        </crs>
        <crsdata>
$(crsdata_item "SENDER1" СУР Б  0 -1)
$(crsdata_item "SENDER1" СУР Э 10  4)
$(crsdata_item "SENDER1" СОЧ Э  6  3)
$(crsdata_item "SENDER2" СОЧ Б  0 -1)
$(crsdata_item "SENDER2" СОЧ П  7 -1)
$(crsdata_item "SENDER2" СОЧ Э 33 -1)
$(crsdata_item "SENDER3" СУР Б -1  1)
$(crsdata_item "SENDER3" СУР П -1  4)
$(crsdata_item "SENDER3" СУР Э -1 16)
$(crsdata_item "SENDER3" СОЧ Б -1  2)
$(crsdata_item "SENDER3" СОЧ П -1  8)
$(crsdata_item "SENDER3" СОЧ Э -1 32)
$(crsdata_item ""        СОЧ Б  0  2)
$(crsdata_item ""        СОЧ П  7  8)
$(crsdata_item ""        СОЧ Э 39 35)
$(crsdata_item ""        СУР Б  0  1)
$(crsdata_item ""        СУР П  0  4)
$(crsdata_item ""        СУР Э 10 20)
        </crsdata>
      </tripdata>

!! capture=on
$(READ_TRIPS $(date_format %d.%m.%Y +1) EN)

>> mode=regex
.*
          <places_out>
            <airp>TJM</airp>
            <airp>SGC</airp>
            <airp>TJM</airp>
          </places_out>
          <classes>
            <class cfg='92'>F</class>
            <class cfg='4'>C</class>
            <class cfg='92'>Y</class>
          </classes>
          <resa>114</resa>
          <stages>
.*
          <places_out>
            <airp>SGC</airp>
            <airp>TJM</airp>
          </places_out>
          <classes>
            <class cfg='92'>F</class>
            <class cfg='4'>C</class>
            <class cfg='92'>Y</class>
          </classes>
          <reg>25</reg>
          <resa>56</resa>
          <stages>
.*
          <places_out>
            <airp>TJM</airp>
          </places_out>
          <classes>
            <class cfg='92'>F</class>
            <class cfg='4'>C</class>
            <class cfg='92'>Y</class>
          </classes>
          <stages>
.*


$(CHANGE_SPP_FLIGHT_REQUEST $(get point_dep2)
{ $(change_spp_point  $(get point_dep1) ЮТ 111 100 44444 ""                   "" "" DME "$(get tomor) 07:00")
  $(change_spp_point  $(get point_dep2) ЮТ 111 100 44444 "$(get tomor) 08:00" "" "" TJM "$(get tomor) 10:00" pr_del=1)
  $(new_spp_point                       ЮТ 111 100 44444 "$(get tomor) 08:00"       AER "$(get tomor) 10:00")
  $(change_spp_point  $(get point_dep3) ЮТ 111 100 44444 "$(get tomor) 11:00" "" "" SGC "$(get tomor) 13:00")
  $(change_spp_point_last $(get point_arv3)              "$(get tomor) 15:00" "" "" TJM ) })


$(GET_TRIP_INFO_REQUEST capture=on $(get point_dep1))

###                       cfg resa tranzit block avail prot
>> lines=auto
      <tripcounters>
$(tripcounters_item Всего 188  114       0     0    85    0)
$(tripcounters_item П      92   11       0     0    81    0)
$(tripcounters_item Б       4   15       0     0     0    0)
$(tripcounters_item Э      92   88       0     0     4    0)
$(tripcounters_item СОЧ   188    6       0     0    74    0)
$(tripcounters_item СУР   188   10       0     0    74    0)
$(tripcounters_item РЩН   188   98       0     0    74    0)
      </tripcounters>
      <tripdata>
        <airps>
          <airp>$(get_elem_id etAirp AER)</airp>
          <airp>$(get_elem_id etAirp SGC)</airp>
          <airp>$(get_elem_id etAirp TJM)</airp>
        </airps>
        <classes>
          <class>$(get_elem_id etClass F)</class>
          <class>$(get_elem_id etClass C)</class>
          <class>$(get_elem_id etClass Y)</class>
        </classes>
        <crs>
$(crs_item ""        "Общие данные" " "  " "  " ")
$(crs_item "SENDER1" "SENDER1"      "x"  "x"  " ")
$(crs_item "SENDER2" "SENDER2"      "x"  "x"  " ")
$(crs_item "SENDER3" "SENDER3"      "x"  " "  " ")
        </crs>
        <crsdata>
$(crsdata_item "SENDER1" СОЧ Э  6 -1)
$(crsdata_item "SENDER1" СУР Б  0 -1)
$(crsdata_item "SENDER1" СУР Э 10 -1)
$(crsdata_item "SENDER1" РЩН Э  6 -1)
$(crsdata_item "SENDER2" РЩН Б 15 -1)
$(crsdata_item "SENDER2" РЩН П 11 -1)
$(crsdata_item "SENDER2" РЩН Э 66 -1)
$(crsdata_item ""        РЩН Б 15  0)
$(crsdata_item ""        РЩН П 11  0)
$(crsdata_item ""        РЩН Э 72  0)
$(crsdata_item ""        СОЧ Б  0  0)
$(crsdata_item ""        СОЧ П  0  0)
$(crsdata_item ""        СОЧ Э  6  0)
$(crsdata_item ""        СУР Б  0  0)
$(crsdata_item ""        СУР П  0  0)
$(crsdata_item ""        СУР Э 10  0)
        </crsdata>
      </tripdata>

$(set point_dep4 $(get_point_dep_for_flight ЮТ 111 "" $(yymmdd +1) AER))

$(CHANGE_SPP_FLIGHT_REQUEST $(get point_dep2)
{ $(change_spp_point  $(get point_dep1) ЮТ 111 100 44444 ""                   "" "" DME "$(get tomor) 07:00")
  $(change_spp_point  $(get point_dep2) ЮТ 111 100 44444 "$(get tomor) 08:00" "" "" TJM "$(get tomor) 10:00" pr_del=0)
  $(change_spp_point  $(get point_dep4) ЮТ 111 100 44444 "$(get tomor) 08:00" "" "" AER "$(get tomor) 10:00" pr_del=-1)
  $(change_spp_point  $(get point_dep3) ЮТ 111 100 44444 "$(get tomor) 11:00" "" "" SGC "$(get tomor) 13:00")
  $(change_spp_point_last $(get point_arv3)              "$(get tomor) 15:00" "" "" TJM pr_del=-1)
  $(new_spp_point_last                                   "$(get tomor) 15:00"       AER ) })

$(GET_TRIP_INFO_REQUEST capture=on $(get point_dep2))

###                       cfg resa tranzit block avail prot
>> lines=auto
      <tripcounters>
$(tripcounters_item Всего 188   56      70     0    74    0)
$(tripcounters_item П      92    7      12     0    73    0)
$(tripcounters_item Б       4    0       3     0     1    0)
$(tripcounters_item Э      92   49      55     0     0    0)
$(tripcounters_item СУР   188   10      25     0    62    0)
$(tripcounters_item СОЧ   188   46      45     0    62    0)
      </tripcounters>
      <tripdata>
        <airps>
          <airp>$(get_elem_id etAirp SGC)</airp>
          <airp>$(get_elem_id etAirp AER)</airp>
        </airps>
        <classes>
          <class>$(get_elem_id etClass F)</class>
          <class>$(get_elem_id etClass C)</class>
          <class>$(get_elem_id etClass Y)</class>
        </classes>
        <crs>
$(crs_item ""        "Общие данные" " "  " "  " ")
$(crs_item "SENDER1" "SENDER1"      "x"  "x"  " ")
$(crs_item "SENDER2" "SENDER2"      "x"  "x"  " ")
$(crs_item "SENDER3" "SENDER3"      "x"  " "  " ")
        </crs>
        <crsdata>
$(crsdata_item "SENDER1" СУР Б  0 -1)
$(crsdata_item "SENDER1" СУР Э 10  4)
$(crsdata_item "SENDER1" СОЧ Э  6  3)
$(crsdata_item "SENDER2" СОЧ Б  0 -1)
$(crsdata_item "SENDER2" СОЧ П  7 -1)
$(crsdata_item "SENDER2" СОЧ Э 33 -1)
$(crsdata_item "SENDER3" СУР Б -1  1)
$(crsdata_item "SENDER3" СУР П -1  4)
$(crsdata_item "SENDER3" СУР Э -1 16)
$(crsdata_item "SENDER3" СОЧ Б -1  2)
$(crsdata_item "SENDER3" СОЧ П -1  8)
$(crsdata_item "SENDER3" СОЧ Э -1 32)
$(crsdata_item ""        СОЧ Б  0  2)
$(crsdata_item ""        СОЧ П  7  8)
$(crsdata_item ""        СОЧ Э 39 35)
$(crsdata_item ""        СУР Б  0  1)
$(crsdata_item ""        СУР П  0  4)
$(crsdata_item ""        СУР Э 10 20)
        </crsdata>
      </tripdata>

!! capture=on
$(READ_TRIPS $(date_format %d.%m.%Y +1) EN)

>> mode=regex
.*
          <places_out>
            <airp>TJM</airp>
            <airp>SGC</airp>
            <airp>AER</airp>
          </places_out>
          <classes>
            <class cfg='92'>F</class>
            <class cfg='4'>C</class>
            <class cfg='92'>Y</class>
          </classes>
          <resa>114</resa>
          <stages>
.*
          <places_out>
            <airp>SGC</airp>
            <airp>AER</airp>
          </places_out>
          <classes>
            <class cfg='92'>F</class>
            <class cfg='4'>C</class>
            <class cfg='92'>Y</class>
          </classes>
          <reg>70</reg>
          <resa>56</resa>
          <stages>
.*
          <places_out>
            <airp>AER</airp>
          </places_out>
          <classes>
            <class cfg='92'>F</class>
            <class cfg='4'>C</class>
            <class cfg='92'>Y</class>
          </classes>
          <stages>
.*


$(cache PIKE RU CRS_SET $(cache_iface_ver CRS_SET) ""
  insert airline:$(get_elem_id etAirline ЮТ)
         flt_no:111
         airp_dep:$(get_elem_id etAirp РЩН)
         crs:SENDER1
         priority:9
         pr_numeric_pnl:0)

### посылаем пустую ADL, чтобы перерасчитались счетчики

<<
MOWKK1H
.SENDER2 $(dd)1201
ADL
UT111/$(ddmon +1 en) TJM PART1
RBD F/FA C/CJID Y/YSTEQGNBXWUORVHLKPZ
-AER033Z
-AER000J
-AER007A
ENDADL

$(GET_TRIP_INFO_REQUEST capture=on lang=EN $(get point_dep2))

###                       cfg resa tranzit block avail prot
>> lines=auto
      <tripcounters>
$(tripcounters_item Total 188   16       7     0   165    0)
$(tripcounters_item F      92    0       0     0    92    0)
$(tripcounters_item C       4    0       0     0     4    0)
$(tripcounters_item Y      92   16       7     0    69    0)
$(tripcounters_item SGC   188   10       4     0   165    0)
$(tripcounters_item AER   188    6       3     0   165    0)
      </tripcounters>
      <tripdata>
        <airps>
          <airp>$(get_elem_id etAirp SGC)</airp>
          <airp>$(get_elem_id etAirp AER)</airp>
        </airps>
        <classes>
          <class>$(get_elem_id etClass F)</class>
          <class>$(get_elem_id etClass C)</class>
          <class>$(get_elem_id etClass Y)</class>
        </classes>
        <crs>
$(crs_item ""        "Common data"  " "  " "  " ")
$(crs_item "SENDER1" "SENDER1"      "x"  "x"  "x")
$(crs_item "SENDER2" "SENDER2"      "x"  "x"  " ")
$(crs_item "SENDER3" "SENDER3"      "x"  " "  " ")
        </crs>
        <crsdata>
$(crsdata_item "SENDER1" СУР Б  0 -1)
$(crsdata_item "SENDER1" СУР Э 10  4)
$(crsdata_item "SENDER1" СОЧ Э  6  3)
$(crsdata_item "SENDER2" СОЧ Б  0 -1)
$(crsdata_item "SENDER2" СОЧ П  7 -1)
$(crsdata_item "SENDER2" СОЧ Э 33 -1)
$(crsdata_item "SENDER3" СУР Б -1  1)
$(crsdata_item "SENDER3" СУР П -1  4)
$(crsdata_item "SENDER3" СУР Э -1 16)
$(crsdata_item "SENDER3" СОЧ Б -1  2)
$(crsdata_item "SENDER3" СОЧ П -1  8)
$(crsdata_item "SENDER3" СОЧ Э -1 32)
$(crsdata_item ""        СОЧ Э  6  3)
$(crsdata_item ""        СУР Б  0  0)
$(crsdata_item ""        СУР Э 10  4)
        </crsdata>
      </tripdata>

!! capture=on
$(READ_TRIPS $(date_format %d.%m.%Y +1) EN)

>> mode=regex
.*
          <places_out>
            <airp>SGC</airp>
            <airp>AER</airp>
          </places_out>
          <classes>
            <class cfg='92'>F</class>
            <class cfg='4'>C</class>
            <class cfg='92'>Y</class>
          </classes>
          <reg>21</reg>
          <resa>16</resa>
          <stages>
.*

$(CRS_DATA_UPDATE_REQUEST capture=on lang=EN $(get point_dep2)
{$(crsdata_update СУР F  11  15)
 $(crsdata_update СОЧ F  22  13)
 $(crsdata_update СОЧ Y   0   0)})

###                       cfg resa tranzit block avail prot
>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...>
    <data>
      <tripcounters>
$(tripcounters_item Total 188   43      32     0   113    0)
$(tripcounters_item F      92   33      28     0    31    0)
$(tripcounters_item C       4    0       0     0     4    0)
$(tripcounters_item Y      92   10       4     0    78    0)
$(tripcounters_item SGC   188   21      19     0   113    0)
$(tripcounters_item AER   188   22      13     0   113    0)
      </tripcounters>
    </data>
  </answer>
</term>

!! capture=on
$(READ_TRIPS $(date_format %d.%m.%Y +1) EN)

>> mode=regex
.*
          <places_out>
            <airp>SGC</airp>
            <airp>AER</airp>
          </places_out>
          <classes>
            <class cfg='92'>F</class>
            <class cfg='4'>C</class>
            <class cfg='92'>Y</class>
          </classes>
          <reg>32</reg>
          <resa>43</resa>
          <stages>
.*





