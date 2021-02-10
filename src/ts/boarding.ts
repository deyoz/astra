include(ts/macro.ts)
include(ts/adm_macro.ts)

# meta: suite boarding

$(defmacro PREPARE_SPP_FLIGHT_SETTINGS
  airline
  airp_dep
{

$(set airp_other $(if $(eq $(get_elem_id etAirp $(airp_dep)) "ДМД") "VKO" "ДМД"))

$(deny_ets_interactive $(airline))
$(sql {INSERT INTO halls2(id, airp, terminal, name, name_lat, rpt_grp, pr_vip)
       VALUES(776, '$(get_elem_id etAirp $(airp_dep))', NULL, '$(airp_dep)', NULL, NULL, 0)})
$(sql {INSERT INTO halls2(id, airp, terminal, name, name_lat, rpt_grp, pr_vip)
       VALUES(777, '$(get_elem_id etAirp $(airp_dep))', NULL, '$(airp_dep)', NULL, NULL, 0)})
$(sql {INSERT INTO halls2(id, airp, terminal, name, name_lat, rpt_grp, pr_vip)
       VALUES(778, '$(get_elem_id etAirp $(get airp_other))', NULL, '$(get airp_other)', NULL, NULL, 0)})

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
    <pax_id>$(get pax_id_03)</pax_id>
    <surname>VASILIADI</surname>
    <name>KSENIYA VALEREVNA</name>
    <pers_type>ВЗ</pers_type>
    <seat_no>6Г</seat_no>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>2986145143703</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>RUS</issue_country>
      <no>0307611933</no>
      <nationality>RUS</nationality>
      <birth_date>13.09.1984 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>VASILIADI</surname>
      <first_name>KSENIYA VALEREVNA</first_name>
    </document>
    <subclass>L</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id_04)</pax_id>
    <surname>CHEKMAREV</surname>
    <name>RONALD</name>
    <pers_type>РМ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>0</seats>
    <ticket_no>2986145143704</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>F</type>
      <issue_country>RUS</issue_country>
      <no>VАГ815247</no>
      <nationality>RUS</nationality>
      <birth_date>29.01.$(date_format %Y -1y) 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>CHEKMAREV</surname>
      <first_name>RONALD KONSTANTINOVICH</first_name>
    </document>
    <subclass>L</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
</passengers>
})


$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(airp_dep) $(airp_arv)
{
<passengers>
  <pax>
    <pax_id>$(get pax_id_01)</pax_id>
    <surname>STIPIDI</surname>
    <name>ANGELINA</name>
    <pers_type>ВЗ</pers_type>
    <seat_no>11Б</seat_no>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>2986145134262</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>RU</issue_country>
      <no>0305555064</no>
      <nationality>RU</nationality>
      <birth_date>23.07.1982 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>STIPIDI</surname>
      <first_name>ANGELINA</first_name>
    </document>
    <subclass>V</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id_02)</pax_id>
    <surname>AKOPOVA</surname>
    <name>OLIVIIA</name>
    <pers_type>РМ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>0</seats>
    <ticket_no>2986145134263</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>F</type>
      <issue_country>RU</issue_country>
      <no>VIAG519994</no>
      <nationality>RU</nationality>
      <birth_date>22.08.$(date_format %Y -1y) 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>AKOPOVA</surname>
      <first_name>OLIVIIA</first_name>
    </document>
    <subclass>V</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
</passengers>
})

