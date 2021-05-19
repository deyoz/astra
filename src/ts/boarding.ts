include(ts/macro.ts)
include(ts/adm_macro.ts)
include(ts/pax/checkin_macro.ts)
include(ts/pax/boarding_macro.ts)

# meta: suite boarding

$(defmacro PREPARE_SPP_FLIGHT_SETTINGS
  airline
  airp_dep
{
$(deny_ets_interactive $(airline))
})

$(defmacro DO_CHECKIN
  airline
  flt_no
  airp_dep
  airp_arv
{

$(PREPARE_SEASON_SCD $(airline) $(airp_dep) $(airp_arv) $(flt_no))
$(make_spp)
$(PREPARE_SPP_FLIGHT_SETTINGS $(airline) $(airp_dep))
$(PREPARE_HALLS_FOR_BOARDING $(airp_dep))

$(INB_PNL_UT $(airp_dep) $(airp_arv) $(flt_no) $(ddmon +0))

$(set point_dep $(last_point_id_spp))

$(CHANGE_TRIP_SETS $(get point_dep) use_jmp=1 jmp_cfg=5)

$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id_01 $(get_pax_id $(get point_dep) STIPIDI ANGELINA))
$(set pax_id_02 $(get_pax_id $(get point_dep) AKOPOVA OLIVIIA))
$(set pax_id_03 $(get_pax_id $(get point_dep) VASILIADI "KSENIYA VALEREVNA"))
$(set pax_id_04 $(get_pax_id $(get point_dep) CHEKMAREV "RONALD"))
$(set pax_id_05 $(get_pax_id $(get point_dep) VERGUNOV "VASILII LEONIDOVICH"))

$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(airp_dep) $(airp_arv)
{
<passengers>
  <pax>
$(NEW_CHECKIN_2986145143703 $(get pax_id_03))
  </pax>
  <pax>
$(NEW_CHECKIN_2986145143704 $(get pax_id_04))
  </pax>
</passengers>
})


$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(airp_dep) $(airp_arv)
{
<passengers>
  <pax>
$(NEW_CHECKIN_2986145134262 $(get pax_id_01))
  </pax>
  <pax>
$(NEW_CHECKIN_2986145134263 $(get pax_id_02))
  </pax>
</passengers>
})

$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(airp_dep) $(airp_arv)
{
<passengers>
  <pax>
$(NEW_CHECKIN_2986145212943 $(get pax_id_05))
    <rems>
      <rem>
        <rem_code>JMP</rem_code>
        <rem_text>JMP</rem_text>
      </rem>
    </rems>
    <fqt_rems/>
  </pax>
</passengers>
})

$(set grp_id_01 $(get_single_grp_id $(get pax_id_01)))
$(set grp_id_02 $(get_single_grp_id $(get pax_id_02)))
$(set grp_id_03 $(get_single_grp_id $(get pax_id_03)))
$(set grp_id_04 $(get_single_grp_id $(get pax_id_04)))
$(set grp_id_05 $(get_single_grp_id $(get pax_id_05)))

$(set apis_01
{          <apis>
            <document>
              <type>P</type>
              <issue_country>RUS</issue_country>
              <no>0305555064</no>
              <nationality>RUS</nationality>
              <birth_date>23.07.1982 00:00:00</birth_date>
              <gender>F</gender>
              <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
              <surname>STIPIDI</surname>
              <first_name>ANGELINA</first_name>
            </document>
            <doco/>
            <addresses/>
          </apis>}
)

$(set apis_02
{          <apis>
            <document>
              <type>F</type>
              <issue_country>RUS</issue_country>
              <no>VIAG519994</no>
              <nationality>RUS</nationality>
              <birth_date>22.08.$(date_format %Y -1y) 00:00:00</birth_date>
              <gender>F</gender>
              <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
              <surname>AKOPOVA</surname>
              <first_name>OLIVIIA</first_name>
            </document>
            <doco/>
            <addresses/>
          </apis>}
)

$(set apis_03
{          <apis>
            <document>
              <type>P</type>
              <issue_country>RUS</issue_country>
              <no>0307611933</no>
              <nationality>RUS</nationality>
              <birth_date>13.09.1984 00:00:00</birth_date>
              <gender>F</gender>
              <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
              <surname>VASILIADI</surname>
              <first_name>KSENIYA VALEREVNA</first_name>
            </document>
            <doco/>
            <addresses/>
          </apis>}
)

$(set apis_04
{          <apis>
            <document>
              <type>F</type>
              <issue_country>RUS</issue_country>
              <no>V��815247</no>
              <nationality>RUS</nationality>
              <birth_date>29.01.$(date_format %Y -1y) 00:00:00</birth_date>
              <gender>M</gender>
              <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
              <surname>CHEKMAREV</surname>
              <first_name>RONALD KONSTANTINOVICH</first_name>
            </document>
            <doco/>
            <addresses/>
          </apis>}
)

