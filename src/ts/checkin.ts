include(ts/macro.ts)
include(ts/sirena_exchange_macro.ts)

# meta: suite checkin


### test 1 - เฅฃจแโเ ๆจ๏ ฎคญฎฃฎ ฏ แแ ฆจเ  ญ  ฎคญฎฌ แฅฃฌฅญโฅ
#########################################################################################

$(init)
$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

$(PREPARE_FLIGHT_1PAX_1SEG  103    )

$(sql "update TRIP_SETS set PIECE_CONCEPT=1")
$(sql "update DESKS set VERSION='201707-0195750'")

$(settcl SIRENA_HOST localhost)
$(settcl SIRENA_PORT 8008)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_pax_id $(get point_dep)  ))

$(OPEN_CHECKIN $(get point_dep))

$(SAVE_ET_DISP $(get point_dep) 2981212121212  )


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
          <airp_dep></airp_dep>
          <airp_arv></airp_arv>
          <class></class>
          <status>K</status>
          <wl_type/>
          <mark_flight>
            <airline></airline>
            <flt_no>103</flt_no>
            <suffix/>
            <scd>$(date_format %d.%m.%Y) 00:00:00</scd>
            <airp_dep></airp_dep>
            <pr_mark_norms>0</pr_mark_norms>
          </mark_flight>
          <passengers>
            <pax>
              <pax_id>$(get pax_id)</pax_id>
              <surname></surname>
              <name></name>
              <pers_type></pers_type>
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
                <surname></surname>
                <first_name></first_name>
              </document>
              <doco/>
              <addresses/>
              <subclass></subclass>
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
$(TKCREQ_ET_COS UTDC UTET $(last_edifact_ref)  2981212121212 1 CK)
<<
$(TKCRES_ET_COS UTET UTDC $(last_edifact_ref) 2981212121212 1 CK)


$(http_forecast content=$(get_svc_availability_resp))

$(KICK_IN_SILENT)

>> lines=auto
<query>
  <svc_availability show_brand_info=\"true\" show_all_svc=\"true\" show_free_carry_on_norm=\"true\">
    <passenger id=\"...\" surname=\"\" name=\"\" category=\"ADT\" birthdate=\"1976-05-01\" sex=\"male\">
      <document number=\"7774441110\" country=\"RUS\"/>
      <segment id=\"0\" company=\"UT\" flight=\"103\" operating_company=\"UT\" operating_flight=\"103\" departure=\"DME\" arrival=\"LED\" departure_time=\"xxxx-xx-xxT10:15:00\" arrival_time=\"xxxx-xx-xxT12:00:00\" equipment=\"xxx\" subclass=\"Y\">
        <ticket number=\"2981212121212\" coupon_num=\"1\" display_id=\"1\"/>
        <recloc crs=\"1H\">09T1B3</recloc>
        <recloc crs=\"UT\">0840Z6</recloc>
      </segment>
    </passenger>
    <display id=\"1\">UNB+IATA:1+UTET+UTDC+xxxxxx:xxxx+xxxxxxxxxx0001+++T'UNH+1+TKCRES:06:1:IA+xxxxxxxxxx'MSG+:131+3'TIF++'TAI+0162'RCI+:G4LK6W:1'MON+B:20.00:USD+T:20.00:USD'FOP+CA:3'PTK+++$(ddmmyy)+++:US'ODI+MOW+LED'ORG+:MOW++IAH++A+US+D80D1BWO'EQN+1:TD'TXD+700+0.00:::US'IFT+4:15:1+ /FC 20DEC MOW UT SGC10.00YINF UT MOW10.00YINF NUC20.00END'IFT+4:5+00001230161213'IFT+4:10+REFUNDABLE'IFT+4:39+HOUSTON+UNITED AIRLINES INC'TKT+2981212121212:T:1:3'CPN+1:I'TVL+$(ddmmyy):2205+DME+LED++103:Y+J'RPI++NS'PTS++YINF'UNT+19+1'UNZ+1+xxxxxxxxxx0001'</display>
  </svc_availability>
