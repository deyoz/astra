include(ts/macro.ts)
include(ts/adm_macro.ts)
include(ts/sirena_exchange_macro.ts)
include(ts/spp/write_trips_macro.ts)
include(ts/spp/write_dests_macro.ts)
include(ts/pnl/pnl_ut_580_461.ts)
include(ts/pax/checkin_macro.ts)

# meta: suite checkin


### test 1 - ॣ������ ������ ���ᠦ�� �� ����� ᥣ����
#########################################################################################

$(init_term)
$(init_eds �� UTET UTDC)

$(PREPARE_FLIGHT_1PAX_1SEG �� 103 ��� ��� ����� ����)

$(settcl SIRENA_HOST localhost)
$(settcl SIRENA_PORT 8008)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_pax_id $(get point_dep) ����� ����))

$(CHANGE_TRIP_SETS $(get point_dep) piece_concept=1)

$(OPEN_CHECKIN $(get point_dep))

$(SAVE_ET_DISP $(get point_dep) 2981212121212 ����� ����)


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
          <airp_dep>���</airp_dep>
          <airp_arv>���</airp_arv>
          <class>�</class>
          <status>K</status>
          <wl_type/>
          <mark_flight>
            <airline>��</airline>
            <flt_no>103</flt_no>
            <suffix/>
            <scd>$(date_format %d.%m.%Y) 00:00:00</scd>
            <airp_dep>���</airp_dep>
            <pr_mark_norms>0</pr_mark_norms>
          </mark_flight>
          <passengers>
            <pax>
              <pax_id>$(get pax_id)</pax_id>
              <surname>�����</surname>
              <name>����</name>
              <pers_type>��</pers_type>
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
                <surname>�����</surname>
                <first_name>����</first_name>
              </document>
              <doco/>
              <addresses/>
              <subclass>�</subclass>
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
$(TKCREQ_ET_COS UTDC UTET $(last_edifact_ref) �� 2981212121212 1 CK)
<<
$(TKCRES_ET_COS UTET UTDC $(last_edifact_ref) 2981212121212 1 CK)


$(http_forecast content=$(get_svc_availability_resp))

$(KICK_IN_SILENT)

>> lines=auto
<query>
  <svc_availability show_brand_info=\"true\" show_all_svc=\"true\" show_free_carry_on_norm=\"true\">
    <passenger id=\"...\" surname=\"�����\" name=\"����\" category=\"ADT\" birthdate=\"1976-05-01\" sex=\"male\">
      <document number=\"7774441110\" country=\"RUS\"/>
      <segment id=\"0\" company=\"UT\" flight=\"103\" operating_company=\"UT\" operating_flight=\"103\" departure=\"DME\" arrival=\"LED\" departure_time=\"xxxx-xx-xxT10:15:00\" arrival_time=\"xxxx-xx-xxT12:00:00\" equipment=\"xxx\" subclass=\"Y\">
        <ticket number=\"2981212121212\" coupon_num=\"1\" display_id=\"1\"/>
        <recloc crs=\"1H\">09T1B3</recloc>
        <recloc crs=\"UT\">0840Z6</recloc>
      </segment>
    </passenger>
    <display id=\"1\">UNB+IATA:1+UTET+UTDC+xxxxxx:xxxx+xxxxxxxxxx0001+++T'UNH+1+TKCRES:06:1:IA+xxxxxxxxxx'MSG+:131+3'TIF+�����+����'TAI+0162'RCI+��:G4LK6W:1'MON+B:20.00:USD+T:20.00:USD'FOP+CA:3'PTK+++$(ddmmyy)+++:US'ODI+MOW+LED'ORG+��:MOW++IAH++A+US+D80D1BWO'EQN+1:TD'TXD+700+0.00:::US'IFT+4:15:1+ /FC 20DEC MOW UT SGC10.00YINF UT MOW10.00YINF NUC20.00END'IFT+4:5+00001230161213'IFT+4:10+REFUNDABLE'IFT+4:39+HOUSTON+UNITED AIRLINES INC'TKT+2981212121212:T:1:3'CPN+1:I'TVL+$(ddmmyy):2205+DME+LED+��+103:Y+J'RPI++NS'PTS++YINF'UNT+19+1'UNZ+1+xxxxxxxxxx0001'</display>
  </svc_availability>
</query>


>> lines=auto
    <kick req_ctxt_id...

!!
$(lastRedisplay)

$(set grp_id $(get_single_grp_id $(get point_dep) ����� ����))
$(set tid $(get_single_tid $(get point_dep) ����� ����))

$(prepare_bt_for_flight $(get point_dep) ����)


# ���������� ������ c �訡���

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
          <airp_dep>���</airp_dep>
          <airp_arv>���</airp_arv>
          <class>�</class>
          <grp_id>$(get grp_id)</grp_id>
          <tid>$(get tid)</tid>
          <passengers>
            <pax>
              <pax_id>$(last_generated_pax_id)</pax_id>
              <surname>�����</surname>
              <name>����</name>
              <pers_type>��</pers_type>
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
                <surname>�����</surname>
                <first_name>����</first_name>
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
              <subclass>�</subclass>
              <bag_pool_num>1</bag_pool_num>
              <subclass>�</subclass>
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
          <airline>��</airline>
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
    <passenger id=\"...\" surname=\"�����\" name=\"����\" category=\"ADT\" birthdate=\"1976-05-01\" sex=\"male\">
      <document number=\"7774441110\" country=\"RUS\"/>
      <segment id=\"0\" company=\"UT\" flight=\"103\" operating_company=\"UT\" operating_flight=\"103\" departure=\"DME\" arrival=\"LED\" departure_time=\"xxxx-xx-xxTxx:xx:xx\" arrival_time=\"xxxx-xx-xxTxx:xx:xx\" equipment=\"xxx\" subclass=\"Y\">
        <ticket number=\"2981212121212\" coupon_num=\"1\" display_id=\"1\"/>
        <recloc crs=\"1H\">09T1B3</recloc>
        <recloc crs=\"UT\">0840Z6</recloc>
      </segment>
    </passenger>
    <display id=\"1\">UNB+IATA:1+UTET+UTDC+xxxxxx:xxxx+xxxxxxxxxx0001+++T'UNH+1+TKCRES:06:1:IA+xxxxxxxxxx'MSG+:131+3'TIF+�����+����'TAI+0162'RCI+��:G4LK6W:1'MON+B:20.00:USD+T:20.00:USD'FOP+CA:3'PTK+++$(ddmmyy)+++:US'ODI+MOW+LED'ORG+��:MOW++IAH++A+US+D80D1BWO'EQN+1:TD'TXD+700+0.00:::US'IFT+4:15:1+ /FC 20DEC MOW UT SGC10.00YINF UT MOW10.00YINF NUC20.00END'IFT+4:5+00001230161213'IFT+4:10+REFUNDABLE'IFT+4:39+HOUSTON+UNITED AIRLINES INC'TKT+2981212121212:T:1:3'CPN+1:I'TVL+$(ddmmyy):2205+DME+LED+��+103:Y+J'RPI++NS'PTS++YINF'UNT+19+1'UNZ+1+xxxxxxxxxx0001'</display>
    <svc passenger-id=\"...\" segment-id=\"0\" company=\"UT\" service_type=\"C\" rfisc=\"0L1\" rfic=\"C\" emd_type=\"EMD-A\">
      <name language=\"en\">FISHING EQUIPMENT UPTO44LB20KG</name>
      <name language=\"ru\">���������� ������ �� 44� 20��</name>
    </svc>
  </svc_payment_status>
