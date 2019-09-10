include(ts/macro.ts)
include(ts/sirena_exchange_macro.ts)

# meta: suite checkin


### test 1 - регистрация одного пассажира на одном сегменте
#########################################################################################

$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_eds ЮТ UTET UTDC)

$(PREPARE_FLIGHT_1PAX_1SEG ЮТ 103 ДМД ПЛК РЕПИН ИВАН)

$(sql "update TRIP_SETS set PIECE_CONCEPT=1")
$(sql "update DESKS set VERSION='201707-0195750'")

$(settcl SIRENA_HOST localhost)
$(settcl SIRENA_PORT 8008)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) РЕПИН ИВАН))

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
              <fqt_rems/>
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
$(TKCREQ_ET_COS UTDC UTET $(last_edifact_ref) ЮТ 2981212121212 1 CK)
<<
$(TKCRES_ET_COS UTET UTDC $(last_edifact_ref) 2981212121212 1 CK)


$(http_forecast content=$(get_svc_availability_resp))

$(KICK_IN_SILENT)

>> lines=auto
<query>
  <svc_availability show_brand_info=\"true\" show_all_svc=\"true\" show_free_carry_on_norm=\"true\">
    <passenger id=\"...\" surname=\"РЕПИН\" name=\"ИВАН\" category=\"ADT\" birthdate=\"1976-05-01\" sex=\"male\">
      <document number=\"7774441110\" country=\"RUS\"/>
      <segment id=\"0\" company=\"UT\" flight=\"103\" operating_company=\"UT\" operating_flight=\"103\" departure=\"DME\" arrival=\"LED\" departure_time=\"xxxx-xx-xxT10:15:00\" arrival_time=\"xxxx-xx-xxT12:00:00\" equipment=\"xxx\" subclass=\"Y\">
        <ticket number=\"2981212121212\" coupon_num=\"1\" display_id=\"1\"/>
        <recloc crs=\"1H\">09T1B3</recloc>
        <recloc crs=\"UT\">0840Z6</recloc>
      </segment>
    </passenger>
    <display id=\"1\">UNB+SIRE:1+UTET+UTDC+xxxxxx:xxxx+xxxxxxxxxx0001+++T"
UNH+1+TKCRES:06:1:IA+xxxxxxxxxx"
MSG+:131+3"
TIF+РЕПИН+ИВАН"
TAI+0162"
RCI+ЮТ:G4LK6W:1"
MON+B:20.00:USD+T:20.00:USD"
FOP+CA:3"
PTK+++$(ddmmyy)+++:US"
ODI+MOW+LED"
ORG+xx:MOW++IAH++A+US+D80D1BWO"
EQN+1:TD"
TXD+700+0.00:::US"
IFT+4:15:1+ /FC 20DEC MOW UT SGC10.00YINF UT MOW10.00YINF NUC20.00END"
IFT+4:5+00001230161213"
IFT+4:10+REFUNDABLE"
IFT+4:39+HOUSTON+UNITED AIRLINES INC"
TKT+2981212121212:T:1:3"
CPN+1:I"
TVL+$(ddmmyy):2205+DME+LED+ЮТ+103:Y+J"
RPI++NS"
PTS++YINF"
UNT+19+1"
UNZ+1+xxxxxxxxxx0001"
</display>
  </svc_availability>
</query>


>> lines=auto
    <kick req_ctxt_id...

!!
$(lastRedisplay)

$(set grp_id $(get_single_grp_id $(get point_dep) РЕПИН ИВАН))
$(set tid $(get_single_tid $(get point_dep) РЕПИН ИВАН))

$(sql "insert into TRIP_BT(POINT_ID, TAG_TYPE) values($(get point_dep), 'ВНКС')")


# добавление багажа c ошибкой

$(http_forecast content=$(get_svc_payment_status_invalid_resp))