$(set apis_05
{          <apis>
            <document>
              <type>P</type>
              <issue_country>RUS</issue_country>
              <no>0305984920</no>
              <nationality>RUS</nationality>
              <birth_date>04.11.1960 00:00:00</birth_date>
              <gender>M</gender>
              <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
              <surname>VERGUNOV</surname>
              <first_name>VASILII LEONIDOVICH</first_name>
            </document>
            <doco/>
            <addresses/>
          </apis>}
)

})

$(defmacro FORM_DATA_SECTION
  total
  total_brd
  total_not_brd
  total_exam
  total_not_exam

{    <form_data>
      <variables update=''>
        <total>$(total)</total>
        <total_brd>$(total_brd)</total_brd>
        <total_not_brd>$(total_not_brd)</total_not_brd>
        <exam_totals>...$(total)
...$(total_exam)
...$(total_not_exam)</exam_totals>
        <brd_totals>...$(total)
...$(total_brd)
...$(total_not_brd)</brd_totals>
        <test_server>1</test_server>
      </variables>
    </form_data>}
)

$(defmacro DEFAULTS_SECTION
{      <defaults>
        <pr_brd>0</pr_brd>
        <pr_exam>0</pr_exam>
        <name/>
        <pers_type>...
        <class>Y</class>
        <seats>1</seats>
        <ticket_no/>
        <coupon_no>0</coupon_no>
        <bag_norm/>
        <document/>
        <excess>0</excess>
        <value_bag_count>0</value_bag_count>
        <pr_payment>0</pr_payment>
        <bag_amount>0</bag_amount>
        <bag_weight>0</bag_weight>
        <rk_amount>0</rk_amount>
        <rk_weight>0</rk_weight>
        <tags/>
        <remarks/>
        <client_name/>
        <inbound_flt/>
        <inbound_delay_alarm>0</inbound_delay_alarm>
        <document_alarm>0</document_alarm>
      </defaults>}
)

$(defmacro PAX_LIST_RESPONSE
  point_dep
  total
  total_brd
  total_not_brd
  total_exam
  total_not_exam
  passengers
  counters
  command
{

>> lines=auto
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...
$(FORM_DATA_SECTION $(total) $(total_brd) $(total_not_brd) $(total_exam) $(total_not_exam))
    <data>
$(passengers)
$(DEFAULTS_SECTION)
      <col_excess_type>0</col_excess_type>
$(counters)
    </data>\
$(if $(eq $(command) "") "" {
$(command)})
  </answer>
</term>

})

$(defmacro BOARDING_RESPONSE_ONE_PAX
  point_dep
  total
  total_brd
  total_not_brd
  total_exam
  total_not_exam
  pax_id
  pr_brd
  reg_no
  surname
  name
  pers_type
  airp_arv
  seat_no
  seats
  ticket_no
  coupon_no
  document
  pr_payment
  apis
  counters
  command
  updated=true
{

$(set grp_id $(get_single_grp_id $(pax_id)))
$(set pax_tid $(get_single_pax_tid $(pax_id)))

### �ᥣ�� total_brd=total_exam � total_not_brd=total_not_exam � �� ��࠭�����, ���, �த�, � ���� �� ��ଠ�쭮

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...
$(FORM_DATA_SECTION $(total) $(total_brd) $(total_not_brd) $(total_exam) $(total_not_exam))
    <data>\
$(if $(eq $(updated) true) {
      <updated>
        <pax_id>$(pax_id)</pax_id>
      </updated>
      <trip_sets>
        <pr_etl_only>1</pr_etl_only>
        <pr_etstatus>0</pr_etstatus>
      </trip_sets>})
      <passengers>
        <pax>
          <pax_id>$(pax_id)</pax_id>
          <grp_id>$(get grp_id)</grp_id>\
$(if $(eq $(pr_brd) 0) "" {
          <pr_brd>$(pr_brd)</pr_brd>})
          <reg_no>$(reg_no)</reg_no>
          <surname>$(surname)</surname>
          <name>$(name)</name>\
$(if $(eq $(pers_type) "") {} {
          <pers_type>$(pers_type)</pers_type>})
          <airp_arv>$(airp_arv)</airp_arv>\
$(if $(eq $(seat_no) "") {
          <seat_no/>} {
          <seat_no>$(seat_no)</seat_no>})\
$(if $(eq $(seats) 1) "" {
          <seats>$(seats)</seats>})
          <ticket_no>$(ticket_no)</ticket_no>
          <coupon_no>$(coupon_no)</coupon_no>
          <document>$(document)</document>
          <tid>$(get pax_tid)</tid>
          <pr_payment>$(pr_payment)</pr_payment>
          <check_info>
            <doc/>
            <doco/>
            <doca_b/>
            <doca_r/>
            <doca_d/>
            <tkn/>
          </check_info>
$(apis)
        </pax>
      </passengers>
$(DEFAULTS_SECTION)
      <col_excess_type>0</col_excess_type>
$(counters)
    </data>
$(command)
  </answer>
</term>

})