</query>


$(KICK_IN)

>> lines=auto
    <command>
      <user_error lexema_id='MSG.CHECKIN.UNABLE_CALC_PAID_BAG_TRY_RE_CHECKIN' code='0'>���������� �ந����� ���� ����稢������ ������. ���஡�� ���ॣ����஢��� ���ᠦ�஢</user_error>
    </command>

# ���������� ������

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
          <airp_dep>���</airp_dep>
          <airp_arv>���</airp_arv>
          <class>�</class>
          <grp_id>$(get grp_id)</grp_id>
          <tid>$(get tid)</tid>
          <passengers>
            <pax>
              <pax_id>$(last_generated_pax_id)</pax_id>
              <surname>�����</surname>
              <name>����</name>
              <pers_type>��</pers_type>
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
                <surname>�����</surname>
                <first_name>����</first_name>
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
              <subclass>�</subclass>
              <bag_pool_num>1</bag_pool_num>
              <subclass>�</subclass>
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
          <airline>��</airline>
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
    <passenger id=\"...\" surname=\"�����\" name=\"����\" category=\"ADT\" birthdate=\"1976-05-01\" sex=\"male\">
      <document number=\"7774441110\" country=\"RUS\"/>
      <segment id=\"0\" company=\"UT\" flight=\"103\" operating_company=\"UT\" operating_flight=\"103\" departure=\"DME\" arrival=\"LED\" departure_time=\"xxxx-xx-xxTxx:xx:xx\" arrival_time=\"xxxx-xx-xxTxx:xx:xx\" equipment=\"xxx\" subclass=\"Y\">
        <ticket number=\"2981212121212\" coupon_num=\"1\" display_id=\"1\"/>
        <recloc crs=\"1H\">09T1B3</recloc>
        <recloc crs=\"UT\">0840Z6</recloc>
      </segment>
    </passenger>
    <display id=\"1\">UNB+IATA:1+UTET+UTDC+xxxxxx:xxxx+xxxxxxxxxx0001+++T'UNH+1+TKCRES:06:1:IA+xxxxxxxxxx'MSG+:131+3'TIF+�����+����'TAI+0162'RCI+��:G4LK6W:1'MON+B:20.00:USD+T:20.00:USD'FOP+CA:3'PTK+++$(ddmmyy)+++:US'ODI+MOW+LED'ORG+��:MOW++IAH++A+US+D80D1BWO'EQN+1:TD'TXD+700+0.00:::US'IFT+4:15:1+ /FC 20DEC MOW UT SGC10.00YINF UT MOW10.00YINF NUC20.00END'IFT+4:5+00001230161213'IFT+4:10+REFUNDABLE'IFT+4:39+HOUSTON+UNITED AIRLINES INC'TKT+2981212121212:T:1:3'CPN+1:I'TVL+$(ddmmyy):2205+DME+LED+��+103:Y+J'RPI++NS'PTS++YINF'UNT+19+1'UNZ+1+xxxxxxxxxx0001'</display>
    <svc passenger-id=\"...\" segment-id=\"0\" company=\"UT\" service_type=\"C\" rfisc=\"0L1\" rfic=\"C\" emd_type=\"EMD-A\">
      <name language=\"en\">FISHING EQUIPMENT UPTO44LB20KG</name>
      <name language=\"ru\">���������� ������ �� 44� 20��</name>
    </svc>
  </svc_payment_status>
</query>


$(KICK_IN)

>> lines=auto
    <paid_rfiscs>
      <item>
        <rfisc>0L1</rfisc>
        <service_type>C</service_type>
        <airline>��</airline>
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
### test 2 - ॣ������ ��㯯� �� ���� ���ᠦ�஢ �� ����� ᥣ����
#########################################################################################

$(init_term 201509-0173355)

$(init_eds �� UTET UTDC)

$(PREPARE_FLIGHT_2PAXES_1SEG �� 103 ��� ��� ����� ������� ������ ����)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(set pax1_id $(get_pax_id $(get point_dep) ����� �������))
$(set pax2_id $(get_pax_id $(get point_dep) ������ ����))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2981212121212 ����� �������)
$(SAVE_ET_DISP $(get point_dep) 2981212121213 ������ ����)


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
          <airp_dep>���</airp_dep>
          <airp_arv>���</airp_arv>
          <class>�</class>
          <status>K</status>
          <wl_type/>
          <mark_flight>
            <airline>��</airline>
            <flt_no>103</flt_no>
            <suffix/>
            <scd>$(date_format %d.%m.%Y) 00:00:00</scd>
            <airp_dep>���</airp_dep>
            <pr_mark_norms>0</pr_mark_norms>
          </mark_flight>
          <passengers>
            <pax>
              <pax_id>$(get pax1_id)</pax_id>
              <surname>�����</surname>
              <name>����</name>
              <pers_type>��</pers_type>
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
                <surname>�����</surname>
                <first_name>����</first_name>
              </document>
              <doco/>
              <addresses/>
              <subclass>�</subclass>
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
              <surname>������</surname>
              <name>����</name>
              <pers_type>��</pers_type>
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
                <gender>�</gender>
                <surname>������</surname>
                <first_name>����</first_name>
              </document>
              <doco/>
              <addresses/>
              <subclass>�</subclass>
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
$(TKCREQ_ET_COS UTDC UTET $(last_edifact_ref 1) �� 2981212121212 1 CK)
>>
$(TKCREQ_ET_COS UTDC UTET $(last_edifact_ref 0) �� 2981212121213 1 CK)

<<
$(TKCRES_ET_COS UTET UTDC $(last_edifact_ref 1) 2981212121212 1 CK)
<<
$(TKCRES_ET_COS UTET UTDC $(last_edifact_ref 0) 2981212121213 1 CK)



$(KICK_IN)

