include(ts/macro.ts)

# meta: suite checkin

### test 1 - ॣ������ ������ ���ᠦ�� �� ����� ᥣ����
#########################################################################################

$(init)
$(init_jxt_pult ������)
$(login)
$(init_eds �� UTET UTDC)

$(PREPARE_FLIGHT_1 �� 103 ��� ��� ����� ����)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) ����� ���� K))

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
ORG+��:���++++Y+::RU+������"
EQN+1:TD"
TKT+2981212121212:T"
CPN+1:CK"
TVL+$(ddmmyy)+���+���+��+103: ++1"
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


$(KICK_IN)

>> lines=auto
    <segments>
      <segment>
        <tripheader>
          <flight>��103 ���</flight>
          <airline>��</airline>
          <aircode>298</aircode>
          <flt_no>103</flt_no>
          <suffix/>
          <airp>���</airp>
          <scd_out_local>$(date_format %d.%m.%Y) 10:00:00</scd_out_local>
          <pr_etl_only>0</pr_etl_only>
          <pr_etstatus>0</pr_etstatus>
          <pr_no_ticket_check>0</pr_no_ticket_check>
        </tripheader>
        <tripdata>
          <airps>
            <airp>
              <point_id>...
              <airp_code>���</airp_code>
              <city_code>���</city_code>
              <target_view>�����-��������� (���)</target_view>
              <check_doc_info/>
              <check_doco_info/>
              <check_tkn_info/>
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
              <id>...
              <name>��� 1</name>
            </hall>
            <hall>
              <id>...
              <name>��.</name>
            </hall>
            <hall>
              <id>...
              <name>VIP</name>
            </hall>
            <hall>
              <id>...
              <name>���. ����.</name>
            </hall>
            <hall>
              <id>...
              <name>�㯥� ���</name>
            </hall>
          </halls>
          <mark_flights>
            <flight>
              <airline>��</airline>
              <flt_no>103</flt_no>
              <suffix/>
              <scd>$(date_format %d.%m.%Y) 10:00:00</scd>
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
        <bag_types_id>0</bag_types_id>
        <piece_concept>0</piece_concept>
        <tid>...
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
            <pax_id>...
            <surname>�����</surname>
            <name>����</name>
            <pers_type>��</pers_type>
            <crew_type/>
            <seat_no>1A</seat_no>
            <seat_type/>
            <seats>1</seats>
            <refuse/>
            <reg_no>1</reg_no>
            <subclass>�</subclass>
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
### test 2 - ॣ������ ��㯯� �� ���� ���ᠦ�஢ �� ����� ᥣ����
#########################################################################################

$(init)
$(init_jxt_pult ������)
$(login)
$(init_eds �� UTET UTDC)

$(PREPARE_FLIGHT_4 �� 103 ��� ��� ����� ������� ������ ����)

$(dump_table CRS_PAX)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(set pax1_id $(get_single_pax_id $(get point_dep) ����� ������� K))
$(set pax2_id $(get_single_pax_id $(get point_dep) ������ ���� K))

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
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref 1)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref 1)"
MSG+:142"
ORG+��:���++++Y+::RU+������"
EQN+1:TD"
TKT+2981212121212:T"
CPN+1:CK"
TVL+$(ddmmyy)+���+���+��+103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref 1)0001"

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+��:���++++Y+::RU+������"
EQN+1:TD"
TKT+2981212121213:T"
CPN+1:CK"
TVL+$(ddmmyy)+���+���+��+103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"


<<
UNB+SIRE:1+UTET+UTDC+160408:0828+$(last_edifact_ref 1)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref 1)"
MSG+:142+3"
EQN+2:TD"
TKT+2981212121212:T::3"
CPN+1:CK::E"
UNT+6+1"
UNZ+1+$(last_edifact_ref 1)0001"

<<
UNB+SIRE:1+UTET+UTDC+160408:0828+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref)"
MSG+:142+3"
EQN+2:TD"
TKT+2981212121213:T::3"
CPN+1:CK::E"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"


$(KICK_IN)

>> lines=auto
    <segments>
      <segment>
        <tripheader>
          <flight>��103 ���</flight>
          <airline>��</airline>
          <aircode>298</aircode>
          <flt_no>103</flt_no>
          <suffix/>
          <airp>���</airp>
          <scd_out_local>$(date_format %d.%m.%Y) 10:00:00</scd_out_local>
          <pr_etl_only>0</pr_etl_only>
          <pr_etstatus>0</pr_etstatus>
          <pr_no_ticket_check>0</pr_no_ticket_check>
        </tripheader>
        <tripdata>
          <airps>
            <airp>
              <point_id>...
              <airp_code>���</airp_code>
              <city_code>���</city_code>
              <target_view>�����-��������� (���)</target_view>
              <check_doc_info/>
              <check_doco_info/>
              <check_tkn_info/>
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
              <scd>$(date_format %d.%m.%Y) 10:00:00</scd>
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
        <bag_types_id>0</bag_types_id>
        <piece_concept>0</piece_concept>
        <tid>...
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
            <seat_no>1B</seat_no>
            <seat_type/>
            <seats>1</seats>
            <refuse/>
            <reg_no>2</reg_no>
            <subclass>�</subclass>
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
### test 2 - ᪢����� ॣ����樨 ������ ���ᠦ��
#########################################################################################


# TODO