### test 1
### ��������㥬 3�� � 2��. ���� �� - JMP
### ������ 5-� 祫���� (�� ॣ. ������, �� ��. ���ᠦ��, �� �⠭���⭮�� 2D, �� ���७���� 2D
### ��ᠦ����� 4-� 祫���� (�� ॣ. ������, �� ��. ���ᠦ��, �� �⠭���⭮�� 2D, �� ���७���� 2D)
### ���-� ����� ��ᠦ�����ﬨ ������塞 ᯨ᮪ �� ��ᠤ��
#########################################################################################

$(init_term)

$(set airline UT)
$(set flt_no 280)
$(set airp_dep DME)
$(set airp_arv AER)

$(DO_CHECKIN $(get airline) $(get flt_no) $(get airp_dep) $(get airp_arv))

$(set point_dep $(last_point_id_spp))

$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_01) 777 "" 1 capture=on)

$(set counters
{      <counters>
        <reg>5</reg>
        <brd>1</brd>
        <reg_str>C0 Y5 </reg_str>
        <brd_str>C0 Y1 </brd_str>
      </counters>}
)

$(BOARDING_RESPONSE_ONE_PAX
  $(get point_dep)
  "3/0/2(C0/Y5)" "1/0/0(C0/Y1)" "2/0/2(C0/Y4)" "1/0/0(C0/Y1)" "2/0/2(C0/Y4)"
  $(get pax_id_01) 1 3 STIPIDI ANGELINA "" $(get_lat_code aer $(get airp_arv))
  " 11�" 1 2986145134262 2 0305555064 1
  $(get apis_01) $(get counters) $(MESSAGE_TAG MSG.PASSENGER.BOARDING2)

)

$(BOARDING_REQUEST_BY_REG_NO $(get point_dep) 4 777 1 capture=on)

$(set counters
{      <counters>
        <reg>5</reg>
        <brd>2</brd>
        <reg_str>C0 Y5 </reg_str>
        <brd_str>C0 Y2 </brd_str>
      </counters>}
)

$(BOARDING_RESPONSE_ONE_PAX
  $(get point_dep)
  "3/0/2(C0/Y5)" "1/0/1(C0/Y2)" "2/0/1(C0/Y3)" "1/0/1(C0/Y2)" "2/0/1(C0/Y3)"
  $(get pax_id_02) 1 4 AKOPOVA OLIVIIA INF $(get_lat_code aer $(get airp_arv))
  "" 0 2986145134263 2 VIAG519994 1
  $(get apis_02) $(get counters) $(MESSAGE_TAG MSG.PASSENGER.BOARDING2)

)

### ᠦ��� �� ���媮�� #########################################################################################

$(set scan_data_03_1d $(lpad $(get pax_id_03) 10 0))
$(set scan_data_04_1d $(lpad $(get pax_id_04) 10 0))
$(set scan_data_05_1d $(lpad $(get pax_id_05) 10 0))

$(set julian_date $(substr $(get_bcbp $(get grp_id_03) $(get pax_id_03)) 44, 3))

$(set scan_data_03_2d_ru_wide {M1VASILIADI/KSENIYA VAEF52MM0 �������� 0280 $(get julian_date)�006D0001 131>2180OO    B                00ASTRA$(get scan_data_03_1d)UT *})
$(set scan_data_03_2d_en      {M1VASILIADI/KSENIYA VAEF52MM0 DMEAERUT 0280 $(get julian_date)Y006D0001 128>2180OO    B                00$(get scan_data_03_1d)})

$(set scan_data_04_2d_ru      {M1CHEKMAREV/RONALD    EF52MM0 �������� 0280 $(get julian_date)�00000002 128>2184OO    B                00$(get scan_data_04_1d)})
$(set scan_data_04_2d_en      {M1CHEKMAREV/RONALD    EF52MM0 DMEAERUT 0280 $(get julian_date)Y00000002 128>2184OO    B                00$(get scan_data_04_1d)})
$(set scan_data_04_2d_en_wide {M1CHEKMAREV/RONALD    EF52MM0 DMEAERUT 0280 $(get julian_date)Y00000002 131>2184OO    B                00ASTRA$(get scan_data_04_1d)UT *})

$(set scan_data_05_2d_ru      {M1VERGUNOV/VASILII LEOEF58457 �������� 0280 $(get julian_date)�0JMP0005 128>2180OO    B                00$(get scan_data_05_1d)})

??
$(get_bcbp $(get grp_id_03) $(get pax_id_03))
>>
$(get scan_data_03_2d_en)

??
$(get_bcbp $(get grp_id_04) $(get pax_id_04))
>>
$(get scan_data_04_2d_en)

??
$(get_bcbp $(get grp_id_04) $(get pax_id_04) RU)
>>
$(get scan_data_04_2d_ru)

??
$(get_bcbp $(get grp_id_05) $(get pax_id_05) RU)
>>
$(get scan_data_05_2d_ru)

$(BOARDING_REQUEST_BY_SCAN_DATA $(get point_dep) $(get scan_data_03_2d_en) 777 1 capture=on)

$(set counters
{      <counters>
        <reg>5</reg>
        <brd>3</brd>
        <reg_str>C0 Y5 </reg_str>
        <brd_str>C0 Y3 </brd_str>
      </counters>}
)