!!
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>3</agent_stat_period>
      <segments>
        <segment>
          <point_dep>$(get point_dep)</point_dep>
          <point_arv>$(get point_arv)</point_arv>
          <airp_dep>ДМД</airp_dep>
          <airp_arv>ПЛК</airp_arv>
          <class>Э</class>
          <grp_id>$(get grp_id)</grp_id>
          <tid>$(get tid)</tid>
          <passengers>
            <pax>
              <pax_id>$(last_generated_pax_id)</pax_id>
              <surname>РЕПИН</surname>
              <name>ИВАН</name>
              <pers_type>ВЗ</pers_type>
              <refuse/>
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
              <addresses>
                <doca>
                  <type>B</type>
                </doca>
                <doca>
                  <type>R</type>
                </doca>
                <doca>
                  <type>D</type>
                </doca>
              </addresses>
              <subclass>Э</subclass>
              <bag_pool_num>1</bag_pool_num>
              <subclass>Э</subclass>
              <tid>$(get tid)</tid>
            </pax>
          </passengers>
          <service_payment/>
        </segment>
      </segments>
      <hall>1</hall>
      <bag_refuse/>
      <value_bags/>
      <bags>
        <bag>
          <rfisc>0L1</rfisc>
          <airline>ЮТ</airline>
          <service_type>C</service_type>
          <num>1</num>
          <pr_cabin>0</pr_cabin>
          <amount>1</amount>
          <weight>13</weight>
          <value_bag_num/>
          <pr_liab_limit>0</pr_liab_limit>
          <to_ramp>0</to_ramp>
          <using_scales>0</using_scales>
          <is_trfer>0</is_trfer>
          <bag_pool_num>1</bag_pool_num>
        </bag>
      </bags>
      <tags pr_print='1'/>
      <unaccomps/>
    </TCkinSavePax>
  </query>
</term>}


>> lines=auto
<query>
  <svc_payment_status show_free_carry_on_norm=\"true\">
    <passenger id=\"...\" surname=\"РЕПИН\" name=\"ИВАН\" category=\"ADT\" birthdate=\"1976-05-01\" sex=\"male\">
      <document number=\"7774441110\" country=\"RUS\"/>
      <segment id=\"0\" company=\"UT\" flight=\"103\" operating_company=\"UT\" operating_flight=\"103\" departure=\"DME\" arrival=\"LED\" departure_time=\"xxxx-xx-xxTxx:xx:xx\" arrival_time=\"xxxx-xx-xxTxx:xx:xx\" equipment=\"xxx\" subclass=\"Y\">
        <ticket number=\"2981212121212\" coupon_num=\"1\" display_id=\"1\"/>
        <recloc crs=\"1H\">09T1B3</recloc>
        <recloc crs=\"UT\">0840Z6</recloc>
      </segment>
    </passenger>
    <display id=\"1\">UNB+SIRE:1+UTET+UTDC+xxxxxx:xxxx+xxxxxxxxxx0001+++T"
UNH+1+TKCRES:06:1:IA+xxxxxxxxxx"
MSG+:131+3"
TIF+РЕПИН+ИВАН"
TAI+0162"
RCI+ЮТ:G4LK6W:1"
MON+B:20.00:USD+T:20.00:USD"
FOP+CA:3"
PTK+++$(ddmmyy)+++:US"
ODI+MOW+LED"
ORG+xx:MOW++IAH++A+US+D80D1BWO"
EQN+1:TD"
TXD+700+0.00:::US"
IFT+4:15:1+ /FC 20DEC MOW UT SGC10.00YINF UT MOW10.00YINF NUC20.00END"
IFT+4:5+00001230161213"
IFT+4:10+REFUNDABLE"
IFT+4:39+HOUSTON+UNITED AIRLINES INC"
TKT+2981212121212:T:1:3"
CPN+1:I"
TVL+$(ddmmyy):2205+DME+LED+ЮТ+103:Y+J"
RPI++NS"
PTS++YINF"
UNT+19+1"
UNZ+1+xxxxxxxxxx0001"
</display>
    <svc passenger-id=\"...\" segment-id=\"0\" company=\"UT\" service_type=\"C\" rfisc=\"0L1\" rfic=\"C\" emd_type=\"EMD-A\">
      <name language=\"en\">FISHING EQUIPMENT UPTO44LB20KG</name>
      <name language=\"ru\">РЫБОЛОВНЫЕ СНАСТИ ДО 44Ф 20КГ</name>
    </svc>
  </svc_payment_status>
