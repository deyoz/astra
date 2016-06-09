include(ts/macro.ts)

# meta: suite print

###
### ИНФО:
### ДЛЯ НОВЫХ ТЕСТОВ ВОЗМОЖНО ПОТРЕБУЮТСЯ другие данные в Loader'ах ТАБЛИЦ
### bp_types, bp_models, prn_form_vers, prn_tag_props  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
###

$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_eds ЮТ UTET UTDC)

$(prepare_bp_printing ЮТ 103 ДМД)
$(PREPARE_FLIGHT_1 ЮТ 103 ДМД ПЛК РЕПИН ИВАН)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) РЕПИН ИВАН K))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2981212121212 РЕПИН ИВАН)


!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>3</agent_stat_period>
      <transfer/>
      <segments>
        <segment>
          <point_dep>$(get point_dep)</point_dep>
          <point_arv>$(get point_arv)</point_arv>
          <airp_dep>ДМД</airp_dep>
          <airp_arv>ПЛК</airp_arv>
          <class>Э</class>
          <status>K</status>
          <wl_type/>
          <mark_flight>
            <airline>ЮТ</airline>
            <flt_no>103</flt_no>
            <suffix/>
            <scd>$(date_format %d.%m.%Y) 00:00:00</scd>
            <airp_dep>ДМД</airp_dep>
            <pr_mark_norms>0</pr_mark_norms>
          </mark_flight>
          <passengers>
            <pax>
              <pax_id>$(get pax_id)</pax_id>
              <surname>РЕПИН</surname>
              <name>ИВАН</name>
              <pers_type>ВЗ</pers_type>
              <seat_no/>
              <preseat_no/>
              <seat_type/>
              <seats>1</seats>
              <ticket_no>2981212121212</ticket_no>
              <coupon_no>1</coupon_no>
              <ticket_rem>TKNE</ticket_rem>
              <ticket_confirm>0</ticket_confirm>
              <document>
                <type>P</type>
                <issue_country>RUS</issue_country>
                <no>7774441110</no>
                <nationality>RUS</nationality>
                <birth_date>01.05.1976 00:00:00</birth_date>
                <gender>M</gender>
                <surname>РЕПИН</surname>
                <first_name>ИВАН</first_name>
              </document>
              <doco/>
              <addresses/>
              <subclass>Э</subclass>
              <bag_pool_num/>
              <transfer/>
              <rems>
                <rem>
                  <rem_code>FOID</rem_code>
                  <rem_text>FOID PP7774441110</rem_text>
                </rem>
              </rems>
              <norms/>
            </pax>
          </passengers>
          <paid_bag_emd/>
        </segment>
      </segments>
      <excess>0</excess>
      <hall>1</hall>
      <paid_bags>
        <paid_bag>
          <bag_type/>
          <weight>0</weight>
          <rate_id/>
          <rate_trfer/>
        </paid_bag>
      </paid_bags>
    </TCkinSavePax>
  </query>
</term>}

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+ЮТ:МОВ++++Y+::RU+МОВРОМ"
EQN+1:TD"
TKT+2981212121212:T"
CPN+1:CK"
TVL+$(ddmmyy)+ДМД+ПЛК+ЮТ+103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"


<<
UNB+SIRE:1+UTET+UTDC+160408:0828+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref)"
MSG+:142+3"
EQN+2:TD"
TKT+2981212121212:T::3"
CPN+1:CK::E"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"


$(KICK_IN_SILENT)

$(set grp_id $(get_single_grp_id $(get point_dep) РЕПИН ИВАН))
$(set tid $(get_single_tid $(get point_dep) РЕПИН ИВАН))

$(dump_table TRIP_BP)


!! capture=on
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='print' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <GetGRPPrintDataBP>
      <grp_id>$(get grp_id)</grp_id>
      <pr_all>1</pr_all>
      <dev_model>506</dev_model>
      <fmt_type>ATB</fmt_type>
      <prnParams>
        <pr_lat>0</pr_lat>
        <encoding>CP866</encoding>
        <offset>20</offset>
        <top>0</top>
      </prnParams>
      <clientData>
        <gate>1</gate>
      </clientData>
    </GetGRPPrintDataBP>
  </query>
</term>}