$(BOARDING_RESPONSE_ONE_PAX
  $(get point_dep)
  "3/0/2(C0/Y5)" "2/0/1(C0/Y3)" "1/0/1(C0/Y2)" "2/0/1(C0/Y3)" "1/0/1(C0/Y2)"
  $(get pax_id_03) 1 1 VASILIADI "KSENIYA VALEREVNA" "" $(get_lat_code aer $(get airp_arv))
  "  6�" 1 2986145143703 2 0307611933 1
  $(get apis_03) $(get counters) $(MESSAGE_TAG MSG.PASSENGER.BOARDING2)

)

$(BOARDING_REQUEST_BY_SCAN_DATA $(get point_dep) $(get scan_data_04_2d_en_wide) 777 1 capture=on)

$(set counters
{      <counters>
        <reg>5</reg>
        <brd>4</brd>
        <reg_str>C0 Y5 </reg_str>
        <brd_str>C0 Y4 </brd_str>
      </counters>}
)

$(BOARDING_RESPONSE_ONE_PAX
  $(get point_dep)
  "3/0/2(C0/Y5)" "2/0/2(C0/Y4)" "1/0/0(C0/Y1)" "2/0/2(C0/Y4)" "1/0/0(C0/Y1)"
  $(get pax_id_04) 1 2 CHEKMAREV "RONALD" INF $(get_lat_code aer $(get airp_arv))
  "" 0 2986145143704 2 V��815247 1
  $(get apis_04) $(get counters) $(MESSAGE_TAG MSG.PASSENGER.BOARDING2)

)

$(BOARDING_REQUEST_BY_SCAN_DATA $(get point_dep) $(get scan_data_05_2d_ru) 777 1 capture=on)

$(set counters
{      <counters>
        <reg>5</reg>
        <brd>5</brd>
        <reg_str>C0 Y5 </reg_str>
        <brd_str>C0 Y5 </brd_str>
      </counters>}
)

$(BOARDING_RESPONSE_ONE_PAX
  $(get point_dep)
  "3/0/2(C0/Y5)" "3/0/2(C0/Y5)" "0/0/0(C0/Y0)" "3/0/2(C0/Y5)" "0/0/0(C0/Y0)"
  $(get pax_id_05) 1 5 VERGUNOV "VASILII LEONIDOVICH" "" $(get_lat_code aer $(get airp_arv))
  "JMP" 1 2986145212943 1 0305984920 1
  $(get apis_05) $(get counters) $(MESSAGE_TAG MSG.PASSENGER.BOARDING2)

)


### ����� ��ᠦ����� #########################################################################################

$(BOARDING_REQUEST_BY_SCAN_DATA $(get point_dep) $(get scan_data_03_2d_ru_wide) 777 0 capture=on)

$(set counters
{      <counters>
        <reg>5</reg>
        <brd>4</brd>
        <reg_str>C0 Y5 </reg_str>
        <brd_str>C0 Y4 </brd_str>
      </counters>}
)

$(BOARDING_RESPONSE_ONE_PAX
  $(get point_dep)
  "3/0/2(C0/Y5)" "2/0/2(C0/Y4)" "1/0/0(C0/Y1)" "2/0/2(C0/Y4)" "1/0/0(C0/Y1)"
  $(get pax_id_03) 0 1 VASILIADI "KSENIYA VALEREVNA" "" $(get_lat_code aer $(get airp_arv))
  "  6�" 1 2986145143703 2 0307611933 1
  $(get apis_03) $(get counters) $(MESSAGE_TAG MSG.PASSENGER.DEBARKED2)

)


$(BOARDING_REQUEST_BY_SCAN_DATA $(get point_dep) $(get scan_data_04_2d_ru) 777 0 capture=on)

$(set counters
{      <counters>
        <reg>5</reg>
        <brd>3</brd>
        <reg_str>C0 Y5 </reg_str>
        <brd_str>C0 Y3 </brd_str>
      </counters>}
)

$(BOARDING_RESPONSE_ONE_PAX
  $(get point_dep)
  "3/0/2(C0/Y5)" "2/0/1(C0/Y3)" "1/0/1(C0/Y2)" "2/0/1(C0/Y3)" "1/0/1(C0/Y2)"
  $(get pax_id_04) 0 2 CHEKMAREV "RONALD" INF $(get_lat_code aer $(get airp_arv))
  "" 0 2986145143704 2 V��815247 1
  $(get apis_04) $(get counters) $(MESSAGE_TAG MSG.PASSENGER.DEBARKED2)

)

$(CRS_LIST_REQUEST $(get point_dep) lang=EN)

$(BRD_PAX_LIST_REQUEST $(get point_dep) lang=EN capture=on)