</query>


>> lines=auto
    <kick req_ctxt_id...

!!
$(lastRedisplay)

$(set grp_id $(get_single_grp_id $(get point_dep)  ))
$(set tid $(get_single_tid $(get point_dep)  ))

$(sql "insert into TRIP_BT(POINT_ID, TAG_TYPE) values($(get point_dep), '')")


# คฎก ขซฅญจฅ ก ฃ ฆ  c ฎ่จกชฎฉ

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
          <airp_dep></airp_dep>
          <airp_arv></airp_arv>
          <class></class>
          <grp_id>$(get grp_id)</grp_id>
          <tid>$(get tid)</tid>
          <passengers>
            <pax>
              <pax_id>$(last_generated_pax_id)</pax_id>
              <surname></surname>
              <name></name>
              <pers_type></pers_type>
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
                <surname></surname>
                <first_name></first_name>
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
              <subclass></subclass>
              <bag_pool_num>1</bag_pool_num>
              <subclass></subclass>
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
          <airline></airline>
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
  <svc_payment_status show_free_carry_on_norm=\"true\" set_pupil=\"true\">
    <passenger id=\"...\" surname=\"\" name=\"\" category=\"ADT\" birthdate=\"1976-05-01\" sex=\"male\">
      <document number=\"7774441110\" country=\"RUS\"/>
      <segment id=\"0\" company=\"UT\" flight=\"103\" operating_company=\"UT\" operating_flight=\"103\" departure=\"DME\" arrival=\"LED\" departure_time=\"xxxx-xx-xxTxx:xx:xx\" arrival_time=\"xxxx-xx-xxTxx:xx:xx\" equipment=\"xxx\" subclass=\"Y\">
        <ticket number=\"2981212121212\" coupon_num=\"1\" display_id=\"1\"/>
        <recloc crs=\"1H\">09T1B3</recloc>
        <recloc crs=\"UT\">0840Z6</recloc>
      </segment>
    </passenger>
    <display id=\"1\">UNB+IATA:1+UTET+UTDC+xxxxxx:xxxx+xxxxxxxxxx0001+++T'UNH+1+TKCRES:06:1:IA+xxxxxxxxxx'MSG+:131+3'TIF++'TAI+0162'RCI+:G4LK6W:1'MON+B:20.00:USD+T:20.00:USD'FOP+CA:3'PTK+++$(ddmmyy)+++:US'ODI+MOW+LED'ORG+:MOW++IAH++A+US+D80D1BWO'EQN+1:TD'TXD+700+0.00:::US'IFT+4:15:1+ /FC 20DEC MOW UT SGC10.00YINF UT MOW10.00YINF NUC20.00END'IFT+4:5+00001230161213'IFT+4:10+REFUNDABLE'IFT+4:39+HOUSTON+UNITED AIRLINES INC'TKT+2981212121212:T:1:3'CPN+1:I'TVL+$(ddmmyy):2205+DME+LED++103:Y+J'RPI++NS'PTS++YINF'UNT+19+1'UNZ+1+xxxxxxxxxx0001'</display>
    <svc passenger-id=\"...\" segment-id=\"0\" company=\"UT\" service_type=\"C\" rfisc=\"0L1\" rfic=\"C\" emd_type=\"EMD-A\">
      <name language=\"en\">FISHING EQUIPMENT UPTO44LB20KG</name>
      <name language=\"ru\">   44 20</name>
    </svc>
  </svc_payment_status>
</query>


$(KICK_IN)

>> lines=auto
    <command>
      <user_error lexema_id='MSG.CHECKIN.UNABLE_CALC_PAID_BAG_TRY_RE_CHECKIN' code='0'>ฅขฎงฌฎฆญฎ ฏเฎจงขฅแโจ เ แ็ฅโ ฎฏซ ็จข ฅฌฎฃฎ ก ฃ ฆ . ฎฏเฎกใฉโฅ ฏฅเฅเฅฃจแโเจเฎข โ์ ฏ แแ ฆจเฎข</user_error>
    </command>