>> lines=auto
    <segments>
      <segment>
        <tripheader>
          <flight>��103 ���</flight>
          <flight_short>...
          <airline>��</airline>
          <aircode>298</aircode>
          <flt_no>103</flt_no>
          <suffix/>
          <airp>���</airp>
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
              <airp_code>���</airp_code>
              <city_code>���</city_code>
              <target_view>�����-��������� (���)</target_view>
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
              <code>�</code>
              <class_view>������</class_view>
              <cfg>...
            </class>
          </classes>
          <gates/>
          <halls>
            <hall>
              <id>1</id>
              <name>��� 1</name>
            </hall>
            <hall>
              <id>0</id>
              <name>��.</name>
            </hall>
            <hall>
              <id>1141</id>
              <name>VIP</name>
            </hall>
            <hall>
              <id>1439</id>
              <name>���. ����.</name>
            </hall>
            <hall>
              <id>39706</id>
              <name>�㯥� ���</name>
            </hall>
          </halls>
          <mark_flights>
            <flight>
              <airline>��</airline>
              <flt_no>103</flt_no>
              <suffix/>
              <scd>$(date_format %d.%m.%Y) 10:15:00</scd>
              <airp_dep>���</airp_dep>
              <pr_mark_norms>0</pr_mark_norms>
            </flight>
          </mark_flights>
        </tripdata>
        <grp_id>...
        <point_dep>$(get point_dep)</point_dep>
        <airp_dep>���</airp_dep>
        <point_arv>$(get point_arv)</point_arv>
        <airp_arv>���</airp_arv>
        <class>�</class>
        <status>K</status>
        <bag_refuse/>
        <bag_types_id>...
        <piece_concept>0</piece_concept>
        <tid>...
        <show_ticket_norms>1</show_ticket_norms>
        <show_wt_norms>1</show_wt_norms>
        <city_arv>���</city_arv>
        <mark_flight>
          <airline>��</airline>
          <flt_no>103</flt_no>
          <suffix/>
          <scd>$(date_format %d.%m.%Y) 00:00:00</scd>
          <airp_dep>���</airp_dep>
          <pr_mark_norms>0</pr_mark_norms>
        </mark_flight>
        <passengers>
          <pax>
            <pax_id>$(get pax1_id)</pax_id>
            <surname>�����</surname>
            <name>����</name>
            <pers_type>��</pers_type>
            <crew_type/>
            <seat_no>...
            <seat_type/>
            <seats>1</seats>
            <refuse/>
            <reg_no>1</reg_no>
            <subclass>�</subclass>
            <cabin_subclass>�</cabin_subclass>
            <cabin_class>�</cabin_class>
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
              <surname>�����</surname>
              <first_name>����</first_name>
            </document>
            <ticket_bag_norm>���</ticket_bag_norm>
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
            <surname>������</surname>
            <name>����</name>
            <pers_type>��</pers_type>
            <crew_type/>
            <seat_no>...
            <seat_type/>
            <seats>1</seats>
            <refuse/>
            <reg_no>2</reg_no>
            <subclass>�</subclass>
            <cabin_subclass>�</cabin_subclass>
            <cabin_class>�</cabin_class>
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
              <surname>������</surname>
              <first_name>����</first_name>
            </document>
            <ticket_bag_norm>���</ticket_bag_norm>
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
            <class>�</class>
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
### test 3 - ॣ������ ������ ���ᠦ�� �� ����� ᥣ���� � �訡��� ������ � ��७��
#########################################################################################

$(init_term)

$(init_eds �� UTET UTDC)

$(PREPARE_FLIGHT_1PAX_1SEG �� 103 ��� ��� ����� ����)

$(settcl SIRENA_HOST localhost)
$(settcl SIRENA_PORT 8008)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_pax_id $(get point_dep) ����� ����))

$(CHANGE_TRIP_SETS $(get point_dep) piece_concept=1)

$(OPEN_CHECKIN $(get point_dep))

$(SAVE_ET_DISP $(get point_dep) 2981212121212 ����� ����)


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
          <airp_dep>���</airp_dep>
          <airp_arv>���</airp_arv>
          <class>�</class>
          <status>K</status>
          <wl_type/>
          <mark_flight>
            <airline>��</airline>
            <flt_no>103</flt_no>
            <suffix/>
            <scd>$(date_format %d.%m.%Y) 00:00:00</scd>
            <airp_dep>���</airp_dep>
            <pr_mark_norms>0</pr_mark_norms>
          </mark_flight>
          <passengers>
            <pax>
              <pax_id>$(get pax_id)</pax_id>
              <surname>�����</surname>
              <name>����</name>
              <pers_type>��</pers_type>
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
                <surname>�����</surname>
                <first_name>����</first_name>
              </document>
              <doco/>
              <addresses/>
              <subclass>�</subclass>
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
$(TKCREQ_ET_COS UTDC UTET $(last_edifact_ref) �� 2981212121212 1 CK)
<<
$(TKCRES_ET_COS UTET UTDC $(last_edifact_ref) 2981212121212 1 CK)



$(http_forecast content=$(get_svc_availability_invalid_resp))

$(KICK_IN_SILENT)

>> lines=auto
<query>
  <svc_availability show_brand_info=\"true\" show_all_svc=\"true\" show_free_carry_on_norm=\"true\">
    <passenger id=\"...\" surname=\"�����\" name=\"����\" category=\"ADT\" birthdate=\"1976-05-01\" sex=\"male\">
      <document number=\"7774441110\" country=\"RUS\"/>
      <segment id=\"0\" company=\"UT\" flight=\"103\" operating_company=\"UT\" operating_flight=\"103\" departure=\"DME\" arrival=\"LED\" departure_time=\"xxxx-xx-xxT10:15:00\" arrival_time=\"xxxx-xx-xxT12:00:00\" equipment=\"xxx\" subclass=\"Y\">
        <ticket number=\"2981212121212\" coupon_num=\"1\" display_id=\"1\"/>
        <recloc crs=\"1H\">09T1B3</recloc>
        <recloc crs=\"UT\">0840Z6</recloc>
      </segment>
    </passenger>
    <display id=\"1\">UNB+IATA:1+UTET+UTDC+xxxxxx:xxxx+xxxxxxxxxx0001+++T'UNH+1+TKCRES:06:1:IA+xxxxxxxxxx'MSG+:131+3'TIF+�����+����'TAI+0162'RCI+��:G4LK6W:1'MON+B:20.00:USD+T:20.00:USD'FOP+CA:3'PTK+++$(ddmmyy)+++:US'ODI+MOW+LED'ORG+��:MOW++IAH++A+US+D80D1BWO'EQN+1:TD'TXD+700+0.00:::US'IFT+4:15:1+ /FC 20DEC MOW UT SGC10.00YINF UT MOW10.00YINF NUC20.00END'IFT+4:5+00001230161213'IFT+4:10+REFUNDABLE'IFT+4:39+HOUSTON+UNITED AIRLINES INC'TKT+2981212121212:T:1:3'CPN+1:I'TVL+$(ddmmyy):2205+DME+LED+��+103:Y+J'RPI++NS'PTS++YINF'UNT+19+1'UNZ+1+xxxxxxxxxx0001'</display>
  </svc_availability>