$(PAX_LIST_RESPONSE
  $(get point_dep)
  "3/0/2(C0/Y5)" "2/0/1(C0/Y3)" "1/0/1(C0/Y2)" "2/0/1(C0/Y3)" "1/0/1(C0/Y2)"

{      <passengers>
        <pax>
          <pax_id>$(get pax_id_03)</pax_id>
          <grp_id>$(get grp_id_03)</grp_id>
          <reg_no>1</reg_no>
          <surname>VASILIADI</surname>
          <name>KSENIYA VALEREVNA</name>
          <airp_arv>AER</airp_arv>
          <seat_no>...</seat_no>
          <ticket_no>2986145143703</ticket_no>
          <coupon_no>2</coupon_no>
          <document>0307611933</document>
          <tid>$(get_single_pax_tid $(get pax_id_03))</tid>
          <pr_payment>1</pr_payment>
        </pax>
        <pax>
          <pax_id>$(get pax_id_04)</pax_id>
          <grp_id>$(get grp_id_04)</grp_id>
          <reg_no>2</reg_no>
          <surname>CHEKMAREV</surname>
          <name>RONALD</name>
          <pers_type>INF</pers_type>
          <airp_arv>AER</airp_arv>
          <seat_no/>
          <seats>0</seats>
          <ticket_no>2986145143704</ticket_no>
          <coupon_no>2</coupon_no>
          <document>V��815247</document>
          <tid>$(get_single_pax_tid $(get pax_id_04))</tid>
          <pr_payment>1</pr_payment>
        </pax>
        <pax>
          <pax_id>$(get pax_id_01)</pax_id>
          <grp_id>$(get grp_id_01)</grp_id>
          <pr_brd>1</pr_brd>
          <reg_no>3</reg_no>
          <surname>STIPIDI</surname>
          <name>ANGELINA</name>
          <airp_arv>AER</airp_arv>
          <seat_no>...</seat_no>
          <ticket_no>2986145134262</ticket_no>
          <coupon_no>2</coupon_no>
          <document>0305555064</document>
          <tid>$(get_single_pax_tid $(get pax_id_01))</tid>
          <pr_payment>1</pr_payment>
        </pax>
        <pax>
          <pax_id>$(get pax_id_02)</pax_id>
          <grp_id>$(get grp_id_02)</grp_id>
          <pr_brd>1</pr_brd>
          <reg_no>4</reg_no>
          <surname>AKOPOVA</surname>
          <name>OLIVIIA</name>
          <pers_type>INF</pers_type>
          <airp_arv>AER</airp_arv>
          <seat_no/>
          <seats>0</seats>
          <ticket_no>2986145134263</ticket_no>
          <coupon_no>2</coupon_no>
          <document>VIAG519994</document>
          <tid>$(get_single_pax_tid $(get pax_id_02))</tid>
          <pr_payment>1</pr_payment>
        </pax>
        <pax>
          <pax_id>$(get pax_id_05)</pax_id>
          <grp_id>$(get grp_id_05)</grp_id>
          <pr_brd>1</pr_brd>
          <reg_no>5</reg_no>
          <surname>VERGUNOV</surname>
          <name>VASILII LEONIDOVICH</name>
          <airp_arv>AER</airp_arv>
          <seat_no>JMP</seat_no>
          <ticket_no>2986145212943</ticket_no>
          <coupon_no>1</coupon_no>
          <document>0305984920</document>
          <tid>$(get_single_pax_tid $(get pax_id_05))</tid>
          <pr_payment>1</pr_payment>
        </pax>
      </passengers>}

  $(get counters)
)

$(BOARDING_REQUEST_BY_REG_NO $(get point_dep) 3 777 0 capture=on)

$(set counters
{      <counters>
        <reg>5</reg>
        <brd>2</brd>
        <reg_str>C0 Y5 </reg_str>
        <brd_str>C0 Y2 </brd_str>
      </counters>}
)

$(BOARDING_RESPONSE_ONE_PAX
  $(get point_dep)
  "3/0/2(C0/Y5)" "1/0/1(C0/Y2)" "2/0/1(C0/Y3)" "1/0/1(C0/Y2)" "2/0/1(C0/Y3)"
  $(get pax_id_01) 0 3 STIPIDI ANGELINA "" $(get_lat_code aer $(get airp_arv))
  " 11�" 1 2986145134262 2 0305555064 1
  $(get apis_01) $(get counters) $(MESSAGE_TAG MSG.PASSENGER.DEBARKED2)

)

$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_02) 777
  $(get_single_pax_tid $(get pax_id_02)) 0 capture=on)

$(set counters
{      <counters>
        <reg>5</reg>
        <brd>1</brd>
        <reg_str>C0 Y5 </reg_str>
        <brd_str>C0 Y1 </brd_str>
      </counters>}
)

$(BOARDING_RESPONSE_ONE_PAX
  $(get point_dep)
  "3/0/2(C0/Y5)" "1/0/0(C0/Y1)" "2/0/2(C0/Y4)" "1/0/0(C0/Y1)" "2/0/2(C0/Y4)"
  $(get pax_id_02) 0 4 AKOPOVA OLIVIIA INF $(get_lat_code aer $(get airp_arv))
  "" 0 2986145134263 2 VIAG519994 1
  $(get apis_02) $(get counters) $(MESSAGE_TAG MSG.PASSENGER.DEBARKED2)

)