$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(airp_dep) $(airp_arv)
{
<passengers>
  <pax>
    <pax_id>$(get pax_id_05)</pax_id>
    <surname>VERGUNOV</surname>
    <name>VASILII LEONIDOVICH</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>2986145212943</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>RU</issue_country>
      <no>0305984920</no>
      <nationality>RU</nationality>
      <birth_date>04.11.1960 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>VERGUNOV</surname>
      <first_name>VASILII LEONIDOVICH</first_name>
    </document>
    <subclass>W</subclass>
    <bag_pool_num/>
    <transfer/>
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

$(set grp_id_01 $(get_single_grp_id $(get point_dep) STIPIDI ANGELINA))
$(set grp_id_02 $(get_single_grp_id $(get point_dep) AKOPOVA OLIVIIA))
$(set grp_id_03 $(get_single_grp_id $(get point_dep) VASILIADI "KSENIYA VALEREVNA"))
$(set grp_id_04 $(get_single_grp_id $(get point_dep) CHEKMAREV "RONALD"))
$(set grp_id_05 $(get_single_grp_id $(get point_dep) VERGUNOV "VASILII LEONIDOVICH"))

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
              <no>VАГ815247</no>
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

$(defmacro PAX_LIST_REQUEST
  point_dep
{

!! capture=on err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='brd' ver='1' opr='PIKE' screen='BRDBUS.EXE' mode='STAND' lang='EN' term_id='2479792165'>
    <PaxList>
      <dev_model/>
      <fmt_type/>
      <prnParams>
        <pr_lat>0</pr_lat>
        <encoding>UTF-16LE</encoding>
        <offset>20</offset>
        <top>0</top>
      </prnParams>
      <point_id>$(point_dep)</point_id>
    </PaxList>
  </query>
</term>}

})

$(defmacro DEPLANE_ALL_REQUEST
  point_dep
  capture=off
{

!! capture=$(capture) err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='brd' ver='1' opr='PIKE' screen='BRDBUS.EXE' mode='STAND' lang='EN' term_id='2479792165'>
    <DeplaneAll>
      <point_id>$(point_dep)</point_id>
      <boarding>0</boarding>
    </DeplaneAll>
  </query>
</term>}

})

$(defmacro BOARDING_REQUEST_BY_PAX_ID
  point_dep
  pax_id
  hall
  pax_tid
  boarding
  capture=off
{

!! capture=$(capture) err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='brd' ver='1' opr='PIKE' screen='BRDBUS.EXE' mode='STAND' lang='EN' term_id='2479792165'>
    <PaxByPaxId>
      <col_excess_type>0</col_excess_type>
      <point_id>$(point_dep)</point_id>
      <hall>$(hall)</hall>
      <pax_id>$(pax_id)</pax_id>\
$(if $(eq $(pax_tid) "") "" {
      <tid>$(pax_tid)</tid>})
      <boarding>$(boarding)</boarding>
      <dev_model/>
      <fmt_type/>
      <prnParams>
        <pr_lat>0</pr_lat>
        <encoding>UTF-16LE</encoding>
        <offset>20</offset>
        <top>0</top>
      </prnParams>
    </PaxByPaxId>
  </query>
</term>}

})

$(defmacro BOARDING_REQUEST_BY_REG_NO
  point_dep
  reg_no
  hall
  boarding
  capture=off
{

!! capture=$(capture) err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='brd' ver='1' opr='PIKE' screen='BRDBUS.EXE' mode='STAND' lang='EN' term_id='2479792165'>
    <PaxByRegNo>
      <col_excess_type>0</col_excess_type>
      <point_id>$(point_dep)</point_id>
      <hall>$(hall)</hall>
      <reg_no>$(reg_no)</reg_no>
      <boarding>$(boarding)</boarding>
      <dev_model/>
      <fmt_type/>
      <prnParams>
        <pr_lat>0</pr_lat>
        <encoding>UTF-16LE</encoding>
        <offset>20</offset>
        <top>0</top>
      </prnParams>
    </PaxByRegNo>
  </query>
</term>}

})

$(defmacro BOARDING_REQUEST_BY_SCAN_DATA
  point_dep
  scan_data
  hall
  boarding
  capture=off
{

!! capture=$(capture) err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='brd' ver='1' opr='PIKE' screen='BRDBUS.EXE' mode='STAND' lang='EN' term_id='2479792165'>
    <PaxByScanData>
      <col_excess_type>0</col_excess_type>
      <point_id>$(point_dep)</point_id>
      <hall>$(hall)</hall>
      <scan_data hex='0'>$(scan_data)</scan_data>
      <boarding>$(boarding)</boarding>
      <dev_model/>
      <fmt_type/>
      <prnParams>
        <pr_lat>0</pr_lat>
        <encoding>UTF-16LE</encoding>
        <offset>20</offset>
        <top>0</top>
      </prnParams>
    </PaxByScanData>
  </query>
</term>}

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
        <test_server>0</test_server>
      </variables>
    </form_data>}
)