# คฎก ขซฅญจฅ ก ฃ ฆ 

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
          <airp_dep></airp_dep>
          <airp_arv></airp_arv>
          <class></class>
          <grp_id>$(get grp_id)</grp_id>
          <tid>$(get tid)</tid>
          <passengers>
            <pax>
              <pax_id>$(last_generated_pax_id)</pax_id>
              <surname></surname>
              <name></name>
              <pers_type></pers_type>
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
                <surname></surname>
                <first_name></first_name>
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
              <subclass></subclass>
              <bag_pool_num>1</bag_pool_num>
              <subclass></subclass>
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
          <airline></airline>
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
  <svc_payment_status show_free_carry_on_norm=\"true\" set_pupil=\"true\">
    <passenger id=\"...\" surname=\"\" name=\"\" category=\"ADT\" birthdate=\"1976-05-01\" sex=\"male\">
      <document number=\"7774441110\" country=\"RUS\"/>
      <segment id=\"0\" company=\"UT\" flight=\"103\" operating_company=\"UT\" operating_flight=\"103\" departure=\"DME\" arrival=\"LED\" departure_time=\"xxxx-xx-xxTxx:xx:xx\" arrival_time=\"xxxx-xx-xxTxx:xx:xx\" equipment=\"xxx\" subclass=\"Y\">
        <ticket number=\"2981212121212\" coupon_num=\"1\" display_id=\"1\"/>
        <recloc crs=\"1H\">09T1B3</recloc>
        <recloc crs=\"UT\">0840Z6</recloc>
      </segment>
    </passenger>
    <display id=\"1\">UNB+IATA:1+UTET+UTDC+xxxxxx:xxxx+xxxxxxxxxx0001+++T'UNH+1+TKCRES:06:1:IA+xxxxxxxxxx'MSG+:131+3'TIF++'TAI+0162'RCI+:G4LK6W:1'MON+B:20.00:USD+T:20.00:USD'FOP+CA:3'PTK+++$(ddmmyy)+++:US'ODI+MOW+LED'ORG+:MOW++IAH++A+US+D80D1BWO'EQN+1:TD'TXD+700+0.00:::US'IFT+4:15:1+ /FC 20DEC MOW UT SGC10.00YINF UT MOW10.00YINF NUC20.00END'IFT+4:5+00001230161213'IFT+4:10+REFUNDABLE'IFT+4:39+HOUSTON+UNITED AIRLINES INC'TKT+2981212121212:T:1:3'CPN+1:I'TVL+$(ddmmyy):2205+DME+LED++103:Y+J'RPI++NS'PTS++YINF'UNT+19+1'UNZ+1+xxxxxxxxxx0001'</display>
    <svc passenger-id=\"...\" segment-id=\"0\" company=\"UT\" service_type=\"C\" rfisc=\"0L1\" rfic=\"C\" emd_type=\"EMD-A\">
      <name language=\"en\">FISHING EQUIPMENT UPTO44LB20KG</name>
      <name language=\"ru\">   44 20</name>
    </svc>
  </svc_payment_status>
</query>


$(KICK_IN)

>> lines=auto
    <paid_rfiscs>
      <item>
        <rfisc>0L1</rfisc>
        <service_type>C</service_type>
        <airline></airline>
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
### test 2 - เฅฃจแโเ ๆจ๏ ฃเใฏฏ๋ จง คขใๅ ฏ แแ ฆจเฎข ญ  ฎคญฎฌ แฅฃฌฅญโฅ
#########################################################################################

$(init)
$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

$(PREPARE_FLIGHT_2PAXES_1SEG  103      )