%%

### test 2
### ��������㥬 3�� � 2��. ���� �� - JMP
### ������ 5-� 祫����
### ��ᠦ����� ���
#########################################################################################

$(init_term)

$(set airline UT)
$(set flt_no 280)
$(set airp_dep DME)
$(set airp_arv AER)

$(DO_CHECKIN $(get airline) $(get flt_no) $(get airp_dep) $(get airp_arv))

$(BOARDING_REQUEST_BY_REG_NO $(get point_dep) 5 777 1)
$(BOARDING_REQUEST_BY_REG_NO $(get point_dep) 4 777 1)
$(BOARDING_REQUEST_BY_REG_NO $(get point_dep) 3 777 1)
$(BOARDING_REQUEST_BY_REG_NO $(get point_dep) 2 777 1)
$(BOARDING_REQUEST_BY_REG_NO $(get point_dep) 1 777 1)

$(DEPLANE_ALL_REQUEST $(get point_dep) capture=on)

$(PAX_LIST_RESPONSE
  $(get point_dep)
  "3/0/2(C0/Y5)" "0/0/0(C0/Y0)" "3/0/2(C0/Y5)" "0/0/0(C0/Y0)" "3/0/2(C0/Y5)"

{      <passengers>
        <pax>
          <pax_id>$(get pax_id_03)</pax_id>
          <grp_id>$(get grp_id_03)</grp_id>
          <reg_no>1</reg_no>
          <surname>VASILIADI</surname>
          <name>KSENIYA VALEREVNA</name>
          <airp_arv>AER</airp_arv>
          <seat_no>...</seat_no>
          <ticket_no>2986145143703</ticket_no>
          <coupon_no>2</coupon_no>
          <document>0307611933</document>
          <tid>$(get_single_pax_tid $(get pax_id_03))</tid>
          <pr_payment>1</pr_payment>
        </pax>
        <pax>
          <pax_id>$(get pax_id_04)</pax_id>
          <grp_id>$(get grp_id_04)</grp_id>
          <reg_no>2</reg_no>
          <surname>CHEKMAREV</surname>
          <name>RONALD</name>
          <pers_type>INF</pers_type>
          <airp_arv>AER</airp_arv>
          <seat_no/>
          <seats>0</seats>
          <ticket_no>2986145143704</ticket_no>
          <coupon_no>2</coupon_no>
          <document>V��815247</document>
          <tid>$(get_single_pax_tid $(get pax_id_04))</tid>
          <pr_payment>1</pr_payment>
        </pax>
        <pax>
          <pax_id>$(get pax_id_01)</pax_id>
          <grp_id>$(get grp_id_01)</grp_id>
          <reg_no>3</reg_no>
          <surname>STIPIDI</surname>
          <name>ANGELINA</name>
          <airp_arv>AER</airp_arv>
          <seat_no>...</seat_no>
          <ticket_no>2986145134262</ticket_no>
          <coupon_no>2</coupon_no>
          <document>0305555064</document>
          <tid>$(get_single_pax_tid $(get pax_id_01))</tid>
          <pr_payment>1</pr_payment>
        </pax>
        <pax>
          <pax_id>$(get pax_id_02)</pax_id>
          <grp_id>$(get grp_id_02)</grp_id>
          <reg_no>4</reg_no>
          <surname>AKOPOVA</surname>
          <name>OLIVIIA</name>
          <pers_type>INF</pers_type>
          <airp_arv>AER</airp_arv>
          <seat_no/>
          <seats>0</seats>
          <ticket_no>2986145134263</ticket_no>
          <coupon_no>2</coupon_no>
          <document>VIAG519994</document>
          <tid>$(get_single_pax_tid $(get pax_id_02))</tid>
          <pr_payment>1</pr_payment>
        </pax>
        <pax>
          <pax_id>$(get pax_id_05)</pax_id>
          <grp_id>$(get grp_id_05)</grp_id>
          <reg_no>5</reg_no>
          <surname>VERGUNOV</surname>
          <name>VASILII LEONIDOVICH</name>
          <airp_arv>AER</airp_arv>
          <seat_no>JMP</seat_no>
          <ticket_no>2986145212943</ticket_no>
          <coupon_no>1</coupon_no>
          <document>0305984920</document>
          <tid>$(get_single_pax_tid $(get pax_id_05))</tid>
          <pr_payment>1</pr_payment>
        </pax>
      </passengers>}

{      <counters>
        <reg>5</reg>
        <brd>0</brd>
        <reg_str>C0 Y5 </reg_str>
        <brd_str>C0 Y0 </brd_str>
      </counters>}

  command=$(MESSAGE_TAG EVT.PASSENGER.ALL_NOT_BOARDED)

)

%%