</query>


$(KICK_IN)

>> lines=auto
    <command>
      <user_error lexema_id='MSG.CHECKIN.UNABLE_CALC_PAID_BAG_TRY_RE_CHECKIN' code='0'>Невозможно произвести расчет оплачиваемого багажа. Попробуйте перерегистрировать пассажиров</user_error>
    </command>

# добавление багажа

$(http_forecast content=$(get_svc_payment_status_resp))

!!
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>3</agent_stat_period>
      <segments>
        <segment>
          <point_dep>$(get point_dep)</point_dep>
          <point_arv>$(get point_arv)</point_arv>
          <airp_dep>ДМД</airp_dep>
          <airp_arv>ПЛК</airp_arv>
          <class>Э</class>
          <grp_id>$(get grp_id)</grp_id>
          <tid>$(get tid)</tid>
          <passengers>
            <pax>
              <pax_id>$(last_generated_pax_id)</pax_id>
              <surname>РЕПИН</surname>
              <name>ИВАН</name>
              <pers_type>ВЗ</pers_type>
              <refuse/>
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
              <addresses>
                <doca>
                  <type>B</type>
                </doca>
                <doca>
                  <type>R</type>
                </doca>
                <doca>
                  <type>D</type>
                </doca>
              </addresses>
              <subclass>Э</subclass>
              <bag_pool_num>1</bag_pool_num>
              <subclass>Э</subclass>
              <tid>$(get tid)</tid>
            </pax>
          </passengers>
          <service_payment/>
        </segment>
      </segments>
      <hall>1</hall>
      <bag_refuse/>
      <value_bags/>
      <bags>
        <bag>
          <rfisc>0L1</rfisc>
          <airline>ЮТ</airline>
          <service_type>C</service_type>
          <num>1</num>
          <pr_cabin>0</pr_cabin>
          <amount>1</amount>
          <weight>13</weight>
          <value_bag_num/>
          <pr_liab_limit>0</pr_liab_limit>
          <to_ramp>0</to_ramp>
          <using_scales>0</using_scales>
          <is_trfer>0</is_trfer>
          <bag_pool_num>1</bag_pool_num>
        </bag>
      </bags>
      <tags pr_print='1'/>
      <unaccomps/>
    </TCkinSavePax>
  </query>
</term>}


>> lines=auto
<query>
  <svc_payment_status show_free_carry_on_norm=\"true\">
    <passenger id=\"...\" surname=\"РЕПИН\" name=\"ИВАН\" category=\"ADT\" birthdate=\"1976-05-01\" sex=\"male\">
      <document number=\"7774441110\" country=\"RUS\"/>
      <segment id=\"0\" company=\"UT\" flight=\"103\" operating_company=\"UT\" operating_flight=\"103\" departure=\"DME\" arrival=\"LED\" departure_time=\"xxxx-xx-xxTxx:xx:xx\" arrival_time=\"xxxx-xx-xxTxx:xx:xx\" equipment=\"xxx\" subclass=\"Y\">
        <ticket number=\"2981212121212\" coupon_num=\"1\" display_id=\"1\"/>
        <recloc crs=\"1H\">09T1B3</recloc>
        <recloc crs=\"UT\">0840Z6</recloc>
      </segment>
    </passenger>
    <display id=\"1\">UNB+SIRE:1+UTET+UTDC+xxxxxx:xxxx+xxxxxxxxxx0001+++T"
UNH+1+TKCRES:06:1:IA+xxxxxxxxxx"
MSG+:131+3"
TIF+РЕПИН+ИВАН"
TAI+0162"
RCI+ЮТ:G4LK6W:1"
MON+B:20.00:USD+T:20.00:USD"
FOP+CA:3"
PTK+++$(ddmmyy)+++:US"
ODI+MOW+LED"
ORG+xx:MOW++IAH++A+US+D80D1BWO"
EQN+1:TD"
TXD+700+0.00:::US"
IFT+4:15:1+ /FC 20DEC MOW UT SGC10.00YINF UT MOW10.00YINF NUC20.00END"
IFT+4:5+00001230161213"
IFT+4:10+REFUNDABLE"
IFT+4:39+HOUSTON+UNITED AIRLINES INC"
TKT+2981212121212:T:1:3"
CPN+1:I"
TVL+$(ddmmyy):2205+DME+LED+ЮТ+103:Y+J"
RPI++NS"
PTS++YINF"
UNT+19+1"
UNZ+1+xxxxxxxxxx0001"
</display>
    <svc passenger-id=\"...\" segment-id=\"0\" company=\"UT\" service_type=\"C\" rfisc=\"0L1\" rfic=\"C\" emd_type=\"EMD-A\">
      <name language=\"en\">FISHING EQUIPMENT UPTO44LB20KG</name>
      <name language=\"ru\">РЫБОЛОВНЫЕ СНАСТИ ДО 44Ф 20КГ</name>
    </svc>
  </svc_payment_status>