</query>


>> lines=auto
    <kick req_ctxt_id...

!! capture=on
$(lastRedisplay)

>> lines=auto
    <command>
      <user_error_message lexema_id='MSG.ERROR_CHECKING_PIECE_CONCEPT_WEIGHT_CONCEPT_APPLIED' code='0'>�訡�� ��।������ ��⥬� ���� ������. �ਬ����� ��ᮢ�� ��⥬� ����</user_error_message>
    </command>


%%
### test 4 - ॣ������ ������ ���ᠦ�� �� ����� ᥣ���� � �訡��� ������ � ���
#########################################################################################

$(init_term)

$(init_eds �� UTET UTDC)

$(PREPARE_FLIGHT_1PAX_1SEG �� 103 ��� ��� ����� ����)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_pax_id $(get point_dep) ����� ����))

$(OPEN_CHECKIN $(get point_dep))

$(SAVE_ET_DISP $(get point_dep) 2981212121212 ����� ����)


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
          <airp_dep>���</airp_dep>
          <airp_arv>���</airp_arv>
          <class>�</class>
          <status>K</status>
          <wl_type/>
          <mark_flight>
            <airline>��</airline>
            <flt_no>103</flt_no>
            <suffix/>
            <scd>$(date_format %d.%m.%Y) 00:00:00</scd>
            <airp_dep>���</airp_dep>
            <pr_mark_norms>0</pr_mark_norms>
          </mark_flight>
          <passengers>
            <pax>
              <pax_id>$(get pax_id)</pax_id>
              <surname>�����</surname>
              <name>����</name>
              <pers_type>��</pers_type>
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
                <surname>�����</surname>
                <first_name>����</first_name>
              </document>
              <doco/>
              <addresses/>
              <subclass>�</subclass>
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
$(TKCREQ_ET_COS UTDC UTET $(last_edifact_ref) �� 2981212121212 1 CK)
<<
$(TKCRES_ET_COS_FAKE_ERR UTET UTDC $(last_edifact_ref) 2981212121212 1 CK 401)



>> lines=auto
    <kick req_ctxt_id...

!! capture=on
$(lastRedisplay)

>> lines=auto
    <ets_error>���ᠦ�� ����� ���� (��):
     �訡�� �� ��������� ����� �. ����� 2981212121212/1: 401.

%%

### test 5 - �⬥�� ॣ����樨 � �࠭��஬ ������ �/��� ᪢����� ॣ����樥� (�஢�ઠ ���⪨ trfer_trips)
### ��� ���ᠦ�� ����� �� ��ࢮ� ᥣ���� ࠧ�묨 ३ᠬ�, �� ��஬ ᥣ���� �� ����� � ⮬ ��
### 1 ���� ���:
###    ॣ�����㥬 ���ᠦ�� �� ��� ᥣ���� (���������� transfer+tckin_segments),
###    ॣ�����㥬 ���ᠦ�� �� ���� ᥣ���� � �࠭��஬ ������ (���������� ⮫쪮 transfer)
###    �⬥�塞 ������
###    �⬥�塞 ��ண� - ⮫쪮 � ��� ������ ������ trfer_trips
### 2 ���� ���:
###    ॣ�����㥬 ���ᠦ�� �� ��� ᥣ���� (���������� transfer+tckin_segments),
###    ॣ�����㥬 ���ᠦ�� �� ��� ᥣ���� ��� �࠭��� ������ (���������� ⮫쪮 tckin_segments)
###    �⬥�塞 ������
###    �⬥�塞 ��ண� - ⮫쪮 � ��� ������ ������ trfer_trips
#########################################################################################

$(init_term)
$(set_user_time_type LocalAirp PIKE)

$(set today $(date_format %d.%m.%Y +0))
$(set tomor $(date_format %d.%m.%Y +1))

### ��� ३� � ��������� ����஬ � ������⮬ � ࠧ��楩 � ����
### � ������� ३� �࠭��� ��� ᪢����� ॣ������ �� ��騩 ��⨩ ३�

$(NEW_SPP_FLIGHT_REQUEST
{ $(new_spp_point �� 580 TU3 65021 ""                   ��� "$(get today) 12:00")
  $(new_spp_point_last             "$(get today) 15:00" ��� ) })

$(NEW_SPP_FLIGHT_REQUEST
{ $(new_spp_point �� 580 TU3 65021 ""                   ��� "$(get tomor) 12:00")
  $(new_spp_point_last             "$(get tomor) 15:00" ��� ) })

$(NEW_SPP_FLIGHT_REQUEST
{ $(new_spp_point �� 461 TU3 65021 ""                   ��� "$(get tomor) 16:00")
  $(new_spp_point_last             "$(get tomor) 21:20" ��� ) })


$(set point_dep1 $(get_point_dep_for_flight �� 580 "" $(yymmdd +0) ���))
$(set point_arv1 $(get_next_trip_point_id $(get point_dep1)))
$(set point_dep2 $(get_point_dep_for_flight �� 580 "" $(yymmdd +1) ���))
$(set point_arv2 $(get_next_trip_point_id $(get point_dep2)))
$(set point_dep3 $(get_point_dep_for_flight �� 461 "" $(yymmdd +1) ���))
$(set point_arv3 $(get_next_trip_point_id $(get point_dep3)))

$(PNL_UT_580 date_dep=$(ddmon +0 en))
$(PNL_UT_580 date_dep=$(ddmon +1 en))
$(PNL_UT_461 date_dep=$(ddmon +1 en))

$(set pax_id_1479_1 $(get_pax_id $(get point_dep1) KOTOVA IRINA))
$(set pax_id_1480_1 $(get_pax_id $(get point_dep1) MOTOVA IRINA))
$(set pax_id_1479_2 $(get_pax_id $(get point_dep2) KOTOVA IRINA))
$(set pax_id_1480_2 $(get_pax_id $(get point_dep2) MOTOVA IRINA))
$(set pax_id_1479_3 $(get_pax_id $(get point_dep3) KOTOVA IRINA))
$(set pax_id_1480_3 $(get_pax_id $(get point_dep3) MOTOVA IRINA))

$(deny_ets_interactive UT)

###############################
###         1 ����         ###
###############################

$(NEW_TCHECKIN_REQUEST capture=on lang=EN hall=1
$(TRANSFER_SEGMENT �� 461 "" $(dd +1) ��� ���)
{$(NEW_CHECKIN_SEGMENT $(get point_dep1) $(get point_arv1) ��� ���
{<passengers>
  <pax>
$(NEW_CHECKIN_2982410821479 $(get pax_id_1479_1) 1 Y)
  </pax>
</passengers>})
$(NEW_CHECKIN_SEGMENT $(get point_dep3) $(get point_arv3) ��� ���
{<passengers>
  <pax>
$(NEW_CHECKIN_2982410821479 $(get pax_id_1479_3) 2)
  </pax>
</passengers>})})