$(dump_table CRS_PAX)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(set pax1_id $(get_pax_id $(get point_dep)  ))
$(set pax2_id $(get_pax_id $(get point_dep)  ))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2981212121212  )
$(SAVE_ET_DISP $(get point_dep) 2981212121213  )


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
          <airp_dep></airp_dep>
          <airp_arv></airp_arv>
          <class></class>
          <status>K</status>
          <wl_type/>
          <mark_flight>
            <airline></airline>
            <flt_no>103</flt_no>
            <suffix/>
            <scd>$(date_format %d.%m.%Y) 00:00:00</scd>
            <airp_dep></airp_dep>
            <pr_mark_norms>0</pr_mark_norms>
          </mark_flight>
          <passengers>
            <pax>
              <pax_id>$(get pax1_id)</pax_id>
              <surname></surname>
              <name></name>
              <pers_type></pers_type>
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
                <surname></surname>
                <first_name></first_name>
              </document>
              <doco/>
              <addresses/>
              <subclass></subclass>
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
              <surname></surname>
              <name></name>
              <pers_type></pers_type>
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
                <gender></gender>
                <surname></surname>
                <first_name></first_name>
              </document>
              <doco/>
              <addresses/>
              <subclass></subclass>
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
$(TKCREQ_ET_COS UTDC UTET $(last_edifact_ref 1)  2981212121212 1 CK)
>>
$(TKCREQ_ET_COS UTDC UTET $(last_edifact_ref 0)  2981212121213 1 CK)

<<
$(TKCRES_ET_COS UTET UTDC $(last_edifact_ref 1) 2981212121212 1 CK)
<<
$(TKCRES_ET_COS UTET UTDC $(last_edifact_ref 0) 2981212121213 1 CK)



$(KICK_IN)

>> lines=auto
    <segments>
      <segment>
        <tripheader>
          <flight>103 </flight>
          <flight_short>...
          <airline></airline>
          <aircode>298</aircode>
          <flt_no>103</flt_no>
          <suffix/>
          <airp></airp>
          <scd_out_local>$(date_format %d.%m.%Y) 10:15:00</scd_out_local>
          <pr_etl_only>0</pr_etl_only>
          <pr_etstatus>0</pr_etstatus>
          <pr_no_ticket_check>0</pr_no_ticket_check>
          <pr_auto_pt_print>0</pr_auto_pt_print>
          <pr_auto_pt_print_reseat>0</pr_auto_pt_print_reseat>
          <use_jmp>0</use_jmp>
          <pr_payment_at_desk>0</pr_payment_at_desk>
        </tripheader>
        <tripdata>
          <airps>
            <airp>
              <point_id>...
              <airp_code></airp_code>
              <city_code></city_code>
              <target_view>- ()</target_view>
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
              <code></code>
              <class_view></class_view>
              <cfg>...
            </class>
          </classes>
          <gates/>
          <halls>
            <hall>
              <id>1</id>
              <name> ซ 1</name>
            </hall>
            <hall>
              <id>0</id>
              <name>เ.</name>
            </hall>
            <hall>
              <id>1141</id>
              <name>VIP</name>
            </hall>
            <hall>
              <id>1439</id>
              <name> ข. ขฎชง.</name>
            </hall>
            <hall>
              <id>39706</id>
              <name>แใฏฅเ ง ซ</name>
            </hall>
          </halls>
          <mark_flights>
            <flight>
              <airline></airline>
              <flt_no>103</flt_no>
              <suffix/>
              <scd>$(date_format %d.%m.%Y) 10:15:00</scd>
              <airp_dep></airp_dep>
              <pr_mark_norms>0</pr_mark_norms>
            </flight>
          </mark_flights>
        </tripdata>
        <grp_id>...
        <point_dep>$(get point_dep)</point_dep>
        <airp_dep></airp_dep>
        <point_arv>$(get point_arv)</point_arv>
        <airp_arv></airp_arv>
        <class></class>
        <status>K</status>
        <bag_refuse/>
        <bag_types_id>...
        <piece_concept>0</piece_concept>
        <tid>...
        <show_ticket_norms>1</show_ticket_norms>
        <show_wt_norms>1</show_wt_norms>
        <city_arv></city_arv>
        <mark_flight>
          <airline></airline>
          <flt_no>103</flt_no>
          <suffix/>
          <scd>$(date_format %d.%m.%Y) 00:00:00</scd>
          <airp_dep></airp_dep>
          <pr_mark_norms>0</pr_mark_norms>
        </mark_flight>
        <passengers>
          <pax>
            <pax_id>$(get pax1_id)</pax_id>
            <surname></surname>
            <name></name>
            <pers_type></pers_type>
            <crew_type/>
            <seat_no>...
            <seat_type/>
            <seats>1</seats>
            <refuse/>
            <reg_no>1</reg_no>
            <subclass></subclass>
            <cabin_subclass></cabin_subclass>
            <cabin_class></cabin_class>
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
              <surname></surname>
              <first_name></first_name>
            </document>
            <ticket_bag_norm>ญฅโ</ticket_bag_norm>
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
            <surname></surname>
            <name></name>
            <pers_type></pers_type>
            <crew_type/>
            <seat_no>...
            <seat_type/>
            <seats>1</seats>
            <refuse/>
            <reg_no>2</reg_no>
            <subclass></subclass>
            <cabin_subclass></cabin_subclass>
            <cabin_class></cabin_class>
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
              <surname></surname>
              <first_name></first_name>
            </document>
            <ticket_bag_norm>ญฅโ</ticket_bag_norm>
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
            <class></class>
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
### test 3 - เฅฃจแโเ ๆจ๏ ฎคญฎฃฎ ฏ แแ ฆจเ  ญ  ฎคญฎฌ แฅฃฌฅญโฅ แ ฎ่จกชฎฉ ฎกฌฅญ  แ จเฅญฎฉ
#########################################################################################