</query>


$(KICK_IN)

>> lines=auto
    <paid_rfiscs>
      <item>
        <rfisc>0L1</rfisc>
        <service_type>C</service_type>
        <airline>ЮТ</airline>
        <name_view>...
        <transfer_num>0</transfer_num>
        <service_quantity>1</service_quantity>
        <paid>0</paid>
        <priority>0</priority>
        <total_view>1</total_view>
        <paid_view>0</paid_view>
      </item>
    </paid_rfiscs>


%%
### test 2 - регистрация группы из двух пассажиров на одном сегменте
#########################################################################################

$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_eds ЮТ UTET UTDC)

$(PREPARE_FLIGHT_2PAXES_1SEG ЮТ 103 ДМД ПЛК РЕПИН ВАСИЛИЙ РЕПИНА АННА)

$(dump_table CRS_PAX)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(set pax1_id $(get_single_pax_id $(get point_dep) РЕПИН ВАСИЛИЙ))
$(set pax2_id $(get_single_pax_id $(get point_dep) РЕПИНА АННА))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2981212121212 РЕПИН ВАСИЛИЙ)
$(SAVE_ET_DISP $(get point_dep) 2981212121213 РЕПИНА АННА)


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
              <pax_id>$(get pax1_id)</pax_id>
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
            <pax>
              <pax_id>$(get pax2_id)</pax_id>
              <surname>РЕПИНА</surname>
              <name>АННА</name>
              <pers_type>ВЗ</pers_type>
              <seat_no/>
              <preseat_no/>
              <seat_type/>
              <seats>1</seats>
              <ticket_no>2981212121213</ticket_no>
              <coupon_no>1</coupon_no>
              <ticket_rem>TKNE</ticket_rem>
              <ticket_confirm>0</ticket_confirm>
              <document>
                <type>P</type>
                <issue_country>RUS</issue_country>
                <no>7774441111</no>
                <nationality>RUS</nationality>
                <birth_date>01.05.1978 00:00:00</birth_date>
                <gender>Ж</gender>
                <surname>РЕПИНА</surname>
                <first_name>АННА</first_name>
              </document>
              <doco/>
              <addresses/>
              <subclass>Э</subclass>
              <bag_pool_num/>
              <transfer/>
              <rems>
                <rem>
                  <rem_code>FOID</rem_code>
                  <rem_text>FOID PP7774441111</rem_text>
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
$(TKCREQ_ET_COS UTDC UTET $(last_edifact_ref 1) ЮТ 2981212121212 1 CK)
>>
$(TKCREQ_ET_COS UTDC UTET $(last_edifact_ref 0) ЮТ 2981212121213 1 CK)

<<
$(TKCRES_ET_COS UTET UTDC $(last_edifact_ref 1) 2981212121212 1 CK)
<<
$(TKCRES_ET_COS UTET UTDC $(last_edifact_ref 0) 2981212121213 1 CK)



$(KICK_IN)