$(defmacro DEFAULTS_SECTION
{      <defaults>
        <pr_brd>0</pr_brd>
        <pr_exam>0</pr_exam>
        <name/>
        <pers_type/>
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

$(set grp_id $(get_single_grp_id $(point_dep) $(surname) $(name)))
$(set pax_tid $(get_single_pax_tid $(point_dep) $(surname) $(name)))

### всегда total_brd=total_exam и total_not_brd=total_not_exam и это странновато, хотя, вроде, в очетах все нормально

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
          <name>$(name)</name>
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
### Регистрируем 3ВЗ и 2РМ. Один ВЗ - JMP
### Сажаем 5-х человек (по рег. номеру, по ид. пассажира, по стандартному 2D, по расширенному 2D
### Высаживаем 4-х человек (по рег. номеру, по ид. пассажира, по стандартному 2D, по расширенному 2D)
### Где-то между высаживаниями обновляем список на посадке
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
  $(get pax_id_01) 1 3 STIPIDI ANGELINA $(get_lat_code aer $(get airp_arv))
  " 11Б" 1 2986145134262 2 0305555064 1
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
  $(get pax_id_02) 1 4 AKOPOVA OLIVIIA $(get_lat_code aer $(get airp_arv))
  "" 0 2986145134263 2 VIAG519994 1
  $(get apis_02) $(get counters) $(MESSAGE_TAG MSG.PASSENGER.BOARDING2)

)

### сажаем по штрихкоду #########################################################################################

$(set scan_data_03_1d $(lpad $(get pax_id_03) 10 0))
$(set scan_data_04_1d $(lpad $(get pax_id_04) 10 0))
$(set scan_data_05_1d $(lpad $(get pax_id_05) 10 0))

$(set julian_date $(substr $(get_bcbp $(get grp_id_03) $(get pax_id_03)) 44, 3))

$(set scan_data_03_2d_ru_wide {M1VASILIADI/KSENIYA VAEF52MM0 ДМДСОЧЮТ 0280 $(get julian_date)Э006D0001 131>2180OO    B                00ASTRA$(get scan_data_03_1d)UT *})
$(set scan_data_03_2d_en      {M1VASILIADI/KSENIYA VAEF52MM0 DMEAERUT 0280 $(get julian_date)Y006D0001 128>2180OO    B                00$(get scan_data_03_1d)})

$(set scan_data_04_2d_ru      {M1CHEKMAREV/RONALD    EF52MM0 ДМДСОЧЮТ 0280 $(get julian_date)Э00000002 128>2184OO    B                00$(get scan_data_04_1d)})
$(set scan_data_04_2d_en      {M1CHEKMAREV/RONALD    EF52MM0 DMEAERUT 0280 $(get julian_date)Y00000002 128>2184OO    B                00$(get scan_data_04_1d)})
$(set scan_data_04_2d_en_wide {M1CHEKMAREV/RONALD    EF52MM0 DMEAERUT 0280 $(get julian_date)Y00000002 131>2184OO    B                00ASTRA$(get scan_data_04_1d)UT *})

$(set scan_data_05_2d_ru      {M1VERGUNOV/VASILII LEOEF58457 ДМДСОЧЮТ 0280 $(get julian_date)Э0JMP0005 128>2180OO    B                00$(get scan_data_05_1d)})

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
  $(get pax_id_03) 1 1 VASILIADI "KSENIYA VALEREVNA" $(get_lat_code aer $(get airp_arv))
  "  6Г" 1 2986145143703 2 0307611933 1
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
  $(get pax_id_04) 1 2 CHEKMAREV "RONALD" $(get_lat_code aer $(get airp_arv))
  "" 0 2986145143704 2 VАГ815247 1
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
  $(get pax_id_05) 1 5 VERGUNOV "VASILII LEONIDOVICH" $(get_lat_code aer $(get airp_arv))
  "JMP" 1 2986145212943 1 0305984920 1
  $(get apis_05) $(get counters) $(MESSAGE_TAG MSG.PASSENGER.BOARDING2)

)


