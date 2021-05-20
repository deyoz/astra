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

### test 1 - ���� ����, ⥫��ࠬ�� PNL/ADL �� ��諨, �������� ����� ������
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
$(tripcounters_item �ᥣ�)
      </tripcounters>
      <tripdata>
        <airps>
          <airp>$(get_elem_id etAirp $(get airp_arv))</airp>
        </airps>
        <classes/>
        <crs>
$(crs_item "" "��騥 �����" "" "" "")
        </crs>
        <crsdata/>
      </tripdata>

$(CHANGE_SPP_FLIGHT_ONE_LEG $(get point_dep) "" "" $(get airline) $(get flt_no) $(get craft) $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv) $(get bort))

$(GET_TRIP_INFO_REQUEST capture=on $(get point_dep))

###                       cfg resa tranzit block avail prot
>> lines=auto
      <tripcounters>
$(tripcounters_item �ᥣ�  68    0       0     0    68    0)
$(tripcounters_item �       8    0       0     0     8    0)
$(tripcounters_item �      60    0       0     0    60    0)
$(tripcounters_item ���    68    0       0     0    68    0)
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
$(crs_item "" "��騥 �����" "" "" "")
        </crs>
        <crsdata/>
      </tripdata>

!! capture=on
<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='prepreg' ver='1' opr='PIKE' screen='PREPREG.EXE' mode='STAND' lang='RU' term_id='2479792165'>                        \
    <CrsDataApplyUpdates>
      <point_id>$(get point_dep)</point_id>
      <question>1</question>
      <crsdata>
        <item>
          <target>$(get_elem_id etAirp $(get airp_arv))</target>
          <class>�</class>
          <resa>1</resa>
          <tranzit>0</tranzit>
        </item>
        <item>
          <target>$(get_elem_id etAirp $(get airp_arv))</target>
          <class>�</class>
          <resa>8</resa>
          <tranzit>2</tranzit>
        </item>
        <item>
          <target>$(get_elem_id etAirp $(get airp_arv))</target>
          <class>�</class>
          <resa>22</resa>
          <tranzit>4</tranzit>
        </item>
      </crsdata>
      <tripcounters/>
    </CrsDataApplyUpdates>
  </query>
</term>

###                       cfg resa tranzit block avail prot
>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...>
    <data>
      <tripcounters>
$(tripcounters_item �ᥣ�  68   30       0     0    38    0)
$(tripcounters_item �       8    8       0     0     0    0)
$(tripcounters_item �      60   22       0     0    38    0)
$(tripcounters_item ���    68   30       0     0    38    0)
      </tripcounters>
    </data>
  </answer>
</term>

!! capture=on
$(READ_TRIPS $(date_format %d.%m.%Y +1))

>> lines=auto
          <places_out>
            <airp>AYT</airp>
          </places_out>
          <classes>
            <class cfg='8'>C</class>
            <class cfg='60'>Y</class>
          </classes>
          <resa>31</resa>

%%

### test 2
### ���� ���� � ��ᮢ�����騬� �㭪⠬� �����祭��
### ��⮬ ���ࠢ�塞 �������
### ����� �ᯥਬ����㥬 � �ਧ����� ����஢�� PNL � � �ਮ��⠬� ��ࠢ�⥫��
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
$(crsdata_item "SENDER1" ��� �  3 -1)
$(crsdata_item "SENDER1" ��� � 15 -1)
$(crsdata_item "SENDER2" ��� �  0 -1)
$(crsdata_item "SENDER2" ��� �  4 -1)
$(crsdata_item "SENDER2" ��� �  2 -1)
$(crsdata_item ""        ��� �  3  0)
$(crsdata_item ""        ��� �  4  0)
$(crsdata_item ""        ��� � 17  0)
        </crsdata>
      </tripdata>

!! capture=on
$(READ_TRIPS $(date_format %d.%m.%Y +1))

