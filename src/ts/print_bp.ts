include(ts/macro.ts)

# meta: suite print

###
### ����:
### ��� ����� ������ �������� ����������� ��㣨� ����� � Loader'�� ������
### bp_types, bp_models, prn_form_vers, prn_tag_props  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
###

$(init)
$(init_jxt_pult ������)
$(login)
$(init_eds �� UTET UTDC)

$(prepare_bp_printing �� 103 ���)
$(PREPARE_FLIGHT_1 �� 103 ��� ��� ����� ����)

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
ORG+1H:���+++��+Y+::RU+������"
EQN+1:TD"
TKT+2981212121212:T"
CPN+1:CK"
TVL+$(ddmmyy)+���+���+��+103++1"
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

$(set grp_id $(get_single_grp_id $(get point_dep) ����� ����))
$(set tid $(get_single_tid $(get point_dep) ����� ����))


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
            <prn_form hex='0'>CP{#}1C01{#}01K{#}02{#}02����� ����{#}03����������{#}04�������{#}05 ��103{#}07$(date_format %d.%m +0){#}091{#}0A09:50{#}0B�{#}0C    xx{#}0DKGS{#}0EETKT2981212121212/1{#}0F7774441110{#}20����� ����{#}21����������{#}22�������{#}25 ��103{#}27$(date_format %d.%m +0){#}2B�{#}2C  xx{#}3Fxxxxxxxxxx{#}F1001{#}FF{#}</prn_form>
          </pax>
        </passengers>
      </printBP>
    </data>