### далее высаживаем #########################################################################################

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
  $(get pax_id_03) 0 1 VASILIADI "KSENIYA VALEREVNA" $(get_lat_code aer $(get airp_arv))
  "  6Г" 1 2986145143703 2 0307611933 1
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
  $(get pax_id_04) 0 2 CHEKMAREV "RONALD" $(get_lat_code aer $(get airp_arv))
  "" 0 2986145143704 2 VАГ815247 1
  $(get apis_04) $(get counters) $(MESSAGE_TAG MSG.PASSENGER.DEBARKED2)

)

$(PAX_LIST_REQUEST $(get point_dep))

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
          <tid>$(get_single_pax_tid $(get point_dep) VASILIADI "KSENIYA VALEREVNA")</tid>
          <pr_payment>1</pr_payment>
        </pax>
        <pax>
          <pax_id>$(get pax_id_04)</pax_id>
          <grp_id>$(get grp_id_04)</grp_id>
          <reg_no>2</reg_no>
          <surname>CHEKMAREV</surname>
          <name>RONALD</name>
          <airp_arv>AER</airp_arv>
          <seat_no/>
          <seats>0</seats>
          <ticket_no>2986145143704</ticket_no>
          <coupon_no>2</coupon_no>
          <document>VАГ815247</document>
          <tid>$(get_single_pax_tid $(get point_dep) CHEKMAREV RONALD)</tid>
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
          <tid>$(get_single_pax_tid $(get point_dep) STIPIDI ANGELINA)</tid>
          <pr_payment>1</pr_payment>
        </pax>
        <pax>
          <pax_id>$(get pax_id_02)</pax_id>
          <grp_id>$(get grp_id_02)</grp_id>
          <pr_brd>1</pr_brd>
          <reg_no>4</reg_no>
          <surname>AKOPOVA</surname>
          <name>OLIVIIA</name>
          <airp_arv>AER</airp_arv>
          <seat_no/>
          <seats>0</seats>
          <ticket_no>2986145134263</ticket_no>
          <coupon_no>2</coupon_no>
          <document>VIAG519994</document>
          <tid>$(get_single_pax_tid $(get point_dep) AKOPOVA OLIVIIA)</tid>
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
          <tid>$(get_single_pax_tid $(get point_dep) VERGUNOV "VASILII LEONIDOVICH")</tid>
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
  $(get pax_id_01) 0 3 STIPIDI ANGELINA $(get_lat_code aer $(get airp_arv))
  " 11Б" 1 2986145134262 2 0305555064 1
  $(get apis_01) $(get counters) $(MESSAGE_TAG MSG.PASSENGER.DEBARKED2)

)

$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_02) 777
  $(get_single_pax_tid $(get point_dep) AKOPOVA OLIVIIA) 0 capture=on)

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
  $(get pax_id_02) 0 4 AKOPOVA OLIVIIA $(get_lat_code aer $(get airp_arv))
  "" 0 2986145134263 2 VIAG519994 1
  $(get apis_02) $(get counters) $(MESSAGE_TAG MSG.PASSENGER.DEBARKED2)

)

%%