>> lines=auto
          <places_out>
            <airp>AER</airp>
          </places_out>
          <classes>
            <class cfg='8'>C</class>
            <class cfg='60'>Y</class>
          </classes>
          <resa>24</resa>

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
1������/����
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
$(crsdata_item "SENDER1" ��� �  3 -1)
$(crsdata_item "SENDER1" ��� � 15 -1)
$(crsdata_item "SENDER2" ��� �  0 -1)
$(crsdata_item "SENDER2" ��� �  4 -1)
$(crsdata_item "SENDER2" ��� �  2 -1)
$(crsdata_item ""        ��� �  3  0)
$(crsdata_item ""        ��� �  4  0)
$(crsdata_item ""        ��� � 17  0)
        </crsdata>
      </tripdata>

$(cache PIKE RU CRS_SET $(cache_iface_ver CRS_SET) ""
  insert airline:$(get_elem_id etAirline $(get airline))
         flt_no:$(get flt_no)
         airp_dep:$(get_elem_id etAirp $(get airp_dep))
         crs:SENDER0
         priority:1
         pr_numeric_pnl:0)

### ���뫠�� ������ ADL, �⮡� ������⠫��� ���稪�

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
$(crsdata_item "SENDER1" ��� �  3 -1)
$(crsdata_item "SENDER1" ��� � 15 -1)
$(crsdata_item "SENDER2" ��� �  0 -1)
$(crsdata_item "SENDER2" ��� �  4 -1)
$(crsdata_item "SENDER2" ��� �  2 -1)
        </crsdata>
      </tripdata>

$(cache PIKE RU CRS_SET $(cache_iface_ver CRS_SET) ""
  insert airline:$(get_elem_id etAirline $(get airline))
         flt_no:$(get flt_no)
         airp_dep:$(get_elem_id etAirp $(get airp_dep))
         crs:SENDER3
         priority:1
         pr_numeric_pnl:0)

### ���뫠�� ������ ADL, �⮡� ������⠫��� ���稪�

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
$(crsdata_item "SENDER1" ��� �  3 -1)
$(crsdata_item "SENDER1" ��� � 15 -1)
$(crsdata_item "SENDER2" ��� �  0 -1)
$(crsdata_item "SENDER2" ��� �  4 -1)
$(crsdata_item "SENDER2" ��� �  2 -1)
$(crsdata_item ""        ��� �  0  0)
$(crsdata_item ""        ��� �  0  0)
        </crsdata>
      </tripdata>

$(cache PIKE RU CRS_SET $(cache_iface_ver CRS_SET) ""
  insert airline:$(get_elem_id etAirline $(get airline))
         flt_no:$(get flt_no)
         airp_dep:$(get_elem_id etAirp $(get airp_dep))
         crs:SENDER2
         priority:1
         pr_numeric_pnl:0)

### ���뫠�� ������ ADL, �⮡� ������⠫��� ���稪�

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
$(crsdata_item "SENDER1" ��� �  3 -1)
$(crsdata_item "SENDER1" ��� � 15 -1)
$(crsdata_item "SENDER2" ��� �  0 -1)
$(crsdata_item "SENDER2" ��� �  4 -1)
$(crsdata_item "SENDER2" ��� �  2 -1)
$(crsdata_item ""        ��� �  0  0)
$(crsdata_item ""        ��� �  4  0)
$(crsdata_item ""        ��� �  2  0)
        </crsdata>
      </tripdata>

$(cache PIKE RU CRS_SET $(cache_iface_ver CRS_SET) ""
  insert airline:$(get_elem_id etAirline $(get airline))
         flt_no:$(get flt_no)
         airp_dep:$(get_elem_id etAirp $(get airp_dep))
         crs:SENDER1
         priority:9
         pr_numeric_pnl:0)

### ���뫠�� ������ ADL, �⮡� ������⠫��� ���稪�

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
$(crsdata_item "SENDER1" ��� �  3 -1)
$(crsdata_item "SENDER1" ��� � 15 -1)
$(crsdata_item "SENDER2" ��� �  0 -1)
$(crsdata_item "SENDER2" ��� �  4 -1)
$(crsdata_item "SENDER2" ��� �  2 -1)
$(crsdata_item ""        ��� �  3  0)
$(crsdata_item ""        ��� � 15  0)
        </crsdata>
      </tripdata>