$(init)
$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

$(PREPARE_FLIGHT_1PAX_1SEG  103    )

$(sql "update TRIP_SETS set PIECE_CONCEPT=1")
$(sql "update DESKS set VERSION='201707-0195750'")

$(settcl SIRENA_HOST localhost)
$(settcl SIRENA_PORT 8008)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_pax_id $(get point_dep)  ))

$(OPEN_CHECKIN $(get point_dep))

$(SAVE_ET_DISP $(get point_dep) 2981212121212  )


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
          <airp_dep></airp_dep>
          <airp_arv></airp_arv>
          <class></class>
          <status>K</status>
          <wl_type/>
          <mark_flight>
            <airline></airline>
            <flt_no>103</flt_no>
            <suffix/>
            <scd>$(date_format %d.%m.%Y) 00:00:00</scd>
            <airp_dep></airp_dep>
            <pr_mark_norms>0</pr_mark_norms>
          </mark_flight>
          <passengers>
            <pax>
              <pax_id>$(get pax_id)</pax_id>
              <surname></surname>
              <name></name>
              <pers_type></pers_type>
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
                <surname></surname>
                <first_name></first_name>
              </document>
              <doco/>
              <addresses/>
              <subclass></subclass>
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
$(TKCREQ_ET_COS UTDC UTET $(last_edifact_ref)  2981212121212 1 CK)
<<
$(TKCRES_ET_COS UTET UTDC $(last_edifact_ref) 2981212121212 1 CK)



$(http_forecast content=$(get_svc_availability_invalid_resp))

$(KICK_IN_SILENT)