>> lines=auto
    <segments>
      <segment>
        <tripheader>
          <flight>ЮТ103 ДМД</flight>
          <flight_short>...
          <airline>ЮТ</airline>
          <aircode>298</aircode>
          <flt_no>103</flt_no>
          <suffix/>
          <airp>ДМД</airp>
          <scd_out_local>$(date_format %d.%m.%Y) 10:15:00</scd_out_local>
          <pr_etl_only>0</pr_etl_only>
          <pr_etstatus>0</pr_etstatus>
          <pr_no_ticket_check>0</pr_no_ticket_check>
          <pr_auto_pt_print>0</pr_auto_pt_print>
          <pr_auto_pt_print_reseat>0</pr_auto_pt_print_reseat>
          <use_jmp>0</use_jmp>
        </tripheader>
        <tripdata>
          <airps>
            <airp>
              <point_id>...
              <airp_code>ПЛК</airp_code>
              <city_code>СПТ</city_code>
              <target_view>САНКТ-ПЕТЕРБУРГ (ПЛК)</target_view>
              <check_info>
                <pass>
                  <doc/>
                  <doco/>
                  <doca_b/>
                  <doca_r/>
                  <doca_d/>
                  <tkn/>
                </pass>
                <crew>
                  <doc/>
                  <doco/>
                  <doca_b/>
                  <doca_r/>
                  <doca_d/>
                  <tkn/>
                </crew>
              </check_info>
            </airp>
          </airps>
          <classes>
            <class>
              <code>Э</code>
              <class_view>ЭКОНОМ</class_view>
              <cfg>...
            </class>
          </classes>
          <gates/>
          <halls>
            <hall>
              <id>1</id>
              <name>Зал 1</name>
            </hall>
            <hall>
              <id>0</id>
              <name>Др.</name>
            </hall>
            <hall>
              <id>1141</id>
              <name>VIP</name>
            </hall>
            <hall>
              <id>1439</id>
              <name>Пав. вокз.</name>
            </hall>
            <hall>
              <id>39706</id>
              <name>супер зал</name>
            </hall>
          </halls>
          <mark_flights>
            <flight>
              <airline>ЮТ</airline>
              <flt_no>103</flt_no>
              <suffix/>
              <scd>$(date_format %d.%m.%Y) 10:15:00</scd>
              <airp_dep>ДМД</airp_dep>
              <pr_mark_norms>0</pr_mark_norms>
            </flight>
          </mark_flights>
        </tripdata>
        <grp_id>...
        <point_dep>$(get point_dep)</point_dep>
        <airp_dep>ДМД</airp_dep>
        <point_arv>$(get point_arv)</point_arv>
        <airp_arv>ПЛК</airp_arv>
        <class>Э</class>
        <status>K</status>
        <bag_refuse/>
        <bag_types_id>...
        <piece_concept>0</piece_concept>
        <tid>...
        <show_ticket_norms>0</show_ticket_norms>
        <show_wt_norms>1</show_wt_norms>
        <city_arv>СПТ</city_arv>
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
            <pax_id>$(get pax1_id)</pax_id>
            <surname>РЕПИН</surname>
            <name>ИВАН</name>
            <pers_type>ВЗ</pers_type>
            <crew_type/>
            <seat_no>...
            <seat_type/>
            <seats>1</seats>
            <refuse/>
            <reg_no>1</reg_no>
            <subclass>Э</subclass>
            <cabin_subclass>Э</cabin_subclass>
            <cabin_class>Э</cabin_class>
            <bag_pool_num/>
            <tid>...
            <ticket_no>2981212121212</ticket_no>
            <coupon_no>1</coupon_no>
            <ticket_rem>TKNE</ticket_rem>
            <ticket_confirm>1</ticket_confirm>
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
            <ticket_bag_norm>нет</ticket_bag_norm>
            <pr_norec>0</pr_norec>
            <pr_bp_print>0</pr_bp_print>
            <pr_bi_print>0</pr_bi_print>
            <service_lists>
              <service_list...
              <service_list...
            </service_lists>
            <rems>
              <rem>
                <rem_code>FOID</rem_code>
                <rem_text>FOID PP7774441110</rem_text>
              </rem>
            </rems>
            <asvc_rems/>
            <fqt_rems/>
            <norms/>
          </pax>
          <pax>
            <pax_id>$(get pax2_id)</pax_id>
            <surname>РЕПИНА</surname>
            <name>АННА</name>
            <pers_type>ВЗ</pers_type>
            <crew_type/>
            <seat_no>1B</seat_no>
            <seat_type/>
            <seats>1</seats>
            <refuse/>
            <reg_no>2</reg_no>
            <subclass>Э</subclass>
            <cabin_subclass>Э</cabin_subclass>
            <cabin_class>Э</cabin_class>
            <bag_pool_num/>
            <tid>...
            <ticket_no>2981212121213</ticket_no>
            <coupon_no>1</coupon_no>
            <ticket_rem>TKNE</ticket_rem>
            <ticket_confirm>1</ticket_confirm>
            <document>
              <type>P</type>
              <issue_country>RUS</issue_country>
              <no>7774441111</no>
              <nationality>RUS</nationality>
              <birth_date>01.05.1978 00:00:00</birth_date>
              <surname>РЕПИНА</surname>
              <first_name>АННА</first_name>
            </document>
            <ticket_bag_norm>нет</ticket_bag_norm>
            <pr_norec>0</pr_norec>
            <pr_bp_print>0</pr_bp_print>
            <pr_bi_print>0</pr_bi_print>
            <service_lists>
              <service_list...
              <service_list...
            </service_lists>
            <rems>
              <rem>
                <rem_code>FOID</rem_code>
                <rem_text>FOID PP7774441111</rem_text>
              </rem>
            </rems>
            <asvc_rems/>
            <fqt_rems/>
            <norms/>
          </pax>
        </passengers>
        <paid_bag_emd/>
        <tripcounters>
          <item>
            <point_arv>$(get point_arv)</point_arv>
            <class>Э</class>
            <noshow>...
            <trnoshow>...
            <show>...
            <free_ok>...
            <free_goshow>...
            <nooccupy>...
          </item>
        </tripcounters>
        <load_residue/>
      </segment>
    </segments>