>> lines=auto
    <data>
      <printBP>
        <pectab>PT{##}?K1Z{#}@;{#}TICK{#}&gt;&gt;/{#}BOARD{#}0101{#}0250E01W{#}0311H01W{#}0411L01W{#}0508H13W{#}0705H40{#}0906L27W{#}0A05M12R{#}0B01H30W{#}0C06L38W{#}0D16O02W{#}0E21Q01Q52W{#}0F15R01R53W{#}2020C53W{#}2120E53W{#}2220H53W{#}2508O53W{#}2705L53O{#}2B01L67W{#}2C06O64W{#}3FB1R30B601031{#}F104D41A54{#}FF72M01W{#}</pectab>
        <passengers>
          <pax pax_id...
            <prn_form hex='0'>0,20,0,ACT:             '$(date_format %d.%m.%Y) 10:00:00                     ' 0,25,0,AGENT:           'PIKE                                    ' 0,30,0,AIRLINE:         'ЮТ                                      ' 0,35,0,AIRLINE_NAME:    'ОАО АВИАКОМПАНИЯ ЮТЭЙР                  ' 0,40,0,AIRLINE_SHORT:   'ЮТЭЙР                                   ' 0,45,0,AIRP_ARV:        'ПЛК                                     ' 0,50,0,AIRP_ARV_NAME:   'ПУЛКОВО                                 ' 0,55,0,AIRP_DEP:        'ДМД                                     ' 0,60,0,AIRP_DEP_NAME:   'ДОМОДЕДОВО                              ' 0,65,0,BAG_AMOUNT:      '0                                       ' 0,70,0,BAGGAGE:         '                                        ' 0,75,0,BAG_WEIGHT:      '0                                       ' 0,80,0,BCBP_M_2:        'M1?????/????          E0840Z6 DMELEDUT 0103 161Y001A0001 128&gt;2180OO    B                00000000$(get pax_id)' 0,85,0,BRD_FROM:        '$(date_format %d.%m.%Y) 09:15:00                     ' 0,90,0,BRD_TO:          '$(date_format %d.%m.%Y) 09:35:00                     ' 0,95,0,CHD:             '                                        ' 0,100,0,CITY_ARV_NAME:  'САНКТ-ПЕТЕРБУРГ                         ' 0,105,0,CITY_DEP_NAME:  'МОСКВА                                  ' 0,110,0,CLASS:          'Э                                       ' 0,115,0,CLASS_NAME:     'ЭКОНОМ                                  ' 0,120,0,DESK:           'МОВРОМ                                  ' 0,125,0,DOCUMENT:       '7774441110                              ' 0,130,0,DUPLICATE:      '                                        ' 0,135,0,EST:            '$(date_format %d.%m.%Y) 10:00:00                     ' 0,140,0,ETICKET_NO:     '2981212121212/1                         ' 0,145,0,ETKT:           'ETKT2981212121212/1                     ' 0,150,0,EXCESS:         '0                                       ' 0,155,0,FLT_NO:         '103                                     ' 0,160,0,FQT:            '                                        ' 0,165,0,FULLNAME:       'РЕПИН ИВАН                              ' 0,170,0,FULL_PLACE_ARV: 'САНКТ-ПЕТЕРБУРГ ПУЛКОВО                 ' 0,175,0,FULL_PLACE_DEP: 'МОСКВА ДОМОДЕДОВО                       ' 0,180,0,GATE:           '1                                       ' 0,185,0,GATES:          '                                        ' 0,190,0,HALL:           'Зал 1                                   ' 0,195,0,INF:            '                                        ' 0,200,0,LIST_SEAT_NO:   '1A                                      ' 0,205,0,LONG_ARV:       'САНКТ-ПЕТЕРБУРГ(ПЛК)/ST PETERSBURG(LED) ' 0,210,0,LONG_DEP:       'МОСКВА(ДМД)/MOSCOW(DME)                 ' 0,215,0,NAME:           'ИВАН                                    ' 0,220,0,NO_SMOKE:       'X                                       ' 0,225,0,ONE_SEAT_NO:    '1A                                      ' 0,230,0,PAX_ID:         '000000$(get pax_id)                              ' 0,235,0,PAX_TITLE:      'Г-Н                                     ' 0,240,0,PLACE_ARV:      'САНКТ-ПЕТЕРБУРГ(ПЛК)                    ' 0,245,0,PLACE_DEP:      'МОСКВА(ДМД)                             ' 0,250,0,PNR:            '0840Z6                                  ' 0,255,0,REG_NO:         '001                                     ' 0,260,0,REM:            '                                        ' 0,265,0,RK_AMOUNT:      '0                                       ' 0,270,0,RK_WEIGHT:      '0                                       ' 0,275,0,RSTATION:       '                                        ' 0,280,0,SCD:            '$(date_format %d.%m.%Y) 10:00:00                     ' 0,285,0,SEAT_NO:        '1A                                      ' 0,290,0,STR_SEAT_NO:    '1A                                      ' 0,295,0,SUBCLS:         'Э                                       ' 0,300,0,SURNAME:        'РЕПИН                                   ' 0,305,0,TAGS:           '                                        ' 0,310,0,TEST_SERVER:    '                                        ' 0,315,0,TIME_PRINT:     '$(date_format %d.%m.%Y) xx:xx:xx                     '</prn_form>
          </pax>
        </passengers>
      </printBP>
    </data>
  </answer>