### test 3
### �訡�� �� ����⪠� ��ᠤ��:
###  1. ��ᮢ������� pax.tid (�� ��ᠤ�� ��� �� �����㧪� APIS)
###  2. ���ᠦ�� �� ��ॣ����஢�� (�� ॣ. ������, ��., ����-����)
###  3. ��-� �� ⠪ � ����-�����
###  4. ���ᠦ�� � ��㣮�� ३� (�� ��., ����-����)
###  5. �� 㪠��� ��� ��ᠤ��, ������ ��� ��ᠤ��
###  6. ���ᠦ�� 㦥 ��ᠦ��, ���ᠦ�� 㦥 ��ᠦ��
###  7. ���ᠦ�� �� ��襫 ��ᬮ��
###  8. ��ᠤ�� JMP ����饭� (tsDeniedBoardingJMP)
### �� ���饥:
### ���ᠦ�� �� ���⢥ত�� � ���� ��������
### ���ᠦ�� �� ����⨫ �����/��㣨
### � ���ᠦ�� ������� APIS
### � ���ᠦ�� ����� APIS ������� ������
#########################################################################################

$(init_term)

$(set airline UT)
$(set flt_no 280)
$(set airp_dep DME)
$(set airp_arv AER)

$(DO_CHECKIN $(get airline) $(get flt_no) $(get airp_dep) $(get airp_arv))

$(set pax_id_not_checked $(get_pax_id $(get point_dep) BABAKHANOVA KIRA))

$(set pax_tid_01 $(get_single_pax_tid $(get pax_id_01)))

$(BOARDING_REQUEST_BY_PAX_ID capture=on $(get point_dep) $(get pax_id_01) 777 $(+ $(get pax_tid_01) 5) 1)

$(USER_ERROR_RESPONSE MSG.PASSENGER.CHANGED_FROM_OTHER_DESK.REFRESH_DATA)

### ����� APIS � ���㠫�� tid
$(LOAD_APIS_REQUEST capture=on $(get point_dep) $(get pax_id_01) $(get pax_tid_01))

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...
    <check_info>
      <doc/>
      <doco/>
      <doca_b/>
      <doca_r/>
      <doca_d/>
      <tkn/>
    </check_info>
    <apis>
      <document>
        <type>P</type>
        <issue_country>RUS</issue_country>
        <no>0305555064</no>
        <nationality>RUS</nationality>
        <birth_date>23.07.1982 00:00:00</birth_date>
        <gender>F</gender>
        <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
        <surname>STIPIDI</surname>
        <first_name>ANGELINA</first_name>
      </document>
      <doco/>
      <addresses/>
    </apis>
  </answer>
</term>

### ����� APIS � �����㠫�� tid

$(LOAD_APIS_REQUEST capture=on $(get point_dep) $(get pax_id_01) $(+ $(get pax_tid_01) 5))

$(USER_ERROR_RESPONSE MSG.PASSENGER.NO_PARAM.CHANGED_FROM_OTHER_DESK.REFRESH_DATA)


$(BOARDING_REQUEST_BY_REG_NO capture=on $(get point_dep) 560 777 1)

$(USER_ERROR_RESPONSE MSG.PASSENGER.NOT_CHECKIN)


$(BOARDING_REQUEST_BY_PAX_ID capture=on $(get point_dep) $(get pax_id_not_checked) 777 "" 1)

$(USER_ERROR_RESPONSE MSG.PASSENGER.NOT_CHECKIN)


### !!!�஢���� SearchPaxByScanData

$(BOARDING_REQUEST_BY_REG_NO capture=on $(+  $(get point_dep) 1000) 2 777 1)

$(USER_ERROR_RESPONSE MSG.FLIGHT.NOT_FOUND.REFRESH_DATA)


### ��室���� �� ��㣮� ३� - ��।��� ��㣮� point_dep

$(BOARDING_REQUEST_BY_PAX_ID capture=on $(+  $(get point_dep) 1000) $(get pax_id_01) 777 "" 1)

$(USER_ERROR_RESPONSE MSG.PASSENGER.FROM_FLIGHT 100)


$(BOARDING_REQUEST_BY_SCAN_DATA capture=on $(+  $(get point_dep) 1000) $(get_bcbp $(get grp_id_03) $(get pax_id_03)) 777 1)

$(USER_ERROR_RESPONSE MSG.PASSENGER.FROM_FLIGHT 100)


$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_05) "" "" 1 capture=on)  #��� �� 㪠���

>> lines=auto
$(USER_ERROR_MESSAGE_TAG MSG.NOT_SET_BOARDING_HALL)

$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_05) 778 "" 1 capture=on) #��� �� ᮮ⢥����� �/� �뫥�
>> lines=auto
$(USER_ERROR_MESSAGE_TAG MSG.NOT_SET_BOARDING_HALL)


$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_05) 777 "" 1)
$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_05) 777 "" 1 capture=on)

$(set counters
{      <counters>
        <reg>5</reg>
        <brd>1</brd>
        <reg_str>C0 Y5 </reg_str>
        <brd_str>C0 Y1 </brd_str>
      </counters>}
)

