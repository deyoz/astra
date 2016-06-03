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
        <encoding>UTF-16LE</encoding>
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
        <pectab>PT$(sharp)$(sharp)?K1Z$(sharp)@;$(sharp)TICK$(sharp)&gt;&gt;/$(sharp)BOARD$(sharp)0101$(sharp)0250E01W$(sharp)0311H01W$(sharp)0411L01W$(sharp)0508H13W$(sharp)0705H40$(sharp)0906L27W$(sharp)0A05M12R$(sharp)0B01H30W$(sharp)0C06L38W$(sharp)0D16O02W$(sharp)0E21Q01Q52W$(sharp)0F15R01R53W$(sharp)2020C53W$(sharp)2120E53W$(sharp)2220H53W$(sharp)2508O53W$(sharp)2705L53O$(sharp)2B01L67W$(sharp)2C06O64W$(sharp)3FB1R30B601031$(sharp)F104D41A54$(sharp)FF72M01W$(sharp)</pectab>
        <passengers>
          <pax pax_id...
            <prn_form hex='0'>CP$(sharp)1C01$(sharp)01K$(sharp)02$(sharp)02РЕПИН ИВАН                                        $(sharp)03ДОМОДЕДОВО $(sharp)04ПУЛКОВО    $(sharp)05 ЮТ103  $(sharp)xxxx.xx$(sharp)091     $(sharp)0A09:35$(sharp)0BЭ$(sharp)0C    1A$(sharp)0D0KGS$(sharp)0EETKT2981212121212/1  $(sharp)0F7774441110     $(sharp)20РЕПИН ИВАН          $(sharp)21ДОМОДЕДОВО          $(sharp)22ПУЛКОВО             $(sharp)25 ЮТ103  $(sharp)xxxx.xx$(sharp)2BЭ$(sharp)2C  1A  $(sharp)3F000000$(get pax_id)$(sharp)F1001$(sharp)FF                                                                        $(sharp)</prn_form>
          </pax>
        </passengers>
      </printBP>
    </data>
  </answer>