### test 2
### Регистрируем 3ВЗ и 2РМ. Один ВЗ - JMP
### Сажаем 5-х человек
### Высаживаем всех
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
          <tid>$(get_single_pax_tid $(get point_dep) VASILIADI "KSENIYA VALEREVNA")</tid>
          <pr_payment>1</pr_payment>
        </pax>
        <pax>
          <pax_id>$(get pax_id_04)</pax_id>
          <grp_id>$(get grp_id_04)</grp_id>
          <reg_no>2</reg_no>
          <surname>CHEKMAREV</surname>
          <name>RONALD</name>
          <airp_arv>AER</airp_arv>
          <seat_no/>
          <seats>0</seats>
          <ticket_no>2986145143704</ticket_no>
          <coupon_no>2</coupon_no>
          <document>VАГ815247</document>
          <tid>$(get_single_pax_tid $(get point_dep) CHEKMAREV RONALD)</tid>
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
          <tid>$(get_single_pax_tid $(get point_dep) STIPIDI ANGELINA)</tid>
          <pr_payment>1</pr_payment>
        </pax>
        <pax>
          <pax_id>$(get pax_id_02)</pax_id>
          <grp_id>$(get grp_id_02)</grp_id>
          <reg_no>4</reg_no>
          <surname>AKOPOVA</surname>
          <name>OLIVIIA</name>
          <airp_arv>AER</airp_arv>
          <seat_no/>
          <seats>0</seats>
          <ticket_no>2986145134263</ticket_no>
          <coupon_no>2</coupon_no>
          <document>VIAG519994</document>
          <tid>$(get_single_pax_tid $(get point_dep) AKOPOVA OLIVIIA)</tid>
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
          <tid>$(get_single_pax_tid $(get point_dep) VERGUNOV "VASILII LEONIDOVICH")</tid>
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
### Ошибки при попытках посадки:
###  1. Несовпадение pax.tid (при посадке или при подгрузке APIS)
###  2. Пассажир не зарегистрирован (по рег. номеру, ид., штрих-коду)
###  3. Что-то не так со штрих-кодом
###  4. Пассажир с другого рейса (по ид., штрих-коду)
###  5. Не указан зал посадки, неверный зал посадки
###  6. Пассажир уже посажен, пассажир уже высажен
###  7. Пассажир не прошел досмотр
###  8. Посадка JMP запрещена (tsDeniedBoardingJMP)
### На будущее:
### Пассажир не подтвержден с листа ожидания
### Пассажир не оплатил багаж/услуги
### У пассажира неполный APIS
### У пассажира данные APIS введены вручную
#########################################################################################

$(init_term)

$(set airline UT)
$(set flt_no 280)
$(set airp_dep DME)
$(set airp_arv AER)

$(DO_CHECKIN $(get airline) $(get flt_no) $(get airp_dep) $(get airp_arv))

$(set pax_id_not_checked $(get_pax_id $(get point_dep) BABAKHANOVA KIRA))

$(set pax_tid_01 $(get_single_pax_tid $(get point_dep) STIPIDI ANGELINA))

$(BOARDING_REQUEST_BY_PAX_ID capture=on $(get point_dep) $(get pax_id_01) 777 $(+ $(get pax_tid_01) 5) 1)

$(USER_ERROR_RESPONSE MSG.PASSENGER.CHANGED_FROM_OTHER_DESK.REFRESH_DATA)

### запрос APIS с актуальным tid
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

### запрос APIS с неактуальным tid

$(LOAD_APIS_REQUEST capture=on $(get point_dep) $(get pax_id_01) $(+ $(get pax_tid_01) 5))

$(USER_ERROR_RESPONSE MSG.PASSENGER.NO_PARAM.CHANGED_FROM_OTHER_DESK.REFRESH_DATA)


$(BOARDING_REQUEST_BY_REG_NO capture=on $(get point_dep) 560 777 1)

$(USER_ERROR_RESPONSE MSG.PASSENGER.NOT_CHECKIN)


$(BOARDING_REQUEST_BY_PAX_ID capture=on $(get point_dep) $(get pax_id_not_checked) 777 "" 1)

$(USER_ERROR_RESPONSE MSG.PASSENGER.NOT_CHECKIN)


### !!!проверить SearchPaxByScanData

$(BOARDING_REQUEST_BY_REG_NO capture=on $(+  $(get point_dep) 1000) 2 777 1)

$(USER_ERROR_RESPONSE MSG.FLIGHT.NOT_FOUND.REFRESH_DATA)


### находимся на другом рейсе - передаем другой point_dep