$(BOARDING_RESPONSE_ONE_PAX updated=false
  $(get point_dep)
  "3/0/2(C0/Y5)" "1/0/0(C0/Y1)" "2/0/2(C0/Y4)" "1/0/0(C0/Y1)" "2/0/2(C0/Y4)"
  $(get pax_id_05) 1 5 VERGUNOV "VASILII LEONIDOVICH" "" $(get_lat_code aer $(get airp_arv))
  "JMP" 1 2986145212943 1 0305984920 1
  $(get apis_05) $(get counters) $(USER_ERROR_MESSAGE_TAG MSG.PASSENGER.BOARDED_ALREADY2 120)
)

$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_05) 777 "" 0)
$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_05) 777 "" 0 capture=on)

$(set counters
{      <counters>
        <reg>5</reg>
        <brd>0</brd>
        <reg_str>C0 Y5 </reg_str>
        <brd_str>C0 Y0 </brd_str>
      </counters>}
)

$(BOARDING_RESPONSE_ONE_PAX updated=false
  $(get point_dep)
  "3/0/2(C0/Y5)" "0/0/0(C0/Y0)" "3/0/2(C0/Y5)" "0/0/0(C0/Y0)" "3/0/2(C0/Y5)"
  $(get pax_id_05) 0 5 VERGUNOV "VASILII LEONIDOVICH" "" $(get_lat_code aer $(get airp_arv))
  "JMP" 1 2986145212943 1 0305984920 1
  $(get apis_05) $(get counters) $(USER_ERROR_MESSAGE_TAG MSG.PASSENGER.NOT_BOARDING2 120)
)


### ����頥� ��ᠤ�� JMP

$(cache PIKE RU MISC_SET $(cache_iface_ver MISC_SET) ""
  insert type_code:80
         airline:$(get_elem_id etAirline $(get airline))
         pr_misc:1)

$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_05) 777 "" 1 capture=on)

$(set counters
{      <counters>
        <reg>5</reg>
        <brd>0</brd>
        <reg_str>C0 Y5 </reg_str>
        <brd_str>C0 Y0 </brd_str>
      </counters>}
)

$(BOARDING_RESPONSE_ONE_PAX updated=false
  $(get point_dep)
  "3/0/2(C0/Y5)" "0/0/0(C0/Y0)" "3/0/2(C0/Y5)" "0/0/0(C0/Y0)" "3/0/2(C0/Y5)"
  $(get pax_id_05) 0 5 VERGUNOV "VASILII LEONIDOVICH" "" $(get_lat_code aer $(get airp_arv))
  "JMP" 1 2986145212943 1 0305984920 1
  $(get apis_05) $(get counters) $(USER_ERROR_MESSAGE_TAG MSG.PASSENGER.JMP_BOARDED_DENIAL)
)


#########################################################################################
### �஢��塞, �� �� ��� ����� �� ��ᠤ��, �᫨ ����ன�� "��ᬮ�஢� ����஫� ��। ��ᠤ���" ����祭�

$(CHANGE_TRIP_SETS $(get point_dep) pr_exam=1)

$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_01) 776 "" 1 capture=on)

>> lines=auto
$(USER_ERROR_MESSAGE_TAG MSG.PASSENGER.NOT_EXAM2)

$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_03) 777 "" 1 capture=on)

>> lines=auto
$(USER_ERROR_MESSAGE_TAG MSG.PASSENGER.NOT_EXAM2)


#########################################################################################
### �஢��塞, �� �� ������ �� ����� ��ᠤ��, �᫨ ����ன�� "��ᬮ�� �� ��ᠤ�� �� ३�" ��� �⮣� ����

$(combine_exam_with_brd $(get point_dep) 777)

$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_01) 776 "" 1 capture=on)

>> lines=auto
$(USER_ERROR_MESSAGE_TAG MSG.PASSENGER.NOT_EXAM2)

$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_03) 777 "" 1 capture=on)

>> lines=auto
$(MESSAGE_TAG MSG.PASSENGER.BOARDING2)


#########################################################################################
### �஢��塞, �� ⥯��� � �� ��㣮�� ���� ��ᠤ��, �᫨ ����ன�� "��ᬮ�� �� ��ᠤ�� �� ३�" ��� ��� �����

$(combine_exam_with_brd $(get point_dep) "")

$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_01) 776 "" 1 capture=on)

>> lines=auto
$(MESSAGE_TAG MSG.PASSENGER.BOARDING2)


#########################################################################################
### 㤠�塞 "��ᬮ�� �� ��ᠤ�� �� ३�" ��� ��� �����
### �஢��塞, �� ����ன�� "��ᬮ�஢� ����஫� ��। ��ᠤ���" �� ��ᠤ�� ����� �� �����

$(sql {DELETE FROM trip_hall WHERE point_id=$(get point_dep)})

$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_01) 776 "" 0 capture=on)

>> lines=auto
$(MESSAGE_TAG MSG.PASSENGER.DEBARKED2)

$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_03) 777 "" 0 capture=on)

>> lines=auto
$(MESSAGE_TAG MSG.PASSENGER.DEBARKED2)

%%

### test 4
### ���⢥ত���� �� ��ᠤ��:
#########################################################################################