>> mode=regex
.*
            <reg_no>1</reg_no>.*
            <reg_no>1</reg_no>.*

### ��஬� ���ᠦ��� ��ଫ塞 �࠭��� ������ ��� ᪢����� ॣ����樨

$(NEW_TCHECKIN_REQUEST capture=on lang=EN hall=1
$(TRANSFER_SEGMENT �� 461 "" $(dd +1) ��� ���)
{$(NEW_CHECKIN_SEGMENT $(get point_dep2) $(get point_arv2) ��� ���
{<passengers>
  <pax>
$(NEW_CHECKIN_2982410821480 $(get pax_id_1480_2) 1 Y)
  </pax>
</passengers>})})

>> mode=regex
.*
            <reg_no>1</reg_no>.*


$(set grp_id_1479_1 $(get_single_grp_id $(get pax_id_1479_1)))
$(set grp_id_1480_2 $(get_single_grp_id $(get pax_id_1480_2)))
$(set grp_id_1479_3 $(get_single_grp_id $(get pax_id_1479_3)))

$(set pax_tid_1479_1 $(get_single_pax_tid $(get pax_id_1479_1)))
$(set pax_tid_1480_2 $(get_single_pax_tid $(get pax_id_1480_2)))
$(set pax_tid_1479_3 $(get_single_pax_tid $(get pax_id_1479_3)))

$(CHANGE_TCHECKIN_REQUEST capture=on lang=EN hall=1
{$(CHANGE_CHECKIN_SEGMENT $(get point_dep1) $(get point_arv1) ��� ���
                          $(get grp_id_1479_1) $(get_single_grp_tid $(get pax_id_1479_1))
{<passengers>
  <pax>
$(CHANGE_CHECKIN_2982410821479 $(get pax_id_1479_1) $(get pax_tid_1479_1) 1 refuse=�)
  </pax>
</passengers>})
$(CHANGE_CHECKIN_SEGMENT $(get point_dep3) $(get point_arv3) ��� ���
                         $(get grp_id_1479_3) $(get_single_grp_tid $(get pax_id_1479_3))
{<passengers>
  <pax>
$(CHANGE_CHECKIN_2982410821479 $(get pax_id_1479_3) $(get pax_tid_1479_3) 2 refuse=�)
  </pax>
</passengers>})}
)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <segments/>
  </answer>
</term>


??
$(dump_table trfer_trips display=on)

>> lines=auto
------------------- END trfer_trips DUMP COUNT=1 -------------------


$(CHANGE_TCHECKIN_REQUEST capture=on lang=EN hall=1
{$(CHANGE_CHECKIN_SEGMENT $(get point_dep2) $(get point_arv2) ��� ���
                          $(get grp_id_1480_2) $(get_single_grp_tid $(get pax_id_1480_2))
{<passengers>
  <pax>
$(CHANGE_CHECKIN_2982410821480 $(get pax_id_1480_2) $(get pax_tid_1480_2) 1 refuse=�)
  </pax>
</passengers>})}
)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <segments/>
  </answer>
</term>


### ࠧॣ����஢��� �� �訡�� ����� ���

??
$(dump_table trfer_trips display=on)

>> lines=auto
------------------- END trfer_trips DUMP COUNT=0 -------------------


###############################
###         2 ����         ###
###############################

$(NEW_TCHECKIN_REQUEST capture=on lang=EN hall=1
$(TRANSFER_SEGMENT �� 461 "" $(dd +1) ��� ���)
{$(NEW_CHECKIN_SEGMENT $(get point_dep1) $(get point_arv1) ��� ���
{<passengers>
  <pax>
$(NEW_CHECKIN_2982410821479 $(get pax_id_1479_1) 1 Y)
  </pax>
</passengers>})
$(NEW_CHECKIN_SEGMENT $(get point_dep3) $(get point_arv3) ��� ���
{<passengers>
  <pax>
$(NEW_CHECKIN_2982410821479 $(get pax_id_1479_3) 2)
  </pax>
</passengers>})})

>> mode=regex
.*
            <reg_no>1</reg_no>.*
            <reg_no>1</reg_no>.*

### ��஬� ���ᠦ��� ��ଫ塞 ᪢����� ॣ������ ��� �࠭��� ������

$(NEW_TCHECKIN_REQUEST capture=on lang=EN hall=1
""
{$(NEW_CHECKIN_SEGMENT $(get point_dep2) $(get point_arv2) ��� ���
{<passengers>
  <pax>
$(NEW_CHECKIN_2982410821480 $(get pax_id_1480_2) 1)
  </pax>
</passengers>})
$(NEW_CHECKIN_SEGMENT $(get point_dep3) $(get point_arv3) ��� ���
{<passengers>
  <pax>
$(NEW_CHECKIN_2982410821480 $(get pax_id_1480_3) 2)
  </pax>
</passengers>})})

>> mode=regex
.*
            <reg_no>1</reg_no>.*
            <reg_no>2</reg_no>.*

$(set grp_id_1479_1 $(get_single_grp_id $(get pax_id_1479_1)))
$(set grp_id_1480_2 $(get_single_grp_id $(get pax_id_1480_2)))
$(set grp_id_1479_3 $(get_single_grp_id $(get pax_id_1479_3)))
$(set grp_id_1480_3 $(get_single_grp_id $(get pax_id_1480_3)))

$(set pax_tid_1479_1 $(get_single_pax_tid $(get pax_id_1479_1)))
$(set pax_tid_1480_2 $(get_single_pax_tid $(get pax_id_1480_2)))
$(set pax_tid_1479_3 $(get_single_pax_tid $(get pax_id_1479_3)))
$(set pax_tid_1480_3 $(get_single_pax_tid $(get pax_id_1480_3)))

$(CHANGE_TCHECKIN_REQUEST capture=on lang=EN hall=1
{$(CHANGE_CHECKIN_SEGMENT $(get point_dep1) $(get point_arv1) ��� ���
                          $(get grp_id_1479_1) $(get_single_grp_tid $(get pax_id_1479_1))
{<passengers>
  <pax>
$(CHANGE_CHECKIN_2982410821479 $(get pax_id_1479_1) $(get pax_tid_1479_1) 1 refuse=�)
  </pax>
</passengers>})
$(CHANGE_CHECKIN_SEGMENT $(get point_dep3) $(get point_arv3) ��� ���
                         $(get grp_id_1479_3) $(get_single_grp_tid $(get pax_id_1479_3))
{<passengers>
  <pax>
$(CHANGE_CHECKIN_2982410821479 $(get pax_id_1479_3) $(get pax_tid_1479_3) 2 refuse=�)
  </pax>
</passengers>})}
)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <segments/>
  </answer>
</term>