$(BOARDING_REQUEST_BY_PAX_ID capture=on $(+  $(get point_dep) 1000) $(get pax_id_01) 777 "" 1)

$(USER_ERROR_RESPONSE MSG.PASSENGER.FROM_FLIGHT 100)


$(BOARDING_REQUEST_BY_SCAN_DATA capture=on $(+  $(get point_dep) 1000) $(get_bcbp $(get grp_id_03) $(get pax_id_03)) 777 1)

$(USER_ERROR_RESPONSE MSG.PASSENGER.FROM_FLIGHT 100)


$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_05) "" "" 1 capture=on)  #зал не указан

>> lines=auto
$(USER_ERROR_MESSAGE_TAG MSG.NOT_SET_BOARDING_HALL)

$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_05) 778 "" 1 capture=on) #зал не соответствует а/п вылета
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
  $(get pax_id_05) 1 5 VERGUNOV "VASILII LEONIDOVICH" $(get_lat_code aer $(get airp_arv))
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
  $(get pax_id_05) 0 5 VERGUNOV "VASILII LEONIDOVICH" $(get_lat_code aer $(get airp_arv))
  "JMP" 1 2986145212943 1 0305984920 1
  $(get apis_05) $(get counters) $(USER_ERROR_MESSAGE_TAG MSG.PASSENGER.NOT_BOARDING2 120)
)


### запрещаем посадку JMP

$(sql {INSERT INTO misc_set(id, type, airline, flt_no, airp_dep, pr_misc)
       VALUES(id__seq.nextval, 80, '$(get_elem_id etAirline $(get airline))', NULL, NULL, 1)})

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
  $(get pax_id_05) 0 5 VERGUNOV "VASILII LEONIDOVICH" $(get_lat_code aer $(get airp_arv))
  "JMP" 1 2986145212943 1 0305984920 1
  $(get apis_05) $(get counters) $(USER_ERROR_MESSAGE_TAG MSG.PASSENGER.JMP_BOARDED_DENIAL)
)


#########################################################################################
### проверяем, что по двум залам не посадим, если настройка "Досмотровый контроль перед посадкой" включена

$(CHANGE_TRIP_SETS $(get point_dep) pr_exam=1)

$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_01) 776 "" 1 capture=on)

>> lines=auto
$(USER_ERROR_MESSAGE_TAG MSG.PASSENGER.NOT_EXAM2)

$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_03) 777 "" 1 capture=on)

>> lines=auto
$(USER_ERROR_MESSAGE_TAG MSG.PASSENGER.NOT_EXAM2)


#########################################################################################
### проверяем, что по одному из залов посадим, если настройка "Досмотр при посадке на рейс" для этого зала

$(combine_exam_with_brd $(get point_dep) 777)

$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_01) 776 "" 1 capture=on)

>> lines=auto
$(USER_ERROR_MESSAGE_TAG MSG.PASSENGER.NOT_EXAM2)

$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_03) 777 "" 1 capture=on)

>> lines=auto
$(MESSAGE_TAG MSG.PASSENGER.BOARDING2)


#########################################################################################
### проверяем, что теперь и по другому залу посадим, если настройка "Досмотр при посадке на рейс" для всех залов

$(combine_exam_with_brd $(get point_dep) "")

$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_01) 776 "" 1 capture=on)

>> lines=auto
$(MESSAGE_TAG MSG.PASSENGER.BOARDING2)


#########################################################################################
### удаляем "Досмотр при посадке на рейс" для всех залов
### проверяем, что настройка "Досмотровый контроль перед посадкой" на высадку никак не влияет

$(sql {DELETE FROM trip_hall WHERE point_id=$(get point_dep)})

$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_01) 776 "" 0 capture=on)

>> lines=auto
$(MESSAGE_TAG MSG.PASSENGER.DEBARKED2)

$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id_03) 777 "" 0 capture=on)

>> lines=auto
$(MESSAGE_TAG MSG.PASSENGER.DEBARKED2)

%%

### test 4
### Подтверждения при посадке:
#########################################################################################