>> lines=auto
<query>
  <svc_availability show_brand_info=\"true\" show_all_svc=\"true\" show_free_carry_on_norm=\"true\">
    <passenger id=\"...\" surname=\"\" name=\"\" category=\"ADT\" birthdate=\"1976-05-01\" sex=\"male\">
      <document number=\"7774441110\" country=\"RUS\"/>
      <segment id=\"0\" company=\"UT\" flight=\"103\" operating_company=\"UT\" operating_flight=\"103\" departure=\"DME\" arrival=\"LED\" departure_time=\"xxxx-xx-xxT10:15:00\" arrival_time=\"xxxx-xx-xxT12:00:00\" equipment=\"xxx\" subclass=\"Y\">
        <ticket number=\"2981212121212\" coupon_num=\"1\" display_id=\"1\"/>
        <recloc crs=\"1H\">09T1B3</recloc>
        <recloc crs=\"UT\">0840Z6</recloc>
      </segment>
    </passenger>
    <display id=\"1\">UNB+IATA:1+UTET+UTDC+xxxxxx:xxxx+xxxxxxxxxx0001+++T'UNH+1+TKCRES:06:1:IA+xxxxxxxxxx'MSG+:131+3'TIF++'TAI+0162'RCI+:G4LK6W:1'MON+B:20.00:USD+T:20.00:USD'FOP+CA:3'PTK+++$(ddmmyy)+++:US'ODI+MOW+LED'ORG+:MOW++IAH++A+US+D80D1BWO'EQN+1:TD'TXD+700+0.00:::US'IFT+4:15:1+ /FC 20DEC MOW UT SGC10.00YINF UT MOW10.00YINF NUC20.00END'IFT+4:5+00001230161213'IFT+4:10+REFUNDABLE'IFT+4:39+HOUSTON+UNITED AIRLINES INC'TKT+2981212121212:T:1:3'CPN+1:I'TVL+$(ddmmyy):2205+DME+LED++103:Y+J'RPI++NS'PTS++YINF'UNT+19+1'UNZ+1+xxxxxxxxxx0001'</display>
  </svc_availability>
</query>


>> lines=auto
    <kick req_ctxt_id...

!! capture=on
$(lastRedisplay)

>> lines=auto
    <command>
      <user_error_message lexema_id='MSG.ERROR_CHECKING_PIECE_CONCEPT_WEIGHT_CONCEPT_APPLIED' code='0'>่จกช  ฎฏเฅคฅซฅญจ๏ แจแโฅฌ๋ เ แ็ฅโ  ก ฃ ฆ . เจฌฅญฅญ  ขฅแฎข ๏ แจแโฅฌ  เ แ็ฅโ </user_error_message>
    </command>


%%
### test 4 - เฅฃจแโเ ๆจ๏ ฎคญฎฃฎ ฏ แแ ฆจเ  ญ  ฎคญฎฌ แฅฃฌฅญโฅ แ ฎ่จกชฎฉ ฎกฌฅญ  แ 
#########################################################################################

$(init)
$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

$(PREPARE_FLIGHT_1PAX_1SEG  103    )

$(sql "update DESKS set VERSION='201707-0195750'")

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_pax_id $(get point_dep)  ))

$(OPEN_CHECKIN $(get point_dep))

$(SAVE_ET_DISP $(get point_dep) 2981212121212  )


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
          <airp_dep></airp_dep>
          <airp_arv></airp_arv>
          <class></class>
          <status>K</status>
          <wl_type/>
          <mark_flight>
            <airline></airline>
            <flt_no>103</flt_no>
            <suffix/>
            <scd>$(date_format %d.%m.%Y) 00:00:00</scd>
            <airp_dep></airp_dep>
            <pr_mark_norms>0</pr_mark_norms>
          </mark_flight>
          <passengers>
            <pax>
              <pax_id>$(get pax_id)</pax_id>
              <surname></surname>
              <name></name>
              <pers_type></pers_type>
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
                <surname></surname>
                <first_name></first_name>
              </document>
              <doco/>
              <addresses/>
              <subclass></subclass>
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
$(TKCREQ_ET_COS UTDC UTET $(last_edifact_ref)  2981212121212 1 CK)
<<
$(TKCRES_ET_COS_FAKE_ERR UTET UTDC $(last_edifact_ref) 2981212121212 1 CK 401)


>> lines=auto
    <kick req_ctxt_id...

!! capture=on
$(lastRedisplay)

>> lines=auto
    <ets_error> แแ ฆจเ   ():
     ่จกช  ฏเจ จงฌฅญฅญจจ แโ โใแ  ํซ. กจซฅโ  2981212121212/1: 401.