??
$(dump_table trfer_trips display=on)

>> lines=auto
------------------- END trfer_trips DUMP COUNT=1 -------------------


$(CHANGE_TCHECKIN_REQUEST capture=on lang=EN hall=1
{$(CHANGE_CHECKIN_SEGMENT $(get point_dep2) $(get point_arv2) ��� ���
                          $(get grp_id_1480_2) $(get_single_grp_tid $(get pax_id_1480_2))
{<passengers>
  <pax>
$(CHANGE_CHECKIN_2982410821480 $(get pax_id_1480_2) $(get pax_tid_1480_2) 1 refuse=�)
  </pax>
</passengers>})
$(CHANGE_CHECKIN_SEGMENT $(get point_dep3) $(get point_arv3) ��� ���
                         $(get grp_id_1480_3) $(get_single_grp_tid $(get pax_id_1480_3))
{<passengers>
  <pax>
$(CHANGE_CHECKIN_2982410821480 $(get pax_id_1480_3) $(get pax_tid_1480_3) 2 refuse=�)
  </pax>
</passengers>})}
)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <segments/>
  </answer>
</term>


### ࠧॣ����஢��� �� �訡�� ����� ���

??
$(dump_table trfer_trips display=on)

>> lines=auto
------------------- END trfer_trips DUMP COUNT=0 -------------------

%%


$(defmacro PAX_LIST_2CREWMEN
  user_id
  point_dep
  point_arv
  surname1
  grp_id1
  pax_id1
  surname2
  grp_id2
  pax_id2
{

!! capture=on
<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
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
      <tripcounters>
        <fields>
          <class/>
        </fields>
      </tripcounters>
      <LoadForm/>
    </PaxList>
  </query>
</term>

>> lines=auto
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <tripcounters>
      <fields>
        <field>title</field>
        <field>class</field>
        <field>cfg</field>
        <field>crs_ok</field>
        <field>crs_tranzit</field>
        <field>seats</field>
        <field>adult_m</field>
        <field>adult_f</field>
        <field>child</field>
        <field>baby</field>
        <field>rk_weight</field>
        <field>bag_amount</field>
        <field>bag_weight</field>
        <field>excess</field>
        <field>load</field>
      </fields>
      <rows>
        <row>
          <title>�ᥣ�</title>
        </row>
        <row>
          <seats>2</seats>
          <adult_m>2</adult_m>
          <class> </class>
          <title>������</title>
        </row>
      </rows>
    </tripcounters>
    <flight>6�776/$(date_format %d +1)... ���</flight>
    <passengers>
      <pax>
        <pax_id>$(pax_id2)</pax_id>
        <reg_no>-2</reg_no>
        <surname>$(surname2)</surname>
        <name>DIMA</name>
        <airp_arv>���</airp_arv>
        <class> </class>
        <subclass/>
        <seat_no/>
        <document>99988887774 RUS</document>
        <grp_id>$(grp_id2)</grp_id>
        <cl_grp_id>1000000000</cl_grp_id>
        <hall_id>-1</hall_id>
        <point_arv>$(point_arv)</point_arv>
        <user_id>$(user_id)</user_id>
        <client_type_id>4</client_type_id>
        <status_id>4</status_id>
      </pax>
      <pax>
        <pax_id>$(pax_id1)</pax_id>
        <reg_no>-1</reg_no>
        <surname>$(surname1)</surname>
        <name>IVAN ROMANOVIC</name>
        <airp_arv>���</airp_arv>
        <class> </class>
        <subclass/>
        <seat_no/>
        <document>1234567891 RUS</document>
        <grp_id>$(grp_id1)</grp_id>
        <cl_grp_id>1000000000</cl_grp_id>
        <hall_id>-1</hall_id>
        <point_arv>$(point_arv)</point_arv>
        <user_id>$(user_id)</user_id>
        <client_type_id>4</client_type_id>
        <status_id>4</status_id>
      </pax>
    </passengers>
    <unaccomp_bag/>
    <defaults>
      <last_trfer/>
      <last_tckin_seg/>
      <bag_amount>0</bag_amount>
      <bag_weight>0</bag_weight>
      <rk_weight>0</rk_weight>
      <excess>0</excess>
      <tags/>
      <name/>
      <class>�</class>
      <brand/>
      <seats>1</seats>
      <seat_no_alarm>0</seat_no_alarm>
      <pers_type>��</pers_type>
      <document/>
      <ticket_rem/>
      <ticket_no/>
      <rems/>
      <mark_flt_str/>
      <client_type_id>0</client_type_id>
      <status_id>0</status_id>
    </defaults>

})

### test 6 - ॣ������ �����, ����ୠ� ॣ������ �����
### 1. ���砫� ॣ�����㥬 2 童�� ����� ������ ��㯯��
### 2. ��⮬ ॣ�����㥬 2 童�� ����� ࠧ�묨 ��㯯��� � ��㣨�� 䠬���ﬨ (��ॣ����஢���� ࠭�� ������ 㤠������)
#########################################################################################

$(init_term)
$(set_user_time_type LocalAirp PIKE)

$(set tomor $(date_format %d.%m.%Y +1))

$(NEW_SPP_FLIGHT_ONE_LEG 6W 776 TU3 RTW "$(get tomor) 12:00" "$(get tomor) 15:00" DME)


### ����� ��� �室��� http-����ᮢ �� ��ਤ����

$(CREATE_USER CREW6WUSER CREW6WUSER)
$(CREATE_DESK CREW6W 1)
$(ADD_HTTP_CLIENT CREWCHECKIN CREW6W CREW6WUSER CREW6W HTTPUSER yhjdx5r0)

$(set http_user_id $(get_user_id CREW6WUSER))

$(set http_heading
{POST / HTTP/1.0
Host: /
X-Real-IP: 146.120.94.7
Connection: close
Content-Length: $()
Authorization: Basic SFRUUFVTRVI6eWhqZHg1cjA=
User-Agent: Meridian.DB
CLIENT-ID: CREW6W
Accept-Encoding: gzip,deflate
Content-Type: application/x-www-form-urlencoded; charset=utf-8
})

#########################################################################################
### ॣ�����㥬 ���� ������ ��㯯��

!! capture=on http_heading=$(get http_heading)
<?xml version='1.0' encoding='CP866'?>
<term>
  <query lang='RU'>
    <CREWCHECKIN>
      <FLIGHT>
        <AIRLINE>6W</AIRLINE>
        <FLT_NO>776</FLT_NO>
        <SUFFIX/>
        <SCD>$(get tomor)</SCD>
        <AIRP_DEP>RTW</AIRP_DEP>
      </FLIGHT>
      <CREW_GROUPS>
        <CREW_GROUP>
          <AIRP_ARV>DME</AIRP_ARV>
          <CREW_MEMBERS>
            <CREW_MEMBER>
                <CREW_TYPE>CR1</CREW_TYPE>
                <DUTY>PIC</DUTY>
                <ORDER>1</ORDER>
                <PERSONAL_DATA>
                    <DOCS>
                        <NO>1234567891</NO>
                        <TYPE>P</TYPE>
                        <ISSUE_COUNTRY>RUS</ISSUE_COUNTRY>
                        <NO>1234567891</NO>
                        <NATIONALITY>RUS</NATIONALITY>
                        <BIRTH_DATE>01.05.1976</BIRTH_DATE>
                        <GENDER>M</GENDER>
                        <EXPIRY_DATE>12.05.$(date_format %Y +1y)</EXPIRY_DATE>
                        <SURNAME>IVANOV</SURNAME>
                        <FIRST_NAME>IVAN</FIRST_NAME>
                        <SECOND_NAME>ROMANOVIC</SECOND_NAME>
                    </DOCS>
                    <DOCO>
                        <BIRTH_PLACE>MOSKVA RUSSIY</BIRTH_PLACE>
                        <TYPE>V</TYPE>
                        <NO>VI78787787878</NO>
                        <ISSUE_PLACE>MOSKVA TURISTKAY STRIT 25</ISSUE_PLACE>
                        <ISSUE_DATE>12.02.2014</ISSUE_DATE>
                        <EXPIRY_DATE>15.12.$(date_format %Y +1y)</EXPIRY_DATE>
                        <APPLIC_COUNTRY>USA</APPLIC_COUNTRY>
                    </DOCO>
                    <DOCA>
                        <TYPE>B</TYPE>
                        <COUNTRY>RUS</COUNTRY>
                        <ADDRESS>DUBROVCA CTRIT 25 256</ADDRESS>
                        <CITY>MOSKVA</CITY>
                        <REGION>MOSKVA</REGION>
                        <POSTAL_CODE>125373</POSTAL_CODE>
                    </DOCA>
                    <DOCA>
                        <TYPE>R</TYPE>
                        <COUNTRY>RUS</COUNTRY>
                        <ADDRESS>POPOVCA CTRIT 48</ADDRESS>
                        <CITY>MOSKVA</CITY>
                        <REGION>MOSKVA</REGION>
                        <POSTAL_CODE>266373</POSTAL_CODE>
                    </DOCA>
                    <DOCA>
                        <TYPE>D</TYPE>
                        <COUNTRY>USA</COUNTRY>
                        <ADDRESS>FIFTH AVENUE</ADDRESS>
                        <CITY>NY</CITY>
                        <REGION>NY</REGION>
                        <POSTAL_CODE>9999</POSTAL_CODE>
                    </DOCA>
                </PERSONAL_DATA>
            </CREW_MEMBER>
            <CREW_MEMBER>
                <CREW_TYPE>CR1</CREW_TYPE>
                <DUTY>PIC</DUTY>
                <ORDER>2</ORDER>
                <PERSONAL_DATA>
                    <DOCS>
                        <NO>7778889991</NO>
                        <TYPE>P</TYPE>
                        <ISSUE_COUNTRY>RUS</ISSUE_COUNTRY>
                        <NO>99988887774</NO>
                        <NATIONALITY>RUS</NATIONALITY>
                        <BIRTH_DATE>02.05.1976</BIRTH_DATE>
                        <GENDER>M</GENDER>
                        <EXPIRY_DATE>22.05.$(date_format %Y +1y)</EXPIRY_DATE>
                        <SURNAME>REPIN</SURNAME>
                        <FIRST_NAME>DIMA</FIRST_NAME>
                        <SECOND_NAME></SECOND_NAME>
                    </DOCS>
                    <DOCO>
                        <BIRTH_PLACE>MOSKVA RUSSIY</BIRTH_PLACE>
                        <TYPE>V</TYPE>
                        <NO>VI78787787878</NO>
                        <ISSUE_PLACE>MOSKVA POLKA STRIT 25</ISSUE_PLACE>
                        <ISSUE_DATE>15.02.2014</ISSUE_DATE>
                        <EXPIRY_DATE>19.12.$(date_format %Y +1y)</EXPIRY_DATE>
                        <APPLIC_COUNTRY>USA</APPLIC_COUNTRY>
                    </DOCO>
                    <DOCA>
                        <TYPE>B</TYPE>
                        <COUNTRY>RUS</COUNTRY>
                        <ADDRESS>DUBROVCA CTRIT 25 256</ADDRESS>
                        <CITY>MOSKVA</CITY>
                        <REGION>MOSKVA</REGION>
                        <POSTAL_CODE>125373</POSTAL_CODE>
                    </DOCA>
                    <DOCA>
                        <TYPE>R</TYPE>
                        <COUNTRY>RUS</COUNTRY>
                        <ADDRESS>POPOVCA CTRIT 48</ADDRESS>
                        <CITY>MOSKVA</CITY>
                        <REGION>MOSKVA</REGION>
                        <POSTAL_CODE>266373</POSTAL_CODE>
                    </DOCA>
                    <DOCA>
                        <TYPE>D</TYPE>
                        <COUNTRY>USA</COUNTRY>
                        <ADDRESS>FIFTH AVENUE</ADDRESS>
                        <CITY>NY</CITY>
                        <REGION>NY</REGION>
                        <POSTAL_CODE>9999</POSTAL_CODE>
                    </DOCA>
                </PERSONAL_DATA>
            </CREW_MEMBER>
          </CREW_MEMBERS>
        </CREW_GROUP>
      </CREW_GROUPS>
    </CREWCHECKIN>
  </query>
</term>

>> lines=auto
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <CREWCHECKIN>
      <proc_status>OK</proc_status>
    </CREWCHECKIN>
  </answer>
</term>

$(set point_dep $(get_point_dep_for_flight 6W 776 "" $(yymmdd +1) RTW))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id1 $(get_pax_id $(get point_dep) -1))
$(set pax_id2 $(get_pax_id $(get point_dep) -2))
$(set grp_id1 $(get_single_grp_id $(get pax_id1)))
$(set grp_id2 $(get_single_grp_id $(get pax_id2)))

??
$(eq $(get grp_id1) $(get grp_id2))
>>
true

$(PAX_LIST_2CREWMEN $(get http_user_id) $(get point_dep) $(get point_arv)
  IVANOV $(get grp_id1) $(get pax_id1)
  REPIN  $(get grp_id1) $(get pax_id2))

#########################################################################################
### ॣ�����㥬 ���� ࠧ�묨 ��㯯��� � ���塞 䠬����

!! capture=on http_heading=$(get http_heading)
<?xml version='1.0' encoding='CP866'?>
<term>
  <query lang='RU'>
    <CREWCHECKIN>
      <FLIGHT>
        <AIRLINE>6W</AIRLINE>
        <FLT_NO>776</FLT_NO>
        <SUFFIX/>
        <SCD>$(get tomor)</SCD>
        <AIRP_DEP>RTW</AIRP_DEP>
      </FLIGHT>
      <CREW_GROUPS>
        <CREW_GROUP>
          <AIRP_ARV>DME</AIRP_ARV>
          <CREW_MEMBERS>
            <CREW_MEMBER>
                <CREW_TYPE>CR1</CREW_TYPE>
                <DUTY>PIC</DUTY>
                <ORDER>1</ORDER>
                <PERSONAL_DATA>
                    <DOCS>
                        <NO>1234567891</NO>
                        <TYPE>P</TYPE>
                        <ISSUE_COUNTRY>RUS</ISSUE_COUNTRY>
                        <NO>1234567891</NO>
                        <NATIONALITY>RUS</NATIONALITY>
                        <BIRTH_DATE>01.05.1976</BIRTH_DATE>
                        <GENDER>M</GENDER>
                        <EXPIRY_DATE>12.05.$(date_format %Y +1y)</EXPIRY_DATE>
                        <SURNAME>IVASHKIN</SURNAME>
                        <FIRST_NAME>IVAN</FIRST_NAME>
                        <SECOND_NAME>ROMANOVIC</SECOND_NAME>
                    </DOCS>
                    <DOCO>
                        <BIRTH_PLACE>MOSKVA RUSSIY</BIRTH_PLACE>
                        <TYPE>V</TYPE>
                        <NO>VI78787787878</NO>
                        <ISSUE_PLACE>MOSKVA TURISTKAY STRIT 25</ISSUE_PLACE>
                        <ISSUE_DATE>12.02.2014</ISSUE_DATE>
                        <EXPIRY_DATE>15.12.$(date_format %Y +1y)</EXPIRY_DATE>
                        <APPLIC_COUNTRY>USA</APPLIC_COUNTRY>
                    </DOCO>
                    <DOCA>
                        <TYPE>B</TYPE>
                        <COUNTRY>RUS</COUNTRY>
                        <ADDRESS>DUBROVCA CTRIT 25 256</ADDRESS>
                        <CITY>MOSKVA</CITY>
                        <REGION>MOSKVA</REGION>
                        <POSTAL_CODE>125373</POSTAL_CODE>
                    </DOCA>
                    <DOCA>
                        <TYPE>R</TYPE>
                        <COUNTRY>RUS</COUNTRY>
                        <ADDRESS>POPOVCA CTRIT 48</ADDRESS>
                        <CITY>MOSKVA</CITY>
                        <REGION>MOSKVA</REGION>
                        <POSTAL_CODE>266373</POSTAL_CODE>
                    </DOCA>
                    <DOCA>
                        <TYPE>D</TYPE>
                        <COUNTRY>USA</COUNTRY>
                        <ADDRESS>FIFTH AVENUE</ADDRESS>
                        <CITY>NY</CITY>
                        <REGION>NY</REGION>
                        <POSTAL_CODE>9999</POSTAL_CODE>
                    </DOCA>
                </PERSONAL_DATA>
            </CREW_MEMBER>
          </CREW_MEMBERS>
        </CREW_GROUP>
        <CREW_GROUP>
          <AIRP_ARV>DME</AIRP_ARV>
          <CREW_MEMBERS>
            <CREW_MEMBER>
                <CREW_TYPE>CR1</CREW_TYPE>
                <DUTY>PIC</DUTY>
                <ORDER>2</ORDER>
                <PERSONAL_DATA>
                    <DOCS>
                        <NO>7778889991</NO>
                        <TYPE>P</TYPE>
                        <ISSUE_COUNTRY>RUS</ISSUE_COUNTRY>
                        <NO>99988887774</NO>
                        <NATIONALITY>RUS</NATIONALITY>
                        <BIRTH_DATE>02.05.1976</BIRTH_DATE>
                        <GENDER>M</GENDER>
                        <EXPIRY_DATE>22.05.$(date_format %Y +1y)</EXPIRY_DATE>
                        <SURNAME>REPKIN</SURNAME>
                        <FIRST_NAME>DIMA</FIRST_NAME>
                        <SECOND_NAME></SECOND_NAME>
                    </DOCS>
                    <DOCO>
                        <BIRTH_PLACE>MOSKVA RUSSIY</BIRTH_PLACE>
                        <TYPE>V</TYPE>
                        <NO>VI78787787878</NO>
                        <ISSUE_PLACE>MOSKVA POLKA STRIT 25</ISSUE_PLACE>
                        <ISSUE_DATE>15.02.2014</ISSUE_DATE>
                        <EXPIRY_DATE>19.12.$(date_format %Y +1y)</EXPIRY_DATE>
                        <APPLIC_COUNTRY>USA</APPLIC_COUNTRY>
                    </DOCO>
                    <DOCA>
                        <TYPE>B</TYPE>
                        <COUNTRY>RUS</COUNTRY>
                        <ADDRESS>DUBROVCA CTRIT 25 256</ADDRESS>
                        <CITY>MOSKVA</CITY>
                        <REGION>MOSKVA</REGION>
                        <POSTAL_CODE>125373</POSTAL_CODE>
                    </DOCA>
                    <DOCA>
                        <TYPE>R</TYPE>
                        <COUNTRY>RUS</COUNTRY>
                        <ADDRESS>POPOVCA CTRIT 48</ADDRESS>
                        <CITY>MOSKVA</CITY>
                        <REGION>MOSKVA</REGION>
                        <POSTAL_CODE>266373</POSTAL_CODE>
                    </DOCA>
                    <DOCA>
                        <TYPE>D</TYPE>
                        <COUNTRY>USA</COUNTRY>
                        <ADDRESS>FIFTH AVENUE</ADDRESS>
                        <CITY>NY</CITY>
                        <REGION>NY</REGION>
                        <POSTAL_CODE>9999</POSTAL_CODE>
                    </DOCA>
                </PERSONAL_DATA>
            </CREW_MEMBER>
          </CREW_MEMBERS>
        </CREW_GROUP>
      </CREW_GROUPS>
    </CREWCHECKIN>
  </query>
</term>

>> lines=auto
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <CREWCHECKIN>
      <proc_status>OK</proc_status>
    </CREWCHECKIN>
  </answer>
</term>

$(set pax_id1 $(get_pax_id $(get point_dep) -1))
$(set pax_id2 $(get_pax_id $(get point_dep) -2))
$(set grp_id1 $(get_single_grp_id $(get pax_id1)))
$(set grp_id2 $(get_single_grp_id $(get pax_id2)))

??
$(eq $(get grp_id1) $(get grp_id2))
>>
false

$(PAX_LIST_2CREWMEN $(get http_user_id) $(get point_dep) $(get point_arv)
  IVASHKIN $(get grp_id1) $(get pax_id1)
  REPKIN   $(get grp_id2) $(get pax_id2))