%%
### test 3 - регистрация одного пассажира на одном сегменте с ошибкой обмена с Сиреной
#########################################################################################

$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_eds ЮТ UTET UTDC)

$(PREPARE_FLIGHT_1PAX_1SEG ЮТ 103 ДМД ПЛК РЕПИН ИВАН)

$(sql "update TRIP_SETS set PIECE_CONCEPT=1")
$(sql "update DESKS set VERSION='201707-0195750'")

$(settcl SIRENA_HOST localhost)
$(settcl SIRENA_PORT 8008)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) РЕПИН ИВАН))

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
              <fqt_rems/>
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
$(TKCREQ_ET_COS UTDC UTET $(last_edifact_ref) ЮТ 2981212121212 1 CK)
<<
$(TKCRES_ET_COS UTET UTDC $(last_edifact_ref) 2981212121212 1 CK)



$(http_forecast content=$(get_svc_availability_invalid_resp))

$(KICK_IN_SILENT)

>> lines=auto
<query>
  <svc_availability show_brand_info=\"true\" show_all_svc=\"true\" show_free_carry_on_norm=\"true\">
    <passenger id=\"...\" surname=\"РЕПИН\" name=\"ИВАН\" category=\"ADT\" birthdate=\"1976-05-01\" sex=\"male\">
      <document number=\"7774441110\" country=\"RUS\"/>
      <segment id=\"0\" company=\"UT\" flight=\"103\" operating_company=\"UT\" operating_flight=\"103\" departure=\"DME\" arrival=\"LED\" departure_time=\"xxxx-xx-xxT10:15:00\" arrival_time=\"xxxx-xx-xxT12:00:00\" equipment=\"xxx\" subclass=\"Y\">
        <ticket number=\"2981212121212\" coupon_num=\"1\" display_id=\"1\"/>
        <recloc crs=\"1H\">09T1B3</recloc>
        <recloc crs=\"UT\">0840Z6</recloc>
      </segment>
    </passenger>
    <display id=\"1\">UNB+SIRE:1+UTET+UTDC+xxxxxx:xxxx+xxxxxxxxxx0001+++T"
UNH+1+TKCRES:06:1:IA+xxxxxxxxxx"
MSG+:131+3"
TIF+РЕПИН+ИВАН"
TAI+0162"
RCI+ЮТ:G4LK6W:1"
MON+B:20.00:USD+T:20.00:USD"
FOP+CA:3"
PTK+++$(ddmmyy)+++:US"
ODI+MOW+LED"
ORG+xx:MOW++IAH++A+US+D80D1BWO"
EQN+1:TD"
TXD+700+0.00:::US"
IFT+4:15:1+ /FC 20DEC MOW UT SGC10.00YINF UT MOW10.00YINF NUC20.00END"
IFT+4:5+00001230161213"
IFT+4:10+REFUNDABLE"
IFT+4:39+HOUSTON+UNITED AIRLINES INC"
TKT+2981212121212:T:1:3"
CPN+1:I"
TVL+$(ddmmyy):2205+DME+LED+ЮТ+103:Y+J"
RPI++NS"
PTS++YINF"
UNT+19+1"
UNZ+1+xxxxxxxxxx0001"
</display>
  </svc_availability>
</query>


>> lines=auto
    <kick req_ctxt_id...

!! capture=on
$(lastRedisplay)

>> lines=auto
    <command>
      <user_error_message lexema_id='MSG.ERROR_CHECKING_PIECE_CONCEPT_WEIGHT_CONCEPT_APPLIED' code='0'>Ошибка определения системы расчета багажа. Применена весовая система расчета</user_error_message>
    </command>


%%
### test 4 - регистрация одного пассажира на одном сегменте с ошибкой обмена с СЭБ
#########################################################################################

$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_eds ЮТ UTET UTDC)

$(PREPARE_FLIGHT_1PAX_1SEG ЮТ 103 ДМД ПЛК РЕПИН ИВАН)

$(sql "update DESKS set VERSION='201707-0195750'")

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) РЕПИН ИВАН))

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
              <fqt_rems/>
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
$(TKCREQ_ET_COS UTDC UTET $(last_edifact_ref) ЮТ 2981212121212 1 CK)
<<
$(TKCRES_ET_COS_FAKE_ERR UTET UTDC $(last_edifact_ref) 2981212121212 1 CK 401)



>> lines=auto
    <kick req_ctxt_id...

!! capture=on
$(lastRedisplay)

>> lines=auto
    <ets_error>Пассажир РЕПИН ИВАН ():
     Ошибка при изменении статуса эл. билета 2981212121212/1: 401.



%%
### test 5 - iapi
#########################################################################################

$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_eds ЮТ UTET UTDC)

$(PREPARE_FLIGHT_1PAX_1SEG ЮТ 103 ДМД BJS REPIN IVAN)

$(sql "update DESKS set VERSION='201707-0195750'")
$(sql "insert into APPS_SETS(AIRLINE, APPS_COUNTRY, FLT_CLOSEOUT, FORMAT, ID, INBOUND, OUTBOUND, PR_DENIAL) values ('ЮТ', 'ЦН', 1, 'IAPI_CN', id__seq.nextval, 1, 1, 0)")
$(sql "insert into EDI_ADDRS(ADDR, CFG_ID, CANON_NAME) values ('NIAC', 0, 'MOWET')")
$(sql "insert into EDIFACT_PROFILES (NAME, VERSION, SUB_VERSION, CTRL_AGENCY, SYNTAX_NAME, SYNTAX_VER) values ('IAPI', 'D', '05B', 'UN', 'UNOA', 4)")
$(sql "update ROT set H2H=1, H2H_REM_ADDR_NUM=1, H2H_ADDR='1HCNIAPIR', OUR_H2H_ADDR='1HCNIAPIQ'")

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set move_id $(get_move_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN))

$(OPEN_CHECKIN $(get point_dep))

$(sql "insert into TRIP_HALL (PR_MISC, TYPE, POINT_ID) values (1, 101, $(get point_dep))")

$(SAVE_ET_DISP $(get point_dep) 2981212121212 REPIN IVAN ЮТ UTDC UTET G4LK6W DME BJS)


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
          <airp_arv>BJS</airp_arv>
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
              <surname>REPIN</surname>
              <name>IVAN</name>
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
                <expiry_date>09.01.2021 00:00:00</expiry_date>
                <gender>M</gender>
                <surname>REPIN</surname>
                <first_name>IVAN</first_name>
              </document>
              <doco>
                <birth_place/>
                <type>V</type>
                <no>4538926</no>
                <issue_place>RUS</issue_place>
                <issue_date>09.01.2011 00:00:00</issue_date>
                <expiry_date>09.01.2021 00:00:00</expiry_date>
                <applic_country>CHN</applic_country>
              </doco>
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
              <fqt_rems/>
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


$(set ediref_tktreq $(last_edifact_ref))


>> lines=auto
BGM+745"


>>
$(TKCREQ_ET_COS UTDC UTET $(get ediref_tktreq) ЮТ 2981212121212 1 BD xxxxxx ДМД BJS)
<<
$(TKCRES_ET_COS UTET UTDC $(get ediref_tktreq) 2981212121212 1 BD)

>> lines=auto
    <kick req_ctxt_id...

!!
$(lastRedisplay)

$(set ediref_paxlst $(last_edifact_ref))
$(set tpr $(substr $(get ediref_paxlst) 6, 4))

>>
UNB+SIRE:4+NORDWIND:ZZ+NIAC:ZZ+xxxxxx:xxxx+$(get ediref_paxlst)0001++IAPI+O"
UNG+PAXLST+NORDWIND:ZZ+NIAC:ZZ+xxxxxx:xxxx+1+UN+D:05B"
UNH+11085B94E1F8FA+PAXLST:D:05B:UN:IATA+$(get ediref_paxlst)+01:F"
BGM+745"
RFF+TN...
NAD+MS+++TEST CONTACT"
COM+12 34 56 78:TE+98765432:FX"
TDT+20+UT103+++UT"
LOC+125+DME"
DTM+189:xxxxxxxxxx:201"
LOC+87+BJS"
DTM+232:xxxxxxxxxx:201"
NAD+FL+++REPIN:IVAN"
ATT+2++M"
DTM+329:760501"
GEI+4+173"
LOC+178+DME"
LOC+179+BJS"
NAT+2+RUS"
RFF+AVF:xxxxxx"
RFF+ABO...
RFF+YZY:2981212121212C1"
DOC+P:110:ZZZ+7774441110"
DTM+36:210109"
LOC+91+RUS"
DOC+V:110:ZZZ+4538926"
DTM+36:210109"
LOC+91+RUS"
CNT+42:1"
UNT+28+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst)0001"


<< h2h=V.\VDLG.WA/E11HCNIAPIR/I11HCNIAPIQ/P$(get tpr)\VGYA\$() charset=UNOA
UNB+SIRE:4+NIAC:ZZ+NORDWIND:ZZ+190819:0930+$(get ediref_paxlst)0001++IAPI"
UNG+CUSRES+NIAC:ZZ+NORDWIND:ZZ+190819:0930+1+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN:IATA"
BGM+962"
RFF+TN:$(lastAppsMsgId)"
RFF+AF:FV256"
DTM+189:1908011800:201"
DTM+232:1908012300:201"
LOC+125+DME"
LOC+87+BJS"
ERP+2"
RFF+AVF:0840Z6"
RFF+ABO:$(get pax_id)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
UNT+14+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst)"


!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='sopp' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <WriteDests>
      <data>
        <move_id>$(get move_id)</move_id>
        <canexcept>1</canexcept>
        <reference/>
        <dests>
          <dest>
            <modify/>
            <point_id>$(get point_dep)</point_id>
            <point_num>0</point_num>
            <airp>ДМД</airp>
            <airline>ЮТ</airline>
            <flt_no>103</flt_no>
            <craft>735</craft>
            <scd_out>$(date_format %d.%m.%Y) 14:00:00</scd_out>
            <act_out>$(date_format %d.%m.%Y) 14:00:00</act_out>
            <trip_type>п</trip_type>
            <pr_tranzit>0</pr_tranzit>
            <pr_reg>1</pr_reg>
          </dest>
          <dest>
            <point_id>$(get point_arv)</point_id>
            <point_num>1</point_num>
            <first_point>$(get point_dep)</first_point>
            <airp>BJS</airp>
            <scd_in>$(date_format %d.%m.%Y) 18:00:00</scd_in>
            <trip_type>п</trip_type>
            <pr_tranzit>0</pr_tranzit>
            <pr_reg>0</pr_reg>
          </dest>
        </dests>
      </data>
    </WriteDests>
  </query>
</term>}
